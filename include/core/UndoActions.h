#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include "core/UndoableAction.h"
#include "core/Types.h"
#include "core/Transform.h"
#include "core/Project.h"
#include "core/Mesh.h"
#include "sculpting/BrushStroke.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace MetaVisage {

// Undo action for alignment transforms (move/rotate/scale of target mesh)
class TransformUndoAction : public UndoableAction {
public:
    TransformUndoAction(Project* project, const Transform& before, const Transform& after)
        : project_(project), before_(before), after_(after) {}

    void Undo() override {
        project_->GetTargetMesh().transform = before_;
        project_->GetAlignmentData().targetMeshTransform = before_;
    }

    void Redo() override {
        project_->GetTargetMesh().transform = after_;
        project_->GetAlignmentData().targetMeshTransform = after_;
    }

    QString Description() const override { return "Transform"; }

private:
    Project* project_;
    Transform before_;
    Transform after_;
};

// Undo action for point placement/deletion (snapshot-based)
class PointPlacementUndoAction : public UndoableAction {
public:
    PointPlacementUndoAction(Project* project,
                              const std::vector<PointCorrespondence>& before,
                              const std::vector<PointCorrespondence>& after,
                              const QString& description = "Point Placement")
        : project_(project), before_(before), after_(after), description_(description) {}

    void Undo() override {
        project_->GetPointReferenceData().correspondences = before_;
    }

    void Redo() override {
        project_->GetPointReferenceData().correspondences = after_;
    }

    QString Description() const override { return description_; }

private:
    Project* project_;
    std::vector<PointCorrespondence> before_;
    std::vector<PointCorrespondence> after_;
    QString description_;
};

// Undo action for morph processing
class MorphUndoAction : public UndoableAction {
public:
    struct MorphState {
        std::shared_ptr<Mesh> deformedMesh;
        bool isProcessed;
        bool isAccepted;
        std::vector<float> displacementMagnitudes;
        float maxDisplacement;
        float avgDisplacement;
    };

    MorphUndoAction(Project* project, const MorphState& before, const MorphState& after)
        : project_(project), before_(before), after_(after) {}

    void Undo() override { ApplyState(before_); }
    void Redo() override { ApplyState(after_); }
    QString Description() const override { return "Morph"; }

private:
    void ApplyState(const MorphState& state) {
        MorphData& morph = project_->GetMorphData();
        morph.deformedMorphMesh = state.deformedMesh;
        morph.isProcessed = state.isProcessed;
        morph.isAccepted = state.isAccepted;
        morph.displacementMagnitudes = state.displacementMagnitudes;
        morph.maxDisplacement = state.maxDisplacement;
        morph.avgDisplacement = state.avgDisplacement;
    }

    Project* project_;
    MorphState before_;
    MorphState after_;
};

// Undo action for sculpt brush strokes
class SculptStrokeUndoAction : public UndoableAction {
public:
    SculptStrokeUndoAction(Mesh* mesh, const BrushStroke& stroke,
                            const std::map<int, Vector3>& afterPositions)
        : mesh_(mesh), stroke_(stroke), afterPositions_(afterPositions) {}

    void Undo() override {
        if (!mesh_) return;
        auto& vertices = mesh_->GetVerticesMutable();
        stroke_.Undo(vertices);
        mesh_->CalculateNormals();
    }

    void Redo() override {
        if (!mesh_) return;
        auto& vertices = mesh_->GetVerticesMutable();
        for (const auto& pair : afterPositions_) {
            if (pair.first >= 0 && pair.first < static_cast<int>(vertices.size())) {
                vertices[pair.first] = pair.second;
            }
        }
        mesh_->CalculateNormals();
    }

    QString Description() const override { return "Sculpt Stroke"; }

private:
    Mesh* mesh_;
    BrushStroke stroke_;
    std::map<int, Vector3> afterPositions_;
};

} // namespace MetaVisage

#endif // UNDOACTIONS_H
