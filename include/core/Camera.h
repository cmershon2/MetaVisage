#ifndef CAMERA_H
#define CAMERA_H

#include "core/Types.h"

namespace MetaVisage {

class Camera {
public:
    Camera();
    ~Camera();

    // Accessors
    const Vector3& GetPosition() const { return position_; }
    const Vector3& GetTarget() const { return target_; }
    const Vector3& GetUp() const { return up_; }
    float GetFOV() const { return fov_; }
    float GetNearPlane() const { return nearPlane_; }
    float GetFarPlane() const { return farPlane_; }
    ProjectionMode GetProjectionMode() const { return projectionMode_; }

    // Modifiers
    void SetPosition(const Vector3& position) { position_ = position; }
    void SetTarget(const Vector3& target) { target_ = target; }
    void SetUp(const Vector3& up) { up_ = up; }
    void SetFOV(float fov) { fov_ = fov; }
    void SetProjectionMode(ProjectionMode mode) { projectionMode_ = mode; }

    // Camera operations
    void Orbit(float deltaX, float deltaY);
    void Pan(float deltaX, float deltaY);
    void Zoom(float delta);
    void Reset();
    void FocusOn(const Vector3& point, float distance);
    void FocusOnBounds(const BoundingBox& bounds);

    // Matrix generation
    Matrix4x4 GetViewMatrix() const;
    Matrix4x4 GetProjectionMatrix(float aspectRatio) const;

private:
    Vector3 position_;
    Vector3 target_;
    Vector3 up_;
    float fov_;
    float nearPlane_;
    float farPlane_;
    ProjectionMode projectionMode_;

    // Internal state
    float distance_;
    float yaw_;
    float pitch_;

    void UpdatePositionFromAngles();
};

} // namespace MetaVisage

#endif // CAMERA_H
