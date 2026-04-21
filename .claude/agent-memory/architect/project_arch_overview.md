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

## Sprint 3.5 vs 3.6 Boundary

**3.5 scope** (this sprint):
- main.cpp, MainWindow, ViewportWidget, ViewportContainer
- Rendering layer (QOpenGLFunctions_4_3_Core → GLAD)
- Logger (used before window opens)
- MorphWorker threading

**3.6 scope** (next sprint, do not touch):
- SidebarWidget, ExportDialog, ShortcutsDialog, WelcomeDialog, TexturePreviewDialog
- Menu bar, toolbar, status bar
- File dialogs (QFileDialog), QSettings, QDesktopServices
- ErrorHelper (QMessageBox)

**Why:** The category B portable layers (core, rendering, io, utils) all have hidden Qt coupling that was not originally intended. The rendering layer's QOpenGLFunctions inheritance is the largest structural risk.
