#ifndef SMOOTHBRUSH_H
#define SMOOTHBRUSH_H

#include "sculpting/SculptBrush.h"
#include <vector>

namespace MetaVisage {

class SmoothBrush : public SculptBrush {
public:
    SmoothBrush();
    ~SmoothBrush() override;

    bool Apply(Mesh& mesh, const Transform& transform,
               const Vector3& worldCenter, const Vector3& worldNormal,
               const Vector3& mouseDelta, float deltaTime) override;

    BrushType GetType() const override { return BrushType::Smooth; }

    // Build adjacency data from mesh topology (called lazily on first Apply)
    void BuildAdjacency(const Mesh& mesh);

private:
    std::vector<std::vector<int>> adjacency_;
    bool adjacencyBuilt_;
};

} // namespace MetaVisage

#endif // SMOOTHBRUSH_H
