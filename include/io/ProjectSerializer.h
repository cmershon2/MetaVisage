#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include "core/Project.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace MetaVisage {

struct SerializationResult {
    bool success;
    std::string errorMessage;
    std::vector<std::string> warnings;

    SerializationResult() : success(false) {}
};

class ProjectSerializer {
public:
    using Json = nlohmann::ordered_json;

    ProjectSerializer();
    ~ProjectSerializer();

    // Save project to .mmproj file (JSON format)
    SerializationResult Save(const Project& project, const std::string& filepath);

    // Load project from .mmproj file
    SerializationResult Load(Project& project, const std::string& filepath);

private:
    // Serialization helpers
    Json SerializeVector3(const Vector3& v) const;
    Json SerializeQuaternion(const Quaternion& q) const;
    Json SerializeTransform(const Transform& t) const;
    Json SerializeMeshReference(const MeshReference& ref, const std::string& projectDir) const;
    Json SerializeAlignmentData(const AlignmentData& data) const;
    Json SerializePointCorrespondence(const PointCorrespondence& corr) const;
    Json SerializePointReferenceData(const PointReferenceData& data) const;
    Json SerializeMorphData(const MorphData& data) const;
    Json SerializeTextureSet(const TextureSet& textures, const std::string& projectDir) const;

    // Deserialization helpers
    Vector3 DeserializeVector3(const Json& obj) const;
    Quaternion DeserializeQuaternion(const Json& obj) const;
    Transform DeserializeTransform(const Json& obj) const;
    MeshReference DeserializeMeshReference(const Json& obj, const std::string& projectDir,
                                            std::vector<std::string>& warnings) const;
    AlignmentData DeserializeAlignmentData(const Json& obj) const;
    PointCorrespondence DeserializePointCorrespondence(const Json& obj) const;
    PointReferenceData DeserializePointReferenceData(const Json& obj) const;
    MorphData DeserializeMorphData(const Json& obj) const;
    TextureSet DeserializeTextureSet(const Json& obj, const std::string& projectDir,
                                      std::vector<std::string>& warnings) const;

    // Path utilities
    std::string MakeRelativePath(const std::string& filepath, const std::string& projectDir) const;
    std::string ResolveRelativePath(const std::string& relativePath, const std::string& projectDir) const;

    // v3 (Sprint 11): adds sourceProfileId. v1 / v2 files load with an
    // empty id.
    static const int FORMAT_VERSION = 3;
};

} // namespace MetaVisage

#endif // PROJECTSERIALIZER_H
