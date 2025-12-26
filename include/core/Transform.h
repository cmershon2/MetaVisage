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

    // Modifiers
    void SetPosition(const Vector3& position) { position_ = position; }
    void SetRotation(const Quaternion& rotation) { rotation_ = rotation; }
    void SetScale(const Vector3& scale) { scale_ = scale; }

    // Operations
    Matrix4x4 GetMatrix() const;
    void Reset();

    // Transform operations
    void Translate(const Vector3& delta);
    void Rotate(const Quaternion& delta);
    void Scale(const Vector3& delta);

private:
    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_;
};

} // namespace MetaVisage

#endif // TRANSFORM_H
