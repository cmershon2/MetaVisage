#include "sculpting/SmoothBrush.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace MetaVisage {

SmoothBrush::SmoothBrush()
    : adjacencyBuilt_(false) {}

SmoothBrush::~SmoothBrush() {}

void SmoothBrush::BuildAdjacency(const Mesh& mesh) {
    const auto& faces = mesh.GetFaces();
    size_t vertexCount = mesh.GetVertexCount();

    adjacency_.clear();
    adjacency_.resize(vertexCount);

    // Use sets to avoid duplicate neighbors
    std::vector<std::set<int>> adjSets(vertexCount);

    for (const auto& face : faces) {
        const auto& indices = face.vertexIndices;
        // Connect each vertex in the face to every other vertex in the same face
        for (size_t i = 0; i < indices.size(); ++i) {
            for (size_t j = i + 1; j < indices.size(); ++j) {
                int a = static_cast<int>(indices[i]);
                int b = static_cast<int>(indices[j]);
                if (a >= 0 && a < static_cast<int>(vertexCount) &&
                    b >= 0 && b < static_cast<int>(vertexCount)) {
                    adjSets[a].insert(b);
                    adjSets[b].insert(a);
                }
            }
        }
    }

    // Convert sets to vectors for fast iteration
    for (size_t i = 0; i < vertexCount; ++i) {
        adjacency_[i].assign(adjSets[i].begin(), adjSets[i].end());
    }

    adjacencyBuilt_ = true;
}

bool SmoothBrush::Apply(Mesh& mesh, const Transform& transform,
                         const Vector3& worldCenter, const Vector3& /*worldNormal*/,
                         const Vector3& /*mouseDelta*/, float deltaTime) {
    if (!adjacencyBuilt_) {
        BuildAdjacency(mesh);
    }

    auto affected = GetAffectedVertices(mesh, transform, worldCenter, settings_.radius);
    if (affected.empty()) return false;

    auto& vertices = mesh.GetVerticesMutable();
    float strength = settings_.strength * deltaTime * 10.0f; // Scale for responsiveness

    // For each affected vertex, blend toward the average of its neighbors
    for (const auto& av : affected) {
        int idx = av.index;
        if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;
        if (idx >= static_cast<int>(adjacency_.size())) continue;

        const auto& neighbors = adjacency_[idx];
        if (neighbors.empty()) continue;

        // Compute average neighbor position
        Vector3 avg(0.0f, 0.0f, 0.0f);
        int count = 0;
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(vertices.size())) {
                avg = avg + vertices[neighborIdx];
                count++;
            }
        }
        if (count == 0) continue;

        avg = avg * (1.0f / count);

        // Blend toward average based on weight and strength
        float t = std::min(av.weight * strength, 1.0f);
        vertices[idx].x += (avg.x - vertices[idx].x) * t;
        vertices[idx].y += (avg.y - vertices[idx].y) * t;
        vertices[idx].z += (avg.z - vertices[idx].z) * t;
    }

    return true;
}

} // namespace MetaVisage
