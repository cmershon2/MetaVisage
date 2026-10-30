# Sprint 9: Alignment Stage Usability Fixes

**Sprint Goal**: Make Stage 1 Alignment usable without workarounds: the gizmo
tracks the cursor, the advertised hotkeys work, transform values can be typed,
and target textures survive a target-mesh swap.
**Duration**: 2026-11-02 — 2026-11-06
**Capacity**: ~13 points (no pointed velocity history; Sprint 8 was unpointed)
**Committed**: 10 points
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Approved by Casey on 2026-10-29 (keep Blender G/R/S hotkeys; correct the tool rail labels)

Source: user feedback triage on 2026-10-29 (items 2, 3, 4, 5). Shell and Morph
stage items from the same triage are planned in [Sprint 10](sprint-10.md).

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [x] Sprint plan approved before execution
- [x] Approved scope executed
- [x] Tests run and validation recorded
- [x] Sprint completion confirmed (Casey, 2026-10-30)
- [x] Retrospective saved in `.agents/context/retrospectives/`
- [x] Context updated with `$update-context`
- [ ] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 9.1 | **Alignment gizmo moves at cursor speed** | 3 | None | [x] Implemented, tested (perspective, orbited, Numpad 1/3/7) |
| 9.2 | **Alignment tool hotkeys match the tool rail** | 1 | Decided: keep G/R/S | [x] Implemented, tested |
| 9.3 | **Typed entry for target transform fields** | 3 | None | [x] Implemented, tested |
| 9.4 | **Target textures persist across target mesh replacement** | 2 | None | [x] Implemented, tested, verified by Casey |
| 9.5 | **Remove placeholder Alignment rail tools** | 1 | None | [x] Implemented; screenshot captured |

### 9.1 Alignment gizmo moves at cursor speed (feedback #2)

**Root cause**: `Viewport::OnMouseMove` (`src/ui/Viewport.cpp:336`) applies a
fixed `0.01` world units per pixel and always translates along world X/Y. With
cm-scale heads and a camera several hundred units away, the mesh crawls; after
orbiting, free move also goes in the wrong screen direction.

- [ ] Free move translates in the camera's right/up plane, scaled by
      camera distance and FOV (or ortho extent) over viewport height, so the
      mesh stays under the cursor.
- [ ] Axis-constrained move projects the screen delta onto the axis's
      screen-space direction, so dragging along a visible axis feels 1:1.
- [ ] Review rotate/scale sensitivity at cm scale; keep current values unless
      they are also visibly off.
- [ ] One undo entry per gizmo drag is preserved.

### 9.2 Alignment tool hotkeys match the tool rail (feedback #2)

**Root cause**: the rail badges and tooltips say W / E / R
(`src/ui/ToolRail.cpp:38`), but `Viewport::OnKey` binds Blender-style
G / R / S (`src/ui/Viewport.cpp:451`). R therefore selects Rotate, not Scale,
and W / E do nothing.

**Open decision**: AGENTS.md lists G/R/S transforms as a non-negotiable, and
the shortcuts modal and README already document G/R/S. W/E/R and G/R/S cannot
coexist because R means Rotate in one scheme and Scale in the other.

- Recommended: keep G/R/S and correct the rail badges and tooltips.
- Alternative: switch to W/E/R, which requires updating AGENTS.md, README,
  the shortcuts modal and the UI smoke test.

Tasks:
- [ ] Apply the chosen scheme to rail badges, tooltips, `OnKey`, the shortcuts
      modal and docs so all of them agree.
- [ ] UI smoke scenario: press each hotkey in Alignment and assert the active
      transform mode.

### 9.3 Typed entry for target transform fields (feedback #3)

**Root cause**: `MvInspector::DrawAxisInput`
(`src/ui/MvInspectorPrimitives.cpp:240`) is an `InvisibleButton` drag widget
with no text-entry mode.

- [ ] Double-click or Ctrl+click enters an inline `InputFloat` in the same
      frame; Enter or focus loss commits, Esc cancels.
- [ ] A typed commit produces exactly one `TransformUndoAction`, matching the
      existing one-undo-per-gesture contract (`editStarted` / `editFinished`).
- [ ] Uniform scale typed into X still cascades to Y/Z.
- [ ] Position drag speed scales with the target mesh bounds so dragging is
      usable at cm scale (the fixed `0.01` drag speed has the same crawl as 9.1).
- [ ] While the field is in text mode, viewport hotkeys stay suppressed
      (`WantCaptureKeyboard` already gates this; verify).

### 9.4 Target textures persist across target mesh replacement (feedback #4)

**Root cause**: textures are stored on the project (`TargetTextures`), but the
GPU albedo is uploaded only to the `MeshRenderer` keyed by the mesh pointer at
import time (`AppWindow::ImportTextureForSlot`, `src/ui/AppWindow.cpp:2061`).
`ImportTargetMeshFromDisk` creates a new mesh and never re-uploads, so the new
model renders untextured while the sidebar still lists the textures. Opening a
saved project appears to have the same gap: `OpenProjectFromDisk` never uploads
the albedo.

- [ ] Add one helper that uploads the project's albedo to the current target
      mesh renderer, and call it after texture import, target mesh import and
      project open.
- [ ] Release the stale renderer and texture for the replaced mesh so GPU
      memory doesn't grow with each swap.
- [ ] Manual check: mesh A + albedo, then replace with mesh B, shows the
      textured mesh B; save, reopen, still textured.

### 9.5 Remove placeholder Alignment rail tools (feedback #5)

The two always-disabled rail buttons are "Symmetry X" and "Pivot snap"
(`src/ui/ToolRail.cpp:271`); neither has a backend.

- [ ] Remove both stubs and the Stage 1 divider when nothing follows it.
- [ ] Add a backlog note if transform symmetry / pivot snapping is still wanted.

---

## Sprint Context (Key Affected Files)

- `src/ui/Viewport.cpp`, `include/ui/Viewport.h` — gizmo drag math, hotkeys
- `src/core/Camera.cpp`, `include/core/Camera.h` — camera basis / FOV accessors
- `src/ui/ToolRail.cpp` — Stage 1 tool badges and stub removal
- `src/ui/MvInspectorPrimitives.cpp`, `include/ui/MvInspectorPrimitives.h` — typed axis input
- `src/ui/StageSidebar.cpp` — Alignment transform group drag speeds
- `src/ui/AppWindow.cpp` — texture upload helper, target import, project open
- `src/rendering/Renderer.cpp`, `src/rendering/MeshRenderer.cpp` — texture lifetime
- `src/ui/Modals.cpp`, `README.md`, `AGENTS.md` — only if the hotkey scheme changes
- `tests/smoke/ui_smoke.cpp` — hotkey and typed-entry scenarios

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| Ctrl+Z defers to an active text field (`HandleGlobalShortcut`) | Typed entry would otherwise run an app undo mid-gesture | [x] |
| Transform group axis IDs scoped per group (`PushID(label)`) | Position / Rotation / Scale "X" fields can't share drag or typing state | [x] |
| Replaced target mesh's GPU renderer invalidated | Frees buffers/texture and prevents reuse of a stale cache entry at a recycled address | [x] |
| Viewport stage hint updated from W/E/R to G/R/S | Same mismatch as the rail badges | [x] |
| `Viewport::GetPixelWidth/Height` accessors | Lets the smoke test project the gizmo to screen pixels | [x] |
| Numpad 7 top view fixed: camera up now derived from yaw/pitch; Numpad 1/3/7 set yaw/pitch and keep the current target and distance | Top view had a degenerate view basis (forward parallel to +Y up), so it rendered wrongly and the gizmo couldn't track; views also jumped to the world origin and orbit snapped back afterwards | [x] |
| `Renderer/Viewport::HasMeshTexture` query | Lets the smoke test assert the albedo is on the new target's renderer | [x] |
| UI smoke harness `RunOnMainThread` | Test bodies run on the engine's coroutine thread without a current GL context; GL uploads must run on the main loop | [x] |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes | New `ui_smoke` scenarios: Alignment hotkeys, typed transform entry with single undo | Behavior regressions here are UI-only |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy`, `ctest --test-dir build -C Release --output-on-failure`, `-L ui` | Full four-target suite |
| Live smoke tests | Not required | | Offline desktop app; no services |
| Rendered image checks | Yes | Screenshot of a replaced target mesh with albedo applied | Covers 9.4 |
| Link audit | Not required | | No links touched |
| Screenshots / visual audit | Yes | Tool rail after stub removal; transform field in text mode | |
| Impeccable design pass | Not required | | Small changes inside the existing design system |
| Loading / empty / error states | Yes | Typed entry with invalid / empty input; texture present with no target mesh | |

Manual: gizmo drag at default camera and after orbit, with and without X/Y/Z
constraint; Undo/Redo after gizmo drag and typed entry.

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Camera-plane math differs between perspective and ortho views | Med | Med | Branch on projection mode; manual check in Numpad 1/3/7 views |
| Typed entry breaks one-undo-per-gesture | Low | Med | Reuse `DragEditResult` start/finish signals; smoke test asserts one undo |
| Hotkey change conflicts with project non-negotiables | Med | Low | Decision recorded in this plan before implementation |

---

## Definition of Done

- [ ] Code reviewed
- [ ] PR opened against `development` and `$review-pr` run; all blocking findings resolved
- [x] Tests passing
- [x] Windows manual UI smoke: Stage 1 -> 2 -> 3 -> 4 -> Export with the bundled MetaHuman head, plus Undo/Redo on gizmo drag and typed transform entry
- [x] Required quality evidence captured in the retrospective
- [x] PO approval
- [x] Sprint outcome confirmed
- [x] Retrospective saved
- [x] Context updated

---

## Sprint Outcome

**Status**: Complete (confirmed by Casey 2026-10-30; PR review pending)
**Velocity (Actual)**: 10 points implemented
**Carryover**: None planned
**Validation** (2026-10-29):
- `.\build-msvc.ps1 -Deploy`: builds and deploys `MetaVisage.exe`.
- `ctest --test-dir build -C Release --output-on-failure`: 4/4 passed (40.6 s).
- `metavisage_smoke_ui.exe`: 11/11 scenarios passed. New: `alignment_transform_hotkeys`; `alignment_gizmo_tracks_cursor` (centre within 1 px after free and X-constrained drags, one undo per drag); `alignment_gizmo_all_views` (same checks after orbiting and in Numpad 1/3/7; constrained drag changes only its axis); `alignment_texture_survives_target_swap` (albedo on the new target's renderer after importing a replacement OBJ; confirmed failing with the 9.4 fix reverted); `alignment_typed_transform_entry` (double-click, type, Enter commits with one undo; Esc cancels).
- `git diff --check`: clean.
**Manual verification** (Casey, 2026-10-30): gizmo move is fast and tracks well; double-click typed entry works; G/R/S hotkeys work; Stage 1 rail is clean (screenshot supplied in session); textures survive target swap; full Stage 1 -> 4 -> Export completed. Not hand-tested: Numpad 1/3/7 views (no numpad on Casey's keyboard); covered by `alignment_gizmo_all_views` and carried into Sprint 10.3 as a View-menu check.
**Remaining**: PR to `development` with `$review-pr`, then the focused sprint commit (on request).
**Found during the sprint (backlog)**: the shortcuts modal lists Ctrl+Y for redo but no handler exists; mesh renderer cache entries for the previous project's meshes are not released on New/Open Project.
**Quality Evidence**: See [retro-sprint-09.md](../retrospectives/retro-sprint-09.md).
**Notes**: —
**Campaign Handoff**: N/A
