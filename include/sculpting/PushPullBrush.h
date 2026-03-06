#ifndef PUSHPULLBRUSH_H
#define PUSHPULLBRUSH_H

#include "sculpting/SculptBrush.h"

namespace MetaVisage {

class PushPullBrush : public SculptBrush {
public:
    PushPullBrush();
    ~PushPullBrush() override;

    bool Apply(Mesh& mesh, const Transform& transform,
               const Vector3& worldCenter, const Vector3& worldNormal,
               const Vector3& mouseDelta, float deltaTime) override;

    BrushType GetType() const override { return BrushType::PushPull; }
};

} // namespace MetaVisage

#endif // PUSHPULLBRUSH_H
