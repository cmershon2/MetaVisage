#ifndef METAVISAGE_UI_STAGE2INFOBAR_H
#define METAVISAGE_UI_STAGE2INFOBAR_H

namespace MetaVisage {

class Project;

namespace Stage2InfoBar {

// Sprint 3.8.8 — 36 px-high info bar that sits above the dual viewports
// in Stage 2 (Point Reference). Layout:
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │ [SOURCE] file.obj  14,206 verts · 3 pts    N pts paired    [TARGET] target.obj  9,182 verts · 3 pts │
//   └──────────────────────────────────────────────────────────────────┘
//
// The SOURCE pill is amber (MvWarning), the TARGET pill is blue
// (MvBlue400) — matches the per-pane label colours from the design's
// stage2.jsx. The centred indicator shows the paired-point count, the
// same number that drives Stage 2's PAIRED stat card and gates Continue.
//
// Caller (AppWindow::Run) is responsible for shrinking the central rect
// passed to ViewportHost::BuildLayout by HeightPx so the viewports start
// below the bar instead of being painted over.

constexpr float HeightPx = 36.0f;

// Render the info bar at (x, y) with the given full width. `project` may
// be null during init / shutdown windows; the bar then renders empty
// placeholder pills.
void Render(const Project* project, float x, float y, float w);

} // namespace Stage2InfoBar
} // namespace MetaVisage

#endif // METAVISAGE_UI_STAGE2INFOBAR_H
