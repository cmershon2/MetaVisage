#include "core/Transform.h"

namespace MetaVisage {

Transform::Transform()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(Quaternion::Identity()),
      scale_(1.0f, 1.0f, 1.0f) {
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
}

void Transform::Translate(const Vector3& delta) {
    position_ = position_ + delta;
}

void Transform::Rotate(const Quaternion& delta) {
    // TODO: Implement quaternion multiplication
    rotation_ = delta;
}

void Transform::Scale(const Vector3& delta) {
    scale_.x *= delta.x;
    scale_.y *= delta.y;
    scale_.z *= delta.z;
}

} // namespace MetaVisage
