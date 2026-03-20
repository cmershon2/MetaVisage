#ifndef MASKBRUSH_H
#define MASKBRUSH_H

#include "sculpting/SculptBrush.h"
#include <vector>

namespace MetaVisage {

class MaskBrush : public SculptBrush {
public:
    MaskBrush();
    ~MaskBrush() override;

    // Override Apply -- modifies vertexMask instead of vertex positions
    bool Apply(Mesh& mesh, const Transform& transform,
               const Vector3& worldCenter, const Vector3& worldNormal,
               const Vector3& mouseDelta, float deltaTime) override;

    BrushType GetType() const override { return BrushType::Mask; }

    // Set the mask vector to operate on (pointer to MorphData::vertexMask)
    void SetMaskData(std::vector<bool>* mask) { maskData_ = mask; }

    // Erase mode: false = paint mask (add), true = erase mask (remove)
    void SetEraseMode(bool erase) { eraseMode_ = erase; }
    bool IsEraseMode() const { return eraseMode_; }

private:
    std::vector<bool>* maskData_;
    bool eraseMode_;
};

} // namespace MetaVisage

#endif // MASKBRUSH_H
