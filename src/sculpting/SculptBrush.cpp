#include "sculpting/SculptBrush.h"
#include "utils/SpatialHash.h"
#include <cmath>

namespace MetaVisage {

SculptBrush::SculptBrush() {}
SculptBrush::~SculptBrush() {}

void SculptBrush::BeginStroke(Mesh& /*mesh*/, const Transform& /*transform*/,
                               const Vector3& /*worldCenter*/) {}

void SculptBrush::EndStroke() {}

float SculptBrush::CalculateFalloff(float distance, float radius) const {
    if (distance >= radius || radius <= 0.0f) return 0.0f;

    float t = distance / radius; // 0 at center, 1 at edge

    switch (settings_.falloff) {
        case FalloffType::Linear:
            return 1.0f - t;

        case FalloffType::Smooth:
            // Smooth hermite interpolation (3t^2 - 2t^3)
            return 1.0f - (3.0f * t * t - 2.0f * t * t * t);

        case FalloffType::Sharp:
            // Sharp falloff (1 - t^3)
            return 1.0f - t * t * t;
    }

    return 0.0f;
}

std::vector<AffectedVertex> SculptBrush::GetAffectedVertices(
    const Mesh& mesh, const Transform& transform,
    const Vector3& worldCenter, float radius) const {

    std::vector<AffectedVertex> affected;
    const auto& vertices = mesh.GetVertices();
    Matrix4x4 modelMatrix = transform.GetMatrix();

    float radiusSq = radius * radius;

    for (size_t i = 0; i < vertices.size(); ++i) {
        // Transform vertex to world space
        const Vector3& lp = vertices[i];
        Vector3 wp;
        wp.x = modelMatrix.m[0] * lp.x + modelMatrix.m[4] * lp.y + modelMatrix.m[8] * lp.z + modelMatrix.m[12];
        wp.y = modelMatrix.m[1] * lp.x + modelMatrix.m[5] * lp.y + modelMatrix.m[9] * lp.z + modelMatrix.m[13];
        wp.z = modelMatrix.m[2] * lp.x + modelMatrix.m[6] * lp.y + modelMatrix.m[10] * lp.z + modelMatrix.m[14];

        Vector3 diff = wp - worldCenter;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < radiusSq) {
            float dist = std::sqrt(distSq);
            float weight = CalculateFalloff(dist, radius);
            if (weight > 0.001f) {
                affected.push_back({static_cast<int>(i), weight});
            }
        }
    }

    return affected;
}

std::vector<AffectedVertex> SculptBrush::GetAffectedVertices(
    const Mesh& mesh, const Transform& transform,
    const Vector3& worldCenter, float radius,
    const SpatialHash& spatialHash) const {

    std::vector<AffectedVertex> affected;
    const auto& vertices = mesh.GetVertices();
    Matrix4x4 modelMatrix = transform.GetMatrix();

    // Transform world center to local space for spatial hash query
    Matrix4x4 invModel = modelMatrix.Inverse();
    Vector3 localCenter;
    localCenter.x = invModel.m[0] * worldCenter.x + invModel.m[4] * worldCenter.y + invModel.m[8] * worldCenter.z + invModel.m[12];
    localCenter.y = invModel.m[1] * worldCenter.x + invModel.m[5] * worldCenter.y + invModel.m[9] * worldCenter.z + invModel.m[13];
    localCenter.z = invModel.m[2] * worldCenter.x + invModel.m[6] * worldCenter.y + invModel.m[10] * worldCenter.z + invModel.m[14];

    // Approximate local radius (use max scale component)
    float sx = std::sqrt(modelMatrix.m[0]*modelMatrix.m[0] + modelMatrix.m[1]*modelMatrix.m[1] + modelMatrix.m[2]*modelMatrix.m[2]);
    float localRadius = (sx > 0.001f) ? radius / sx : radius;

    // Query spatial hash for candidate vertices in local space
    auto candidates = spatialHash.Query(localCenter, localRadius);

    float radiusSq = radius * radius;

    for (int idx : candidates) {
        const Vector3& lp = vertices[idx];
        Vector3 wp;
        wp.x = modelMatrix.m[0] * lp.x + modelMatrix.m[4] * lp.y + modelMatrix.m[8] * lp.z + modelMatrix.m[12];
        wp.y = modelMatrix.m[1] * lp.x + modelMatrix.m[5] * lp.y + modelMatrix.m[9] * lp.z + modelMatrix.m[13];
        wp.z = modelMatrix.m[2] * lp.x + modelMatrix.m[6] * lp.y + modelMatrix.m[10] * lp.z + modelMatrix.m[14];

        Vector3 diff = wp - worldCenter;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < radiusSq) {
            float dist = std::sqrt(distSq);
            float weight = CalculateFalloff(dist, radius);
            if (weight > 0.001f) {
                affected.push_back({idx, weight});
            }
        }
    }

    return affected;
}

} // namespace MetaVisage
