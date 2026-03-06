#include "sculpting/InflateBrush.h"
#include <cmath>

namespace MetaVisage {

InflateBrush::InflateBrush() {}
InflateBrush::~InflateBrush() {}

bool InflateBrush::Apply(Mesh& mesh, const Transform& transform,
                          const Vector3& worldCenter, const Vector3& /*worldNormal*/,
                          const Vector3& /*mouseDelta*/, float deltaTime) {
    auto affected = GetAffectedVertices(mesh, transform, worldCenter, settings_.radius);
    if (affected.empty()) return false;

    auto& vertices = mesh.GetVerticesMutable();
    const auto& normals = mesh.GetNormals();
    float strength = settings_.strength * deltaTime * 5.0f; // Scale for responsiveness

    // Move each affected vertex along its own normal (inflate/deflate uniformly)
    for (const auto& av : affected) {
        int idx = av.index;
        if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;
        if (idx >= static_cast<int>(normals.size())) continue;

        Vector3 n = normals[idx].Normalized();
        float t = av.weight * strength;

        vertices[idx].x += n.x * t;
        vertices[idx].y += n.y * t;
        vertices[idx].z += n.z * t;
    }

    return true;
}

} // namespace MetaVisage
