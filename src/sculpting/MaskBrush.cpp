#include "sculpting/MaskBrush.h"

namespace MetaVisage {

MaskBrush::MaskBrush()
    : maskData_(nullptr), eraseMode_(false) {}

MaskBrush::~MaskBrush() {}

bool MaskBrush::Apply(Mesh& mesh, const Transform& transform,
                       const Vector3& worldCenter, const Vector3& /*worldNormal*/,
                       const Vector3& /*mouseDelta*/, float /*deltaTime*/) {
    if (!maskData_) return false;

    // Ensure mask vector is sized to match mesh
    size_t vertexCount = mesh.GetVertexCount();
    if (maskData_->size() != vertexCount) {
        maskData_->resize(vertexCount, false);
    }

    auto affected = GetAffectedVertices(mesh, transform, worldCenter, settings_.radius);
    if (affected.empty()) return false;

    bool modified = false;
    bool targetValue = !eraseMode_;

    for (const auto& av : affected) {
        int idx = av.index;
        if (idx < 0 || idx >= static_cast<int>(vertexCount)) continue;
        // Use weight threshold for crisp mask edges
        if (av.weight > 0.1f) {
            if ((*maskData_)[idx] != targetValue) {
                (*maskData_)[idx] = targetValue;
                modified = true;
            }
        }
    }

    return modified;
}

} // namespace MetaVisage
