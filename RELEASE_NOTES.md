# MetaVisage release notes

## v2.1.0 - Auto Point Reference

Stage 2 no longer has to be done by hand. MetaVisage can place the landmarks
for you, show how sure it is of each one, and let you fix any point by
dragging it. Source profiles make this work for any source mesh, not just the
bundled MetaHuman head.

### New

- **Source profiles**: a source mesh plus its landmarks, default mask,
  symmetry and transform. Stage 1 picks a profile instead of a bare mesh; the
  built-in MetaHuman profile ships with 52 authored landmarks and a
  mouth-interior mask. Importing a mesh whose topology matches a profile
  attaches it automatically.
- **Auto-map**: on entering an empty Stage 2, MetaVisage offers to place the
  profile's landmarks on the target. It runs in the background with Cancel,
  never changes your Stage 1 alignment, refuses a target facing the wrong way
  or too far from the alignment, and is one undo step. Re-run it from the
  Stage 2 sidebar to replace auto-placed points or fill in missing ones; points
  you placed or moved are never touched.
- **Confidence**: each auto-placed target point has a green, amber or red
  ring (toggleable), and the sidebar lists the ones to check first.
- **Drag to refine**: drag any placed point along the mesh. Mirrored pairs
  move together (hold Alt to move one side); midline points slide along the
  symmetry plane unless you unlock them. One undo step per drag.
- **Custom profiles** (Stage 1 > Profile tools):
  - a profile editor with the 52-landmark checklist grouped by region, mirror
    fill, custom landmarks and a default mask brush
  - Save source as profile, to turn a manual setup into a profile
  - Manage profiles: rename, duplicate (built-ins too), delete, set a default
    profile for File > New, and import/export as one `.mvprofile.zip`
- **Help > Show Profile Landmarks (Dev)** and **Export Source Profile Draft
  (Dev)** for authoring built-in profiles.

### Also in this release

- **Alignment**: the move gizmo tracks the cursor at its speed, tool hotkeys
  match the tool rail (Blender-style G/R/S kept), transform fields accept
  typed values, target textures survive replacing the target mesh, and the
  placeholder rail tools are gone.
- **App shell**: a working View menu with a projection toggle and Auto
  Perspective, tidier menus, the MetaVisage icon on the window and
  executable, and the debug HUD hidden by default (Help menu).
- **Morph**: Reset to source mesh (undoable) returns to the unprocessed
  source; "Reset to defaults" now resets parameters only; the vertex mask has
  its own section.

### Fixed

- Reopening a project restores the painted vertex mask and all wrap
  parameters.
- Undo and redo refresh the mask overlay.
- Changing the source mesh is one undo step and no longer wraps the previous
  source.
- Confirm and error dialogs keep a readable width with long messages.

### Compatibility

- Projects are saved in format v3 (source profile, landmark ids, confidence).
  2.1.0 opens v1 and v2 projects; 2.0.0 cannot open v3 projects.
- User profiles are stored in `%APPDATA%\MetaVisage\profiles\`, each with
  its own copy of its mesh.
- New dependency: kubazip (MIT), for profile packages.

### Validation

Six CTest smoke targets, including synthetic auto-map accuracy (median
0.09–0.52% of head size), source profiles and packages, and 16 live UI
scenarios. On six hand-placed reference heads, auto-map's median error is
0.6–1.7% of head size: accurate on realistic heads, weaker on stylized
features. See [known issues](docs/KNOWN_ISSUES.md).

## v2.0.0 - Standalone open-source release

Released 2026-10-16. Restores the standalone open-source desktop application.

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
