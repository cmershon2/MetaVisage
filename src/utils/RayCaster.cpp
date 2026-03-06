#include "utils/RayCaster.h"
#include "utils/BVH.h"
#include <cmath>
#include <limits>

namespace MetaVisage {

Vector3 RayCaster::TransformPoint(const Vector3& point, const Matrix4x4& matrix) {
    float x = matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8]  * point.z + matrix.m[12];
    float y = matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9]  * point.z + matrix.m[13];
    float z = matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14];
    float w = matrix.m[3] * point.x + matrix.m[7] * point.y + matrix.m[11] * point.z + matrix.m[15];

    if (std::abs(w) > 1e-6f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return Vector3(x, y, z);
}

Vector3 RayCaster::TransformDirection(const Vector3& dir, const Matrix4x4& matrix) {
    float x = matrix.m[0] * dir.x + matrix.m[4] * dir.y + matrix.m[8]  * dir.z;
    float y = matrix.m[1] * dir.x + matrix.m[5] * dir.y + matrix.m[9]  * dir.z;
    float z = matrix.m[2] * dir.x + matrix.m[6] * dir.y + matrix.m[10] * dir.z;
    return Vector3(x, y, z);
}

Ray RayCaster::ScreenToRay(float screenX, float screenY,
                           int viewportWidth, int viewportHeight,
                           const Camera& camera) {
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * screenX / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / viewportHeight); // Flip Y

    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    // Get inverse view-projection matrix
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 projection = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProjection = projection * view;
    Matrix4x4 invVP = viewProjection.Inverse();

    // Unproject near and far points
    Vector3 nearPoint(ndcX, ndcY, -1.0f);
    Vector3 farPoint(ndcX, ndcY, 1.0f);

    Vector3 worldNear = TransformPoint(nearPoint, invVP);
    Vector3 worldFar = TransformPoint(farPoint, invVP);

    Vector3 direction = (worldFar - worldNear).Normalized();

    return Ray(worldNear, direction);
}

bool RayCaster::RayTriangleIntersect(const Ray& ray,
                                      const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                      float& t, float& u, float& v) {
    const float EPSILON = 1e-7f;

    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 h = ray.direction.Cross(edge2);
    float a = edge1.Dot(h);

    if (a > -EPSILON && a < EPSILON) {
        return false; // Ray parallel to triangle
    }

    float f = 1.0f / a;
    Vector3 s = ray.origin - v0;
    u = f * s.Dot(h);

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    Vector3 q = s.Cross(edge1);
    v = f * ray.direction.Dot(q);

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    t = f * edge2.Dot(q);

    return t > EPSILON; // Hit is in front of ray origin
}

RaycastHit RayCaster::RayIntersectMesh(const Ray& ray, const Mesh& mesh,
                                        const Transform& transform) {
    RaycastHit result;
    result.hit = false;
    result.distance = std::numeric_limits<float>::max();

    const auto& vertices = mesh.GetVertices();
    const auto& faces = mesh.GetFaces();

    if (vertices.empty() || faces.empty()) {
        return result;
    }

    // Transform ray into mesh's local space
    Matrix4x4 modelMatrix = transform.GetMatrix();
    Matrix4x4 invModel = modelMatrix.Inverse();

    Vector3 localOrigin = TransformPoint(ray.origin, invModel);
    Vector3 localDir = TransformDirection(ray.direction, invModel).Normalized();
    Ray localRay(localOrigin, localDir);

    int triangleIdx = 0;

    for (size_t faceIdx = 0; faceIdx < faces.size(); ++faceIdx) {
        const Face& face = faces[faceIdx];

        if (face.vertexIndices.size() < 3) continue;

        // Fan triangulation (same as MeshRenderer)
        for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
            unsigned int i0 = face.vertexIndices[0];
            unsigned int i1 = face.vertexIndices[i];
            unsigned int i2 = face.vertexIndices[i + 1];

            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
                triangleIdx++;
                continue;
            }

            const Vector3& v0 = vertices[i0];
            const Vector3& v1 = vertices[i1];
            const Vector3& v2 = vertices[i2];

            float t, u, v;
            if (RayTriangleIntersect(localRay, v0, v1, v2, t, u, v)) {
                if (t < result.distance) {
                    result.hit = true;
                    result.distance = t;
                    result.triangleIndex = triangleIdx;

                    // Hit position in local space
                    Vector3 localHitPos = localRay.PointAt(t);

                    // Transform hit position to world space
                    result.position = TransformPoint(localHitPos, modelMatrix);

                    // Find nearest vertex (in local space, then convert)
                    float d0 = (localHitPos - v0).Length();
                    float d1 = (localHitPos - v1).Length();
                    float d2 = (localHitPos - v2).Length();

                    if (d0 <= d1 && d0 <= d2) {
                        result.vertexIndex = static_cast<int>(i0);
                    } else if (d1 <= d0 && d1 <= d2) {
                        result.vertexIndex = static_cast<int>(i1);
                    } else {
                        result.vertexIndex = static_cast<int>(i2);
                    }
                }
            }
            triangleIdx++;
        }
    }

    return result;
}

int RayCaster::FindNearestVertex(const Vector3& worldPos, const Mesh& mesh,
                                  const Transform& transform) {
    const auto& vertices = mesh.GetVertices();
    if (vertices.empty()) return -1;

    Matrix4x4 modelMatrix = transform.GetMatrix();
    float minDist = std::numeric_limits<float>::max();
    int nearestIdx = -1;

    for (size_t i = 0; i < vertices.size(); ++i) {
        Vector3 vertWorldPos = TransformPoint(vertices[i], modelMatrix);
        float dist = (vertWorldPos - worldPos).Length();
        if (dist < minDist) {
            minDist = dist;
            nearestIdx = static_cast<int>(i);
        }
    }

    return nearestIdx;
}

Vector3 RayCaster::WorldToScreen(const Vector3& worldPos,
                                  int viewportWidth, int viewportHeight,
                                  const Camera& camera) {
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 projection = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProjection = projection * view;

    Vector3 clipPos = TransformPoint(worldPos, viewProjection);

    // Convert from NDC (-1 to 1) to screen coordinates
    float screenX = (clipPos.x + 1.0f) * 0.5f * viewportWidth;
    float screenY = (1.0f - clipPos.y) * 0.5f * viewportHeight; // Flip Y
    float depth = (clipPos.z + 1.0f) * 0.5f;

    return Vector3(screenX, screenY, depth);
}

RaycastHit RayCaster::RayIntersectMeshBVH(const Ray& ray, const Mesh& mesh,
                                           const Transform& transform,
                                           const BVH& bvh) {
    RaycastHit result;
    result.hit = false;
    result.distance = std::numeric_limits<float>::max();

    const auto& vertices = mesh.GetVertices();
    if (vertices.empty()) return result;

    // Transform ray into mesh's local space
    Matrix4x4 modelMatrix = transform.GetMatrix();
    Matrix4x4 invModel = modelMatrix.Inverse();

    Vector3 localOrigin = TransformPoint(ray.origin, invModel);
    Vector3 localDir = TransformDirection(ray.direction, invModel).Normalized();
    Ray localRay(localOrigin, localDir);

    float t;
    int triIndex;
    unsigned int i0, i1, i2;

    if (bvh.RayIntersect(localRay, vertices, t, triIndex, i0, i1, i2)) {
        result.hit = true;
        result.distance = t;
        result.triangleIndex = triIndex;

        Vector3 localHitPos = localRay.PointAt(t);
        result.position = TransformPoint(localHitPos, modelMatrix);

        // Find nearest vertex
        float d0 = (localHitPos - vertices[i0]).Length();
        float d1 = (localHitPos - vertices[i1]).Length();
        float d2 = (localHitPos - vertices[i2]).Length();

        if (d0 <= d1 && d0 <= d2) result.vertexIndex = static_cast<int>(i0);
        else if (d1 <= d0 && d1 <= d2) result.vertexIndex = static_cast<int>(i1);
        else result.vertexIndex = static_cast<int>(i2);
    }

    return result;
}

} // namespace MetaVisage
