#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "core/Types.h"

namespace MetaVisage {

class Transform {
public:
    Transform();
    ~Transform();

    // Accessors
    const Vector3& GetPosition() const { return position_; }
    const Quaternion& GetRotation() const { return rotation_; }
    const Vector3& GetScale() const { return scale_; }

    // Get Euler angles for display. Uses the cached hint if available (set via
    // SetRotationFromEuler), otherwise decomposes the quaternion.
    Vector3 GetEulerAngles() const;

    // Modifiers
    void SetPosition(const Vector3& position) { position_ = position; }
    void SetRotation(const Quaternion& rotation) { rotation_ = rotation; eulerHintValid_ = false; }
    void SetScale(const Vector3& scale) { scale_ = scale; }

    // Set rotation from Euler angles (degrees, XYZ extrinsic). Stores the exact
    // Euler values so GetEulerAngles() returns them without decomposition jitter.
    void SetRotationFromEuler(float x, float y, float z);

    // Operations
    Matrix4x4 GetMatrix() const;
    void Reset();

    // Transform operations
    void Translate(const Vector3& delta);
    void Rotate(const Quaternion& delta);  // Invalidates Euler hint
    void Scale(const Vector3& delta);

private:
    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_;

    // Cached Euler angles to avoid decomposition jitter
    Vector3 eulerHint_;
    bool eulerHintValid_ = false;
};

} // namespace MetaVisage

#endif // TRANSFORM_H
