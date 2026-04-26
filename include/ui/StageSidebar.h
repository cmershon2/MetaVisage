#ifndef METAVISAGE_UI_STAGESIDEBAR_H
#define METAVISAGE_UI_STAGESIDEBAR_H

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
    };

    // Render docked panel titled "Stage". Returns the actions requested this
    // frame — the caller (AppWindow) dispatches them to the appropriate
    // command / worker.
    Actions Render(Project* project, bool morphProcessing, float morphProgress);

private:
    void RenderAlignment(Project& project, Actions& out);
    void RenderPointReference(Project& project, Actions& out);
    void RenderMorph(Project& project, bool processing, float progress, Actions& out);
    void RenderTouchUp(Project& project, Actions& out);

    // Stage 3 mask painting — state persists across frames. When painting is
    // active, LMB in the morph viewport paints mask bits instead of doing
    // camera work. Radius/erase are brush settings.
    bool maskPaintingActive_ = false;
    bool maskShowMask_       = true;
    bool maskEraseMode_      = false;
    float maskBrushRadius_   = 10.0f;
};

} // namespace MetaVisage

#endif
