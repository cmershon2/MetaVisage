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
    // TODO: Implement proper matrix composition
    // For now, return identity matrix
    return Matrix4x4::Identity();
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
