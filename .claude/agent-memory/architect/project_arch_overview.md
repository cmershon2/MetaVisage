---
name: MetaVisage Architecture Overview
description: High-level layer map, Qt6::Core surface inventory (Sprint 3.7 audit), MorphWorker state, Undo/Redo wiring state, and key build system facts as of 2026-04-26
type: project
---

## Layer Map

```
Presentation  → src/ui/AppWindow.cpp, StageSidebar.cpp, Modals.cpp, ViewportHost.cpp, Viewport.cpp, FileDialogs.cpp
Application   → src/ui/AppWindow.cpp (stage orchestration, morph threading, undo/redo wiring)
Domain        → src/core/, src/deformation/, src/sculpting/, src/baking/
Infrastructure → src/rendering/, src/io/, src/utils/
```

Dead files still present (never instantiated, not in CORE_SOURCES):
- src/ui/MainWindow.cpp — Qt QMainWindow; contains old MorphWorker (QThread-based, lines 36–102)
- src/ui/ViewportContainer.cpp, ViewportWidget.cpp, SidebarWidget.cpp
- src/ui/WelcomeDialog.cpp, ExportDialog.cpp, ShortcutsDialog.cpp, TexturePreviewDialog.cpp

## MorphWorker — Sprint 3.7.4 status (ALREADY DONE in AppWindow)

AppWindow.cpp already ports MorphWorker to std::thread + std::atomic:
- `morphThread_` is `std::thread` (AppWindow.h:133)
- `morphRunning_`, `morphFinished_`, `morphProgress_` are `std::atomic<bool/float>` (h:134-136)
- `morphResultMutex_` is `std::mutex` (h:137)
- `StartMorph()` at AppWindow.cpp:1304 — spawns std::thread, wires MeshDeformer directly
- `CancelMorph()` at AppWindow.cpp:1394 — calls morphDeformer_->Cancel() (std::atomic<bool>)
- `PollMorphCompletion()` at AppWindow.cpp:1400 — joins thread each frame on finish
- `AcceptMorph()` at AppWindow.cpp:1451
- All four sidebar flags (processMorphRequested, cancelMorphRequested, acceptMorphRequested) ARE wired at AppWindow.cpp:480-482
- Story 3.7.4 is effectively DONE — only remaining issue: QStringLiteral in two errorModal_.Open() calls at AppWindow.cpp:1317-1318 and :1442-1445

## Qt6::Core Surface — Full Inventory (Sprint 3.7 audit)

### Headers with Qt symbols (LIVE, in CORE_SOURCES):

**include/core/Mesh.h** — QString in public API
- Line 5: #include <QString>
- Lines 19-20: Load(const QString&), Save(const QString&)
- Lines 29-30: GetName(), GetFilePath() return const QString&
- Line 41: SetName(const QString&)
- Lines 69-70: QString name_, filepath_ members

**include/core/Project.h** — QString + QDateTime
- Lines 8-9: #include <QString>, #include <QDateTime>
- Line 15: MeshReference::filepath is QString
- Lines 152-153: Save/Load take const QString&
- Lines 156-159: GetName/GetPath return const QString&; GetCreated/GetLastModified return const QDateTime&
- Lines 179-180: SetName/SetPath take const QString&
- Lines 187-190: projectName_, projectPath_ QString; created_, lastModified_ QDateTime

**include/core/TextureData.h** — QString throughout
- Line 5: #include <QString>
- Line 20: LoadFromFile(const QString& path)
- Line 40: GetFilePath() returns const QString&
- Line 49: QString filePath_ member
- Line 57: NamedTexture::name is QString
- Lines 66,74,82: TextureSet::Add/Remove/GetByName take const QString&

**include/core/UndoableAction.h**
- Line 4: #include <QString>
- Line 13: virtual QString Description() const = 0

**include/core/UndoStack.h** (via UndoableAction)
- Lines 21-22: UndoDescription()/RedoDescription() return QString

**include/core/UndoActions.h**
- All four action classes return QString from Description()
- PointPlacementUndoAction ctor takes const QString& description
- QString description_ member

**include/utils/Logger.h** — full Qt API surface
- Lines 4-6: #include <QString>, <QFile>, <QTextStream>
- Lines 17-18: Initialize(const QString&), Log(LogLevel, const QString&, ...)
- Line 21: GetLogFilePath() returns QString
- Lines 30-31: WriteToFile(const QString&), LevelToString returns QString
- Lines 33-34: QFile logFile_, QString logFilePath_ members
- Line 40: qint64 MAX_LOG_SIZE constant (Qt type)
- MV_LOG_* macros pass QString to Log()

**include/utils/Paths.h** — all 4 functions return QString
- Line 4: #include <QString>

**include/utils/HttpClient.h** — QString in all public methods
- Line 4: #include <QString>
- Lines 27,31,33,35-36: PostJson/PostJsonBearer/GetJsonBearer take const QString&

**include/utils/Config.h** — QString throughout
- Line 4: #include <QString>
- Lines 29-30,34-41,55-58: all token/path storage uses QString/std::optional<QString>

**include/utils/OpenUrl.h**
- Line 4: #include <QString>; OpenUrl(const QString&)

**include/io/ProjectSerializer.h** — QJsonObject/QJsonArray/QStringList/QString everywhere
- Lines 5-7: #include <QString>, <QJsonObject>, <QJsonArray>
- Line 13: SerializationResult::errorMessage is QString
- Line 14: SerializationResult::warnings is QStringList
- Lines 25,28: Save/Load take const QString&
- All 10 serialize helpers return QJsonObject / take QJsonObject& with QStringList&
- Path helpers return/take QString

**include/io/MeshExporter.h** — QString in structs + all methods
- Line 6: #include <QString>
- ExportOptions: ueNamingPrefix (QString), textureFormat (QString)
- ExportResult: errorMessage, exportedFilePath, bakedAlbedoPath, bakedNormalMapPath (all QString)
- ExportProgressCallback typedef uses QString
- Export(), ExportOBJDirect(), ExportWithAssimp(), etc. all take const QString&

**include/baking/TextureWriter.h** — WritePNG/WriteTGA take const QString&

**include/rendering/ShaderManager.h** — all methods take/return QString, map key is QString

**include/ui/AppWindow.h** — #include <QString>, projectPath_ is QString, private methods take const QString&

**include/ui/Viewport.h** — #include <QString>, viewportLabel_ is QString, SetViewportLabel takes const QString&

**include/ui/StageSidebar.h** — #include <QString>, Actions::importTextureSlot is QString

**include/ui/Modals.h** — #include <QString>; all modal structs (ConfirmModalState, ErrorModalState, WelcomeModalState, ExportModalState) have QString fields and Open() methods taking QString

**include/ui/FileDialogs.h** — #include <QString>; OpenFile/SaveFile return std::optional<QString>

**include/baking/TextureBaker.h** — BakeResult::errorMessage is QString

### Dead headers (never compiled in CORE_SOURCES, still in include/):
- include/utils/ErrorHelper.h — QWidget* params, QString throughout
- include/ui/MainWindow.h — QThread*, QTimer*, QString throughout
- include/ui/SidebarWidget.h, ViewportWidget.h, ViewportContainer.h — full Qt Widgets
- include/ui/WelcomeDialog.h, ExportDialog.h, ShortcutsDialog.h, TexturePreviewDialog.h

### Implementation files with Qt usage (LIVE in CORE_SOURCES):

**src/core/Mesh.cpp** — 13 MV_LOG_* calls with QString().arg(); #include <QFileInfo>; QFileInfo for baseName extraction at line 53

**src/core/Project.cpp** — QDateTime::currentDateTime() at lines 9,10,22

**src/core/UndoStack.cpp** — 4 MV_LOG_* calls with QString().arg(); UndoDescription/RedoDescription return QString

**src/core/TextureData.cpp** — 2 MV_LOG_* calls; LoadFromFile(const QString&)

**src/io/ProjectSerializer.cpp** — #include <QFile>, <QJsonDocument>, <QDir>, <QFileInfo>; entire file uses QFile + QJsonDocument + QJsonObject + QJsonArray + QDir + QFileInfo + QDateTime; ~95 Qt symbol uses across ~250 lines

**src/io/MeshExporter.cpp** — (not read in full but header implies QString throughout)

**src/baking/TextureBaker.cpp** — 7 MV_LOG_* with QString().arg()

**src/baking/TextureWriter.cpp** — 6 MV_LOG_* with QString; WritePNG/WriteTGA take const QString&; uses path.toUtf8().constData() for stb_image_write

**src/rendering/ShaderManager.cpp** — #include <QFile>, <QTextStream>, <QDebug>; QFile + QTextStream for shader file reading; all methods take QString; map key is QString

**src/rendering/GizmoRenderer.cpp** — #include <QDebug>; qDebug()/qWarning() for load logging; CWD-relative paths "assets/models/gizmos/..." at lines 34-36

**src/utils/Logger.cpp** — #include <QDir>, <QDateTime>, <QDebug>, <QFileInfo>; QDir/QFile/QTextStream/QDateTime/QFileInfo/qDebug throughout; entire file is Qt-dependent

**src/utils/Paths.cpp** — #include <QDir>; QDir for EnsureDir, QString::fromWCharArray for Windows API results

**src/utils/Config.cpp** — #include <QFile>, <QByteArray>; QFile + QByteArray for file I/O; conversion helpers QsToUtf8/Utf8ToQs throughout; stores tokens as plaintext in config.json

**src/utils/HttpClient.cpp** — 1 MV_LOG_WARNING with QString().arg(); url.toUtf8().toStdString() conversions at lines 54, 110, 116

**src/deformation/MeshDeformer.cpp** — 4 MV_LOG_* with QString().arg()

**src/deformation/NRICPDeformer.cpp** — 21 MV_LOG_* with QString().arg() (largest single consumer)

**src/deformation/RBFInterpolator.cpp** — #include <QDebug>

**src/ui/AppWindow.cpp** — #include <QFileInfo>, <QString>; ~25 MV_LOG_* with QString().arg(); QFileInfo for fileName() extraction at multiple sites; QStringLiteral at lines 1118, 1317, 1318, 1442, 1445

**src/ui/StageSidebar.cpp** — #include <QFileInfo>; QString FileNameOrEmpty helper; QFileInfo(path).fileName(); QString::fromUtf8 at line 155; morph.filepath.isEmpty() (QString API)

**src/ui/ViewportHost.cpp** — QStringLiteral for viewport labels at lines 52, 54, 60 (in SetDualMode)

## Build System

**CMakeLists.txt:**
- Line 38: find_package(Qt6 REQUIRED COMPONENTS Core)
- Line 217: Qt6::Core linked PUBLIC to MetaVisageCore static library
- No qt_add_executable, no qt_standard_project_setup, no AUTOMOC/AUTORCC/AUTOUIC
- No enable_testing(), no add_test(), no CTest infrastructure
- No CMake guard against Qt6::Gui/Widgets/OpenGL/OpenGLWidgets headers

**vcpkg.json:** No qt* dependency listed — Qt is a system install, NOT a vcpkg package.
This means "drop qtbase from vcpkg.json" is a no-op; the real change is find_package(Qt6) removal from CMakeLists.txt and the -DCMAKE_PREFIX_PATH=$QtPath removal from build-msvc.ps1.

**build-msvc.ps1:**
- Line 4: $QtPath = "C:/Qt/6.10.3/msvc2022_64" default parameter
- Line 41: -DCMAKE_PREFIX_PATH="$QtPath" in CMake args (needs removal)
- Lines 84-94: windeployqt invocation to deploy Qt6Core.dll (needs removal)
- No build-mingw.ps1 content read but presumably similar

## Sprint 3.6 Code Review Blockers — Current State

### CancelTransform Escape (Story 3.7.5a)
- Viewport.cpp:80-86: CancelTransform() sets isTransforming_=false, clears activeGizmoAxis_ and axisConstraint_, but the comment "3.5.4 restores transformBeforeState_" at line 85 is a TODO — the actual restoration of project_->GetTargetMesh().transform = transformBeforeState_ is NOT implemented.
- transformBeforeState_ IS captured at Viewport.cpp:272 on LMB press.
- OnKey at Viewport.cpp:493-499: Escape calls CancelTransform() — but CancelTransform doesn't restore the transform.
- BUG CONFIRMED: restore is missing from CancelTransform().

### HeaderCallback off-by-one (Story 3.7.5b)
- HttpClient.cpp:35: checks line.size() > 13 (should be >= 13 for "Retry-After: " = 12 chars + value)
- HttpClient.cpp:35-36: _strnicmp checks 12 chars ("Retry-After:"), then strtol starts at line.c_str() + 12 which is the space after the colon — strtol naturally skips leading whitespace, so the space is handled correctly
- BUT: the line captured from curl includes the CRLF ("\r\n"). strtol stops at '\r', so retryAfterSeconds gets correct digits. No off-by-one in the actual value parsed.
- HOWEVER: The sprint plan says "drops the last char" — this may refer to the fact that the full header line including \r\n is passed to strtol, and \r would terminate parsing. Need to verify if curl strips \r\n from header lines (it does NOT by default). If the value is "120\r\n", strtol("120\r\n") = 120 correctly. The "off-by-one" bug is that size > 13 check may silently skip lines where "Retry-After: " + value < 13 total chars (e.g. "Retry-After: 1" = 15 chars, fine). The actual documented bug may be different from what's apparent from reading — needs runtime verification.

### Gizmo asset paths (Story 3.7.5c)
- GizmoRenderer.cpp:34-36: LoadGizmoModel called with literal "assets/models/gizmos/move_gizmo.obj" etc. — CWD-relative, NOT via Paths::ExecutableDir()
- Paths::ExecutableDir() exists at include/utils/Paths.h:19 and is implemented
- BUG CONFIRMED: needs change to Paths::ExecutableDir() + "/assets/models/gizmos/..."

### Plaintext token storage (Story 3.7.5d)
- Config.cpp:96-102: accessToken_, refreshToken_, licenseJwt_ written as plaintext JSON strings in %APPDATA%\MetaVisage\config.json
- No CryptProtectData/CryptUnprotectData anywhere in the codebase
- No SecureStorage class exists
- BUG CONFIRMED: tokens are plaintext; DPAPI wrapper needed

## Undo/Redo Wiring — Current State (Story 3.7.7)

### UndoStack and action classes — all defined
- UndoStack: include/core/UndoStack.h — Push/Undo/Redo/CanUndo/CanRedo all implemented
- UndoableAction subclasses in include/core/UndoActions.h:
  - TransformUndoAction (line 18) — class exists, Undo/Redo implemented
  - PointPlacementUndoAction (line 42) — class exists, Undo/Redo implemented
  - MorphUndoAction (line 68) — class exists
  - SculptStrokeUndoAction (line 103) — class exists

### AppWindow onPointPlaced (AppWindow.cpp:171-278)
- Fully implemented point placement + symmetry mirror — DOES NOT push to undoStack_
- No PointPlacementUndoAction push anywhere in the lambda

### AppWindow onPointDelete (AppWindow.cpp:283-306)
- Fully implemented deletion — DOES NOT push to undoStack_

### Viewport::OnTransformApplied callback
- Defined in Viewport.h:131: std::function<void(Transform, Transform)> OnTransformApplied
- Called at Viewport.cpp:291-294 on LMB release when isTransforming_
- In AppWindow.cpp Initialize(): NO OnTransformApplied callback is wired to any viewport

### StageSidebar transform spinboxes (StageSidebar.cpp:168-179)
- DragFloat3 for Position/Rotation/Scale at lines 168/173/177
- No edit-end detection; no undo push per-frame or at deactivation
- ImGui::DragFloat3 returns true every frame during drag, not just on release

### AppWindow Edit menu (AppWindow.cpp:694-701)
- Undo/Redo menu items call OnEditUndo()/OnEditRedo() — wired
- HandleGlobalShortcut handles Ctrl+Z/Ctrl+Shift+Z — wired
- undoStack_->CanUndo/CanRedo gates menu items correctly
- BUT: no actions are ever pushed, so Ctrl+Z does nothing

## Sprint 3.5 Hygiene (Story 3.7.6)
- No tests/ directory exists
- No enable_testing() in CMakeLists.txt
- No CTest, gtest, catch2, doctest wiring
- No add_custom_target(qt_purity_check) or configure-time Qt header guard
- Both items are net-new work

## MV_LOG_* call counts per file (all use QString().arg() pattern):
- src/deformation/NRICPDeformer.cpp: 21 calls (largest)
- src/ui/AppWindow.cpp: 18 calls
- src/core/Mesh.cpp: 13 calls
- src/baking/TextureBaker.cpp: 7
- src/baking/TextureWriter.cpp: 6
- src/utils/HttpClient.cpp: 1
- src/core/UndoStack.cpp: 4
- src/deformation/MeshDeformer.cpp: 4
- src/ui/FileDialogs.cpp: 4
- src/core/TextureData.cpp: 2
- src/utils/SingleInstance.cpp: 2
- src/utils/Config.cpp: 2
- src/utils/OpenUrl.cpp: 2
- src/utils/ErrorHelper.cpp: 3
- src/io/ProjectSerializer.cpp: 3
- src/ui/Viewport.cpp: 1
- src/main.cpp: 6
- src/ui/MainWindow.cpp: 1 (dead file)
- Total across live files: ~99 calls

**Why:** Full audit for Sprint 3.7 Qt removal planning as of branch sprint/qt-decouple-and-deferred (clean, no uncommitted changes).
