#include "sculpting/GrabBrush.h"
#include <cmath>

namespace MetaVisage {

GrabBrush::GrabBrush()
    : strokeActive_(false) {}

GrabBrush::~GrabBrush() {}

void GrabBrush::BeginStroke(Mesh& mesh, const Transform& transform,
                             const Vector3& worldCenter) {
    // Capture the vertices within brush radius at stroke start
    capturedVertices_ = GetAffectedVertices(mesh, transform, worldCenter, settings_.radius);
    strokeActive_ = true;
}

void GrabBrush::EndStroke() {
    capturedVertices_.clear();
    strokeActive_ = false;
}

bool GrabBrush::Apply(Mesh& mesh, const Transform& transform,
                       const Vector3& /*worldCenter*/, const Vector3& /*worldNormal*/,
                       const Vector3& mouseDelta, float /*deltaTime*/) {
    if (!strokeActive_ || capturedVertices_.empty()) return false;

    auto& vertices = mesh.GetVerticesMutable();
    float strength = settings_.strength;

    // Convert world-space delta to local/model space
    Matrix4x4 invModel = transform.GetMatrix().Inverse();

    // Transform direction only (no translation component)
    Vector3 localDelta;
    localDelta.x = invModel.m[0] * mouseDelta.x + invModel.m[4] * mouseDelta.y + invModel.m[8] * mouseDelta.z;
    localDelta.y = invModel.m[1] * mouseDelta.x + invModel.m[5] * mouseDelta.y + invModel.m[9] * mouseDelta.z;
    localDelta.z = invModel.m[2] * mouseDelta.x + invModel.m[6] * mouseDelta.y + invModel.m[10] * mouseDelta.z;

    // Move captured vertices by delta, weighted by falloff and strength
    for (const auto& av : capturedVertices_) {
        int idx = av.index;
        if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;

        float t = av.weight * strength;
        vertices[idx].x += localDelta.x * t;
        vertices[idx].y += localDelta.y * t;
        vertices[idx].z += localDelta.z * t;
    }

    return true;
}

} // namespace MetaVisage
