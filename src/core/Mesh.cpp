#include "core/Mesh.h"
#include "utils/BVH.h"
#include "utils/SpatialHash.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>
#include <unordered_map>
#ifdef HAVE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif
#include <QFileInfo>

namespace MetaVisage {

Mesh::Mesh() : name_("Untitled"), filepath_("") {
}

Mesh::~Mesh() {
}

bool Mesh::Load(const QString& filepath) {
    Clear();
    filepath_ = filepath;

#ifdef HAVE_ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filepath.toStdString(),
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        MV_LOG_ERROR(QString("Assimp error loading '%1': %2").arg(filepath, importer.GetErrorString()));
        return false;
    }

    // For simplicity, load only the first mesh
    if (scene->mNumMeshes == 0) {
        MV_LOG_WARNING(QString("No meshes found in file: %1").arg(filepath));
        return false;
    }

    aiMesh* mesh = scene->mMeshes[0];
    name_ = QString::fromStdString(mesh->mName.C_Str());
    if (name_.isEmpty()) {
        QFileInfo fileInfo(filepath);
        name_ = fileInfo.baseName();
    }

    // Load vertices
    vertices_.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D vertex = mesh->mVertices[i];
        vertices_.push_back(Vector3(vertex.x, vertex.y, vertex.z));
    }

    // Load normals
    if (mesh->HasNormals()) {
        normals_.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D normal = mesh->mNormals[i];
            normals_.push_back(Vector3(normal.x, normal.y, normal.z));
        }
    }

    // Load UVs (first texture coordinate set)
    if (mesh->HasTextureCoords(0)) {
        uvs_.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D uv = mesh->mTextureCoords[0][i];
            uvs_.push_back(Vector2(uv.x, uv.y));
        }
    }

    // Load faces
    faces_.reserve(mesh->mNumFaces);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        Face meshFace;
        meshFace.vertexIndices.reserve(face.mNumIndices);
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            meshFace.vertexIndices.push_back(face.mIndices[j]);
        }
        meshFace.materialIndex = mesh->mMaterialIndex;
        faces_.push_back(meshFace);
    }

    // Load materials
    if (scene->HasMaterials()) {
        materials_.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* mat = scene->mMaterials[i];
            Material material;

            aiString name;
            if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
                material.name = name.C_Str();
            }

            aiColor3D diffuse(0.8f, 0.8f, 0.8f);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            material.diffuseColor = Vector3(diffuse.r, diffuse.g, diffuse.b);

            aiColor3D specular(1.0f, 1.0f, 1.0f);
            mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
            material.specularColor = Vector3(specular.r, specular.g, specular.b);

            float shininess = 32.0f;
            mat->Get(AI_MATKEY_SHININESS, shininess);
            material.shininess = shininess;

            materials_.push_back(material);
        }
    }

    CalculateBounds();

    MV_LOG_INFO(QString("Loaded mesh '%1': %2 vertices, %3 faces, %4 normals, %5 UVs, %6 materials")
        .arg(name_).arg(vertices_.size()).arg(faces_.size())
        .arg(normals_.size()).arg(uvs_.size()).arg(materials_.size()));

    // --- Post-import: Merge vertices split at UV seams ---
    // Assimp creates separate vertices for each unique (position, normal, UV) combination.
    // MetaHuman requires the original vertex count (positions only), so we merge vertices
    // that share the same position and store UVs with separate per-face-corner indices.
    if (!vertices_.empty() && !uvs_.empty()) {
        const float epsilon = 1e-5f;
        const float cellSize = epsilon * 10.0f;
        const float invCell = 1.0f / cellSize;
        const float epsSq = epsilon * epsilon;

        struct CellHash {
            size_t operator()(const std::tuple<int,int,int>& k) const {
                return std::hash<int>{}(std::get<0>(k)) ^
                       (std::hash<int>{}(std::get<1>(k)) * 2654435761u) ^
                       (std::hash<int>{}(std::get<2>(k)) * 40503u);
            }
        };

        std::unordered_map<std::tuple<int,int,int>, std::vector<int>, CellHash> grid;
        int vCount = static_cast<int>(vertices_.size());

        for (int i = 0; i < vCount; ++i) {
            const Vector3& v = vertices_[i];
            grid[{static_cast<int>(std::floor(v.x * invCell)),
                  static_cast<int>(std::floor(v.y * invCell)),
                  static_cast<int>(std::floor(v.z * invCell))}].push_back(i);
        }

        // For each vertex, find coincident vertices and map them to the canonical (lowest index)
        std::vector<int> canonicalMap(vCount);
        std::iota(canonicalMap.begin(), canonicalMap.end(), 0);
        std::vector<bool> visited(vCount, false);
        int mergeCount = 0;

        for (int i = 0; i < vCount; ++i) {
            if (visited[i]) continue;
            visited[i] = true;

            const Vector3& vi = vertices_[i];
            int cx = static_cast<int>(std::floor(vi.x * invCell));
            int cy = static_cast<int>(std::floor(vi.y * invCell));
            int cz = static_cast<int>(std::floor(vi.z * invCell));

            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dz = -1; dz <= 1; ++dz) {
                        auto it = grid.find({cx + dx, cy + dy, cz + dz});
                        if (it == grid.end()) continue;
                        for (int j : it->second) {
                            if (j <= i || visited[j]) continue;
                            Vector3 diff = vertices_[j] - vi;
                            if (diff.Dot(diff) < epsSq) {
                                canonicalMap[j] = i;
                                visited[j] = true;
                                mergeCount++;
                            }
                        }
                    }
                }
            }
        }

        if (mergeCount > 0) {
            // Build compaction map: old index → new sequential index
            std::vector<int> compactMap(vCount, -1);
            std::vector<Vector3> mergedVertices;
            std::vector<Vector3> mergedNormals;

            for (int i = 0; i < vCount; ++i) {
                if (canonicalMap[i] == i) {
                    compactMap[i] = static_cast<int>(mergedVertices.size());
                    mergedVertices.push_back(vertices_[i]);
                    if (i < static_cast<int>(normals_.size())) {
                        mergedNormals.push_back(normals_[i]);
                    }
                }
            }

            for (int i = 0; i < vCount; ++i) {
                if (canonicalMap[i] != i) {
                    compactMap[i] = compactMap[canonicalMap[i]];
                }
            }

            // Update faces: save original indices as uvIndices, remap vertexIndices
            for (auto& face : faces_) {
                face.uvIndices = face.vertexIndices; // Preserve for UV lookup
                for (auto& idx : face.vertexIndices) {
                    idx = static_cast<unsigned int>(compactMap[idx]);
                }
            }

            MV_LOG_INFO(QString("Mesh '%1': Merged %2 UV-seam vertices (%3 -> %4 vertices)")
                .arg(name_).arg(mergeCount).arg(vCount).arg(mergedVertices.size()));

            // Store mapping data for MetaHuman-compatible export (must be done before overwriting normals_)
            originalAssimpVertexCount_ = static_cast<size_t>(vCount);
            assimpToMergedMap_.resize(vCount);
            for (int i = 0; i < vCount; ++i) {
                assimpToMergedMap_[i] = compactMap[i];
            }
            originalAssimpNormals_ = normals_;  // Save original N normals before merge overwrites them

            vertices_ = std::move(mergedVertices);
            normals_ = std::move(mergedNormals);
            // uvs_ stays at original size, indexed by face uvIndices
        }
    }

    CalculateBounds();

    return Validate();
#else
    MV_LOG_WARNING(QString("Assimp not available - cannot load mesh: %1").arg(filepath));
    return false;
#endif
}

bool Mesh::Save(const QString& filepath) {
#ifdef HAVE_ASSIMP
    Assimp::Exporter exporter;

    aiScene scene;
    scene.mRootNode = new aiNode();
    scene.mRootNode->mName = aiString("root");

    scene.mNumMeshes = 1;
    scene.mMeshes = new aiMesh*[1];
    scene.mMeshes[0] = new aiMesh();
    scene.mRootNode->mNumMeshes = 1;
    scene.mRootNode->mMeshes = new unsigned int[1];
    scene.mRootNode->mMeshes[0] = 0;

    aiMesh* outMesh = scene.mMeshes[0];
    outMesh->mName = aiString(name_.toStdString());

    outMesh->mNumVertices = static_cast<unsigned int>(vertices_.size());
    outMesh->mVertices = new aiVector3D[outMesh->mNumVertices];
    for (unsigned int i = 0; i < outMesh->mNumVertices; ++i) {
        outMesh->mVertices[i] = aiVector3D(vertices_[i].x, vertices_[i].y, vertices_[i].z);
    }

    if (!normals_.empty()) {
        outMesh->mNormals = new aiVector3D[outMesh->mNumVertices];
        for (unsigned int i = 0; i < outMesh->mNumVertices && i < static_cast<unsigned int>(normals_.size()); ++i) {
            outMesh->mNormals[i] = aiVector3D(normals_[i].x, normals_[i].y, normals_[i].z);
        }
    }

    if (!uvs_.empty()) {
        outMesh->mTextureCoords[0] = new aiVector3D[outMesh->mNumVertices];
        outMesh->mNumUVComponents[0] = 2;
        for (unsigned int i = 0; i < outMesh->mNumVertices && i < static_cast<unsigned int>(uvs_.size()); ++i) {
            outMesh->mTextureCoords[0][i] = aiVector3D(uvs_[i].x, uvs_[i].y, 0.0f);
        }
    }

    outMesh->mNumFaces = static_cast<unsigned int>(faces_.size());
    outMesh->mFaces = new aiFace[outMesh->mNumFaces];
    for (unsigned int i = 0; i < outMesh->mNumFaces; ++i) {
        aiFace& dstFace = outMesh->mFaces[i];
        dstFace.mNumIndices = static_cast<unsigned int>(faces_[i].vertexIndices.size());
        dstFace.mIndices = new unsigned int[dstFace.mNumIndices];
        for (unsigned int j = 0; j < dstFace.mNumIndices; ++j) {
            dstFace.mIndices[j] = faces_[i].vertexIndices[j];
        }
    }

    scene.mNumMaterials = 1;
    scene.mMaterials = new aiMaterial*[1];
    scene.mMaterials[0] = new aiMaterial();
    outMesh->mMaterialIndex = 0;

    std::string formatId = "fbx";
    if (filepath.endsWith(".obj", Qt::CaseInsensitive)) formatId = "obj";
    else if (filepath.endsWith(".gltf", Qt::CaseInsensitive)) formatId = "gltf2";

    aiReturn ret = exporter.Export(&scene, formatId, filepath.toStdString());
    if (ret != aiReturn_SUCCESS) {
        MV_LOG_ERROR(QString("Mesh save failed: %1").arg(exporter.GetErrorString()));
        return false;
    }

    MV_LOG_INFO(QString("Mesh saved to: %1").arg(filepath));
    return true;
#else
    Q_UNUSED(filepath);
    MV_LOG_WARNING("Assimp not available - cannot save mesh");
    return false;
#endif
}

void Mesh::SetVertices(const std::vector<Vector3>& vertices) {
    vertices_ = vertices;
    CalculateBounds();
    InvalidateAccelerationStructures();
}

void Mesh::SetNormals(const std::vector<Vector3>& normals) {
    normals_ = normals;
}

void Mesh::SetUVs(const std::vector<Vector2>& uvs) {
    uvs_ = uvs;
}

void Mesh::SetFaces(const std::vector<Face>& faces) {
    faces_ = faces;
}

void Mesh::SetMaterials(const std::vector<Material>& materials) {
    materials_ = materials;
}

void Mesh::SetAssimpMapping(const std::vector<int>& map, const std::vector<Vector3>& normals, size_t originalCount) {
    assimpToMergedMap_ = map;
    originalAssimpNormals_ = normals;
    originalAssimpVertexCount_ = originalCount;
}

void Mesh::CalculateNormals() {
    // Clear existing normals
    normals_.clear();
    normals_.resize(vertices_.size(), Vector3(0.0f, 0.0f, 0.0f));

    // Calculate face normals and accumulate
    for (const auto& face : faces_) {
        if (face.vertexIndices.size() < 3) continue;

        // Get first three vertices to calculate face normal
        unsigned int i0 = face.vertexIndices[0];
        unsigned int i1 = face.vertexIndices[1];
        unsigned int i2 = face.vertexIndices[2];

        if (i0 >= vertices_.size() || i1 >= vertices_.size() || i2 >= vertices_.size()) {
            continue;
        }

        Vector3 v0 = vertices_[i0];
        Vector3 v1 = vertices_[i1];
        Vector3 v2 = vertices_[i2];

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 faceNormal = edge1.Cross(edge2).Normalized();

        // Accumulate to vertex normals
        for (unsigned int idx : face.vertexIndices) {
            if (idx < normals_.size()) {
                normals_[idx] = normals_[idx] + faceNormal;
            }
        }
    }

    // Normalize all vertex normals
    for (auto& normal : normals_) {
        normal = normal.Normalized();
    }
}

void Mesh::CalculateBounds() {
    if (vertices_.empty()) {
        bounds_.min = Vector3(0.0f, 0.0f, 0.0f);
        bounds_.max = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    bounds_.min = vertices_[0];
    bounds_.max = vertices_[0];

    for (const auto& vertex : vertices_) {
        bounds_.min.x = std::min(bounds_.min.x, vertex.x);
        bounds_.min.y = std::min(bounds_.min.y, vertex.y);
        bounds_.min.z = std::min(bounds_.min.z, vertex.z);

        bounds_.max.x = std::max(bounds_.max.x, vertex.x);
        bounds_.max.y = std::max(bounds_.max.y, vertex.y);
        bounds_.max.z = std::max(bounds_.max.z, vertex.z);
    }
}

bool Mesh::Validate() const {
    // Check if we have vertices
    if (vertices_.empty()) {
        MV_LOG_WARNING("Mesh validation failed: no vertices");
        return false;
    }

    // Check if we have faces
    if (faces_.empty()) {
        MV_LOG_WARNING("Mesh validation failed: no faces");
        return false;
    }

    // Check if face indices are valid
    for (size_t fi = 0; fi < faces_.size(); ++fi) {
        const auto& face = faces_[fi];
        if (face.vertexIndices.size() < 3) {
            MV_LOG_WARNING(QString("Mesh validation: face %1 has fewer than 3 vertices").arg(fi));
            return false;
        }
        for (unsigned int idx : face.vertexIndices) {
            if (idx >= vertices_.size()) {
                MV_LOG_WARNING(QString("Mesh validation: face %1 has out-of-bounds vertex index %2 (max %3)")
                    .arg(fi).arg(idx).arg(vertices_.size() - 1));
                return false;
            }
        }
    }

    // Check normals match vertices if present
    if (!normals_.empty() && normals_.size() != vertices_.size()) {
        MV_LOG_WARNING(QString("Mesh validation: normal count (%1) doesn't match vertex count (%2)")
            .arg(normals_.size()).arg(vertices_.size()));
        return false;
    }

    return true;
}

void Mesh::Clear() {
    vertices_.clear();
    normals_.clear();
    uvs_.clear();
    faces_.clear();
    materials_.clear();
    bounds_ = BoundingBox();
    assimpToMergedMap_.clear();
    originalAssimpNormals_.clear();
    originalAssimpVertexCount_ = 0;
}

size_t Mesh::GetTriangleCount() const {
    size_t count = 0;
    for (const auto& face : faces_) {
        if (face.vertexIndices.size() >= 3) {
            count += face.vertexIndices.size() - 2;
        }
    }
    return count;
}

BVH* Mesh::GetBVH() const {
    if (!bvh_) {
        bvh_ = std::make_unique<BVH>();
        bvh_->Build(vertices_, faces_);
    }
    return bvh_.get();
}

SpatialHash* Mesh::GetSpatialHash(float cellSize) const {
    if (!spatialHash_ || std::abs(spatialHashCellSize_ - cellSize) > 1e-6f) {
        spatialHash_ = std::make_unique<SpatialHash>();
        spatialHash_->Build(vertices_, cellSize);
        spatialHashCellSize_ = cellSize;
    }
    return spatialHash_.get();
}

void Mesh::InvalidateAccelerationStructures() {
    bvh_.reset();
    spatialHash_.reset();
    spatialHashCellSize_ = 0.0f;
}

} // namespace MetaVisage
