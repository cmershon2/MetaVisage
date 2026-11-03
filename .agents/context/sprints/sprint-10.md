# Sprint 10: App Shell & Morph Stage Polish

**Sprint Goal**: Every visible menu item works, developer-only UI is hidden,
the app ships with its icon, and the Morph stage can return to the source mesh
safely.
**Duration**: 2026-11-09 — 2026-11-13
**Capacity**: ~13 points (revisit after Sprint 9 velocity)
**Committed**: 12 points
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Approved by Casey on 2026-10-30 ("start sprint 10 in a new development branch"); branched from `fix/sprint-09-alignment` because PR #29 is not merged yet

Source: user feedback triage on 2026-10-29 (items 1, 6, 7, 8, 9, 10, 11).
Alignment items from the same triage are planned in [Sprint 9](sprint-09.md).
Both sprints edit `AppWindow.cpp`, so run them in order, not in parallel.

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [x] Sprint plan approved before execution
- [x] Approved scope executed
- [x] Tests run and validation recorded
- [x] Sprint completion confirmed (Casey, 2026-10-30)
- [x] Retrospective saved in `.agents/context/retrospectives/`
- [x] Context updated with `$update-context`
- [x] Focused sprint commit created with `$commit`
- [x] Campaign handoff written, if this sprint is part of Campaign Autopilot (N/A: no campaign)

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 10.1 | **Debug HUD hidden by default** | 1 | None | [x] Implemented, tested |
| 10.2 | **Remove stub Mesh menu** | 1 | None | [x] Implemented, tested |
| 10.3 | **Working View menu** | 3 | None | [x] Implemented, tested, verified by Casey (+ projection toggle follow-up) |
| 10.4 | **Menu spacing pass** | 1 | 10.1–10.3 (final menu contents) | [x] Implemented; screenshots supplied by Casey |
| 10.5 | **MetaVisage icon on the executable and window** | 2 | None | [x] Implemented, verified (EXE + window class icon) |
| 10.6 | **Reset morph result to source mesh** | 3 | None | [x] Implemented, tested |
| 10.7 | **Vertex mask in its own Morph section** | 1 | None | [x] Implemented, tested; moved above Reset to defaults per Casey |

### 10.1 Debug HUD hidden by default (feedback #1)

`AppWindow::RenderFrame` (`src/ui/AppWindow.cpp:2224`) draws the "Debug HUD"
window every frame; its own comment says it was meant to be removed in 3.6.

- [x] Gate it behind a `showDebugHud_` flag, default off, toggled from
      Help > "Debug HUD (Dev)" next to the Icon Glyph Explorer entry.
- [x] Confirm the dockspace/ImGui ini doesn't re-open it on next launch.

### 10.2 Remove stub Mesh menu (feedback #6)

All four Mesh items (`src/ui/AppWindow.cpp:1422`) are permanently disabled
placeholders.

- [x] Remove the menu. Record Recompute Normals / Validate Topology /
      Center on Origin / Snap to Floor in the backlog only if still wanted.
      (Not added: no user request for them.)

### 10.3 Working View menu (feedback #7)

Every View item (`src/ui/AppWindow.cpp:1405`) is disabled, though the
behavior already exists: `Renderer::SetShowGrid` and numpad / Home handling
in `Viewport::OnKey` (`src/ui/Viewport.cpp:548`).

- [x] Extract the numpad/Home camera presets into `Viewport` methods used by
      both the key handler and the menu (no duplicated camera math).
- [x] Show Grid is a checked toggle applied to every viewport (Stage 2 is
      dual-view) and persisted in `Config`.
- [x] Front / Right / Top / Perspective act on the primary viewport (or the
      hovered one in dual mode); add "Toggle Ortho/Persp (Numpad 5)", which the
      shortcuts modal already advertises.
- [x] Shortcut labels go in the `MenuItem` shortcut column, not the label text.
- [x] UI smoke: toggle Show Grid via menu and assert renderer state.
- [x] Manual: Casey has no numpad, so the View menu is the hand-test path for
      the orthographic views. Sprint 9 fixed the degenerate Numpad 7 top view
      and made Numpad 1/3/7 set yaw/pitch (keeping target and distance); they
      are covered only by `alignment_gizmo_all_views` until this lands. Check
      Top view rendering and orbiting out of each view from the menu.

### 10.4 Menu spacing pass (feedback #8)

Menus inherit the global `ItemSpacing.y = 6` and `FramePadding.y = 6`
(`src/ui/MvStyle.cpp:75`), which reads cramped in popups.

- [x] Add menu-specific spacing/padding tokens to `MvStyle` (no pixel
      literals in widgets) and apply them to every main menu popup.
- [x] Before/after screenshots of File, Edit, View and Help. (After
      screenshots supplied by Casey 2026-10-30.)

### 10.5 MetaVisage icon on the executable and window (feedback #9)

There is no `.ico`, no `.rc` and no `glfwSetWindowIcon` call. Sized PNGs
already exist under `assets/icons/Windows/` and `assets/icons/MacOS.appiconset/`.

- [x] Generate a committed multi-resolution `assets/icons/metavisage.ico`
      (16, 24, 32, 48, 64, 256) from the existing PNGs; document how it was made.
- [x] Add `resources/metavisage.rc` and include it in the MSVC and MinGW
      executable sources in `CMakeLists.txt`.
      (Shipped as `resources/metavisage.rc.in`, configured with the absolute
      icon path. MinGW compiles it in a custom `windres` step because windres
      splits include paths containing spaces.)
- [x] Call `glfwSetWindowIcon` at startup so the title bar and taskbar match.
      (Not needed: the resource is named `GLFW_ICON`, which GLFW's Win32
      backend loads as the window-class icon. Verified on the running window.)
- [/] Verify in Explorer, the taskbar, and the packaged ZIP from `package-release.ps1`.
      (EXE icon and live window-class icon verified by extraction; packaged
      ZIP not built — the icon is embedded in the EXE the ZIP copies.)

### 10.6 Reset morph result to source mesh (feedback #10)

There is no way to discard a processed morph. Related bug found during triage:
"Reset to defaults" (`src/ui/StageSidebar.cpp:1281`) runs `morph = MorphData()`,
which also wipes the processed result, the vertex mask and the accepted flag,
and isn't undoable.

- [x] Add "Reset to source mesh" in the Morph inspector, enabled when
      `isProcessed`. It clears `deformedMorphMesh`, displacement data,
      `isProcessed` and `isAccepted`, and returns the viewport to the source mesh.
- [x] Route it through an undoable command (reuse or extend `MorphUndoAction`)
      so Ctrl+Z restores the result.
- [x] If the morph was accepted and Touch Up sculpting exists, confirm before
      discarding. (Confirms whenever the result is accepted; Touch Up edits
      are only possible after accepting.)
- [x] Fix "Reset to defaults" to reset only wrap parameters, keeping the
      result and mask. (Resets to Quick Wrap, the new-project default.)

### 10.7 Vertex mask in its own Morph section (feedback #11)

The mask controls sit under a flat "VERTEX MASK" label between the preset
cards and the collapsible General / Advanced sections, so they read as part
of the parameter block.

- [x] Move them into a collapsible "Vertex Mask" section (default expanded,
      collapse state persisted via `Config` like General / Advanced), placed
      after Advanced and before "Reset to defaults" (Casey's review moved it
      above the reset button).
- [x] Keep the no-mesh guard that disables mask painting.

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
| `Config::Save` is a no-op until `Load` has run | The smoke harness never loads the user config, so menu tests would otherwise overwrite `%APPDATA%\MetaVisage\config.json` | [x] |
| Home / View > Reset Camera also returns to perspective | `Camera::Reset` kept the orthographic mode, leaving a "Front/Right/Top" mode on a reset view | [x] |
| View > Frame Meshes (F) added to the menu | Same camera-preset extraction; gives a mouse path for F | [x] |
| Viewport toolbar projection toggle (camera icon + PERSP / FRONT / RIGHT / TOP / ORTHO) replaces the static "PERSP" label, which was wrong in orthographic views | Casey: the View menu's Perspective item was too tucked away and users could get stuck in orthographic | [x] |
| Auto Perspective: orbiting out of a Front / Right / Top preset returns to perspective; an orthographic view chosen with the toggle stays orthographic (Blender default) | Same feedback; flag lives on `Camera` so dual-pane camera sync carries it | [x] |
| Menu popups previously inherited the app host's ItemSpacing.y = 0 | Root cause of the crowded menus; `MvStyle::Menu` tokens now apply inside each popup | [x] |

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

- [x] Code reviewed
- [ ] PR opened against `development` and `$review-pr` run; all blocking findings resolved
- [x] Tests passing
- [x] Windows manual UI smoke: Stage 1 -> 2 -> 3 -> 4 -> Export with the bundled MetaHuman head, plus Undo/Redo on Morph reset and mask paint
- [x] Required quality evidence captured in the retrospective
- [x] PO approval
- [x] Sprint outcome confirmed
- [x] Retrospective saved
- [x] Context updated

---

## Sprint Outcome

**Status**: Complete (confirmed by Casey 2026-10-30; PR pending)
**Velocity (Actual)**: 12 points implemented
**Carryover**: None planned
**Branch**: `feat/sprint-10-shell-morph`, stacked on `fix/sprint-09-alignment` (PR #29 not yet merged)
**Validation** (2026-10-30):
- `.\build-msvc.ps1 -Deploy`: builds and deploys `MetaVisage.exe`.
- `ctest --test-dir build -C Release --output-on-failure`: 4/4 passed (43.8 s).
- `metavisage_smoke_ui.exe`: 13/13. New: `shell_menus_and_debug_hud` (HUD hidden until Help toggle; no Mesh menu; Show Grid toggles both panes; Top / Perspective / Right / Reset Camera menu items; orbit out of Front returns to perspective; toolbar projection toggle, which stays orthographic while orbiting); `morph_reset_to_source` (section order Advanced < Vertex Mask < Reset to defaults; Reset to defaults keeps result and mask; reset clears the result with one undo, and undo restores it; accepted result confirms, Cancel keeps it; button disabled with no result).
- Icon: extracted from `MetaVisage.exe` and from the running window's class icon; both show the MetaVisage logo.
- MinGW: `.\build-mingw.ps1` not run because it shares `build\` with MSVC. The resource step was validated with a throwaway MinGW Makefiles project in a spaced path (`C:\Qt\Tools\mingw1310_64`): builds, links `.rsrc`, icon extractable.
- `git diff --check`: clean.
**Manual verification** (Casey, 2026-10-30): menu spacing good (File / Edit / View / Help screenshots); Vertex Mask options good (screenshot); View presets snap and orbit away; taskbar icon good; Reset to source mesh + undo work on a real morph; full Stage 1 -> 4 -> Export works. Follow-ups requested and implemented: a visible projection toggle (plus Auto Perspective), and the Vertex Mask section above Reset to defaults.
**Review** (2026-10-30, `$review-pr` on the working diff): no blocking findings. Fixed: the Keyboard Shortcuts dialog now lists F (frame meshes) and Home (reset camera) to match the View menu. Rebuilt; ctest 4/4.
**Remaining**: PR into `development` once cmershon2/MetaVisage#29 merges (or stacked on it); MinGW full build still unvalidated (shared `build\` directory).
**Quality Evidence**: See [retro-sprint-10.md](../retrospectives/retro-sprint-10.md).
**Notes**: —
**Campaign Handoff**: N/A
