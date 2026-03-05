#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace MetaVisage {

// Forward declarations
class Mesh;
class Material;
struct Matrix4x4;

// Basic vector types
struct Vector2 {
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x_, float y_) : x(x_), y(y_) {}

    float Length() const { return std::sqrt(x * x + y * y); }
};

struct Vector3 {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    Vector3 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return Vector3(x / len, y / len, z / len);
        }
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
};

struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

    // Create quaternion from axis-angle (angle in degrees)
    static Quaternion FromAxisAngle(const Vector3& axis, float angleDegrees) {
        const float PI = 3.14159265359f;
        float angleRad = angleDegrees * PI / 180.0f;
        float halfAngle = angleRad * 0.5f;
        float s = std::sin(halfAngle);
        Vector3 normalizedAxis = axis.Normalized();
        return Quaternion(
            normalizedAxis.x * s,
            normalizedAxis.y * s,
            normalizedAxis.z * s,
            std::cos(halfAngle)
        );
    }

    // Quaternion multiplication (Hamilton product)
    // Combines rotations: result = this * other (this rotation applied after other)
    Quaternion operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }

    // Normalize quaternion to unit length
    Quaternion Normalized() const {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len > 0.0f) {
            return Quaternion(x / len, y / len, z / len, w / len);
        }
        return Identity();
    }

    // Create quaternion from Euler angles (in degrees, XYZ order)
    static Quaternion FromEulerAngles(float pitchDeg, float yawDeg, float rollDeg) {
        const float PI = 3.14159265359f;
        float pitch = pitchDeg * PI / 180.0f * 0.5f;  // X
        float yaw = yawDeg * PI / 180.0f * 0.5f;      // Y
        float roll = rollDeg * PI / 180.0f * 0.5f;    // Z

        float cp = std::cos(pitch);
        float sp = std::sin(pitch);
        float cy = std::cos(yaw);
        float sy = std::sin(yaw);
        float cr = std::cos(roll);
        float sr = std::sin(roll);

        return Quaternion(
            sr * cp * cy - cr * sp * sy,  // x
            cr * sp * cy + sr * cp * sy,  // y
            cr * cp * sy - sr * sp * cy,  // z
            cr * cp * cy + sr * sp * sy   // w
        );
    }

    // Convert quaternion to Euler angles (in degrees, XYZ order)
    Vector3 ToEulerAngles() const {
        const float PI = 3.14159265359f;
        Vector3 euler;

        // Roll (X-axis rotation)
        float sinr_cosp = 2.0f * (w * x + y * z);
        float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        euler.x = std::atan2(sinr_cosp, cosr_cosp);

        // Pitch (Y-axis rotation)
        float sinp = 2.0f * (w * y - z * x);
        if (std::abs(sinp) >= 1.0f) {
            euler.y = std::copysign(PI / 2.0f, sinp);  // Use 90 degrees if out of range
        } else {
            euler.y = std::asin(sinp);
        }

        // Yaw (Z-axis rotation)
        float siny_cosp = 2.0f * (w * z + x * y);
        float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        euler.z = std::atan2(siny_cosp, cosy_cosp);

        // Convert to degrees
        euler.x *= 180.0f / PI;
        euler.y *= 180.0f / PI;
        euler.z *= 180.0f / PI;

        return euler;
    }

    // Convert quaternion to rotation matrix (column-major for OpenGL)
    // Defined after Matrix4x4
    Matrix4x4 ToMatrix() const;
};

struct Matrix4x4 {
    float m[16];

    Matrix4x4() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4x4 Identity() { return Matrix4x4(); }

    static Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
        Vector3 f = (center - eye).Normalized();
        Vector3 s = f.Cross(up).Normalized();
        Vector3 u = s.Cross(f);

        Matrix4x4 result;
        result.m[0] = s.x;
        result.m[4] = s.y;
        result.m[8] = s.z;
        result.m[1] = u.x;
        result.m[5] = u.y;
        result.m[9] = u.z;
        result.m[2] = -f.x;
        result.m[6] = -f.y;
        result.m[10] = -f.z;
        result.m[12] = -s.Dot(eye);
        result.m[13] = -u.Dot(eye);
        result.m[14] = f.Dot(eye);
        result.m[15] = 1.0f;
        return result;
    }

    static Matrix4x4 Perspective(float fov, float aspect, float nearPlane, float farPlane) {
        const float PI = 3.14159265359f;
        float tanHalfFovy = std::tan(fov * PI / 360.0f);

        Matrix4x4 result;
        for (int i = 0; i < 16; ++i) result.m[i] = 0.0f;

        result.m[0] = 1.0f / (aspect * tanHalfFovy);
        result.m[5] = 1.0f / tanHalfFovy;
        result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        return result;
    }

    static Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
        Matrix4x4 result;
        for (int i = 0; i < 16; ++i) result.m[i] = 0.0f;

        result.m[0] = 2.0f / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (farPlane - nearPlane);
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[15] = 1.0f;
        return result;
    }

    // Create translation matrix (column-major for OpenGL)
    static Matrix4x4 Translation(const Vector3& t) {
        Matrix4x4 result;
        result.m[12] = t.x;
        result.m[13] = t.y;
        result.m[14] = t.z;
        return result;
    }

    // Create scale matrix (column-major for OpenGL)
    static Matrix4x4 Scale(const Vector3& s) {
        Matrix4x4 result;
        result.m[0] = s.x;
        result.m[5] = s.y;
        result.m[10] = s.z;
        return result;
    }

    // Column-major matrix multiplication (for OpenGL)
    // result = this * other (column-major convention)
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 16; ++i) result.m[i] = 0.0f;

        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                for (int k = 0; k < 4; ++k) {
                    // Column-major: element at (row, col) is at index [col * 4 + row]
                    result.m[col * 4 + row] += m[k * 4 + row] * other.m[col * 4 + k];
                }
            }
        }
        return result;
    }

    Matrix4x4 Transpose() const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[j * 4 + i] = m[i * 4 + j];
            }
        }
        return result;
    }

    // Inverse for 4x4 matrix (used for normal matrix calculation)
    Matrix4x4 Inverse() const {
        Matrix4x4 inv;
        float det;

        inv.m[0] = m[5]  * m[10] * m[15] -
                   m[5]  * m[11] * m[14] -
                   m[9]  * m[6]  * m[15] +
                   m[9]  * m[7]  * m[14] +
                   m[13] * m[6]  * m[11] -
                   m[13] * m[7]  * m[10];

        inv.m[4] = -m[4]  * m[10] * m[15] +
                    m[4]  * m[11] * m[14] +
                    m[8]  * m[6]  * m[15] -
                    m[8]  * m[7]  * m[14] -
                    m[12] * m[6]  * m[11] +
                    m[12] * m[7]  * m[10];

        inv.m[8] = m[4]  * m[9] * m[15] -
                   m[4]  * m[11] * m[13] -
                   m[8]  * m[5] * m[15] +
                   m[8]  * m[7] * m[13] +
                   m[12] * m[5] * m[11] -
                   m[12] * m[7] * m[9];

        inv.m[12] = -m[4]  * m[9] * m[14] +
                     m[4]  * m[10] * m[13] +
                     m[8]  * m[5] * m[14] -
                     m[8]  * m[6] * m[13] -
                     m[12] * m[5] * m[10] +
                     m[12] * m[6] * m[9];

        inv.m[1] = -m[1]  * m[10] * m[15] +
                    m[1]  * m[11] * m[14] +
                    m[9]  * m[2] * m[15] -
                    m[9]  * m[3] * m[14] -
                    m[13] * m[2] * m[11] +
                    m[13] * m[3] * m[10];

        inv.m[5] = m[0]  * m[10] * m[15] -
                   m[0]  * m[11] * m[14] -
                   m[8]  * m[2] * m[15] +
                   m[8]  * m[3] * m[14] +
                   m[12] * m[2] * m[11] -
                   m[12] * m[3] * m[10];

        inv.m[9] = -m[0]  * m[9] * m[15] +
                    m[0]  * m[11] * m[13] +
                    m[8]  * m[1] * m[15] -
                    m[8]  * m[3] * m[13] -
                    m[12] * m[1] * m[11] +
                    m[12] * m[3] * m[9];

        inv.m[13] = m[0]  * m[9] * m[14] -
                    m[0]  * m[10] * m[13] -
                    m[8]  * m[1] * m[14] +
                    m[8]  * m[2] * m[13] +
                    m[12] * m[1] * m[10] -
                    m[12] * m[2] * m[9];

        inv.m[2] = m[1]  * m[6] * m[15] -
                   m[1]  * m[7] * m[14] -
                   m[5]  * m[2] * m[15] +
                   m[5]  * m[3] * m[14] +
                   m[13] * m[2] * m[7] -
                   m[13] * m[3] * m[6];

        inv.m[6] = -m[0]  * m[6] * m[15] +
                    m[0]  * m[7] * m[14] +
                    m[4]  * m[2] * m[15] -
                    m[4]  * m[3] * m[14] -
                    m[12] * m[2] * m[7] +
                    m[12] * m[3] * m[6];

        inv.m[10] = m[0]  * m[5] * m[15] -
                    m[0]  * m[7] * m[13] -
                    m[4]  * m[1] * m[15] +
                    m[4]  * m[3] * m[13] +
                    m[12] * m[1] * m[7] -
                    m[12] * m[3] * m[5];

        inv.m[14] = -m[0]  * m[5] * m[14] +
                     m[0]  * m[6] * m[13] +
                     m[4]  * m[1] * m[14] -
                     m[4]  * m[2] * m[13] -
                     m[12] * m[1] * m[6] +
                     m[12] * m[2] * m[5];

        inv.m[3] = -m[1] * m[6] * m[11] +
                    m[1] * m[7] * m[10] +
                    m[5] * m[2] * m[11] -
                    m[5] * m[3] * m[10] -
                    m[9] * m[2] * m[7] +
                    m[9] * m[3] * m[6];

        inv.m[7] = m[0] * m[6] * m[11] -
                   m[0] * m[7] * m[10] -
                   m[4] * m[2] * m[11] +
                   m[4] * m[3] * m[10] +
                   m[8] * m[2] * m[7] -
                   m[8] * m[3] * m[6];

        inv.m[11] = -m[0] * m[5] * m[11] +
                     m[0] * m[7] * m[9] +
                     m[4] * m[1] * m[11] -
                     m[4] * m[3] * m[9] -
                     m[8] * m[1] * m[7] +
                     m[8] * m[3] * m[5];

        inv.m[15] = m[0] * m[5] * m[10] -
                    m[0] * m[6] * m[9] -
                    m[4] * m[1] * m[10] +
                    m[4] * m[2] * m[9] +
                    m[8] * m[1] * m[6] -
                    m[8] * m[2] * m[5];

        det = m[0] * inv.m[0] + m[1] * inv.m[4] + m[2] * inv.m[8] + m[3] * inv.m[12];

        if (det == 0.0f) {
            return Matrix4x4::Identity();
        }

        det = 1.0f / det;

        Matrix4x4 result;
        for (int i = 0; i < 16; i++) {
            result.m[i] = inv.m[i] * det;
        }

        return result;
    }

    const float* Data() const { return m; }
};

// Quaternion::ToMatrix implementation (needs Matrix4x4 to be fully defined)
inline Matrix4x4 Quaternion::ToMatrix() const {
    Matrix4x4 result;

    float xx = x * x;
    float xy = x * y;
    float xz = x * z;
    float xw = x * w;
    float yy = y * y;
    float yz = y * z;
    float yw = y * w;
    float zz = z * z;
    float zw = z * w;

    // Column-major order for OpenGL
    result.m[0] = 1.0f - 2.0f * (yy + zz);
    result.m[1] = 2.0f * (xy + zw);
    result.m[2] = 2.0f * (xz - yw);
    result.m[3] = 0.0f;

    result.m[4] = 2.0f * (xy - zw);
    result.m[5] = 1.0f - 2.0f * (xx + zz);
    result.m[6] = 2.0f * (yz + xw);
    result.m[7] = 0.0f;

    result.m[8] = 2.0f * (xz + yw);
    result.m[9] = 2.0f * (yz - xw);
    result.m[10] = 1.0f - 2.0f * (xx + yy);
    result.m[11] = 0.0f;

    result.m[12] = 0.0f;
    result.m[13] = 0.0f;
    result.m[14] = 0.0f;
    result.m[15] = 1.0f;

    return result;
}

// Ray for ray casting
struct Ray {
    Vector3 origin;
    Vector3 direction;

    Ray() {}
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}

    Vector3 PointAt(float t) const {
        return origin + direction * t;
    }
};

// Ray cast hit result
struct RaycastHit {
    bool hit;
    Vector3 position;
    int vertexIndex;       // Nearest vertex to hit point
    int triangleIndex;     // Which triangle was hit
    float distance;        // Distance along ray

    RaycastHit() : hit(false), vertexIndex(-1), triangleIndex(-1), distance(0.0f) {}
};

// Which side of a correspondence pair
enum class PointSide {
    Target,
    Morph
};

// Point selection state
struct PointSelection {
    int correspondenceIndex;  // -1 for no selection
    PointSide side;

    PointSelection() : correspondenceIndex(-1), side(PointSide::Target) {}

    bool HasSelection() const { return correspondenceIndex >= 0; }
    void Clear() { correspondenceIndex = -1; }
};

struct BoundingBox {
    Vector3 min;
    Vector3 max;

    BoundingBox() : min(0.0f, 0.0f, 0.0f), max(0.0f, 0.0f, 0.0f) {}

    Vector3 Center() const {
        return Vector3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }

    Vector3 Size() const {
        return Vector3(
            max.x - min.x,
            max.y - min.y,
            max.z - min.z
        );
    }
};

struct Face {
    std::vector<unsigned int> vertexIndices;
    std::vector<unsigned int> normalIndices;
    std::vector<unsigned int> uvIndices;
    unsigned int materialIndex;

    Face() : materialIndex(0) {}
};

struct Material {
    std::string name;
    Vector3 diffuseColor;
    Vector3 specularColor;
    float shininess;
    std::string diffuseTexturePath;

    Material() : diffuseColor(0.8f, 0.8f, 0.8f),
                 specularColor(1.0f, 1.0f, 1.0f),
                 shininess(32.0f) {}
};

// Enums
enum class ShadingMode {
    Solid,
    Wireframe,
    SolidWireframe,
    Textured
};

enum class ProjectionMode {
    Perspective,
    OrthographicFront,
    OrthographicRight,
    OrthographicTop
};

enum class Axis {
    X,
    Y,
    Z
};

enum class WorkflowStage {
    Alignment = 0,
    PointReference = 1,
    Morph = 2,
    TouchUp = 3
};

// Transform tool modes (for alignment stage)
enum class TransformMode {
    None,       // No transform active - camera controls work normally
    Move,       // G key - translate target mesh
    Rotate,     // R key - rotate target mesh
    Scale       // S key - scale target mesh
};

// Axis constraint for transform tools
enum class AxisConstraint {
    None,       // Free movement on all axes
    X,          // Constrain to X axis only
    Y,          // Constrain to Y axis only
    Z           // Constrain to Z axis only
};

// Render filter for dual viewport mode
enum class RenderFilter {
    All,        // Render both morph and target meshes
    MorphOnly,  // Render only the morph mesh
    TargetOnly  // Render only the target mesh
};

} // namespace MetaVisage

#endif // TYPES_H
