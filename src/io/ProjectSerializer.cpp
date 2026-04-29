#include "io/ProjectSerializer.h"
#include "core/TextureData.h"
#include "utils/Logger.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace MetaVisage {

namespace {

// Helpers for nlohmann::ordered_json reads with defaults — mirror QJsonValue::toX(default)
template <typename T>
T GetOr(const ProjectSerializer::Json& obj, const char* key, T fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    try { return it->get<T>(); } catch (...) { return fallback; }
}

std::string GetStr(const ProjectSerializer::Json& obj, const char* key, const std::string& fallback = {}) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) return fallback;
    return it->get<std::string>();
}

bool Has(const ProjectSerializer::Json& obj, const char* key) {
    return obj.find(key) != obj.end();
}

std::string IsoNow() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                       tm.tm_hour, tm.tm_min, tm.tm_sec);
}

} // namespace

ProjectSerializer::ProjectSerializer() {
}

ProjectSerializer::~ProjectSerializer() {
}

// --- Save ---

SerializationResult ProjectSerializer::Save(const Project& project, const std::string& filepath) {
    SerializationResult result;
    const std::string projectDir = std::filesystem::path(filepath).parent_path().string();

    Json root;
    root["formatVersion"] = FORMAT_VERSION;
    root["projectName"] = project.GetName();
    // For "created" we serialize the current time when this is a fresh save —
    // the original Qt code used GetCreated() which mirrors what the project
    // tracks internally; we keep parity by mirroring that getter.
    {
        const std::time_t tt = std::chrono::system_clock::to_time_t(project.GetCreated());
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        root["created"] = std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                       tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    root["lastModified"] = IsoNow();
    root["currentStage"] = static_cast<int>(project.GetCurrentStage());

    // Mesh references
    root["morphMesh"] = SerializeMeshReference(project.GetMorphMesh(), projectDir);
    root["targetMesh"] = SerializeMeshReference(project.GetTargetMesh(), projectDir);

    // Stage data
    root["alignment"] = SerializeAlignmentData(project.GetAlignmentData());
    root["pointReference"] = SerializePointReferenceData(project.GetPointReferenceData());
    root["morph"] = SerializeMorphData(project.GetMorphData());

    // Texture data (v2+)
    root["targetTextures"] = SerializeTextureSet(project.GetTargetTextures(), projectDir);

    std::ofstream out(filepath);
    if (!out.is_open()) {
        result.errorMessage = std::format("Cannot open file for writing: {}", filepath);
        return result;
    }
    out << root.dump(2);
    out.flush();
    if (out.fail()) {
        // Disk full / I/O error mid-write would otherwise leave a truncated
        // .mmproj on disk and report success. Surface the failure to the
        // user via SerializationResult so the caller can show an error
        // dialog instead of silently corrupting their project.
        result.errorMessage = std::format("Write failed for {} (disk full or I/O error)", filepath);
        return result;
    }

    result.success = true;
    MV_LOG_INFO(std::format("Project saved to: {}", filepath));
    return result;
}

// --- Load ---

SerializationResult ProjectSerializer::Load(Project& project, const std::string& filepath) {
    SerializationResult result;

    std::ifstream in(filepath);
    if (!in.is_open()) {
        result.errorMessage = std::format("Cannot open file for reading: {}", filepath);
        return result;
    }

    std::stringstream buf;
    buf << in.rdbuf();
    in.close();

    Json root;
    try {
        root = Json::parse(buf.str());
    } catch (const std::exception& e) {
        result.errorMessage = std::format("JSON parse error: {}", e.what());
        return result;
    }

    if (!root.is_object()) {
        result.errorMessage = "Project root is not a JSON object";
        return result;
    }

    // Check format version
    int version = GetOr<int>(root, "formatVersion", 0);
    if (version < 1 || version > FORMAT_VERSION) {
        result.errorMessage = std::format("Unsupported project format version: {}", version);
        return result;
    }

    const std::string projectDir = std::filesystem::path(filepath).parent_path().string();

    // Project metadata
    project.SetName(GetStr(root, "projectName", "Untitled Project"));
    project.SetCurrentStage(static_cast<WorkflowStage>(GetOr<int>(root, "currentStage", 0)));

    // Mesh references
    if (Has(root, "morphMesh")) {
        MeshReference& morphRef = project.GetMorphMesh();
        morphRef = DeserializeMeshReference(root["morphMesh"], projectDir, result.warnings);
    }

    if (Has(root, "targetMesh")) {
        MeshReference& targetRef = project.GetTargetMesh();
        targetRef = DeserializeMeshReference(root["targetMesh"], projectDir, result.warnings);
    }

    // Stage data
    if (Has(root, "alignment")) {
        project.GetAlignmentData() = DeserializeAlignmentData(root["alignment"]);
    }

    if (Has(root, "pointReference")) {
        project.GetPointReferenceData() = DeserializePointReferenceData(root["pointReference"]);
    }

    if (Has(root, "morph")) {
        MorphData loaded = DeserializeMorphData(root["morph"]);
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

    // Texture data (v2+, backward compatible)
    if (Has(root, "targetTextures")) {
        project.GetTargetTextures() = DeserializeTextureSet(root["targetTextures"],
                                                              projectDir, result.warnings);
    }

    result.success = true;
    MV_LOG_INFO(std::format("Project loaded from: {}", filepath));
    if (!result.warnings.empty()) {
        std::ostringstream joined;
        for (size_t i = 0; i < result.warnings.size(); ++i) {
            if (i > 0) joined << "; ";
            joined << result.warnings[i];
        }
        MV_LOG_WARNING(std::format("Project load warnings: {}", joined.str()));
    }
    return result;
}

// --- Serialization helpers ---

ProjectSerializer::Json ProjectSerializer::SerializeVector3(const Vector3& v) const {
    Json obj;
    obj["x"] = static_cast<double>(v.x);
    obj["y"] = static_cast<double>(v.y);
    obj["z"] = static_cast<double>(v.z);
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializeQuaternion(const Quaternion& q) const {
    Json obj;
    obj["x"] = static_cast<double>(q.x);
    obj["y"] = static_cast<double>(q.y);
    obj["z"] = static_cast<double>(q.z);
    obj["w"] = static_cast<double>(q.w);
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializeTransform(const Transform& t) const {
    Json obj;
    obj["position"] = SerializeVector3(t.GetPosition());
    obj["rotation"] = SerializeQuaternion(t.GetRotation());
    obj["scale"] = SerializeVector3(t.GetScale());
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializeMeshReference(const MeshReference& ref, const std::string& projectDir) const {
    Json obj;
    obj["filepath"] = MakeRelativePath(ref.filepath, projectDir);
    obj["isLoaded"] = ref.isLoaded;
    obj["transform"] = SerializeTransform(ref.transform);
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializeAlignmentData(const AlignmentData& data) const {
    Json obj;
    obj["targetMeshTransform"] = SerializeTransform(data.targetMeshTransform);
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializePointCorrespondence(const PointCorrespondence& corr) const {
    Json obj;
    obj["pointID"] = corr.pointID;
    obj["morphMeshPosition"] = SerializeVector3(corr.morphMeshPosition);
    obj["morphMeshVertexIndex"] = corr.morphMeshVertexIndex;
    obj["targetMeshPosition"] = SerializeVector3(corr.targetMeshPosition);
    obj["targetMeshVertexIndex"] = corr.targetMeshVertexIndex;
    obj["isSymmetric"] = corr.isSymmetric;
    obj["symmetricPairID"] = corr.symmetricPairID;
    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializePointReferenceData(const PointReferenceData& data) const {
    Json obj;
    obj["symmetryEnabled"] = data.symmetryEnabled;
    obj["symmetryAxis"] = static_cast<int>(data.symmetryAxis);
    obj["symmetryPlaneOffset"] = static_cast<double>(data.symmetryPlaneOffset);

    Json correspondences = Json::array();
    for (const auto& corr : data.correspondences) {
        correspondences.push_back(SerializePointCorrespondence(corr));
    }
    obj["correspondences"] = std::move(correspondences);

    return obj;
}

ProjectSerializer::Json ProjectSerializer::SerializeMorphData(const MorphData& data) const {
    Json obj;
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
        Json maskRLE = Json::array();
        bool current = data.vertexMask[0];
        int count = 1;
        for (size_t i = 1; i < data.vertexMask.size(); ++i) {
            if (data.vertexMask[i] == current) {
                count++;
            } else {
                maskRLE.push_back(current ? count : -count);
                current = data.vertexMask[i];
                count = 1;
            }
        }
        maskRLE.push_back(current ? count : -count);
        obj["vertexMaskRLE"] = std::move(maskRLE);
        obj["vertexMaskSize"] = static_cast<int>(data.vertexMask.size());
    }

    // Save deformed mesh vertex data if it exists
    if (data.deformedMorphMesh && data.isProcessed) {
        const auto& vertices = data.deformedMorphMesh->GetVertices();
        const auto& normals = data.deformedMorphMesh->GetNormals();

        Json vertexArray = Json::array();
        for (const auto& v : vertices) {
            vertexArray.push_back(static_cast<double>(v.x));
            vertexArray.push_back(static_cast<double>(v.y));
            vertexArray.push_back(static_cast<double>(v.z));
        }
        obj["deformedVertices"] = std::move(vertexArray);

        Json normalArray = Json::array();
        for (const auto& n : normals) {
            normalArray.push_back(static_cast<double>(n.x));
            normalArray.push_back(static_cast<double>(n.y));
            normalArray.push_back(static_cast<double>(n.z));
        }
        obj["deformedNormals"] = std::move(normalArray);

        obj["maxDisplacement"] = static_cast<double>(data.maxDisplacement);
        obj["avgDisplacement"] = static_cast<double>(data.avgDisplacement);
    }

    return obj;
}

// --- Deserialization helpers ---

Vector3 ProjectSerializer::DeserializeVector3(const Json& obj) const {
    return Vector3(
        static_cast<float>(GetOr<double>(obj, "x", 0.0)),
        static_cast<float>(GetOr<double>(obj, "y", 0.0)),
        static_cast<float>(GetOr<double>(obj, "z", 0.0))
    );
}

Quaternion ProjectSerializer::DeserializeQuaternion(const Json& obj) const {
    return Quaternion(
        static_cast<float>(GetOr<double>(obj, "x", 0.0)),
        static_cast<float>(GetOr<double>(obj, "y", 0.0)),
        static_cast<float>(GetOr<double>(obj, "z", 0.0)),
        static_cast<float>(GetOr<double>(obj, "w", 1.0))
    );
}

Transform ProjectSerializer::DeserializeTransform(const Json& obj) const {
    Transform t;
    if (Has(obj, "position")) {
        t.SetPosition(DeserializeVector3(obj["position"]));
    }
    if (Has(obj, "rotation")) {
        t.SetRotation(DeserializeQuaternion(obj["rotation"]));
    }
    if (Has(obj, "scale")) {
        t.SetScale(DeserializeVector3(obj["scale"]));
    }
    return t;
}

MeshReference ProjectSerializer::DeserializeMeshReference(const Json& obj, const std::string& projectDir,
                                                            std::vector<std::string>& warnings) const {
    MeshReference ref;
    const std::string relativePath = GetStr(obj, "filepath");
    ref.filepath = ResolveRelativePath(relativePath, projectDir);
    if (Has(obj, "transform")) {
        ref.transform = DeserializeTransform(obj["transform"]);
    }

    // Try to load the mesh
    if (!ref.filepath.empty() && std::filesystem::exists(ref.filepath)) {
        ref.mesh = std::make_shared<Mesh>();
        if (ref.mesh->Load(ref.filepath)) {
            ref.isLoaded = true;
        } else {
            warnings.push_back(std::format("Failed to load mesh: {}", ref.filepath));
            ref.mesh.reset();
            ref.isLoaded = false;
        }
    } else if (!relativePath.empty()) {
        warnings.push_back(std::format("Mesh file not found: {} (resolved to: {})",
                                        relativePath, ref.filepath));
        ref.isLoaded = false;
    }

    return ref;
}

AlignmentData ProjectSerializer::DeserializeAlignmentData(const Json& obj) const {
    AlignmentData data;
    if (Has(obj, "targetMeshTransform")) {
        data.targetMeshTransform = DeserializeTransform(obj["targetMeshTransform"]);
    }
    return data;
}

PointCorrespondence ProjectSerializer::DeserializePointCorrespondence(const Json& obj) const {
    PointCorrespondence corr;
    corr.pointID = GetOr<int>(obj, "pointID", -1);
    if (Has(obj, "morphMeshPosition"))
        corr.morphMeshPosition = DeserializeVector3(obj["morphMeshPosition"]);
    corr.morphMeshVertexIndex = GetOr<int>(obj, "morphMeshVertexIndex", -1);
    if (Has(obj, "targetMeshPosition"))
        corr.targetMeshPosition = DeserializeVector3(obj["targetMeshPosition"]);
    corr.targetMeshVertexIndex = GetOr<int>(obj, "targetMeshVertexIndex", -1);
    corr.isSymmetric = GetOr<bool>(obj, "isSymmetric", false);
    corr.symmetricPairID = GetOr<int>(obj, "symmetricPairID", -1);
    return corr;
}

PointReferenceData ProjectSerializer::DeserializePointReferenceData(const Json& obj) const {
    PointReferenceData data;
    data.symmetryEnabled = GetOr<bool>(obj, "symmetryEnabled", false);
    data.symmetryAxis = static_cast<Axis>(GetOr<int>(obj, "symmetryAxis", 0));
    data.symmetryPlaneOffset = static_cast<float>(GetOr<double>(obj, "symmetryPlaneOffset", 0.0));

    if (Has(obj, "correspondences") && obj["correspondences"].is_array()) {
        for (const auto& val : obj["correspondences"]) {
            data.correspondences.push_back(DeserializePointCorrespondence(val));
        }
    }

    return data;
}

MorphData ProjectSerializer::DeserializeMorphData(const Json& obj) const {
    MorphData data;
    // Sprint 4 (Wrap Engine Redesign): the enum collapsed to a single member
    // (NRICP). v1.0.0 .mmproj files may carry any of the discontinued ordinals
    // (0=RBF_TPS, 1=RBF_GAUSSIAN, 2=RBF_MULTIQUADRIC, 3=ARAP). Read the field
    // for forward-compat but ignore the value — every project runs through the
    // surviving Wrap Engine path.
    (void)GetOr<int>(obj, "algorithm", static_cast<int>(DeformationAlgorithm::NRICP));
    data.algorithm = DeformationAlgorithm::NRICP;
    data.stiffness = static_cast<float>(GetOr<double>(obj, "stiffness", 0.5));
    data.smoothness = static_cast<float>(GetOr<double>(obj, "smoothness", 0.5));
    data.previewMode = static_cast<MorphPreviewMode>(GetOr<int>(obj, "previewMode", 0));
    data.isProcessed = GetOr<bool>(obj, "isProcessed", false);
    data.isAccepted = GetOr<bool>(obj, "isAccepted", false);
    data.maxDisplacement = static_cast<float>(GetOr<double>(obj, "maxDisplacement", 0.0));
    data.avgDisplacement = static_cast<float>(GetOr<double>(obj, "avgDisplacement", 0.0));

    // NRICP parameters — fallbacks must match the MorphData() constructor
    // defaults in include/core/Project.h. PR #22 review caught nricpAlphaFinal
    // diverging (was 1.0, ctor is 50.0); audit found three more drifts
    // (nricpIcpIterations, nricpSamplingInitial, nricpSamplingFinal). Sprint 4
    // fixed all four — keep this list aligned with the ctor going forward.
    data.nricpStiffnessSteps = GetOr<int>(obj, "nricpStiffnessSteps", 5);
    data.nricpAlphaInitial = static_cast<float>(GetOr<double>(obj, "nricpAlphaInitial", 100.0));
    data.nricpAlphaFinal = static_cast<float>(GetOr<double>(obj, "nricpAlphaFinal", 50.0));
    data.nricpIcpIterations = GetOr<int>(obj, "nricpIcpIterations", 7);
    data.nricpNormalThreshold = static_cast<float>(GetOr<double>(obj, "nricpNormalThreshold", 60.0));
    data.nricpLandmarkWeight = static_cast<float>(GetOr<double>(obj, "nricpLandmarkWeight", 10.0));
    data.nricpEpsilon = static_cast<float>(GetOr<double>(obj, "nricpEpsilon", 1e-4));
    data.nricpEnableBoundaryExclusion = GetOr<bool>(obj, "nricpEnableBoundaryExclusion", true);
    data.nricpBoundaryExclusionHops = GetOr<int>(obj, "nricpBoundaryExclusionHops", 3);

    // NRICP optimization, rigidity, and subsampling parameters
    data.nricpOptimizationIterations = GetOr<int>(obj, "nricpOptimizationIterations", 1);
    data.nricpDpInitial = static_cast<float>(GetOr<double>(obj, "nricpDpInitial", 1.0));
    data.nricpDpFinal = static_cast<float>(GetOr<double>(obj, "nricpDpFinal", 1.0));
    data.nricpGammaInitial = static_cast<float>(GetOr<double>(obj, "nricpGammaInitial", 0.0));
    data.nricpGammaFinal = static_cast<float>(GetOr<double>(obj, "nricpGammaFinal", 0.0));
    data.nricpSamplingInitial = static_cast<float>(GetOr<double>(obj, "nricpSamplingInitial", 0.1));
    data.nricpSamplingFinal = static_cast<float>(GetOr<double>(obj, "nricpSamplingFinal", 0.01));
    data.nricpNormalizeSampling = GetOr<bool>(obj, "nricpNormalizeSampling", true);

    // Restore vertex mask (run-length encoded)
    if (Has(obj, "vertexMaskRLE") && Has(obj, "vertexMaskSize")) {
        const int maskSize = GetOr<int>(obj, "vertexMaskSize", 0);
        const Json& maskRLE = obj["vertexMaskRLE"];
        data.vertexMask.assign(static_cast<size_t>(maskSize), false);
        int idx = 0;
        if (maskRLE.is_array()) {
            for (size_t i = 0; i < maskRLE.size() && idx < maskSize; ++i) {
                int run = maskRLE[i].is_number_integer() ? maskRLE[i].get<int>() : 0;
                bool value = (run > 0);
                int runLength = std::abs(run);
                for (int j = 0; j < runLength && idx < maskSize; ++j) {
                    data.vertexMask[idx++] = value;
                }
            }
        }
    }

    // Restore deformed mesh vertex data if present
    if (Has(obj, "deformedVertices") && Has(obj, "deformedNormals")) {
        const Json& vertexArray = obj["deformedVertices"];
        const Json& normalArray = obj["deformedNormals"];

        if (vertexArray.is_array() && vertexArray.size() >= 3 && vertexArray.size() % 3 == 0) {
            std::vector<Vector3> vertices;
            vertices.reserve(vertexArray.size() / 3);
            for (size_t i = 0; i < vertexArray.size(); i += 3) {
                vertices.push_back(Vector3(
                    static_cast<float>(vertexArray[i].get<double>()),
                    static_cast<float>(vertexArray[i + 1].get<double>()),
                    static_cast<float>(vertexArray[i + 2].get<double>())
                ));
            }

            std::vector<Vector3> normals;
            normals.reserve(normalArray.size() / 3);
            for (size_t i = 0; i < normalArray.size(); i += 3) {
                normals.push_back(Vector3(
                    static_cast<float>(normalArray[i].get<double>()),
                    static_cast<float>(normalArray[i + 1].get<double>()),
                    static_cast<float>(normalArray[i + 2].get<double>())
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

std::string ProjectSerializer::MakeRelativePath(const std::string& filepath, const std::string& projectDir) const {
    if (filepath.empty()) return "";
    std::error_code ec;
    auto rel = std::filesystem::relative(filepath, projectDir, ec);
    if (ec) return filepath;
    std::string result = rel.generic_string();
    return result;
}

std::string ProjectSerializer::ResolveRelativePath(const std::string& relativePath, const std::string& projectDir) const {
    if (relativePath.empty()) return "";

    std::filesystem::path p(relativePath);
    if (p.is_absolute()) return relativePath;

    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(projectDir) / p, ec);
    if (ec) return (std::filesystem::path(projectDir) / p).generic_string();
    return resolved.generic_string();
}

ProjectSerializer::Json ProjectSerializer::SerializeTextureSet(const TextureSet& textures, const std::string& projectDir) const {
    Json obj;

    // New v3 format: entries array
    Json entries = Json::array();
    for (const auto& tex : textures.textures) {
        if (tex.data && tex.data->IsLoaded()) {
            Json entry;
            entry["name"] = tex.name;
            entry["path"] = MakeRelativePath(tex.data->GetFilePath(), projectDir);
            entries.push_back(std::move(entry));
        }
    }
    obj["entries"] = std::move(entries);

    return obj;
}

TextureSet ProjectSerializer::DeserializeTextureSet(const Json& obj, const std::string& projectDir,
                                                     std::vector<std::string>& warnings) const {
    TextureSet textures;

    // New v3 format: entries array
    if (Has(obj, "entries")) {
        const Json& entries = obj["entries"];
        if (entries.is_array()) {
            for (const auto& entry : entries) {
                const std::string name = GetStr(entry, "name");
                const std::string path = ResolveRelativePath(GetStr(entry, "path"), projectDir);
                if (std::filesystem::exists(path)) {
                    auto data = TextureData::LoadFromFile(path);
                    if (data) {
                        textures.Add(name, data);
                    } else {
                        warnings.push_back(std::format("Failed to load texture '{}': {}", name, path));
                    }
                } else {
                    warnings.push_back(std::format("Texture '{}' not found: {}", name, path));
                }
            }
        }
        return textures;
    }

    // Backward compat: v1/v2 format with albedoPath/normalMapPath
    if (Has(obj, "albedoPath")) {
        const std::string path = ResolveRelativePath(GetStr(obj, "albedoPath"), projectDir);
        if (std::filesystem::exists(path)) {
            auto data = TextureData::LoadFromFile(path);
            if (data) {
                textures.Add("Albedo", data);
            } else {
                warnings.push_back(std::format("Failed to load albedo texture: {}", path));
            }
        } else {
            warnings.push_back(std::format("Albedo texture not found: {}", path));
        }
    }

    if (Has(obj, "normalMapPath")) {
        const std::string path = ResolveRelativePath(GetStr(obj, "normalMapPath"), projectDir);
        if (std::filesystem::exists(path)) {
            auto data = TextureData::LoadFromFile(path);
            if (data) {
                textures.Add("Normal Map", data);
            } else {
                warnings.push_back(std::format("Failed to load normal map: {}", path));
            }
        } else {
            warnings.push_back(std::format("Normal map not found: {}", path));
        }
    }

    return textures;
}

} // namespace MetaVisage
