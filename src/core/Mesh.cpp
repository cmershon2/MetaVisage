#include "core/Mesh.h"
#include <algorithm>
#include <cmath>
#ifdef HAVE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif
#include <QDebug>
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
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qWarning() << "Assimp error:" << importer.GetErrorString();
        return false;
    }

    // For simplicity, load only the first mesh
    if (scene->mNumMeshes == 0) {
        qWarning() << "No meshes found in file:" << filepath;
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

    qDebug() << "Loaded mesh:" << name_;
    qDebug() << "  Vertices:" << vertices_.size();
    qDebug() << "  Faces:" << faces_.size();
    qDebug() << "  Normals:" << normals_.size();
    qDebug() << "  UVs:" << uvs_.size();
    qDebug() << "  Materials:" << materials_.size();

    return Validate();
#else
    qWarning() << "Assimp not available - cannot load mesh:" << filepath;
    qWarning() << "To enable mesh loading, install Assimp: vcpkg install assimp:x64-mingw-dynamic";
    return false;
#endif
}

bool Mesh::Save(const QString& filepath) {
    // TODO: Implement mesh saving in Sprint 10
    return false;
}

void Mesh::SetVertices(const std::vector<Vector3>& vertices) {
    vertices_ = vertices;
    CalculateBounds();
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
        return false;
    }

    // Check if we have faces
    if (faces_.empty()) {
        return false;
    }

    // Check if face indices are valid
    for (const auto& face : faces_) {
        for (unsigned int idx : face.vertexIndices) {
            if (idx >= vertices_.size()) {
                return false;
            }
        }
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

} // namespace MetaVisage
