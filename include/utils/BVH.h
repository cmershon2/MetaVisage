#ifndef BVH_H
#define BVH_H

#include "core/Types.h"
#include <vector>
#include <memory>

namespace MetaVisage {

// Triangle data stored in BVH leaf nodes
struct BVHTriangle {
    unsigned int i0, i1, i2;  // Vertex indices
    int triangleIndex;         // Global triangle index for hit reporting
};

// BVH node - either internal (has children) or leaf (has triangles)
struct BVHNode {
    BoundingBox bounds;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    std::vector<BVHTriangle> triangles; // Non-empty only for leaf nodes

    bool IsLeaf() const { return !left && !right; }
};

// Result of a closest-point-on-surface query
struct BVHClosestPointResult {
    Vector3 point;        // Closest point on surface
    Vector3 normal;       // Surface normal at closest point (face normal)
    float distanceSq;     // Squared distance from query to closest point
    int triangleIndex;    // Which triangle contains the closest point
    bool found;

    BVHClosestPointResult() : distanceSq(std::numeric_limits<float>::max()),
                               triangleIndex(-1), found(false) {}
};

class BVH {
public:
    BVH();
    ~BVH();

    // Build BVH from mesh triangles (call once after mesh load)
    void Build(const std::vector<Vector3>& vertices, const std::vector<Face>& faces);

    // Ray intersection query - returns closest hit
    // Ray should be in mesh local space
    bool RayIntersect(const Ray& ray, const std::vector<Vector3>& vertices,
                      float& outT, int& outTriangleIndex,
                      unsigned int& outI0, unsigned int& outI1, unsigned int& outI2) const;

    // Closest point on surface query - returns nearest surface point to query point
    BVHClosestPointResult FindClosestPoint(const Vector3& queryPoint,
                                            const std::vector<Vector3>& vertices) const;

    bool IsBuilt() const { return root_ != nullptr; }

    // Closest point on a triangle to a query point (Voronoi region method)
    static Vector3 ClosestPointOnTriangle(const Vector3& p,
                                           const Vector3& a, const Vector3& b, const Vector3& c);

private:
    static const int MAX_LEAF_TRIANGLES = 4;
    static const int MAX_DEPTH = 32;

    std::unique_ptr<BVHNode> root_;

    std::unique_ptr<BVHNode> BuildRecursive(std::vector<BVHTriangle>& triangles,
                                             const std::vector<Vector3>& vertices,
                                             int depth);

    BoundingBox ComputeBounds(const std::vector<BVHTriangle>& triangles,
                              const std::vector<Vector3>& vertices) const;

    bool RayAABBIntersect(const Ray& ray, const BoundingBox& box,
                          float tMin, float tMax) const;

    void RayIntersectNode(const BVHNode* node, const Ray& ray,
                          const std::vector<Vector3>& vertices,
                          float& closestT, int& closestTriIndex,
                          unsigned int& closestI0, unsigned int& closestI1,
                          unsigned int& closestI2) const;

    // Recursive closest-point BVH traversal
    void FindClosestPointNode(const BVHNode* node, const Vector3& queryPoint,
                               const std::vector<Vector3>& vertices,
                               BVHClosestPointResult& best) const;

    // Squared distance from point to AABB (for BVH pruning)
    static float PointAABBDistanceSq(const Vector3& point, const BoundingBox& box);

    static bool RayTriangleIntersect(const Ray& ray,
                                      const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                      float& t, float& u, float& v);
};

} // namespace MetaVisage

#endif // BVH_H
