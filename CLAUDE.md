# MetaVisage

Windows desktop application for automated MetaHuman mesh morphing. Wraps MetaHuman head meshes to custom head models for Unreal Engine 5.x, providing a commercial alternative to enterprise mesh-wrapping tools.

**Platform**: Windows 10/11 (64-bit) · **Language**: C++20 · **Status**: v2.0 in development (proprietary)

See @.claude/context/product-vision.md for strategic goals and current state.

## Technical Stack

- **UI Framework**: Dear ImGui + GLFW + GLEW. Sprint 3.5 cut Qt Gui/Widgets/OpenGL/OpenGLWidgets; Sprint 3.6 rebuilt the menus, sidebars, modals, viewport host, and system services on the new stack; **Sprint 3.7 removed `Qt6::Core` entirely** (QString/QFile/QJson/QDateTime → `std::string` / `std::filesystem` / `std::fstream` / `nlohmann::ordered_json` / `std::chrono`). **Sprint 3.8** layered the design system on top: `MvStyle` (palette, spacing, radii, shadows, eight font handles), `MvIcons` (Lucide TTF glyph constants in PUA), and `MvInspectorPrimitives` (shared section/row/drag helpers with `DragEditResult` for one-undo-per-gesture; **Sprint 5.1 added `DrawSubSectionLabel` for nested collapsible hierarchies**). **Sprint 5.4 added the `imgui[test-engine]` vcpkg feature** — folded into the single `imgui::imgui` target, hooks dormant unless an `ImGuiTestEngine` context is created. The window is fully **frameless** — GLFW creates an undecorated OS window, and a Win32 `WM_NCHITTEST`/`WM_NCCALCSIZE` subclass restores native drag, edge resize, Aero Snap, and the system menu while a custom title bar is drawn in ImGui. The codebase is fully self-hosted on `std::format` (C++20), `nlohmann::json`, and Win32 APIs. A configure-time CMake guard fails the build if any Qt target is reintroduced.
- **Graphics API**: OpenGL 4.3+
- **Mesh I/O**: Assimp (FBX, OBJ, GLTF)
- **Mesh Processing**: libigl / OpenMesh / CGAL
- **Deformation**: NRICP (replaces v1.0 RBF/TPS) · custom implementation using Eigen. Sprint 5.2 added an env-gated diagnostic dump (`METAVISAGE_DUMP_RESIDUALS=1` writes per-vertex residual CSVs to `%TEMP%` at the end of each stiffness step) for the cottage-cheese-face follow-up spike.
- **Math**: Eigen / GLM
- **Serialization**: nlohmann/json for `.mmproj` project files
- **System services**: `utils/Config` (JSON config at `%APPDATA%\MetaVisage\config.json`; **Sprint 5.1 added `ui_state.collapsed_sections` for inspector-header persistence via `Get/SetCollapsedSection`** — see the perf contract in the header; **Sprint 6 added `UserEmail` / `LicenseJwt` / `AccessToken` / `RefreshToken` getters/setters with DPAPI-encrypted at-rest storage**), `utils/HttpClient` (libcurl POST), `utils/SingleInstance` (Win32 named pipe at `\\.\pipe\MetaVisageSingleInstance`; **Sprint 6 added `AllowSetForegroundWindow(ASFW_ANY)` on the second-instance path so the running first instance can take foreground after a `metavisage://` deep-link redirect**), `utils/OpenUrl` (`ShellExecuteEx`), `utils/Paths` (exe-relative asset resolution; **Sprint 6 added `ExecutablePath()` for HKCU URL-handler registration**), `utils/SecureStorage` (Win32 DPAPI for cached license/auth tokens), NFD for native file dialogs.
- **Licensing (Sprint 6)**: `LicenseManager` singleton owns the auth state machine (`Unauthenticated` / `Authenticating` / `Verifying` / `Authenticated` / `Expired` / `Inactive`), browser-based OAuth via the `metavisage://` URL handler (registered to `HKCU\Software\Classes\metavisage` on first run), `/api/desktop/exchange` worker thread + `/api/desktop/token` 24h refresh thread + Supabase direct refresh on 401 (Option A per QA Round 2 Decision 2), `jwt-cpp` RS256 verification against the public key baked at configure time from `keys/<profile>-public.pem`, 7-day offline grace via the cached JWT's `exp` claim, amber `Offline` status-bar pill on transport-fail, `LicenseStatusModal` + tier badge + Sign Out flow.
- **Build profile** (Sprint 6): `build-msvc.ps1 -Profile development|staging|production` reads `.env.<profile>` and forwards each `KEY=VALUE` to CMake as a `-D`. CMake validates per-profile (production refuses to configure with empty API URL, empty Supabase URL, empty anon key, or empty public key) and bakes the values into a generated `licensing/LicenseConfig.h` via `cmake/LicenseConfig.h.in`. `.env.development` is checked in (localhost + non-secret Supabase project URL + NEXT_PUBLIC anon key); `.env.staging` / `.env.production` and `keys/*.pem` are gitignored.
- **Installer**: Inno Setup · **Auto-update**: WinSparkle

> **Algorithm secrecy**: Do not reference NRICP by name in user-facing UI text, logs, documentation, or error messages. Internal code comments and sprint docs are fine.

## Architecture

Layered architecture — keep concerns separated across layers:

```
Presentation   → ImGui shell (AppWindow, StageSidebar, Modals, ViewportHost, Viewport, LoginScreen)
Application    → Project (per-frame source of truth), StageManager, UndoStack, LicenseManager
Domain         → Mesh engine, deformation algorithm, sculpting engine, WrapPreset
Infrastructure → File I/O (MeshExporter, ProjectSerializer, PresetStore), Renderer (OpenGL),
                 system services (Config, HttpClient, SingleInstance, OpenUrl, Paths,
                 SecureStorage), licensing (LicenseManager + jwt-cpp + OpenSSL)
```

Key components: Mesh Engine (load/validate/optimize/export), Wrap Engine (single-path solver with Quick Wrap / Fantasy Wrap built-in presets and user-saved preset persistence), Sculpting Engine (brush-based editing), Renderer (OpenGL), Project Manager (save/load `.mmproj`), Preset Store (`%APPDATA%\MetaVisage\presets\<id>.mvpreset.json` with `schema_version: 1`).

**Per-frame state model**: `Project` is the single source of truth that the ImGui sidebars read and write each frame — no signal/slot bus. Sidebars surface action requests via the `StageSidebar::Actions` struct, which `AppWindow` consumes per frame and dispatches to the appropriate command, worker, or modal.

**Design system**: `MvStyle` owns every color, spacing unit, radius, and shadow used across the shell — never hardcode `IM_COL32` or pixel literals in widgets, look up an `MvStyle::Colors::*` / `MvStyle::Spacing::*` token instead. `MvStyle::Asset` holds GL texture handles for shipped PNGs (logo + four Stage 4 brush previews under `assets/brushes/`); load failures gracefully degrade to Lucide glyphs. Iconography goes through `MvIcons::*` (Lucide TTF, PUA codepoints) — never hand-draw glyphs with `ImDrawList` if a Lucide equivalent exists. Inspector rows go through `MvInspectorPrimitives` (`DrawSectionLabel` for top-level dividers, `DrawSubSectionLabel` for nested collapsibles, plus `DrawAxisInput` / `DrawTransformGroup` / `DrawInspectorFooter`) so undo grouping stays consistent.

## Repository Structure

```
MetaVisage/
├── assets/
│   ├── brushes/        # Stage 4 brush-effect preview PNGs (Sprint 5.3 — Smooth/Grab/PushPull/Inflate)
│   ├── fonts/          # Inter (Regular/Medium/SemiBold), JetBrains Mono, Saira ExtraBoldItalic, Lucide
│   ├── icons/          # App icons (Windows/macOS sets)
│   ├── models/         # Default MetaHuman head + Move/Rotate/Scale gizmo OBJs
│   ├── shaders/        # GLSL — basic, gizmo, grid, heatmap, matcap, point, overlay, textured
│   └── metavisage-logo-mark.png
├── cmake/              # configure_file templates (Sprint 6 — LicenseConfig.h.in)
├── docs/               # Banner, KNOWN_ISSUES.md
├── include/            # Public headers, mirrored by src/ layout below
│   ├── baking/         # TextureBaker, TextureWriter (PNG/TGA via stb)
│   ├── core/           # Mesh, Project, Camera, Transform, Types, UndoStack, UndoActions,
│   │                   # WrapPreset (preset data model — built-ins + user-saved)
│   ├── deformation/    # MeshDeformer, NRICPDeformer (env-gated diagnostic dump in Solve())
│   ├── io/             # MeshExporter, ProjectSerializer, PresetStore
│   ├── licensing/      # LicenseManager (Sprint 6 — auth state machine + exchange + refresh +
│   │                   #   RS256 JWT verify); LicenseConfig.h is generated from cmake/LicenseConfig.h.in
│   ├── rendering/      # Renderer, MeshRenderer, GizmoRenderer, PointRenderer, ShaderManager
│   ├── sculpting/      # Brush base + Smooth/Grab/PushPull/Inflate/Mask brushes
│   ├── thirdparty/     # stb_image, stb_image_write
│   ├── ui/             # Shell — AppWindow (TickOnce + GetWindow + RequestForegroundFocus),
│   │                   #   StageSidebar, StageStepper, ToolRail, Viewport (role-based
│   │                   #   toolbar window IDs), ViewportHost, Modals (+ LicenseModalState),
│   │                   #   FileDialogs, LoginScreen (Idle/Waiting/Verifying)
│   │                   # Design system — MvStyle (+ BrushPreviewTextureIds),
│   │                   #   MvIcons, MvInspectorPrimitives (+ DrawSubSectionLabel)
│   └── utils/          # Config (+ Get/SetCollapsedSection + UserEmail), HttpClient,
│                       # SingleInstance (+ AllowSetForegroundWindow on raise), OpenUrl,
│                       # Paths (+ ExecutablePath), SecureStorage, BVH, Logger, RayCaster,
│                       # SpatialHash
├── keys/               # RS256 license JWT public keys per environment (Sprint 6;
│                       # development-public.pem / staging-public.pem / production-public.pem
│                       # all gitignored — README + .gitkeep are committed)
├── src/                # Implementation, mirroring include/ structure
│   ├── licensing/      # LicenseManager.cpp (Sprint 6)
│   └── main.cpp        # GLFW window + ImGui init + main loop entry +
│                       # argv[1] metavisage:// pre-filter
├── tests/              # CTest targets — four smoke executables share MetaVisageCore
│   ├── fixtures/       # Pinned v1.0.0 .mmproj inputs + wrap_smoke_points.json
│   └── smoke/          # launch_to_export, v1_backward_compat, wrap_parity, ui_smoke
│                       # (ui_smoke uses imgui_test_engine — runs under `ctest -L ui`;
│                       # 4 scenarios as of Sprint 6: welcome_modal_dismiss /
│                       # dockspace_window_smoke / viewport_shading_modes / login_screen_cycle)
├── .claude/            # Slash skills, agents, hooks, rules, sprint context
├── .env.example        # Build-config template for licensing endpoints + key file (Sprint 6)
├── .env.development    # Localhost dev profile (committed; non-secret Supabase URL + anon key)
├── CMakeLists.txt      # Build config (vcpkg toolchain expected)
├── build-msvc.ps1      # MSVC build helper with -Profile flag + optional DLL staging
├── build-mingw.ps1     # MinGW build helper with -Profile flag
└── vcpkg.json          # Pinned dependency manifest (imgui[test-engine] + jwt-cpp + openssl)
```

All UI lives under the ImGui hosts (`AppWindow`, `Viewport`, `ViewportHost`, `StageSidebar`, `StageStepper`, `ToolRail`, `Modals`, `FileDialogs`, `LoginScreen`) and shares the design-system primitives (`MvStyle`, `MvIcons`, `MvInspectorPrimitives`). The Qt-era UI files were deleted at the end of Sprint 3.7 once the migration was verified — earlier sprint plans, retros, and commits still reference `MainWindow.cpp` / `SidebarWidget.cpp` / `ViewportWidget.cpp` / `ExportDialog.cpp` / etc. as the historical equivalents.

## Four-Stage Workflow

1. **Alignment** — transform target mesh to align with morph mesh (Move/Rotate/Scale gizmos, X/Y/Z locks)
2. **Point Reference** — place correspondence points across dual synchronized viewports; symmetry mode mirrors points across an axis
3. **Morph** — deformation with Quick Wrap / Fantasy Wrap presets, tunable stiffness/rigidity/sampling parameters, vertex masking, threaded computation
4. **Touch Up** — sculpting brushes (Smooth, Grab, Push/Pull, Inflate/Deflate) with radius/strength/falloff and symmetry

See the sprint plan for active stage-specific work.

## Mesh Data Integrity (Non-Negotiable)

Topology, UVs, and normals must survive the full pipeline unmodified. Any regression is P0.

- Import preserves vertex ordering, face indices, UV coordinates, and normals
- Morphing operates on vertex positions only — never retopologizes
- Export writes Unreal Engine-compatible FBX (Y-up, geometry + UVs + normals + materials)

## Key Conventions

- **Heavy operations run on worker threads** — mesh loading, deformation, and export must not block the UI thread.
- **All user actions are Commands** — implement undo/redo via the Command pattern for every mutating action.
- **Smart pointers for ownership** — `unique_ptr` / `shared_ptr`; no raw `new`/`delete` in new code.
- **Blender-style viewport controls** — MMB orbit, Shift+MMB pan, scroll zoom, numpad for ortho views, G/R/S for transform, X/Y/Z for axis constraints, `[`/`]` for brush radius. Consistency with Blender is a deliberate UX choice — do not invent alternate bindings.
- **Dark theme only** — force the dark palette at launch regardless of OS theme; v1.0 light-mode bleed-through was a reported bug.
- **Performance target** — 30+ FPS for meshes under 100k vertices. Profile before optimizing, but treat regressions below this threshold as bugs.

## File Formats

- **Import**: FBX (primary), OBJ, GLTF
- **Export**: FBX with Unreal Engine naming conventions — Y-up, geometry + UVs + normals + materials. **Triangulation defaults OFF** because UE5 MetaHuman Conform requires source quad topology end-to-end; do not flip this default without a UE5 import test.
- **Project files**: `.mmproj` (JSON), optional zlib compression

## Build & Run

Build via the helper scripts — they handle vcpkg toolchain wiring, build profile selection, and (with `-Deploy`) stage runtime DLLs next to the executable:

```powershell
.\build-msvc.ps1 -Profile development -Deploy   # default profile is development
.\build-msvc.ps1 -Profile staging               # reads .env.staging (gitignored)
.\build-msvc.ps1 -Profile production            # reads .env.production (gitignored)
.\build-mingw.ps1 -Profile development          # MinGW build
```

Output: `build\bin\Release\MetaVisage.exe`. **Kill any running instance before relinking** — Windows MSVC fails with `LNK1104` if `MetaVisage.exe` is held open by a previous run.

A successful CMake configure step requires `VCPKG_ROOT` (or `-DCMAKE_TOOLCHAIN_FILE=...`) so vcpkg can resolve GLFW, GLEW, ImGui (with the `test-engine` feature for the smoke-UI lane), NFD, libcurl, Eigen, Assimp, nlohmann_json, jwt-cpp, and OpenSSL. **Qt is no longer required** as of Sprint 3.7. Smoke tests run via `ctest -C Release` (four targets: `smoke_load_serialize_export`, `smoke_v1_backward_compat`, `smoke_wrap_parity`, and `smoke_ui` which drives the live AppWindow via `imgui_test_engine` across 4 scenarios — welcome modal dismiss / dockspace window-existence / viewport shading-mode toggles / login screen Sign In→Cancel cycle). Filter to just the UI lane with `ctest -L ui`.

**Licensing config** (Sprint 6): the build profile flag drives which `.env.<profile>` is read into CMake cache vars, then baked at configure time into a generated `licensing/LicenseConfig.h` via `cmake/LicenseConfig.h.in`. `LicenseManager` consumes the constants directly. Production builds refuse to configure with empty API URL / Supabase URL / Supabase anon key / public key — `.env.development` ships with localhost defaults and the non-secret Supabase dev creds; staging/production env files and `keys/*.pem` are gitignored and delivered out-of-band via 1Password per `DESKTOP_INTEGRATION_QA.md`.

## Web Team Integration (v2.0)

Licensing, auth, and updates depend on the web team's API contract. When working on these features, read the relevant spec first:

- @.claude/context/DESKTOP_APP_INTEGRATION.md — licensing API contract (activation, tier lookup, feature flags)
- @.claude/context/DESKTOP_AUTH_FLOW.md — browser-based OAuth flow
- @.claude/context/DESKTOP_UPDATE_FLOW.md — WinSparkle update flow
- @.claude/context/DESKTOP_INTEGRATION_QA.md — Sprint 6 web-team Q&A: RS256 migration, Option A direct-Supabase refresh, email on the exchange response, license_key inside the JWT, `/manage-subscription` Stripe redirect

## Standards

See @.claude/rules/code-style.md for coding conventions.
See @.claude/rules/testing.md for testing requirements.
See @.claude/rules/security.md for security guidelines.
See @.claude/rules/git-workflow.md for git workflow.

## Agents

- `code-reviewer` — Invoked by `/review-pr`. Read-only, learns project conventions over time.
- `architect` — Answers questions about how the codebase works. Read-only, builds a structural map in memory.
- `test-writer` — Writes tests for existing code, matching project test patterns.
- `dependency-auditor` — Scans dependencies for vulnerabilities. Read-only, tracks deferred issues in memory.

## Available Skills

- `/commit` — Generate a semantic commit message
- `/write-pr [base-branch]` — Open a PR from the current branch
- `/review-pr [pr-number|branch]` — Review a pull request in an isolated agent context
- `/sprint-planner` — Seed a per-sprint file from the aggregated sprint plan at the start of each sprint
- `/sprint-retrospective` — Facilitate a sprint retrospective
- `/product-backlog` — Add, update, complete, or reprioritize backlog items
- `/update-context` — Sync CLAUDE.md, product-vision.md, and README.md after a sprint
- `/find-skills` — Discover and install community skills
- `/create-skill [description]` — Scaffold a new skill

## Key Documents

- [Product Vision](./.claude/context/product-vision.md) — strategic goals and current state
- [PRD](./.claude/context/PRD.md) — v1.0 requirements (still the authority for workflow spec)
- [Sprint Plan (aggregated)](./.claude/context/sprints/SPRINT_PLAN.md) — v2.0 roadmap
- [v1.0.0 Feedback](./.claude/context/Version%201.0.0%20Feedback.md) — source of v2.0 scope
- [Product Backlog](./.claude/context/development/product_backlog.md) — items not yet committed to a sprint

## Glossary

- **MetaHuman** — Epic Games' digital human system in Unreal Engine
- **Morph Mesh** — the base MetaHuman head mesh being deformed
- **Target Mesh** — the custom head model the morph mesh is wrapped to
- **Point Correspondence** — matching points marking the same anatomical location on both meshes
- **Wrap Engine** — the user-facing brand for the deformation solver (NRICP internally — don't surface that name to users per the algorithm-secrecy rule)
- **Quick Wrap / Fantasy Wrap** — built-in v2.0 presets; Quick Wrap = `MorphData()` defaults + `nricpGammaInitial = nricpGammaFinal = 1.0` for human-like characters, Fantasy Wrap = pure `MorphData()` defaults for dramatic features (pointed ears, large noses). Both ship in `WrapPreset::BuiltIns()`.
- **Wrap Preset** — `core::WrapPreset` snapshot: stable id, display name + description + Lucide icon glyph, `isBuiltIn` flag, and a `WrapParams` parameter snapshot. Built-ins live in code; user presets persist to disk via `PresetStore`.
- **User Preset** — Wrap Preset saved by the user via the inspector's "+ Save preset" card. Stored at `%APPDATA%\MetaVisage\presets\<id>.mvpreset.json` with `schema_version: 1`. Right-click on the inspector card surfaces Rename / Replace with current parameters / Delete.
