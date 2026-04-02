#include "core/Transform.h"

namespace MetaVisage {

Transform::Transform()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(Quaternion::Identity()),
      scale_(1.0f, 1.0f, 1.0f),
      eulerHint_(0.0f, 0.0f, 0.0f),
      eulerHintValid_(true) {
}

Transform::~Transform() {
}

Matrix4x4 Transform::GetMatrix() const {
    // Model matrix = Translation * Rotation * Scale (TRS order for column-major)
    Matrix4x4 translationMatrix = Matrix4x4::Translation(position_);
    Matrix4x4 rotationMatrix = rotation_.ToMatrix();
    Matrix4x4 scaleMatrix = Matrix4x4::Scale(scale_);

    // Column-major: apply scale first, then rotation, then translation
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void Transform::Reset() {
    position_ = Vector3(0.0f, 0.0f, 0.0f);
    rotation_ = Quaternion::Identity();
    scale_ = Vector3(1.0f, 1.0f, 1.0f);
    eulerHint_ = Vector3(0.0f, 0.0f, 0.0f);
    eulerHintValid_ = true;
}

Vector3 Transform::GetEulerAngles() const {
    if (eulerHintValid_) {
        return eulerHint_;
    }
    return rotation_.ToEulerAngles();
}

void Transform::SetRotationFromEuler(float x, float y, float z) {
    eulerHint_ = Vector3(x, y, z);
    eulerHintValid_ = true;
    rotation_ = Quaternion::FromEulerAngles(x, y, z);
}

void Transform::Translate(const Vector3& delta) {
    position_ = position_ + delta;
}

void Transform::Rotate(const Quaternion& delta) {
    // Quaternion multiplication: new rotation = delta * current rotation
    // This applies the delta rotation to the existing rotation
    rotation_ = (delta * rotation_).Normalized();
    eulerHintValid_ = false;  // Mouse/gizmo rotation invalidates the hint
}

void Transform::Scale(const Vector3& delta) {
    scale_.x *= delta.x;
    scale_.y *= delta.y;
    scale_.z *= delta.z;
}

} // namespace MetaVisage
