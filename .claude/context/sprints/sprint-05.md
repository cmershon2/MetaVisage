# Sprint 5: Advanced Parameters, Cottage-Cheese Investigation & Stage 4 Polish

**Sprint Goal**: Ship the full collapsible Stage 3 parameter hierarchy that the master plan has been holding open since v1.0 feedback, investigate (and ideally close) the "cottage cheese face" deformation artifact, polish the Stage 4 brush selector to match the design system, and finally land the `imgui_test_engine` smoke-UI lane that has now been deferred across four consecutive retros.
**Duration**: 2026-04-30 — 2026-05-13 (2 weeks, 10 working days)
**Capacity**: 28 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor — match to Sprint 4's 28 capacity since velocity and process discipline have been stable for five sprints)
**Committed**: 22 points

> Scope is set by [SPRINT_PLAN.md § Sprint 5](./SPRINT_PLAN.md) (Stories 5.1–5.3) plus the two carryover items from [retro-sprint-04.md](../retrospectives/retro-sprint-04.md) — Action Item #1 (Story 4.5 → Story 5.4) and Action Item #2 (Story 4.7 gate refinement → Story 5.5). The pre-sprint architect audit (saved as agent memory `a96a946a41c618f7f`) confirmed: all NRICP params are already in `MorphData`, the brush grid is **not** yet on `ToolRail` (still a 2×2 ImGui table at [src/ui/StageSidebar.cpp:1230](../../../src/ui/StageSidebar.cpp#L1230)), `MvInspectorPrimitives` lacks a sub-section helper, `Config` has no UI-state section, `vcpkg.json` does not yet declare the `imgui[test-engine]` feature, and `AppWindow::TickOnce()` does not exist.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 5.1: Collapsible Stage 3 Parameter Hierarchy** | 8 | None — but blocks 5.2 (the warning-banner exit ramp lives in the same inspector) | [ ] |
| 2 | **Story 5.2: "Cottage Cheese Face" Artifact Investigation** | 5 | Soft-depends on 5.1 if the fix is a UI warning | [ ] |
| 3 | **Story 5.3: Stage 4 Brush Selector Visual Polish** | 3 | None | [ ] |
| 4 | **Story 5.4: imgui_test_engine Smoke-UI Lane** *(carryover from Sprint 4 — retro AI #1, fourth carry)* | 5 | None — Path A confirmed at Sprint 4 day-1 spike | [ ] |
| 5 | **Story 5.5: Refine `/sprint-retrospective` Gate Bypass** *(carryover from Sprint 4 — retro AI #2)* | 1 | None — natural sprint-kickoff task | [ ] |

> **Critical-path**: 5.5 → 5.4 → 5.1 → 5.2 → 5.3 is the natural ordering — 5.5 (1 pt skill change) and 5.4 day-1 spike (vcpkg + `TickOnce()`) clear the deck for 5.1's eight-point UI rework, then 5.2's investigation can build on 5.1's Reset-to-Defaults relocation, and 5.3 is independent polish that can slot wherever capacity opens.

---

## Story Detail

### Story 5.1: Collapsible Stage 3 Parameter Hierarchy (8 pts)

The Stage 3 inspector currently surfaces eight advanced parameters as a flat list under a single `CollapsingHeader("Advanced")` ([src/ui/StageSidebar.cpp:1036–1062](../../../src/ui/StageSidebar.cpp#L1036)). The master plan's hierarchy reduces visual clutter by grouping params into named sub-sections that match how power users think about the wrap solver.

- **New `MvInspectorPrimitives::DrawSubSectionLabel(const char* label)` helper** — same family as the existing `DrawSectionLabel` but no hairline rule, indented one spacing unit, smaller font weight. Lives in [include/ui/MvInspectorPrimitives.h](../../../include/ui/MvInspectorPrimitives.h) / [src/ui/MvInspectorPrimitives.cpp](../../../src/ui/MvInspectorPrimitives.cpp). All sub-section headers in the new hierarchy use this primitive — never hand-render with raw `ImGui::Text` + spacing literals (per CLAUDE.md design-system rules).
- **Hierarchy restructure** in `RenderMorph` ([src/ui/StageSidebar.cpp:900–1190](../../../src/ui/StageSidebar.cpp#L900)):
  - **Vertex Masking** — moves to the **top** of the sidebar body (currently at lines ~1112–1167 below the result summary); always visible; existing controls untouched.
  - **General** (`ImGui::CollapsingHeader`, `ImGuiTreeNodeFlags_DefaultOpen`):
    - Stiffness Init / Final / Steps, ICP Iterations, Normal Threshold, Landmark Weight (all already in `MorphData`)
    - **Interior Geometry** (sub-section via `DrawSubSectionLabel`): Exclude Boundary Regions checkbox + Boundary Depth slider — currently at [StageSidebar.cpp:1051–1054](../../../src/ui/StageSidebar.cpp#L1051) inside the flat Advanced block.
  - **Advanced** (`ImGui::CollapsingHeader`, default-collapsed):
    - **Optimization** sub-section: Inner Iterations spinbox, Step Damping init/final
    - **Rigidity** sub-section: Rigidity init/final
    - **Control Node Sampling** sub-section: Sampling init/final, Normalize Sampling
- **Reset to Defaults relocation**: currently inside the `Advanced` collapsing header ([line 1058](../../../src/ui/StageSidebar.cpp#L1058)) — therefore hidden when Advanced is collapsed. Move to a dedicated row **below the Advanced section header**, always visible. Reset behavior unchanged: `morph = MorphData(); activePresetId_.clear();`
- **Persisted collapsed state**: extend `Config` ([include/utils/Config.h](../../../include/utils/Config.h)) with a `std::map<std::string, bool> collapsedSections` field serialized as a top-level `ui_state.collapsed_sections` JSON object. Add `Config::GetCollapsedSection(key, defaultValue)` and `Config::SetCollapsedSection(key, bool)`. Bump the existing implicit config version with a `schema_version: 1` stamp at the top of the JSON if the file lacks it (additive — old configs still load via `nlohmann::ordered_json::value(key, default)`). Sidebar reads/writes via three keys: `"morph.general"`, `"morph.advanced"`, `"morph.interior_geometry"`.
- **One-undo-per-gesture** is already handled by the existing `paramEdited` flag at [StageSidebar.cpp:1064](../../../src/ui/StageSidebar.cpp#L1064) (clears `activePresetId_`) and the `MvInspectorPrimitives::DragEditResult::editFinished` hook from Sprint 3.8. Do **not** introduce per-frame undo pushes — that's a regression of the Sprint 3.7 retro lesson.
- **Backward compat**: `MorphData` already has every field this story exposes, with sensible defaults already shipped through the v1.0 backward-compat smoke test (`smoke_v1_backward_compat`). No serializer changes needed.

### Story 5.2: "Cottage Cheese Face" Artifact Investigation (5 pts)

Open-ended bug investigation. The master plan's exit-ramp authorizes a stopgap UI warning if the root cause isn't isolated within the sprint — that ramp is the budget cap.

**Audit-grounded starting points** (from the pre-sprint architect run, saved as agent memory `a96a946a41c618f7f`):
1. Landmark-weight injection at [src/deformation/NRICPDeformer.cpp:764–790](../../../src/deformation/NRICPDeformer.cpp#L764) — `wl` is a flat scalar on landmark rows in `BuildSystemMatrix`. No floor relative to `alpha`. When `alpha` decays to its final value at the last stiffness step, `wl` retains full weight, locally over-constraining the system around landmark vertices. **Strong match for the reported symptom pattern.**
2. BFS boundary exclusion at [NRICPDeformer.cpp:198–286](../../../src/deformation/NRICPDeformer.cpp#L198) — frontier expands uniformly from all boundary edges; UV-seam edges on a closed-but-seamed MetaHuman LOD0 head are treated as open boundaries via the weld map, potentially excluding a wider neighbourhood than intended near seams.
3. Correspondence skipping at [NRICPDeformer.cpp:350](../../../src/deformation/NRICPDeformer.cpp#L350) and [387](../../../src/deformation/NRICPDeformer.cpp#L387) — excluded vertices are skipped from ICP nearest-point search, but **landmark contributions are still injected** for excluded landmark vertices. Unbalanced pull near UV seams.

**Tasks:**
- **Reproduce reliably**: load the bundled MetaHuman head as both source and target with a non-trivial deformation (e.g. nose pulled forward 5cm), place 8 dense correspondence points across a single facial feature (lip cluster), run Quick Wrap. Capture screenshots of the artifact for the retro.
- **Add a one-time diagnostic dump** behind a debug-only env flag (`METAVISAGE_DUMP_RESIDUALS=1`): per-stiffness-step, write a compact CSV of `(vertex_id, residual_magnitude, is_landmark, is_excluded)` to `%TEMP%\metavisage_residuals_<timestamp>.csv`. Lets us see whether residual spikes correlate with landmark vertices vs. boundary-adjacent vertices vs. neither.
- **Hypothesis 1**: Add a `LandmarkWeightFloor` rule to `BuildSystemMatrix` — clamp the effective `wl` to `min(landmarkWeight, alpha * landmarkWeightToStiffnessRatio)`. Test with the reproduction case.
- **Hypothesis 2**: Audit the weld map's effect on `IsBoundaryEdge` — confirm whether UV seams contribute false-positive boundary edges. If yes, exclude welded-edge faces from boundary detection.
- **Outcome A — root cause isolated and fixed**: ship the fix, add a regression assertion to `smoke_wrap_parity` that the dense-landmark reproduction case stays inside a known-good displacement band, document the fix in the sprint outcome notes.
- **Outcome B — root cause partial / open**: implement the master plan's stopgap — a user-visible warning in the Stage 3 inspector when `landmarkWeight / nricpAlphaFinal > threshold` (initial threshold: 0.5, refine empirically). Warning text: `"High landmark weight relative to final stiffness can cause local geometry artifacts. Consider lowering Landmark Weight or raising Stiffness Final."` Use `MvStyle::MvAmber*` palette tokens (already in the design system) — never raw `ImVec4`. Log a follow-up backlog item under High Priority titled "Cottage cheese face — root cause spike".
- **Out of scope**: refactoring `BuildSystemMatrix` or rewriting the boundary-detection BFS. Either is its own sprint.

### Story 5.3: Stage 4 Brush Selector Visual Polish (3 pts)

The brush grid at [StageSidebar.cpp:1230](../../../src/ui/StageSidebar.cpp#L1230) is currently a 2×2 `ImGui::BeginTable` rendering four `DrawPresetCard` calls (Smooth / Grab / Push-Pull / Inflate). It's functional and on-design (uses the same `DrawPresetCard` helper as the Stage 3 preset cards) but ships **no visual brush-effect previews** — the cards rely on Lucide glyphs alone, where the v1.0 feedback specifically asked for sample illustrations.

The architect audit confirmed `assets/brushes/` does not exist. Two viable paths — pick the cheaper one in implementation:

- **Path A — PNG thumbnails (master plan original)**: hand-author 4 small (e.g. 96×96px) brush-effect previews showing a "before / after" surface diagram per brush. Drop them at `assets/brushes/{smooth,grab,pushpull,inflate}_preview.png`. Load via `stb_image` (already a thirdparty dep), upload to GL textures at startup, render via `ImGui::Image` inside `DrawPresetCard`. Adds ~50 lines of texture-loading code to `AppWindow::Initialize` and ~10 lines to `DrawPresetCard` to render an `ImGui::Image` slot when a texture handle is provided.
- **Path B — Procedural mini-renderings**: render a tiny 64×64 ImDrawList preview inside each card showing a stylized "surface bump" or "smoothing curve" that visually conveys the brush effect. No assets needed. ~80 lines of `ImDrawList` math in `DrawPresetCard`.

**Tasks** (Path A is the default — falls back to Path B if asset turnaround stalls):
- Author or commission the four preview PNGs; commit to `assets/brushes/`.
- Extend `DrawPresetCard` ([src/ui/MvInspectorPrimitives.cpp](../../../src/ui/MvInspectorPrimitives.cpp) — confirm location during impl) with an optional `ImTextureID` thumbnail slot rendered above the glyph + label.
- Wire the four brush thumbnails through `AppWindow::Initialize` — load once, store the GL texture handles on `AppWindow`, pass into `RenderTouchUp` per frame.
- **Hotkey badge** addition: on each card, render the keyboard shortcut (`1`/`2`/`3`/`4` or whatever the existing brush switch hotkeys are — confirm in [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp) keyboard handler) in a small badge bottom-right. Reuses `MvStyle::MvFg4` for the badge color.
- **Active highlight consistency check**: confirm the active brush card's highlight uses the same ring/fill treatment as the active Stage 3 preset card — design-system parity is the polish goal.
- **Out of scope**: promoting brush selection to ToolRail. The architect audit confirmed Stage 4 brush selection is not on ToolRail despite earlier sprint docs implying otherwise; that's a separate Stage-aware-ToolRail story for a future sprint, not Stage 4 polish.

### Story 5.4: imgui_test_engine Smoke-UI Lane (5 pts) *(carryover from Sprint 4 — fourth carry)*

Sprint 4 day-1 spike confirmed Path A is license-clear for MetaVisage's commercial profile (<$2M turnover Free License under the upstream Article 1.1 clause). The plan documented in [sprint-04.md § Story 4.5](./sprint-04.md#L75) ports forward verbatim — repeated here for self-containment, with execution-tasks split out:

**Day 1 — Toolchain prep:**
- Add the `test-engine` feature to the `imgui` dependency in [vcpkg.json](../../../vcpkg.json) (currently lists `["glfw-binding", "opengl3-binding", "docking-experimental"]`). Verify `vcpkg install` succeeds locally; commit the resulting `vcpkg-configuration.json` lock changes if any.
- Expose `AppWindow::TickOnce(int fbWidth, int fbHeight)` as a public method that runs one frame of the existing private render loop. The current `Run()` loop ([src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp)) is the body to factor — extract the per-frame work into `TickOnce` and have `Run` call it in a `while (!glfwWindowShouldClose)` loop. Test parity by running the app normally before any harness work.

**Day 2–4 — Harness scaffolding:**
- New `tests/ui_smoke/` directory with `CMakeLists.txt` adds for each scenario.
- New CTest target `smoke_ui` registered with `LABELS ui` so it can be filtered with `ctest -L ui`. Runtime budget: under 60 seconds combined for all five scenarios.
- Hook `imgui_test_engine` against the live `AppWindow::TickOnce()` — pattern is `ImGuiTestEngine_QueueTest(engine, test)` + `ImGuiTestEngine_TickEngine(engine, ctx)`. The engine's `ItemAction("Stage 1/Load morph")` style queries become the scenario building blocks.

**Day 4–5 — Five scenarios:**
1. **Stage 1 → 4 progression** — load default head, advance through stages via the Stage Stepper, assert `Project::GetWorkflowStage()` matches at each step.
2. **Shading-mode + visibility toggles** — click each viewport-toolbar toggle, assert renderer state changes (catches the click-through class of bug from Sprint 3.8.5).
3. **Stage 3 process + Accept** — click Quick Wrap card, click Process Morph, wait for `MorphWorker` completion (timeout 30s), click Accept; assert `morph.deformedMorphMesh.isLoaded == true` and `morph.isAccepted == true`.
4. **Stage 4 brush switch + sculpt + undo** — switch brush, perform a synthetic stroke (BVH-raycasted single-vertex displacement), undo via `Ctrl+Z`; assert mesh state restored to pre-stroke vertices.
5. **Login screen Sign in → Cancel → Sign in cycle** — toggles via the Help-menu flag (Sprint 3.8.12 visual-only login screen).

**Failure surfacing:**
- Screenshot-on-failure to `build/test-output/<test-name>_<timestamp>.png` via `ImGuiTestEngine_CaptureScreenshot`.
- Per-scenario log lines on `MV_LOG_INFO` stream so CI / human eyeball can correlate failures.

**Sprint 4 retro accountability**: Action Item #1 is due **Sprint 5 close**. If any of the five scenarios fail to drive cleanly via `imgui_test_engine` even after the toolchain is in place, ship the four that work and log the remainder as a Sprint 6 carryover with a specific blocker description — but the lane itself (target + at least 3 scenarios) must ship this sprint. **Fifth carry is not an option.**

### Story 5.5: Refine `/sprint-retrospective` Gate Bypass (1 pt) *(carryover from Sprint 4 — retro AI #2)*

Sprint 4's mechanical DoD gate (Story 4.7, [c72e6e7](https://github.com/cmershon2/MetaVisage/commit/c72e6e7)) fires only when Status is `Complete` or `Implementation Complete`. Sprint 4's own retro surfaced the bypass: a user invoking `/sprint-retrospective` with Status left at `Planned` or `In Progress` slips the gate even when validation is genuinely pending.

- Edit [.claude/skills/sprint-retrospective/SKILL.md](../../../.claude/skills/sprint-retrospective/SKILL.md) to add a second clause to the pre-flight check: if Status is `Planned` or `In Progress` **and** every committed story in the Sprint Backlog table is checked `[x]` (i.e. all-shipped, "validation pending" pattern), the gate also fires — same DoD checklist scan, same refusal message naming the unsatisfied items.
- Distinguish from the legitimate ad-hoc-retro path: ad-hoc retros on partially-complete sprints (some `[ ]` items still open) continue to fall through.
- Sanity test: run `/sprint-retrospective` on a copy of `sprint-04.md` with Status flipped to `In Progress` and all stories marked `[x]` but DoD `PR opened` unchecked — expect refusal. Same sprint with Story 4.5 left as `[/]` (ad-hoc mid-sprint) — expect fall-through.

---

## Sprint Context (Key Affected Files)

**Story 5.1 (Stage 3 collapsible hierarchy)**
- [src/ui/StageSidebar.cpp:900–1190](../../../src/ui/StageSidebar.cpp#L900) — `RenderMorph` restructure
- [include/ui/MvInspectorPrimitives.h](../../../include/ui/MvInspectorPrimitives.h) / [src/ui/MvInspectorPrimitives.cpp](../../../src/ui/MvInspectorPrimitives.cpp) — new `DrawSubSectionLabel` helper
- [include/utils/Config.h](../../../include/utils/Config.h) / [src/utils/Config.cpp](../../../src/utils/Config.cpp) — `collapsedSections` field + getters/setters; JSON `ui_state.collapsed_sections` block
- [include/ui/StageSidebar.h](../../../include/ui/StageSidebar.h) — only if collapsed-state caching needs a member field (otherwise read straight from `Config`)

**Story 5.2 (cottage-cheese investigation)**
- [src/deformation/NRICPDeformer.cpp:198–286](../../../src/deformation/NRICPDeformer.cpp#L198) — boundary-exclusion BFS
- [src/deformation/NRICPDeformer.cpp:350, 387](../../../src/deformation/NRICPDeformer.cpp#L350) — correspondence skipping path
- [src/deformation/NRICPDeformer.cpp:764–790](../../../src/deformation/NRICPDeformer.cpp#L764) — landmark-weight injection in `BuildSystemMatrix`
- [src/ui/StageSidebar.cpp:RenderMorph](../../../src/ui/StageSidebar.cpp#L900) — Outcome B warning row, after the Vertex Masking block
- New (Outcome A only): regression case in [tests/smoke/wrap_parity.cpp](../../../tests/smoke/wrap_parity.cpp) for the dense-landmark reproduction

**Story 5.3 (Stage 4 brush polish)**
- [src/ui/StageSidebar.cpp:1196–1313](../../../src/ui/StageSidebar.cpp#L1196) — `RenderTouchUp` brush-grid wiring
- [src/ui/MvInspectorPrimitives.cpp](../../../src/ui/MvInspectorPrimitives.cpp) — extend `DrawPresetCard` with an `ImTextureID` thumbnail slot
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — load brush PNGs at init, store GL texture handles
- New: `assets/brushes/{smooth,grab,pushpull,inflate}_preview.png` (~96×96 each), or Path B's procedural rendering

**Story 5.4 (imgui_test_engine smoke)**
- [vcpkg.json](../../../vcpkg.json) — add `test-engine` feature on the `imgui` dep
- [include/ui/AppWindow.h](../../../include/ui/AppWindow.h) / [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — public `TickOnce(int fbWidth, int fbHeight)`
- [CMakeLists.txt](../../../CMakeLists.txt) — new `smoke_ui` CTest target with `LABELS ui`
- New: `tests/ui_smoke/` directory with five scenario drivers (one `.cpp` per scenario)

**Story 5.5 (sprint-retrospective gate refinement)**
- [.claude/skills/sprint-retrospective/SKILL.md](../../../.claude/skills/sprint-retrospective/SKILL.md) — add second gate clause

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Story 5.2's root cause is genuinely deeper than the three audit-flagged hypotheses (e.g. an arnoldi-iteration numerical issue, or a subdivide-and-conquer bug in `BuildSystemMatrix`'s sparsity pattern). Sprint runs out of investigation budget at 5 pts. | **Med** | **Med** | The master plan's stopgap-warning exit ramp is the budget cap. Outcome B ships the warning + a backlog item titled "Cottage cheese face — root cause spike" under High Priority. The diagnostic dump (`METAVISAGE_DUMP_RESIDUALS=1`) survives even if no fix lands, accelerating the follow-up spike. |
| `imgui_test_engine` Path A integration runs into the same fragility class that tied up Sprint 3.8 (vcpkg DLL/feature interactions). Toolchain quicksand consumes Days 1–2 with no scenarios shipped. | **Low** | **High** | Day-1 spike on Sprint 4 already confirmed the feature exists in vcpkg and the license clears. If `vcpkg install imgui[test-engine]` does not produce a working library on the first attempt, fall back to a hand-rolled GLFW-input driver (Path B from sprint-04.md plan) — adds ~150 lines of harness, costs the same 5 points, but guarantees the lane exists. **Fifth carry is not an option** per Sprint 4 retro AI #1. |
| Story 5.1's `Config` JSON schema extension (`ui_state.collapsed_sections`) accidentally breaks existing user configs at `%APPDATA%\MetaVisage\config.json` — Sprint 3.6's config layer has been silently working for two sprints and no one wants to debug "settings reset" reports. | **Low** | **Med** | Use `nlohmann::ordered_json::value(key, default)` everywhere — reads of `ui_state.collapsed_sections` on a v1 config without that key fall through to the empty map silently. Add a single-line regression test inside the existing `Config` smoke (or a new `smoke_config_backcompat` if none exists) that loads a fixture v0 config and asserts no fields are dropped. |
| Story 5.3 Path A stalls on asset authorship — the four brush-effect PNGs are easy in concept but consume open-ended design time. | **Med** | **Low** | Path B (procedural mini-renderings) is the explicit fallback documented in the story; ~80 lines of `ImDrawList` math, no assets. Make the Path A vs B decision on Day 8 — if the PNGs aren't committed by then, switch. |
| Story 5.5's gate refinement introduces a new false-positive case — e.g. user has all stories `[x]` with Status=Planned because they're about to flip Status but want to retro first. | **Low** | **Low** | The new clause's refusal message includes guidance: *"All stories shipped; Status is `Planned` but DoD items pending. Either flip Status to `Complete` and tick boxes, or set a story's status to `[/]` if work is genuinely in progress."* The bypass is intentional in the legitimate-ad-hoc case (`[/]` story). |
| Cumulative scope: 22 points in a 28-point capacity feels conservative, but Sprint 5 adds the `imgui_test_engine` integration on top of three product stories + investigation, and the test-engine carryover has been famously sticky across four sprints. | **Low** | **Low** | Buffer is deliberate. If 5.4 lands cleanly by Day 6, the slack absorbs Story 5.2 going from Outcome B to Outcome A (root-cause fix instead of stopgap), or pulls in a Medium-priority backlog item (texture thumbnail preview modal, ~3 pts). Don't pre-commit — surface as a Day-6 decision. |

---

## Definition of Done

- [ ] All committed stories implemented and merged to the sprint branch
- [ ] Build clean: `cmake --build . --config Release` succeeds with no new warnings (the three pre-existing C5054 warnings in the dockspace flag-combine code carry over from Sprint 3.6 unchanged — out of scope here, same as Sprints 3.8 and 4)
- [ ] CTest green: `ctest -C Release` passes the existing four targets (`smoke_load_serialize_export`, `smoke_v1_backward_compat`, `smoke_wrap_parity`) plus the new `smoke_ui` lane. `smoke_ui` runs in under 60s combined; existing targets unchanged in runtime.
- [ ] Stage 3 inspector shows Vertex Masking always-visible at top, **General** collapsible-default-open with **Interior Geometry** sub-section nested under it, **Advanced** collapsible-default-collapsed with three sub-sections (Optimization / Rigidity / Control Node Sampling). All eight existing NRICP params + the boundary-exclusion controls render under their correct sub-sections.
- [ ] Reset to Defaults button visible regardless of whether Advanced is expanded; clicking it fully resets parameters and clears the active preset highlight.
- [ ] Collapsed/expanded state for each section persists across app sessions — verified by collapsing General + opening Advanced, restarting, confirming state restored.
- [ ] v0 `%APPDATA%\MetaVisage\config.json` files (without `ui_state.collapsed_sections`) load without crash or warning; missing keys fall back to defaults.
- [ ] Cottage-cheese-face investigation has produced **either** a root-cause fix landed with a regression case in `smoke_wrap_parity` **or** a documented stopgap warning in the Stage 3 inspector + a High-priority backlog item filed for the follow-up spike. Sprint outcome notes name which path landed.
- [ ] Stage 4 brush selector renders visible brush-effect previews (PNG thumbnails or procedural) in each card; active card is consistently highlighted; hotkey badges visible.
- [ ] `vcpkg.json` declares `imgui[test-engine]`; `AppWindow::TickOnce()` is a public method exposed in the header; `smoke_ui` CTest target exercises **at least four** of the five scenarios (target is five — log the gap if any scenario blocks).
- [ ] `/sprint-retrospective` skill refuses to run on a sprint with all stories `[x]` and Status `In Progress`/`Planned` if any DoD items containing `PR opened`/`/review-pr`/`manual UI smoke`/`Phase D` are unchecked. Manual sanity test on a copy of `sprint-05.md`.
- [ ] PR opened against `development` and `/review-pr` agent run; **all blocking findings resolved**
- [ ] Phase D manual UI smoke completed on Windows — Stage 1 → 2 → 3 → 4 → Export end-to-end with the bundled MetaHuman head, plus: collapsible sections persist across restart; cottage-cheese reproduction case behaves as the chosen Outcome (A or B) describes; brush previews render in Stage 4; `smoke_ui` lane runs from `ctest -L ui` and produces a screenshot on a deliberately broken scenario for failure-surfacing verification. **Plus** Undo/Redo verification on point placement, gizmo drag, mask paint, and sculpt brush.
- [ ] Sprint outcome notes updated with actual velocity, carryover (or "None"), retrospective link, and — if Outcome B on 5.2 — the High-priority backlog item link.

> **Mechanical guard (Sprint 4 Action Item #3):** the `/sprint-retrospective`
> skill refuses to run when this sprint's `Sprint Outcome` Status is set to
> `Complete` and any DoD item containing `"PR opened"`, `"/review-pr"`,
> `"manual UI smoke"`, or `"Phase D"` is still unchecked. Story 5.5 of *this*
> sprint also closes the bypass: the gate fires when all stories are `[x]`
> even if Status is left as `In Progress` or `Planned`. Don't flip Status to
> `Complete` until those boxes are genuinely ticked — and don't try to slip
> past it via the Status field.

---

## Sprint Outcome
**Status**: Planned
**Velocity (Actual)**: TBD
**Carryover**: TBD
**Notes**: TBD
