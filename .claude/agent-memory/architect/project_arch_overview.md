---
name: MetaVisage Architecture Overview
description: High-level layer map, key files, build system, and Qt coupling as of Sprint 3.5 audit
type: project
---

## Layer Map

```
Presentation  → src/ui/ (MainWindow, ViewportWidget, ViewportContainer, SidebarWidget, dialogs)
Application   → src/ui/MainWindow.cpp (stage orchestration, morph threading, undo/redo wiring)
Domain        → src/core/, src/deformation/, src/sculpting/, src/baking/
Infrastructure → src/rendering/, src/io/, src/utils/
```

Single root CMakeLists.txt only — no subdirectory cmake files.

## Entry Point

`src/main.cpp` — QApplication init, OpenGL format (4.3 core, 4xMSAA), dark Fusion palette, icon load, Logger init, MainWindow show.

## Key Structural Facts

- No .qrc files; no embedded resources. Shaders/assets copied to build dir via file(COPY) in CMakeLists.
- No vcpkg.json — must be created for Sprint 3.5.
- No single-instance detection, crash handler, or CLI arg parsing in main.cpp.
- tr() used ~50+ times but no QTranslator installed — pure string literals, no real i18n.
- QSettings appears only in WelcomeDialog.cpp (recently-opened projects).
- MorphWorker is a QObject defined inside MainWindow.cpp (lines 36-99), requires #include "MainWindow.moc" at line 1855.

## Qt Threading Pattern

QThread + moveToThread pattern for morph deformation:
- MainWindow.cpp:1491 creates QThread, MorphWorker moved to it
- Signals: progressUpdated, finished — connect back to MainWindow slots
- Replacement: std::thread + std::atomic + callback/future

## Rendering Layer Qt Coupling (Risk)

All 5 renderer classes inherit QOpenGLFunctions_4_3_Core as base:
- Renderer, ShaderManager, MeshRenderer, PointRenderer, GizmoRenderer
- Each calls initializeOpenGLFunctions() in constructor
- ShaderManager uses QFile/QTextStream for shader file loading
- Under GLAD, GL functions are global — base class and initializeOpenGLFunctions() both go away

## Domain Layer Qt Coupling (Risk)

QString used as cross-layer currency in:
- include/core/Mesh.h (Load/Save/GetName/GetFilePath)
- include/core/Project.h (QString + QDateTime in structs)
- include/core/TextureData.h (QString + QImage — ToQImage() used only by TexturePreviewDialog)
- include/core/UndoableAction.h (Description() returns QString)
- include/utils/Logger.h (entire API surface: QString, QFile, QMutex, QTextStream)
- include/utils/ErrorHelper.h (QString, QWidget* params)
- include/io/ProjectSerializer.h (QString, QJsonObject, QJsonArray)
- include/io/MeshExporter.h (QString)
- include/baking/TextureWriter.h (QString)

## Serialization

ProjectSerializer uses QJsonDocument/QJsonObject/QJsonArray/QFile throughout.
.mmproj format is plain JSON. nlohmann/json + std::fstream is the planned replacement.

## Dialog Contracts (Sprint 3.6.3 audit — 2026-04-21)

### ExportDialog (src/ui/ExportDialog.cpp)
- Modal QDialog; no signals emitted
- Public API: GetOptions() → ExportOptions, GetFilePath() → QString, SetTexturesAvailable(bool)
- Contains its own QFileDialog::getSaveFileName call (ExportDialog.cpp:134)
- Launch: MainWindow::OnExportMesh() at MainWindow.cpp:780

### WelcomeDialog (src/ui/WelcomeDialog.cpp)
- Modal QDialog; Result enum: NewProject / OpenProject / OpenRecent / Cancelled
- Public API: GetResult(), GetSelectedProjectPath(); static GetRecentProjects(), AddRecentProject()
- Contains QFileDialog::getOpenFileName at WelcomeDialog.cpp:220 (Open Project button)
- QSettings at lines 237,242,246 — key "recentProjects" (QStringList, max 5 MRU)
- Launch: MainWindow::ShowWelcomeDialog() via QTimer::singleShot(0,...) in MainWindow ctor (line 142)
- AddRecentProject called from MainWindow at lines 544, 565, 587, 1102

### ShortcutsDialog (src/ui/ShortcutsDialog.cpp)
- Modal read-only QDialog; hardcoded shortcut table; no signals, no output
- Launch: MainWindow::OnKeyboardShortcuts() at MainWindow.cpp:1151
- Content: 6 categories, ~21 shortcut rows

### TexturePreviewDialog (src/ui/TexturePreviewDialog.cpp)
- Non-modal (show(), WA_DeleteOnClose); no signals, no output
- Accepts (QString textureName, const TextureData& texture)
- Calls texture.ToQImage() — only consumer of that method; removing this dialog frees ToQImage()
- Launch: SidebarWidget.cpp:2156 — texture thumbnail click in Morph sidebar

## QFileDialog Call Sites (7 total — all → NFD)
- WelcomeDialog.cpp:220 — getOpenFileName, *.mmproj
- MainWindow.cpp:452 — getOpenFileName, *.mmproj
- MainWindow.cpp:575 — getSaveFileName, *.mmproj
- MainWindow.cpp:598 — getOpenFileName, mesh files
- MainWindow.cpp:675 — getOpenFileName, mesh files
- MainWindow.cpp:717 — getOpenFileName, image files
- ExportDialog.cpp:134 — getSaveFileName, format-specific

## QMessageBox Call Sites
- All via ErrorHelper.cpp (ShowError/ShowWarning/ConfirmAction) — 14 call sites in MainWindow.cpp
- Direct QMessageBox in MainWindow.cpp: lines 894, 1189, 1719, 1842
- Replacement: queued ImGui modal system (cannot show mid-frame synchronously)

## System Services (Sprint 3.6.4 audit — 2026-04-21)

### QSettings
- Only in WelcomeDialog.cpp:237,242,246
- Key: "recentProjects", value: QStringList of absolute paths, max 5 MRU
- Migration target: %APPDATA%\MetaVisage\config.json

### QDesktopServices::openUrl (3 call sites, MainWindow.cpp)
- Line 900: open export folder in Explorer (fromLocalFile)
- Line 1147: https://docs.metavisage.io
- Line 1156: https://github.com/cmershon2/MetaVisage/issues
- Replacement: ShellExecuteW for all three

### QLocalServer / QLocalSocket — ABSENT; net-new via CreateMutexW
### QNetworkAccessManager — ABSENT; net-new via WinHTTP or libcurl

### Qt Threading (all sites)
- morphThread_: MainWindow.cpp:1491-1514 (MorphWorker defined inline MainWindow.cpp:36-99)
- statsTimer_: MainWindow.cpp:133-135, QTimer 500ms stats tick
- QTimer::singleShot: MainWindow.cpp:142, deferred welcome dialog
- QProgressDialog: MainWindow.cpp:798, export progress
- QMutex: Logger.h:38, Logger.cpp:25,57 only
- QWaitCondition, QAtomicInt: ABSENT
- No texture thumbnail loader thread exists — only MorphWorker

### About Dialog
- Does NOT exist. Help menu has Documentation + Report Issue → both QDesktopServices::openUrl.

## ViewportWidget Deep Structure (Sprint 3.5 port target)

### Owns (must survive port)
- unique_ptr<Camera> — zero Qt deps, portable as-is
- unique_ptr<Renderer> — needs QOpenGLFunctions→GLAD rework
- All brush instances (SmoothBrush, GrabBrush, PushPullBrush, InflateBrush, MaskBrush)
- QComboBox* shadingModeCombo_ at (8,8) — becomes ImGui::Combo in viewport window
- QPoint lastMousePos_, transformStartPos_ — become plain int/float pairs

### GL context lifecycle
- initializeGL(): calls initializeOpenGLFunctions(), creates Renderer, glEnable(GL_MULTISAMPLE)
- paintGL(): renderer_->Render() then QPainter overlay pass (label, axis letters, point numbers)
- resizeGL(): only glViewport(0, 0, w, h)

### QPainter overlays in paintGL (lines 835–968) — must become ImGui draw-list calls
1. Viewport label text (e.g. "Target Mesh") — line 840–856
2. Orientation gizmo X/Y/Z axis letter labels with NDC projection — lines 862–904
3. Point number labels with WorldToScreen projection (Stage 2 only) — lines 907–968

### makeCurrent()/doneCurrent() call sites (GPU uploads from sculpt/mask strokes)
Lines: 194, 201, 209, 388, 420, 492, 532, 627, 681, 715
All on main thread — become no-ops under GLFW (context permanently current on main thread).

### Keyboard shortcuts in keyPressEvent (line 1209)
All via QKeyEvent::key() switch — no QShortcut objects:
- G/R/S: transform modes (Alignment stage only)
- X/Y/Z: axis constraints (when transform mode active)
- [/]: brush radius ±10% (TouchUp stage only) — emits BrushRadiusChanged
- Escape: cancel transform or deselect point
- Delete: remove selected point (PointReference stage) — emits PointDeleteRequested
- Home: camera reset
- 1/3/5/7: ortho views (MAIN ROW numbers, not numpad — critical GLFW distinction)
- 5: toggle perspective/ortho
- F: focus on mesh bounds

### QPoint member variable coupling
QPoint lastMousePos_ and QPoint transformStartPos_ (h:180–181) come transitively from QMouseEvent.
ApplyTransform(const QPoint& delta) (cpp:1436) takes QPoint. Must replace with plain struct { int x, y; } or glm::ivec2 when QMouseEvent is removed.

### Signals emitted by ViewportWidget (h:107–127)
TransformModeChanged, TargetTransformChanged, CameraChanged, PointPlaced, PointSelected,
PointDeleteRequested, BrushRadiusChanged, TransformApplied, SculptStrokeCompleted, MaskStrokeCompleted.
SculptStrokeCompleted, MaskStrokeCompleted, BrushRadiusChanged, TransformApplied are NOT connected
inside the viewport layer files — wired upstream in stage controllers or MainWindow.

### Dead code in ViewportContainer
OnLeftViewportClicked() and OnRightViewportClicked() are declared (h:50–51) and defined (container.cpp:162–168)
but are never connected via connect() anywhere in these files. Safe to remove.

### Camera orbit/pan/zoom
- MMB → orbit; Shift+MMB → pan; scroll → zoom
- Qt modifier check: event->modifiers() & Qt::ShiftModifier (line 1027)
- GLFW: check GLFW_KEY_LEFT_SHIFT state at time of MMB press
- Camera::Orbit/Pan/Zoom take plain floats — portable

### Dual viewport camera sync (ViewportContainer)
- CameraChanged signal → OnLeft/RightCameraChanged → SyncCameraFrom()
- syncing_ bool prevents re-entrant loops (lines 134–160)
- Cameras are COPIED, not shared pointer
- Splitter sizes: setSizes({w/2, w/2}) — no QSettings persistence — becomes ImGui 50/50 default

## Sprint 3.6 Regression Audit (as of 2026-04-23)

### Issue 1 — Sidebar Dock (AppWindow.cpp:380–393, StageSidebar.cpp:60–61)
DockingEnable is set and DockBuilder fires. Root cause: `io.IniFilename = nullptr` (line 122) means the corrected dock state is never persisted. `StageSidebar::Render` passes a fixed pixel size hint (`ImVec2(320, 520)`, `ImGuiCond_FirstUseEver`) which can override the proportional dock slot before dockspace geometry resolves, causing the window to land undocked and stay there.

### Issue 2 — Transform Gizmo (Viewport.cpp:208–244, StageSidebar.cpp:156–170)
G/R/S keys set `transformMode_` and the gizmo renders. LMB drag path is explicitly deferred — `isTransforming_` is never set true, `OnMouseButton` has no gizmo-hit logic (Viewport.cpp:226–227 comment). The sidebar DragFloat3 spinboxes (Position/Rotation/Scale) DO write to `project_->GetAlignmentData().targetMeshTransform` and work.

### Issue 3 — Font Size (AppWindow.cpp:116–141)
No `io.Fonts->AddFontFromFileTTF` call anywhere — falls back to ProggyClean 13px bitmap font. No `glfwGetWindowContentScale` / `io.FontGlobalScale` DPI scaling. Dark theme is correctly applied via `ApplyDarkTheme()`.

### Issue 4 — Texture Preview (Modals.cpp, StageSidebar.cpp:130–150)
`TexturePreviewDialog` was never ported. No `ImGui::Image`, no `ImTextureID`, no GL texture upload for preview in sidebar. Texture list renders as BulletText labels only.

### Issue 5 — Shading Mode (AppWindow.cpp:447–452, Viewport.cpp:50–57)
View > Shading Mode submenu exists but all items stubbed with `false, false` (display only). `Viewport::SetShadingMode()` and renderer plumbing are intact. Story 3.6.5 deferred — hook lands in `RenderMainMenuBar`.

### Issue 6 — Status Bar Cut Off (AppWindow.cpp:370–400)
`statusH` is correctly reserved from dockspace. However `ImGui::SetCursorPosY(ImGui::GetCursorPosY())` at line 398 is a no-op — cursor is not explicitly advanced to `avail.y - statusH` after `DockSpace()`, so `RenderStatusBar` renders at an incorrect Y position.

### Issue 7 — Default Head Orientation (AppWindow.cpp:747–765, src/rendering/Renderer.cpp)
`UseDefaultMorphMesh()` applies -90° X rotation to `MorphMesh.transform` (matching Qt era). Coordinate system did not change between sprints. Suspected cause: `Renderer::Render()` may not consume `MeshReference::transform` for the morph mesh's model matrix. Needs verification in Renderer.cpp.

## Sprint 3.5 Completed State (as of 2026-04-21)

**Live (new ImGui/GLFW shell):**
- `src/ui/AppWindow.cpp` — owns GLFW window, GLEW init, ImGui context, render loop (Run/RenderFrame/Shutdown)
- `src/ui/ViewportHost.cpp` — replaced ViewportContainer; dual-viewport layout via ImGui InvisibleButton regions
- `src/ui/Viewport.cpp` — replaced ViewportWidget; plain C++ with GLFW/ImGui callbacks

**Dead (still compile, never instantiated):**
- `src/ui/MainWindow.cpp` — Qt QMainWindow; all stage orchestration/undo lived here
- `src/ui/ViewportContainer.cpp` — Qt QSplitter dual-viewport
- `src/ui/ViewportWidget.cpp` — Qt QOpenGLWidget

**Key gaps entering Sprint 3.6:**
- `ImGuiConfigFlags_DockingEnable` NOT set (AppWindow.cpp:72 — only NavEnableKeyboard)
- No Project instance constructed in shell — ViewportHost::SetProject never called
- No UndoStack instantiated anywhere (was owned by dead MainWindow)
- No StageManager class exists — stage set directly via Project::SetCurrentStage()
- No application-level shortcut handling (Ctrl+Z/S/O etc.) — only viewport shortcuts in Viewport::OnKey
- Menu bar: clean slate — RenderFrame contains only debug HUD (AppWindow.cpp:221–233)
- GLEW is used (not GLAD) — note for 3.6 implementation

**3.6 scope** (next sprint):
- SidebarWidget, ExportDialog, ShortcutsDialog, WelcomeDialog, TexturePreviewDialog
- Menu bar, toolbar, status bar — extend AppWindow::RenderFrame
- File dialogs (QFileDialog → NFD), QSettings, QDesktopServices
- ErrorHelper (QMessageBox)

**Why:** The category B portable layers (core, rendering, io, utils) all have hidden Qt coupling that was not originally intended. The rendering layer's QOpenGLFunctions inheritance is the largest structural risk.

## Stage 1 (Alignment) UI Inventory — Qt Reference (for ImGui port)

### Import group buttons (SidebarWidget.cpp:201–252)
- "Import Morph Mesh (MetaHuman)" → emits ImportMorphMeshRequested
- "Use Default MetaHuman Head" → emits UseDefaultMorphMeshRequested
- "Import Target Mesh (Custom)" → emits ImportTargetMeshRequested

### Default MetaHuman head — single preset, no picker
- Hardcoded path: `<appDir>/assets/models/MetaHumanHeadLod0.obj`
- File exists at: `assets/models/MetaHumanHeadLod0.obj`
- After load, applies -90° X rotation: `Quaternion::FromAxisAngle({1,0,0}, -90.0f)` — MainWindow.cpp:651-653
- Then calls `camera->FocusOnBounds(bounds)` and re-evaluates CanProceedToNextStage

### Mesh Status group (SidebarWidget.cpp:255–268)
- Two QLabel* with objectNames "morphStatus" and "targetStatus" — red when unloaded, green when loaded

### Target Textures group (SidebarWidget.cpp:271–338)
- Dynamic list populated by `RefreshTextureList(const TextureSet&)`
- Preset slot names (offered in order, filtered if in use): "Albedo", "Normal Map", "Roughness", "AO", "Emissive", "Metallic"
- "+ Add Texture" button opens name picker dialog, emits `ImportTextureRequested(name)`
- Per-slot: emits `ImportTextureRequested(name)` and `RemoveTextureRequested(name)`
- importAlbedoButton_ and importNormalMapButton_ are nullptr — legacy signals not active
- project_->GetTargetTextures().textures is the backing store (vector of named TextureEntry)

### Transform Tools group (SidebarWidget.cpp:341–359)
- Static keybind hint label: "G - Move | R - Rotate | S - Scale / X/Y/Z - Constrain to axis / Esc - Cancel | Left-click + drag to apply"
- transformModeLabel_ initialized "Mode: None", updated by OnTransformModeChanged(TransformMode, AxisConstraint)

### Target Transform group (SidebarWidget.cpp:361–547)
- Pos XYZ spinboxes: range ±1000, 2dp, step 0.01
- Rot XYZ spinboxes: range ±360, 1dp, step 1, degree suffix
- Scale XYZ spinboxes: range 0.001–100, 3dp, step 0.01
- All 9 spinboxes → OnSpinBoxValueChanged() → emits TransformValuesChanged
- Reset Transform button → emits ResetTransformRequested
- NO axis lock checkboxes; NO snap; NO symmetry (symmetry is Stage 2 only)

### FocusOnBounds (camera framing after import)
- Qt path: camera->FocusOnBounds(bounds) after every successful Mesh::Load()
- MainWindow.cpp:624-626 (morph import), :665-667 (default), :698-699 (target)
- ImGui AppWindow::ImportMorphMeshFromDisk / ImportTargetMeshFromDisk are STUBS — only set filepath/isLoaded=false, no actual Mesh::Load() call, no FocusOnBounds. Must be fully implemented.

### CanProceedToNextStage — Stage 1 → Stage 2 (Project.cpp:36-40)
- Condition: morphMesh_.isLoaded && targetMesh_.isLoaded (both must be true)
- isLoaded set to false in current ImGui stubs — "Next Stage" will never enable until load is real

### Grid rendering
- Owned by Renderer (src/rendering/Renderer.cpp)
- Flag: showGrid_ (bool, defaults true in constructor)
- API: Renderer::SetShowGrid(bool) / GetShowGrid() — Renderer.h:29-30
- Fullscreen triangle approach: empty gridVAO_, gridVertexCount_=3, gl_VertexID driven
- Shaders: assets/shaders/grid.vert + grid.frag
- No Project or Config field gates it — purely runtime renderer flag

## Qt-isms Remaining in UI-Adjacent Code (Post-3.5)

- AppWindow.cpp:20 — #include <QString> for MV_LOG macros
- Viewport.h:4,68,167 — QString for viewportLabel_ member and setter
- ViewportHost.cpp:52–63 — QStringLiteral for viewport labels in SetDualMode (easy to convert to const char*)
- Project.h:8–9 — QString + QDateTime throughout; all Save/Load paths use QString
- UndoStack::UndoDescription/RedoDescription return QString
- Logger entire API: QString, QFile, QMutex, QTextStream
