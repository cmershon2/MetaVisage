---
name: Codebase Conventions
description: Established patterns, anti-patterns, and recurring review notes observed across Sprints 3.5–3.7 PRs
type: project
---

## Qt Removal Complete (Sprint 3.7 / PR #20)

As of PR #20 (Sprint 3.7), `Qt6::Core` is fully removed. All public APIs use `std::string` / `std::filesystem` / `std::chrono` / `std::ofstream`. Do NOT flag any remaining `QString` references as issues — the PR deletes all of them. The `MainWindow.cpp`, `SidebarWidget.cpp`, `ViewportWidget.cpp`, `ViewportContainer.cpp`, `ExportDialog.cpp`, `ShortcutsDialog.cpp`, `TexturePreviewDialog.cpp`, `WelcomeDialog.cpp`, and `ErrorHelper.cpp` files still exist on disk but no longer compile and are excluded from CORE_SOURCES. A follow-up purge will delete them outright.

## C++ Standard

CMakeLists.txt now requires C++20 (`CMAKE_CXX_STANDARD 20`). `std::format` is used pervasively for log messages and error strings throughout the codebase as of Sprint 3.7.

## Undo/Redo Architecture (Sprint 3.7)

All five mutation sites now push to `UndoStack`:
- Point placement / deletion: `PointPlacementUndoAction` pushed in `AppWindow::Initialize` lambdas
- Gizmo drag: `TransformUndoAction` pushed via `Viewport::OnTransformApplied` callback at LMB release
- Transform spinboxes: `IsItemActivated` / `IsItemDeactivatedAfterEdit` in `StageSidebar::RenderAlignment` capture one undo per gesture (not per frame) via `transformEditCapture_` member
- Reset Transform: `TransformUndoAction` pushed before applying reset in the `Run()` frame loop
- Mask stroke / Sculpt stroke: `MaskStrokeUndoAction` / `SculptStrokeUndoAction` pushed via callbacks

Every `UndoableAction` subclass also captures `originatingStage_` and restores it on both `Undo` and `Redo` — this is intentional to teleport the user back to the stage where the action was performed.

**Known limitation (PR #20, not yet fixed):** `SculptStrokeUndoAction` holds a raw `Mesh*` from `deformedMorphMesh.get()`. `StartMorph()` replaces `deformedMorphMesh` without clearing the undo stack, which leaves a dangling pointer in any sculpt undo actions pushed before the re-morph. The undo stack must be cleared in `StartMorph()` before `morphData.isProcessed = false` to prevent this UB.

## DPAPI Token Encryption (SecureStorage, Sprint 3.7)

`utils/SecureStorage.{h,cpp}` wraps Win32 `CryptProtectData`/`CryptUnprotectData` with base64 output for JSON safety. `Config` uses it to encrypt `access_token`, `refresh_token`, and `license_jwt` at rest. `pOptionalEntropy` is `nullptr` — only the Windows user account key is used, NOT the machine ID + license key the spec (`DESKTOP_APP_INTEGRATION.md`) requires. This is an intentional deferral; the spec's machine-binding requirement is not yet met. Do not re-flag the `nullptr` entropy as a bug unless the spec compliance gap is explicitly being closed.

Legacy-plaintext migration is handled in `Config::ReadEncryptedString`: pre-DPAPI tokens (starting with "ey") are accepted as-is and re-encrypted on the next `Save()`.

## Qt Purity Guard (CMakeLists.txt, Sprint 3.7)

The `foreach` guard at the top of `CMakeLists.txt` fires `FATAL_ERROR` if any `Qt6::*` target is defined. It runs BEFORE all `find_package()` calls, so it catches: (a) Qt pre-loaded from a toolchain/parent scope, and (b) a stray `find_package(Qt6)` added near the top. It does NOT catch a Qt re-introduction via a transitive `find_package(X)` later in the file. This is a known limitation and is non-blocking.

## GL State Save/Restore Pattern

`Viewport::RenderGL` saves scissor, blend, depth, cull, VAO, program, and viewport state before the 3D pass and restores them after. This is the established pattern for this codebase.

## GLFW Callback Install Pattern

`install_callbacks=false` on `ImGui_ImplGlfw_InitForOpenGL` with manual forwarding. Do not flag this as bypassing ImGui callbacks.

## sDockBuilt Static-Local

`AppWindow::RenderAppShell()` guards DockBuilder setup with `static bool sDockBuilt`. Fires once per process — `io.IniFilename = nullptr` is set so there is no ini file to fight. Do not flag.

## Morph Worker Lifetime Pattern

`AppWindow::StartMorph` launches `morphThread_` as `[this]()` lambda. Join in `Shutdown()` correctly cancels then joins before `project_` reset. `PollMorphCompletion` joins on the main thread. This pattern is safe.

## ImGui Popup Style Var Count

`ViewportHost::BuildLayout` pushes 3 style vars before `ImGui::Begin` and pops them after `ImGui::End`. Pop-count must match push-count — verify whenever this pattern appears.

## Scale Z-Axis Inversion in Gizmo Drag

`sfInv` (inverted scale) for Z-axis constraint in `Viewport::OnMouseMove`. Intentional UX choice — do not flag.

## WideToUtf8 Helper

Defined in two separate anonymous namespaces: `src/utils/FileDialogs.cpp` and `src/utils/Paths.cpp`. No ODR violation (both are anonymous-namespace-scoped). The `Paths.cpp` version accepts an explicit `wideLen` parameter to handle `GetModuleFileNameW`'s non-null-terminated output (length excludes null); the `FileDialogs.cpp` version always null-terminates. Do not flag the duplication.

## Smoke Test Asset Resolution

`tests/smoke/launch_to_export.cpp` uses `Paths::ExecutableDir()` then probes five candidate paths for `assets/models/MetaHumanHeadLod0.obj` (local to exe, then relative to `bin/Release`, `bin/Debug`). The `add_test` WORKING_DIRECTORY is `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (build/bin). With MSVC config-subdirs (build/bin/Release/), the exe is at build/bin/Release and `candidate[0]` = build/bin/Release/assets finds the staged assets. This is correct.

## Debug HUD

`AppWindow::RenderFrame` contained a "Debug HUD" ImGui window — it was present in Sprint 3.6, now removed in Sprint 3.7 as part of the cleanup.

## ProjectSerializer Uses ordered_json

`nlohmann::ordered_json` (not `nlohmann::json`) is intentional in `ProjectSerializer` so `.mmproj` key order stays stable and v1.0 round-trips remain byte-clean. Do not suggest switching to unordered json.

## Sprint 3.8 Design System Rules (PR #21)

`MvStyle` owns the design palette. New code may NOT use raw `IM_COL32` or `ImVec4(r,g,b,a)` literals in stage/modal/overlay code — look up an `MvStyle::*` token. **Exceptions that are explicitly sanctioned:**
- `IM_COL32(0x05, 0x13, 0x22, 0xFF)` / `ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f)` — "dark text on bright blue" used for active-button labels. This colour is `MvBg0` but the context is "text atop filled MvBlue500 button" and it appears in multiple draw-list paths that predate/run before the style is applied. PR #21 has 8 occurrences. Flag if it proliferates further — the right fix is `MvStyle::MvBg0` or a named `MvTextOnBlue` token.
- `IM_COL32(0xE8, 0x11, 0x23, 0xFF)` — Windows close-button red, intentional OS-convention colour (only in `RenderTitleBar`).
- `IM_COL32(0, 0, 0, 80)` — drop shadow black. Acceptable as it has no design-token equivalent.

## Frameless Window (Sprint 3.8, PR #21)

`GLFW_DECORATED = GLFW_FALSE` + Win32 `WM_NCHITTEST`/`WM_NCCALCSIZE` subclass via `SetWindowLongPtr`. The global `g_originalWndProc` is file-scoped in `AppWindow.cpp` — safe for the single-window architecture but would need refactoring if a second window is ever created. `NOMINMAX` / `WIN32_LEAN_AND_MEAN` must remain at the very top of `AppWindow.cpp` (before any include that transitively pulls `<windows.h>`).

## ownsChrome Branch in StageSidebar (Sprint 3.8, PR #21)

The `ownsChrome` bool in `StageSidebar::Render` is always true (all four `WorkflowStage` values are listed). The legacy generic header/footer code in the `!ownsChrome` branches is dead code. It exists as a guardrail against future stages being added before the design is ready. Do not flag as a bug; flag as dead code if it still exists in Sprint 4.

## FormatRelativeTime uses clock_cast (Sprint 3.8, PR #21)

`Modals.cpp::FormatRelativeTime` uses `std::chrono::clock_cast<system_clock>` to convert `file_time_type` to `system_clock::time_point`. This is C++20 and correct for the codebase's stated C++20 standard. MSVC 2022 supports it. Do not flag.

## Static-Local sSelectedSlot (Sprint 3.8, PR #21)

`static int sSelectedSlot = 0` in `StageSidebar::RenderAlignment` is an ImGui pattern — the combo selection persists across frames within the session without needing a member variable. This matches the pre-existing `static const char* kSlotNames[]` in the same function. Acceptable for a per-session UI value, but worth noting if state needs to be serialized/reset between projects.

## DeletePointAt_ lambda (Sprint 3.8, PR #21)

`deletePointAt_` (`std::function<void(int)>` member) is initialized in `Initialize()`. Invoked both by the Delete-key path lambda and by the sidebar's per-row X click dispatch. Both invocations are on the main UI thread — no concurrency risk.

## ImGui CalcTextSize Inside ScopedFont (Sprint 3.8, PR #21)

Several call sites use `ImGui::CalcTextSize(str)` without a surrounding `ScopedFont` — in those paths the measurement is taken against the active (default) font, not the font used to render. This can cause minor horizontal alignment drift in the status bar and breadcrumb. Not a P0 but worth watching as more chrome lands.
