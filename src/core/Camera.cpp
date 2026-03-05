#include "core/Camera.h"
#include <cmath>

namespace MetaVisage {

Camera::Camera()
    : position_(0.0f, 0.0f, 5.0f),
      target_(0.0f, 0.0f, 0.0f),
      up_(0.0f, 1.0f, 0.0f),
      fov_(45.0f),
      nearPlane_(0.1f),
      farPlane_(100000.0f),  // Large far plane for big meshes
      projectionMode_(ProjectionMode::Perspective),
      distance_(5.0f),
      yaw_(0.0f),
      pitch_(0.0f) {
}

Camera::~Camera() {
}

void Camera::CopyStateFrom(const Camera& other) {
    position_ = other.position_;
    target_ = other.target_;
    up_ = other.up_;
    fov_ = other.fov_;
    projectionMode_ = other.projectionMode_;
    distance_ = other.distance_;
    yaw_ = other.yaw_;
    pitch_ = other.pitch_;
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
    // Use percentage-based zoom for consistent feel at any distance
    const float zoomFactor = 0.15f;  // 15% per scroll notch for faster zooming

    if (delta > 0) {
        distance_ *= (1.0f - zoomFactor);  // Zoom in
    } else if (delta < 0) {
        distance_ *= (1.0f + zoomFactor);  // Zoom out
    }

    // Clamp distance - allow very close for small meshes, very far for large
    if (distance_ < 0.01f) distance_ = 0.01f;
    if (distance_ > 100000.0f) distance_ = 100000.0f;

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

void Camera::FocusOnBounds(const BoundingBox& bounds) {
    // Calculate center and size of bounding box
    Vector3 center = bounds.Center();
    Vector3 size = bounds.Size();

    // Calculate the maximum dimension
    float maxDim = std::max(std::max(size.x, size.y), size.z);

    // Calculate distance needed to fit the object in view
    // Using FOV to determine appropriate distance
    const float PI = 3.14159265359f;
    float halfFovRad = (fov_ * 0.5f) * PI / 180.0f;
    float distance = (maxDim * 0.5f) / std::tan(halfFovRad);

    // Add padding to ensure object fits comfortably in view
    distance *= 2.0f;

    // Ensure minimum distance based on object size
    float minDist = maxDim * 0.5f;
    if (distance < minDist) distance = minDist;

    // Reset to front view (looking down -Z axis at the object)
    yaw_ = 0.0f;
    pitch_ = 0.0f;

    target_ = center;
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
    // Dynamically adjust near plane based on distance to avoid z-fighting
    // Near plane should be about 1/1000th of the distance, minimum 0.001 for small meshes
    float dynamicNear = std::max(0.001f, distance_ * 0.001f);
    // Far plane should be much larger than distance
    float dynamicFar = std::max(1000.0f, distance_ * 100.0f);

    if (projectionMode_ == ProjectionMode::Perspective) {
        return Matrix4x4::Perspective(fov_, aspectRatio, dynamicNear, dynamicFar);
    } else {
        float orthoSize = distance_;
        float left = -orthoSize * aspectRatio;
        float right = orthoSize * aspectRatio;
        float bottom = -orthoSize;
        float top = orthoSize;
        return Matrix4x4::Orthographic(left, right, bottom, top, dynamicNear, dynamicFar);
    }
}

} // namespace MetaVisage
