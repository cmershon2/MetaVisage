---
name: MetaVisage Product Vision
description: Current strategic direction for the standalone open-source application.
---

# Product Vision

**Last Updated**: 2026-10-16

MetaVisage is a standalone, MIT-licensed Windows desktop application for wrapping
MetaHuman head meshes to custom models. The owner has retired the commercial web
integration following Unreal Engine 5.8 and archived that version. The current
mandate is to make all implemented tools available offline without accounts,
subscriptions, tier gates, conversion limits, or web integrations.

The four-stage workflow remains Alignment, Point Reference, Morph, and Touch Up,
followed by local FBX/OBJ/GLTF export. Preserve topology, vertex order, UVs,
normals, material slots, and Unreal-compatible defaults. Qt remains retired;
the application uses C++20, Dear ImGui, GLFW, GLEW, Eigen, Assimp, and local JSON
settings/project files.

The standalone implementation removes login/account UI, license/token storage,
browser auth/deep links, HTTP and JWT dependencies, paid gates, export reporting,
remote help/update links, and credential-based build profiles. All six texture
slots, custom source heads, masking, manual parameters, and sculpt brushes are
available locally. The named-pipe single-instance focus behavior remains.

Sprints 1-7 and their integration specifications describe historical work, not
the current product direction. The backlog's commercial, auth, and web integration
items are superseded by this mandate. Sprint 7's artifact investigation retained
the parameter warning stopgap; core morph quality and topology remain relevant.

See `open-source-transition.md` for implementation and validation evidence.

Sprint 8 completes the standalone transition. Final MSVC Release and all four
CTest targets pass, including five live UI scenarios with staged assets loaded.
The review has no material outstanding findings. Future validation priorities
are a loaded-mesh UI fixture and clean-machine package smoke checks; commercial
follow-ups from Sprint 7 are retired. See `retrospectives/retro-sprint-08.md`.

Public setup, contribution, release, and known-issue documentation now describes
the standalone build. Before release packaging, document the provenance and
redistribution terms of the reference head, bundled fonts/icons, and other
third-party assets, and collect dependency notices. The dependency manifest
does not pin a vcpkg baseline. See
`reviews/open-source-documentation.md` for the documentation review and remaining
release-readiness findings; these were not resolved by the functional transition.

The promotion review into `master` adds a project-generation guard so an old
morph result cannot populate a replacement workspace, and keeps the public
Wrap Engine name in progress messages and diagnostic metadata. A tiny in-memory
mesh regression expands the UI lane to six scenarios; full loaded-asset
Process/Accept and sculpt/undo coverage remains separate follow-up work.

Release packaging now uses the standalone staged MSVC output, with a portable
Windows ZIP, SHA-256 checksum, run instructions, and available vcpkg/stb notices.
The helper preserves existing packages and excludes test/developer artifacts.
Clean-machine validation and bundled-asset provenance remain release follow-ups;
creating an archive is not publication or redistribution signoff.

Sprint 9 makes Stage 1 Alignment usable without workarounds. The move gizmo
keeps the mesh under the cursor in every view, the G/R/S hotkeys match the tool
rail, transform fields accept typed values, and target textures survive a mesh
swap. The orthographic top view is also fixed. The UI smoke lane now has eleven
scenarios. Sprint 10 covers the shell and Morph-stage feedback from the same
triage. See `retrospectives/retro-sprint-09.md`.
