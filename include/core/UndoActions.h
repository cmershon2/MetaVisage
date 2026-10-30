#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include "core/UndoableAction.h"
#include "core/Types.h"
#include "core/Transform.h"
#include "core/Project.h"
#include "core/Mesh.h"
#include "core/WrapPreset.h"
#include "sculpting/BrushStroke.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MetaVisage {

// Undo action for alignment transforms (move/rotate/scale of target mesh)
class TransformUndoAction : public UndoableAction {
public:
    TransformUndoAction(Project* project, const Transform& before, const Transform& after)
        : project_(project), before_(before), after_(after),
          originatingStage_(project->GetCurrentStage()) {}

    void Undo() override {
        project_->GetTargetMesh().transform = before_;
        project_->GetAlignmentData().targetMeshTransform = before_;
        project_->SetCurrentStage(originatingStage_);
    }

    void Redo() override {
        project_->GetTargetMesh().transform = after_;
        project_->GetAlignmentData().targetMeshTransform = after_;
        project_->SetCurrentStage(originatingStage_);
    }

    std::string Description() const override { return "Transform"; }

private:
    Project* project_;
    Transform before_;
    Transform after_;
    WorkflowStage originatingStage_;
};

// Undo action for point placement/deletion (snapshot-based)
class PointPlacementUndoAction : public UndoableAction {
public:
    PointPlacementUndoAction(Project* project,
                              const std::vector<PointCorrespondence>& before,
                              const std::vector<PointCorrespondence>& after,
                              const std::string& description = "Point Placement")
        : project_(project), before_(before), after_(after), description_(description),
          originatingStage_(project->GetCurrentStage()) {}

    void Undo() override {
        project_->GetPointReferenceData().correspondences = before_;
        project_->SetCurrentStage(originatingStage_);
    }

    void Redo() override {
        project_->GetPointReferenceData().correspondences = after_;
        project_->SetCurrentStage(originatingStage_);
    }

    std::string Description() const override { return description_; }

private:
    Project* project_;
    std::vector<PointCorrespondence> before_;
    std::vector<PointCorrespondence> after_;
    std::string description_;
    WorkflowStage originatingStage_;
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
        : project_(project), before_(before), after_(after),
          originatingStage_(project->GetCurrentStage()) {}

    void Undo() override { ApplyState(before_); }
    void Redo() override { ApplyState(after_); }
    std::string Description() const override { return "Morph"; }

private:
    void ApplyState(const MorphState& state) {
        MorphData& morph = project_->GetMorphData();
        morph.deformedMorphMesh = state.deformedMesh;
        morph.isProcessed = state.isProcessed;
        morph.isAccepted = state.isAccepted;
        morph.displacementMagnitudes = state.displacementMagnitudes;
        morph.maxDisplacement = state.maxDisplacement;
        morph.avgDisplacement = state.avgDisplacement;
        project_->SetCurrentStage(originatingStage_);
    }

    Project* project_;
    MorphState before_;
    MorphState after_;
    WorkflowStage originatingStage_;
};

// Sprint 11 — undo for replacing the source mesh (picking a Source Profile
// or importing a mesh). Everything that depends on the source's vertex
// indices changes together: the mesh reference, the profile id, the vertex
// mask, the point pairs, the morph result, and the wrap parameters a
// profile's default preset may have set. Meshes are held by shared_ptr, so
// no geometry is copied.
class SourceSwapUndoAction : public UndoableAction {
public:
    struct SourceState {
        MeshReference morphMesh;
        std::string sourceProfileId;
        std::vector<bool> vertexMask;
        std::vector<PointCorrespondence> correspondences;
        WrapParams wrapParams;
        std::shared_ptr<Mesh> originalMorphMesh;
        std::shared_ptr<Mesh> deformedMorphMesh;
        bool isProcessed = false;
        bool isAccepted = false;
        std::vector<float> displacementMagnitudes;
        float maxDisplacement = 0.0f;
        float avgDisplacement = 0.0f;

        static SourceState Capture(const Project& project) {
            const MorphData& m = project.GetMorphData();
            SourceState s;
            s.morphMesh              = project.GetMorphMesh();
            s.sourceProfileId        = project.GetSourceProfileId();
            s.vertexMask             = m.vertexMask;
            s.correspondences        = project.GetPointReferenceData().correspondences;
            s.wrapParams             = WrapParams::FromMorphData(m);
            s.originalMorphMesh      = m.originalMorphMesh;
            s.deformedMorphMesh      = m.deformedMorphMesh;
            s.isProcessed            = m.isProcessed;
            s.isAccepted             = m.isAccepted;
            s.displacementMagnitudes = m.displacementMagnitudes;
            s.maxDisplacement        = m.maxDisplacement;
            s.avgDisplacement        = m.avgDisplacement;
            return s;
        }

        void ApplyTo(Project& project) const {
            MorphData& m = project.GetMorphData();
            project.GetMorphMesh() = morphMesh;
            project.SetSourceProfileId(sourceProfileId);
            m.vertexMask = vertexMask;
            project.GetPointReferenceData().correspondences = correspondences;
            project.GetPointReferenceData().activeSelection.Clear();
            WrapPreset preset;
            preset.params = wrapParams;
            preset.ApplyTo(m);
            m.originalMorphMesh      = originalMorphMesh;
            m.deformedMorphMesh      = deformedMorphMesh;
            m.isProcessed            = isProcessed;
            m.isAccepted             = isAccepted;
            m.displacementMagnitudes = displacementMagnitudes;
            m.maxDisplacement        = maxDisplacement;
            m.avgDisplacement        = avgDisplacement;
        }
    };

    SourceSwapUndoAction(Project* project, SourceState before, SourceState after)
        : project_(project), before_(std::move(before)), after_(std::move(after)),
          originatingStage_(project->GetCurrentStage()) {}

    void Undo() override {
        before_.ApplyTo(*project_);
        project_->SetCurrentStage(originatingStage_);
    }
    void Redo() override {
        after_.ApplyTo(*project_);
        project_->SetCurrentStage(originatingStage_);
    }
    std::string Description() const override { return "Change Source Mesh"; }

private:
    Project* project_;
    SourceState before_;
    SourceState after_;
    WorkflowStage originatingStage_;
};

// Undo action for Stage 3 mask paint strokes. Captures the full vertexMask
// vector before/after — small enough to copy (one bool per vertex) and
// strictly correct across erase + symmetric-paint scenarios that per-vertex
// deltas would have to track manually.
class MaskStrokeUndoAction : public UndoableAction {
public:
    MaskStrokeUndoAction(Project* project,
                         std::vector<bool> before,
                         std::vector<bool> after)
        : project_(project), before_(std::move(before)), after_(std::move(after)),
          originatingStage_(project->GetCurrentStage()) {}

    void Undo() override {
        project_->GetMorphData().vertexMask = before_;
        project_->SetCurrentStage(originatingStage_);
    }
    void Redo() override {
        project_->GetMorphData().vertexMask = after_;
        project_->SetCurrentStage(originatingStage_);
    }
    std::string Description() const override { return "Paint Mask"; }

private:
    Project* project_;
    std::vector<bool> before_;
    std::vector<bool> after_;
    WorkflowStage originatingStage_;
};

// Undo action for sculpt brush strokes
class SculptStrokeUndoAction : public UndoableAction {
public:
    SculptStrokeUndoAction(Project* project, Mesh* mesh, const BrushStroke& stroke,
                            const std::map<int, Vector3>& afterPositions)
        : project_(project), mesh_(mesh), stroke_(stroke), afterPositions_(afterPositions),
          originatingStage_(project ? project->GetCurrentStage() : WorkflowStage::TouchUp) {}

    void Undo() override {
        if (!mesh_) return;
        auto& vertices = mesh_->GetVerticesMutable();
        stroke_.Undo(vertices);
        mesh_->CalculateNormals();
        if (project_) project_->SetCurrentStage(originatingStage_);
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
        if (project_) project_->SetCurrentStage(originatingStage_);
    }

    std::string Description() const override { return "Sculpt Stroke"; }

private:
    Project* project_;
    Mesh* mesh_;
    BrushStroke stroke_;
    std::map<int, Vector3> afterPositions_;
    WorkflowStage originatingStage_;
};

} // namespace MetaVisage

#endif // UNDOACTIONS_H
