# MetaVisage release notes

## Standalone transition (unreleased)

The current `development` branch restores the standalone open-source desktop
application. CMake and the dependency manifest use version `2.0.0`; this section
does not announce a published binary release.

- All implemented local tools are available without login, subscriptions,
  feature tiers, or conversion limits.
- Removed browser authentication, token storage, license services, export
  reporting, and runtime web integrations.
- Retained the four-stage workflow, Wrap Engine presets and advanced controls,
  sculpting, masking, texture slots, and local export.
- The application uses C++20, Dear ImGui, GLFW, and GLEW. Qt is not required.
- Existing v1.0.0 `.mmproj` files remain supported. Legacy account fields in
  settings are ignored and omitted when settings are saved.
- Four smoke targets cover mesh round-trip/export, v1 project compatibility,
  wrap parity, and live UI startup/navigation/shading/brush selection.

MSVC Release and all four smoke targets passed during the standalone transition.
MinGW and clean-machine package validation remain outstanding. See
[known issues](docs/KNOWN_ISSUES.md) for remaining limitations and
[BUILD.md](BUILD.md) for current setup instructions.

## v1.0.0 (historical release)

The original release notes recorded March 2026 as the release date and Windows
10/11 x64 as the supported platform. This historical summary is separate from
the current development build.

- Alignment with move, rotate, and scale controls.
- Landmark placement in synchronized dual viewports with symmetry.
- Mesh wrapping followed by Smooth, Grab, Push/Pull, and Inflate sculpting.
- OpenGL rendering, MatCap shading, displacement heatmaps, and mesh statistics.
- Mesh import/export, `.mmproj` project files, undo/redo, and local logging.

Use the current build guide for development; the original Qt/C++17 setup no
longer applies.
