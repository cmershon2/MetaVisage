# MetaVisage Agent Guide

MetaVisage is a Windows desktop application for automated MetaHuman mesh
morphing. It wraps MetaHuman head meshes to custom head models for Unreal
Engine 5.x and is currently in v2.0 development.

Use `.agents/context/product-vision.md` for strategic goals and current state.
Use `.agents/context/PRD.md` as the v1.0 workflow authority. Active sprint,
retro, backlog, design, and integration context lives under `.agents/context/`.
Long-lived specialist notes live under `.agents/agent-memory/`.

The legacy Claude runtime has been retired. Do not use `.claude` paths, Claude
slash commands, Claude hooks, or `CLAUDE.md` as active project instructions.

## Codex Runtime

- Project-local skills live in `.agents/skills/`; invoke them with `$skill-name`
  after restarting Codex so the new files are discovered.
- Project rules live in `.agents/rules/`.
- Custom subagents live in `.codex/agents/`.
- Advisory hooks live in `.codex/hooks/` and are registered by
  `.codex/hooks.json`.
- The boilerplate reference baseline lives in
  `.agents/vendor/codex-agent-boilerplate/`.
- Use `$boilerplate-upgrade` for future boilerplate checks, `$update-context`
  after meaningful project changes, `$sprint-planner` and
  `$sprint-retrospective` for sprint flow, `$review-pr` for review, and
  `$commit` for focused commit messages.

## Technical Stack

- Language: C++20.
- Platform: Windows 10/11 64-bit.
- UI: Dear ImGui + GLFW + GLEW. Qt must not be reintroduced; CMake has a guard
  for Qt targets.
- Graphics: OpenGL 4.3+.
- Mesh I/O: Assimp for FBX, OBJ, and GLTF.
- Mesh processing: libigl, OpenMesh, CGAL.
- Math and serialization: Eigen, GLM, nlohmann/json.
- Licensing/auth: libcurl, jwt-cpp, OpenSSL, Win32 URL handlers, DPAPI-backed
  token storage, Supabase refresh flow, tier feature flags, durable conversion
  reporting queue.
- Installer/update: Inno Setup and WinSparkle.

## Build And Test

Build through the helper scripts so vcpkg, profile selection, and runtime DLL
deployment stay consistent:

```powershell
.\build-msvc.ps1 -Profile development -Deploy
.\build-msvc.ps1 -Profile staging
.\build-msvc.ps1 -Profile production
.\build-mingw.ps1 -Profile development
```

Output is `build\bin\Release\MetaVisage.exe`. Kill any running instance before
relinking; Windows MSVC fails with `LNK1104` if the executable is open.

Run smoke tests with:

```powershell
ctest -C Release --output-on-failure
ctest -C Release -L ui --output-on-failure
```

`VCPKG_ROOT` or `-DCMAKE_TOOLCHAIN_FILE=...` is required for configure.
Production builds must have non-empty API URL, Supabase URL, anon key, and
public key inputs. The full smoke suite has four targets; `smoke_wrap_parity`
can take about 70 seconds on development machines and has a 120-second CTest
timeout.

## Architecture

Keep the layered architecture intact:

```text
Presentation    -> ImGui shell, sidebars, modals, viewport, login screen
Application     -> Project, StageManager, UndoStack, LicenseManager
Domain          -> Mesh engine, deformation algorithm, sculpting, WrapPreset
Infrastructure  -> File I/O, renderer, config, HTTP, single-instance, storage
```

`Project` is the per-frame source of truth for UI state. Sidebars emit action
requests via `StageSidebar::Actions`, and `AppWindow` consumes them each frame.
Avoid adding a signal/slot bus.

## Non-Negotiables

- Topology, UVs, normals, vertex order, and face indices must survive the full
  import, morph, and export pipeline.
- Morphing changes vertex positions only; never retopologize.
- Export defaults must remain Unreal-compatible FBX with Y-up geometry, UVs,
  normals, and materials.
- Triangulation defaults off because UE5 MetaHuman Conform requires quad
  topology end to end.
- Heavy operations run on worker threads; mesh loading, deformation, auth
  exchange/refresh, and export must not block the UI thread.
- All mutating user actions use the Command pattern and participate in
  undo/redo.
- Use smart pointers for ownership; no raw `new` or `delete` in new code.
- Keep the dark theme forced regardless of OS theme.
- Preserve Blender-style viewport controls: MMB orbit, Shift+MMB pan, scroll
  zoom, numpad orthographic views, G/R/S transforms, X/Y/Z axis constraints,
  and `[` / `]` brush sizing.

## UI Conventions

Use `MvStyle` for colors, spacing, radius, shadows, and font handles. Do not
hardcode `IM_COL32`, `ImVec4`, or pixel literals in widgets when a token exists.

Use `MvIcons::*` for Lucide icon glyphs. Do not hand-draw glyphs with
`ImDrawList` when a Lucide equivalent exists.

Inspector rows should go through `MvInspectorPrimitives` so layout and
one-undo-per-gesture behavior stay consistent.

Paid feature gates should use `UpgradePromptWidget`: compact locked rows for
standard inspector controls and hover cards for brush/tool buttons. Brush gates
must keep the underlying brush preview image visible under the lock treatment.

Algorithm secrecy matters: do not reference NRICP by name in user-facing UI
text, logs, documentation, or error messages. Internal code comments and sprint
docs may use the internal name.

## Key Documents

- Product vision: `.agents/context/product-vision.md`
- PRD: `.agents/context/PRD.md`
- Aggregated sprint plan: `.agents/context/sprints/SPRINT_PLAN.md`
- v1.0.0 feedback: `.agents/context/Version 1.0.0 Feedback.md`
- Product backlog: `.agents/context/development/product_backlog.md`
- Latest retrospective: `.agents/context/retrospectives/retro-sprint-07.md`
- Web integration specs:
  `.agents/context/DESKTOP_APP_INTEGRATION.md`,
  `.agents/context/DESKTOP_AUTH_FLOW.md`,
  `.agents/context/DESKTOP_UPDATE_FLOW.md`, and
  `.agents/context/DESKTOP_INTEGRATION_QA.md`

## Repository Map

```text
assets/      fonts, icons, models, shaders, brush previews, logo assets
cmake/       configure templates, including licensing config
docs/        user/developer docs and known issues
include/     public headers mirrored by src/
src/         implementation
tests/       CTest smoke targets and fixtures
keys/        per-environment public keys; private/profile keys stay ignored
.agents/     Codex skills, rules, planning context, memory, vendor baseline
.codex/      Codex custom agents, hooks, and local config
```
