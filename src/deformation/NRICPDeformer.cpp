#include "deformation/NRICPDeformer.h"
#include "utils/BVH.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace MetaVisage {

NRICPDeformer::NRICPDeformer()
    : vertexCount_(0), cancelled_(nullptr) {
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

    for (int i = 0; i < vertexCount_; ++i) {
        BVHClosestPointResult result = targetBVH->FindClosestPoint(currentPositions[i], targetVertices);

        if (!result.found) continue;

        // Normal angle filtering
        if (i < static_cast<int>(currentNormals.size()) && currentNormals[i].Length() > 0.01f) {
            float dotProduct = currentNormals[i].Normalized().Dot(result.normal.Normalized());
            if (dotProduct < normalThresholdCos) continue;
        }

        correspondenceIndices.push_back(i);
        correspondencePoints.push_back(result.point);
    }
}

void NRICPDeformer::SolveLinearSystem(
    float alpha,
    const std::vector<int>& corrIndices,
    const std::vector<Vector3>& corrPoints,
    Eigen::MatrixXd& X) {

    const int N = vertexCount_;
    const int cols = 4 * N;

    int stiffRows = 4 * static_cast<int>(edges_.size());
    int dataRows = static_cast<int>(corrIndices.size());
    int landmarkRows = static_cast<int>(landmarks_.size());
    int totalRows = stiffRows + dataRows + landmarkRows;

    // Build sparse matrix A using triplets
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(stiffRows * 2 + dataRows * 4 + landmarkRows * 4);

    // Right-hand side (3 columns for x, y, z)
    Eigen::MatrixXd B = Eigen::MatrixXd::Zero(totalRows, 3);

    int row = 0;

    // --- Stiffness term ---
    // For each edge (i,j), penalize difference in per-vertex transforms
    double alphaD = static_cast<double>(alpha);
    for (const auto& edge : edges_) {
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

    // --- Landmark term ---
    double wl = static_cast<double>(params_.landmarkWeight);
    for (const auto& [vi, target] : landmarks_) {
        if (vi < 0 || vi >= N) continue;
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

    X = solver.solve(AtB); // X is 4N x 3
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

    // Ensure target BVH is built
    if (progressCallback_) {
        progressCallback_(0.04f, "NRICP: Building target acceleration structure...");
    }
    targetMesh_->GetBVH(); // Triggers lazy build

    // Step 2: Initialize X to identity transforms
    // X is 4N x 3: for each vertex, rows [4i..4i+3] form a 4x3 affine block
    // Identity: row 0 = [1,0,0], row 1 = [0,1,0], row 2 = [0,0,1], row 3 = [0,0,0]
    const int N = vertexCount_;
    Eigen::MatrixXd X = Eigen::MatrixXd::Zero(4 * N, 3);
    for (int i = 0; i < N; ++i) {
        X(4 * i + 0, 0) = 1.0; // identity transform
        X(4 * i + 1, 1) = 1.0;
        X(4 * i + 2, 2) = 1.0;
        // row 3 = translation = [0,0,0]
    }

    // Step 3: Compute stiffness schedule (logarithmic spacing from alphaInitial to alphaFinal)
    std::vector<float> alphaSchedule;
    int steps = std::max(1, params_.stiffnessSteps);
    if (steps == 1) {
        alphaSchedule.push_back(params_.alphaFinal);
    } else {
        float logStart = std::log(std::max(params_.alphaInitial, 0.01f));
        float logEnd = std::log(std::max(params_.alphaFinal, 0.001f));
        for (int s = 0; s < steps; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(steps - 1);
            float alpha = std::exp(logStart + t * (logEnd - logStart));
            alphaSchedule.push_back(alpha);
        }
    }

    // Step 4: Main coarse-to-fine loop
    std::vector<Vector3> currentPositions = sourceVertices_;
    std::vector<Vector3> currentNormals = sourceNormals_;

    float normalThresholdCos = std::cos(params_.normalThreshold * 3.14159265359f / 180.0f);
    int totalIterations = steps * params_.icpIterations;
    int iterationCount = 0;

    MV_LOG_INFO(QString("NRICP: Starting solve with %1 vertices, %2 edges, %3 landmarks, %4 stiffness steps")
        .arg(N).arg(edges_.size()).arg(landmarks_.size()).arg(steps));

    for (int s = 0; s < steps; ++s) {
        float alpha = alphaSchedule[s];

        for (int iter = 0; iter < params_.icpIterations; ++iter) {
            // Check cancellation
            if (cancelled_ && cancelled_->load()) {
                MV_LOG_INFO("NRICP: Cancelled by user");
                return {};
            }

            // Progress
            float progress = 0.05f + 0.90f * (static_cast<float>(iterationCount) / static_cast<float>(totalIterations));
            if (progressCallback_) {
                progressCallback_(progress,
                    "NRICP: Level " + std::to_string(s + 1) + "/" + std::to_string(steps) +
                    ", iter " + std::to_string(iter + 1) + "/" + std::to_string(params_.icpIterations) +
                    " (alpha=" + std::to_string(static_cast<int>(alpha)) + ")");
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

            // Save previous positions for convergence check
            std::vector<Vector3> prevPositions = currentPositions;

            // Solve the sparse linear system
            SolveLinearSystem(alpha, corrIndices, corrPoints, X);

            // Apply transformations to get new positions
            ApplyTransformations(X, currentPositions);

            // Check convergence
            float rmsd = ComputeRMSDisplacement(prevPositions, currentPositions);
            if (rmsd < params_.epsilon) {
                MV_LOG_INFO(QString("NRICP: Converged at level %1, iter %2 (RMSD=%3)")
                    .arg(s + 1).arg(iter + 1).arg(rmsd));
                break;
            }

            iterationCount++;
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
