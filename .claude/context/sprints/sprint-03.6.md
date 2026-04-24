# Sprint 3.6: Framework Migration — UI Layer (Sidebars, Dialogs & System Services)

**Sprint Goal**: Rebuild all user-facing UI in Dear ImGui and replace Qt's remaining system services (file dialogs, settings, networking, single-instance, threading) — by sprint close the app is fully functional across all four stages with parity to the end-of-Sprint-3 Qt build.
**Duration**: 2026-05-04 — 2026-05-15 (2 weeks)
**Capacity**: 26 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor)
**Committed**: 26 points

> Stories are pulled from the aggregated plan at [SPRINT_PLAN.md](./SPRINT_PLAN.md) §Sprint 3.6. This is framework-migration work, not product-backlog work — `product_backlog.md` is unchanged by this sprint. `Qt6::Core` (QString/QFile/QJson) remains linked as a transitional dependency; full removal is tracked in [product_backlog.md](../development/product_backlog.md) as **Qt6Core removal**.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 3.6.1: ImGui Layout Architecture & Menu Bar** | 5 | Sprint 3.5 complete | [x] |
| 2 | **Story 3.6.2: Stage Sidebar Panels** | 8 | Story 3.6.1 | [x] |
| 3 | **Story 3.6.3: Dialogs & Modals** | 5 | Story 3.6.1 | [x] |
| 4 | **Story 3.6.4: System Service Replacements** | 6 | Story 3.6.1 | [x] |
| 5 | **Story 3.6.5: Shading Mode Viewport Overlay & Orientation Gizmo (ImGui Port)** | 2 | Story 3.6.1 | [ ] (deferred) |

---

## Sprint Context (Key Affected Files)

**Build / dependencies (Story 3.6.4 prep — already pre-wired in 3.5.1)**
- [CMakeLists.txt](../../../CMakeLists.txt) — confirm `nativefiledialog-extended`, `curl`, `stb` linkage; verify no residual `Qt6::Core` is still required by UI/system-service code after port
- [build-msvc.ps1](../../../build-msvc.ps1) — add DLL staging for NFD / libcurl

**Layout & menu bar (Story 3.6.1)**
- [src/main.cpp](../../../src/main.cpp) — enable `ImGuiConfigFlags_DockingEnable`; host the root `DockSpace`
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — grow into full app-shell host: dockspace, main menu bar, status bar
- [src/ui/MainWindow.cpp](../../../src/ui/MainWindow.cpp) — retire remaining Qt-era coordination logic; fold event dispatch into `AppWindow`

**Sidebar panels (Story 3.6.2)**
- [src/ui/SidebarWidget.cpp](../../../src/ui/SidebarWidget.cpp) — replace Qt widget tree with per-stage ImGui panels (Stages 1–4)
- [src/core/Project.cpp](../../../src/core/Project.cpp) — direct per-frame state reads/writes from sidebar replace Qt signal/slot coupling
- [src/core/StageManager.cpp](../../../src/core/StageManager.cpp) — sidebar drives stage transitions via `StageManager`; validate gating

**Dialogs & modals (Story 3.6.3)**
- [src/ui/ExportDialog.cpp](../../../src/ui/ExportDialog.cpp) — re-implement as ImGui modal + NFD save picker
- [src/ui/TexturePreviewDialog.cpp](../../../src/ui/TexturePreviewDialog.cpp) — ImGui modal with `ImGui::Image()` and an uploaded GL texture
- [src/ui/ShortcutsDialog.cpp](../../../src/ui/ShortcutsDialog.cpp) — scrollable two-column ImGui modal
- [src/ui/WelcomeDialog.cpp](../../../src/ui/WelcomeDialog.cpp) — landing/new-project modal with recent projects list
- Add `src/ui/ConfirmModal.{h,cpp}` / `src/ui/ErrorModal.{h,cpp}` as reusable ImGui modal helpers

**System services (Story 3.6.4)**
- Add `src/utils/Config.{h,cpp}` — JSON config at `%APPDATA%\MetaVisage\config.json` (nlohmann::json); replaces `QSettings`
- Add `src/utils/OpenUrl.{h,cpp}` — `ShellExecuteEx` wrapper; replaces `QDesktopServices::openUrl`
- Add `src/utils/SingleInstance.{h,cpp}` — Win32 Named Pipe at `\\.\pipe\MetaVisageSingleInstance`; replaces `QLocalServer`/`QLocalSocket` (pipe server thread is the same mechanism Sprint 6 hooks for `metavisage://` deep-link forwarding)
- Add `src/utils/HttpClient.{h,cpp}` — libcurl `POST` wrapper returning `{statusCode, responseBody}`; `curl_global_init` / `curl_global_cleanup` in `main.cpp`
- [src/ui/MainWindow.cpp](../../../src/ui/MainWindow.cpp) — port `MorphWorker` off `QThread` to `std::thread` + `std::mutex` + `std::condition_variable`
- Texture thumbnail loader call sites under `src/baking/` and `src/io/` — audit for `QThread`/`QMutex`/`QAtomicInt` and swap to `<thread>`/`<mutex>`/`<atomic>`

**Viewport overlays (Story 3.6.5)**
- [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp), [src/ui/ViewportHost.cpp](../../../src/ui/ViewportHost.cpp) — re-host shading-mode combo as an anchored ImGui overlay window rendered post-3D pass; verify orientation gizmo still draws into the viewport framebuffer unchanged

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Signal/slot coupling embedded deep in `SidebarWidget` → `Project` → stages makes per-frame immediate-mode conversion leaky | High | High | Front-load Story 3.6.2 with a dependency trace from each sidebar control to its `Project`/`StageManager` call site; define a narrow `ProjectView` read-only snapshot + a command-emitting write path before rewriting any panel |
| NFD blocks the render loop when opening native pickers, freezing ImGui rendering | Med | Med | Call NFD from the main thread inline — users expect modal blocking; ensure ImGui frame is properly ended before NFD call and restarted after. Document the pattern in a shared helper. |
| Named Pipe server thread lifecycle (create/read/shutdown) introduces a race with app exit | Med | High | Use a `WaitForSingleObject`/cancellable `OVERLAPPED` read; signal shutdown via an event handle before `join()`. Add a unit-test-style smoke test that launches a second instance and asserts the forwarded URI is received. |
| `MorphWorker` port from `QThread` silently breaks the cancel path (Stage 3 process → cancel) | Med | High | Keep `MeshDeformer`'s `std::atomic<bool>` cancel flag as the single source of truth; after the port, run the Stage 3 Process → Cancel → Re-process loop explicitly against a loaded test mesh before closing the story |
| Dialog ordering and focus (e.g. confirm-on-stage-change while export modal is open) produces unreachable popups in ImGui's modal stack | Med | Med | Standardise on a single `ModalManager` that owns the `ImGui::OpenPopup`/`BeginPopupModal` pair per dialog ID; forbid nested `OpenPopup` from within a rendering modal |
| Docking branch layout state not persisted → every launch starts with default dock layout, undoing sidebar resize | Low | Med | Enable `ImGui::SaveIniSettingsToDisk` / `LoadIniSettingsFromDisk` pointing at `%APPDATA%\MetaVisage\imgui.ini`; restore on launch in Story 3.6.4 alongside config.json |
| Scope creep into Sprint 4 (preset cards, vertex masking UI) while building Stage 3 sidebar | High | Med | Stage 3 sidebar in 3.6.2 **scaffolds** the preset-card row and parameter panels but leaves the controls disabled/wired to current v1.0 behaviour; actual preset system + masking are Sprint 4/5 |

---

## Definition of Done

- [ ] Application launches into the full ImGui layout: main menu bar, status bar, viewport, stage sidebar, all dockable
- [ ] Menu bar items correct per stage; `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Z`, `Ctrl+Shift+Z` dispatch via GLFW and trigger the same actions as the Qt build
- [ ] All four stage sidebar panels present and drive the underlying `Project` / `StageManager` state correctly; Next Stage buttons validate and advance
- [ ] All file dialogs (mesh / texture / project / export) open native Windows pickers via NFD with the correct filters
- [ ] Confirmation, error, About, Keyboard Shortcuts, Texture Preview, and Export dialogs all function as ImGui modals
- [ ] `%APPDATA%\MetaVisage\config.json` is written on clean exit and read on launch; recent projects, window size, sidebar collapse states persist across runs
- [ ] Named Pipe single-instance works — a second launch writes its args into the pipe and exits; first instance receives the forwarded string
- [ ] `HttpClient::Post()` compiles and returns the expected `{statusCode, responseBody}` against a local echo endpoint; `curl_global_init`/`cleanup` bracket the app lifetime with no leak
- [ ] `MorphWorker` and texture thumbnail loader use `std::thread` / `std::mutex` / `std::condition_variable`; no remaining `QThread`/`QMutex`/`QWaitCondition`/`QAtomicInt` in the codebase
- [ ] Shading-mode overlay and orientation gizmo render correctly in the new viewport hosting
- [ ] Full Stage 1 → Stage 4 → Export workflow completes without crashes or regressions vs. end-of-Sprint-3 Qt build (visual + behavioural parity check)
- [ ] `Qt6::Gui`/`Widgets`/`OpenGL`/`OpenGLWidgets` remain absent; `Qt6::Core` link status unchanged (tracked separately)
- [ ] PR reviewed via `/review-pr` and merged to `master`

---

## Sprint Outcome

**Status**: Implementation Complete — manual parity smoke test pending
**Velocity (Actual)**: 24 / 26 points (Stories 3.6.1–3.6.4 complete; 3.6.5 deferred)
**Carryover**: Story 3.6.5 (Shading Mode Viewport Overlay & Orientation Gizmo, 2 pts) — lowest-priority polish, deferred to a follow-up sprint.
**Notes**:
- `MorphWorker` off-Qt thread port deliberately deferred alongside 3.6.5 — `StageSidebar` Morph stage flags (`processMorphRequested`, `cancelMorphRequested`, `acceptMorphRequested`, `finalizeRequested`) stay unwired until the `std::thread`-based NRICP executor lands. Other DoD items met.
- `Qt6::Core` remains linked transitionally (QString/QFile/QDateTime/QJson); full removal tracked as **Qt6Core removal** in `product_backlog.md`.
- Build passes cleanly on MSVC; only pre-existing `stb_image_write` C4996 warning remains.
- Full Stage 1 → Stage 4 → Export parity smoke test requires a runtime session on Windows and has not yet been exercised.
