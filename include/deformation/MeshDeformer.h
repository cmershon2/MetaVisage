#ifndef MESH_DEFORMER_H
#define MESH_DEFORMER_H

#include "core/Types.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Project.h"
#include "deformation/RBFInterpolator.h"
#include "deformation/NRICPDeformer.h"
#include <memory>
#include <atomic>
#include <functional>

namespace MetaVisage {

// Progress callback: receives progress (0.0-1.0) and status message
using ProgressCallback = std::function<void(float progress, const std::string& message)>;

// Result of a deformation operation
struct DeformationResult {
    bool success;
    std::string errorMessage;
    std::shared_ptr<Mesh> deformedMesh;
    float maxDisplacement;    // Maximum vertex displacement magnitude
    float avgDisplacement;    // Average vertex displacement magnitude

    DeformationResult() : success(false), maxDisplacement(0.0f), avgDisplacement(0.0f) {}
};

class MeshDeformer {
public:
    MeshDeformer();
    ~MeshDeformer();

    // Set the source mesh to deform (morph mesh)
    void SetSourceMesh(std::shared_ptr<Mesh> mesh);

    // Set the target mesh (for vertex position lookups)
    void SetTargetMesh(std::shared_ptr<Mesh> mesh);

    // Set transforms for coordinate space conversion
    void SetMorphTransform(const Transform& transform);
    void SetTargetTransform(const Transform& transform);

    // Set point correspondences from the project
    void SetCorrespondences(const std::vector<PointCorrespondence>& correspondences);

    // Set deformation parameters
    void SetKernelType(DeformationAlgorithm kernel);
    void SetStiffness(float stiffness);
    void SetSmoothness(float smoothness);

    // Set NRICP-specific parameters
    void SetNRICPParams(const NRICPParams& params);

    // Set progress callback
    void SetProgressCallback(ProgressCallback callback);

    // Request cancellation of ongoing deformation
    void Cancel();

    // Check if cancellation was requested
    bool IsCancelled() const { return cancelled_.load(); }

    // Perform the deformation
    // Returns a DeformationResult containing the deformed mesh
    DeformationResult Deform();

    // Validate inputs before deformation
    bool ValidateInputs(std::string& errorMessage) const;

    // Get per-vertex displacement magnitudes (for heat map visualization)
    const std::vector<float>& GetDisplacementMagnitudes() const { return displacementMagnitudes_; }

private:
    // Create a deep copy of the source mesh
    std::shared_ptr<Mesh> CopyMesh(const std::shared_ptr<Mesh>& source) const;

    // Extract source and target points from correspondences
    void ExtractControlPoints(std::vector<Vector3>& sourcePoints,
                              std::vector<Vector3>& targetPoints) const;

    // Helper to transform a point by a 4x4 matrix
    static Vector3 TransformPoint(const Vector3& point, const Matrix4x4& matrix);

    // NRICP deformation path
    DeformationResult DeformNRICP();

    // Create a copy of target mesh with vertices in morph local space
    std::shared_ptr<Mesh> CreateTargetMeshInMorphSpace() const;

    // Find the closest source mesh vertex to a given position
    static int FindClosestVertex(const std::vector<Vector3>& vertices, const Vector3& point);

    // Source mesh, target mesh, and correspondences
    std::shared_ptr<Mesh> sourceMesh_;
    std::shared_ptr<Mesh> targetMesh_;
    Transform morphTransform_;
    Transform targetTransform_;
    std::vector<PointCorrespondence> correspondences_;

    // Parameters
    DeformationAlgorithm kernelType_;
    float stiffness_;
    float smoothness_;
    NRICPParams nricpParams_;

    // Progress tracking
    ProgressCallback progressCallback_;
    std::atomic<bool> cancelled_;

    // Results
    std::vector<float> displacementMagnitudes_;
};

} // namespace MetaVisage

#endif // MESH_DEFORMER_H
