#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include "core/Project.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace MetaVisage {

struct SerializationResult {
    bool success;
    QString errorMessage;
    QStringList warnings;

    SerializationResult() : success(false) {}
};

class ProjectSerializer {
public:
    ProjectSerializer();
    ~ProjectSerializer();

    // Save project to .mmproj file (JSON format)
    SerializationResult Save(const Project& project, const QString& filepath);

    // Load project from .mmproj file
    SerializationResult Load(Project& project, const QString& filepath);

private:
    // Serialization helpers
    QJsonObject SerializeVector3(const Vector3& v) const;
    QJsonObject SerializeQuaternion(const Quaternion& q) const;
    QJsonObject SerializeTransform(const Transform& t) const;
    QJsonObject SerializeMeshReference(const MeshReference& ref, const QString& projectDir) const;
    QJsonObject SerializeAlignmentData(const AlignmentData& data) const;
    QJsonObject SerializePointCorrespondence(const PointCorrespondence& corr) const;
    QJsonObject SerializePointReferenceData(const PointReferenceData& data) const;
    QJsonObject SerializeMorphData(const MorphData& data) const;
    QJsonObject SerializeTextureSet(const TextureSet& textures, const QString& projectDir) const;

    // Deserialization helpers
    Vector3 DeserializeVector3(const QJsonObject& obj) const;
    Quaternion DeserializeQuaternion(const QJsonObject& obj) const;
    Transform DeserializeTransform(const QJsonObject& obj) const;
    MeshReference DeserializeMeshReference(const QJsonObject& obj, const QString& projectDir,
                                            QStringList& warnings) const;
    AlignmentData DeserializeAlignmentData(const QJsonObject& obj) const;
    PointCorrespondence DeserializePointCorrespondence(const QJsonObject& obj) const;
    PointReferenceData DeserializePointReferenceData(const QJsonObject& obj) const;
    MorphData DeserializeMorphData(const QJsonObject& obj) const;
    TextureSet DeserializeTextureSet(const QJsonObject& obj, const QString& projectDir,
                                      QStringList& warnings) const;

    // Path utilities
    QString MakeRelativePath(const QString& filepath, const QString& projectDir) const;
    QString ResolveRelativePath(const QString& relativePath, const QString& projectDir) const;

    static const int FORMAT_VERSION = 2;
};

} // namespace MetaVisage

#endif // PROJECTSERIALIZER_H
