#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "core/Types.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "core/Camera.h"

namespace MetaVisage {

class BVH;

class RayCaster {
public:
    // Create a ray from screen coordinates using camera matrices
    // screenX, screenY: pixel coordinates (origin top-left)
    // viewportWidth, viewportHeight: viewport dimensions in pixels
    static Ray ScreenToRay(float screenX, float screenY,
                           int viewportWidth, int viewportHeight,
                           const Camera& camera);

    // Test ray against a mesh (considering its transform)
    // Returns the closest hit point
    static RaycastHit RayIntersectMesh(const Ray& ray, const Mesh& mesh,
                                       const Transform& transform);

    // BVH-accelerated ray-mesh intersection (O(log n) instead of O(n))
    static RaycastHit RayIntersectMeshBVH(const Ray& ray, const Mesh& mesh,
                                           const Transform& transform,
                                           const BVH& bvh);

    // Find the nearest vertex index to a world-space position on a mesh
    static int FindNearestVertex(const Vector3& worldPos, const Mesh& mesh,
                                 const Transform& transform);

    // Project a 3D world position to 2D screen coordinates
    // Returns screen coordinates (origin top-left), z is depth (0-1)
    static Vector3 WorldToScreen(const Vector3& worldPos,
                                 int viewportWidth, int viewportHeight,
                                 const Camera& camera);

private:
    // Moller-Trumbore ray-triangle intersection
    // Returns true if hit, sets t (distance along ray), u, v (barycentric coords)
    static bool RayTriangleIntersect(const Ray& ray,
                                     const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                     float& t, float& u, float& v);

    // Transform a point by a Matrix4x4
    static Vector3 TransformPoint(const Vector3& point, const Matrix4x4& matrix);

    // Transform a direction by a Matrix4x4 (no translation)
    static Vector3 TransformDirection(const Vector3& dir, const Matrix4x4& matrix);
};

} // namespace MetaVisage

#endif // RAYCASTER_H
