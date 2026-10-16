---
name: Codebase Conventions
description: Established patterns, anti-patterns, and recurring review notes observed across Sprints 3.5–5 PRs
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

## Sprint 4 — WrapPreset / PresetStore Patterns

- `WrapPreset::activePresetId_` in `StageSidebar` is NOT explicitly reset on New Project / Open Project. This is intentional: the auto-derive logic at the top of `RenderMorph` re-derives the id from param equality each frame when `activePresetId_` is empty, and `OnFileNewProject` applies QuickWrap to the fresh project so the first frame of Stage 3 auto-selects "quick". If this invariant ever breaks (e.g., a new project path forgets to apply a preset), the card highlight would just be blank until the user clicks one.

- `PresetStore::Save` uses `out << root.dump(2)` without checking `out.fail()` after the write — matching the pre-existing `ProjectSerializer` pattern. Both silently return `true` on a partial write caused by a full disk. Do not flag as new; flag if either site is ever fixed without fixing both.

- `PresetStore` uses `preset.id` directly in the file path construction (`fs::path(dir) / (preset.id + kFileExtension)`) in both `Save` and `Delete`. IDs generated by `GenerateUserPresetId` are of the form `user-XXXXXXXX` (hex digits only, no path separators). IDs loaded from disk are read from the JSON `"id"` field without validation — a handcrafted preset file with `"id": "../evil"` would allow writing or deleting files one level above the presets dir. The attack surface is user-local (APPDATA), not a remote vector.

- Schema version check in `PresetStore::LoadAll` uses exact equality (`version != kSchemaVersion`) instead of `version > kSchemaVersion`. This means a downgraded app (older binary, newer preset file) silently skips the preset with a WARNING rather than hard-failing. Intentional per the "forward-compat" comment.

- `WrapPreset::BuiltIns()` copies the two static presets by value on every `RenderMorph` call (~60fps). Not measurable, but worth noting if preset list ever grows large.

- `SavePresetModalState::takenNames` does NOT include the built-in names ("Quick Wrap", "Fantasy Wrap"). A user can create a preset named "Quick Wrap". This is documented in the `Modals.h` comment as intentional (id-keyed, not name-keyed). Do not flag.

- `nricpAlphaFinal` (and three other fallback defaults — `nricpIcpIterations`, `nricpSamplingInitial`, `nricpSamplingFinal`) were mismatched between `ProjectSerializer::DeserializeMorphData` and the `MorphData()` constructor. **Fixed in PR #22** (Sprint 4): fallbacks now read `50.0`, `7`, `0.1`, `0.01` respectively, matching the ctor and `WrapParams`. Any future change to `MorphData()` defaults must also update the corresponding serializer fallback.

- `PresetStore::Save` always writes `created_at = IsoNow()` — the timestamp is refreshed on every Save call, so a Rename or Replace flow updates `created_at` to the time of the mutation rather than the original creation time. This is a minor semantic issue; the field is not exposed in the UI today. If `created_at` is ever surfaced (e.g., preset sort order) it should be preserved across mutations by reading the existing value from disk first.

- `IsValidUserPresetId` was added in PR #22 to guard `PresetStore::Save` and `Delete` against path-traversal via malformed JSON `id` fields. IDs must match `user-[0-9a-f]{8}`. Both `Save` and `LoadAll` validate the id before constructing any filesystem path. This was the single blocking finding from the PR #22 review cycle.

## Sprint 5 Patterns (PR #24)

### Config.SetCollapsedSection — Synchronous Write-on-Toggle
`Config::SetCollapsedSection` calls `Save()` immediately on every state change. This matches the pre-existing pattern for all other Config setters (`SetTokens`, `SetLicenseJwt`, etc.). Do NOT flag as a write-per-frame concern — the lambda `DrawCollapsibleSection` only calls `SetCollapsedSection` when the open/closed state actually changes (`isOpen != wasOpen`), not on every frame.

### DrawCollapsibleSection Lambda — ImGuiCond_Once Seeding
The lambda uses `ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Once)` to seed ImGui's internal TreeNode storage on the first frame the node is encountered. On subsequent frames ImGui ignores the seed and tracks the state itself. The wasOpen-vs-isOpen diff is caught post-render and written back to Config. This is the correct pattern for persistent collapsible sections — do not flag.

### corrIndices/corrPoints Scope Lift (NRICPDeformer)
In `Solve()`, `corrIndices` and `corrPoints` were moved from inner ICP loop scope to outer stiffness-step scope for `MaybeDumpResiduals`. `FindCorrespondences` clears and refills both vectors on every call, so behavior is identical in production. If `cancelled_` fires mid-loop, `return {}` exits before the dump is called — the empty vectors are never passed to `MaybeDumpResiduals`.

### BrushPreviewTextureIds Index Order is Load-Bearing
`MvStyle::Asset.BrushPreviewTextureIds[4]` index order (0=Smooth, 1=Grab, 2=PushPull, 3=Inflate) must match `kBrushes` in `StageSidebar.cpp::RenderTouchUp` AND `kStage4Tools` in `ToolRail.cpp`. Both arrays are currently ordered identically. Any reordering of either array is a latent ordering bug — the PNG assignment in `MvStyle.cpp::LoadAssets` must also be updated.

### GetWindow() Accessor — Test-Only Escape Hatch
`AppWindow::GetWindow()` exposes the raw `GLFWwindow*` for `imgui_test_engine` harness use only. The comment explicitly warns production code not to touch the GLFW handle directly. Do not flag as an encapsulation violation in future reviews.

### ToolRail.cpp — isStage4 Index Assumption
In `ToolRail::Render`, the loop index `i` is used to index `BrushPreviewTextureIds[i]` only when `isStage4` is true. The guard is `isStage4 && i < 4` implicitly (loop runs over `kStage4Tools` which has exactly 4 entries). Safe, but if `kStage4Tools` ever grows beyond 4, the `BrushPreviewTextureIds` array must be expanded in parallel.

## Sprint 6 — LicenseManager Patterns (PR sprint/Licensing-Infrastructure-Core-Integration)

### StartRefreshThread — joinable() guard after AuthInvalid self-stop
After a `RefreshOutcome::AuthInvalid` drain, the main thread sets `stopRefresh_ = true` and notifies the CV, causing the worker to return. The worker thread is now finished but `refreshThread_.joinable()` is still `true` (un-joined). `StartRefreshThread()` guards on `joinable()` and returns early without restarting. If the user re-authenticates after this, `DrainPendingExchange` calls `StartRefreshThread()` which silently no-ops, leaving the new session with no 24h refresh loop. Fix: call `refreshThread_.join()` before `return` in the `if (refreshThread_.joinable()) return;` guard inside `StartRefreshThread`, OR join in the `AuthInvalid` drain path before calling `stopRefresh_.store(true)` (can't join from worker — only set the flag there; join must happen on the main thread drain).

### Inactive/Expired states pass through to the app shell
`gateLogin` in `AppWindow::TickOnce` only gates on `Unauthenticated` and `Authenticating`. `Inactive` and `Expired` both fall through to the full app shell with no enrollment barrier. The sprint 6 design treats these as "user sees error modal + License Status shows amber pill" rather than hard-blocking. This is intentional for the sprint but a future enforcement sprint will need per-action tier checks.

### SUPABASE_ANON_KEY has no CMake FATAL_ERROR guard for staging/production
`METAVISAGE_API_BASE_URL` and the public-key PEM guard at FATAL_ERROR for production. `METAVISAGE_SUPABASE_ANON_KEY` (and `SUPABASE_PROJECT_URL`) have no guard — an empty anon key would silently produce a binary that cannot refresh access tokens. Low-risk today because the token-refresh path falls back gracefully to `AuthInvalid`, but worth adding parity guards.

### MvWarning used for Sign Out button (amber, not red)
`RenderLicenseModal` styles the Sign Out button with `MvWarning` (amber `#F2B547`). The design-system convention flags destructive actions with a red; amber is for "caution / degraded state" (e.g. offline pill, cottage-cheese warning). This is a visual inconsistency with the planned high-fidelity modal redesign already queued in the backlog.

### RenderLicenseModal section headers use raw ImVec4(0.8f, 0.85f, 1.0f, 1.0f)
The ACCOUNT / LICENSE / USAGE section headers use `ImVec4(0.8f, 0.85f, 1.0f, 1.0f)` (#CCDAFF), which has no MvStyle token equivalent. This violates the Sprint 3.8 design-token rule ("never hardcode ImVec4 literals in widgets"). The value is close to MvBlue300 (#7BC2FA) but visually different. A `MvStyle::MvLabel` or `MvBlueLabel` token should be minted when the modal redesign lands.

### .env.development committed with real Supabase anon key
`METAVISAGE_SUPABASE_ANON_KEY` in `.env.development` contains the live Supabase project anon key and URL. The `keys/README.md` and `.env.example` comments confirm this is intentional (Supabase anon keys are designed to be client-distributable), not a secret leak. Do not flag as a credential exposure — the anon key is the JWT-signed "safe to embed" credential per the Supabase architecture.

### Deep-link validation in main.cpp uses magic literal 13
`arg.size() > 13 && arg.substr(0, 13) == "metavisage://"` in `main.cpp` uses a raw `13` instead of `kScheme.size()`. The `kScheme` constant is defined inside the `TryAcquire` callback closure, not at the earlier deep-link parse site. Minor inconsistency — the `ParseDeepLink` helper in `LicenseManager.cpp` is the authoritative validator; `main.cpp`'s pre-filter just needs to be consistent with it.
