#ifndef MESH_H
#define MESH_H

#include "core/Types.h"
#include <QString>
#include <memory>

namespace MetaVisage {

class BVH;
class SpatialHash;

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Load and save
    bool Load(const QString& filepath);
    bool Save(const QString& filepath);

    // Accessors
    const std::vector<Vector3>& GetVertices() const { return vertices_; }
    const std::vector<Vector3>& GetNormals() const { return normals_; }
    const std::vector<Vector2>& GetUVs() const { return uvs_; }
    const std::vector<Face>& GetFaces() const { return faces_; }
    const std::vector<Material>& GetMaterials() const { return materials_; }
    const BoundingBox& GetBounds() const { return bounds_; }
    const QString& GetName() const { return name_; }
    const QString& GetFilePath() const { return filepath_; }

    // Modifiers
    void SetVertices(const std::vector<Vector3>& vertices);
    void SetNormals(const std::vector<Vector3>& normals);

    // Mutable access for sculpting (modifies vertices in-place)
    std::vector<Vector3>& GetVerticesMutable() { return vertices_; }
    void SetUVs(const std::vector<Vector2>& uvs);
    void SetFaces(const std::vector<Face>& faces);
    void SetMaterials(const std::vector<Material>& materials);
    void SetName(const QString& name) { name_ = name; }

    // Mesh operations
    void CalculateNormals();
    void CalculateBounds();
    bool Validate() const;
    void Clear();

    // Mesh info
    size_t GetVertexCount() const { return vertices_.size(); }
    size_t GetFaceCount() const { return faces_.size(); }
    size_t GetUVCount() const { return uvs_.size(); }
    bool HasSeparateUVIndices() const { return !faces_.empty() && !faces_[0].uvIndices.empty(); }
    size_t GetTriangleCount() const;

    // MetaHuman-compatible export data (populated during UV-seam merge on import)
    const std::vector<int>& GetAssimpToMergedMap() const { return assimpToMergedMap_; }
    const std::vector<Vector3>& GetOriginalAssimpNormals() const { return originalAssimpNormals_; }
    size_t GetOriginalAssimpVertexCount() const { return originalAssimpVertexCount_; }
    bool HasAssimpMapping() const { return !assimpToMergedMap_.empty() && originalAssimpVertexCount_ > 0; }
    void SetAssimpMapping(const std::vector<int>& map, const std::vector<Vector3>& normals, size_t originalCount);

    // Acceleration structures (lazy-built, mutable for const access)
    BVH* GetBVH() const;
    SpatialHash* GetSpatialHash(float cellSize = 1.0f) const;
    void InvalidateAccelerationStructures();

private:
    QString name_;
    QString filepath_;

    std::vector<Vector3> vertices_;
    std::vector<Vector3> normals_;
    std::vector<Vector2> uvs_;
    std::vector<Face> faces_;
    std::vector<Material> materials_;

    BoundingBox bounds_;

    // MetaHuman-compatible export: maps original Assimp vertex index to merged vertex index
    std::vector<int> assimpToMergedMap_;
    std::vector<Vector3> originalAssimpNormals_;  // Pre-merge normals (N entries, one per split vertex)
    size_t originalAssimpVertexCount_ = 0;

    // Lazy acceleration structures
    mutable std::unique_ptr<BVH> bvh_;
    mutable std::unique_ptr<SpatialHash> spatialHash_;
    mutable float spatialHashCellSize_ = 0.0f;
};

} // namespace MetaVisage

#endif // MESH_H
