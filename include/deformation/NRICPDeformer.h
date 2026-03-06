#ifndef NRICP_DEFORMER_H
#define NRICP_DEFORMER_H

#include "core/Types.h"
#include "core/Mesh.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <vector>
#include <functional>
#include <atomic>
#include <set>
#include <utility>

namespace MetaVisage {

// Parameters for the NRICP algorithm
struct NRICPParams {
    int stiffnessSteps;          // Number of coarse-to-fine stiffness levels
    float alphaInitial;          // Initial stiffness weight (high = rigid)
    float alphaFinal;            // Final stiffness weight (low = flexible)
    int icpIterations;           // ICP iterations per stiffness level
    float normalThreshold;       // Max normal angle for correspondence (degrees)
    float landmarkWeight;        // Weight for user-defined landmarks
    float epsilon;               // Convergence threshold for early termination

    NRICPParams()
        : stiffnessSteps(5), alphaInitial(100.0f), alphaFinal(1.0f),
          icpIterations(3), normalThreshold(60.0f),
          landmarkWeight(10.0f), epsilon(1e-4f) {}
};

// Edge in the mesh graph for stiffness regularization
struct MeshEdge {
    int v0, v1;
};

using NRICPProgressCallback = std::function<void(float progress, const std::string& message)>;

class NRICPDeformer {
public:
    NRICPDeformer();
    ~NRICPDeformer();

    // Set source (floating/morph) mesh data
    void SetSourceMesh(const std::vector<Vector3>& vertices,
                       const std::vector<Vector3>& normals,
                       const std::vector<Face>& faces);

    // Set target (fixed) mesh - used for closest-point queries
    void SetTargetMesh(std::shared_ptr<Mesh> targetMesh);

    // Set landmark correspondences (source vertex index -> target position in source local space)
    void SetLandmarks(const std::vector<std::pair<int, Vector3>>& landmarks);

    // Set algorithm parameters
    void SetParams(const NRICPParams& params);

    // Set progress callback
    void SetProgressCallback(NRICPProgressCallback callback);

    // Set cancellation flag reference
    void SetCancellationFlag(std::atomic<bool>* cancelled);

    // Execute NRICP - returns deformed vertex positions (empty on failure/cancel)
    std::vector<Vector3> Solve();

private:
    // Build the mesh edge adjacency list from faces
    void BuildEdges();

    // Find correspondences for all source vertices against target mesh
    // Applies normal angle threshold filtering
    void FindCorrespondences(const std::vector<Vector3>& currentPositions,
                             const std::vector<Vector3>& currentNormals,
                             float normalThresholdCos,
                             std::vector<int>& correspondenceIndices,
                             std::vector<Vector3>& correspondencePoints);

    // Build and solve the sparse linear system for one NRICP step
    // X is 4N x 3 matrix (per-vertex affine transforms)
    void SolveLinearSystem(float alpha,
                           const std::vector<int>& corrIndices,
                           const std::vector<Vector3>& corrPoints,
                           Eigen::MatrixXd& X);

    // Apply the solved transformations to produce new vertex positions
    void ApplyTransformations(const Eigen::MatrixXd& X,
                              std::vector<Vector3>& outPositions);

    // Compute per-vertex normals from current positions and faces
    void ComputeNormals(const std::vector<Vector3>& positions,
                        std::vector<Vector3>& normals);

    // Compute RMS displacement between two position sets (for convergence check)
    float ComputeRMSDisplacement(const std::vector<Vector3>& posA,
                                 const std::vector<Vector3>& posB);

    // Source mesh data
    std::vector<Vector3> sourceVertices_;
    std::vector<Vector3> sourceNormals_;
    std::vector<Face> sourceFaces_;
    std::vector<MeshEdge> edges_;
    int vertexCount_;

    // Target mesh (owns BVH for closest-point queries)
    std::shared_ptr<Mesh> targetMesh_;

    // Landmarks: pairs of (source vertex index, target position)
    std::vector<std::pair<int, Vector3>> landmarks_;

    // Parameters
    NRICPParams params_;

    // Progress
    NRICPProgressCallback progressCallback_;
    std::atomic<bool>* cancelled_;
};

} // namespace MetaVisage

#endif // NRICP_DEFORMER_H
