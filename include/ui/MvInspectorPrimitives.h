#ifndef METAVISAGE_UI_MVINSPECTORPRIMITIVES_H
#define METAVISAGE_UI_MVINSPECTORPRIMITIVES_H

#include <imgui.h>

// Sprint 3.8.6 — shared inspector-panel primitives. Every per-stage panel
// (Stage 1 alignment, Stage 2 point reference, Stage 3 morph, Stage 4
// touch up — Stories 3.8.7–3.8.10) is composed from this set of seven
// helpers. Keeping the building blocks in a single namespace ensures the
// design-token usage stays consistent across stages and makes future
// theme tweaks a single-file edit.
//
// Usage convention:
//
//   { MetaVisage::MvInspector::DrawStageHeader(
//         "STAGE 01 / 04", "Align", "ment",
//         "Position the target mesh ..."); }
//
// Stateful helpers (axis input, file slot, footer) take in/out parameters
// or return a small Result struct so the call site decides what to do
// with the click — these primitives do not mutate Project state.

namespace MetaVisage::MvInspector {

// ---------------------------------------------------------------------------
// 1. Stage header — mono eyebrow + Saira display split heading + body desc.
//    `baseTitle` is the unaccented portion (e.g. "Align");
//    `accentSuffix` is the portion rendered in MvBlue500 (e.g. "ment").
//    Pass an empty string for either to skip that span.
// ---------------------------------------------------------------------------
void DrawStageHeader(const char* eyebrow,
                     const char* baseTitle,
                     const char* accentSuffix,
                     const char* description);

// ---------------------------------------------------------------------------
// 2. Section label — hairline rule above + 6 px MvBlue500 square dot +
//    10 px MonoSm uppercase title in MvFg3.
// ---------------------------------------------------------------------------
// Sprint 7 round-7 -- `gated` appends a small amber zap glyph after the
// title to mark sections that are locked behind a higher tier. The body
// content of the section should still be rendered (the caller wraps it
// in ImGui::BeginDisabled) so users can see what's locked; the zap is
// the visual cue that this section needs an upgrade.
void DrawSectionLabel(const char* title, bool gated = false);

// ---------------------------------------------------------------------------
// 2b. Sub-section label — Story 5.1. Smaller, indented variant for nested
//     hierarchies inside CollapsingHeaders (Stage 3's General > Interior
//     Geometry, Advanced > Optimization / Rigidity / Sampling). No hairline
//     rule, no blue dot — those are reserved for top-level sections so the
//     visual hierarchy stays one level deep. MonoSm uppercase, MvFg4.
// ---------------------------------------------------------------------------
void DrawSubSectionLabel(const char* title);

// ---------------------------------------------------------------------------
// 3. File slot — MvBg1 container with folder icon, mono filename or
//    placeholder, optional sub-line ("OBJ · 1.4 MB"), and an Import button
//    underneath. The button style flips to primary blue when `isPrimary`.
//    Caller decides what the click does (NFD picker, project load, etc.).
// ---------------------------------------------------------------------------
struct FileSlotResult {
    bool importClicked = false;
};
FileSlotResult DrawFileSlot(const char* filename,        // empty/null shows placeholder
                            const char* placeholder,
                            bool isLoaded,
                            bool isPrimary,              // primary blue Import vs ghost
                            const char* fileTypeLabel,   // optional sub-line, may be null
                            const char* importLabel = "Import");

// ---------------------------------------------------------------------------
// 4. Coloured-axis numeric input — drag-to-edit field with the X / Y / Z
//    letter on the left in `axisColor`, value right-aligned in mono on the
//    right. The result struct carries enough signal for the caller to
//    capture pre-state on drag-start and emit a single undo on drag-end:
//
//      auto r = DrawAxisInput(...);
//      if (r.editStarted)  capture = currentValue;
//      if (r.changed)      // apply
//      if (r.editFinished) // diff capture vs current and push undo
//
//    InvisibleButton is the last ImGui item, so the call site can also
//    use ImGui::IsItemActivated()/IsItemDeactivated() — but for the
//    transform group below, the Z-axis InvisibleButton is the last item,
//    making per-axis IsItem* calls unreliable. The struct flags work
//    everywhere.
// ---------------------------------------------------------------------------
struct DragEditResult {
    bool changed       = false;  // value changed at least once this frame
    bool editStarted   = false;  // user just began a drag (this frame)
    bool editFinished  = false;  // user just released a drag (this frame)
};

DragEditResult DrawAxisInput(const char* axisLetter,
                             const ImVec4& axisColor,
                             float& valueInOut,
                             float dragSpeed,
                             bool disabled = false);

// ---------------------------------------------------------------------------
// 5. Transform group — label + unit row, then a 3-column grid of axis
//    inputs. When `uniform` is true, only the X axis is editable; Y and Z
//    follow X automatically and render greyed.
//
//    The aggregate DragEditResult fires editStarted/editFinished if ANY
//    axis transitions; `changed` if any axis changed. Sufficient for the
//    "single undo per gesture" pattern.
// ---------------------------------------------------------------------------
struct TransformValues3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};
DragEditResult DrawTransformGroup(const char* label,
                                  const char* unit,         // "cm" / "°" / "×" — may be null
                                  TransformValues3& valuesInOut,
                                  bool uniform,
                                  float dragSpeed = 0.5f);

// ---------------------------------------------------------------------------
// 6. Validation row — circle status badge (✓ green / ! amber / × red) +
//    label in MvFg2 + value in mono right-aligned (MvFg1).
// ---------------------------------------------------------------------------
enum class ValidationStatus { Ok, Warning, Error };
void DrawValidationRow(const char* label,
                       const char* value,
                       ValidationStatus status);

// ---------------------------------------------------------------------------
// 7. Inspector footer — Previous / step counter / Continue layout pinned
//    to the bottom of the sidebar. `finishStyle` flips the Continue button
//    to MvSuccess green for the Stage 4 "Finish" variant.
// ---------------------------------------------------------------------------
struct InspectorFooterResult {
    bool previousClicked = false;
    bool continueClicked = false;
};
InspectorFooterResult DrawInspectorFooter(const char* stepLabel,
                                           bool previousEnabled,
                                           bool continueEnabled,
                                           const char* continueLabel = "Continue",
                                           bool finishStyle = false);

} // namespace MetaVisage::MvInspector

#endif // METAVISAGE_UI_MVINSPECTORPRIMITIVES_H
