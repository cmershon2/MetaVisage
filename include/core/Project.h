#ifndef PROJECT_H
#define PROJECT_H

#include "core/Types.h"
#include "core/Mesh.h"
#include "core/TextureData.h"
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
    ARAP,
    NRICP
};

struct MorphData {
    DeformationAlgorithm algorithm;
    float stiffness;
    float smoothness;

    // NRICP parameters (only used when algorithm == NRICP)
    int nricpStiffnessSteps;       // Number of coarse-to-fine levels
    float nricpAlphaInitial;       // Initial stiffness weight (high = rigid)
    float nricpAlphaFinal;         // Final stiffness weight (low = flexible)
    int nricpIcpIterations;        // ICP iterations per stiffness level
    float nricpNormalThreshold;    // Max normal angle for correspondence (degrees)
    float nricpLandmarkWeight;     // Weight for user-defined landmarks
    float nricpEpsilon;            // Convergence threshold

    // NRICP boundary exclusion parameters
    bool nricpEnableBoundaryExclusion;   // Auto-exclude boundary/interior regions from ICP
    int nricpBoundaryExclusionHops;      // Edge hops from boundary to exclude

    // NRICP optimization iterations and delta
    int nricpOptimizationIterations;     // Inner optimization iterations per ICP step (1 = disabled)
    float nricpDpInitial;               // Initial step-size damping (1.0 = no damping)
    float nricpDpFinal;                 // Final step-size damping (1.0 = no damping)

    // NRICP rigidity regularization
    float nricpGammaInitial;            // Initial ARAP rigidity weight (0 = disabled)
    float nricpGammaFinal;              // Final ARAP rigidity weight (0 = disabled)

    // NRICP control node subsampling
    float nricpSamplingInitial;         // Initial control node sampling (0 = all vertices)
    float nricpSamplingFinal;           // Final control node sampling (0 = all vertices)
    bool nricpNormalizeSampling;        // Sampling values relative to bbox diagonal

    // User-painted vertex mask: true = excluded from NRICP correspondence
    std::vector<bool> vertexMask;

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

    MorphData() : algorithm(DeformationAlgorithm::NRICP),
                  stiffness(0.5f), smoothness(0.5f),
                  nricpStiffnessSteps(5),
                  nricpAlphaInitial(100.0f), nricpAlphaFinal(50.0f),
                  nricpIcpIterations(7), nricpNormalThreshold(60.0f),
                  nricpLandmarkWeight(10.0f), nricpEpsilon(1e-4f),
                  nricpEnableBoundaryExclusion(true), nricpBoundaryExclusionHops(3),
                  nricpOptimizationIterations(1),
                  nricpDpInitial(1.0f), nricpDpFinal(1.0f),
                  nricpGammaInitial(0.0f), nricpGammaFinal(0.0f),
                  nricpSamplingInitial(0.1f), nricpSamplingFinal(0.01f),
                  nricpNormalizeSampling(true),
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
    TextureSet& GetTargetTextures() { return targetTextures_; }

    const MeshReference& GetMorphMesh() const { return morphMesh_; }
    const MeshReference& GetTargetMesh() const { return targetMesh_; }
    const AlignmentData& GetAlignmentData() const { return alignment_; }
    const PointReferenceData& GetPointReferenceData() const { return pointReference_; }
    const MorphData& GetMorphData() const { return morph_; }
    const TextureSet& GetTargetTextures() const { return targetTextures_; }

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
    TextureSet targetTextures_;
};

} // namespace MetaVisage

#endif // PROJECT_H
