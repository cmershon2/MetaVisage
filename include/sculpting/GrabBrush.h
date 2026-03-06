#ifndef GRABBRUSH_H
#define GRABBRUSH_H

#include "sculpting/SculptBrush.h"
#include <vector>

namespace MetaVisage {

class GrabBrush : public SculptBrush {
public:
    GrabBrush();
    ~GrabBrush() override;

    bool Apply(Mesh& mesh, const Transform& transform,
               const Vector3& worldCenter, const Vector3& worldNormal,
               const Vector3& mouseDelta, float deltaTime) override;

    void BeginStroke(Mesh& mesh, const Transform& transform,
                     const Vector3& worldCenter) override;
    void EndStroke() override;

    BrushType GetType() const override { return BrushType::Grab; }

private:
    std::vector<AffectedVertex> capturedVertices_;
    bool strokeActive_;
};

} // namespace MetaVisage

#endif // GRABBRUSH_H
