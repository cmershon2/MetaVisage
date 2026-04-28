---
name: MetaVisage Architecture Overview
description: Layer map, ImGui shell structure, Qt removal completion, and Sprint 3.8 UI/UX audit findings as of 2026-04-27
type: project
---

## Layer Map

```
Presentation  → src/ui/AppWindow.cpp, StageSidebar.cpp, Modals.cpp, ViewportHost.cpp, Viewport.cpp, FileDialogs.cpp
Application   → src/ui/AppWindow.cpp (stage orchestration, morph threading, undo/redo wiring)
Domain        → src/core/, src/deformation/, src/sculpting/, src/baking/
Infrastructure → src/rendering/, src/io/, src/utils/
```

## Qt Status (Sprint 3.7 complete, branch sprint/UI-UX-Redesign-Implementation)

Qt6::Core is fully removed from all live files. AppWindow, StageSidebar, Modals, ViewportHost, Viewport use only std:: types. All Qt dead files were deleted. CMakeLists.txt has a configure-time guard blocking Qt reintroduction.

## ImGui Shell Structure (Sprint 3.8 audit)

### Theme / Style init
- Single function: `AppWindow::ApplyDarkTheme()` at `src/ui/AppWindow.cpp:471–505`
- Called once during `Initialize()` at line 150
- Pattern: `ImGui::StyleColorsDark()` baseline, then explicit `s.Colors[...]` overrides
- Current palette: WindowBg/ChildBg = rgb(30,30,30), PopupBg/FrameBg = rgb(44,62,80), accents = rgb(52,152,219)
- Rounding: WindowRounding=4, FrameRounding=3, GrabRounding=3
- No PushStyleColor overrides in the main frame loop except: StageSidebar.cpp:77 (stage name text blue tint), Modals.cpp:92–93 (error/info title color), and Viewport.cpp hardcoded PackColor() calls

### App shell / DockSpace
- `AppWindow::RenderAppShell()` at `src/ui/AppWindow.cpp:679–771`
- Single full-screen host window `##AppHostWindow` with `ImGuiWindowFlags_MenuBar | NoBackground | NoDocking | NoTitleBar...`
- `ImGui::DockSpace("AppDockSpace", ...)` with `PassthruCentralNode` flag — the central area is transparent, OpenGL renders behind it
- Dockspace split rebuilt every launch (static bool sDockBuilt): 28% right → "Stage" window, 72% center → viewport. Locked with `NoUndocking | NoDockingOverMe | NoDockingSplit | HiddenTabBar`
- `centralRectPos_` / `centralRectSize_` captured from `DockBuilderGetCentralNode` and passed to `ViewportHost::BuildLayout`

### Menu bar
- `AppWindow::RenderMainMenuBar()` at `src/ui/AppWindow.cpp:773–847`
- Menus: File (New/Open/Save/SaveAs/Exit), Edit (Undo/Redo), View (Show Grid/camera presets — all disabled stubs except grid toggle), Help (Docs/Account/Updates/Shortcuts/About)
- Right-aligned stage indicator text: "Stage N / 4 — StageName"

### Status bar
- `AppWindow::RenderStatusBar()` at `src/ui/AppWindow.cpp:849–859`
- Single `ImGui::Text(...)` line at bottom of host window: "Stage N / 4 — StageName | StatusMessage"
- No separate top-level window — rendered as last child of host window

### StageSidebar
- Entry: `StageSidebar::Render()` at `src/ui/StageSidebar.cpp:55–106`
- Single `ImGui::Begin("Stage", ...)` window with `NoMove | NoCollapse | NoTitleBar`
- Header: stage counter, blue-tinted stage name (PushStyleColor), hint text, Separator
- Per-stage dispatch via switch → RenderAlignment / RenderPointReference / RenderMorph / RenderTouchUp
- Footer: Previous / Next buttons with CanProceedToNextStage gate

### Per-stage sidebar render functions (all in StageSidebar.cpp):
- `RenderAlignment()` lines 112–206 (~95 lines): flat layout — TextUnformatted labels, Button rows, DragFloat3 for Position/Rotation/Scale, Combo for texture slot, Button for Reset Transform. No collapsing headers or BeginChild.
- `RenderPointReference()` lines 212–246 (~35 lines): Text stats (paired/morph-only/target-only), Checkbox + Combo for symmetry, SliderFloat for point size, Button for clear.
- `RenderMorph()` lines 252–370 (~120 lines): flat SliderInt/SliderFloat for main params, CollapsingHeader("Advanced") wrapping 10+ advanced sliders + Reset button, ProgressBar + Cancel when processing, Combo for preview mode, Accept button, then BeginChild-free Vertex Mask sub-panel with Checkbox/SliderFloat.
- `RenderTouchUp()` lines 376–428 (~55 lines): RadioButton row for brush type, SliderFloat for Radius/Strength, Combo for Falloff, Checkbox for Symmetry + axis Combo, Checkbox for target overlay, full-width Export button.

### Modals (all in src/ui/Modals.cpp except About)
- `RenderConfirmModal()` lines 48–67: `BeginPopupModal("##ConfirmModal")` — AlwaysAutoResize, title + separator + message + two buttons
- `RenderErrorModal()` lines 87–101: `BeginPopupModal("##ErrorModal")` — title color push (red or green), separator, message, OK button
- `RenderWelcomeModal()` lines 113–172: `BeginPopupModal("Welcome to MetaVisage")` — 520×420, New Project / Open Project buttons, BeginChild("##recent_projects", {0,180}, border=true) scrollable recent list, Close button
- `RenderExportModal()` lines 185–279: `BeginPopupModal("Export Mesh")` — 560 wide, file path + Browse button, format/materials/transform/Y-up/MetaHuman/scale/prefix checkboxes, texture baking combos, Export + Cancel. One PushStyleVar(FrameBorderSize, 1.0f) override.
- `RenderShortcutsModal()` lines 287–349: `BeginPopupModal("Keyboard Shortcuts")` — 560×520, BeginChild("##shortcuts_scroll", {0,420}, border=true), groups rendered via BeginTable 2-col
- `RenderAboutModal()` at `src/ui/AppWindow.cpp:947–961`: inline in AppWindow, not in Modals.cpp — `BeginPopupModal("About MetaVisage")` — plain text, OK button
- `RenderModals()` at AppWindow.cpp:963 dispatches all six

### Welcome screen
- Implemented as `BeginPopupModal` (centered modal), not a separate window or docked panel
- Triggered on first frame if project has no content: AppWindow.cpp:651–660
- Contains a `BeginChild` scrollable recent-projects list

### Custom drawing (ImDrawList)
- `Viewport::DrawOverlays()` at `src/ui/Viewport.cpp:610–631` — clips a foreground draw list to viewport rect, then calls four sub-functions:
  - `DrawViewportLabelOverlay()` line 694: AddRectFilled + AddText (bottom-left label badge)
  - `DrawOrientationGizmoOverlay()` line 717: AddLine + AddCircleFilled + AddText (XYZ axis gizmo, top-right)
  - `DrawPointLabelsOverlay()` line 770: AddRectFilled + AddText per correspondence point (orange selected, black unselected)
  - `DrawActiveBorderOverlay()` line 821: AddRect — blue border (52,152,219) on active viewport in dual mode
- `DrawShadingModeOverlay()` line 828: floating ImGui window `##ShadingMode_<ptr>` at top-left of viewport, semi-transparent (BgAlpha=0.55), 120px combo
- All hardcoded colors via PackColor() helper (IM_COL32) — no theme tokens

### Viewport background
- GL clear: `glClearColor(0.2f, 0.2f, 0.2f, 1.0f)` at Viewport.cpp:662 (medium grey)
- Host window clear: `glClearColor(30/255, 30/255, 30/255, ...)` at AppWindow.cpp:668
- No blueprint grid background — there is a `assets/shaders/grid.vert/.frag` but the GL grid is rendered by the Renderer class, not ImGui

### Font loading
- No TTF fonts loaded anywhere in src/. `io.Fonts->AddFont*` is never called.
- App uses ImGui's default embedded ProggyClean bitmap font throughout
- All overlay text (`AddText`, label combos, sidebar widgets) uses `ImGui::GetFont()` / `ImGui::GetFontSize()` — always returns the single default font
- No font asset files exist under assets/

### Sidebar sidebar width
- Dock split hardcoded at 28% right at AppWindow.cpp:733: `ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Right, 0.28f, ...)`

## Constraints for Sprint 3.8 Visual Overhaul

1. **DockSpace pins "Stage" window identity** — the string "Stage" at StageSidebar.cpp:67 and DockBuilderDockWindow("Stage") at AppWindow.cpp:749 must stay in sync. Renaming or wrapping that window breaks docking.
2. **Hardcoded 28% sidebar split** — changing sidebar width requires touching AppWindow.cpp:733.
3. **ImDrawList overlay colors are not themed** — all four overlay drawing functions use hardcoded PackColor() values (active border blue=52,152,219; selected point orange=230,126,34; axis colors; label backgrounds). These will not respond to ImGuiStyle changes.
4. **Gizmo colors live in the renderer** — transform gizmo axis colors are set by GizmoRenderer, not ImGui, and are OpenGL-rendered meshes, not ImGui widgets.
5. **GL viewport clear color** — Viewport.cpp:662 hardcodes the 3D background grey; must be changed separately from ImGuiStyle.
6. **Shading combo is a freestanding ImGui window** with hardcoded position offset (x+8, y+8) and BgAlpha=0.55 — custom positioning logic would need updating for any new chrome around the viewport.
7. **No font infrastructure** — adding Saira/Inter/JetBrains Mono requires: (a) adding TTF files to assets/fonts/, (b) adding font load calls in AppWindow::Initialize() after ImGui::CreateContext(), (c) updating Paths::ExecutableDir() asset resolution, (d) wiring multiple ImFont* handles for per-context PushFont/PopFont. Every text site in Viewport.cpp passes `ImGui::GetFont()` directly to AddText — those all need to become named font handles.
8. **FrameBorderSize=0 default** — ExportModal manually pushes FrameBorderSize=1 as a one-off fix (Modals.cpp:197). The new theme will want a consistent FrameBorderSize policy across all modals.
9. **No existing blueprint grid or glow** — zero ImDrawList-based background grid or shadow/glow primitives exist today; all must be net-new.
10. **CollapsingHeader in RenderMorph** — the "Advanced" collapsing header (StageSidebar.cpp:269) uses ImGui default header styling. The new spec's custom panel chrome will need to decide whether to replace this.
