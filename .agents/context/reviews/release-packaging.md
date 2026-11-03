# Standalone release packaging validation

**Date**: 2026-10-16
**Scope**: Replace the retired Qt packaging script with packaging of an existing
MSVC Release build. No application source changes or publication.

The script reads its default version from `vcpkg.json`, copies the main
executable, staged DLLs/assets, project license, available vcpkg notices and
embedded stb licenses, and generates run instructions, a ZIP and SHA-256 file.
Existing output names cause failure before copying; no prior releases are
deleted. Generated `release/` content is ignored by Git.

## Evidence

- PowerShell 5.1 parser and execution passed against the existing Release build.
- Created `release/MetaVisage-2.0.0-win-x64.zip`: approximately 11.14 MiB,
  110 files, nine runtime DLLs, and 24 dependency notices.
- Verified ZIP integrity, exact executable/asset bytes against staged build
  output, complete staged DLL inclusion, and matching SHA-256 sidecar.
- Verified exclusion of smoke executables, fixtures, logs and developer config.
- Isolated fixture checks passed for manifest version defaults, invocation from
  another directory, release-candidate versions, existing-package preservation,
  invalid version/configuration/retired Qt arguments, missing DLLs, and missing
  staged assets. Failure cases did not create a new package or modify earlier
  archive bytes. The temporary fixture was removed after verification.

The user reported the Release build and tests already passed. No application
rebuild or repeat CTest run was needed for this packaging-script change.
Extracted-package launch on a clean Windows machine remains unverified.
The Visual C++ redistributable installer is not bundled. Asset-specific
provenance/notices remain open as recorded in
[the documentation review](open-source-documentation.md); copied dependency
notices do not establish rights for the reference head or bundled fonts/icons.
