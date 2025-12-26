#include "core/Camera.h"
#include <cmath>

namespace MetaVisage {

Camera::Camera()
    : position_(0.0f, 0.0f, 5.0f),
      target_(0.0f, 0.0f, 0.0f),
      up_(0.0f, 1.0f, 0.0f),
      fov_(45.0f),
      nearPlane_(0.1f),
      farPlane_(1000.0f),
      projectionMode_(ProjectionMode::Perspective),
      distance_(5.0f),
      yaw_(0.0f),
      pitch_(0.0f) {
}

Camera::~Camera() {
}

void Camera::Orbit(float deltaX, float deltaY) {
    const float sensitivity = 0.5f;
    yaw_ -= deltaX * sensitivity;
    pitch_ += deltaY * sensitivity;

    // Clamp pitch to avoid gimbal lock
    const float maxPitch = 89.0f;
    if (pitch_ > maxPitch) pitch_ = maxPitch;
    if (pitch_ < -maxPitch) pitch_ = -maxPitch;

    UpdatePositionFromAngles();
}

void Camera::Pan(float deltaX, float deltaY) {
    const float sensitivity = 0.01f;

    // Calculate right vector
    Vector3 forward = (target_ - position_).Normalized();
    Vector3 right = forward.Cross(up_).Normalized();
    Vector3 cameraUp = right.Cross(forward).Normalized();

    // Pan camera and target
    Vector3 panOffset = (right * deltaX + cameraUp * deltaY) * sensitivity * distance_;
    position_ = position_ + panOffset;
    target_ = target_ + panOffset;
}

void Camera::Zoom(float delta) {
    const float sensitivity = 0.1f;
    distance_ -= delta * sensitivity;

    // Clamp distance
    if (distance_ < 0.1f) distance_ = 0.1f;
    if (distance_ > 100.0f) distance_ = 100.0f;

    UpdatePositionFromAngles();
}

void Camera::Reset() {
    position_ = Vector3(0.0f, 0.0f, 5.0f);
    target_ = Vector3(0.0f, 0.0f, 0.0f);
    up_ = Vector3(0.0f, 1.0f, 0.0f);
    distance_ = 5.0f;
    yaw_ = 0.0f;
    pitch_ = 0.0f;
}

void Camera::FocusOn(const Vector3& point, float distance) {
    target_ = point;
    distance_ = distance;
    UpdatePositionFromAngles();
}

void Camera::UpdatePositionFromAngles() {
    // Convert angles to radians
    const float PI = 3.14159265359f;
    float yawRad = yaw_ * PI / 180.0f;
    float pitchRad = pitch_ * PI / 180.0f;

    // Calculate new position
    float x = distance_ * std::cos(pitchRad) * std::sin(yawRad);
    float y = distance_ * std::sin(pitchRad);
    float z = distance_ * std::cos(pitchRad) * std::cos(yawRad);

    position_ = target_ + Vector3(x, y, z);
}

Matrix4x4 Camera::GetViewMatrix() const {
    return Matrix4x4::LookAt(position_, target_, up_);
}

Matrix4x4 Camera::GetProjectionMatrix(float aspectRatio) const {
    if (projectionMode_ == ProjectionMode::Perspective) {
        return Matrix4x4::Perspective(fov_, aspectRatio, nearPlane_, farPlane_);
    } else {
        float orthoSize = distance_;
        float left = -orthoSize * aspectRatio;
        float right = orthoSize * aspectRatio;
        float bottom = -orthoSize;
        float top = orthoSize;
        return Matrix4x4::Orthographic(left, right, bottom, top, nearPlane_, farPlane_);
    }
}

} // namespace MetaVisage
