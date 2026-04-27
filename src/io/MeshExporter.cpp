#include "io/MeshExporter.h"
#include "utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef HAVE_ASSIMP
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

namespace MetaVisage {

namespace {

bool EndsWithICase(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - suffix.size() + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

// Split on whitespace, skipping empty tokens. Mirrors QString::split(' ', SkipEmptyParts).
std::vector<std::string> SplitWhitespace(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) out.push_back(std::move(tok));
    return out;
}

std::string Trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

bool StartsWith(const std::string& s, const char* prefix) {
    const size_t n = std::char_traits<char>::length(prefix);
    if (s.size() < n) return false;
    return std::equal(prefix, prefix + n, s.begin());
}

void WriteFloat(std::ostream& out, float value) {
    out.precision(8);
    out << value;
}

} // namespace

MeshExporter::MeshExporter() {
}

MeshExporter::~MeshExporter() {
}

void MeshExporter::ReportProgress(float progress, const std::string& message) {
    if (progressCallback_) {
        progressCallback_(progress, message);
    }
}

std::string MeshExporter::GetFormatExtension(ExportFormat format) {
    switch (format) {
        case ExportFormat::FBX: return ".fbx";
        case ExportFormat::OBJ: return ".obj";
        case ExportFormat::GLTF: return ".gltf";
    }
    return ".fbx";
}

std::string MeshExporter::GetFormatFilter(ExportFormat format) {
    switch (format) {
        case ExportFormat::FBX: return "FBX (*.fbx)";
        case ExportFormat::OBJ: return "OBJ (*.obj)";
        case ExportFormat::GLTF: return "GLTF (*.gltf)";
    }
    return "FBX (*.fbx)";
}

std::string MeshExporter::GetAllFormatsFilter() {
    return "FBX (*.fbx);;OBJ (*.obj);;GLTF (*.gltf);;All Files (*.*)";
}

ExportResult MeshExporter::Export(const Mesh& mesh, const std::string& filepath,
                                   const ExportOptions& options,
                                   const Transform* transform,
                                   const Mesh* originalMorphMesh) {
    ExportResult result;

    if (mesh.GetVertexCount() == 0) {
        result.errorMessage = "Mesh has no vertices to export.";
        return result;
    }

    if (filepath.empty()) {
        result.errorMessage = "No filepath specified.";
        return result;
    }

    ReportProgress(0.0f, "Starting export...");

    // MetaHuman-compatible export: rewrite the original file with deformed positions
    if (options.metaHumanCompatible) {
        // Prefer OBJ rewrite if original morph mesh is available and was loaded from OBJ
        if (originalMorphMesh) {
            const std::string& origPath = originalMorphMesh->GetFilePath();
            if (!origPath.empty() && EndsWithICase(origPath, ".obj")) {
                result = ExportOBJMetaHuman(mesh, *originalMorphMesh, filepath, options, transform);
                return result;
            }
        }

        // Fallback: Assimp-based MetaHuman export (for FBX source meshes)
        if (mesh.HasAssimpMapping()) {
#ifdef HAVE_ASSIMP
            result = ExportFBXMetaHuman(mesh, filepath, options, transform);
#else
            result.errorMessage = "Assimp not available. Cannot export MetaHuman-compatible mesh.";
#endif
            return result;
        }
    }

    // Use direct OBJ writer when mesh has merged UV-seam vertices (preserves topology)
    if (options.format == ExportFormat::OBJ && mesh.HasSeparateUVIndices()) {
        result = ExportOBJDirect(mesh, filepath, options, transform);
    } else {
#ifdef HAVE_ASSIMP
        result = ExportWithAssimp(mesh, filepath, options, transform);
#else
        result.errorMessage = "Assimp not available. Cannot export mesh.";
#endif
    }

    return result;
}

ExportResult MeshExporter::ExportOBJDirect(const Mesh& mesh, const std::string& filepath,
                                             const ExportOptions& options,
                                             const Transform* transform) {
    ExportResult result;
    ReportProgress(0.1f, "Preparing OBJ export with preserved topology...");

    const auto& vertices = mesh.GetVertices();
    const auto& normals = mesh.GetNormals();
    const auto& uvs = mesh.GetUVs();
    const auto& faces = mesh.GetFaces();

    Matrix4x4 transformMatrix;
    bool hasTransform = options.applyTransform && transform;
    if (hasTransform) {
        transformMatrix = transform->GetMatrix();
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        result.errorMessage = std::format("Cannot open file for writing: {}", filepath);
        return result;
    }
    out.precision(8);

    out << "# MetaVisage OBJ Export\n";
    out << "# Vertices: " << vertices.size() << "\n";
    out << "# UVs: " << uvs.size() << "\n";
    out << "# Normals: " << normals.size() << "\n";
    out << "# Faces: " << faces.size() << "\n\n";

    ReportProgress(0.2f, "Writing vertices...");

    // Write positions (v)
    for (size_t i = 0; i < vertices.size(); ++i) {
        Vector3 v = vertices[i];

        if (hasTransform) {
            const float* m = transformMatrix.Data();
            float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
            float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
            float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
            v = Vector3(x, y, z);
        }

        if (std::abs(options.scaleFactor - 1.0f) > 0.0001f) {
            v = v * options.scaleFactor;
        }

        out << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    ReportProgress(0.4f, "Writing UVs...");

    // Write texture coordinates (vt) - uses separate UV array
    for (size_t i = 0; i < uvs.size(); ++i) {
        out << "vt " << uvs[i].x << " " << uvs[i].y << "\n";
    }

    ReportProgress(0.5f, "Writing normals...");

    // Write normals (vn)
    for (size_t i = 0; i < normals.size(); ++i) {
        Vector3 n = normals[i];

        if (hasTransform) {
            const float* m = transformMatrix.Data();
            float x = m[0] * n.x + m[4] * n.y + m[8] * n.z;
            float y = m[1] * n.x + m[5] * n.y + m[9] * n.z;
            float z = m[2] * n.x + m[6] * n.y + m[10] * n.z;
            n = Vector3(x, y, z).Normalized();
        }

        out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
    }

    ReportProgress(0.6f, "Writing faces...");

    // Write faces with separate v/vt/vn indices (OBJ indices are 1-based)
    for (size_t fi = 0; fi < faces.size(); ++fi) {
        const auto& face = faces[fi];
        if (face.vertexIndices.empty()) continue;

        out << "f";
        for (size_t j = 0; j < face.vertexIndices.size(); ++j) {
            unsigned int vi = face.vertexIndices[j] + 1;  // 1-based

            // UV index from separate uvIndices
            unsigned int ti = (j < face.uvIndices.size()) ? (face.uvIndices[j] + 1) : vi;

            // Normal index same as vertex index (normals are merged with positions)
            unsigned int ni = vi;

            out << " " << vi << "/" << ti << "/" << ni;
        }
        out << "\n";
    }

    out.close();

    ReportProgress(1.0f, "Export complete!");

    result.success = true;
    result.exportedFilePath = filepath;
    result.vertexCount = static_cast<int>(vertices.size());
    result.faceCount = static_cast<int>(faces.size());

    MV_LOG_DEBUG(std::format("Exported OBJ (direct) to: {}", filepath));
    MV_LOG_DEBUG(std::format("  Vertices: {}  UVs: {}  Faces: {}",
                              result.vertexCount, uvs.size(), result.faceCount));

    return result;
}

#ifdef HAVE_ASSIMP

ExportResult MeshExporter::ExportWithAssimp(const Mesh& mesh, const std::string& filepath,
                                              const ExportOptions& options,
                                              const Transform* transform) {
    ExportResult result;
    ReportProgress(0.1f, "Preparing mesh data...");

    // Build an aiScene from our Mesh data
    aiScene scene;
    scene.mRootNode = new aiNode();
    scene.mRootNode->mName = aiString("root");

    // Create one mesh
    scene.mNumMeshes = 1;
    scene.mMeshes = new aiMesh*[1];
    scene.mMeshes[0] = new aiMesh();

    // Link mesh to root node
    scene.mRootNode->mNumMeshes = 1;
    scene.mRootNode->mMeshes = new unsigned int[1];
    scene.mRootNode->mMeshes[0] = 0;

    aiMesh* outMesh = scene.mMeshes[0];

    // Set mesh name with optional UE prefix
    std::string meshName = mesh.GetName();
    if (!options.ueNamingPrefix.empty()) {
        meshName = options.ueNamingPrefix + meshName;
    }
    outMesh->mName = aiString(meshName);

    ReportProgress(0.2f, "Writing vertices...");

    // Get source data
    const auto& vertices = mesh.GetVertices();
    const auto& normals = mesh.GetNormals();
    const auto& uvs = mesh.GetUVs();
    const auto& faces = mesh.GetFaces();
    bool hasSeparateUVs = mesh.HasSeparateUVIndices();

    // Prepare transform matrix if needed
    Matrix4x4 transformMatrix;
    bool hasTransform = options.applyTransform && transform;
    if (hasTransform) {
        transformMatrix = transform->GetMatrix();
    }

    // If mesh has separate UV indices, Assimp needs unified vertices.
    // Re-split at UV seams by creating one vertex per unique (vertexIdx, uvIdx) pair.
    std::vector<unsigned int> splitVertexMap;   // GPU idx → mesh vertex idx
    std::vector<unsigned int> splitUVMap;       // GPU idx → mesh UV idx
    std::vector<std::vector<unsigned int>> remappedFaceIndices; // per-face, remapped indices

    if (hasSeparateUVs) {
        std::map<std::pair<unsigned int, unsigned int>, unsigned int> cornerMap;

        for (size_t fi = 0; fi < faces.size(); ++fi) {
            const auto& face = faces[fi];
            std::vector<unsigned int> newIndices;

            for (size_t j = 0; j < face.vertexIndices.size(); ++j) {
                unsigned int vi = face.vertexIndices[j];
                unsigned int ti = (j < face.uvIndices.size()) ? face.uvIndices[j] : vi;
                auto key = std::make_pair(vi, ti);

                auto it = cornerMap.find(key);
                if (it != cornerMap.end()) {
                    newIndices.push_back(it->second);
                } else {
                    unsigned int newIdx = static_cast<unsigned int>(splitVertexMap.size());
                    cornerMap[key] = newIdx;
                    splitVertexMap.push_back(vi);
                    splitUVMap.push_back(ti);
                    newIndices.push_back(newIdx);
                }
            }

            remappedFaceIndices.push_back(std::move(newIndices));
        }
    }

    unsigned int numVerts = hasSeparateUVs
        ? static_cast<unsigned int>(splitVertexMap.size())
        : static_cast<unsigned int>(vertices.size());

    // Write vertices
    outMesh->mNumVertices = numVerts;
    outMesh->mVertices = new aiVector3D[numVerts];

    for (unsigned int i = 0; i < numVerts; ++i) {
        unsigned int srcIdx = hasSeparateUVs ? splitVertexMap[i] : i;
        Vector3 v = vertices[srcIdx];

        if (hasTransform) {
            const float* m = transformMatrix.Data();
            float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
            float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
            float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
            v = Vector3(x, y, z);
        }

        if (std::abs(options.scaleFactor - 1.0f) > 0.0001f) {
            v = v * options.scaleFactor;
        }

        outMesh->mVertices[i] = aiVector3D(v.x, v.y, v.z);
    }

    ReportProgress(0.4f, "Writing normals...");

    // Write normals
    if (!normals.empty()) {
        outMesh->mNormals = new aiVector3D[numVerts];
        for (unsigned int i = 0; i < numVerts; ++i) {
            unsigned int srcIdx = hasSeparateUVs ? splitVertexMap[i] : i;
            if (srcIdx >= normals.size()) continue;
            Vector3 n = normals[srcIdx];

            if (hasTransform) {
                const float* m = transformMatrix.Data();
                float x = m[0] * n.x + m[4] * n.y + m[8] * n.z;
                float y = m[1] * n.x + m[5] * n.y + m[9] * n.z;
                float z = m[2] * n.x + m[6] * n.y + m[10] * n.z;
                n = Vector3(x, y, z).Normalized();
            }

            outMesh->mNormals[i] = aiVector3D(n.x, n.y, n.z);
        }
    }

    ReportProgress(0.5f, "Writing UVs...");

    // Write UVs
    if (!uvs.empty()) {
        outMesh->mTextureCoords[0] = new aiVector3D[numVerts];
        outMesh->mNumUVComponents[0] = 2;
        for (unsigned int i = 0; i < numVerts; ++i) {
            unsigned int uvIdx = hasSeparateUVs ? splitUVMap[i] : i;
            if (uvIdx < uvs.size()) {
                outMesh->mTextureCoords[0][i] = aiVector3D(uvs[uvIdx].x, uvs[uvIdx].y, 0.0f);
            } else {
                outMesh->mTextureCoords[0][i] = aiVector3D(0.0f, 0.0f, 0.0f);
            }
        }
    }

    ReportProgress(0.6f, "Writing faces...");

    // Write faces
    outMesh->mNumFaces = static_cast<unsigned int>(faces.size());
    outMesh->mFaces = new aiFace[outMesh->mNumFaces];

    for (unsigned int i = 0; i < outMesh->mNumFaces; ++i) {
        const auto& srcIndices = hasSeparateUVs ? remappedFaceIndices[i] : faces[i].vertexIndices;
        aiFace& dstFace = outMesh->mFaces[i];
        dstFace.mNumIndices = static_cast<unsigned int>(srcIndices.size());
        dstFace.mIndices = new unsigned int[dstFace.mNumIndices];
        for (unsigned int j = 0; j < dstFace.mNumIndices; ++j) {
            dstFace.mIndices[j] = srcIndices[j];
        }
    }

    ReportProgress(0.7f, "Writing materials...");

    // Write materials
    if (options.includeMaterials) {
        const auto& materials = mesh.GetMaterials();
        if (!materials.empty()) {
            scene.mNumMaterials = static_cast<unsigned int>(materials.size());
            scene.mMaterials = new aiMaterial*[scene.mNumMaterials];

            for (unsigned int i = 0; i < scene.mNumMaterials; ++i) {
                scene.mMaterials[i] = new aiMaterial();
                const Material& srcMat = materials[i];

                aiString matName(srcMat.name);
                scene.mMaterials[i]->AddProperty(&matName, AI_MATKEY_NAME);

                aiColor3D diffuse(srcMat.diffuseColor.x, srcMat.diffuseColor.y, srcMat.diffuseColor.z);
                scene.mMaterials[i]->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

                aiColor3D specular(srcMat.specularColor.x, srcMat.specularColor.y, srcMat.specularColor.z);
                scene.mMaterials[i]->AddProperty(&specular, 1, AI_MATKEY_COLOR_SPECULAR);

                scene.mMaterials[i]->AddProperty(&srcMat.shininess, 1, AI_MATKEY_SHININESS);
            }

            outMesh->mMaterialIndex = 0;
        } else {
            // Add a default material
            scene.mNumMaterials = 1;
            scene.mMaterials = new aiMaterial*[1];
            scene.mMaterials[0] = new aiMaterial();

            aiString matName("DefaultMaterial");
            scene.mMaterials[0]->AddProperty(&matName, AI_MATKEY_NAME);

            aiColor3D diffuse(0.8f, 0.8f, 0.8f);
            scene.mMaterials[0]->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

            outMesh->mMaterialIndex = 0;
        }
    } else {
        // Still need at least one material for valid scene
        scene.mNumMaterials = 1;
        scene.mMaterials = new aiMaterial*[1];
        scene.mMaterials[0] = new aiMaterial();

        aiString matName("DefaultMaterial");
        scene.mMaterials[0]->AddProperty(&matName, AI_MATKEY_NAME);
        outMesh->mMaterialIndex = 0;
    }

    ReportProgress(0.8f, "Exporting file...");

    // Determine export format ID for Assimp
    std::string formatId;
    switch (options.format) {
        case ExportFormat::FBX:
            formatId = "fbx";
            break;
        case ExportFormat::OBJ:
            formatId = "obj";
            break;
        case ExportFormat::GLTF:
            formatId = "gltf2";
            break;
    }

    // Set up post-processing flags
    unsigned int postProcessFlags = 0;
    if (options.triangulate) {
        postProcessFlags |= aiProcess_Triangulate;
    }

    // Export using Assimp
    Assimp::Exporter exporter;
    aiReturn exportResult = exporter.Export(&scene, formatId, filepath, postProcessFlags);

    if (exportResult != aiReturn_SUCCESS) {
        result.errorMessage = std::format("Export failed: {}", exporter.GetErrorString());
        MV_LOG_WARNING(result.errorMessage);
        return result;
    }

    ReportProgress(1.0f, "Export complete!");

    result.success = true;
    result.exportedFilePath = filepath;
    result.vertexCount = static_cast<int>(vertices.size());
    result.faceCount = static_cast<int>(faces.size());

    MV_LOG_DEBUG(std::format("Exported mesh to: {} (format={})", filepath, formatId));
    MV_LOG_DEBUG(std::format("  Vertices: {}  Faces: {}", result.vertexCount, result.faceCount));

    return result;
}

ExportResult MeshExporter::ExportFBXMetaHuman(const Mesh& mesh, const std::string& filepath,
                                                const ExportOptions& options,
                                                const Transform* transform) {
    ExportResult result;
    ReportProgress(0.1f, "Preparing MetaHuman-compatible export...");

    const auto& assimpToMerged = mesh.GetAssimpToMergedMap();
    size_t originalVertCount = mesh.GetOriginalAssimpVertexCount();
    const auto& deformedVerts = mesh.GetVertices();
    const auto& uvs = mesh.GetUVs();
    const auto& faces = mesh.GetFaces();

    if (assimpToMerged.size() != originalVertCount) {
        result.errorMessage = "Invalid Assimp mapping data. Cannot perform MetaHuman-compatible export.";
        return result;
    }

    // Prepare transform matrix if needed
    Matrix4x4 transformMatrix;
    bool hasTransform = options.applyTransform && transform;
    if (hasTransform) {
        transformMatrix = transform->GetMatrix();
    }

    // Build the aiScene
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

    // Set mesh name
    std::string meshName = mesh.GetName();
    if (!options.ueNamingPrefix.empty()) {
        meshName = options.ueNamingPrefix + meshName;
    }
    outMesh->mName = aiString(meshName);

    ReportProgress(0.2f, "Reconstructing original vertex layout...");

    // Create N vertices in the original Assimp layout
    unsigned int numVerts = static_cast<unsigned int>(originalVertCount);
    outMesh->mNumVertices = numVerts;
    outMesh->mVertices = new aiVector3D[numVerts];

    // Map deformed positions back to original Assimp vertex indices
    for (unsigned int i = 0; i < numVerts; ++i) {
        int mergedIdx = assimpToMerged[i];
        Vector3 v = deformedVerts[mergedIdx];

        if (hasTransform) {
            const float* m = transformMatrix.Data();
            float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
            float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
            float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
            v = Vector3(x, y, z);
        }

        if (std::abs(options.scaleFactor - 1.0f) > 0.0001f) {
            v = v * options.scaleFactor;
        }

        outMesh->mVertices[i] = aiVector3D(v.x, v.y, v.z);
    }

    ReportProgress(0.4f, "Recomputing normals for split vertices...");

    // Recompute normals for the N-vertex layout using face.uvIndices
    // This preserves per-seam-side normals (each split vertex accumulates only its own faces)
    std::vector<Vector3> recomputedNormals(originalVertCount, Vector3(0.0f, 0.0f, 0.0f));
    for (const auto& face : faces) {
        const auto& indices = face.uvIndices.empty() ? face.vertexIndices : face.uvIndices;
        if (indices.size() < 3) continue;

        // Get positions using the merged vertex lookup
        Vector3 v0 = deformedVerts[assimpToMerged[indices[0]]];
        Vector3 v1 = deformedVerts[assimpToMerged[indices[1]]];
        Vector3 v2 = deformedVerts[assimpToMerged[indices[2]]];

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 faceNormal = edge1.Cross(edge2).Normalized();

        for (unsigned int idx : indices) {
            if (idx < originalVertCount) {
                recomputedNormals[idx] = recomputedNormals[idx] + faceNormal;
            }
        }
    }

    outMesh->mNormals = new aiVector3D[numVerts];
    for (unsigned int i = 0; i < numVerts; ++i) {
        Vector3 n = recomputedNormals[i].Normalized();

        if (hasTransform) {
            const float* m = transformMatrix.Data();
            float x = m[0] * n.x + m[4] * n.y + m[8] * n.z;
            float y = m[1] * n.x + m[5] * n.y + m[9] * n.z;
            float z = m[2] * n.x + m[6] * n.y + m[10] * n.z;
            n = Vector3(x, y, z).Normalized();
        }

        outMesh->mNormals[i] = aiVector3D(n.x, n.y, n.z);
    }

    ReportProgress(0.5f, "Writing UVs...");

    // Write UVs (original Assimp UVs, optionally undo the Y-flip from aiProcess_FlipUVs)
    if (!uvs.empty()) {
        outMesh->mTextureCoords[0] = new aiVector3D[numVerts];
        outMesh->mNumUVComponents[0] = 2;
        for (unsigned int i = 0; i < numVerts; ++i) {
            if (i < uvs.size()) {
                float u = uvs[i].x;
                float v = options.undoUVFlip ? (1.0f - uvs[i].y) : uvs[i].y;
                outMesh->mTextureCoords[0][i] = aiVector3D(u, v, 0.0f);
            } else {
                outMesh->mTextureCoords[0][i] = aiVector3D(0.0f, 0.0f, 0.0f);
            }
        }
    }

    ReportProgress(0.6f, "Writing faces...");

    // Write faces using uvIndices (original Assimp vertex indices)
    outMesh->mNumFaces = static_cast<unsigned int>(faces.size());
    outMesh->mFaces = new aiFace[outMesh->mNumFaces];

    for (unsigned int i = 0; i < outMesh->mNumFaces; ++i) {
        const auto& srcIndices = faces[i].uvIndices.empty() ? faces[i].vertexIndices : faces[i].uvIndices;
        aiFace& dstFace = outMesh->mFaces[i];
        dstFace.mNumIndices = static_cast<unsigned int>(srcIndices.size());
        dstFace.mIndices = new unsigned int[dstFace.mNumIndices];
        for (unsigned int j = 0; j < dstFace.mNumIndices; ++j) {
            dstFace.mIndices[j] = srcIndices[j];
        }
    }

    ReportProgress(0.7f, "Writing materials...");

    // Materials (same logic as ExportWithAssimp)
    if (options.includeMaterials) {
        const auto& materials = mesh.GetMaterials();
        if (!materials.empty()) {
            scene.mNumMaterials = static_cast<unsigned int>(materials.size());
            scene.mMaterials = new aiMaterial*[scene.mNumMaterials];

            for (unsigned int i = 0; i < scene.mNumMaterials; ++i) {
                scene.mMaterials[i] = new aiMaterial();
                const Material& srcMat = materials[i];

                aiString matName(srcMat.name);
                scene.mMaterials[i]->AddProperty(&matName, AI_MATKEY_NAME);

                aiColor3D diffuse(srcMat.diffuseColor.x, srcMat.diffuseColor.y, srcMat.diffuseColor.z);
                scene.mMaterials[i]->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

                aiColor3D specular(srcMat.specularColor.x, srcMat.specularColor.y, srcMat.specularColor.z);
                scene.mMaterials[i]->AddProperty(&specular, 1, AI_MATKEY_COLOR_SPECULAR);

                scene.mMaterials[i]->AddProperty(&srcMat.shininess, 1, AI_MATKEY_SHININESS);
            }
            outMesh->mMaterialIndex = 0;
        } else {
            scene.mNumMaterials = 1;
            scene.mMaterials = new aiMaterial*[1];
            scene.mMaterials[0] = new aiMaterial();
            aiString matName("DefaultMaterial");
            scene.mMaterials[0]->AddProperty(&matName, AI_MATKEY_NAME);
            outMesh->mMaterialIndex = 0;
        }
    } else {
        scene.mNumMaterials = 1;
        scene.mMaterials = new aiMaterial*[1];
        scene.mMaterials[0] = new aiMaterial();
        aiString matName("DefaultMaterial");
        scene.mMaterials[0]->AddProperty(&matName, AI_MATKEY_NAME);
        outMesh->mMaterialIndex = 0;
    }

    ReportProgress(0.8f, "Exporting FBX file...");

    // Force FBX format for MetaHuman compatibility
    Assimp::Exporter exporter;
    unsigned int postProcessFlags = 0;
    if (options.triangulate) {
        postProcessFlags |= aiProcess_Triangulate;
    }

    aiReturn exportResult = exporter.Export(&scene, "fbx", filepath, postProcessFlags);

    if (exportResult != aiReturn_SUCCESS) {
        result.errorMessage = std::format("MetaHuman export failed: {}", exporter.GetErrorString());
        MV_LOG_WARNING(result.errorMessage);
        return result;
    }

    ReportProgress(1.0f, "MetaHuman-compatible export complete!");

    result.success = true;
    result.exportedFilePath = filepath;
    result.vertexCount = static_cast<int>(originalVertCount);
    result.faceCount = static_cast<int>(faces.size());

    MV_LOG_DEBUG(std::format("Exported MetaHuman-compatible FBX to: {}", filepath));
    MV_LOG_DEBUG(std::format("  Original Assimp vertices: {}  Merged deformed vertices: {}  Faces: {}",
                              result.vertexCount, deformedVerts.size(), result.faceCount));

    return result;
}

#endif // HAVE_ASSIMP

ExportResult MeshExporter::ExportOBJMetaHuman(const Mesh& deformedMesh,
                                               const Mesh& originalMesh,
                                               const std::string& filepath,
                                               const ExportOptions& options,
                                               const Transform* transform) {
    ExportResult result;
    ReportProgress(0.1f, "Preparing MetaHuman OBJ export...");

    const std::string& originalPath = originalMesh.GetFilePath();
    std::ifstream origFile(originalPath);
    if (!origFile.is_open()) {
        result.errorMessage = std::format("Cannot open original morph mesh file: {}", originalPath);
        return result;
    }

    // --- Pass 1: Parse original OBJ to extract positions and normal count ---
    ReportProgress(0.15f, "Parsing original OBJ file...");

    std::vector<Vector3> objPositions;
    int objNormalCount = 0;
    std::vector<std::string> allLines;

    std::string rawLine;
    while (std::getline(origFile, rawLine)) {
        // Strip a trailing CR (Windows-encoded files round-tripped through git on \n hosts).
        if (!rawLine.empty() && rawLine.back() == '\r') rawLine.pop_back();
        allLines.push_back(rawLine);
        const std::string trimmed = Trim(rawLine);

        if (StartsWith(trimmed, "v ")) {
            std::vector<std::string> parts = SplitWhitespace(trimmed);
            if (parts.size() >= 4) {
                float x = std::stof(parts[1]);
                float y = std::stof(parts[2]);
                float z = std::stof(parts[3]);
                objPositions.push_back(Vector3(x, y, z));
            }
        } else if (StartsWith(trimmed, "vn ")) {
            objNormalCount++;
        }
    }
    origFile.close();

    int objPosCount = static_cast<int>(objPositions.size());
    MV_LOG_DEBUG(std::format("Original OBJ: {} positions, {} normals", objPosCount, objNormalCount));

    // --- Build mapping: OBJ position index → merged vertex index ---
    ReportProgress(0.25f, "Building position mapping...");

    const auto& originalMergedVerts = originalMesh.GetVertices();
    const auto& deformedVerts = deformedMesh.GetVertices();

    // Build spatial hash of original merged vertices for efficient lookup
    const float epsilon = 1e-4f;
    const float epsilonSq = epsilon * epsilon;
    const float cellSize = epsilon * 10.0f;
    const float invCell = 1.0f / cellSize;

    struct CellHash {
        size_t operator()(const std::tuple<int,int,int>& k) const {
            return std::hash<int>{}(std::get<0>(k)) ^
                   (std::hash<int>{}(std::get<1>(k)) * 2654435761u) ^
                   (std::hash<int>{}(std::get<2>(k)) * 40503u);
        }
    };

    std::unordered_map<std::tuple<int,int,int>, std::vector<int>, CellHash> mergedGrid;
    for (int i = 0; i < static_cast<int>(originalMergedVerts.size()); ++i) {
        const Vector3& v = originalMergedVerts[i];
        mergedGrid[{static_cast<int>(std::floor(v.x * invCell)),
                     static_cast<int>(std::floor(v.y * invCell)),
                     static_cast<int>(std::floor(v.z * invCell))}].push_back(i);
    }

    std::vector<int> objToMerged(objPosCount, -1);

    for (int i = 0; i < objPosCount; ++i) {
        const Vector3& p = objPositions[i];
        int cx = static_cast<int>(std::floor(p.x * invCell));
        int cy = static_cast<int>(std::floor(p.y * invCell));
        int cz = static_cast<int>(std::floor(p.z * invCell));

        float bestDistSq = epsilonSq * 100.0f;  // Allow slightly more tolerance
        int bestIdx = -1;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    auto it = mergedGrid.find({cx + dx, cy + dy, cz + dz});
                    if (it == mergedGrid.end()) continue;
                    for (int j : it->second) {
                        Vector3 diff = p - originalMergedVerts[j];
                        float distSq = diff.Dot(diff);
                        if (distSq < bestDistSq) {
                            bestDistSq = distSq;
                            bestIdx = j;
                        }
                    }
                }
            }
        }

        objToMerged[i] = bestIdx;
    }

    // Check mapping quality
    int unmatched = 0;
    for (int idx : objToMerged) {
        if (idx < 0) unmatched++;
    }
    if (unmatched > 0) {
        MV_LOG_WARNING(std::format("MetaHuman OBJ export: {} of {} positions could not be matched to merged vertices",
                                    unmatched, objPosCount));
        if (unmatched > objPosCount / 10) {
            result.errorMessage = std::format("Too many unmatched positions ({} of {}). "
                                              "Original mesh may have changed.", unmatched, objPosCount);
            return result;
        }
    }

    MV_LOG_DEBUG(std::format("Position mapping: OBJ positions: {} -> merged vertices: {} matched: {}",
                              objPosCount, originalMergedVerts.size(), objPosCount - unmatched));

    // --- Prepare transform ---
    Matrix4x4 transformMatrix;
    bool hasTransform = options.applyTransform && transform;
    if (hasTransform) {
        transformMatrix = transform->GetMatrix();
    }

    // --- Precompute normals for the original OBJ position layout ---
    ReportProgress(0.4f, "Recomputing normals...");

    std::vector<Vector3> recomputedNormals(objPosCount, Vector3(0.0f, 0.0f, 0.0f));

    for (const auto& line : allLines) {
        const std::string trimmed = Trim(line);
        if (!StartsWith(trimmed, "f ")) continue;

        std::vector<std::string> parts = SplitWhitespace(trimmed);
        if (parts.size() < 4) continue;  // Need at least 3 vertices

        // Parse position indices from face (OBJ format: v/vt/vn or v/vt or v//vn or v)
        std::vector<int> posIndices;
        for (size_t pi = 1; pi < parts.size(); ++pi) {
            const std::string& vertex = parts[pi];
            size_t slashPos = vertex.find('/');
            int vIdx = 0;
            try {
                if (slashPos != std::string::npos) {
                    vIdx = std::stoi(vertex.substr(0, slashPos));
                } else {
                    vIdx = std::stoi(vertex);
                }
            } catch (...) {
                continue;
            }
            // OBJ indices are 1-based, convert to 0-based
            if (vIdx > 0) {
                posIndices.push_back(vIdx - 1);
            } else if (vIdx < 0) {
                // Negative = relative from end
                posIndices.push_back(objPosCount + vIdx);
            }
        }

        if (posIndices.size() < 3) continue;

        // Compute face normal using deformed positions
        int i0 = posIndices[0], i1 = posIndices[1], i2 = posIndices[2];
        if (i0 >= 0 && i0 < objPosCount && objToMerged[i0] >= 0 &&
            i1 >= 0 && i1 < objPosCount && objToMerged[i1] >= 0 &&
            i2 >= 0 && i2 < objPosCount && objToMerged[i2] >= 0) {

            Vector3 v0 = deformedVerts[objToMerged[i0]];
            Vector3 v1 = deformedVerts[objToMerged[i1]];
            Vector3 v2 = deformedVerts[objToMerged[i2]];

            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 faceNormal = edge1.Cross(edge2).Normalized();

            for (int idx : posIndices) {
                if (idx >= 0 && idx < objPosCount) {
                    recomputedNormals[idx] = recomputedNormals[idx] + faceNormal;
                }
            }
        }
    }

    // Normalize
    for (auto& n : recomputedNormals) {
        n = n.Normalized();
    }

    // --- Pass 2: Rewrite the OBJ file with deformed positions and normals ---
    ReportProgress(0.6f, "Writing MetaHuman-compatible OBJ...");

    // Ensure output path has .obj extension
    std::string outputPath = filepath;
    if (!EndsWithICase(outputPath, ".obj")) {
        outputPath += ".obj";
    }

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        result.errorMessage = std::format("Cannot open output file: {}", outputPath);
        return result;
    }
    out.precision(8);

    int vIndex = 0;   // Current position index (0-based)
    int vnIndex = 0;  // Current normal index (0-based)
    int faceCount = 0;

    for (const auto& line : allLines) {
        const std::string trimmed = Trim(line);

        if (StartsWith(trimmed, "v ") && vIndex < objPosCount) {
            // Replace position with deformed position
            int mergedIdx = objToMerged[vIndex];
            Vector3 v;

            if (mergedIdx >= 0 && mergedIdx < static_cast<int>(deformedVerts.size())) {
                v = deformedVerts[mergedIdx];
            } else {
                // Unmatched - keep original position
                v = objPositions[vIndex];
            }

            if (hasTransform) {
                const float* m = transformMatrix.Data();
                float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
                float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
                float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
                v = Vector3(x, y, z);
            }

            if (std::abs(options.scaleFactor - 1.0f) > 0.0001f) {
                v = v * options.scaleFactor;
            }

            out << "v " << v.x << " " << v.y << " " << v.z << "\n";
            vIndex++;
        } else if (StartsWith(trimmed, "vn ") && vnIndex < objPosCount) {
            // Replace normal with recomputed normal
            Vector3 n = recomputedNormals[vnIndex];

            if (hasTransform) {
                const float* m = transformMatrix.Data();
                float x = m[0] * n.x + m[4] * n.y + m[8] * n.z;
                float y = m[1] * n.x + m[5] * n.y + m[9] * n.z;
                float z = m[2] * n.x + m[6] * n.y + m[10] * n.z;
                n = Vector3(x, y, z).Normalized();
            }

            out << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            vnIndex++;
        } else {
            // Copy everything else verbatim (vt, f, mtllib, usemtl, comments, etc.)
            out << line << "\n";

            if (StartsWith(trimmed, "f ")) {
                faceCount++;
            }
        }
    }

    out.close();

    ReportProgress(1.0f, "MetaHuman-compatible OBJ export complete!");

    result.success = true;
    result.exportedFilePath = outputPath;
    result.vertexCount = objPosCount;
    result.faceCount = faceCount;

    MV_LOG_DEBUG(std::format("Exported MetaHuman-compatible OBJ to: {}", outputPath));
    MV_LOG_DEBUG(std::format("  Positions: {} (matches original)  Normals recomputed: {}  Faces: {}  Unmatched: {}",
                              objPosCount, vnIndex, faceCount, unmatched));

    return result;
}

} // namespace MetaVisage
