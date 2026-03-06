#include "sculpting/PushPullBrush.h"
#include <cmath>

namespace MetaVisage {

PushPullBrush::PushPullBrush() {}
PushPullBrush::~PushPullBrush() {}

bool PushPullBrush::Apply(Mesh& mesh, const Transform& transform,
                           const Vector3& worldCenter, const Vector3& worldNormal,
                           const Vector3& /*mouseDelta*/, float deltaTime) {
    auto affected = GetAffectedVertices(mesh, transform, worldCenter, settings_.radius);
    if (affected.empty()) return false;

    auto& vertices = mesh.GetVerticesMutable();
    float strength = settings_.strength * deltaTime * 5.0f; // Scale for responsiveness

    // Convert world-space normal direction to local/model space for displacement
    Matrix4x4 invModel = transform.GetMatrix().Inverse();
    Vector3 localNormal;
    localNormal.x = invModel.m[0] * worldNormal.x + invModel.m[4] * worldNormal.y + invModel.m[8] * worldNormal.z;
    localNormal.y = invModel.m[1] * worldNormal.x + invModel.m[5] * worldNormal.y + invModel.m[9] * worldNormal.z;
    localNormal.z = invModel.m[2] * worldNormal.x + invModel.m[6] * worldNormal.y + invModel.m[10] * worldNormal.z;
    localNormal = localNormal.Normalized();

    // Move affected vertices along the surface normal direction
    for (const auto& av : affected) {
        int idx = av.index;
        if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;

        float t = av.weight * strength;
        vertices[idx].x += localNormal.x * t;
        vertices[idx].y += localNormal.y * t;
        vertices[idx].z += localNormal.z * t;
    }

    return true;
}

} // namespace MetaVisage
