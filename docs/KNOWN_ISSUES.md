# Known issues and validation limits

These notes apply to MetaVisage 2.1.0 and the `development` branch.
Historical v1.0.0 implementation notes are not a list of current defects.

## Workflow limitations

- **No auto-save or crash recovery**: Save frequently with `Ctrl+S`. Unsaved
  work can be lost if the application exits unexpectedly.
- **Auto-map accuracy varies by style**: Auto-map places landmarks well on
  realistic heads; stylized features (very large eyes or ears, exaggerated
  noses) are often off and get review or low confidence rings. Check those
  points and drag them into place. Custom profiles with few landmarks are not
  yet measured on real heads.
- **Auto-map needs a source profile with landmarks**: Pick the built-in
  MetaHuman profile or build one in the profile editor (Stage 1 > Profile
  tools). A mesh without a profile is placed manually as before.
- **Projects saved by 2.1.0 use format v3**: They don't open in 2.0.0. Older
  projects open in 2.1.0 unchanged.
- **Large-file responsiveness**: Mesh import, project save/load, and texture
  baking/export currently run synchronously and can temporarily pause the UI.
  Morph processing runs on a background worker. Moving the remaining heavy I/O
  to workers is follow-up work.
- **Morph artifacts with aggressive parameters**: High landmark weighting
  relative to final stiffness can cause local geometry artifacts. The Morph
  inspector displays a warning for this combination. Lower landmark weight or
  increase final stiffness, then inspect the processed result before accepting.

## Platform and validation

- **Windows 10/11 x64 only**: macOS and Linux are not currently supported.
- **OpenGL 4.3+ required**: The application and live UI smoke test require a
  compatible GPU and driver.
- **MSVC is the validated toolchain**: The retained MinGW helper needs
  compatible dependencies and has not been validated for the standalone release.
- **Clean-machine package smoke pending**: A fresh Windows installation still
  needs validation of the extracted package, runtime DLLs, assets, and local
  export before the next binary release.
- **UI automation coverage is limited**: Startup, stage navigation, shading,
  brush selection, source profiles, auto-map, point dragging and the profile
  editor are covered. Sculpt/undo UI automation remains follow-up work;
  mesh/export behavior has separate smoke coverage.

## Unreal workflow

FBX is the default export format, with Y-up conversion and materials enabled.
Triangulation is disabled by default to preserve quad topology for MetaHuman
Conform. Keep the original source topology and verify the exported result in
your Unreal Engine version; the automated smoke suite does not run Unreal Editor.

## Reporting issues

Report bugs through [GitHub Issues](https://github.com/cmershon2/MetaVisage/issues).
Include the version or commit, Windows/GPU/driver details, reproduction steps,
mesh format and counts, and relevant log excerpts. Logs are stored at
`%APPDATA%/MetaVisage/logs/metavisage.log`. Remove private paths and share only
assets you have permission to redistribute.
