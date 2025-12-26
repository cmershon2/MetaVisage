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
};

struct Matrix4x4 {
    float m[16];

    Matrix4x4() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4x4 Identity() { return Matrix4x4(); }
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

} // namespace MetaVisage

#endif // TYPES_H
