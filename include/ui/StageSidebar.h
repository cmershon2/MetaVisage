#ifndef METAVISAGE_UI_STAGESIDEBAR_H
#define METAVISAGE_UI_STAGESIDEBAR_H

#include <QString>

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
        bool finalizeRequested = false;
        bool clearAllPointsRequested = false;
        bool clearMaskRequested = false;
        bool invertMaskRequested = false;

        // Stage 1 extras
        bool useDefaultMorphMeshRequested = false;
        // When non-empty, AppWindow opens an NFD picker, loads the texture,
        // and adds it to Project::GetTargetTextures() under this slot name.
        QString importTextureSlot;
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
};

} // namespace MetaVisage

#endif
