# Sprint 3.8 Retrospective

**Date**: 2026-04-28
**Sprint**: [Sprint 3.8 — UI/UX Redesign Implementation](../sprints/sprint-03.8.md)
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary

- **Velocity (Planned)**: 32 points
- **Velocity (Actual)**: 32 points
- **Sprint Goal Achievement**: 100% on planned scope. All twelve committed stories shipped, plus a substantial fold-in of UX-review feedback that pushed the chrome past the original spec in several places (frameless OS window, Lucide icon font, Stage 4 tool rail variant, point list with badges + S/T squares + per-row delete, Stage 2 info bar dropped, Live Link UI removed, "Use offline" omitted).
- **Stories Completed**: 12 of 12 (3.8.1 through 3.8.12)
- **Carryover Items**: None.

---

## Retrospective

### LIKED (What we liked)

- **Story 3.8.6 (Inspector Primitives) sequenced perfectly as the load-bearing story.** Landing the seven `MvInspector` helpers before any per-stage panel meant 3.8.7 / 3.8.8 / 3.8.9 / 3.8.10 each came in around 200–500 lines of compose-and-wire code instead of a fresh chrome rewrite per stage. The four panels look like cousins because they actually share a header / section-label / footer implementation — not because four separate copies coincidentally matched.
- **Architect-agent audit before Story 3.8.1 saved a week of guesswork.** The pre-sprint audit mapped every PackColor literal in `Viewport.cpp`, identified that `ApplyDarkTheme` was the single style entry point (not 17 scattered ones), and confirmed the `"Stage"` window identity is load-bearing for the dockspace. Everything in the audit's ≈30-bullet "constraints for the visual overhaul" turned out to be true; nothing surprised us mid-sprint that the audit hadn't called out.
- **Lucide icon font adoption mid-sprint paid off in the same sprint.** Story 3.8.4 shipped 9 hand-drawn ImDrawList icons (~280 lines of geometry). Once Lucide landed in `c16491c`, the immediate refactor (`cca29ab`) deleted those 280 lines and every subsequent story (3.8.5 toolbar, 3.8.7 file slot, 3.8.8 point-list X delete, 3.8.9 preset cards, 3.8.10 brush cards, 3.8.11 welcome modal, 3.8.12 login screen) referenced glyph constants from `MvIcons.h` instead. Net negative line count after the migration despite touching seven new surfaces.
- **Frameless window promotion (Option C from the original risk matrix) was much smaller than feared.** The Sprint 3.8.2 plan flagged GLFW frameless mode as a multi-day rabbit hole; the actual Win32 `WM_NCHITTEST` + `WM_NCCALCSIZE` subclass turned out to be ~120 well-bounded lines (`fd287b0`). Aero Snap, edge-resize cursors, system menu, double-click-maximize all came back free. Lesson worth remembering: "rabbit hole" estimates inherited from web-app contexts overestimate the work for native Win32 because the OS does most of it.
- **`MvInspector::DragEditResult` extension caught the right abstraction layer.** The first draft of `DrawAxisInput` returned a bare `bool changed`, which forced Stage 1's transform-edit undo capture to call `ImGui::IsItemActivated()` at the call site. That worked for one axis but broke for the X+Y+Z group because the per-axis InvisibleButton wasn't the last item. Promoting the return type to `{ changed, editStarted, editFinished }` made the gesture-level event the API instead of the per-axis event — and the caller's "one undo per drag" pattern got 3 lines simpler at every consumer site.
- **Headless smoke test stayed green on every commit, including the four mid-sprint pivots.** 32 commits, every one of them passes `ctest -C Release` in 0.6–1.0 s. Build cleanliness preserved through the Lucide migration, the `Stage2InfoBar` add+remove cycle, the frameless promotion, and the four 3.8.5 click-through bug fixes. The smoke test couldn't have caught the visual regressions, but it caught zero plumbing regressions, which is exactly what it's there for.

### LEARNED (What we learned)

- **`ImGuiWindowFlags_NoBringToFrontOnFocus` and "must receive input" are different problems.** The Story 3.8.5 click-through bug (`f788584`) had the new viewport overlay windows stuck behind the host's central dockspace node because the flag was copy-pasted from the host window's flags. The pre-3.8.5 `DrawShadingModeOverlay` worked because it didn't set the flag — clicking it brought the window to front and registered the click. Heuristic: **`NoBringToFrontOnFocus` is for chrome that exists *under* an interactive surface; remove it from anything the user is supposed to click.**
- **Design-file values aren't NRICP parameter ground truth.** Story 3.8.9's first cut baked the `stage3.jsx` Quick Wrap / Fantasy Wrap parameters into the preset cards verbatim. The numbers came from the design team's mockup, not the algorithm tuning, and applying either preset visibly broke the wrap (`3e80734`). The actual semantics were a one-line note in CLAUDE.md ("Quick Wrap sets rigidity to 1 for human-like characters") that I had read at sprint planning time and forgotten by the time I wrote the preset structs. **Lesson: design files give the surface; the codebase's prior docs (CLAUDE.md, product-vision.md) give the behavior.**
- **MSVC's hex-escape consumer is greedy across UTF-8 byte sequences.** `"session=8f3a\xE2\x80\xA6d21c"` reads `\xA6d21c` as a single hex escape and overflows. Two-string concatenation (`"\xA6" "d21c"`) terminates the escape correctly. Hit twice in this sprint (Story 3.8.12 first); worth grepping any future literal that combines `\xC2`/`\xE2`-prefixed UTF-8 with hex digits as the next character.
- **`<windows.h>`'s `min` / `max` macros need `NOMINMAX` *before any header that could transitively include them*.** Hit when the frameless WM_NCHITTEST hook landed (`fd287b0`) — the `#define NOMINMAX` was inside the Win32 block (line ~70 of AppWindow.cpp) but ImGui's GLFW backend transitively pulled `windows.h` in via the ImGui include block (line ~30) before the define. Existing `std::min` calls in the sculpt-undo builder broke cryptically. **Fix: hoist `NOMINMAX` to the very top of the translation unit, before any include.**
- **The OS framebuffer clear color shows everywhere ImGui doesn't paint AND the GL pass doesn't cover.** The "gray spacers" the user kept reporting (`0ba47bd`) weren't ImGui-drawn hairlines — they were the legacy `30/255` glClearColor leaking through 1–2 px gaps where the host window's `NoBackground` transparency met adjacent docked windows. Switching the OS clear to `MvBg0` made the leftover seams invisible. **Heuristic: if a chrome colour you didn't intend keeps appearing, search for `glClearColor` first, not for `AddLine`.**
- **ImGui's `ItemSpacing.y` between top-level layout blocks adds visible gaps when the host has `NoBackground`.** `7d0d644` pushed `ItemSpacing.y = 0` for the host window's direct children (menu bar / stepper / dockspace / status bar) so they butt up against each other. The default 6 px y-gap was invisible in fully-painted windows but read as a "chrome strip" in a passthrough host. **For full-bleed chrome layouts, ItemSpacing needs explicit zero.**
- **`SetNextWindowFocus` every frame is not the same as "draw on top".** Story 3.8.12 used `SetNextWindowFocus` in `LoginScreen::Render` to override the host shell's focus chain (necessary when the host still rendered after login). After `7a4d365` short-circuited the host shell entirely in login mode, the focus pin became redundant — and started routing every click to the login window, swallowing title-bar caption-button hits even though the title bar was visually drawn on top. **`SetNextWindowFocus` is for input routing; ImGui's default hit-testing handles draw-order-based click priority on its own.**

### LACKED (What we lacked)

- **No automated test caught the click-through bug.** Story 3.8.5's `NoBringToFrontOnFocus` mistake stayed undetected through a clean build, a green smoke test, and the architect-agent review until the user manually clicked a shading-mode toggle and got nothing. The exact scenario Sprint 3.7's retro Action Item #3 (an `imgui_test_engine` lane that drives clicks through the workflow) was supposed to address — and which still doesn't exist. Three out of the four manual review cycles in this sprint surfaced regressions an interactive script would have caught: the toolbar click-through (3.8.5), the preset values breaking the wrap (3.8.9), the login caption buttons not firing (3.8.12 follow-up).
- **Visual review of the Stage 3 preset values would have caught the "wrap broken" issue at design-team sign-off, not at implementation.** The sprint plan's DoD listed "side-by-side screenshot capture: every redesigned surface vs its `previews/*.png` reference" but only really applied for layout / colour matching, not for behavioural parameters. Stage 3 needs a different test: load the bundled MetaHuman head, click Quick Wrap, hit Process, compare displacement against a known-good baseline. We don't have any of that scaffolding.
- **No automated way to verify Lucide glyph rendering.** The Icon Glyph Explorer shipped in `c16491c` is great for picking codepoints but does nothing to confirm that a given `MV_ICON_*` constant produces the expected visual at runtime. If a future Lucide release renumbers glyphs, the build stays green and the wrong icon appears at every consumer site. Worth a smoke step that walks every constant in `MvIcons.h` and checks the glyph atlas has a non-empty bitmap for that codepoint.
- **Status-line "Implementation Complete" still went up before manual smoke ran.** Sprint 3.7's retro Action Item #2 specifically tightened the Definition of Done template to forbid this; I still set the sprint outcome to "Implementation Complete" before the manual UI smoke + review-pr lane closes. The notes say "pending /review-pr + manual UI smoke" so the framing is honest, but the recurring pattern is real.

### LONGED FOR (What we longed for)

- **An `imgui_test_engine` lane that runs against the actual app shell.** Same Action Item #3 from Sprint 3.7. Sprint 3.8 had four real bugs that an interactive script would have caught (the click-through, the preset values, the login caption buttons, and the Welcome modal Open Project edge collision). Five scenarios would cover the workflow's contact surface; would have saved most of the post-sprint follow-up commits.
- **A "design intent vs implementation parity" sub-lane in the manual smoke checklist.** Today the manual smoke verifies stage progression, undo round-trip, mask paint, and sculpt brush. It doesn't verify that Quick Wrap actually produces a tight wrap, that Fantasy Wrap actually produces a loose one, or that the preset cards round-trip through `Project::MorphData::previewMode` correctly. Splitting the smoke into "behavioural parity" (load → process → assert disp magnitude in expected range) and "interactive UX parity" (clicks fire, panels render) would have caught the Story 3.8.9 preset bug before user verification.
- **Action Item #2 from Sprint 3.7 fully enforced: don't mark "Implementation Complete" before manual smoke + review-pr.** The `sprint-planner` skill's DoD template was updated last sprint; I still ignored it this sprint. Worth promoting from "process note in the sprint file" to "an explicit blocker on the close-out script" — i.e. don't even let `/sprint-retrospective` run until the DoD checklist is mechanically satisfied.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | **Promoted from Sprint 3.7 Action Item #3** — Add an `imgui_test_engine` smoke-UI lane covering: (a) Stage 1 → 4 progression with the bundled head, (b) shading-mode + visibility toggles in the viewport toolbar, (c) Stage 3 process + Accept, (d) Stage 4 brush switch + sculpt + undo, (e) login screen Sign in → Cancel → Sign in cycle. Goal: catch the click-through / focus-routing class of bug before user review. | Casey | Mid Sprint 4 | [ ] |
| 2 | Add a behavioural-parity smoke target: load the bundled head, apply Quick Wrap, run Process, assert `MorphData::maxDisplacement` falls within a known-good band. Same for Fantasy Wrap. Catches preset-value drift like the one Sprint 3.8.9 shipped first time. | Casey | Sprint 4 close | [ ] |
| 3 | Make the `sprint-retrospective` skill's pre-flight check verify the DoD checklist is mechanically satisfied (PR opened, manual smoke completed, screenshot review posted) before the retro can run. Stops the "Implementation Complete before review" pattern that's now spanned three retros. | Casey | Sprint 4 kickoff | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 3.7) | Status |
|---|---------------------------|--------|
| 1 | Fix the PR #20 review blocker — `AppWindow::StartMorph()` must `undoStack_->Clear()` before replacing `morphData.deformedMorphMesh` | ✅ Done — closed in commit `2883559` (`fix(ui): clear undo stack in StartMorph to drop dangling sculpt Mesh*`) before PR #20 merged |
| 2 | Update the `sprint-planner` skill's Definition of Done template to require "PR review passed AND Phase D manual UI smoke completed" before "Complete" | ✅ Done — Sprint 3.8's plan file ([sprint-03.8.md](../sprints/sprint-03.8.md)) shipped with the updated DoD template including the explicit guard. **But** the sprint outcome still went up as "Implementation Complete" before that gate cleared (see Sprint 3.8 LACKED section); the template improved, the discipline didn't. Promoted to Sprint 3.8 Action Item #3 with mechanical enforcement. |
| 3 | Add an `imgui_test_engine` (or scripted `--smoke-ui` lane) target alongside the existing CTest smoke test | ❌ Not started — Sprint 3.8 was a UI-heavy sprint that would have benefited the most from this; it stayed deferred. Carried forward as Sprint 3.8 Action Item #1. |

Two of three closed. The unfinished imgui-test-engine lane is the one that would have prevented the most pain this sprint.

---

## Key Metrics

- **Velocity Trend**: 32/32 actual vs 32/32 planned (vs 26/26 in 3.7, 24/26 in 3.6, 17/22 in 3.5). Second consecutive sprint hitting 100% of committed scope. The 32-point commitment was a stretch over the 26-point Sprint 3.7 baseline; the per-stage panel stories (3.8.7 / 3.8.8 / 3.8.9 / 3.8.10) compressed as expected once the Story 3.8.6 primitives landed, which is exactly the assumption the plan banked on.
- **Team Morale**: High. The app went from "Qt removed, looks like the v1.0 era ImGui" at sprint start to "fully styled blueprint dark theme with the design's typography, icons, and chrome" at sprint end. Ten new modules, six font assets bundled, ~17,700 net lines added, and the smoke test stayed green on every commit. The frameless window flip late in the sprint was the kind of "should have been a separate sprint" change that turned out to take an afternoon — small wins like that are why this sprint felt good.
- **Process Health**: Mixed. **Improving** — the architect-agent audit before sprint planning is now a habit and consistently pays off (caught the load-bearing surfaces in 3.8.1, would catch them in 3.9 too). The sprint-planner DoD template improvements from Sprint 3.7 carried forward correctly. Two of three previous action items closed. **Concerning** — the same "Implementation Complete before manual smoke" anti-pattern repeated for the third sprint in a row, despite explicit retro action items in 3.6 and 3.7. Action Item #3 mechanically enforces it this time. The imgui-test-engine lane has been wished for in three retros now and built in zero of them; Action Item #1 is now overdue rather than aspirational.
