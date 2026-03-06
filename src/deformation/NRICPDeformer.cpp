#include "deformation/NRICPDeformer.h"
#include "utils/BVH.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <set>
#include <map>
#include <queue>
#include <tuple>

namespace MetaVisage {

NRICPDeformer::NRICPDeformer()
    : vertexCount_(0), cancelled_(nullptr), resolvedMaxCorrespondenceDistance_(-1.0f),
      bboxDiagonal_(0.0f) {
}

NRICPDeformer::~NRICPDeformer() {
}

void NRICPDeformer::SetSourceMesh(const std::vector<Vector3>& vertices,
                                   const std::vector<Vector3>& normals,
                                   const std::vector<Face>& faces) {
    sourceVertices_ = vertices;
    sourceNormals_ = normals;
    sourceFaces_ = faces;
    vertexCount_ = static_cast<int>(vertices.size());
}

void NRICPDeformer::SetTargetMesh(std::shared_ptr<Mesh> targetMesh) {
    targetMesh_ = targetMesh;
}

void NRICPDeformer::SetLandmarks(const std::vector<std::pair<int, Vector3>>& landmarks) {
    landmarks_ = landmarks;
}

void NRICPDeformer::SetParams(const NRICPParams& params) {
    params_ = params;
}

void NRICPDeformer::SetProgressCallback(NRICPProgressCallback callback) {
    progressCallback_ = callback;
}

void NRICPDeformer::SetCancellationFlag(std::atomic<bool>* cancelled) {
    cancelled_ = cancelled;
}

void NRICPDeformer::BuildEdges() {
    edges_.clear();
    std::set<std::pair<int, int>> edgeSet;

    for (const auto& face : sourceFaces_) {
        const auto& indices = face.vertexIndices;
        if (indices.size() < 3) continue;

        // Fan triangulation for n-gons, collect edges
        for (size_t i = 0; i < indices.size(); ++i) {
            int a = static_cast<int>(indices[i]);
            int b = static_cast<int>(indices[(i + 1) % indices.size()]);
            int lo = std::min(a, b);
            int hi = std::max(a, b);
            edgeSet.insert({lo, hi});
        }
    }

    edges_.reserve(edgeSet.size());
    for (const auto& [v0, v1] : edgeSet) {
        edges_.push_back({v0, v1});
    }

    MV_LOG_INFO(QString("NRICP: Built %1 unique edges from %2 faces")
        .arg(edges_.size()).arg(sourceFaces_.size()));
}

void NRICPDeformer::DetectAndExcludeBoundaryVertices() {
    excludedVertices_.assign(vertexCount_, false);

    if (!params_.enableBoundaryExclusion || vertexCount_ == 0) {
        MV_LOG_INFO("NRICP: Boundary exclusion disabled or no vertices");
        return;
    }

    // Step 1: Build edge-to-face-count map to find boundary edges
    std::map<std::pair<int, int>, int> edgeFaceCount;

    for (const auto& face : sourceFaces_) {
        const auto& indices = face.vertexIndices;
        if (indices.size() < 3) continue;

        for (size_t i = 0; i < indices.size(); ++i) {
            int a = static_cast<int>(indices[i]);
            int b = static_cast<int>(indices[(i + 1) % indices.size()]);
            int lo = std::min(a, b);
            int hi = std::max(a, b);
            edgeFaceCount[{lo, hi}]++;
        }
    }

    // Step 2: Find boundary vertices (vertices on edges with face count == 1)
    std::vector<bool> isBoundaryVertex(vertexCount_, false);
    int boundaryEdgeCount = 0;

    for (const auto& [edge, count] : edgeFaceCount) {
        if (count == 1) {
            isBoundaryVertex[edge.first] = true;
            isBoundaryVertex[edge.second] = true;
            boundaryEdgeCount++;
        }
    }

    int boundaryVertexCount = 0;
    for (int i = 0; i < vertexCount_; ++i) {
        if (isBoundaryVertex[i]) boundaryVertexCount++;
    }

    MV_LOG_INFO(QString("NRICP: Found %1 boundary edges, %2 boundary vertices")
        .arg(boundaryEdgeCount).arg(boundaryVertexCount));

    if (boundaryVertexCount == 0) {
        MV_LOG_INFO("NRICP: No boundary vertices found - mesh is closed");
        return;
    }

    // Step 3: Build vertex adjacency list from edges_ (for BFS propagation)
    vertexAdjacency_.assign(vertexCount_, std::vector<int>());
    for (const auto& edge : edges_) {
        vertexAdjacency_[edge.v0].push_back(edge.v1);
        vertexAdjacency_[edge.v1].push_back(edge.v0);
    }

    // Step 4: BFS from all boundary vertices, propagating N hops
    int hops = params_.boundaryExclusionHops;
    std::vector<int> distance(vertexCount_, -1);

    std::queue<int> bfsQueue;
    for (int i = 0; i < vertexCount_; ++i) {
        if (isBoundaryVertex[i]) {
            distance[i] = 0;
            excludedVertices_[i] = true;
            bfsQueue.push(i);
        }
    }

    while (!bfsQueue.empty()) {
        int current = bfsQueue.front();
        bfsQueue.pop();

        if (distance[current] >= hops) continue;

        for (int neighbor : vertexAdjacency_[current]) {
            if (distance[neighbor] == -1) {
                distance[neighbor] = distance[current] + 1;
                excludedVertices_[neighbor] = true;
                bfsQueue.push(neighbor);
            }
        }
    }

    int excludedCount = 0;
    for (int i = 0; i < vertexCount_; ++i) {
        if (excludedVertices_[i]) excludedCount++;
    }

    MV_LOG_INFO(QString("NRICP: Excluded %1 vertices (%2% of mesh) from ICP correspondence (boundary + %3 hops)")
        .arg(excludedCount)
        .arg(static_cast<float>(excludedCount) / vertexCount_ * 100.0f, 0, 'f', 1)
        .arg(hops));
}

void NRICPDeformer::FindCorrespondences(
    const std::vector<Vector3>& currentPositions,
    const std::vector<Vector3>& currentNormals,
    float normalThresholdCos,
    std::vector<int>& correspondenceIndices,
    std::vector<Vector3>& correspondencePoints) {

    correspondenceIndices.clear();
    correspondencePoints.clear();

    if (!targetMesh_) return;

    // Ensure target BVH is built
    BVH* targetBVH = targetMesh_->GetBVH();
    if (!targetBVH || !targetBVH->IsBuilt()) return;

    const auto& targetVertices = targetMesh_->GetVertices();

    // Pre-compute squared max distance for fast comparison
    float maxDistSq = (resolvedMaxCorrespondenceDistance_ > 0.0f)
        ? resolvedMaxCorrespondenceDistance_ * resolvedMaxCorrespondenceDistance_
        : -1.0f;

    for (int i = 0; i < vertexCount_; ++i) {
        // Skip excluded boundary/interior vertices (cheap check before BVH query)
        if (!excludedVertices_.empty() && excludedVertices_[i]) continue;

        BVHClosestPointResult result = targetBVH->FindClosestPoint(currentPositions[i], targetVertices);

        if (!result.found) continue;

        // Distance threshold filtering
        if (maxDistSq > 0.0f && result.distanceSq > maxDistSq) continue;

        // Normal angle filtering
        if (i < static_cast<int>(currentNormals.size()) && currentNormals[i].Length() > 0.01f) {
            float dotProduct = currentNormals[i].Normalized().Dot(result.normal.Normalized());
            if (dotProduct < normalThresholdCos) continue;
        }

        correspondenceIndices.push_back(i);
        correspondencePoints.push_back(result.point);
    }
}

// ============================================================================
// Phase 2: Rigidity regularization - compute closest rotation via SVD
// ============================================================================

void NRICPDeformer::ComputeRotationTargets(const Eigen::MatrixXd& X, int nodeCount) {
    rotationTargets_.resize(nodeCount);

    for (int i = 0; i < nodeCount; ++i) {
        // Extract 3x3 linear part from X block [4i:4i+2, 0:2]
        Eigen::Matrix3d A;
        A(0, 0) = X(4 * i + 0, 0); A(0, 1) = X(4 * i + 0, 1); A(0, 2) = X(4 * i + 0, 2);
        A(1, 0) = X(4 * i + 1, 0); A(1, 1) = X(4 * i + 1, 1); A(1, 2) = X(4 * i + 1, 2);
        A(2, 0) = X(4 * i + 2, 0); A(2, 1) = X(4 * i + 2, 1); A(2, 2) = X(4 * i + 2, 2);

        // Polar decomposition via SVD: A = U * S * V^T => R = U * V^T
        Eigen::JacobiSVD<Eigen::Matrix3d> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::Matrix3d R = svd.matrixU() * svd.matrixV().transpose();

        // Ensure proper rotation (det = +1, not reflection)
        if (R.determinant() < 0.0) {
            Eigen::Matrix3d U = svd.matrixU();
            U.col(2) *= -1.0;
            R = U * svd.matrixV().transpose();
        }

        rotationTargets_[i] = R;
    }
}

// ============================================================================
// Phase 3: Control node subsampling methods
// ============================================================================

ControlNodeData NRICPDeformer::ComputeControlNodes(float voxelSize) {
    ControlNodeData data;

    if (voxelSize <= 0.0f) {
        // Disabled: all vertices are control nodes
        data.controlNodeCount = vertexCount_;
        data.controlNodeIndices.resize(vertexCount_);
        std::iota(data.controlNodeIndices.begin(), data.controlNodeIndices.end(), 0);
        return data;
    }

    // Voxel grid: map each vertex to a voxel, pick closest vertex to voxel center
    float invVoxel = 1.0f / voxelSize;
    // For each voxel, store (vertex index, distance to voxel center)
    std::map<std::tuple<int, int, int>, std::pair<int, float>> voxelBest;

    for (int i = 0; i < vertexCount_; ++i) {
        const Vector3& v = sourceVertices_[i];
        int vx = static_cast<int>(std::floor((v.x - bboxMin_.x) * invVoxel));
        int vy = static_cast<int>(std::floor((v.y - bboxMin_.y) * invVoxel));
        int vz = static_cast<int>(std::floor((v.z - bboxMin_.z) * invVoxel));
        auto key = std::make_tuple(vx, vy, vz);

        // Compute distance to voxel center
        float cx = bboxMin_.x + (vx + 0.5f) * voxelSize;
        float cy = bboxMin_.y + (vy + 0.5f) * voxelSize;
        float cz = bboxMin_.z + (vz + 0.5f) * voxelSize;
        float dx = v.x - cx, dy = v.y - cy, dz = v.z - cz;
        float distSq = dx * dx + dy * dy + dz * dz;

        auto it = voxelBest.find(key);
        if (it == voxelBest.end() || distSq < it->second.second) {
            voxelBest[key] = {i, distSq};
        }
    }

    data.controlNodeIndices.reserve(voxelBest.size());
    for (const auto& [key, val] : voxelBest) {
        data.controlNodeIndices.push_back(val.first);
    }
    std::sort(data.controlNodeIndices.begin(), data.controlNodeIndices.end());
    data.controlNodeCount = static_cast<int>(data.controlNodeIndices.size());

    MV_LOG_INFO(QString("NRICP: Selected %1 control nodes from %2 vertices (voxel size=%3)")
        .arg(data.controlNodeCount).arg(vertexCount_).arg(voxelSize, 0, 'f', 4));

    return data;
}

void NRICPDeformer::BuildControlNodeEdges(ControlNodeData& data, int kNeighbors) {
    data.controlEdges.clear();
    int K = data.controlNodeCount;
    if (K <= 1) return;

    // Build positions array for control nodes
    std::vector<Vector3> controlPositions(K);
    for (int i = 0; i < K; ++i) {
        controlPositions[i] = sourceVertices_[data.controlNodeIndices[i]];
    }

    // For each control node, find k nearest neighbors
    std::set<std::pair<int, int>> edgeSet;
    for (int i = 0; i < K; ++i) {
        // Compute distances to all other control nodes
        std::vector<std::pair<float, int>> dists;
        dists.reserve(K - 1);
        for (int j = 0; j < K; ++j) {
            if (i == j) continue;
            Vector3 diff = controlPositions[j] - controlPositions[i];
            float d = diff.Dot(diff);
            dists.push_back({d, j});
        }

        int count = std::min(kNeighbors, static_cast<int>(dists.size()));
        std::partial_sort(dists.begin(), dists.begin() + count, dists.end());

        for (int n = 0; n < count; ++n) {
            int lo = std::min(i, dists[n].second);
            int hi = std::max(i, dists[n].second);
            edgeSet.insert({lo, hi});
        }
    }

    data.controlEdges.reserve(edgeSet.size());
    for (const auto& [v0, v1] : edgeSet) {
        data.controlEdges.push_back({v0, v1});
    }

    MV_LOG_INFO(QString("NRICP: Built %1 control node edges (k=%2)")
        .arg(data.controlEdges.size()).arg(kNeighbors));
}

void NRICPDeformer::ComputeInterpolationWeights(ControlNodeData& data, int kNearest) {
    int K = data.controlNodeCount;
    data.interpolationWeights.resize(vertexCount_);

    // Build map: source vertex index -> control node local index
    std::vector<int> vertexToControlLocal(vertexCount_, -1);
    for (int i = 0; i < K; ++i) {
        vertexToControlLocal[data.controlNodeIndices[i]] = i;
    }

    std::vector<Vector3> controlPositions(K);
    for (int i = 0; i < K; ++i) {
        controlPositions[i] = sourceVertices_[data.controlNodeIndices[i]];
    }

    int actualK = std::min(kNearest, K);

    for (int v = 0; v < vertexCount_; ++v) {
        int localIdx = vertexToControlLocal[v];
        if (localIdx >= 0) {
            // This vertex IS a control node: weight 1.0 for itself
            data.interpolationWeights[v] = {{localIdx, 1.0f}};
            continue;
        }

        // Find kNearest control nodes by distance
        std::vector<std::pair<float, int>> dists;
        dists.reserve(K);
        for (int c = 0; c < K; ++c) {
            Vector3 diff = controlPositions[c] - sourceVertices_[v];
            float d = diff.Dot(diff); // squared distance for sorting
            dists.push_back({d, c});
        }
        std::partial_sort(dists.begin(), dists.begin() + actualK, dists.end());

        float totalWeight = 0.0f;
        std::vector<std::pair<int, float>> weights;
        weights.reserve(actualK);

        for (int n = 0; n < actualK; ++n) {
            float d = std::sqrt(std::max(dists[n].first, 1e-12f));
            float w = 1.0f / d;
            weights.push_back({dists[n].second, w});
            totalWeight += w;
        }

        // Normalize
        if (totalWeight > 0.0f) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        }

        data.interpolationWeights[v] = std::move(weights);
    }
}

// ============================================================================
// Linear system solve (supports both all-vertex and subsampled paths)
// ============================================================================

void NRICPDeformer::SolveLinearSystem(
    float alpha,
    float gamma,
    const std::vector<int>& corrIndices,
    const std::vector<Vector3>& corrPoints,
    const ControlNodeData* controlData,
    Eigen::MatrixXd& X) {

    // Determine if we're using subsampled control nodes or all vertices
    const bool subsampled = (controlData != nullptr && controlData->controlNodeCount < vertexCount_);

    const int nodeCount = subsampled ? controlData->controlNodeCount : vertexCount_;
    const int cols = 4 * nodeCount;

    // Choose edge set based on path
    const auto& activeEdges = subsampled ? controlData->controlEdges : edges_;

    int stiffRows = 4 * static_cast<int>(activeEdges.size());
    int dataRows = static_cast<int>(corrIndices.size());
    int landmarkRows = static_cast<int>(landmarks_.size());
    int rigidityRows = (gamma > 0.0f && !rotationTargets_.empty()) ? 3 * nodeCount : 0;
    int totalRows = stiffRows + dataRows + landmarkRows + rigidityRows;

    // Build sparse matrix A using triplets
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(stiffRows * 2 + dataRows * 4 + landmarkRows * 4 + rigidityRows);

    // Right-hand side (3 columns for x, y, z)
    Eigen::MatrixXd B = Eigen::MatrixXd::Zero(totalRows, 3);

    int row = 0;

    // --- Stiffness term ---
    // For each edge (i,j), penalize difference in per-node transforms
    double alphaD = static_cast<double>(alpha);
    for (const auto& edge : activeEdges) {
        int i = edge.v0;
        int j = edge.v1;
        for (int k = 0; k < 4; ++k) {
            triplets.emplace_back(row + k, 4 * i + k, alphaD);
            triplets.emplace_back(row + k, 4 * j + k, -alphaD);
        }
        // B rows are zero (stiffness drives uniform transformation)
        row += 4;
    }

    // --- Data term (ICP correspondences) ---
    if (subsampled) {
        // Subsampled path: scatter data term across interpolated control nodes
        for (size_t c = 0; c < corrIndices.size(); ++c) {
            int vi = corrIndices[c];
            const Vector3& sv = sourceVertices_[vi];

            const auto& weights = controlData->interpolationWeights[vi];
            for (const auto& [ci, w] : weights) {
                double wd = static_cast<double>(w);
                triplets.emplace_back(row, 4 * ci + 0, wd * static_cast<double>(sv.x));
                triplets.emplace_back(row, 4 * ci + 1, wd * static_cast<double>(sv.y));
                triplets.emplace_back(row, 4 * ci + 2, wd * static_cast<double>(sv.z));
                triplets.emplace_back(row, 4 * ci + 3, wd);
            }

            B(row, 0) = static_cast<double>(corrPoints[c].x);
            B(row, 1) = static_cast<double>(corrPoints[c].y);
            B(row, 2) = static_cast<double>(corrPoints[c].z);
            row++;
        }
    } else {
        // All-vertex path: direct 1-to-1 mapping
        for (size_t c = 0; c < corrIndices.size(); ++c) {
            int vi = corrIndices[c];
            const Vector3& sv = sourceVertices_[vi];

            triplets.emplace_back(row, 4 * vi + 0, static_cast<double>(sv.x));
            triplets.emplace_back(row, 4 * vi + 1, static_cast<double>(sv.y));
            triplets.emplace_back(row, 4 * vi + 2, static_cast<double>(sv.z));
            triplets.emplace_back(row, 4 * vi + 3, 1.0);

            B(row, 0) = static_cast<double>(corrPoints[c].x);
            B(row, 1) = static_cast<double>(corrPoints[c].y);
            B(row, 2) = static_cast<double>(corrPoints[c].z);
            row++;
        }
    }

    // --- Landmark term ---
    double wl = static_cast<double>(params_.landmarkWeight);
    if (subsampled) {
        // Subsampled path: scatter landmark term across interpolated control nodes
        for (const auto& [vi, target] : landmarks_) {
            if (vi < 0 || vi >= vertexCount_) continue;
            const Vector3& sv = sourceVertices_[vi];

            const auto& weights = controlData->interpolationWeights[vi];
            for (const auto& [ci, w] : weights) {
                double wd = wl * static_cast<double>(w);
                triplets.emplace_back(row, 4 * ci + 0, wd * static_cast<double>(sv.x));
                triplets.emplace_back(row, 4 * ci + 1, wd * static_cast<double>(sv.y));
                triplets.emplace_back(row, 4 * ci + 2, wd * static_cast<double>(sv.z));
                triplets.emplace_back(row, 4 * ci + 3, wd);
            }

            B(row, 0) = wl * static_cast<double>(target.x);
            B(row, 1) = wl * static_cast<double>(target.y);
            B(row, 2) = wl * static_cast<double>(target.z);
            row++;
        }
    } else {
        // All-vertex path: direct mapping
        for (const auto& [vi, target] : landmarks_) {
            if (vi < 0 || vi >= nodeCount) continue;
            const Vector3& sv = sourceVertices_[vi];

            triplets.emplace_back(row, 4 * vi + 0, wl * static_cast<double>(sv.x));
            triplets.emplace_back(row, 4 * vi + 1, wl * static_cast<double>(sv.y));
            triplets.emplace_back(row, 4 * vi + 2, wl * static_cast<double>(sv.z));
            triplets.emplace_back(row, 4 * vi + 3, wl);

            B(row, 0) = wl * static_cast<double>(target.x);
            B(row, 1) = wl * static_cast<double>(target.y);
            B(row, 2) = wl * static_cast<double>(target.z);
            row++;
        }
    }

    // --- Rigidity regularization term (ARAP) ---
    if (gamma > 0.0f && !rotationTargets_.empty()) {
        double gammaD = static_cast<double>(gamma);
        int rigidityNodeCount = std::min(nodeCount, static_cast<int>(rotationTargets_.size()));
        for (int i = 0; i < rigidityNodeCount; ++i) {
            const Eigen::Matrix3d& R = rotationTargets_[i];
            // Penalize: gamma * (X[4i+k, :] - R[k, :]) for k=0,1,2
            for (int k = 0; k < 3; ++k) {
                triplets.emplace_back(row, 4 * i + k, gammaD);
                B(row, 0) = gammaD * R(k, 0);
                B(row, 1) = gammaD * R(k, 1);
                B(row, 2) = gammaD * R(k, 2);
                row++;
            }
        }
    }

    // Build sparse matrix
    Eigen::SparseMatrix<double> A(row, cols);
    A.setFromTriplets(triplets.begin(), triplets.end());

    // Form normal equations: A^T * A * X = A^T * B
    Eigen::SparseMatrix<double> AtA = (A.transpose() * A).eval();
    Eigen::MatrixXd AtB = A.transpose() * B;

    // Add small diagonal regularization for numerical stability
    for (int i = 0; i < cols; ++i) {
        AtA.coeffRef(i, i) += 1e-6;
    }

    // Solve using Cholesky decomposition (symmetric positive definite)
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.compute(AtA);

    if (solver.info() != Eigen::Success) {
        MV_LOG_WARNING("NRICP: Cholesky decomposition failed, increasing regularization");
        for (int i = 0; i < cols; ++i) {
            AtA.coeffRef(i, i) += 1e-3;
        }
        solver.compute(AtA);
    }

    X = solver.solve(AtB);
}

void NRICPDeformer::ApplyTransformations(const Eigen::MatrixXd& X,
                                          std::vector<Vector3>& outPositions) {
    outPositions.resize(vertexCount_);
    for (int i = 0; i < vertexCount_; ++i) {
        const Vector3& sv = sourceVertices_[i];
        double vx = static_cast<double>(sv.x);
        double vy = static_cast<double>(sv.y);
        double vz = static_cast<double>(sv.z);

        // Per-vertex affine transform: [vx, vy, vz, 1] * X[4i:4i+3, :]
        double px = vx * X(4 * i + 0, 0) + vy * X(4 * i + 1, 0) + vz * X(4 * i + 2, 0) + X(4 * i + 3, 0);
        double py = vx * X(4 * i + 0, 1) + vy * X(4 * i + 1, 1) + vz * X(4 * i + 2, 1) + X(4 * i + 3, 1);
        double pz = vx * X(4 * i + 0, 2) + vy * X(4 * i + 1, 2) + vz * X(4 * i + 2, 2) + X(4 * i + 3, 2);

        outPositions[i] = Vector3(static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz));
    }
}

void NRICPDeformer::ApplyTransformationsSubsampled(
    const Eigen::MatrixXd& X,
    const ControlNodeData& controlData,
    std::vector<Vector3>& outPositions) {

    outPositions.resize(vertexCount_);

    for (int v = 0; v < vertexCount_; ++v) {
        const auto& weights = controlData.interpolationWeights[v];
        const Vector3& sv = sourceVertices_[v];
        double vx = static_cast<double>(sv.x);
        double vy = static_cast<double>(sv.y);
        double vz = static_cast<double>(sv.z);

        double px = 0.0, py = 0.0, pz = 0.0;

        for (const auto& [ci, w] : weights) {
            double wd = static_cast<double>(w);

            // Apply this control node's transform to this vertex, then weight
            double cx = vx * X(4 * ci + 0, 0) + vy * X(4 * ci + 1, 0) + vz * X(4 * ci + 2, 0) + X(4 * ci + 3, 0);
            double cy = vx * X(4 * ci + 0, 1) + vy * X(4 * ci + 1, 1) + vz * X(4 * ci + 2, 1) + X(4 * ci + 3, 1);
            double cz = vx * X(4 * ci + 0, 2) + vy * X(4 * ci + 1, 2) + vz * X(4 * ci + 2, 2) + X(4 * ci + 3, 2);

            px += wd * cx;
            py += wd * cy;
            pz += wd * cz;
        }

        outPositions[v] = Vector3(static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz));
    }
}

void NRICPDeformer::ComputeNormals(const std::vector<Vector3>& positions,
                                    std::vector<Vector3>& normals) {
    normals.assign(positions.size(), Vector3(0.0f, 0.0f, 0.0f));

    for (const auto& face : sourceFaces_) {
        const auto& indices = face.vertexIndices;
        if (indices.size() < 3) continue;

        // Fan triangulation
        for (size_t i = 1; i < indices.size() - 1; ++i) {
            unsigned int i0 = indices[0];
            unsigned int i1 = indices[i];
            unsigned int i2 = indices[i + 1];

            if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size())
                continue;

            Vector3 edge1 = positions[i1] - positions[i0];
            Vector3 edge2 = positions[i2] - positions[i0];
            Vector3 faceNormal = edge1.Cross(edge2);

            normals[i0] = normals[i0] + faceNormal;
            normals[i1] = normals[i1] + faceNormal;
            normals[i2] = normals[i2] + faceNormal;
        }
    }

    // Normalize
    for (auto& n : normals) {
        float len = n.Length();
        if (len > 1e-8f) {
            n = n * (1.0f / len);
        }
    }
}

float NRICPDeformer::ComputeRMSDisplacement(const std::vector<Vector3>& posA,
                                              const std::vector<Vector3>& posB) {
    if (posA.size() != posB.size() || posA.empty()) return 0.0f;

    double sum = 0.0;
    for (size_t i = 0; i < posA.size(); ++i) {
        Vector3 diff = posA[i] - posB[i];
        sum += static_cast<double>(diff.Dot(diff));
    }
    return static_cast<float>(std::sqrt(sum / posA.size()));
}

// ============================================================================
// Main solve loop with all features
// ============================================================================

std::vector<Vector3> NRICPDeformer::Solve() {
    if (vertexCount_ == 0 || !targetMesh_) {
        MV_LOG_WARNING("NRICP: No source vertices or target mesh");
        return {};
    }

    // Step 1: Build edge graph
    if (progressCallback_) {
        progressCallback_(0.02f, "NRICP: Building mesh edge graph...");
    }
    BuildEdges();

    if (edges_.empty()) {
        MV_LOG_WARNING("NRICP: No edges found in source mesh");
        return {};
    }

    // Step 1b: Detect boundary vertices and mark exclusion zone
    if (progressCallback_) {
        progressCallback_(0.03f, "NRICP: Detecting boundary regions...");
    }
    DetectAndExcludeBoundaryVertices();

    // Step 1c: Compute bounding box (used for correspondence distance and sampling normalization)
    bboxMin_ = Vector3(std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max(),
                       std::numeric_limits<float>::max());
    bboxMax_ = Vector3(std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest());
    for (const auto& v : sourceVertices_) {
        bboxMin_.x = std::min(bboxMin_.x, v.x);
        bboxMin_.y = std::min(bboxMin_.y, v.y);
        bboxMin_.z = std::min(bboxMin_.z, v.z);
        bboxMax_.x = std::max(bboxMax_.x, v.x);
        bboxMax_.y = std::max(bboxMax_.y, v.y);
        bboxMax_.z = std::max(bboxMax_.z, v.z);
    }
    Vector3 diagonal = bboxMax_ - bboxMin_;
    bboxDiagonal_ = diagonal.Length();

    // Compute max correspondence distance
    if (params_.maxCorrespondenceDistance < 0.0f) {
        resolvedMaxCorrespondenceDistance_ = bboxDiagonal_ * 0.15f;
        MV_LOG_INFO(QString("NRICP: Auto-computed max correspondence distance: %1 (15%% of bbox diagonal %2)")
            .arg(resolvedMaxCorrespondenceDistance_, 0, 'f', 3).arg(bboxDiagonal_, 0, 'f', 3));
    } else {
        resolvedMaxCorrespondenceDistance_ = params_.maxCorrespondenceDistance;
    }

    // Ensure target BVH is built
    if (progressCallback_) {
        progressCallback_(0.04f, "NRICP: Building target acceleration structure...");
    }
    targetMesh_->GetBVH(); // Triggers lazy build

    // Step 2: Compute schedules for all parameters
    const int N = vertexCount_;
    int steps = std::max(1, params_.stiffnessSteps);
    int optIters = std::max(1, params_.optimizationIterations);

    // Alpha schedule (logarithmic spacing from alphaInitial to alphaFinal)
    std::vector<float> alphaSchedule;
    if (steps == 1) {
        alphaSchedule.push_back(params_.alphaFinal);
    } else {
        float logStart = std::log(std::max(params_.alphaInitial, 0.01f));
        float logEnd = std::log(std::max(params_.alphaFinal, 0.001f));
        for (int s = 0; s < steps; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(steps - 1);
            alphaSchedule.push_back(std::exp(logStart + t * (logEnd - logStart)));
        }
    }

    // Dp schedule (linear interpolation from dpInitial to dpFinal)
    std::vector<float> dpSchedule;
    if (steps == 1) {
        dpSchedule.push_back(params_.dpFinal);
    } else {
        for (int s = 0; s < steps; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(steps - 1);
            dpSchedule.push_back(params_.dpInitial + t * (params_.dpFinal - params_.dpInitial));
        }
    }

    // Gamma schedule (logarithmic spacing from gammaInitial to gammaFinal, or 0 if disabled)
    std::vector<float> gammaSchedule;
    bool rigidityEnabled = (params_.gammaInitial > 0.0f || params_.gammaFinal > 0.0f);
    if (rigidityEnabled) {
        if (steps == 1) {
            gammaSchedule.push_back(params_.gammaFinal);
        } else {
            float gStart = std::max(params_.gammaInitial, 0.001f);
            float gEnd = std::max(params_.gammaFinal, 0.001f);
            float logGStart = std::log(gStart);
            float logGEnd = std::log(gEnd);
            for (int s = 0; s < steps; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(steps - 1);
                gammaSchedule.push_back(std::exp(logGStart + t * (logGEnd - logGStart)));
            }
        }
    } else {
        gammaSchedule.assign(steps, 0.0f);
    }

    // Sampling schedule (linear interpolation from samplingInitial to samplingFinal)
    std::vector<float> samplingSchedule;
    bool subsamplingEnabled = (params_.samplingInitial > 0.0f || params_.samplingFinal > 0.0f);
    if (subsamplingEnabled) {
        float sampInit = params_.samplingInitial;
        float sampFin = params_.samplingFinal;

        // Convert normalized values to absolute using bbox diagonal
        if (params_.normalizeSampling && bboxDiagonal_ > 0.0f) {
            sampInit *= bboxDiagonal_;
            sampFin *= bboxDiagonal_;
        }

        if (steps == 1) {
            samplingSchedule.push_back(sampFin);
        } else {
            for (int s = 0; s < steps; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(steps - 1);
                samplingSchedule.push_back(sampInit + t * (sampFin - sampInit));
            }
        }
    }

    // Step 3: Initialize X to identity transforms
    // Start with all-vertex system; may be resized per step if subsampling
    int currentNodeCount = N;
    Eigen::MatrixXd X = Eigen::MatrixXd::Zero(4 * N, 3);
    for (int i = 0; i < N; ++i) {
        X(4 * i + 0, 0) = 1.0;
        X(4 * i + 1, 1) = 1.0;
        X(4 * i + 2, 2) = 1.0;
    }

    // Step 4: Main coarse-to-fine loop
    std::vector<Vector3> currentPositions = sourceVertices_;
    std::vector<Vector3> currentNormals = sourceNormals_;

    float normalThresholdCos = std::cos(params_.normalThreshold * 3.14159265359f / 180.0f);
    int totalIterations = steps * params_.icpIterations * optIters;
    int iterationCount = 0;

    int excludedCount = static_cast<int>(std::count(excludedVertices_.begin(), excludedVertices_.end(), true));
    MV_LOG_INFO(QString("NRICP: Starting solve with %1 vertices (%2 excluded), %3 edges, %4 landmarks, "
                        "%5 stiffness steps, %6 opt iters, rigidity=%7, subsampling=%8")
        .arg(N).arg(excludedCount).arg(edges_.size()).arg(landmarks_.size())
        .arg(steps).arg(optIters)
        .arg(rigidityEnabled ? "on" : "off")
        .arg(subsamplingEnabled ? "on" : "off"));

    for (int s = 0; s < steps; ++s) {
        float alpha = alphaSchedule[s];
        float dp = dpSchedule[s];
        float gamma = gammaSchedule[s];

        // Determine control node data for this step
        ControlNodeData* activeControlData = nullptr;

        if (subsamplingEnabled) {
            float voxelSize = samplingSchedule[s];
            controlNodeData_ = ComputeControlNodes(voxelSize);
            BuildControlNodeEdges(controlNodeData_);
            ComputeInterpolationWeights(controlNodeData_);
            activeControlData = &controlNodeData_;

            // Re-initialize X for new control node count
            int K = controlNodeData_.controlNodeCount;
            if (K != currentNodeCount) {
                currentNodeCount = K;
                X = Eigen::MatrixXd::Zero(4 * K, 3);
                for (int i = 0; i < K; ++i) {
                    X(4 * i + 0, 0) = 1.0;
                    X(4 * i + 1, 1) = 1.0;
                    X(4 * i + 2, 2) = 1.0;
                }
                // Clear rotation targets since node count changed
                rotationTargets_.clear();
            }
        }

        for (int iter = 0; iter < params_.icpIterations; ++iter) {
            // Check cancellation
            if (cancelled_ && cancelled_->load()) {
                MV_LOG_INFO("NRICP: Cancelled by user");
                return {};
            }

            // Compute normals for current deformed positions
            ComputeNormals(currentPositions, currentNormals);

            // Find ICP correspondences
            std::vector<int> corrIndices;
            std::vector<Vector3> corrPoints;
            FindCorrespondences(currentPositions, currentNormals, normalThresholdCos,
                              corrIndices, corrPoints);

            if (corrIndices.empty() && landmarks_.empty()) {
                MV_LOG_WARNING(QString("NRICP: No correspondences found at level %1, iter %2")
                    .arg(s + 1).arg(iter + 1));
                break;
            }

            // Inner optimization loop (re-solve with same correspondences)
            for (int optIter = 0; optIter < optIters; ++optIter) {
                // Check cancellation
                if (cancelled_ && cancelled_->load()) {
                    MV_LOG_INFO("NRICP: Cancelled by user");
                    return {};
                }

                // Progress
                float progress = 0.05f + 0.90f * (static_cast<float>(iterationCount) / static_cast<float>(std::max(totalIterations, 1)));
                if (progressCallback_) {
                    std::string msg = "NRICP: Level " + std::to_string(s + 1) + "/" + std::to_string(steps) +
                        ", ICP " + std::to_string(iter + 1) + "/" + std::to_string(params_.icpIterations);
                    if (optIters > 1) {
                        msg += ", opt " + std::to_string(optIter + 1) + "/" + std::to_string(optIters);
                    }
                    if (activeControlData) {
                        msg += " (K=" + std::to_string(currentNodeCount) + ")";
                    }
                    progressCallback_(progress, msg);
                }

                // Update rotation targets for rigidity (skip on very first global iteration)
                if (rigidityEnabled && gamma > 0.0f && iterationCount > 0) {
                    ComputeRotationTargets(X, currentNodeCount);
                }

                // Save state for dp damping and convergence check
                Eigen::MatrixXd X_prev = X;
                std::vector<Vector3> prevPositions = currentPositions;

                // Solve the sparse linear system
                SolveLinearSystem(alpha, gamma, corrIndices, corrPoints, activeControlData, X);

                // Apply dp damping: X = X_prev + dp * (X_solved - X_prev)
                if (dp < 0.999f) {
                    X = X_prev + dp * (X - X_prev);
                }

                // Apply transformations to get new positions
                if (activeControlData && activeControlData->controlNodeCount < vertexCount_) {
                    ApplyTransformationsSubsampled(X, *activeControlData, currentPositions);
                } else {
                    ApplyTransformations(X, currentPositions);
                }

                // Check convergence
                float rmsd = ComputeRMSDisplacement(prevPositions, currentPositions);
                iterationCount++;

                if (rmsd < params_.epsilon) {
                    MV_LOG_INFO(QString("NRICP: Converged at level %1, ICP %2, opt %3 (RMSD=%4)")
                        .arg(s + 1).arg(iter + 1).arg(optIter + 1).arg(rmsd));
                    break;
                }
            }
        }
    }

    if (progressCallback_) {
        progressCallback_(0.97f, "NRICP: Finalizing...");
    }

    // Validate output
    for (const auto& v : currentPositions) {
        if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z) ||
            std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.z)) {
            MV_LOG_WARNING("NRICP: Output contains NaN or Inf values");
            return {};
        }
    }

    MV_LOG_INFO(QString("NRICP: Solve complete, %1 vertices deformed").arg(currentPositions.size()));

    if (progressCallback_) {
        progressCallback_(1.0f, "NRICP: Complete!");
    }

    return currentPositions;
}

} // namespace MetaVisage
