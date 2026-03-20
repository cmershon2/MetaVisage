#include "io/ProjectSerializer.h"
#include "utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QFileInfo>
#include <cstdlib>

namespace MetaVisage {

ProjectSerializer::ProjectSerializer() {
}

ProjectSerializer::~ProjectSerializer() {
}

// --- Save ---

SerializationResult ProjectSerializer::Save(const Project& project, const QString& filepath) {
    SerializationResult result;
    QString projectDir = QFileInfo(filepath).absolutePath();

    QJsonObject root;
    root["formatVersion"] = FORMAT_VERSION;
    root["projectName"] = project.GetName();
    root["created"] = project.GetCreated().toString(Qt::ISODate);
    root["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["currentStage"] = static_cast<int>(project.GetCurrentStage());

    // Mesh references
    root["morphMesh"] = SerializeMeshReference(project.GetMorphMesh(), projectDir);
    root["targetMesh"] = SerializeMeshReference(project.GetTargetMesh(), projectDir);

    // Stage data
    root["alignment"] = SerializeAlignmentData(project.GetAlignmentData());
    root["pointReference"] = SerializePointReferenceData(project.GetPointReferenceData());
    root["morph"] = SerializeMorphData(project.GetMorphData());

    // Write to file
    QJsonDocument doc(root);
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.errorMessage = QString("Cannot open file for writing: %1").arg(filepath);
        return result;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    result.success = true;
    MV_LOG_INFO(QString("Project saved to: %1").arg(filepath));
    return result;
}

// --- Load ---

SerializationResult ProjectSerializer::Load(Project& project, const QString& filepath) {
    SerializationResult result;

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QString("Cannot open file for reading: %1").arg(filepath);
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.errorMessage = QString("JSON parse error: %1").arg(parseError.errorString());
        return result;
    }

    QJsonObject root = doc.object();

    // Check format version
    int version = root["formatVersion"].toInt(0);
    if (version < 1 || version > FORMAT_VERSION) {
        result.errorMessage = QString("Unsupported project format version: %1").arg(version);
        return result;
    }

    QString projectDir = QFileInfo(filepath).absolutePath();

    // Project metadata
    project.SetName(root["projectName"].toString("Untitled Project"));
    project.SetCurrentStage(static_cast<WorkflowStage>(root["currentStage"].toInt(0)));

    // Mesh references
    if (root.contains("morphMesh")) {
        MeshReference& morphRef = project.GetMorphMesh();
        morphRef = DeserializeMeshReference(root["morphMesh"].toObject(), projectDir, result.warnings);
    }

    if (root.contains("targetMesh")) {
        MeshReference& targetRef = project.GetTargetMesh();
        targetRef = DeserializeMeshReference(root["targetMesh"].toObject(), projectDir, result.warnings);
    }

    // Stage data
    if (root.contains("alignment")) {
        project.GetAlignmentData() = DeserializeAlignmentData(root["alignment"].toObject());
    }

    if (root.contains("pointReference")) {
        project.GetPointReferenceData() = DeserializePointReferenceData(root["pointReference"].toObject());
    }

    if (root.contains("morph")) {
        MorphData loaded = DeserializeMorphData(root["morph"].toObject());
        MorphData& morphData = project.GetMorphData();
        morphData.algorithm = loaded.algorithm;
        morphData.stiffness = loaded.stiffness;
        morphData.smoothness = loaded.smoothness;
        morphData.isProcessed = loaded.isProcessed;
        morphData.isAccepted = loaded.isAccepted;
        morphData.maxDisplacement = loaded.maxDisplacement;
        morphData.avgDisplacement = loaded.avgDisplacement;
        morphData.hasDeformedData = loaded.hasDeformedData;
        morphData.savedDeformedVertices = std::move(loaded.savedDeformedVertices);
        morphData.savedDeformedNormals = std::move(loaded.savedDeformedNormals);
        // NRICP parameters
        morphData.nricpStiffnessSteps = loaded.nricpStiffnessSteps;
        morphData.nricpAlphaInitial = loaded.nricpAlphaInitial;
        morphData.nricpAlphaFinal = loaded.nricpAlphaFinal;
        morphData.nricpIcpIterations = loaded.nricpIcpIterations;
        morphData.nricpNormalThreshold = loaded.nricpNormalThreshold;
        morphData.nricpLandmarkWeight = loaded.nricpLandmarkWeight;
        morphData.nricpEpsilon = loaded.nricpEpsilon;
        morphData.nricpEnableBoundaryExclusion = loaded.nricpEnableBoundaryExclusion;
        morphData.nricpBoundaryExclusionHops = loaded.nricpBoundaryExclusionHops;
    }

    result.success = true;
    MV_LOG_INFO(QString("Project loaded from: %1").arg(filepath));
    if (!result.warnings.isEmpty()) {
        MV_LOG_WARNING(QString("Project load warnings: %1").arg(result.warnings.join("; ")));
    }
    return result;
}

// --- Serialization helpers ---

QJsonObject ProjectSerializer::SerializeVector3(const Vector3& v) const {
    QJsonObject obj;
    obj["x"] = static_cast<double>(v.x);
    obj["y"] = static_cast<double>(v.y);
    obj["z"] = static_cast<double>(v.z);
    return obj;
}

QJsonObject ProjectSerializer::SerializeQuaternion(const Quaternion& q) const {
    QJsonObject obj;
    obj["x"] = static_cast<double>(q.x);
    obj["y"] = static_cast<double>(q.y);
    obj["z"] = static_cast<double>(q.z);
    obj["w"] = static_cast<double>(q.w);
    return obj;
}

QJsonObject ProjectSerializer::SerializeTransform(const Transform& t) const {
    QJsonObject obj;
    obj["position"] = SerializeVector3(t.GetPosition());
    obj["rotation"] = SerializeQuaternion(t.GetRotation());
    obj["scale"] = SerializeVector3(t.GetScale());
    return obj;
}

QJsonObject ProjectSerializer::SerializeMeshReference(const MeshReference& ref, const QString& projectDir) const {
    QJsonObject obj;
    obj["filepath"] = MakeRelativePath(ref.filepath, projectDir);
    obj["isLoaded"] = ref.isLoaded;
    obj["transform"] = SerializeTransform(ref.transform);
    return obj;
}

QJsonObject ProjectSerializer::SerializeAlignmentData(const AlignmentData& data) const {
    QJsonObject obj;
    obj["targetMeshTransform"] = SerializeTransform(data.targetMeshTransform);
    return obj;
}

QJsonObject ProjectSerializer::SerializePointCorrespondence(const PointCorrespondence& corr) const {
    QJsonObject obj;
    obj["pointID"] = corr.pointID;
    obj["morphMeshPosition"] = SerializeVector3(corr.morphMeshPosition);
    obj["morphMeshVertexIndex"] = corr.morphMeshVertexIndex;
    obj["targetMeshPosition"] = SerializeVector3(corr.targetMeshPosition);
    obj["targetMeshVertexIndex"] = corr.targetMeshVertexIndex;
    obj["isSymmetric"] = corr.isSymmetric;
    obj["symmetricPairID"] = corr.symmetricPairID;
    return obj;
}

QJsonObject ProjectSerializer::SerializePointReferenceData(const PointReferenceData& data) const {
    QJsonObject obj;
    obj["symmetryEnabled"] = data.symmetryEnabled;
    obj["symmetryAxis"] = static_cast<int>(data.symmetryAxis);
    obj["symmetryPlaneOffset"] = static_cast<double>(data.symmetryPlaneOffset);

    QJsonArray correspondences;
    for (const auto& corr : data.correspondences) {
        correspondences.append(SerializePointCorrespondence(corr));
    }
    obj["correspondences"] = correspondences;

    return obj;
}

QJsonObject ProjectSerializer::SerializeMorphData(const MorphData& data) const {
    QJsonObject obj;
    obj["algorithm"] = static_cast<int>(data.algorithm);
    obj["stiffness"] = static_cast<double>(data.stiffness);
    obj["smoothness"] = static_cast<double>(data.smoothness);
    obj["previewMode"] = static_cast<int>(data.previewMode);
    obj["isProcessed"] = data.isProcessed;
    obj["isAccepted"] = data.isAccepted;

    // NRICP parameters
    obj["nricpStiffnessSteps"] = data.nricpStiffnessSteps;
    obj["nricpAlphaInitial"] = static_cast<double>(data.nricpAlphaInitial);
    obj["nricpAlphaFinal"] = static_cast<double>(data.nricpAlphaFinal);
    obj["nricpIcpIterations"] = data.nricpIcpIterations;
    obj["nricpNormalThreshold"] = static_cast<double>(data.nricpNormalThreshold);
    obj["nricpLandmarkWeight"] = static_cast<double>(data.nricpLandmarkWeight);
    obj["nricpEpsilon"] = static_cast<double>(data.nricpEpsilon);
    obj["nricpEnableBoundaryExclusion"] = data.nricpEnableBoundaryExclusion;
    obj["nricpBoundaryExclusionHops"] = data.nricpBoundaryExclusionHops;

    // NRICP optimization, rigidity, and subsampling parameters
    obj["nricpOptimizationIterations"] = data.nricpOptimizationIterations;
    obj["nricpDpInitial"] = static_cast<double>(data.nricpDpInitial);
    obj["nricpDpFinal"] = static_cast<double>(data.nricpDpFinal);
    obj["nricpGammaInitial"] = static_cast<double>(data.nricpGammaInitial);
    obj["nricpGammaFinal"] = static_cast<double>(data.nricpGammaFinal);
    obj["nricpSamplingInitial"] = static_cast<double>(data.nricpSamplingInitial);
    obj["nricpSamplingFinal"] = static_cast<double>(data.nricpSamplingFinal);
    obj["nricpNormalizeSampling"] = data.nricpNormalizeSampling;

    // Save vertex mask (run-length encoded for compactness)
    if (!data.vertexMask.empty()) {
        QJsonArray maskRLE;
        bool current = data.vertexMask[0];
        int count = 1;
        for (size_t i = 1; i < data.vertexMask.size(); ++i) {
            if (data.vertexMask[i] == current) {
                count++;
            } else {
                maskRLE.append(current ? count : -count);
                current = data.vertexMask[i];
                count = 1;
            }
        }
        maskRLE.append(current ? count : -count);
        obj["vertexMaskRLE"] = maskRLE;
        obj["vertexMaskSize"] = static_cast<int>(data.vertexMask.size());
    }

    // Save deformed mesh vertex data if it exists
    if (data.deformedMorphMesh && data.isProcessed) {
        const auto& vertices = data.deformedMorphMesh->GetVertices();
        const auto& normals = data.deformedMorphMesh->GetNormals();

        QJsonArray vertexArray;
        for (const auto& v : vertices) {
            vertexArray.append(static_cast<double>(v.x));
            vertexArray.append(static_cast<double>(v.y));
            vertexArray.append(static_cast<double>(v.z));
        }
        obj["deformedVertices"] = vertexArray;

        QJsonArray normalArray;
        for (const auto& n : normals) {
            normalArray.append(static_cast<double>(n.x));
            normalArray.append(static_cast<double>(n.y));
            normalArray.append(static_cast<double>(n.z));
        }
        obj["deformedNormals"] = normalArray;

        obj["maxDisplacement"] = static_cast<double>(data.maxDisplacement);
        obj["avgDisplacement"] = static_cast<double>(data.avgDisplacement);
    }

    return obj;
}

// --- Deserialization helpers ---

Vector3 ProjectSerializer::DeserializeVector3(const QJsonObject& obj) const {
    return Vector3(
        static_cast<float>(obj["x"].toDouble(0.0)),
        static_cast<float>(obj["y"].toDouble(0.0)),
        static_cast<float>(obj["z"].toDouble(0.0))
    );
}

Quaternion ProjectSerializer::DeserializeQuaternion(const QJsonObject& obj) const {
    return Quaternion(
        static_cast<float>(obj["x"].toDouble(0.0)),
        static_cast<float>(obj["y"].toDouble(0.0)),
        static_cast<float>(obj["z"].toDouble(0.0)),
        static_cast<float>(obj["w"].toDouble(1.0))
    );
}

Transform ProjectSerializer::DeserializeTransform(const QJsonObject& obj) const {
    Transform t;
    if (obj.contains("position")) {
        t.SetPosition(DeserializeVector3(obj["position"].toObject()));
    }
    if (obj.contains("rotation")) {
        t.SetRotation(DeserializeQuaternion(obj["rotation"].toObject()));
    }
    if (obj.contains("scale")) {
        t.SetScale(DeserializeVector3(obj["scale"].toObject()));
    }
    return t;
}

MeshReference ProjectSerializer::DeserializeMeshReference(const QJsonObject& obj, const QString& projectDir,
                                                            QStringList& warnings) const {
    MeshReference ref;
    QString relativePath = obj["filepath"].toString();
    ref.filepath = ResolveRelativePath(relativePath, projectDir);
    ref.transform = DeserializeTransform(obj["transform"].toObject());

    // Try to load the mesh
    if (!ref.filepath.isEmpty() && QFile::exists(ref.filepath)) {
        ref.mesh = std::make_shared<Mesh>();
        if (ref.mesh->Load(ref.filepath)) {
            ref.isLoaded = true;
        } else {
            warnings.append(QString("Failed to load mesh: %1").arg(ref.filepath));
            ref.mesh.reset();
            ref.isLoaded = false;
        }
    } else if (!relativePath.isEmpty()) {
        warnings.append(QString("Mesh file not found: %1 (resolved to: %2)")
                        .arg(relativePath, ref.filepath));
        ref.isLoaded = false;
    }

    return ref;
}

AlignmentData ProjectSerializer::DeserializeAlignmentData(const QJsonObject& obj) const {
    AlignmentData data;
    if (obj.contains("targetMeshTransform")) {
        data.targetMeshTransform = DeserializeTransform(obj["targetMeshTransform"].toObject());
    }
    return data;
}

PointCorrespondence ProjectSerializer::DeserializePointCorrespondence(const QJsonObject& obj) const {
    PointCorrespondence corr;
    corr.pointID = obj["pointID"].toInt(-1);
    corr.morphMeshPosition = DeserializeVector3(obj["morphMeshPosition"].toObject());
    corr.morphMeshVertexIndex = obj["morphMeshVertexIndex"].toInt(-1);
    corr.targetMeshPosition = DeserializeVector3(obj["targetMeshPosition"].toObject());
    corr.targetMeshVertexIndex = obj["targetMeshVertexIndex"].toInt(-1);
    corr.isSymmetric = obj["isSymmetric"].toBool(false);
    corr.symmetricPairID = obj["symmetricPairID"].toInt(-1);
    return corr;
}

PointReferenceData ProjectSerializer::DeserializePointReferenceData(const QJsonObject& obj) const {
    PointReferenceData data;
    data.symmetryEnabled = obj["symmetryEnabled"].toBool(false);
    data.symmetryAxis = static_cast<Axis>(obj["symmetryAxis"].toInt(0));
    data.symmetryPlaneOffset = static_cast<float>(obj["symmetryPlaneOffset"].toDouble(0.0));

    QJsonArray correspondences = obj["correspondences"].toArray();
    for (const auto& val : correspondences) {
        data.correspondences.push_back(DeserializePointCorrespondence(val.toObject()));
    }

    return data;
}

MorphData ProjectSerializer::DeserializeMorphData(const QJsonObject& obj) const {
    MorphData data;
    data.algorithm = static_cast<DeformationAlgorithm>(obj["algorithm"].toInt(
        static_cast<int>(DeformationAlgorithm::NRICP)));
    data.stiffness = static_cast<float>(obj["stiffness"].toDouble(0.5));
    data.smoothness = static_cast<float>(obj["smoothness"].toDouble(0.5));
    data.previewMode = static_cast<MorphPreviewMode>(obj["previewMode"].toInt(0));
    data.isProcessed = obj["isProcessed"].toBool(false);
    data.isAccepted = obj["isAccepted"].toBool(false);
    data.maxDisplacement = static_cast<float>(obj["maxDisplacement"].toDouble(0.0));
    data.avgDisplacement = static_cast<float>(obj["avgDisplacement"].toDouble(0.0));

    // NRICP parameters (backward compatible - defaults match MorphData constructor)
    data.nricpStiffnessSteps = obj["nricpStiffnessSteps"].toInt(5);
    data.nricpAlphaInitial = static_cast<float>(obj["nricpAlphaInitial"].toDouble(100.0));
    data.nricpAlphaFinal = static_cast<float>(obj["nricpAlphaFinal"].toDouble(1.0));
    data.nricpIcpIterations = obj["nricpIcpIterations"].toInt(3);
    data.nricpNormalThreshold = static_cast<float>(obj["nricpNormalThreshold"].toDouble(60.0));
    data.nricpLandmarkWeight = static_cast<float>(obj["nricpLandmarkWeight"].toDouble(10.0));
    data.nricpEpsilon = static_cast<float>(obj["nricpEpsilon"].toDouble(1e-4));
    data.nricpEnableBoundaryExclusion = obj["nricpEnableBoundaryExclusion"].toBool(true);
    data.nricpBoundaryExclusionHops = obj["nricpBoundaryExclusionHops"].toInt(3);

    // NRICP optimization, rigidity, and subsampling parameters (backward compatible)
    data.nricpOptimizationIterations = obj["nricpOptimizationIterations"].toInt(1);
    data.nricpDpInitial = static_cast<float>(obj["nricpDpInitial"].toDouble(1.0));
    data.nricpDpFinal = static_cast<float>(obj["nricpDpFinal"].toDouble(1.0));
    data.nricpGammaInitial = static_cast<float>(obj["nricpGammaInitial"].toDouble(0.0));
    data.nricpGammaFinal = static_cast<float>(obj["nricpGammaFinal"].toDouble(0.0));
    data.nricpSamplingInitial = static_cast<float>(obj["nricpSamplingInitial"].toDouble(0.0));
    data.nricpSamplingFinal = static_cast<float>(obj["nricpSamplingFinal"].toDouble(0.0));
    data.nricpNormalizeSampling = obj["nricpNormalizeSampling"].toBool(true);

    // Restore vertex mask (run-length encoded)
    if (obj.contains("vertexMaskRLE") && obj.contains("vertexMaskSize")) {
        int maskSize = obj["vertexMaskSize"].toInt(0);
        QJsonArray maskRLE = obj["vertexMaskRLE"].toArray();
        data.vertexMask.resize(maskSize, false);
        int idx = 0;
        for (int i = 0; i < maskRLE.size() && idx < maskSize; ++i) {
            int run = maskRLE[i].toInt(0);
            bool value = (run > 0);
            int runLength = std::abs(run);
            for (int j = 0; j < runLength && idx < maskSize; ++j) {
                data.vertexMask[idx++] = value;
            }
        }
    }

    // Restore deformed mesh vertex data if present
    if (obj.contains("deformedVertices") && obj.contains("deformedNormals")) {
        QJsonArray vertexArray = obj["deformedVertices"].toArray();
        QJsonArray normalArray = obj["deformedNormals"].toArray();

        if (vertexArray.size() >= 3 && vertexArray.size() % 3 == 0) {
            std::vector<Vector3> vertices;
            vertices.reserve(vertexArray.size() / 3);
            for (int i = 0; i < vertexArray.size(); i += 3) {
                vertices.push_back(Vector3(
                    static_cast<float>(vertexArray[i].toDouble()),
                    static_cast<float>(vertexArray[i + 1].toDouble()),
                    static_cast<float>(vertexArray[i + 2].toDouble())
                ));
            }

            std::vector<Vector3> normals;
            normals.reserve(normalArray.size() / 3);
            for (int i = 0; i < normalArray.size(); i += 3) {
                normals.push_back(Vector3(
                    static_cast<float>(normalArray[i].toDouble()),
                    static_cast<float>(normalArray[i + 1].toDouble()),
                    static_cast<float>(normalArray[i + 2].toDouble())
                ));
            }

            data.hasDeformedData = true;
            data.savedDeformedVertices = std::move(vertices);
            data.savedDeformedNormals = std::move(normals);
        }
    }

    return data;
}

// --- Path utilities ---

QString ProjectSerializer::MakeRelativePath(const QString& filepath, const QString& projectDir) const {
    if (filepath.isEmpty()) return "";
    QDir dir(projectDir);
    return dir.relativeFilePath(filepath);
}

QString ProjectSerializer::ResolveRelativePath(const QString& relativePath, const QString& projectDir) const {
    if (relativePath.isEmpty()) return "";

    // If it's already absolute, return as-is
    QFileInfo info(relativePath);
    if (info.isAbsolute()) return relativePath;

    // Resolve relative to project directory
    return QDir(projectDir).absoluteFilePath(relativePath);
}

} // namespace MetaVisage
