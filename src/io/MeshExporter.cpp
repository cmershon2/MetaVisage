#include "io/MeshExporter.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <map>

#ifdef HAVE_ASSIMP
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

namespace MetaVisage {

MeshExporter::MeshExporter() {
}

MeshExporter::~MeshExporter() {
}

void MeshExporter::ReportProgress(float progress, const QString& message) {
    if (progressCallback_) {
        progressCallback_(progress, message);
    }
}

QString MeshExporter::GetFormatExtension(ExportFormat format) {
    switch (format) {
        case ExportFormat::FBX: return ".fbx";
        case ExportFormat::OBJ: return ".obj";
        case ExportFormat::GLTF: return ".gltf";
    }
    return ".fbx";
}

QString MeshExporter::GetFormatFilter(ExportFormat format) {
    switch (format) {
        case ExportFormat::FBX: return "FBX (*.fbx)";
        case ExportFormat::OBJ: return "OBJ (*.obj)";
        case ExportFormat::GLTF: return "GLTF (*.gltf)";
    }
    return "FBX (*.fbx)";
}

QString MeshExporter::GetAllFormatsFilter() {
    return "FBX (*.fbx);;OBJ (*.obj);;GLTF (*.gltf);;All Files (*.*)";
}

ExportResult MeshExporter::Export(const Mesh& mesh, const QString& filepath,
                                   const ExportOptions& options,
                                   const Transform* transform) {
    ExportResult result;

    if (mesh.GetVertexCount() == 0) {
        result.errorMessage = "Mesh has no vertices to export.";
        return result;
    }

    if (filepath.isEmpty()) {
        result.errorMessage = "No filepath specified.";
        return result;
    }

    ReportProgress(0.0f, "Starting export...");

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

ExportResult MeshExporter::ExportOBJDirect(const Mesh& mesh, const QString& filepath,
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

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = QString("Cannot open file for writing: %1").arg(filepath);
        return result;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(8);

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

    file.close();

    ReportProgress(1.0f, "Export complete!");

    result.success = true;
    result.exportedFilePath = filepath;
    result.vertexCount = static_cast<int>(vertices.size());
    result.faceCount = static_cast<int>(faces.size());

    qDebug() << "Exported OBJ (direct) to:" << filepath;
    qDebug() << "  Vertices:" << result.vertexCount;
    qDebug() << "  UVs:" << uvs.size();
    qDebug() << "  Faces:" << result.faceCount;

    return result;
}

#ifdef HAVE_ASSIMP

ExportResult MeshExporter::ExportWithAssimp(const Mesh& mesh, const QString& filepath,
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
    QString meshName = mesh.GetName();
    if (!options.ueNamingPrefix.isEmpty()) {
        meshName = options.ueNamingPrefix + meshName;
    }
    outMesh->mName = aiString(meshName.toStdString());

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
    aiReturn exportResult = exporter.Export(&scene, formatId, filepath.toStdString(), postProcessFlags);

    if (exportResult != aiReturn_SUCCESS) {
        result.errorMessage = QString("Export failed: %1").arg(exporter.GetErrorString());
        qWarning() << result.errorMessage;
        return result;
    }

    ReportProgress(1.0f, "Export complete!");

    result.success = true;
    result.exportedFilePath = filepath;
    result.vertexCount = static_cast<int>(vertices.size());
    result.faceCount = static_cast<int>(faces.size());

    qDebug() << "Exported mesh to:" << filepath;
    qDebug() << "  Format:" << QString::fromStdString(formatId);
    qDebug() << "  Vertices:" << result.vertexCount;
    qDebug() << "  Faces:" << result.faceCount;

    return result;
}

#endif // HAVE_ASSIMP

} // namespace MetaVisage
