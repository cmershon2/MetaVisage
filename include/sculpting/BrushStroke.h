#ifndef BRUSHSTROKE_H
#define BRUSHSTROKE_H

#include "core/Types.h"
#include <vector>
#include <map>

namespace MetaVisage {

// Records vertex modifications during a brush stroke for undo support
class BrushStroke {
public:
    BrushStroke() {}

    // Record a vertex's original position before modification
    // Only records the first time a vertex is touched in this stroke
    void RecordVertex(int index, const Vector3& originalPosition) {
        if (originalPositions_.find(index) == originalPositions_.end()) {
            originalPositions_[index] = originalPosition;
        }
    }

    // Undo: restore vertices to their positions before this stroke
    void Undo(std::vector<Vector3>& vertices) const {
        for (const auto& pair : originalPositions_) {
            if (pair.first >= 0 && pair.first < static_cast<int>(vertices.size())) {
                vertices[pair.first] = pair.second;
            }
        }
    }

    bool IsEmpty() const { return originalPositions_.empty(); }
    size_t GetModifiedCount() const { return originalPositions_.size(); }

private:
    std::map<int, Vector3> originalPositions_;
};

} // namespace MetaVisage

#endif // BRUSHSTROKE_H
