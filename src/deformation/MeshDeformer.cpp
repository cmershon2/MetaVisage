#include "deformation/MeshDeformer.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace MetaVisage {

MeshDeformer::MeshDeformer()
    : kernelType_(DeformationAlgorithm::RBF_TPS),
      stiffness_(0.5f),
      smoothness_(0.5f),
      cancelled_(false) {
}

MeshDeformer::~MeshDeformer() {
}

void MeshDeformer::SetSourceMesh(std::shared_ptr<Mesh> mesh) {
    sourceMesh_ = mesh;
}

void MeshDeformer::SetCorrespondences(const std::vector<PointCorrespondence>& correspondences) {
    correspondences_ = correspondences;
}

void MeshDeformer::SetKernelType(DeformationAlgorithm kernel) {
    kernelType_ = kernel;
}

void MeshDeformer::SetStiffness(float stiffness) {
    stiffness_ = std::clamp(stiffness, 0.0f, 1.0f);
}

void MeshDeformer::SetSmoothness(float smoothness) {
    smoothness_ = std::clamp(smoothness, 0.0f, 1.0f);
}

void MeshDeformer::SetProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void MeshDeformer::Cancel() {
    cancelled_.store(true);
}

bool MeshDeformer::ValidateInputs(std::string& errorMessage) const {
    if (!sourceMesh_) {
        errorMessage = "No source mesh set";
        return false;
    }

    if (sourceMesh_->GetVertexCount() == 0) {
        errorMessage = "Source mesh has no vertices";
        return false;
    }

    if (correspondences_.empty()) {
        errorMessage = "No point correspondences defined";
        return false;
    }

    // Check that all correspondences have both sides filled
    for (const auto& corr : correspondences_) {
        if (corr.morphMeshVertexIndex < 0 || corr.targetMeshVertexIndex < 0) {
            errorMessage = "Incomplete point correspondences (missing morph or target points)";
            return false;
        }
    }

    // Need at least 4 points for a well-defined 3D deformation with polynomial terms
    if (correspondences_.size() < 4) {
        errorMessage = "At least 4 point correspondences required for stable deformation";
        return false;
    }

    return true;
}

void MeshDeformer::ExtractControlPoints(std::vector<Vector3>& sourcePoints,
                                          std::vector<Vector3>& targetPoints) const {
    sourcePoints.clear();
    targetPoints.clear();
    sourcePoints.reserve(correspondences_.size());
    targetPoints.reserve(correspondences_.size());

    for (const auto& corr : correspondences_) {
        if (corr.morphMeshVertexIndex >= 0 && corr.targetMeshVertexIndex >= 0) {
            sourcePoints.push_back(corr.morphMeshPosition);
            targetPoints.push_back(corr.targetMeshPosition);
        }
    }
}

std::shared_ptr<Mesh> MeshDeformer::CopyMesh(const std::shared_ptr<Mesh>& source) const {
    auto copy = std::make_shared<Mesh>();
    copy->SetVertices(source->GetVertices());
    copy->SetNormals(source->GetNormals());
    copy->SetUVs(source->GetUVs());
    copy->SetFaces(source->GetFaces());
    copy->SetMaterials(source->GetMaterials());
    copy->SetName(source->GetName());
    return copy;
}

DeformationResult MeshDeformer::Deform() {
    DeformationResult result;
    cancelled_.store(false);
    displacementMagnitudes_.clear();

    // Validate inputs
    std::string validationError;
    if (!ValidateInputs(validationError)) {
        result.errorMessage = validationError;
        return result;
    }

    // Report progress: starting
    if (progressCallback_) {
        progressCallback_(0.0f, "Preparing deformation...");
    }

    // Extract control points from correspondences
    std::vector<Vector3> sourcePoints, targetPoints;
    ExtractControlPoints(sourcePoints, targetPoints);

    if (sourcePoints.empty()) {
        result.errorMessage = "No valid control point pairs found";
        return result;
    }

    qDebug() << "MeshDeformer: Starting deformation with"
             << sourcePoints.size() << "control points and"
             << sourceMesh_->GetVertexCount() << "vertices";

    // Check for cancellation
    if (cancelled_.load()) {
        result.errorMessage = "Deformation cancelled";
        return result;
    }

    // Report progress: setting up RBF
    if (progressCallback_) {
        progressCallback_(0.05f, "Setting up RBF interpolation...");
    }

    // Set up and solve RBF system
    RBFInterpolator interpolator;
    interpolator.SetControlPoints(sourcePoints, targetPoints);
    interpolator.SetKernelType(kernelType_);
    interpolator.SetStiffness(stiffness_);
    interpolator.SetSmoothness(smoothness_);

    // Report progress: solving
    if (progressCallback_) {
        progressCallback_(0.1f, "Solving RBF system...");
    }

    if (!interpolator.Solve()) {
        result.errorMessage = "Failed to solve RBF interpolation system";
        return result;
    }

    // Check for cancellation
    if (cancelled_.load()) {
        result.errorMessage = "Deformation cancelled";
        return result;
    }

    // Report progress: applying deformation
    if (progressCallback_) {
        progressCallback_(0.3f, "Applying deformation to vertices...");
    }

    // Create a copy of the source mesh for deformation
    auto deformedMesh = CopyMesh(sourceMesh_);
    const auto& originalVertices = sourceMesh_->GetVertices();
    std::vector<Vector3> newVertices(originalVertices.size());
    displacementMagnitudes_.resize(originalVertices.size());

    float maxDisp = 0.0f;
    double totalDisp = 0.0;
    const size_t vertexCount = originalVertices.size();

    // Apply deformation to each vertex
    for (size_t i = 0; i < vertexCount; ++i) {
        // Check for cancellation periodically
        if (i % 1000 == 0 && cancelled_.load()) {
            result.errorMessage = "Deformation cancelled";
            return result;
        }

        // Evaluate RBF displacement at this vertex
        Vector3 displacement = interpolator.Evaluate(originalVertices[i]);
        newVertices[i] = originalVertices[i] + displacement;

        // Track displacement magnitude
        float mag = displacement.Length();
        displacementMagnitudes_[i] = mag;
        maxDisp = std::max(maxDisp, mag);
        totalDisp += static_cast<double>(mag);

        // Report progress (30% to 90% is vertex processing)
        if (i % 5000 == 0 && progressCallback_) {
            float progress = 0.3f + 0.6f * (static_cast<float>(i) / static_cast<float>(vertexCount));
            progressCallback_(progress, "Deforming vertices... (" +
                std::to_string(i) + "/" + std::to_string(vertexCount) + ")");
        }
    }

    // Set the deformed vertices (UVs are preserved automatically since we only changed positions)
    deformedMesh->SetVertices(newVertices);

    // Report progress: recalculating normals
    if (progressCallback_) {
        progressCallback_(0.9f, "Recalculating normals...");
    }

    // Recalculate normals for the deformed mesh
    deformedMesh->CalculateNormals();

    // Check for cancellation
    if (cancelled_.load()) {
        result.errorMessage = "Deformation cancelled";
        return result;
    }

    // Validate the deformed mesh
    if (!deformedMesh->Validate()) {
        result.errorMessage = "Deformed mesh failed validation";
        return result;
    }

    // Report progress: complete
    if (progressCallback_) {
        progressCallback_(1.0f, "Deformation complete!");
    }

    // Build result
    result.success = true;
    result.deformedMesh = deformedMesh;
    result.maxDisplacement = maxDisp;
    result.avgDisplacement = (vertexCount > 0) ? static_cast<float>(totalDisp / vertexCount) : 0.0f;

    qDebug() << "MeshDeformer: Deformation complete";
    qDebug() << "  Max displacement:" << result.maxDisplacement;
    qDebug() << "  Avg displacement:" << result.avgDisplacement;
    qDebug() << "  Vertices processed:" << vertexCount;

    return result;
}

} // namespace MetaVisage
