#ifndef METAVISAGE_UI_STAGESTEPPER_H
#define METAVISAGE_UI_STAGESTEPPER_H

#include <optional>

namespace MetaVisage {

class Project;

namespace StageStepper {

// Sprint 3.8.3 — full-width 64px progress stepper rendered between the menu
// bar and the dockspace. Each of the four workflow stages is a clickable
// step button with a numbered/checkmarked disc, status eyebrow, and stage
// name. The active step gets a 2px MvBlue500 underline along its bottom.
//
// Forward-jump validation mirrors the StageSidebar `Next >` button: clicks
// on stages strictly less than the current stage always navigate; a click
// on (current+1) navigates only if Project::CanProceedToNextStage() is
// true; clicks further forward are ignored. This keeps the stepper from
// becoming an end-run around the per-stage gating.
//
// Rendering style — MvBg1 fill, MvLine2 bottom hairline. Discs:
//   active   : MvBlue500 fill,  MvBlue300 border, dark mono "0N"
//   complete : MvSuccess @12% fill, MvSuccess @40% border, MvSuccess check
//   upcoming : MvBg2 fill,      MvLine2 border,    MvFg4 mono "0N"
// Eyebrows are MonoSm-uppercase IN PROGRESS / COMPLETE / UPCOMING in the
// matching tint; stage names are BodyMd in MvFg1/MvFg2/MvFg3.

constexpr float HeightPx = 64.0f;

// Render the stepper at the current cursor position with the given width.
// Returns the 0-based index of the stage the user just clicked, or
// std::nullopt if no actionable click occurred this frame. The caller is
// responsible for applying the navigation (typically project->SetCurrentStage
// + ApplyStageViewportMode in AppWindow).
//
// `project` may be null; in that case all four steps render in the upcoming
// style and clicks are ignored.
std::optional<int> Render(const Project* project, float width);

} // namespace StageStepper
} // namespace MetaVisage

#endif // METAVISAGE_UI_STAGESTEPPER_H
