# Contributing to MetaVisage

Contributions to the standalone Windows desktop application are welcome.
Discuss substantial features or new dependencies in an issue before implementing
them so the scope can be agreed with the maintainer.

## Getting started

1. Fork and clone the repository.
2. Create a focused branch from `development`, which contains the standalone
   application changes.
3. Follow [BUILD.md](BUILD.md) to build with Visual Studio 2022, C++20, CMake,
   and vcpkg. The UI uses Dear ImGui, GLFW, and GLEW.
4. Run the smoke tests before and after changes that affect application behavior.

Agent tooling under `.agents/` and `.codex/` is optional for contributors.
Historical web integration plans do not authorize new accounts, licensing,
payment gates, usage limits, or runtime web integrations.

## Code and architecture

- Follow existing naming and formatting conventions and use C++20.
- Use smart pointers for ownership; avoid raw `new` and `delete` in new code.
- Preserve the layers: presentation (ImGui UI), application (`Project`,
  `StageManager`, `UndoStack`), domain (mesh/deformation/sculpting), and
  infrastructure (I/O, rendering, configuration, storage).
- `Project` is the per-frame source of UI state. Sidebars emit action requests
  that `AppWindow` consumes; avoid introducing a signal/slot bus.
- Keep loading, deformation, and export off the UI thread.
- Route mutating user actions through undoable commands.
- Preserve topology, vertex order, face indices, UVs, normals, and material
  slots. Morphing changes vertex positions only. Keep export triangulation
  disabled by default and retain Unreal-compatible FBX defaults.
- Keep the forced dark theme and Blender-style viewport controls.
- Use `MvStyle`, `MvIcons`, and `MvInspectorPrimitives` for UI tokens, icons,
  inspector layout, and one undo step per gesture.
- Use **Wrap Engine** in user-facing text for the wrapping implementation.

Headers in `include/` are mirrored by implementation in `src/`, with `core/`,
`deformation/`, `sculpting/`, `rendering/`, `io/`, `baking/`, `ui/`, and `utils/`
subsystems. Qt must not be reintroduced.

## Validation

Build through the helper script, closing any running instance before relinking:

```powershell
.\build-msvc.ps1 -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -L ui --output-on-failure
git diff --check
```

The UI test opens a real window and needs OpenGL 4.3+. The full suite contains
five targets; wrap parity has a 120-second timeout. For mesh changes, check
round-trip topology and UV preservation. For UI changes, exercise the affected
workflow and include screenshots when useful. Documentation-only changes need
link and content checks rather than a full application rebuild.

## Pull requests

- Target `development` and keep each PR focused on one change.
- Describe the problem, resulting behavior, and validation performed. Identify
  checks you could not run and any compatibility changes.
- Add meaningful regression coverage for behavior changes where appropriate.
- List new dependencies and document the source and redistribution terms of
  any contributed third-party code or assets.
- Use Conventional Commits, such as `fix(export): preserve quad face order` or
  `docs(build): update Windows setup`. Explain the change, reason, and validation
  in the commit body. A sprint identifier is needed only for sprint work.

Never include credentials, private keys, account data, or proprietary mesh files
in commits, fixtures, logs, or screenshots.

## Reporting issues

Use [GitHub Issues](https://github.com/cmershon2/MetaVisage/issues) and include:

- Application version or commit, Windows version, GPU model, and driver version.
- Steps to reproduce, expected behavior, and actual behavior.
- Mesh format, vertex/face counts, and relevant export settings.
- Relevant excerpts from `%APPDATA%/MetaVisage/logs/metavisage.log`, with personal
  paths or other private information removed.
- A small reproducible project or mesh when you have permission to share it.

See [known issues](docs/KNOWN_ISSUES.md) first. Current contribution priorities
include morph quality, topology preservation, loaded-mesh UI test coverage,
standalone packaging validation, and user workflow documentation. Broader ideas
are tracked in the [backlog](.agents/context/development/product_backlog.md).

## License

Contributions to MetaVisage code are accepted under the [MIT License](LICENSE).
Keep third-party notices intact and identify assets with separate terms.
