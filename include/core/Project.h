#ifndef PROJECT_H
#define PROJECT_H

#include "core/Types.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include <QString>
#include <QDateTime>
#include <memory>

namespace MetaVisage {

struct MeshReference {
    QString filepath;
    Transform transform;
    bool isLoaded;
    std::shared_ptr<Mesh> mesh;

    MeshReference() : isLoaded(false) {}
};

struct AlignmentData {
    Transform targetMeshTransform;
};

struct PointCorrespondence {
    int pointID;
    Vector3 morphMeshPosition;
    int morphMeshVertexIndex;
    Vector3 targetMeshPosition;
    int targetMeshVertexIndex;
    bool isSymmetric;
    int symmetricPairID;

    PointCorrespondence() : pointID(-1), morphMeshVertexIndex(-1),
                            targetMeshVertexIndex(-1), isSymmetric(false),
                            symmetricPairID(-1) {}
};

struct PointReferenceData {
    std::vector<PointCorrespondence> correspondences;
    bool symmetryEnabled;
    Axis symmetryAxis;
    float symmetryPlaneOffset;

    PointReferenceData() : symmetryEnabled(false), symmetryAxis(Axis::X),
                           symmetryPlaneOffset(0.0f) {}
};

enum class DeformationAlgorithm {
    RBF_TPS,
    RBF_GAUSSIAN,
    RBF_MULTIQUADRIC,
    ARAP
};

struct MorphData {
    DeformationAlgorithm algorithm;
    float stiffness;
    float smoothness;
    std::shared_ptr<Mesh> originalMorphMesh;
    std::shared_ptr<Mesh> deformedMorphMesh;
    bool isProcessed;
    bool isAccepted;
    MorphPreviewMode previewMode;
    std::vector<float> displacementMagnitudes;
    float maxDisplacement;
    float avgDisplacement;

    // Temporary fields used during project deserialization to restore deformed mesh
    bool hasDeformedData;
    std::vector<Vector3> savedDeformedVertices;
    std::vector<Vector3> savedDeformedNormals;

    MorphData() : algorithm(DeformationAlgorithm::RBF_TPS),
                  stiffness(0.5f), smoothness(0.5f),
                  isProcessed(false), isAccepted(false),
                  previewMode(MorphPreviewMode::Deformed),
                  maxDisplacement(0.0f), avgDisplacement(0.0f),
                  hasDeformedData(false) {}
};

class Project {
public:
    Project();
    ~Project();

    // File operations
    bool Save(const QString& filepath);
    bool Load(const QString& filepath);

    // Accessors
    const QString& GetName() const { return projectName_; }
    const QString& GetPath() const { return projectPath_; }
    const QDateTime& GetCreated() const { return created_; }
    const QDateTime& GetLastModified() const { return lastModified_; }
    WorkflowStage GetCurrentStage() const { return currentStage_; }

    MeshReference& GetMorphMesh() { return morphMesh_; }
    MeshReference& GetTargetMesh() { return targetMesh_; }
    AlignmentData& GetAlignmentData() { return alignment_; }
    PointReferenceData& GetPointReferenceData() { return pointReference_; }
    MorphData& GetMorphData() { return morph_; }

    const MeshReference& GetMorphMesh() const { return morphMesh_; }
    const MeshReference& GetTargetMesh() const { return targetMesh_; }
    const AlignmentData& GetAlignmentData() const { return alignment_; }
    const PointReferenceData& GetPointReferenceData() const { return pointReference_; }
    const MorphData& GetMorphData() const { return morph_; }

    // Modifiers
    void SetName(const QString& name) { projectName_ = name; }
    void SetPath(const QString& path) { projectPath_ = path; }
    void SetCurrentStage(WorkflowStage stage) { currentStage_ = stage; }

    // Validation
    bool CanProceedToNextStage() const;

private:
    QString projectName_;
    QString projectPath_;
    QDateTime created_;
    QDateTime lastModified_;
    WorkflowStage currentStage_;

    MeshReference morphMesh_;
    MeshReference targetMesh_;
    AlignmentData alignment_;
    PointReferenceData pointReference_;
    MorphData morph_;
};

} // namespace MetaVisage

#endif // PROJECT_H
