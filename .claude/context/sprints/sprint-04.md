# Sprint 4: Wrap Engine Redesign — Algorithm & Preset System

**Sprint Goal**: Retire the RBF algorithms, scrub every public reference to the underlying algorithm in favour of "Wrap Engine", lock the built-in Quick Wrap / Fantasy Wrap presets behind a real `WrapPreset` data model, and ship the user-saved preset workflow that the Sprint 3.8.9 "+ Save preset" card has been holding the door open for.
**Duration**: 2026-04-29 — 2026-05-12 (2 weeks, 10 working days)
**Capacity**: 28 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor — slight step down from Sprint 3.8's 32 to absorb the higher risk profile of algorithm-adjacent work and the three retro action items folded in)
**Committed**: 27 points

> Scope is set by [SPRINT_PLAN.md § Sprint 4](./SPRINT_PLAN.md) (Stories 4.1–4.4) plus the three Action Items carried from [retro-sprint-03.8.md](../retrospectives/retro-sprint-03.8.md). The visual preset card row already shipped in Sprint 3.8.9 (`b6fa618` / `3e80734`) — its inline `ApplyMorphPreset(MorphData&, int)` helper is the **temporary scaffold** Sprint 4 replaces. Sprint 3.8.9 left a load-bearing `// Sprint 4 (Wrap Engine Redesign) will replace this with persisted user presets` comment at [src/ui/StageSidebar.cpp:769](../../../src/ui/StageSidebar.cpp#L769).

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 4.1: Remove RBF Algorithms & Rebrand to Wrap Engine** | 5 | None — load-bearing for 4.2 (canonical algorithm field) | [ ] |
| 2 | **Story 4.2: WrapPreset Data Model & Built-In Presets** | 3 | Story 4.1 | [ ] |
| 3 | **Story 4.3: Preset Card Wiring through WrapPreset** | 2 | Story 4.2 | [ ] |
| 4 | **Story 4.4: User-Saved Wrap Presets (Save / Rename / Delete)** | 8 | Story 4.2 | [ ] |
| 5 | **Story 4.5: imgui_test_engine Smoke-UI Lane** *(retro AI #1)* | 5 | None | [ ] |
| 6 | **Story 4.6: Behavioural-Parity Wrap Smoke Target** *(retro AI #2)* | 3 | Story 4.2 | [ ] |
| 7 | **Story 4.7: Mechanical DoD Enforcement in /sprint-retrospective** *(retro AI #3)* | 1 | None | [ ] |

> **Critical-path**: 4.1 → 4.2 unblocks 4.3 / 4.4 / 4.6. 4.5 and 4.7 are independent and can slot wherever capacity opens up; 4.7 is the natural sprint-kickoff task per the retro action item.

---

## Story Detail

### Story 4.1: Remove RBF Algorithms & Rebrand to Wrap Engine (5 pts)

The RBF interpolators predate NRICP and are no longer reachable from the UI (Sprint 3.8.9's preset cards replaced the algorithm dropdown). Remove the dead code and scrub user-visible references to the underlying algorithm in favour of "Wrap Engine".

- **Code removal**: delete [include/deformation/RBFInterpolator.h](../../../include/deformation/RBFInterpolator.h), [src/deformation/RBFInterpolator.cpp](../../../src/deformation/RBFInterpolator.cpp), and the corresponding entries in [CMakeLists.txt](../../../CMakeLists.txt). Strip `RBF_TPS` / `RBF_GAUSSIAN` / `RBF_MULTIQUADRIC` from the `DeformationAlgorithm` enum at [include/core/Project.h:71](../../../include/core/Project.h#L71). Update the algorithm switch in [src/deformation/MeshDeformer.cpp](../../../src/deformation/MeshDeformer.cpp) so `MorphData::algorithm` is no longer a polymorphism dispatcher — it can stay as a single-valued enum or collapse to `bool` (TBD in implementation; preserve serializer roundtrip either way).
- **Project file backward compatibility**: [src/io/ProjectSerializer.cpp:459](../../../src/io/ProjectSerializer.cpp#L459) reads `algorithm` as `int`. Any value (0=RBF_TPS, 1=RBF_GAUSSIAN, 2=RBF_MULTIQUADRIC, 3=NRICP) must coerce to the surviving enum value silently — v1.0.0 `.mmproj` files with `algorithm: 0` must load without warning. Continue writing the field for forward compat. Add a regression test that loads a fixture v1.0.0 `.mmproj` from `tests/fixtures/` containing each historical enum value.
- **User-visible string scrub**: grep every UI-rendered string and user-shown log message for "NRICP", "Non-Rigid ICP", "Iterative Closest Point", "RBF", "TPS", "Gaussian", "Multiquadric". Confirmed surfaces today: [src/ui/StageSidebar.cpp:41](../../../src/ui/StageSidebar.cpp#L41), [src/ui/AppWindow.cpp:20](../../../src/ui/AppWindow.cpp#L20). Any user-facing string (status bar messages, MorphWorker progress text, error dialogs, tooltip copy, the export modal) must read "Wrap Engine" or omit the algorithm name entirely. **Internal code identifiers (class names like `NRICPDeformer`, `nricpAlphaInitial`) stay** — the rule is surface, not source.
- **Comment audit**: leave internal `NRICP` references in `// comment` form unchanged where they document mathematical intent; they're not user-visible. Sprint docs and retros stay untouched.

### Story 4.2: WrapPreset Data Model & Built-In Presets (3 pts)

Replace the inline `ApplyMorphPreset(MorphData&, int presetIdx)` scaffold at [src/ui/StageSidebar.cpp:774](../../../src/ui/StageSidebar.cpp#L774) with a proper data model that the user-preset persistence in Story 4.4 can serialize.

- New `include/core/WrapPreset.h` / `src/core/WrapPreset.cpp` with a `WrapPreset` struct: `std::string id`, `std::string name`, `std::string description`, `std::string iconGlyph` (Lucide PUA codepoint string from `MvIcons.h`), `bool isBuiltIn`, `MorphData::WrapParams` (or the existing parameter subset of `MorphData`). The struct is the snapshot — applying it overwrites the project's `MorphData` parameter fields verbatim, leaving non-parameter fields (mesh refs, deformedMorphMesh, etc.) untouched.
- Free-function or static method `WrapPreset::ApplyTo(MorphData&) const` — replaces the existing inline namespace helper.
- Built-in registry: `WrapPreset::BuiltIns()` returns a `std::vector<WrapPreset>` containing exactly two entries:
  - **Quick Wrap** — `id = "quick"`, defaults from `MorphData()` constructor *plus* `nricpGammaInitial = 1.0f` and `nricpGammaFinal = 1.0f` (per CLAUDE.md and the comment block at [StageSidebar.cpp:755](../../../src/ui/StageSidebar.cpp#L755)). Default-selected on new projects.
  - **Fantasy Wrap** — `id = "fantasy"`, identical to `MorphData()` constructor defaults. The historical "current behaviour" preset.
- Built-in presets are read-only: `isBuiltIn = true`, save/rename/delete UI paths refuse to mutate them.
- The semantics are locked by the Sprint 3.8.9 retro lesson: design files are not algorithm ground truth. Any visual mockup that suggests numerical parameters for built-ins is overridden by `MorphData()` defaults + the rigidity override.

### Story 4.3: Preset Card Wiring through WrapPreset (2 pts)

The Sprint 3.8.9 card row UI keeps its layout and visuals; only the data binding changes from "selected index" to "selected preset id".

- `StageSidebar` keeps a `std::string activePresetId_` instead of `int lastAppliedPreset_` ([src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp)). Active highlighting compares `id` strings.
- Built-in cards iterate `WrapPreset::BuiltIns()` and render via the existing `DrawPresetCard()` helper — no new drawing code. The helper already takes a glyph string parameter; pass `preset.iconGlyph`.
- Click handler calls `preset.ApplyTo(project.GetMorphData())` and sets `activePresetId_ = preset.id`. The "+ Save preset" card stays visible at the end of the row but is **enabled** in this sprint (Story 4.4 wires its callback).
- On project load, derive `activePresetId_` from the loaded `MorphData` parameter values: if they exactly match a built-in preset, mark that as active; otherwise leave `activePresetId_` empty (no card highlighted) — this signals "user has tweaked parameters since last preset apply".
- Param-edit detection: any change to a preset-controlled `MorphData` field via the inspector clears `activePresetId_` (because the project's parameters now diverge from the preset). Reuse the `MvInspectorPrimitives::DragEditResult::editFinished` hook so this fires once per gesture, not per frame.

### Story 4.4: User-Saved Wrap Presets — Save / Rename / Delete (8 pts)

The largest story in the sprint. Net-new persistence layer + three modal dialogs + context menu wiring.

- **Persistence**: new `include/io/PresetStore.h` / `src/io/PresetStore.cpp`. `PresetStore::UserPresetsDir()` resolves to `%APPDATA%\MetaVisage\presets\` via `Paths::AppDataDir()` (extend `Paths` if it doesn't expose APPDATA root yet — Sprint 3.6 added `%APPDATA%\MetaVisage\config.json` so the resolver exists; reuse it). Each user preset is a single JSON file `<safe-name>.mvpreset.json` with `schema_version: 1`, `id` (UUID-like), `name`, `description`, `iconGlyph`, `created_at` (ISO 8601 via `std::chrono`), and the `MorphData` parameter snapshot. nlohmann::ordered_json for stable diffs.
- **Schema version**: `schema_version: 1` on every file from day one — future-us will need it. Loader rejects unknown major versions with a logged warning, not a crash.
- **Save dialog** (new in [src/ui/Modals.cpp](../../../src/ui/Modals.cpp)): title "Save preset", a name field (required, validated against existing user preset names), an optional description field, and a Lucide-glyph picker grid (12 hand-curated options from `MvIcons.h`: target / sparkles / star / heart / circle / square / etc.). Confirm/Cancel footer. Saves to `PresetStore` and re-keys the active card to the new preset.
- **Card list**: render user presets after the two built-ins in the existing card row, before the "+ Save preset" card. The 3.8.9 row used a fixed 3-column `BeginTable`; rework to a horizontally scrolling region (`ImGui::BeginChild` + `SetNextWindowContentSize`) so the user preset list grows without breaking the layout. **Confirm with the design team** that horizontal scroll is acceptable before implementing — fallback is wrap-to-next-row.
- **Right-click context menu**: `ImGui::BeginPopupContextItem` on each user preset card. Options: **Rename...**, **Edit description...**, **Replace with current parameters**, **Delete**. Built-in cards get no context menu.
- **Rename dialog**: same modal harness as Save, prefilled with current name; validates against name collisions across user presets.
- **Delete confirmation**: existing Sprint 3.6 confirm modal (`Modals::OpenConfirm(...)`) — text "Delete preset '<name>'? This cannot be undone."
- **Default selection on new projects**: extend the existing new-project init path so `MorphData::WrapParams` are populated from the Quick Wrap built-in (Story 4.2 already enforces this on the data side; this is just confirming the entry point applies it).
- **Out of scope this sprint** — preset import/export across machines (Sprint 5+ candidate, log a backlog item if it surfaces during dogfooding); preset thumbnails as PNGs (Lucide glyphs replace them, consistent with the 3.8 design system).

### Story 4.5: imgui_test_engine Smoke-UI Lane (5 pts) *(retro AI #1)*

Carried action item from Sprint 3.7 retro and Sprint 3.8 retro — the click-through bug, the preset-values bug, and the login caption-button bug from 3.8 would all have been caught by an interactive test driver. **This sprint builds it.**

- **Spike on day 1**: confirm `imgui_test_engine` is installable via vcpkg or available as a submodule under a license compatible with proprietary distribution (MIT). If unavailable, fall back to a hand-rolled driver using `glfwSetCursorPos` + synthetic GLFW input events + frame stepping (`AppWindow::TickOnce()` exposed for tests). The hand-rolled fallback is ~150 lines of harness code; budget the 5 points either way.
- **New CTest target** `smoke_ui` runnable via `ctest -C Release -L ui`. Headless mode where possible; OS window allowed if `imgui_test_engine` requires it (CI is not a constraint — this runs locally on Windows).
- **Five scenarios** covering the workflow contact surface, per the Sprint 3.8 retro:
  1. Stage 1 → 4 progression — load default head, advance through stages via the Stage Stepper, assert active stage state at each step.
  2. Shading-mode + visibility toggles in the viewport toolbar — click each, assert renderer state changes (catches the click-through class of bug from 3.8.5).
  3. Stage 3 process + Accept — click Quick Wrap, click Process, wait for completion (with timeout), click Accept; assert `MorphData::deformedMorphMesh` is populated.
  4. Stage 4 brush switch + sculpt + undo — switch brush, perform a synthetic stroke, undo; assert mesh state restored.
  5. Login screen Sign in → Cancel → Sign in cycle — toggles via the Help-menu flag (Sprint 3.8.12 visual-only login screen).
- All five scenarios run in under 60 seconds combined; failure surfaces a screenshot under `build/test-output/` for diagnosis.

### Story 4.6: Behavioural-Parity Wrap Smoke Target (3 pts) *(retro AI #2)*

Catches the class of bug Sprint 3.8.9 shipped first — preset numbers that look reasonable in the inspector but produce a visibly broken wrap.

- New CTest target `smoke_wrap_parity` (separate from `smoke_ui` so it can run without GUI infrastructure). Loads the bundled MetaHuman head from [assets/models/](../../../assets/models/) as both source and target with a small, deterministic transform offset (e.g. 0.5cm Y-translation), places a fixture set of 4 correspondence points, runs the wrap, and asserts:
  - `MorphData::result.maxDisplacement` falls within a known-good band per preset (Quick Wrap: tighter band; Fantasy Wrap: wider band — exact bands captured during the Story 4.6 implementation by running the current pipeline and recording its output ±5%).
  - Wrap converges (no `result.failed` flag).
  - Output vertex count == input vertex count (catches retopology regressions).
- Target runtime budget: under 30 seconds for both presets combined.
- The fixture lives at `tests/fixtures/wrap_smoke_points.json` — 4 manually-placed correspondence points in MorphData JSON form.

### Story 4.7: Mechanical DoD Enforcement in /sprint-retrospective (1 pt) *(retro AI #3)*

Three sprints in a row have set "Implementation Complete" before the manual smoke + `/review-pr` lane closed. The DoD template improvements from Sprint 3.7 / 3.8 didn't change the discipline. This story enforces it mechanically.

- Edit `.claude/skills/sprint-retrospective/SKILL.md` to add a pre-flight check at the start of the skill: read the active sprint file's "Sprint Outcome" section, parse the Definition of Done checklist, and refuse to proceed if any of these items are unchecked: PR opened, `/review-pr` blockers resolved, manual UI smoke logged. Bail with a clear message naming the unsatisfied DoD items.
- Add a one-line note in the sprint-planner skill's DoD template that mirrors this: "the /sprint-retrospective skill will refuse to run until the boxes above are checked".
- Sanity test: run `/sprint-retrospective` against `sprint-04.md` with the Status set to "In Progress" — expect it to refuse. Set Status to "Complete" with all DoD boxes checked — expect it to proceed.

---

## Sprint Context (Key Affected Files)

**Story 4.1 (RBF removal + rebrand)**
- DELETE: [include/deformation/RBFInterpolator.h](../../../include/deformation/RBFInterpolator.h), [src/deformation/RBFInterpolator.cpp](../../../src/deformation/RBFInterpolator.cpp)
- [include/core/Project.h:71–74](../../../include/core/Project.h#L71) — strip RBF enum values; preserve `NRICP` as the surviving member
- [src/io/ProjectSerializer.cpp:173, 284, 459–460](../../../src/io/ProjectSerializer.cpp#L173) — accept legacy enum int values, default to surviving member
- [src/deformation/MeshDeformer.cpp:26](../../../src/deformation/MeshDeformer.cpp#L26) — algorithm dispatch collapses to single path
- [src/deformation/NRICPDeformer.cpp:57](../../../src/deformation/NRICPDeformer.cpp#L57) — internal `// NRICP` comments stay; user-visible log strings ("Running NRICP...") become "Running Wrap Engine..."
- [CMakeLists.txt:6](../../../CMakeLists.txt#L6) — remove `RBFInterpolator.cpp` from sources
- [src/ui/StageSidebar.cpp:41](../../../src/ui/StageSidebar.cpp#L41), [src/ui/AppWindow.cpp:20](../../../src/ui/AppWindow.cpp#L20) — user-visible string scrub
- New: `tests/fixtures/v1_project_rbf_tps.mmproj`, `..._gaussian.mmproj`, `..._multiquadric.mmproj` for the backward-compat regression

**Story 4.2 (WrapPreset data model)**
- New: [include/core/WrapPreset.h](../../../include/core/WrapPreset.h), [src/core/WrapPreset.cpp](../../../src/core/WrapPreset.cpp)
- [src/ui/StageSidebar.cpp:755–810](../../../src/ui/StageSidebar.cpp#L755) — delete the inline `ApplyMorphPreset()` namespace helper
- [CMakeLists.txt](../../../CMakeLists.txt) — add WrapPreset.cpp to sources

**Story 4.3 (preset card wiring)**
- [src/ui/StageSidebar.cpp:898–950](../../../src/ui/StageSidebar.cpp#L898) — `RenderMorph` rebind from `lastAppliedPreset_` int to `activePresetId_` string; iterate built-ins via `WrapPreset::BuiltIns()`
- [include/ui/StageSidebar.h](../../../include/ui/StageSidebar.h) — replace `int lastAppliedPreset_` with `std::string activePresetId_`

**Story 4.4 (user-saved presets)**
- New: [include/io/PresetStore.h](../../../include/io/PresetStore.h), [src/io/PresetStore.cpp](../../../src/io/PresetStore.cpp)
- [include/utils/Paths.h](../../../include/utils/Paths.h) / [src/utils/Paths.cpp](../../../src/utils/Paths.cpp) — confirm `AppDataDir()` exposes `%APPDATA%\MetaVisage\` root; extend if currently scoped to `config.json` only
- [src/ui/Modals.cpp](../../../src/ui/Modals.cpp) / [include/ui/Modals.h](../../../include/ui/Modals.h) — new save / rename / delete-confirm modals
- [src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp) — user preset card rendering, context menu wiring, scroll region
- [CMakeLists.txt](../../../CMakeLists.txt) — add PresetStore.cpp

**Story 4.5 (imgui_test_engine smoke)**
- [vcpkg.json](../../../vcpkg.json) — add `imgui[test-engine]` if available
- [CMakeLists.txt](../../../CMakeLists.txt) — new `smoke_ui` CTest target with `add_test(NAME smoke_ui ... LABELS ui)`
- New: `tests/ui_smoke/` directory with the five scenario drivers
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — expose a `TickOnce()` entry point if hand-rolled fallback is needed

**Story 4.6 (behavioural smoke)**
- New: `tests/smoke_wrap_parity.cpp`, `tests/fixtures/wrap_smoke_points.json`
- [CMakeLists.txt](../../../CMakeLists.txt) — new `smoke_wrap_parity` CTest target

**Story 4.7 (sprint-retrospective enforcement)**
- [.claude/skills/sprint-retrospective/SKILL.md](../../../.claude/skills/sprint-retrospective/SKILL.md) — add pre-flight DoD check
- [.claude/skills/sprint-planner/SKILL.md](../../../.claude/skills/sprint-planner/SKILL.md) — one-line note in DoD template

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `imgui[test-engine]` is unavailable in vcpkg or has a license incompatible with the v2.0 proprietary distribution | **Med** | **Med** | Day 1 spike: confirm availability and license. Fallback is a hand-rolled driver using GLFW input synthesis + an exposed `AppWindow::TickOnce()` — adds ~150 lines but unlocks the same 5 scenarios. Budget for Story 4.5 covers either path. If the hand-rolled fallback also blocks (e.g. GLFW input simulation flakes on Windows), defer to Sprint 5 and note in retro — this is the third sprint this is being attempted. |
| RBF removal silently breaks v1.0.0 `.mmproj` files where `algorithm` is one of the discontinued enum ints (0/1/2) | **Low** | **High** | The regression test in Story 4.1 loads three fixture v1.0.0 project files (one per discontinued enum value) and asserts they open without warning. Coerce any unrecognized algorithm int silently to the surviving NRICP member — never reject the file, never surface "your project used an old algorithm" to the user (Story 4.1's rebrand explicitly hides the algorithm name). |
| `WrapPreset` data model + user-preset persistence introduces a JSON schema we'll have to live with — schema mistakes here will haunt every future sprint that touches preset behaviour | **Med** | **Med** | Stamp `schema_version: 1` on every preset file from the first commit. Document the schema in a top-of-file comment block in `PresetStore.cpp`. Loader rejects unknown major versions with a logged warning, not a crash. `nlohmann::ordered_json` so diffs stay stable. |
| User preset list grows unbounded (no cap) and breaks the horizontally-scrolling card row at 50+ presets | **Low** | **Low** | Document the soft limit (~20 user presets visible without significant performance cost) in `PresetStore`'s header comment; add a runtime warning at >50 presets. Cap is informational, not enforced — the scroll region degrades gracefully. |
| Story 4.5 `imgui_test_engine` adds a new toolchain dependency mid-sprint that the smoke test currently avoids — could re-introduce the GLEW / vcpkg DLL fragility Sprint 3.8 worked around | **Low** | **Med** | Land Story 4.5 in a separate commit chain on a side branch first; merge into the sprint branch only after both `smoke_load_serialize_export` (existing) and `smoke_ui` (new) run green from a clean configure. If the integration breaks the existing smoke, revert and switch to the hand-rolled fallback. |
| The "rebrand" scrub in Story 4.1 is fuzzy — easy to miss a status string, log line, or tooltip that surfaces "NRICP" to a user. The grep returns 347 hits across 24 files; not all are user-visible | **Med** | **Low** | Define the exact user-visible surface in Story 4.1 implementation: any string that reaches a `Logger` sink the user can see (status bar, modal text, file dialog filter, error popup), any `MV_LOG_*` call rendered to the in-app log panel, any tooltip, any sidebar/menu/toolbar label. Internal class names (`NRICPDeformer`), member variables (`nricpAlphaInitial`), file names, and code comments are explicitly out of scope. Add a `// TODO(rebrand)` marker on any string the implementer is unsure about; review before sprint close. |
| Story 4.7's mechanical DoD enforcement in `/sprint-retrospective` could lock the user out of running retros legitimately during sprint mid-points or for ad-hoc retros on partially-completed work | **Low** | **Low** | The pre-flight check looks at the **active sprint file** — if Status is "Complete" the boxes must all be checked; if Status is "In Progress" or "Planned" the check is skipped. Ad-hoc retros on incomplete sprints proceed as before. The block fires only on the failure case it's designed to prevent: Status flipped to "Complete" with unchecked DoD boxes. |
| Removing the algorithm dropdown is in the master plan but Sprint 3.8.9 already dropped it (preset cards replaced it). Rewriting story 4.1 around "removing what's already gone" wastes sprint capacity | **Low** | **Low** | The current Story 4.1 acknowledges this: the dropdown task is logged as "no-op, already done". The remaining 4.1 work is RBF code removal + string rebrand + serializer compat — all real. Story budget already discounted accordingly (5 pts, not 8). |
| Preset card row mid-sprint redesign (Story 4.4 horizontal scroll) is a scope risk if user preset count grows during dogfooding and the wrap-to-next-row fallback turns out to look better | **Low** | **Low** | Capture screenshots of both layouts (3 user presets, 7 user presets, 15 user presets) once Story 4.4 is functionally complete; pick the better-looking option in the PR. Don't get stuck on this — both options take <30 lines of layout code. |

---

## Definition of Done

- [ ] All committed stories implemented and merged to the sprint branch
- [ ] Build clean: `cmake --build . --config Release` succeeds with no new warnings (the three pre-existing C5054 warnings in the dockspace flag-combine code carry over from Sprint 3.6 unchanged — out of scope here, same as Sprint 3.8)
- [ ] CTest green: `ctest -C Release` passes (existing `smoke_load_serialize_export` + new `smoke_wrap_parity` + new `smoke_ui` lane). Five `smoke_ui` scenarios run in under 60s combined; `smoke_wrap_parity` runs in under 30s.
- [ ] `RBFInterpolator.h` / `.cpp` are gone; `RBF_TPS` / `RBF_GAUSSIAN` / `RBF_MULTIQUADRIC` no longer appear in `DeformationAlgorithm`; v1.0.0 fixture project files load without warning
- [ ] No user-visible string contains "NRICP", "Non-Rigid ICP", "Iterative Closest Point", "RBF", "TPS", "Gaussian", or "Multiquadric" anywhere in the running application — verified by grep on UI source files filtered to user-rendered strings, plus a manual spot check of status bar, sidebar tooltips, MorphWorker progress text, and the export modal
- [ ] `WrapPreset` struct exists in `include/core/WrapPreset.h`; built-in registry returns exactly two presets (Quick Wrap, Fantasy Wrap); both are read-only; `WrapPreset::ApplyTo(MorphData&)` replaces the inline `ApplyMorphPreset()` helper
- [ ] Preset card row binds `activePresetId_` (string) instead of `lastAppliedPreset_` (int); param edits via inspector clear the active preset highlight; project load derives the active preset by parameter equality
- [ ] User can save the current parameters as a named preset via the "+ Save preset" card; user presets persist to `%APPDATA%\MetaVisage\presets\` as JSON files with `schema_version: 1`; user presets render in the card row; right-click context menu offers Rename / Edit description / Replace / Delete
- [ ] `smoke_ui` CTest target exists and exercises all five scenarios from Story 4.5
- [ ] `smoke_wrap_parity` CTest target exists and asserts displacement bands per preset
- [ ] `/sprint-retrospective` skill refuses to run on a sprint with Status = Complete and unchecked DoD boxes (manual sanity test on a copy of `sprint-04.md`)
- [ ] PR opened against `development` and `/review-pr` agent run; **all blocking findings resolved**
- [ ] Phase D manual UI smoke completed on Windows — Stage 1 → 2 → 3 → 4 → Export end-to-end with the bundled MetaHuman head, plus: Quick Wrap → Process → Accept produces a tight wrap; Fantasy Wrap → Process → Accept produces a looser wrap; user can save / rename / delete a custom preset; v1.0.0 fixture project file loads without warning. **Plus** Undo/Redo verification on point placement, gizmo drag, mask paint, and sculpt brush (Sprint 3.7 retro action item).
- [ ] Sprint outcome notes updated with actual velocity, carryover (or "None"), and a retrospective link

> **Process note (Sprint 3.7 + 3.8 retro action items):** Do NOT mark the Sprint
> Outcome as "Complete" or "Implementation Complete" until **both** the
> `/review-pr` blockers are closed **and** the manual UI smoke has run. Story
> 4.7 makes this mechanical — the `/sprint-retrospective` skill will refuse to
> run on a Complete-status sprint with unchecked DoD boxes, so this is no
> longer a discipline question.

---

## Sprint Outcome

**Status**: Planned
**Velocity (Actual)**: TBD
**Carryover**: TBD
**Notes**: TBD
