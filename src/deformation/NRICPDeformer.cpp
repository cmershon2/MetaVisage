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
#include <unordered_map>

namespace MetaVisage {

NRICPDeformer::NRICPDeformer()
    : vertexCount_(0), cancelled_(nullptr), resolvedMaxCorrespondenceDistance_(-1.0f),
      bboxDiagonal_(0.0f), solverFactorized_(false), cachedRigidityStartRow_(0),
      cachedTotalRows_(0), cachedCols_(0) {
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

void NRICPDeformer::BuildWeldMap() {
    weldGroups_.clear();
    weldGroupIndex_.assign(vertexCount_, -1);

    if (vertexCount_ == 0) return;

    const float epsilon = 1e-5f;
    const float cellSize = epsilon * 10.0f;
    const float invCell = 1.0f / cellSize;

    struct CellHash {
        size_t operator()(const std::tuple<int, int, int>& key) const {
            auto h1 = std::hash<int>{}(std::get<0>(key));
            auto h2 = std::hash<int>{}(std::get<1>(key));
            auto h3 = std::hash<int>{}(std::get<2>(key));
            return h1 ^ (h2 * 2654435761u) ^ (h3 * 40503u);
        }
    };

    std::unordered_map<std::tuple<int, int, int>, std::vector<int>, CellHash> grid;

    for (int i = 0; i < vertexCount_; ++i) {
        const Vector3& v = sourceVertices_[i];
        int cx = static_cast<int>(std::floor(v.x * invCell));
        int cy = static_cast<int>(std::floor(v.y * invCell));
        int cz = static_cast<int>(std::floor(v.z * invCell));
        grid[{cx, cy, cz}].push_back(i);
    }

    float epsSq = epsilon * epsilon;
    std::vector<bool> visited(vertexCount_, false);

    for (int i = 0; i < vertexCount_; ++i) {
        if (visited[i]) continue;

        const Vector3& vi = sourceVertices_[i];
        int cx = static_cast<int>(std::floor(vi.x * invCell));
        int cy = static_cast<int>(std::floor(vi.y * invCell));
        int cz = static_cast<int>(std::floor(vi.z * invCell));

        std::vector<int> group;
        group.push_back(i);

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    auto it = grid.find({cx + dx, cy + dy, cz + dz});
                    if (it == grid.end()) continue;

                    for (int j : it->second) {
                        if (j <= i || visited[j]) continue;
                        Vector3 diff = sourceVertices_[j] - vi;
                        if (diff.Dot(diff) < epsSq) {
                            group.push_back(j);
                            visited[j] = true;
                        }
                    }
                }
            }
        }

        visited[i] = true;

        if (group.size() >= 2) {
            int groupIdx = static_cast<int>(weldGroups_.size());
            for (int idx : group) {
                weldGroupIndex_[idx] = groupIdx;
            }
            weldGroups_.push_back(std::move(group));
        }
    }

    int totalWeldedVerts = 0;
    for (const auto& g : weldGroups_) {
        totalWeldedVerts += static_cast<int>(g.size());
    }

    MV_LOG_INFO(QString("NRICP: Found %1 weld groups (%2 vertices at UV seams)")
        .arg(weldGroups_.size()).arg(totalWeldedVerts));
}

void NRICPDeformer::EnforceWeldConstraints(std::vector<Vector3>& positions) {
    for (const auto& group : weldGroups_) {
        double ax = 0.0, ay = 0.0, az = 0.0;
        for (int idx : group) {
            ax += static_cast<double>(positions[idx].x);
            ay += static_cast<double>(positions[idx].y);
            az += static_cast<double>(positions[idx].z);
        }
        float invCount = 1.0f / static_cast<float>(group.size());
        Vector3 avg(static_cast<float>(ax) * invCount,
                    static_cast<float>(ay) * invCount,
                    static_cast<float>(az) * invCount);

        for (int idx : group) {
            positions[idx] = avg;
        }
    }
}

void NRICPDeformer::BuildEdges() {
    edges_.clear();
    std::set<std::pair<int, int>> edgeSet;

    for (const auto& face : sourceFaces_) {
        const auto& indices = face.vertexIndices;
        if (indices.size() < 3) continue;

        for (size_t i = 0; i < indices.size(); ++i) {
            int a = static_cast<int>(indices[i]);
            int b = static_cast<int>(indices[(i + 1) % indices.size()]);
            int lo = std::min(a, b);
            int hi = std::max(a, b);
            edgeSet.insert({lo, hi});
        }
    }

    int weldEdgeCount = 0;
    for (const auto& group : weldGroups_) {
        for (size_t i = 0; i < group.size(); ++i) {
            for (size_t j = i + 1; j < group.size(); ++j) {
                int lo = std::min(group[i], group[j]);
                int hi = std::max(group[i], group[j]);
                if (edgeSet.insert({lo, hi}).second) {
                    weldEdgeCount++;
                }
            }
        }
    }

    edges_.reserve(edgeSet.size());
    for (const auto& [v0, v1] : edgeSet) {
        edges_.push_back({v0, v1});
    }

    MV_LOG_INFO(QString("NRICP: Built %1 unique edges from %2 faces (%3 weld edges added)")
        .arg(edges_.size()).arg(sourceFaces_.size()).arg(weldEdgeCount));
}

void NRICPDeformer::DetectAndExcludeBoundaryVertices() {
    excludedVertices_.assign(vertexCount_, false);

    if (!params_.enableBoundaryExclusion || vertexCount_ == 0) {
        MV_LOG_INFO("NRICP: Boundary exclusion disabled or no vertices");
        return;
    }

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

    vertexAdjacency_.assign(vertexCount_, std::vector<int>());
    for (const auto& edge : edges_) {
        vertexAdjacency_[edge.v0].push_back(edge.v1);
        vertexAdjacency_[edge.v1].push_back(edge.v0);
    }

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

// ============================================================================
// Correspondence finding - parallelized with OpenMP
// ============================================================================

void NRICPDeformer::FindCorrespondences(
    const std::vector<Vector3>& currentPositions,
    const std::vector<Vector3>& currentNormals,
    float normalThresholdCos,
    std::vector<int>& correspondenceIndices,
    std::vector<Vector3>& correspondencePoints) {

    correspondenceIndices.clear();
    correspondencePoints.clear();

    if (!targetMesh_) return;

    BVH* targetBVH = targetMesh_->GetBVH();
    if (!targetBVH || !targetBVH->IsBuilt()) return;

    const auto& targetVertices = targetMesh_->GetVertices();

    float maxDistSq = (resolvedMaxCorrespondenceDistance_ > 0.0f)
        ? resolvedMaxCorrespondenceDistance_ * resolvedMaxCorrespondenceDistance_
        : -1.0f;

#ifdef _OPENMP
    // Parallel path: each thread collects its own results, then merge
    int numThreads = omp_get_max_threads();
    std::vector<std::vector<int>> threadIndices(numThreads);
    std::vector<std::vector<Vector3>> threadPoints(numThreads);

    // Pre-reserve estimated capacity per thread
    int estimatedPerThread = vertexCount_ / numThreads + 64;
    for (int t = 0; t < numThreads; ++t) {
        threadIndices[t].reserve(estimatedPerThread);
        threadPoints[t].reserve(estimatedPerThread);
    }

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        auto& localIndices = threadIndices[tid];
        auto& localPoints = threadPoints[tid];

        #pragma omp for schedule(dynamic, 256)
        for (int i = 0; i < vertexCount_; ++i) {
            if (!excludedVertices_.empty() && excludedVertices_[i]) continue;

            BVHClosestPointResult result = targetBVH->FindClosestPoint(currentPositions[i], targetVertices);

            if (!result.found) continue;
            if (maxDistSq > 0.0f && result.distanceSq > maxDistSq) continue;

            if (i < static_cast<int>(currentNormals.size()) && currentNormals[i].Length() > 0.01f) {
                float dotProduct = currentNormals[i].Normalized().Dot(result.normal.Normalized());
                if (dotProduct < normalThresholdCos) continue;
            }

            localIndices.push_back(i);
            localPoints.push_back(result.point);
        }
    }

    // Merge thread-local results
    int totalCount = 0;
    for (int t = 0; t < numThreads; ++t) {
        totalCount += static_cast<int>(threadIndices[t].size());
    }
    correspondenceIndices.reserve(totalCount);
    correspondencePoints.reserve(totalCount);

    for (int t = 0; t < numThreads; ++t) {
        correspondenceIndices.insert(correspondenceIndices.end(),
            threadIndices[t].begin(), threadIndices[t].end());
        correspondencePoints.insert(correspondencePoints.end(),
            threadPoints[t].begin(), threadPoints[t].end());
    }
#else
    // Serial fallback
    correspondenceIndices.reserve(vertexCount_);
    correspondencePoints.reserve(vertexCount_);

    for (int i = 0; i < vertexCount_; ++i) {
        if (!excludedVertices_.empty() && excludedVertices_[i]) continue;

        BVHClosestPointResult result = targetBVH->FindClosestPoint(currentPositions[i], targetVertices);

        if (!result.found) continue;
        if (maxDistSq > 0.0f && result.distanceSq > maxDistSq) continue;

        if (i < static_cast<int>(currentNormals.size()) && currentNormals[i].Length() > 0.01f) {
            float dotProduct = currentNormals[i].Normalized().Dot(result.normal.Normalized());
            if (dotProduct < normalThresholdCos) continue;
        }

        correspondenceIndices.push_back(i);
        correspondencePoints.push_back(result.point);
    }
#endif
}

// ============================================================================
// Phase 2: Rigidity regularization - compute closest rotation via SVD
// ============================================================================

void NRICPDeformer::ComputeRotationTargets(const Eigen::MatrixXd& X, int nodeCount) {
    rotationTargets_.resize(nodeCount);

#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < nodeCount; ++i) {
        Eigen::Matrix3d A;
        A(0, 0) = X(4 * i + 0, 0); A(0, 1) = X(4 * i + 0, 1); A(0, 2) = X(4 * i + 0, 2);
        A(1, 0) = X(4 * i + 1, 0); A(1, 1) = X(4 * i + 1, 1); A(1, 2) = X(4 * i + 1, 2);
        A(2, 0) = X(4 * i + 2, 0); A(2, 1) = X(4 * i + 2, 1); A(2, 2) = X(4 * i + 2, 2);

        Eigen::JacobiSVD<Eigen::Matrix3d> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::Matrix3d R = svd.matrixU() * svd.matrixV().transpose();

        if (R.determinant() < 0.0) {
            Eigen::Matrix3d U = svd.matrixU();
            U.col(2) *= -1.0;
            R = U * svd.matrixV().transpose();
        }

        rotationTargets_[i] = R;
    }
}

// ============================================================================
// Phase 3: Control node subsampling methods (grid-accelerated kNN)
// ============================================================================

ControlNodeData NRICPDeformer::ComputeControlNodes(float voxelSize) {
    ControlNodeData data;

    if (voxelSize <= 0.0f) {
        data.controlNodeCount = vertexCount_;
        data.controlNodeIndices.resize(vertexCount_);
        std::iota(data.controlNodeIndices.begin(), data.controlNodeIndices.end(), 0);
        return data;
    }

    float invVoxel = 1.0f / voxelSize;
    std::map<std::tuple<int, int, int>, std::pair<int, float>> voxelBest;

    for (int i = 0; i < vertexCount_; ++i) {
        const Vector3& v = sourceVertices_[i];
        int vx = static_cast<int>(std::floor((v.x - bboxMin_.x) * invVoxel));
        int vy = static_cast<int>(std::floor((v.y - bboxMin_.y) * invVoxel));
        int vz = static_cast<int>(std::floor((v.z - bboxMin_.z) * invVoxel));
        auto key = std::make_tuple(vx, vy, vz);

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

    // Use spatial grid to accelerate kNN search instead of brute-force O(K^2)
    // Estimate cell size: we want each cell to contain roughly kNeighbors nodes
    // Average spacing ~ (bbox_volume / K)^(1/3)
    Vector3 bboxSize = bboxMax_ - bboxMin_;
    float volume = std::max(bboxSize.x, 1e-6f) * std::max(bboxSize.y, 1e-6f) * std::max(bboxSize.z, 1e-6f);
    float avgSpacing = std::cbrt(volume / std::max(K, 1));
    // Search radius should cover kNeighbors: use ~3x average spacing
    float cellSize = avgSpacing * 3.0f;
    float invCell = 1.0f / cellSize;

    struct CellHash {
        size_t operator()(const std::tuple<int, int, int>& key) const {
            auto h1 = std::hash<int>{}(std::get<0>(key));
            auto h2 = std::hash<int>{}(std::get<1>(key));
            auto h3 = std::hash<int>{}(std::get<2>(key));
            return h1 ^ (h2 * 2654435761u) ^ (h3 * 40503u);
        }
    };

    std::unordered_map<std::tuple<int, int, int>, std::vector<int>, CellHash> grid;
    for (int i = 0; i < K; ++i) {
        const Vector3& v = controlPositions[i];
        int cx = static_cast<int>(std::floor(v.x * invCell));
        int cy = static_cast<int>(std::floor(v.y * invCell));
        int cz = static_cast<int>(std::floor(v.z * invCell));
        grid[{cx, cy, cz}].push_back(i);
    }

    // Determine search radius in cells (start with 1, expand if needed)
    int searchRadius = 1;

    std::set<std::pair<int, int>> edgeSet;

    for (int i = 0; i < K; ++i) {
        const Vector3& vi = controlPositions[i];
        int cx = static_cast<int>(std::floor(vi.x * invCell));
        int cy = static_cast<int>(std::floor(vi.y * invCell));
        int cz = static_cast<int>(std::floor(vi.z * invCell));

        // Collect candidates from nearby cells
        std::vector<std::pair<float, int>> dists;

        for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                    auto it = grid.find({cx + dx, cy + dy, cz + dz});
                    if (it == grid.end()) continue;
                    for (int j : it->second) {
                        if (i == j) continue;
                        Vector3 diff = controlPositions[j] - vi;
                        float d = diff.Dot(diff);
                        dists.push_back({d, j});
                    }
                }
            }
        }

        // If we didn't find enough neighbors, fall back to wider search
        if (static_cast<int>(dists.size()) < kNeighbors) {
            dists.clear();
            for (int j = 0; j < K; ++j) {
                if (i == j) continue;
                Vector3 diff = controlPositions[j] - vi;
                float d = diff.Dot(diff);
                dists.push_back({d, j});
            }
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

    // Build spatial grid for control node kNN lookup
    Vector3 bboxSize = bboxMax_ - bboxMin_;
    float volume = std::max(bboxSize.x, 1e-6f) * std::max(bboxSize.y, 1e-6f) * std::max(bboxSize.z, 1e-6f);
    float avgSpacing = std::cbrt(volume / std::max(K, 1));
    float cellSize = avgSpacing * 3.0f;
    float invCell = 1.0f / cellSize;

    struct CellHash {
        size_t operator()(const std::tuple<int, int, int>& key) const {
            auto h1 = std::hash<int>{}(std::get<0>(key));
            auto h2 = std::hash<int>{}(std::get<1>(key));
            auto h3 = std::hash<int>{}(std::get<2>(key));
            return h1 ^ (h2 * 2654435761u) ^ (h3 * 40503u);
        }
    };

    std::unordered_map<std::tuple<int, int, int>, std::vector<int>, CellHash> grid;
    for (int i = 0; i < K; ++i) {
        const Vector3& v = controlPositions[i];
        int cx = static_cast<int>(std::floor(v.x * invCell));
        int cy = static_cast<int>(std::floor(v.y * invCell));
        int cz = static_cast<int>(std::floor(v.z * invCell));
        grid[{cx, cy, cz}].push_back(i);
    }

    int searchRadius = 1;

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 512)
#endif
    for (int v = 0; v < vertexCount_; ++v) {
        int localIdx = vertexToControlLocal[v];
        if (localIdx >= 0) {
            data.interpolationWeights[v] = {{localIdx, 1.0f}};
            continue;
        }

        const Vector3& sv = sourceVertices_[v];
        int cx = static_cast<int>(std::floor(sv.x * invCell));
        int cy = static_cast<int>(std::floor(sv.y * invCell));
        int cz = static_cast<int>(std::floor(sv.z * invCell));

        // Collect candidates from nearby cells
        std::vector<std::pair<float, int>> dists;

        for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                    auto it = grid.find({cx + dx, cy + dy, cz + dz});
                    if (it == grid.end()) continue;
                    for (int c : it->second) {
                        Vector3 diff = controlPositions[c] - sv;
                        float d = diff.Dot(diff);
                        dists.push_back({d, c});
                    }
                }
            }
        }

        // Fall back to brute-force if not enough neighbors found
        if (static_cast<int>(dists.size()) < actualK) {
            dists.clear();
            dists.reserve(K);
            for (int c = 0; c < K; ++c) {
                Vector3 diff = controlPositions[c] - sv;
                float d = diff.Dot(diff);
                dists.push_back({d, c});
            }
        }

        int count = std::min(actualK, static_cast<int>(dists.size()));
        std::partial_sort(dists.begin(), dists.begin() + count, dists.end());

        float totalWeight = 0.0f;
        std::vector<std::pair<int, float>> weights;
        weights.reserve(count);

        for (int n = 0; n < count; ++n) {
            float d = std::sqrt(std::max(dists[n].first, 1e-12f));
            float w = 1.0f / d;
            weights.push_back({dists[n].second, w});
            totalWeight += w;
        }

        if (totalWeight > 0.0f) {
            for (auto& [idx, w] : weights) {
                w /= totalWeight;
            }
        }

        data.interpolationWeights[v] = std::move(weights);
    }
}

// ============================================================================
// Linear system: split into build, factorize, and solve phases
// ============================================================================

int NRICPDeformer::BuildSystemMatrix(
    float alpha,
    float gamma,
    const std::vector<int>& corrIndices,
    const std::vector<Vector3>& /* corrPoints */,
    const ControlNodeData* controlData,
    int nodeCount) {

    const bool subsampled = (controlData != nullptr && controlData->controlNodeCount < vertexCount_);
    cachedCols_ = 4 * nodeCount;

    const auto& activeEdges = subsampled ? controlData->controlEdges : edges_;

    int stiffRows = 4 * static_cast<int>(activeEdges.size());
    int dataRows = static_cast<int>(corrIndices.size());
    int landmarkRows = static_cast<int>(landmarks_.size());
    int rigidityRows = (gamma > 0.0f) ? 3 * nodeCount : 0;
    cachedTotalRows_ = stiffRows + dataRows + landmarkRows + rigidityRows;
    cachedRigidityStartRow_ = stiffRows + dataRows + landmarkRows;

    // Build sparse matrix A using triplets
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(stiffRows * 2 + dataRows * 4 + landmarkRows * 4 + rigidityRows);

    int row = 0;

    // --- Stiffness term ---
    double alphaD = static_cast<double>(alpha);
    for (const auto& edge : activeEdges) {
        int i = edge.v0;
        int j = edge.v1;
        for (int k = 0; k < 4; ++k) {
            triplets.emplace_back(row + k, 4 * i + k, alphaD);
            triplets.emplace_back(row + k, 4 * j + k, -alphaD);
        }
        row += 4;
    }

    // --- Data term (ICP correspondences) ---
    if (subsampled) {
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
            row++;
        }
    } else {
        for (size_t c = 0; c < corrIndices.size(); ++c) {
            int vi = corrIndices[c];
            const Vector3& sv = sourceVertices_[vi];

            triplets.emplace_back(row, 4 * vi + 0, static_cast<double>(sv.x));
            triplets.emplace_back(row, 4 * vi + 1, static_cast<double>(sv.y));
            triplets.emplace_back(row, 4 * vi + 2, static_cast<double>(sv.z));
            triplets.emplace_back(row, 4 * vi + 3, 1.0);
            row++;
        }
    }

    // --- Landmark term ---
    double wl = static_cast<double>(params_.landmarkWeight);
    if (subsampled) {
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
            row++;
        }
    } else {
        for (const auto& [vi, target] : landmarks_) {
            if (vi < 0 || vi >= nodeCount) continue;
            const Vector3& sv = sourceVertices_[vi];

            triplets.emplace_back(row, 4 * vi + 0, wl * static_cast<double>(sv.x));
            triplets.emplace_back(row, 4 * vi + 1, wl * static_cast<double>(sv.y));
            triplets.emplace_back(row, 4 * vi + 2, wl * static_cast<double>(sv.z));
            triplets.emplace_back(row, 4 * vi + 3, wl);
            row++;
        }
    }

    // --- Rigidity regularization term (ARAP) - just the A structure ---
    if (gamma > 0.0f) {
        double gammaD = static_cast<double>(gamma);
        for (int i = 0; i < nodeCount; ++i) {
            for (int k = 0; k < 3; ++k) {
                triplets.emplace_back(row, 4 * i + k, gammaD);
                row++;
            }
        }
    }

    // Build sparse matrix A
    cachedA_.resize(row, cachedCols_);
    cachedA_.setFromTriplets(triplets.begin(), triplets.end());

    // Form normal equations: AtA = A^T * A
    cachedAtA_ = (cachedA_.transpose() * cachedA_).eval();

    // Add diagonal regularization
    for (int i = 0; i < cachedCols_; ++i) {
        cachedAtA_.coeffRef(i, i) += 1e-6;
    }

    // Factorize (symbolic analysis + numeric factorization)
    cachedSolver_.compute(cachedAtA_);

    if (cachedSolver_.info() != Eigen::Success) {
        MV_LOG_WARNING("NRICP: Cholesky decomposition failed, increasing regularization");
        for (int i = 0; i < cachedCols_; ++i) {
            cachedAtA_.coeffRef(i, i) += 1e-3;
        }
        cachedSolver_.compute(cachedAtA_);
    }

    solverFactorized_ = (cachedSolver_.info() == Eigen::Success);
    return row;
}

void NRICPDeformer::BuildRHS(
    float alpha,
    float gamma,
    const std::vector<int>& corrIndices,
    const std::vector<Vector3>& corrPoints,
    const ControlNodeData* controlData,
    int nodeCount,
    int totalRows) {

    (void)alpha; // alpha only affects A, not B

    const bool subsampled = (controlData != nullptr && controlData->controlNodeCount < vertexCount_);

    cachedB_ = Eigen::MatrixXd::Zero(totalRows, 3);

    const auto& activeEdges = subsampled ? controlData->controlEdges : edges_;

    int row = 4 * static_cast<int>(activeEdges.size()); // Skip stiffness rows (all zeros in B)

    // --- Data term RHS ---
    for (size_t c = 0; c < corrIndices.size(); ++c) {
        cachedB_(row, 0) = static_cast<double>(corrPoints[c].x);
        cachedB_(row, 1) = static_cast<double>(corrPoints[c].y);
        cachedB_(row, 2) = static_cast<double>(corrPoints[c].z);
        row++;
    }

    // --- Landmark term RHS ---
    double wl = static_cast<double>(params_.landmarkWeight);
    for (const auto& [vi, target] : landmarks_) {
        if (subsampled) {
            if (vi < 0 || vi >= vertexCount_) continue;
        } else {
            if (vi < 0 || vi >= nodeCount) continue;
        }
        cachedB_(row, 0) = wl * static_cast<double>(target.x);
        cachedB_(row, 1) = wl * static_cast<double>(target.y);
        cachedB_(row, 2) = wl * static_cast<double>(target.z);
        row++;
    }

    // --- Rigidity RHS (rotation targets) ---
    if (gamma > 0.0f && !rotationTargets_.empty()) {
        double gammaD = static_cast<double>(gamma);
        int rigidityNodeCount = std::min(nodeCount, static_cast<int>(rotationTargets_.size()));
        for (int i = 0; i < rigidityNodeCount; ++i) {
            const Eigen::Matrix3d& R = rotationTargets_[i];
            for (int k = 0; k < 3; ++k) {
                cachedB_(row, 0) = gammaD * R(k, 0);
                cachedB_(row, 1) = gammaD * R(k, 1);
                cachedB_(row, 2) = gammaD * R(k, 2);
                row++;
            }
        }
    }
}

void NRICPDeformer::SolveFromFactorized(Eigen::MatrixXd& X) {
    if (!solverFactorized_) {
        MV_LOG_WARNING("NRICP: Solver not factorized, cannot solve");
        return;
    }

    Eigen::MatrixXd AtB = cachedA_.transpose() * cachedB_;
    X = cachedSolver_.solve(AtB);
}

// Legacy single-call interface (used if needed)
void NRICPDeformer::SolveLinearSystem(
    float alpha,
    float gamma,
    const std::vector<int>& corrIndices,
    const std::vector<Vector3>& corrPoints,
    const ControlNodeData* controlData,
    Eigen::MatrixXd& X) {

    const bool subsampled = (controlData != nullptr && controlData->controlNodeCount < vertexCount_);
    const int nodeCount = subsampled ? controlData->controlNodeCount : vertexCount_;

    int totalRows = BuildSystemMatrix(alpha, gamma, corrIndices, corrPoints, controlData, nodeCount);
    BuildRHS(alpha, gamma, corrIndices, corrPoints, controlData, nodeCount, totalRows);
    SolveFromFactorized(X);
}

void NRICPDeformer::ApplyTransformations(const Eigen::MatrixXd& X,
                                          std::vector<Vector3>& outPositions) {
    outPositions.resize(vertexCount_);

#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < vertexCount_; ++i) {
        const Vector3& sv = sourceVertices_[i];
        double vx = static_cast<double>(sv.x);
        double vy = static_cast<double>(sv.y);
        double vz = static_cast<double>(sv.z);

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

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 512)
#endif
    for (int v = 0; v < vertexCount_; ++v) {
        const auto& weights = controlData.interpolationWeights[v];
        const Vector3& sv = sourceVertices_[v];
        double vx = static_cast<double>(sv.x);
        double vy = static_cast<double>(sv.y);
        double vz = static_cast<double>(sv.z);

        double px = 0.0, py = 0.0, pz = 0.0;

        for (const auto& [ci, w] : weights) {
            double wd = static_cast<double>(w);

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

    // Face normal accumulation (sequential - writes to shared vertex normals)
    for (const auto& face : sourceFaces_) {
        const auto& indices = face.vertexIndices;
        if (indices.size() < 3) continue;

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

    // Normalize in parallel
    int normalCount = static_cast<int>(normals.size());
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < normalCount; ++i) {
        float len = normals[i].Length();
        if (len > 1e-8f) {
            normals[i] = normals[i] * (1.0f / len);
        }
    }
}

float NRICPDeformer::ComputeRMSDisplacement(const std::vector<Vector3>& posA,
                                              const std::vector<Vector3>& posB) {
    if (posA.size() != posB.size() || posA.empty()) return 0.0f;

    int count = static_cast<int>(posA.size());
    double sum = 0.0;

#ifdef _OPENMP
    #pragma omp parallel for reduction(+:sum) schedule(static)
#endif
    for (int i = 0; i < count; ++i) {
        Vector3 diff = posA[i] - posB[i];
        sum += static_cast<double>(diff.Dot(diff));
    }
    return static_cast<float>(std::sqrt(sum / count));
}

// ============================================================================
// Main solve loop - optimized with solver reuse and parallelization
// ============================================================================

std::vector<Vector3> NRICPDeformer::Solve() {
    if (vertexCount_ == 0 || !targetMesh_) {
        MV_LOG_WARNING("NRICP: No source vertices or target mesh");
        return {};
    }

    // Step 0: Build weld map to identify UV seam vertices sharing the same position
    if (progressCallback_) {
        progressCallback_(0.01f, "NRICP: Detecting UV seam vertices...");
    }
    BuildWeldMap();

    // Step 1: Build edge graph (includes weld edges between UV seam duplicates)
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

    // Step 1c: Compute bounding box
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

    // Alpha schedule (logarithmic spacing)
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

    // Dp schedule (linear interpolation)
    std::vector<float> dpSchedule;
    if (steps == 1) {
        dpSchedule.push_back(params_.dpFinal);
    } else {
        for (int s = 0; s < steps; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(steps - 1);
            dpSchedule.push_back(params_.dpInitial + t * (params_.dpFinal - params_.dpInitial));
        }
    }

    // Gamma schedule
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

    // Sampling schedule
    std::vector<float> samplingSchedule;
    bool subsamplingEnabled = (params_.samplingInitial > 0.0f || params_.samplingFinal > 0.0f);
    if (subsamplingEnabled) {
        float sampInit = params_.samplingInitial;
        float sampFin = params_.samplingFinal;

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
    // Pre-allocate buffers for swap-based convergence checking
    std::vector<Vector3> prevPositions(vertexCount_);

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

            int K = controlNodeData_.controlNodeCount;
            if (K != currentNodeCount) {
                currentNodeCount = K;
                X = Eigen::MatrixXd::Zero(4 * K, 3);
                for (int i = 0; i < K; ++i) {
                    X(4 * i + 0, 0) = 1.0;
                    X(4 * i + 1, 1) = 1.0;
                    X(4 * i + 2, 2) = 1.0;
                }
                rotationTargets_.clear();
            }
        }

        for (int iter = 0; iter < params_.icpIterations; ++iter) {
            if (cancelled_ && cancelled_->load()) {
                MV_LOG_INFO("NRICP: Cancelled by user");
                return {};
            }

            // Compute normals for current deformed positions
            ComputeNormals(currentPositions, currentNormals);

            // Find ICP correspondences (parallelized)
            std::vector<int> corrIndices;
            std::vector<Vector3> corrPoints;
            FindCorrespondences(currentPositions, currentNormals, normalThresholdCos,
                              corrIndices, corrPoints);

            if (corrIndices.empty() && landmarks_.empty()) {
                MV_LOG_WARNING(QString("NRICP: No correspondences found at level %1, iter %2")
                    .arg(s + 1).arg(iter + 1));
                break;
            }

            // ============================================================
            // OPTIMIZATION: Build and factorize the system ONCE per ICP step.
            // The system matrix A depends on: alpha, edges, correspondences, landmarks, gamma.
            // All of these are constant within the inner optimization loop.
            // When gamma=0: both A and B are constant, so X_solved is the same every iteration.
            //   We solve ONCE, then run dp damping iterations using the cached solution (no re-solve).
            // When gamma>0: A is constant but B changes (rotation targets), so we re-solve each
            //   iteration but reuse the factorization (only recompute AtB, not AtA).
            // ============================================================

            int totalRows = BuildSystemMatrix(alpha, gamma, corrIndices, corrPoints,
                                              activeControlData, currentNodeCount);

            // When gamma=0, solve once and cache X_solved for dp damping iterations
            Eigen::MatrixXd X_solved;
            bool hasCachedSolution = false;

            if (gamma <= 0.0f) {
                BuildRHS(alpha, gamma, corrIndices, corrPoints,
                         activeControlData, currentNodeCount, totalRows);
                SolveFromFactorized(X_solved);
                hasCachedSolution = true;
            }

            // Inner optimization loop
            for (int optIter = 0; optIter < optIters; ++optIter) {
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

                // Save previous positions for convergence check (swap to avoid deep copy)
                std::swap(prevPositions, currentPositions);

                if (hasCachedSolution) {
                    // gamma=0 path: reuse cached X_solved, just apply dp damping
                    // X = X_prev + dp * (X_solved - X_prev)
                    if (dp < 0.999f) {
                        X = X + dp * (X_solved - X);
                    } else {
                        X = X_solved;
                    }
                } else {
                    // gamma>0 path: need to re-solve because rotation targets change B
                    if (rigidityEnabled && gamma > 0.0f && iterationCount > 0) {
                        ComputeRotationTargets(X, currentNodeCount);
                    }

                    BuildRHS(alpha, gamma, corrIndices, corrPoints,
                             activeControlData, currentNodeCount, totalRows);

                    Eigen::MatrixXd X_prev = X;
                    SolveFromFactorized(X);

                    if (dp < 0.999f) {
                        X = X_prev + dp * (X - X_prev);
                    }
                }

                // Apply transformations to get new positions (parallelized)
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
        progressCallback_(0.96f, "NRICP: Enforcing UV seam constraints...");
    }

    EnforceWeldConstraints(currentPositions);

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
