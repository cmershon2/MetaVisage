#include "core/Mesh.h"
#include <algorithm>
#include <cmath>

namespace MetaVisage {

Mesh::Mesh() : name_("Untitled"), filepath_("") {
}

Mesh::~Mesh() {
}

bool Mesh::Load(const QString& filepath) {
    // TODO: Implement Assimp loading in Sprint 2
    filepath_ = filepath;
    return false;
}

bool Mesh::Save(const QString& filepath) {
    // TODO: Implement mesh saving in Sprint 10
    return false;
}

void Mesh::SetVertices(const std::vector<Vector3>& vertices) {
    vertices_ = vertices;
    CalculateBounds();
}

void Mesh::SetNormals(const std::vector<Vector3>& normals) {
    normals_ = normals;
}

void Mesh::SetUVs(const std::vector<Vector2>& uvs) {
    uvs_ = uvs;
}

void Mesh::SetFaces(const std::vector<Face>& faces) {
    faces_ = faces;
}

void Mesh::SetMaterials(const std::vector<Material>& materials) {
    materials_ = materials;
}

void Mesh::CalculateNormals() {
    // Clear existing normals
    normals_.clear();
    normals_.resize(vertices_.size(), Vector3(0.0f, 0.0f, 0.0f));

    // Calculate face normals and accumulate
    for (const auto& face : faces_) {
        if (face.vertexIndices.size() < 3) continue;

        // Get first three vertices to calculate face normal
        unsigned int i0 = face.vertexIndices[0];
        unsigned int i1 = face.vertexIndices[1];
        unsigned int i2 = face.vertexIndices[2];

        if (i0 >= vertices_.size() || i1 >= vertices_.size() || i2 >= vertices_.size()) {
            continue;
        }

        Vector3 v0 = vertices_[i0];
        Vector3 v1 = vertices_[i1];
        Vector3 v2 = vertices_[i2];

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 faceNormal = edge1.Cross(edge2).Normalized();

        // Accumulate to vertex normals
        for (unsigned int idx : face.vertexIndices) {
            if (idx < normals_.size()) {
                normals_[idx] = normals_[idx] + faceNormal;
            }
        }
    }

    // Normalize all vertex normals
    for (auto& normal : normals_) {
        normal = normal.Normalized();
    }
}

void Mesh::CalculateBounds() {
    if (vertices_.empty()) {
        bounds_.min = Vector3(0.0f, 0.0f, 0.0f);
        bounds_.max = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    bounds_.min = vertices_[0];
    bounds_.max = vertices_[0];

    for (const auto& vertex : vertices_) {
        bounds_.min.x = std::min(bounds_.min.x, vertex.x);
        bounds_.min.y = std::min(bounds_.min.y, vertex.y);
        bounds_.min.z = std::min(bounds_.min.z, vertex.z);

        bounds_.max.x = std::max(bounds_.max.x, vertex.x);
        bounds_.max.y = std::max(bounds_.max.y, vertex.y);
        bounds_.max.z = std::max(bounds_.max.z, vertex.z);
    }
}

bool Mesh::Validate() const {
    // Check if we have vertices
    if (vertices_.empty()) {
        return false;
    }

    // Check if we have faces
    if (faces_.empty()) {
        return false;
    }

    // Check if face indices are valid
    for (const auto& face : faces_) {
        for (unsigned int idx : face.vertexIndices) {
            if (idx >= vertices_.size()) {
                return false;
            }
        }
    }

    return true;
}

void Mesh::Clear() {
    vertices_.clear();
    normals_.clear();
    uvs_.clear();
    faces_.clear();
    materials_.clear();
    bounds_ = BoundingBox();
}

size_t Mesh::GetTriangleCount() const {
    size_t count = 0;
    for (const auto& face : faces_) {
        if (face.vertexIndices.size() >= 3) {
            count += face.vertexIndices.size() - 2;
        }
    }
    return count;
}

} // namespace MetaVisage
