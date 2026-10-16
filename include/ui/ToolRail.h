#ifndef METAVISAGE_UI_TOOLRAIL_H
#define METAVISAGE_UI_TOOLRAIL_H

namespace MetaVisage {

class Project;
class Viewport;
enum class WorkflowStage;

namespace ToolRail {

// Sprint 3.8.4 — 56 px-wide vertical tool rail rendered as a left column of
// the work area in stages where it applies. Two variants share the same
// chrome (eyebrow header, 40×40 buttons, divider, Help anchored to bottom):
//
//   Stage 1 (Alignment) — XFORM eyebrow:
//     Move (W) / Rotate (E) / Scale (R)  → Viewport::SetTransformMode
//     Symmetry X / Pivot snap            → visual stubs (no backend yet)
//
//   Stage 4 (Touch Up) — BRUSH eyebrow:
//     Smooth / Grab / Push-Pull / Inflate → Viewport::SetBrushType +
//                                            Project::SculptState::activeBrush
//     Symmetry X                          → SculptState::symmetryEnabled
//
// Stages 2 (Point Reference) and 3 (Morph) do not show the rail — the
// caller is responsible for not invoking Render in those stages.
//
// The rail mutates state in place (via the supplied Viewport / Project
// pointers) and reports only the Help-button click to the caller, which is
// the one cross-stage signal that needs application-level dispatch.

constexpr float WidthPx = 56.0f;

struct Result {
    bool helpRequested = false;
};

// Renders the rail as a 56 px × `height` BeginChild block at the current
// cursor position. The caller advances horizontally via SameLine after
// EndChild so the dockspace docks to the right of the rail.
//
// `viewport` and `project` may be null during init / shutdown windows; the
// rail renders in a read-only state and ignores clicks if either pointer
// it would mutate is null.
Result Render(WorkflowStage stage, Project* project, Viewport* viewport,
              float height);

} // namespace ToolRail
} // namespace MetaVisage

#endif // METAVISAGE_UI_TOOLRAIL_H
