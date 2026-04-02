#include "utils/BVH.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace MetaVisage {

BVH::BVH() {}
BVH::~BVH() {}

void BVH::Build(const std::vector<Vector3>& vertices, const std::vector<Face>& faces) {
    root_.reset();

    // Collect all triangles via fan triangulation
    std::vector<BVHTriangle> triangles;
    int triIdx = 0;

    for (const auto& face : faces) {
        if (face.vertexIndices.size() < 3) continue;
        for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
            BVHTriangle tri;
            tri.i0 = face.vertexIndices[0];
            tri.i1 = face.vertexIndices[i];
            tri.i2 = face.vertexIndices[i + 1];
            tri.triangleIndex = triIdx++;

            if (tri.i0 < vertices.size() && tri.i1 < vertices.size() && tri.i2 < vertices.size()) {
                triangles.push_back(tri);
            }
        }
    }

    if (triangles.empty()) return;

    root_ = BuildRecursive(triangles, vertices, 0);
}

std::unique_ptr<BVHNode> BVH::BuildRecursive(std::vector<BVHTriangle>& triangles,
                                               const std::vector<Vector3>& vertices,
                                               int depth) {
    auto node = std::make_unique<BVHNode>();
    node->bounds = ComputeBounds(triangles, vertices);

    if (static_cast<int>(triangles.size()) <= MAX_LEAF_TRIANGLES || depth >= MAX_DEPTH) {
        node->triangles = std::move(triangles);
        return node;
    }

    // Split on longest axis at midpoint
    Vector3 size = node->bounds.Size();
    int axis = 0;
    if (size.y > size.x) axis = 1;
    if (size.z > (axis == 0 ? size.x : size.y)) axis = 2;

    float mid;
    if (axis == 0) mid = (node->bounds.min.x + node->bounds.max.x) * 0.5f;
    else if (axis == 1) mid = (node->bounds.min.y + node->bounds.max.y) * 0.5f;
    else mid = (node->bounds.min.z + node->bounds.max.z) * 0.5f;

    // Partition triangles by centroid
    std::vector<BVHTriangle> leftTris, rightTris;
    for (auto& tri : triangles) {
        Vector3 centroid;
        centroid.x = (vertices[tri.i0].x + vertices[tri.i1].x + vertices[tri.i2].x) / 3.0f;
        centroid.y = (vertices[tri.i0].y + vertices[tri.i1].y + vertices[tri.i2].y) / 3.0f;
        centroid.z = (vertices[tri.i0].z + vertices[tri.i1].z + vertices[tri.i2].z) / 3.0f;

        float val = (axis == 0) ? centroid.x : (axis == 1) ? centroid.y : centroid.z;
        if (val < mid) {
            leftTris.push_back(tri);
        } else {
            rightTris.push_back(tri);
        }
    }

    // If partitioning failed, split evenly
    if (leftTris.empty() || rightTris.empty()) {
        size_t half = triangles.size() / 2;
        leftTris.assign(triangles.begin(), triangles.begin() + half);
        rightTris.assign(triangles.begin() + half, triangles.end());
    }

    node->left = BuildRecursive(leftTris, vertices, depth + 1);
    node->right = BuildRecursive(rightTris, vertices, depth + 1);

    return node;
}

BoundingBox BVH::ComputeBounds(const std::vector<BVHTriangle>& triangles,
                                const std::vector<Vector3>& vertices) const {
    BoundingBox box;
    box.min = Vector3(std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max());
    box.max = Vector3(-std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max());

    for (const auto& tri : triangles) {
        for (unsigned int idx : {tri.i0, tri.i1, tri.i2}) {
            const Vector3& v = vertices[idx];
            box.min.x = std::min(box.min.x, v.x);
            box.min.y = std::min(box.min.y, v.y);
            box.min.z = std::min(box.min.z, v.z);
            box.max.x = std::max(box.max.x, v.x);
            box.max.y = std::max(box.max.y, v.y);
            box.max.z = std::max(box.max.z, v.z);
        }
    }

    return box;
}

bool BVH::RayAABBIntersect(const Ray& ray, const BoundingBox& box,
                            float tMin, float tMax) const {
    for (int i = 0; i < 3; ++i) {
        float origin = (i == 0) ? ray.origin.x : (i == 1) ? ray.origin.y : ray.origin.z;
        float dir = (i == 0) ? ray.direction.x : (i == 1) ? ray.direction.y : ray.direction.z;
        float bmin = (i == 0) ? box.min.x : (i == 1) ? box.min.y : box.min.z;
        float bmax = (i == 0) ? box.max.x : (i == 1) ? box.max.y : box.max.z;

        if (std::abs(dir) < 1e-8f) {
            if (origin < bmin || origin > bmax) return false;
        } else {
            float invD = 1.0f / dir;
            float t0 = (bmin - origin) * invD;
            float t1 = (bmax - origin) * invD;
            if (invD < 0.0f) std::swap(t0, t1);
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMax < tMin) return false;
        }
    }
    return true;
}

bool BVH::RayTriangleIntersect(const Ray& ray,
                                const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                float& t, float& u, float& v) {
    const float EPSILON = 1e-7f;

    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 h = ray.direction.Cross(edge2);
    float a = edge1.Dot(h);

    if (a > -EPSILON && a < EPSILON) return false;

    float f = 1.0f / a;
    Vector3 s = ray.origin - v0;
    u = f * s.Dot(h);
    if (u < 0.0f || u > 1.0f) return false;

    Vector3 q = s.Cross(edge1);
    v = f * ray.direction.Dot(q);
    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * edge2.Dot(q);
    return t > EPSILON;
}

void BVH::RayIntersectNode(const BVHNode* node, const Ray& ray,
                            const std::vector<Vector3>& vertices,
                            float& closestT, int& closestTriIndex,
                            unsigned int& closestI0, unsigned int& closestI1,
                            unsigned int& closestI2) const {
    if (!node) return;

    if (!RayAABBIntersect(ray, node->bounds, 0.0f, closestT)) return;

    if (node->IsLeaf()) {
        for (const auto& tri : node->triangles) {
            float t, u, v;
            if (RayTriangleIntersect(ray, vertices[tri.i0], vertices[tri.i1],
                                     vertices[tri.i2], t, u, v)) {
                if (t < closestT) {
                    closestT = t;
                    closestTriIndex = tri.triangleIndex;
                    closestI0 = tri.i0;
                    closestI1 = tri.i1;
                    closestI2 = tri.i2;
                }
            }
        }
        return;
    }

    RayIntersectNode(node->left.get(), ray, vertices, closestT, closestTriIndex,
                     closestI0, closestI1, closestI2);
    RayIntersectNode(node->right.get(), ray, vertices, closestT, closestTriIndex,
                     closestI0, closestI1, closestI2);
}

bool BVH::RayIntersect(const Ray& ray, const std::vector<Vector3>& vertices,
                        float& outT, int& outTriangleIndex,
                        unsigned int& outI0, unsigned int& outI1, unsigned int& outI2) const {
    if (!root_) return false;

    outT = std::numeric_limits<float>::max();
    outTriangleIndex = -1;

    RayIntersectNode(root_.get(), ray, vertices, outT, outTriangleIndex,
                     outI0, outI1, outI2);

    return outTriangleIndex >= 0;
}

Vector3 BVH::ClosestPointOnTriangle(const Vector3& p,
                                     const Vector3& a, const Vector3& b, const Vector3& c) {
    // Voronoi region method (Real-Time Collision Detection, Ericson)
    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 ap = p - a;

    float d1 = ab.Dot(ap);
    float d2 = ac.Dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a; // Vertex A region

    Vector3 bp = p - b;
    float d3 = ab.Dot(bp);
    float d4 = ac.Dot(bp);
    if (d3 >= 0.0f && d4 <= d3) return b; // Vertex B region

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return a + ab * v; // Edge AB region
    }

    Vector3 cp = p - c;
    float d5 = ab.Dot(cp);
    float d6 = ac.Dot(cp);
    if (d6 >= 0.0f && d5 <= d6) return c; // Vertex C region

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return a + ac * w; // Edge AC region
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w; // Edge BC region
    }

    // Inside triangle
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

float BVH::PointAABBDistanceSq(const Vector3& point, const BoundingBox& box) {
    float distSq = 0.0f;

    // X axis
    if (point.x < box.min.x) { float d = box.min.x - point.x; distSq += d * d; }
    else if (point.x > box.max.x) { float d = point.x - box.max.x; distSq += d * d; }

    // Y axis
    if (point.y < box.min.y) { float d = box.min.y - point.y; distSq += d * d; }
    else if (point.y > box.max.y) { float d = point.y - box.max.y; distSq += d * d; }

    // Z axis
    if (point.z < box.min.z) { float d = box.min.z - point.z; distSq += d * d; }
    else if (point.z > box.max.z) { float d = point.z - box.max.z; distSq += d * d; }

    return distSq;
}

void BVH::FindClosestPointNode(const BVHNode* node, const Vector3& queryPoint,
                                const std::vector<Vector3>& vertices,
                                BVHClosestPointResult& best) const {
    if (!node) return;

    // Prune: if minimum distance to this AABB is worse than current best, skip
    float aabbDistSq = PointAABBDistanceSq(queryPoint, node->bounds);
    if (aabbDistSq >= best.distanceSq) return;

    if (node->IsLeaf()) {
        for (const auto& tri : node->triangles) {
            const Vector3& v0 = vertices[tri.i0];
            const Vector3& v1 = vertices[tri.i1];
            const Vector3& v2 = vertices[tri.i2];

            Vector3 closest = ClosestPointOnTriangle(queryPoint, v0, v1, v2);
            Vector3 diff = queryPoint - closest;
            float distSq = diff.Dot(diff);

            if (distSq < best.distanceSq) {
                best.point = closest;
                best.distanceSq = distSq;
                best.triangleIndex = tri.triangleIndex;
                best.i0 = tri.i0;
                best.i1 = tri.i1;
                best.i2 = tri.i2;
                best.found = true;

                // Compute face normal
                Vector3 edge1 = v1 - v0;
                Vector3 edge2 = v2 - v0;
                best.normal = edge1.Cross(edge2).Normalized();
            }
        }
        return;
    }

    // Visit the closer child first for better pruning
    float leftDistSq = node->left ? PointAABBDistanceSq(queryPoint, node->left->bounds)
                                  : std::numeric_limits<float>::max();
    float rightDistSq = node->right ? PointAABBDistanceSq(queryPoint, node->right->bounds)
                                    : std::numeric_limits<float>::max();

    if (leftDistSq <= rightDistSq) {
        FindClosestPointNode(node->left.get(), queryPoint, vertices, best);
        FindClosestPointNode(node->right.get(), queryPoint, vertices, best);
    } else {
        FindClosestPointNode(node->right.get(), queryPoint, vertices, best);
        FindClosestPointNode(node->left.get(), queryPoint, vertices, best);
    }
}

BVHClosestPointResult BVH::FindClosestPoint(const Vector3& queryPoint,
                                              const std::vector<Vector3>& vertices) const {
    BVHClosestPointResult result;
    if (!root_) return result;
    FindClosestPointNode(root_.get(), queryPoint, vertices, result);
    return result;
}

} // namespace MetaVisage
