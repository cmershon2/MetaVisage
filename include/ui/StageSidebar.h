#ifndef METAVISAGE_UI_STAGESIDEBAR_H
#define METAVISAGE_UI_STAGESIDEBAR_H

#include "core/Transform.h"

#include <optional>
#include <string>

namespace MetaVisage {

class Project;

// Dockable ImGui panel that renders per-stage workflow controls driven by
// `Project` state. Stateless in the "panel owns the data" sense — every
// stage panel reads and writes Project directly each frame, so the sidebar
// itself keeps only transient UI bookkeeping (selection indices, etc.).
class StageSidebar {
public:
    struct Actions {
        bool nextStageRequested = false;
        bool previousStageRequested = false;
        bool importMorphMeshRequested = false;
        bool importTargetMeshRequested = false;
        bool exportMeshRequested = false;
        bool resetTransformRequested = false;
        bool processMorphRequested = false;
        bool cancelMorphRequested = false;
        bool acceptMorphRequested = false;
        bool clearAllPointsRequested = false;
        bool clearMaskRequested = false;
        bool invertMaskRequested = false;

        // Stage 1 extras
        bool useDefaultMorphMeshRequested = false;
        // When non-empty, AppWindow opens an NFD picker, loads the texture,
        // and adds it to Project::GetTargetTextures() under this slot name.
        std::string importTextureSlot;

        // Stage 3 mask-painting push state — the sidebar is the source of
        // truth for these UI-only toggles; AppWindow mirrors them onto the
        // primary viewport each frame so mouse input and rendering stay in
        // sync without a separate signal bus.
        bool maskPaintingActive = false;
        bool maskShow           = false;
        bool maskEraseMode      = false;
        float maskBrushRadius   = 10.0f;

        // Stage 1 transform-spinbox edit-end signal. Set when the user finishes
        // a Position/Rotation/Scale drag (one undo action per gesture, not per
        // frame). AppWindow pushes a TransformUndoAction with these values.
        bool transformEditCompleted = false;
        Transform transformEditBefore;
        Transform transformEditAfter;

        // Stage 2 — point-list selection request. When set, the user clicked
        // a row in the inspector's point list and AppWindow should mirror
        // that into both viewports via ViewportHost::SetSelectedPointIndex.
        // Use -1 to clear selection.
        std::optional<int> selectPointIndex;

        // Stage 2 — per-row delete request. When set, the user clicked the
        // X on that row; AppWindow erases the correspondence (and its
        // symmetric pair if present) and pushes a PointPlacementUndoAction.
        std::optional<int> deletePointIndex;
    };

    // Render docked panel titled "Stage". Returns the actions requested this
    // frame — the caller (AppWindow) dispatches them to the appropriate
    // command / worker. `selectedPointIndex` is read by Stage 2's point list
    // so the active row stays highlighted when the user picks a point in
    // the viewport.
    Actions Render(Project* project, bool morphProcessing, float morphProgress,
                   int selectedPointIndex = -1);

private:
    void RenderAlignment(Project& project, Actions& out);
    void RenderPointReference(Project& project, Actions& out);
    void RenderMorph(Project& project, bool processing, float progress, Actions& out);
    void RenderTouchUp(Project& project, Actions& out);

    // Stage 3 mask painting — state persists across frames. When painting is
    // active, LMB in the morph viewport paints mask bits instead of doing
    // camera work. Radius/erase are brush settings.
    //
    // Show Mask defaults OFF so the Stage 3 preview-mode dropdown (Deformed /
    // Original / Overlay / Heat Map) drives what the user sees. Toggling
    // Paint Mask flips the renderer to mask colors automatically while
    // painting (see StageSidebar::RenderMorph: out.maskShow ORs in
    // maskPaintingActive_), so the user only ticks Show Mask when they want
    // the painted mask visible while *not* actively painting.
    bool maskPaintingActive_ = false;
    bool maskShowMask_       = false;
    bool maskEraseMode_      = false;
    float maskBrushRadius_   = 10.0f;

    // Captured target-mesh transform on the frame a Stage 1 spinbox is first
    // activated, so we can emit a (before, after) pair on edit-end.
    Transform transformEditCapture_;

    // Mirror of viewport-side point selection so the Stage 2 list can
    // highlight the active row. Refreshed on each Render() call.
    int selectedPointIndexExternal_ = -1;

    // Stage 3 — last preset stamped onto MorphData via the preset cards.
    // 0 = Quick Wrap, 1 = Fantasy Wrap, -1 = Custom (any manual edit
    // resets to this so the active card highlight stays accurate).
    int lastAppliedPreset_ = -1;
};

} // namespace MetaVisage

#endif
