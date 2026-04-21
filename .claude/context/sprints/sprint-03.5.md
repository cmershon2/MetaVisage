# Sprint 3.5: Framework Migration — Platform Layer (Qt → GLFW + ImGui)

**Sprint Goal**: Remove Qt's UI/windowing/GL stack (Gui, Widgets, OpenGL, OpenGLWidgets) and re-host on GLFW + Dear ImGui with the existing OpenGL rendering pipeline intact — app launches, viewport renders, and camera/input work end-to-end.
**Duration**: 2026-04-20 — 2026-05-01 (2 weeks)
**Capacity**: 26 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor)
**Committed**: 26 points

> **Scope note (added post-audit)**: `Qt6::Core` stays linked this sprint (LGPL, no commercial-license cost, dynamic-link compliant) because `QString`, `QDateTime`, `QJsonDocument`, `QFile`, and `QTextStream` are embedded in the public API of `core/*`, `io/*`, `deformation/*`, and `baking/*` — porting all call sites + public APIs is a 15–20 point effort that would blow this sprint. A follow-up item **"Qt6Core Removal (QString/QFile/QJson → std)"** is tracked in [product_backlog.md](../development/product_backlog.md). Sprint 3.5 ships the commercial-licensing-relevant cut: Gui/Widgets/OpenGL/OpenGLWidgets out.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 3.5.1: Dependency & Build System Overhaul** | 5 | None | [x] |
| 2 | **Story 3.5.2: Application Entry Point & Main Loop** | 8 | Story 3.5.1 | [x] |
| 3 | **Story 3.5.3: OpenGL Context & Viewport Rendering** | 8 | Story 3.5.2 | [x] |
| 4 | **Story 3.5.4: Input Handling** | 5 | Story 3.5.2 | [x] |

> Stories are pulled from the aggregated plan at [SPRINT_PLAN.md](./SPRINT_PLAN.md) §Sprint 3.5. This is framework-migration work, not product-backlog work — `product_backlog.md` is unchanged by this sprint.

---

## Sprint Context (Key Affected Files)

**Build / tooling**
- [CMakeLists.txt](../../../CMakeLists.txt) — strip Qt, add glfw3 / imgui / glad (or glew), pre-wire nlohmann_json / nativefiledialog-extended / CURL for Sprint 3.6
- [build-msvc.ps1](../../../build-msvc.ps1) — drop `windeployqt`, add GLFW/ImGui DLL staging
- [build-mingw.ps1](../../../build-mingw.ps1) — parity sweep alongside MSVC

**Entry point / app shell (Story 3.5.2)**
- [src/main.cpp](../../../src/main.cpp) — replace `QApplication` with GLFW + ImGui init/teardown and render loop
- [src/ui/MainWindow.cpp](../../../src/ui/MainWindow.cpp) — retire; fold into new `AppWindow`

**Viewport hosting (Story 3.5.3)**
- [src/ui/ViewportWidget.cpp](../../../src/ui/ViewportWidget.cpp) — convert `QOpenGLWidget` subclass to a plain `Viewport` driven by an ImGui frame region
- [src/ui/ViewportContainer.cpp](../../../src/ui/ViewportContainer.cpp) — replace `QSplitter` dual-viewport layout with two ImGui `InvisibleButton` regions
- [src/rendering/Renderer.cpp](../../../src/rendering/Renderer.cpp), [MeshRenderer.cpp](../../../src/rendering/MeshRenderer.cpp), [PointRenderer.cpp](../../../src/rendering/PointRenderer.cpp), [GizmoRenderer.cpp](../../../src/rendering/GizmoRenderer.cpp), [ShaderManager.cpp](../../../src/rendering/ShaderManager.cpp) — verify unchanged portability; restore GL state after 3D pass before handing back to ImGui

**Input (Story 3.5.4)**
- [src/core/Camera.cpp](../../../src/core/Camera.cpp) — wire GLFW mouse/scroll callbacks
- `src/ui/*Dialog.cpp` / `SidebarWidget.cpp` — **not** in this sprint's scope; re-implementation is deferred to Sprint 3.6

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Hidden Qt coupling in "portable" layers (e.g. `QString`/`qDebug` in rendering or core) | Med | High | Grep for `Q[A-Z]` / `<Q` includes across `src/core`, `src/rendering`, `src/deformation`, `src/sculpting` at start of Story 3.5.1; convert to std equivalents before any entry-point rewrite |
| Dual-viewport (Stage 2) camera sync breaks under new `InvisibleButton` hosting | Med | High | Validate Stage 2 point-placement flow explicitly against acceptance criteria before closing Story 3.5.3; keep a reference screenshot from the Qt build to compare |
| GL state bleed between 3D pass and ImGui draw list (depth test, blend, VAO) | High | Med | Wrap 3D pass in explicit save/restore; smoke-test with grid + mesh + points + overlays all visible |
| vcpkg toolchain drift on fresh checkout after dependency swap | Med | Med | Document exact vcpkg baseline in `BUILD.md` as part of Story 3.5.1; verify `build-msvc.ps1` from a clean build directory before sprint close |
| Viewport input conflicts with ImGui capture (e.g. scroll-to-zoom eaten by ImGui) | Med | Med | Gate all viewport input on `!ImGui::GetIO().WantCaptureMouse / WantCaptureKeyboard` per Story 3.5.4 task |
| Scope creep into Sprint 3.6 UI work (sidebars, menus, dialogs) | High | Med | This sprint only hosts the 3D viewport + a debug ImGui window; sidebars/menus/dialogs are explicitly Sprint 3.6. A debug-only ImGui panel is sufficient to prove the pipeline. |

---

## Definition of Done

- [ ] Zero `Qt6::Gui`, `Qt6::Widgets`, `Qt6::OpenGL`, `Qt6::OpenGLWidgets` headers included or libraries linked (`Qt6::Core` remains — tracked in backlog for follow-up)
- [ ] MSVC Release build succeeds cleanly via `build-msvc.ps1` from an empty build directory
- [ ] Application launches to a GLFW window with the enforced dark background regardless of OS theme
- [ ] Dear ImGui initializes and renders at least one placeholder debug window
- [ ] 3D viewport renders the infinite grid, loaded meshes (all shading modes), point markers, brush cursor, orientation gizmo, and shading-mode overlay
- [ ] Camera orbit (MMB), pan (Shift+MMB), and zoom (scroll) all work
- [ ] Keyboard shortcuts G/R/S, X/Y/Z, numpad views, `[`/`]`, `Delete`, `Escape` dispatch correctly via GLFW
- [ ] Stage 2 dual-viewport layout renders side-by-side with camera sync intact
- [ ] No rendering regressions versus the Qt build at end of Sprint 3 (visual parity check)
- [ ] PR reviewed via `/review-pr` and merged to `master`

---

## Sprint Outcome

**Status**: Implementation Complete (pending full-workflow parity smoke-test in 3.6)
**Velocity (Actual)**: 26/26 points committed landed in source (3.5.1, 3.5.2, 3.5.3, 3.5.4 all [x])
**Carryover**: —
**Notes**:
- All four stories implemented and built clean under MSVC Release.
- Runtime-verified: GLFW window + dark theme, ImGui debug HUD, viewport GL pass
  with grid, axis-label orientation gizmo, GL-state-isolated 3D pass, camera
  orbit (MMB) / pan (Shift+MMB) / zoom (scroll), Home/F/Numpad/Esc shortcuts,
  ImGui input gating working (debug HUD captures its own mouse, viewport owns
  the rest).
- Deferred-to-3.6 (no project/mesh loader in 3.5 runtime): visual parity check
  for loaded meshes, point markers, brush cursor, dual-viewport Stage 2 sync.
  These handler bodies early-return on null project in Sprint 3 too, so they
  wait for the sidebar + loader UI to come back.
- Qt6::Core still linked (QString/QFile/QJson in core/io/deformation/baking).
  Full Qt-free cut tracked as "Qt6Core Removal" backlog item.
