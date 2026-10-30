# Sprint 10: App Shell & Morph Stage Polish

**Sprint Goal**: Every visible menu item works, developer-only UI is hidden,
the app ships with its icon, and the Morph stage can return to the source mesh
safely.
**Duration**: 2026-11-09 — 2026-11-13
**Capacity**: ~13 points (revisit after Sprint 9 velocity)
**Committed**: 12 points
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Pending

Source: user feedback triage on 2026-10-29 (items 1, 6, 7, 8, 9, 10, 11).
Alignment items from the same triage are planned in [Sprint 9](sprint-09.md).
Both sprints edit `AppWindow.cpp`, so run them in order, not in parallel.

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [ ] Sprint plan approved before execution
- [ ] Approved scope executed
- [ ] Tests run and validation recorded
- [ ] Sprint completion confirmed
- [ ] Retrospective saved in `.agents/context/retrospectives/`
- [ ] Context updated with `$update-context`
- [ ] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 10.1 | **Debug HUD hidden by default** | 1 | None | [ ] |
| 10.2 | **Remove stub Mesh menu** | 1 | None | [ ] |
| 10.3 | **Working View menu** | 3 | None | [ ] |
| 10.4 | **Menu spacing pass** | 1 | 10.1–10.3 (final menu contents) | [ ] |
| 10.5 | **MetaVisage icon on the executable and window** | 2 | None | [ ] |
| 10.6 | **Reset morph result to source mesh** | 3 | None | [ ] |
| 10.7 | **Vertex mask in its own Morph section** | 1 | None | [ ] |

### 10.1 Debug HUD hidden by default (feedback #1)

`AppWindow::RenderFrame` (`src/ui/AppWindow.cpp:2224`) draws the "Debug HUD"
window every frame; its own comment says it was meant to be removed in 3.6.

- [ ] Gate it behind a `showDebugHud_` flag, default off, toggled from
      Help > "Debug HUD (Dev)" next to the Icon Glyph Explorer entry.
- [ ] Confirm the dockspace/ImGui ini doesn't re-open it on next launch.

### 10.2 Remove stub Mesh menu (feedback #6)

All four Mesh items (`src/ui/AppWindow.cpp:1422`) are permanently disabled
placeholders.

- [ ] Remove the menu. Record Recompute Normals / Validate Topology /
      Center on Origin / Snap to Floor in the backlog only if still wanted.

### 10.3 Working View menu (feedback #7)

Every View item (`src/ui/AppWindow.cpp:1405`) is disabled, though the
behavior already exists: `Renderer::SetShowGrid` and numpad / Home handling
in `Viewport::OnKey` (`src/ui/Viewport.cpp:548`).

- [ ] Extract the numpad/Home camera presets into `Viewport` methods used by
      both the key handler and the menu (no duplicated camera math).
- [ ] Show Grid is a checked toggle applied to every viewport (Stage 2 is
      dual-view) and persisted in `Config`.
- [ ] Front / Right / Top / Perspective act on the primary viewport (or the
      hovered one in dual mode); add "Toggle Ortho/Persp (Numpad 5)", which the
      shortcuts modal already advertises.
- [ ] Shortcut labels go in the `MenuItem` shortcut column, not the label text.
- [ ] UI smoke: toggle Show Grid via menu and assert renderer state.
- [ ] Manual: Casey has no numpad, so the View menu is the hand-test path for
      the orthographic views. Sprint 9 fixed the degenerate Numpad 7 top view
      and made Numpad 1/3/7 set yaw/pitch (keeping target and distance); they
      are covered only by `alignment_gizmo_all_views` until this lands. Check
      Top view rendering and orbiting out of each view from the menu.

### 10.4 Menu spacing pass (feedback #8)

Menus inherit the global `ItemSpacing.y = 6` and `FramePadding.y = 6`
(`src/ui/MvStyle.cpp:75`), which reads cramped in popups.

- [ ] Add menu-specific spacing/padding tokens to `MvStyle` (no pixel
      literals in widgets) and apply them to every main menu popup.
- [ ] Before/after screenshots of File, Edit, View and Help.

### 10.5 MetaVisage icon on the executable and window (feedback #9)

There is no `.ico`, no `.rc` and no `glfwSetWindowIcon` call. Sized PNGs
already exist under `assets/icons/Windows/` and `assets/icons/MacOS.appiconset/`.

- [ ] Generate a committed multi-resolution `assets/icons/metavisage.ico`
      (16, 24, 32, 48, 64, 256) from the existing PNGs; document how it was made.
- [ ] Add `resources/metavisage.rc` and include it in the MSVC and MinGW
      executable sources in `CMakeLists.txt`.
- [ ] Call `glfwSetWindowIcon` at startup so the title bar and taskbar match.
- [ ] Verify in Explorer, the taskbar, and the packaged ZIP from `package-release.ps1`.

### 10.6 Reset morph result to source mesh (feedback #10)

There is no way to discard a processed morph. Related bug found during triage:
"Reset to defaults" (`src/ui/StageSidebar.cpp:1281`) runs `morph = MorphData()`,
which also wipes the processed result, the vertex mask and the accepted flag,
and isn't undoable.

- [ ] Add "Reset to source mesh" in the Morph inspector, enabled when
      `isProcessed`. It clears `deformedMorphMesh`, displacement data,
      `isProcessed` and `isAccepted`, and returns the viewport to the source mesh.
- [ ] Route it through an undoable command (reuse or extend `MorphUndoAction`)
      so Ctrl+Z restores the result.
- [ ] If the morph was accepted and Touch Up sculpting exists, confirm before
      discarding.
- [ ] Fix "Reset to defaults" to reset only wrap parameters, keeping the
      result and mask.

### 10.7 Vertex mask in its own Morph section (feedback #11)

The mask controls sit under a flat "VERTEX MASK" label between the preset
cards and the collapsible General / Advanced sections, so they read as part
of the parameter block.

- [ ] Move them into a collapsible "Vertex Mask" section (default expanded,
      collapse state persisted via `Config` like General / Advanced), placed
      after Advanced and before Process.
- [ ] Keep the no-mesh guard that disables mask painting.

---

## Sprint Context (Key Affected Files)

- `src/ui/AppWindow.cpp`, `include/ui/AppWindow.h` — menus, Debug HUD, window icon, reset action
- `src/ui/Viewport.cpp`, `include/ui/Viewport.h` — camera preset methods
- `src/ui/ViewportHost.cpp` — apply grid / view to the right viewport(s)
- `src/rendering/Renderer.cpp` — grid toggle
- `src/utils/Config.cpp` — persisted grid toggle
- `src/ui/MvStyle.cpp`, `include/ui/MvStyle.h` — menu spacing tokens
- `src/ui/StageSidebar.cpp`, `include/ui/StageSidebar.h` — Morph reset, mask section
- `include/core/UndoActions.h` — undoable morph reset
- `CMakeLists.txt`, new `resources/metavisage.rc`, new `assets/icons/metavisage.ico`
- `tests/smoke/ui_smoke.cpp` — menu, grid and reset scenarios

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| None | | |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes | `ui_smoke`: Debug HUD hidden at launch, Mesh menu absent, Show Grid toggle, Morph reset + undo (if a loaded-mesh fixture is practical) | |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy`, `.\build-mingw.ps1`, full `ctest` and `-L ui` | MinGW needed because the `.rc` change touches both toolchains |
| Live smoke tests | Not required | | Offline desktop app |
| Rendered image checks | Yes | EXE icon in Explorer and taskbar; packaged ZIP | Covers 10.5 |
| Link audit | Not required | | No links touched |
| Screenshots / visual audit | Yes | Menus before/after spacing; Morph inspector with the new mask section | |
| Impeccable design pass | Optional | Quick critique of menu spacing and Morph inspector order | Only if the skill is enabled |
| Loading / empty / error states | Yes | Reset disabled when nothing is processed; View menu with no project loaded | |

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Undo for morph reset holds a full mesh copy | Med | Low | Store shared_ptr to the existing deformed mesh, not a deep copy |
| `.rc` / `.ico` handling differs between MSVC and MinGW | Med | Med | Build both toolchains in the sprint |
| Reset after Touch Up sculpting loses work | Med | High | Confirm modal plus undo |

---

## Definition of Done

- [ ] Code reviewed
- [ ] PR opened against `development` and `$review-pr` run; all blocking findings resolved
- [ ] Tests passing
- [ ] Windows manual UI smoke: Stage 1 -> 2 -> 3 -> 4 -> Export with the bundled MetaHuman head, plus Undo/Redo on Morph reset and mask paint
- [ ] Required quality evidence captured in the retrospective
- [ ] PO approval
- [ ] Sprint outcome confirmed
- [ ] Retrospective saved
- [ ] Context updated

---

## Sprint Outcome

**Status**: Planned
**Velocity (Actual)**: —
**Carryover**: —
**Validation**: —
**Quality Evidence**: —
**Notes**: —
**Campaign Handoff**: N/A
