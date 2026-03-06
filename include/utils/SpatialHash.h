#ifndef SPATIALHASH_H
#define SPATIALHASH_H

#include "core/Types.h"
#include <vector>
#include <unordered_map>

namespace MetaVisage {

class SpatialHash {
public:
    SpatialHash();
    ~SpatialHash();

    // Build spatial hash from vertex positions
    // cellSize should be roughly 2x the typical brush radius
    void Build(const std::vector<Vector3>& vertices, float cellSize);

    // Query all vertex indices within radius of a center point
    // Returns indices into the original vertex array
    std::vector<int> Query(const Vector3& center, float radius) const;

    bool IsBuilt() const { return !grid_.empty(); }
    float GetCellSize() const { return cellSize_; }

private:
    struct CellKey {
        int x, y, z;

        bool operator==(const CellKey& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& key) const {
            // Large primes for spatial hash
            size_t h = static_cast<size_t>(key.x) * 73856093u;
            h ^= static_cast<size_t>(key.y) * 19349663u;
            h ^= static_cast<size_t>(key.z) * 83492791u;
            return h;
        }
    };

    CellKey ToCellKey(const Vector3& pos) const;

    float cellSize_;
    float invCellSize_;
    const std::vector<Vector3>* vertices_; // Non-owning pointer to vertex data
    std::unordered_map<CellKey, std::vector<int>, CellKeyHash> grid_;
};

} // namespace MetaVisage

#endif // SPATIALHASH_H
