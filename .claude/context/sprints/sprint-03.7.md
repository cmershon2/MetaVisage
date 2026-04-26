# Sprint 3.7: Qt Decoupling & Deferred Code Review Cleanup

**Sprint Goal**: Remove the transitional `Qt6::Core` dependency entirely (QString/QFile/QJson/QDateTime/QThread → std equivalents), close out the four `/review-pr` blockers carried over from Sprint 3.6, land the smoke-test fixture and CMake guard deferred from Sprint 3.5, and re-wire Undo/Redo on the ImGui port so Sprint 4 (Wrap Engine) starts on a clean platform.
**Duration**: 2026-04-27 — 2026-05-08 (2 weeks)
**Capacity**: 26 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor)
**Committed**: 26 points

> Stories pulled from [product_backlog.md](../development/product_backlog.md) (Qt6Core removal, Re-wire Undo/Redo) and the Sprint 3.6 outcome notes (MorphWorker port, Story 3.6.5 carryover) plus the four `/review-pr` blockers and deferred Sprint 3.5 hygiene items called out in [product-vision.md](../product-vision.md). On completion, the project ships with **zero Qt link dependencies** — Sprint 4 begins on a fully self-hosted ImGui + GLFW + std stack.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 3.7.1: Qt6::Core Removal — Core Domain Layer (QString → std::string)** | 5 | Sprint 3.6 complete | [ ] |
| 2 | **Story 3.7.2: Qt6::Core Removal — I/O Layer (QFile / QJson → std::fstream + nlohmann::json)** | 3 | Story 3.7.1 | [ ] |
| 3 | **Story 3.7.3: Qt6::Core Removal — Logging, Utilities & Build System** | 4 | Story 3.7.1 | [ ] |
| 4 | **Story 3.7.4: MorphWorker Threading Port (QThread → std::thread)** | 3 | Story 3.7.3 | [ ] |
| 5 | **Story 3.7.5: Sprint 3.6 Code Review Blockers** | 5 | None (parallelizable) | [ ] |
| 6 | **Story 3.7.6: Sprint 3.5 Hygiene Carryover (Smoke Test + CMake Guard)** | 3 | Story 3.7.3 | [ ] |
| 7 | **Story 3.7.7: Re-wire Undo/Redo on the ImGui Port** | 3 | None (parallelizable) | [ ] |

---

## Story Detail

### Story 3.7.1: Qt6::Core Removal — Core Domain Layer (5 pts)

Port the public API of every `core/` type that currently embeds `QString` to `std::string` (UTF-8 in, UTF-8 out). This is the load-bearing change — all downstream call sites depend on it.

- Strip `QString` from `core/Mesh`, `core/Project`, `core/TextureData`, `core/UndoStack`, `core/UndoableAction`
- Replace `QString::arg()` formatting with `std::format` (C++20) or `std::ostringstream`
- Convert any `QStringList` to `std::vector<std::string>`
- Path arguments switch to `std::filesystem::path` at API boundaries

### Story 3.7.2: Qt6::Core Removal — I/O Layer (3 pts)

- `io/ProjectSerializer`: replace `QFile` + `QJsonDocument` with `std::ifstream`/`std::ofstream` + `nlohmann::json` (already linked since 3.6)
- `io/MeshExporter`: same treatment for FBX/OBJ writes; keep Assimp call signatures unchanged
- Verify `.mmproj` files written by v1.0/v2.0-pre-3.7 still load (forward-compat regression test)

### Story 3.7.3: Qt6::Core Removal — Logging, Utilities & Build System (4 pts)

- `MV_LOG_*` macro family: drop the `QString(...).arg(...)` pattern across `deformation/*`, `baking/*`, `utils/*`
- Replace `QDateTime` timestamp use sites with `std::chrono::system_clock`
- `CMakeLists.txt`: remove `find_package(Qt6 ... Core)` and `Qt6::Core` link target; remove the `windeployqt` invocation in `build-msvc.ps1`
- Confirm `vcpkg.json` no longer references `qtbase`

### Story 3.7.4: MorphWorker Threading Port (3 pts)

The Stage 3 deformation worker is the last `QThread` user — Sprint 3.6 left `StageSidebar` Morph flags (`processMorphRequested`, `cancelMorphRequested`, `acceptMorphRequested`, `finalizeRequested`) unwired pending this port.

- Port `MorphWorker` from `QThread` to `std::thread` + `std::condition_variable` + `std::atomic<bool>` cancel flag
- Wire the four Morph stage flags in `AppWindow` to the new worker lifecycle
- Run the Process → Cancel → Re-process loop end-to-end against a real loaded mesh before closing the story (this is the parity check Sprint 3.6 deferred)

### Story 3.7.5: Sprint 3.6 Code Review Blockers (5 pts)

Four `/review-pr` blockers from the Sprint 3.6 PR review:

- **CancelTransform Escape revert** — pressing `Esc` mid-drag must restore the captured pre-state in `Viewport::OnMouseButton` / `OnKey`, not just clear `isTransforming_`
- **`HeaderCallback` off-by-one** — `utils/HttpClient` header-line trim bug; `Retry-After` parsing currently drops the last char
- **Gizmo asset paths via `Paths::ExecutableDir()`** — `rendering/GizmoRenderer` still uses CWD-relative paths; switch to `Paths::ExecutableDir() / "assets/gizmos/..."` so installed builds load correctly
- **Plaintext token-storage encryption gap** — license JWT and Supabase tokens are written to `%APPDATA%` in cleartext; wrap with Win32 DPAPI (`CryptProtectData` / `CryptUnprotectData`) keyed to the current user

### Story 3.7.6: Sprint 3.5 Hygiene Carryover (3 pts)

- Smoke-test fixture: minimal "boots, loads default MetaHuman, advances Stage 1→2→3→4, exports FBX" headless harness used by CI
- CMake configure-time guard: fail the configure if any `Qt6::Gui` / `Qt6::Widgets` / `Qt6::OpenGL` / `Qt6::OpenGLWidgets` header path leaks back in (e.g. via vcpkg dep transitivity)

### Story 3.7.7: Re-wire Undo/Redo on the ImGui Port (3 pts)

Backlog blocker for Sprint 4 — NRICP work needs reliable undo. `UndoStack` and the Edit menu exist but no ImGui mutation site pushes actions:

- `AppWindow::onPointPlaced` / `onPointDelete` — push `PointPlacementUndoAction` (port from `MainWindow::OnPointPlaced` line 1274)
- `Viewport::OnMouseButton` release path — fire `OnTransformApplied` callback wired to a new `TransformUndoAction`
- `StageSidebar::RenderAlignment` — transform spinbox commits push undo at edit-end, not per-keystroke

---

## Sprint Context (Key Affected Files)

**Core domain layer (Story 3.7.1)**
- [include/core/Mesh.h](../../../include/core/Mesh.h), [src/core/Mesh.cpp](../../../src/core/Mesh.cpp)
- [include/core/Project.h](../../../include/core/Project.h), [src/core/Project.cpp](../../../src/core/Project.cpp)
- [include/core/TextureData.h](../../../include/core/TextureData.h)
- [include/core/UndoStack.h](../../../include/core/UndoStack.h), [src/core/UndoStack.cpp](../../../src/core/UndoStack.cpp)
- [include/core/UndoableAction.h](../../../include/core/UndoableAction.h)

**I/O layer (Story 3.7.2)**
- [include/io/ProjectSerializer.h](../../../include/io/ProjectSerializer.h), [src/io/ProjectSerializer.cpp](../../../src/io/ProjectSerializer.cpp)
- [include/io/MeshExporter.h](../../../include/io/MeshExporter.h), [src/io/MeshExporter.cpp](../../../src/io/MeshExporter.cpp)

**Logging, utilities & build (Story 3.7.3)**
- [include/utils/Logger.h](../../../include/utils/Logger.h), [src/utils/Logger.cpp](../../../src/utils/Logger.cpp) — `MV_LOG_*` macro definitions
- All `MV_LOG_*` call sites under [src/deformation/](../../../src/deformation/), [src/baking/](../../../src/baking/), [src/utils/](../../../src/utils/)
- [CMakeLists.txt](../../../CMakeLists.txt) — drop `Qt6::Core` find/link
- [build-msvc.ps1](../../../build-msvc.ps1) — drop `windeployqt`
- [vcpkg.json](../../../vcpkg.json) — drop `qtbase`

**MorphWorker port (Story 3.7.4)**
- [src/ui/MainWindow.cpp](../../../src/ui/MainWindow.cpp) — `MorphWorker` class
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — wire Morph stage flags into worker
- [src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp) — flag emission already present from Sprint 3.6
- [src/deformation/MeshDeformer.cpp](../../../src/deformation/MeshDeformer.cpp) — already uses `std::atomic<bool>` cancel; keep as-is

**Code review blockers (Story 3.7.5)**
- [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp) — `OnMouseButton` / `OnKey` for Escape revert; `transformBeforeState_` capture exists from Sprint 3.7 in-flight work
- [src/utils/HttpClient.cpp](../../../src/utils/HttpClient.cpp) — `HeaderCallback` off-by-one
- [src/rendering/GizmoRenderer.cpp](../../../src/rendering/GizmoRenderer.cpp), [include/utils/Paths.h](../../../include/utils/Paths.h) — `Paths::ExecutableDir()` usage
- New file: `src/utils/SecureStorage.{h,cpp}` (DPAPI wrapper); call sites in any token-persistence code under `utils/`

**Sprint 3.5 hygiene (Story 3.7.6)**
- New file: `tests/smoke/launch_to_export.{cpp,ps1}`
- [CMakeLists.txt](../../../CMakeLists.txt) — `add_custom_target(qt_purity_check ...)` invoked at configure time

**Undo/Redo wiring (Story 3.7.7)**
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — `onPointPlaced`, `onPointDelete` lambdas
- [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp) — `OnMouseButton` release path; new `OnTransformApplied` callback consumer in `AppWindow`
- [src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp) — `RenderAlignment` transform spinbox edit-end detection

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `QString` ↔ `std::string` UTF-8 round-trip introduces silent encoding regressions on non-ASCII project paths or texture filenames | High | High | Centralize conversion in a single `core/StringConversion.h` helper that uses `QString::toStdString()` only at the wholesale port boundary, then audit each public method with a unicode-path test (Cyrillic + Japanese fixture) before deleting `QString` from that header |
| Removing `Qt6::Core` from CMake breaks the build in a hard-to-bisect way (transitive includes via `<QtCore/...>` lurking in untouched files) | High | Med | Land Story 3.7.3's CMake guard *first*, before Stories 3.7.1/3.7.2; the guard turns each missed include into a clear configure-time error rather than a cascading link failure |
| `nlohmann::json` does not preserve key ordering by default → `.mmproj` diffs become noisy and v1.0 round-trip tests look like real changes | Med | Low | Use `nlohmann::ordered_json` for the project serializer; add a round-trip test that loads a v1.0.0 `.mmproj`, saves, and asserts byte-identical output |
| `MorphWorker` `std::condition_variable` cancel race — Process → Cancel mid-iteration leaves the deformer in a half-applied state that v1.0 `QThread::quit()` happened to cover | Med | High | Reuse the existing `std::atomic<bool>` cancel flag in `MeshDeformer` as the ground truth; the worker thread only owns the lifecycle, not the cancel signal. Run a stress test of 20 Process/Cancel cycles before closing the story |
| DPAPI token encryption (Story 3.7.5) fails silently on some Windows configurations (machine-store vs user-store), locking returning users out of cached licenses | Low | High | Use `CRYPTPROTECT_UI_FORBIDDEN` and treat any decrypt failure as "no cached token" rather than throwing — the user falls back to the browser auth flow rather than a hard error. Add a one-shot migration that reads any pre-existing plaintext token, encrypts it, and rewrites |
| Undo/Redo wiring (Story 3.7.7) changes `Viewport` mouse handlers that Story 3.7.5's Escape-revert work also touches → merge conflicts within the sprint | Med | Low | Land Story 3.7.5's `Viewport.cpp` changes first since they're smaller and well-scoped, then rebase Story 3.7.7 on top |
| Smoke-test fixture (Story 3.7.6) needs a headless GLFW context, but the test machine may not have a discrete GPU available for CI | Low | Med | Use the OSMesa software renderer fallback that GLFW supports via `GLFW_CONTEXT_CREATION_API=GLFW_OSMESA_CONTEXT_API`; document the GPU-vs-software path split in the test README |

---

## Definition of Done

- [ ] `vcpkg.json` no longer lists any `qt*` packages
- [ ] `CMakeLists.txt` contains zero `Qt6::*` link targets and zero `find_package(Qt6 ...)` calls
- [ ] No source or header file under `src/` or `include/` contains a `<Qt...>` include or a `QString`/`QFile`/`QJson*`/`QDateTime`/`QThread`/`QMutex`/`QAtomicInt` symbol
- [ ] CMake configure-time guard fails if any future change re-introduces a `Qt6::Gui`/`Widgets`/`OpenGL`/`OpenGLWidgets` header path
- [ ] `MetaVisage.exe` ships without `Qt6Core.dll` in the deploy folder; total installer payload shrinks by ≥20 MB
- [ ] Stage 3 Process → Cancel → Re-process → Accept cycle works end-to-end against a real loaded mesh (the parity test Sprint 3.6 deferred)
- [ ] All four Sprint 3.6 `/review-pr` blockers resolved with explicit verification: Escape mid-drag restores transform, libcurl `Retry-After` header parses correctly under unit test, gizmos load in installed (non-CWD) build, license JWT cache file is unreadable when copied to a different user account
- [ ] Smoke-test fixture runs in CI and asserts a successful Stage 1→4→Export workflow on a fixture mesh
- [ ] Undo/Redo: point placement, point deletion, gizmo drag, and transform spinbox commits all push to `UndoStack` and `Ctrl+Z` / `Ctrl+Shift+Z` correctly reverts each
- [ ] A v1.0.0 `.mmproj` file loads, edits, and saves without data loss; saved bytes round-trip identically when no edits were made
- [ ] PR reviewed via `/review-pr` and merged to `master`

---

## Sprint Outcome

**Status**: Planned
**Velocity (Actual)**: TBD
**Carryover**: TBD
**Notes**: TBD — Sprint 4 (Wrap Engine Redesign) requires this sprint's Undo/Redo and zero-Qt baseline as preconditions.
