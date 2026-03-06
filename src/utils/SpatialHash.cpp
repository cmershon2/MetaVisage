#include "utils/SpatialHash.h"
#include <cmath>

namespace MetaVisage {

SpatialHash::SpatialHash() : cellSize_(1.0f), invCellSize_(1.0f), vertices_(nullptr) {}
SpatialHash::~SpatialHash() {}

SpatialHash::CellKey SpatialHash::ToCellKey(const Vector3& pos) const {
    return CellKey{
        static_cast<int>(std::floor(pos.x * invCellSize_)),
        static_cast<int>(std::floor(pos.y * invCellSize_)),
        static_cast<int>(std::floor(pos.z * invCellSize_))
    };
}

void SpatialHash::Build(const std::vector<Vector3>& vertices, float cellSize) {
    grid_.clear();
    vertices_ = &vertices;
    cellSize_ = cellSize;
    invCellSize_ = 1.0f / cellSize;

    for (size_t i = 0; i < vertices.size(); ++i) {
        CellKey key = ToCellKey(vertices[i]);
        grid_[key].push_back(static_cast<int>(i));
    }
}

std::vector<int> SpatialHash::Query(const Vector3& center, float radius) const {
    std::vector<int> result;
    if (!vertices_ || grid_.empty()) return result;

    float radiusSq = radius * radius;

    // Compute cell range to search
    int minX = static_cast<int>(std::floor((center.x - radius) * invCellSize_));
    int maxX = static_cast<int>(std::floor((center.x + radius) * invCellSize_));
    int minY = static_cast<int>(std::floor((center.y - radius) * invCellSize_));
    int maxY = static_cast<int>(std::floor((center.y + radius) * invCellSize_));
    int minZ = static_cast<int>(std::floor((center.z - radius) * invCellSize_));
    int maxZ = static_cast<int>(std::floor((center.z + radius) * invCellSize_));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                auto it = grid_.find(CellKey{x, y, z});
                if (it == grid_.end()) continue;

                for (int idx : it->second) {
                    const Vector3& v = (*vertices_)[idx];
                    Vector3 diff = v - center;
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq <= radiusSq) {
                        result.push_back(idx);
                    }
                }
            }
        }
    }

    return result;
}

} // namespace MetaVisage
