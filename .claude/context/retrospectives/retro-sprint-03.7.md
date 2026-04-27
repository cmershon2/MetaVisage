# Sprint 3.7 Retrospective

**Date**: 2026-04-26
**Sprint**: [Sprint 3.7 — Qt Decoupling & Deferred Code Review Cleanup](../sprints/sprint-03.7.md)
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary

- **Velocity (Planned)**: 26 points
- **Velocity (Actual)**: 26 points
- **Sprint Goal Achievement**: 100% on planned scope. `Qt6::Core` removed entirely (zero Qt DLLs in `MetaVisage.exe`'s import table per `dumpbin`), all four Sprint 3.6 `/review-pr` blockers closed, all three Sprint 3.6 action items closed (smoke test, CMake guard, plaintext-token encryption), Undo/Redo wired at every mutation site, the architect-agent confirmed Story 3.7.4 (MorphWorker `QThread` → `std::thread`) was already complete pre-sprint.
- **Stories Completed**: 7 of 7 (3.7.1, 3.7.2, 3.7.3, 3.7.4, 3.7.5, 3.7.6, 3.7.7)
- **Carryover Items**: None on the sprint backlog. Phase D manual UI testing surfaced four regressions that were fixed in-sprint as commit `01952a8`.

---

## Retrospective

### LIKED (What we liked)

- **Pre-sprint architect-agent audit caught a 3-point story already-done.** Story 3.7.4 (MorphWorker port) was complete on the branch before implementation started — verified by reading the live source, not by trusting prior memory observations. The reclaimed 3 points absorbed scope expansion (full Logger rewrite instead of a shim, plus the DPAPI legacy-plaintext migration path) without burning weekend hours.
- **Phased delivery (A → B → C → cleanup), each commit atomic and build-verified.** The Qt removal alone was 49 files / 1,416 / 1,264 lines, but `git log master..HEAD --oneline` reads as a clean story: review blockers → Qt removal → undo+infra → docs → regression fixes → dead-file purge. Each commit was independently buildable; bisecting future regressions stays cheap.
- **All three Sprint 3.6 action items closed in-sprint.** Smoke-test fixture (`tests/smoke/launch_to_export.cpp`), CMake purity guard (`foreach(_forbidden_qt_target …)`), and plaintext-token encryption (`utils/SecureStorage` + DPAPI). The recurring "deferred to next sprint" cycle from 3.5 → 3.6 ended here.
- **C++17 → C++20 bump was load-bearing and risk-free.** `std::format` directly replaced `QString::arg()` across ~99 `MV_LOG_*` call sites — pattern-for-pattern, no ergonomics regression, no third-party dep. MSVC 19.30+ + the project's existing toolchain absorbed the bump without configuration changes.
- **`nlohmann::ordered_json` paid off immediately.** `.mmproj` round-trips through the new serializer produce byte-stable output for unchanged projects. The decision was a one-line change at template-parameter level and avoided a noisy diff every time a project saved.
- **Headless smoke test caught zero regressions but earned its keep anyway.** `tests/smoke/launch_to_export.cpp` runs in <3 s as a CTest target, exercises the load → serialize → reload → export pipeline against the bundled MetaHuman head, and exits with unique non-zero codes per failure stage. CI-ready without a GL context.
- **Manual UI smoke (Phase D) surfaced 4 real regressions the headless test couldn't have caught.** Mask-painting allocation timing, preview-dropdown UX, undo across stage transitions, and the missing export-success modal — all interactive UX bugs that `metavisage_smoke.exe` was deliberately not built to detect. Validates the manual-test investment as complementary to (not redundant with) automated coverage.

### LEARNED (What we learned)

- **`ImGui::IsItemActivated()` + `ImGui::IsItemDeactivatedAfterEdit()` is the right abstraction for one-undo-per-gesture.** Wrapping the three `DragFloat3` calls in `RenderAlignment` with `captureBefore` / `emitOnEditEnd` lambdas turned frame-rate-coupled drag input into gesture-level events. Pattern is reusable for any future spinbox/slider that needs undo wiring.
- **Side-effecting allocation in input-handling code compounds with renderer short-circuits.** `HandleMaskPress` allocated `morphData.vertexMask` *before* checking whether the raycast hit the mesh — so a single missed click on a mesh in mask mode left a non-empty all-false mask. The renderer's `if (showMask_ && !vertexMask.empty())` short-circuit then pinned the morph-stage view into mask-color render forever. Either bug alone is benign; together they break the preview dropdown completely. **Heuristic: state mutation in input handlers must come after success checks, not before.**
- **`UndoableAction` subclasses holding raw pointers to `shared_ptr`-owned objects need a "clear stack on owner replacement" discipline.** `SculptStrokeUndoAction` stores `Mesh*` from `morphData.deformedMorphMesh.get()`; when `PollMorphCompletion` replaces the `shared_ptr`, the old `Mesh` is destroyed and queued undo actions hold dangling pointers. The Qt era's `MainWindow::OnMorphPreviewModeChanged` cleared the undo stack on re-morph — the ImGui port's `StartMorph` does not. This is the `/review-pr` blocker on PR #20.
- **A 30+-file refactor introduces compile errors at link time even when each file appears correct in isolation.** Three rebuild rounds: `.isEmpty()` leftovers in `AppWindow.cpp`, dead Qt-era headers still in `CORE_HEADERS`, and a missing `<map>` include. None caught at compile time of the individual TU. Lesson: **after a wide-surface refactor, build incrementally at logical checkpoints (per-layer), not just at the end.**
- **The `dumpbin /dependents` check on the produced `.exe` is the most trustworthy zero-Qt confirmation.** Source greps catch source-level uses, but transitive link reintroductions show up only in the actual binary's import table. Worth running once per major dep-removal sprint and once after the configure-time Qt-purity guard lands.

### LACKED (What we lacked)

- **"Implementation complete" was marked before manual UI smoke ran — same anti-pattern as Sprint 3.6.** The sprint outcome was updated to "Implementation Complete — manual UI smoke test pending" before Phase D started. Phase D then surfaced four regressions, which had to be patched in-sprint and re-committed. The status line was technically accurate but the framing implied "we're done modulo formality" when the four bugs were real defects.
- **No automated coverage of Stage 3 mask-paint UX or preview-dropdown cycling.** The smoke test is excellent for the load-serialize-export pipeline (it runs the same Mesh / Project / ProjectSerializer / MeshExporter classes that absorbed most of the Qt port changes), but it can't drive ImGui inputs and doesn't exercise interactive UX state machines. Three of the four Phase D bugs were UX-level.
- **`/review-pr` findings on completed sprints have no auto-tracking.** Same gripe as Sprint 3.6's retro, still missing. The PR #20 review surfaced a blocker (sculpt undo dangling `Mesh*` after re-morph) and several non-blocking notes; without an auto-file-to-backlog hook, they live only in the PR comment thread until I read them.
- **Pre-sprint memory observations needed manual verification before being trusted.** The architect-agent's audit caught Story 3.7.4 already-done because it read the *current* source, but it also noted that several Sprint 3.6 memory observations described "remaining Qt coupling" that no longer existed. The memory system is helpful as a first read but **observations are time-stamped state, not present truth.**

### LONGED FOR (What we longed for)

- **An ImGui-input-driven UI integration test.** Even a 5-scenario script that drives `ImGui::IsItemClicked` + cursor-position events would have caught the Phase D regressions before manual testing. Could be backed by `imgui_test_engine` (has a clean public API) or by a custom "scripted-input" lane in `main.cpp` gated by `--smoke-ui <script.txt>`.
- **A type-system invariant for "undo stack must be cleared when this `shared_ptr` is reassigned."** Currently relies on every call site (StartMorph, OnFileNewProject, OpenProjectFromDisk) remembering to call `undoStack_->Clear()`. Easy to miss — exactly the bug PR #20 review caught. A wrapper type `OwnedMesh` that clears the undo stack via RAII on assignment would push the discipline into the type, not into call-site memory.
- **Auto-tracking for `/review-pr` findings.** Promoted from Sprint 3.6's retro, still wanted. When the agent flags a non-blocking issue, it should land in `product_backlog.md` with a citation back to the PR. Today they live in the GitHub PR thread and re-surface only when the same code gets re-reviewed.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Fix the PR #20 review blocker — `AppWindow::StartMorph()` must `undoStack_->Clear()` before replacing `morphData.deformedMorphMesh`. Same pattern as `OnFileNewProject` / `OpenProjectFromDisk`. | Casey | Before PR #20 merge | [ ] |
| 2 | Update the `sprint-planner` skill so the `Definition of Done` template includes an explicit "PR review passed AND Phase D manual UI smoke completed" checkbox. Stop marking sprint outcomes "Implementation Complete" before both. | Casey | Sprint 4 kickoff | [ ] |
| 3 | Add an `imgui_test_engine` (or scripted `--smoke-ui` lane) target alongside the existing CTest smoke test. Even five scenarios — load default head, advance Stage 1→2→3→4, paint+undo mask, switch preview mode, export+confirm-modal — would have caught the four Phase D regressions. | Casey | Mid Sprint 4 | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 3.6) | Status |
|---|--------------------------|--------|
| 1 | Fix the four `/review-pr` blockers (CancelTransform Escape revert, `HeaderCallback` `>=13`, gizmo asset paths via `Paths::ExecutableDir()`, plaintext token storage) | ✅ Done — first three closed in commit `71ea8ab` (Phase A); plaintext token storage closed in commit `d7ac097` (Phase C, `utils/SecureStorage` + DPAPI) |
| 2 | Promote the smoke-test project into a Sprint 3.7 story | ✅ Done — `tests/smoke/launch_to_export.cpp` lands in commit `d7ac097`; CTest passes 1/1 in <3 s exercising load → ProjectSerializer round-trip → FBX export |
| 3 | Land the CMake guard against `Qt6::Gui` / `Widgets` / `OpenGL` / `OpenGLWidgets` | ✅ Done — `CMakeLists.txt` configure-time `foreach(_forbidden_qt_target …)` loop in commit `d7ac097`. Includes `Qt6::Core` even though already removed, so the guard keeps it gone |

All three Sprint 3.6 action items closed in this sprint. The recurring carryover cycle from 3.5 → 3.6 → 3.7 ends here.

---

## Key Metrics

- **Velocity Trend**: 26/26 actual vs 26/26 planned (vs 24/26 in 3.6, 17/22 in 3.5). First sprint to ship the full committed scope plus all carryover from prior sprints. Three points reclaimed from already-complete Story 3.7.4 absorbed scope expansion (Logger rewrite + DPAPI legacy migration) without overrunning.
- **Team Morale**: High. Qt is gone, the dead-file purge eliminated 7,400 lines of inert code, the configure-time Qt-purity guard means it stays gone. The `MetaVisage.exe` import table reading "no Qt6Core.dll" is the most satisfying single line of `dumpbin` output this project has produced.
- **Process Health**: Improving. Three-of-three previous action items closed (vs. one-of-three in 3.6) is a real signal the carryover cycle broke. **But** the same "outcome marked complete before review/UI smoke" anti-pattern repeated, and PR #20 review found a real blocker that earlier verification would have caught. Action item #2 promotes this from a recurring observation to a process change. Action items #1 and #3 give Sprint 4 concrete inputs (PR fix, UI smoke target).
