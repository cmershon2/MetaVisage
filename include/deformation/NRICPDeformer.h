#ifndef NRICP_DEFORMER_H
#define NRICP_DEFORMER_H

#include "core/Types.h"
#include "core/Mesh.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SVD>
#include <vector>
#include <functional>
#include <atomic>
#include <set>
#include <utility>
#include <map>
#include <queue>

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

    // Boundary exclusion parameters
    bool enableBoundaryExclusion;   // Auto-detect and exclude boundary/interior regions from ICP
    int boundaryExclusionHops;      // Number of edge hops from boundary edges to exclude
    float maxCorrespondenceDistance; // Max allowed ICP correspondence distance (-1.0 = auto from bbox)

    // Inner optimization iterations per ICP step
    int optimizationIterations;     // Number of optimization sub-steps (1 = current behavior)

    // Optimization delta (step-size damping)
    float dpInitial;                // Initial damping factor (1.0 = no damping)
    float dpFinal;                  // Final damping factor (1.0 = no damping)

    // Rigidity regularization (ARAP term)
    float gammaInitial;             // Initial rigidity weight (0 = disabled)
    float gammaFinal;               // Final rigidity weight (0 = disabled)

    // Control node subsampling
    float samplingInitial;          // Initial voxel size for control nodes (0 = all vertices)
    float samplingFinal;            // Final voxel size for control nodes (0 = all vertices)
    bool normalizeSampling;         // If true, sampling values are relative to bbox diagonal

    NRICPParams()
        : stiffnessSteps(3), alphaInitial(100.0f), alphaFinal(20.0f),
          icpIterations(7), normalThreshold(60.0f),
          landmarkWeight(10.0f), epsilon(1e-4f),
          enableBoundaryExclusion(true), boundaryExclusionHops(3),
          maxCorrespondenceDistance(-1.0f),
          optimizationIterations(20),
          dpInitial(1.0f), dpFinal(0.1f),
          gammaInitial(0.0f), gammaFinal(0.0f),
          samplingInitial(0.1f), samplingFinal(0.001f),
          normalizeSampling(true) {}
};

// Edge in the mesh graph for stiffness regularization
struct MeshEdge {
    int v0, v1;
};

// Control node subsampling data
struct ControlNodeData {
    std::vector<int> controlNodeIndices;                          // K indices into source vertices
    std::vector<MeshEdge> controlEdges;                           // kNN edges among control nodes
    std::vector<std::vector<std::pair<int, float>>> interpolationWeights; // per-vertex: (controlLocalIdx, weight)
    int controlNodeCount = 0;
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

    // Detect boundary vertices (near open edges) and mark exclusion zone via BFS
    void DetectAndExcludeBoundaryVertices();

    // Find correspondences for all source vertices against target mesh
    // Applies normal angle threshold filtering
    void FindCorrespondences(const std::vector<Vector3>& currentPositions,
                             const std::vector<Vector3>& currentNormals,
                             float normalThresholdCos,
                             std::vector<int>& correspondenceIndices,
                             std::vector<Vector3>& correspondencePoints);

    // Build and solve the sparse linear system for one NRICP step
    // X is 4K x 3 matrix (per-node affine transforms, K = nodeCount)
    void SolveLinearSystem(float alpha,
                           float gamma,
                           const std::vector<int>& corrIndices,
                           const std::vector<Vector3>& corrPoints,
                           const ControlNodeData* controlData,
                           Eigen::MatrixXd& X);

    // Apply the solved transformations to produce new vertex positions (all-vertex path)
    void ApplyTransformations(const Eigen::MatrixXd& X,
                              std::vector<Vector3>& outPositions);

    // Apply transforms using control node interpolation (subsampled path)
    void ApplyTransformationsSubsampled(const Eigen::MatrixXd& X,
                                        const ControlNodeData& controlData,
                                        std::vector<Vector3>& outPositions);

    // Compute closest rotation matrices from current per-node affine transforms (ARAP)
    void ComputeRotationTargets(const Eigen::MatrixXd& X, int nodeCount);

    // Control node subsampling methods
    ControlNodeData ComputeControlNodes(float voxelSize);
    void BuildControlNodeEdges(ControlNodeData& data, int kNeighbors = 8);
    void ComputeInterpolationWeights(ControlNodeData& data, int kNearest = 4);

    // Compute per-vertex normals from current positions and faces
    void ComputeNormals(const std::vector<Vector3>& positions,
                        std::vector<Vector3>& normals);

    // Compute RMS displacement between two position sets (for convergence check)
    float ComputeRMSDisplacement(const std::vector<Vector3>& posA,
                                 const std::vector<Vector3>& posB);

    // Build weld map: find groups of vertices sharing the same position (UV seam duplicates)
    void BuildWeldMap();

    // After deformation, enforce exact position coincidence within each weld group
    void EnforceWeldConstraints(std::vector<Vector3>& positions);

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

    // Boundary exclusion state
    std::vector<bool> excludedVertices_;
    std::vector<std::vector<int>> vertexAdjacency_;
    float resolvedMaxCorrespondenceDistance_;

    // Rigidity regularization state (ARAP rotation targets)
    std::vector<Eigen::Matrix3d> rotationTargets_;

    // Control node subsampling state
    ControlNodeData controlNodeData_;

    // Cached bounding box for sampling normalization
    Vector3 bboxMin_, bboxMax_;
    float bboxDiagonal_;

    // Weld map: groups of vertex indices sharing the same position (UV seam duplicates)
    std::vector<std::vector<int>> weldGroups_;       // Each group has 2+ vertices at same position
    std::vector<int> weldGroupIndex_;                // Per-vertex: index into weldGroups_ (-1 if not welded)
};

} // namespace MetaVisage

#endif // NRICP_DEFORMER_H
