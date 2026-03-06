#ifndef INFLATEBRUSH_H
#define INFLATEBRUSH_H

#include "sculpting/SculptBrush.h"

namespace MetaVisage {

class InflateBrush : public SculptBrush {
public:
    InflateBrush();
    ~InflateBrush() override;

    bool Apply(Mesh& mesh, const Transform& transform,
               const Vector3& worldCenter, const Vector3& worldNormal,
               const Vector3& mouseDelta, float deltaTime) override;

    BrushType GetType() const override { return BrushType::Inflate; }
};

} // namespace MetaVisage

#endif // INFLATEBRUSH_H
