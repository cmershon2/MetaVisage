#include "deformation/MeshDeformer.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace MetaVisage {

MeshDeformer::MeshDeformer()
    : kernelType_(DeformationAlgorithm::NRICP),
      stiffness_(0.5f),
      smoothness_(0.5f),
      cancelled_(false) {
}

MeshDeformer::~MeshDeformer() {
}

void MeshDeformer::SetSourceMesh(std::shared_ptr<Mesh> mesh) {
    sourceMesh_ = mesh;
}

void MeshDeformer::SetTargetMesh(std::shared_ptr<Mesh> mesh) {
    targetMesh_ = mesh;
}

void MeshDeformer::SetMorphTransform(const Transform& transform) {
    morphTransform_ = transform;
}

void MeshDeformer::SetTargetTransform(const Transform& transform) {
    targetTransform_ = transform;
}

void MeshDeformer::SetCorrespondences(const std::vector<PointCorrespondence>& correspondences) {
    correspondences_ = correspondences;
}

Vector3 MeshDeformer::TransformPoint(const Vector3& point, const Matrix4x4& matrix) {
    float x = matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8]  * point.z + matrix.m[12];
    float y = matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9]  * point.z + matrix.m[13];
    float z = matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14];
    float w = matrix.m[3] * point.x + matrix.m[7] * point.y + matrix.m[11] * point.z + matrix.m[15];
    if (std::abs(w) > 1e-6f) { x /= w; y /= w; z /= w; }
    return Vector3(x, y, z);
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

void MeshDeformer::SetNRICPParams(const NRICPParams& params) {
    nricpParams_ = params;
}

void MeshDeformer::SetUserExcludedVertices(const std::vector<bool>& mask) {
    userExcludedVertices_ = mask;
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

    // NRICP only needs 1 landmark (ICP finds its own correspondences)
    // RBF needs at least 4 for the polynomial terms
    size_t minPoints = (kernelType_ == DeformationAlgorithm::NRICP) ? 1 : 4;
    if (correspondences_.size() < minPoints) {
        if (kernelType_ == DeformationAlgorithm::NRICP) {
            errorMessage = "At least 1 point correspondence required for NRICP";
        } else {
            errorMessage = "At least 4 point correspondences required for stable RBF deformation";
        }
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

    // We need all control points in the morph mesh's LOCAL space, because
    // the deformation operates on local-space vertices.
    //
    // Source points: morph vertex positions are already in morph local space.
    // Target points: target vertex positions need to be transformed:
    //   target local -> world (via targetTransform) -> morph local (via inverse morphTransform)

    const auto& morphVertices = sourceMesh_ ? sourceMesh_->GetVertices() : std::vector<Vector3>();
    const auto& targetVertices = targetMesh_ ? targetMesh_->GetVertices() : std::vector<Vector3>();

    Matrix4x4 targetToWorld = targetTransform_.GetMatrix();
    Matrix4x4 worldToMorphLocal = morphTransform_.GetMatrix().Inverse();
    // Combined: target local -> morph local
    Matrix4x4 targetToMorphLocal = worldToMorphLocal * targetToWorld;

    for (const auto& corr : correspondences_) {
        if (corr.morphMeshVertexIndex < 0 || corr.targetMeshVertexIndex < 0) continue;

        // Source: use actual morph mesh vertex position (local space)
        if (corr.morphMeshVertexIndex < static_cast<int>(morphVertices.size())) {
            sourcePoints.push_back(morphVertices[corr.morphMeshVertexIndex]);
        } else {
            // Fallback: transform the stored world-space position back to morph local
            sourcePoints.push_back(TransformPoint(corr.morphMeshPosition, worldToMorphLocal));
        }

        // Target: transform target vertex from target local space to morph local space
        if (corr.targetMeshVertexIndex < static_cast<int>(targetVertices.size())) {
            Vector3 targetLocalPos = targetVertices[corr.targetMeshVertexIndex];
            targetPoints.push_back(TransformPoint(targetLocalPos, targetToMorphLocal));
        } else {
            // Fallback: transform the stored world-space position to morph local
            targetPoints.push_back(TransformPoint(corr.targetMeshPosition, worldToMorphLocal));
        }
    }

    MV_LOG_INFO(QString("ExtractControlPoints: Extracted %1 control point pairs").arg(sourcePoints.size()));
}

std::shared_ptr<Mesh> MeshDeformer::CopyMesh(const std::shared_ptr<Mesh>& source) const {
    auto copy = std::make_shared<Mesh>();
    copy->SetVertices(source->GetVertices());
    copy->SetNormals(source->GetNormals());
    copy->SetUVs(source->GetUVs());
    copy->SetFaces(source->GetFaces());
    copy->SetMaterials(source->GetMaterials());
    copy->SetName(source->GetName());

    // Propagate MetaHuman-compatible export mapping
    if (source->HasAssimpMapping()) {
        copy->SetAssimpMapping(source->GetAssimpToMergedMap(),
                               source->GetOriginalAssimpNormals(),
                               source->GetOriginalAssimpVertexCount());
    }

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

    // Dispatch to NRICP if selected
    if (kernelType_ == DeformationAlgorithm::NRICP) {
        return DeformNRICP();
    }

    // --- Legacy RBF path ---

    // Report progress: starting
    if (progressCallback_) {
        progressCallback_(0.0f, "Preparing RBF deformation...");
    }

    // Extract control points from correspondences
    std::vector<Vector3> sourcePoints, targetPoints;
    ExtractControlPoints(sourcePoints, targetPoints);

    if (sourcePoints.empty()) {
        result.errorMessage = "No valid control point pairs found";
        return result;
    }

    MV_LOG_INFO(QString("Starting deformation with %1 control points and %2 vertices")
        .arg(sourcePoints.size()).arg(sourceMesh_->GetVertexCount()));

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

    MV_LOG_INFO(QString("Deformation complete: max displacement=%1, avg=%2, vertices=%3")
        .arg(result.maxDisplacement).arg(result.avgDisplacement).arg(vertexCount));

    return result;
}

int MeshDeformer::FindClosestVertex(const std::vector<Vector3>& vertices, const Vector3& point) {
    int closest = 0;
    float bestDistSq = std::numeric_limits<float>::max();
    for (size_t i = 0; i < vertices.size(); ++i) {
        Vector3 diff = vertices[i] - point;
        float distSq = diff.Dot(diff);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            closest = static_cast<int>(i);
        }
    }
    return closest;
}

std::shared_ptr<Mesh> MeshDeformer::CreateTargetMeshInMorphSpace() const {
    auto targetCopy = CopyMesh(targetMesh_);

    Matrix4x4 targetToWorld = targetTransform_.GetMatrix();
    Matrix4x4 worldToMorphLocal = morphTransform_.GetMatrix().Inverse();
    Matrix4x4 targetToMorphLocal = worldToMorphLocal * targetToWorld;

    const auto& origVerts = targetMesh_->GetVertices();
    std::vector<Vector3> transformedVerts(origVerts.size());
    for (size_t i = 0; i < origVerts.size(); ++i) {
        transformedVerts[i] = TransformPoint(origVerts[i], targetToMorphLocal);
    }
    targetCopy->SetVertices(transformedVerts);
    targetCopy->CalculateNormals();
    targetCopy->InvalidateAccelerationStructures();
    return targetCopy;
}

DeformationResult MeshDeformer::DeformNRICP() {
    DeformationResult result;

    if (progressCallback_) {
        progressCallback_(0.01f, "Preparing NRICP deformation...");
    }

    // Extract landmark correspondences in morph local space
    std::vector<Vector3> sourcePoints, targetPoints;
    ExtractControlPoints(sourcePoints, targetPoints);

    if (sourcePoints.empty()) {
        result.errorMessage = "No valid landmark pairs found";
        return result;
    }

    // Convert to landmark pairs: (source vertex index, target position in morph local space)
    const auto& morphVertices = sourceMesh_->GetVertices();
    std::vector<std::pair<int, Vector3>> landmarks;
    for (size_t i = 0; i < sourcePoints.size(); ++i) {
        int closestIdx = FindClosestVertex(morphVertices, sourcePoints[i]);
        landmarks.push_back({closestIdx, targetPoints[i]});
    }

    // Transform target mesh to morph local space
    if (progressCallback_) {
        progressCallback_(0.02f, "Transforming target mesh to morph space...");
    }
    auto targetInMorphSpace = CreateTargetMeshInMorphSpace();

    // Set up NRICP solver
    NRICPDeformer nricp;
    nricp.SetSourceMesh(sourceMesh_->GetVertices(),
                        sourceMesh_->GetNormals(),
                        sourceMesh_->GetFaces());
    nricp.SetTargetMesh(targetInMorphSpace);
    nricp.SetLandmarks(landmarks);
    nricp.SetParams(nricpParams_);
    if (!userExcludedVertices_.empty()) {
        nricp.SetUserExcludedVertices(userExcludedVertices_);
    }
    nricp.SetCancellationFlag(&cancelled_);
    nricp.SetProgressCallback(progressCallback_);

    // Run NRICP
    std::vector<Vector3> deformedVertices = nricp.Solve();

    if (deformedVertices.empty()) {
        result.errorMessage = cancelled_.load() ? "Deformation cancelled" : "NRICP solver failed";
        return result;
    }

    // Build result mesh
    auto deformedMesh = CopyMesh(sourceMesh_);
    deformedMesh->SetVertices(deformedVertices);

    if (progressCallback_) {
        progressCallback_(0.98f, "Recalculating normals...");
    }
    deformedMesh->CalculateNormals();

    if (!deformedMesh->Validate()) {
        result.errorMessage = "Deformed mesh failed validation";
        return result;
    }

    // Compute displacement magnitudes for heat map
    const auto& origVerts = sourceMesh_->GetVertices();
    displacementMagnitudes_.resize(origVerts.size());
    float maxDisp = 0.0f;
    double totalDisp = 0.0;
    for (size_t i = 0; i < origVerts.size(); ++i) {
        float mag = (deformedVertices[i] - origVerts[i]).Length();
        displacementMagnitudes_[i] = mag;
        maxDisp = std::max(maxDisp, mag);
        totalDisp += static_cast<double>(mag);
    }

    result.success = true;
    result.deformedMesh = deformedMesh;
    result.maxDisplacement = maxDisp;
    result.avgDisplacement = origVerts.empty() ? 0.0f : static_cast<float>(totalDisp / origVerts.size());

    MV_LOG_INFO(QString("NRICP deformation complete: max displacement=%1, avg=%2, vertices=%3")
        .arg(result.maxDisplacement).arg(result.avgDisplacement).arg(origVerts.size()));

    return result;
}

} // namespace MetaVisage
