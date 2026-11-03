# Product Backlog

**Project**: MetaVisage
**Last Updated**: October 30, 2026

> Active v2.0 work is tracked in `.agents/context/sprints/SPRINT_PLAN.md` and individual sprint files seeded via `$sprint-planner`. Sprint 8 closes the standalone transition; Sprints 9 and 10 deliver the 2026-10-29 feedback triage. Sprints 11–14 are reserved for Auto Point Reference; Sprint 11 is complete. This backlog holds items **not yet committed to a sprint** — candidates pulled from v1.0.0 feedback that fell out of v2.0 scope, and post-v2.0 enhancements from the PRD.

---

## 🔥 High Priority

### Auto Point Reference

Design: [auto-point-reference.md](../design/auto-point-reference.md). Replaces the former lower-priority "Automated point placement (ML)" item. Sprints run in order; each depends on the one before. All four sprints shipped: Source Profiles (11), the auto-map solver (12), the Stage 2 workflow (13) and custom profiles (14); see the changelog. Follow-ups are listed below.

- [ ] **Built-in profile landmark adjustments**
  - Copy Casey's adjusted cheekbone, jaw middle and ear-top points (placed on a duplicate of the built-in in the profile editor) into `assets/profiles/metahuman-head-lod0.mvprofile.json`, then rerun the profile and landmark-solver smokes and the dataset bench. Raised in Sprint 12; the editor shipped in Sprint 14.
  - **Effort**: < 1 day
  - **Value**: Auto-map's most consistent real-head misses came from these points

- [ ] **Mask brush symmetry**
  - Mirror mask strokes across the symmetry plane in Stage 3 and the profile editor, like the sculpt brushes.
  - **Effort**: ~1 day
  - **Value**: Halves mask painting for symmetric heads

### Other

- [ ] **Auto-save and crash recovery**
  - Periodic project snapshots to a `.mvautosave` sidecar file. On launch after unclean shutdown, offer to restore.
  - **Effort**: ~3–5 days
  - **Value**: Data loss during long morph sessions is the highest-severity user complaint category not addressed in v2.0

---

## 🟡 Medium Priority

- [ ] **Separate MinGW build directory**
  - `build-mingw.ps1` builds into the same `build\` folder as `build-msvc.ps1`, so a full MinGW build can't run without overwriting the MSVC cache. Give it its own directory (for example `build-mingw\`) and validate the full MinGW build, including the Sprint 10 icon resource step. Found in Sprint 10.
  - **Effort**: ~0.5 day

- [ ] **Stage 1 mesh auto-aligner**
  - Add an "Auto-align" action to Stage 1 that moves, rotates and scales the target to match the source, as one undoable step. Separate from Auto Point Reference: per design decision 3 (confirmed 2026-10-31), auto-map never changes Stage 1 transforms. The Sprint 12 similarity refinement is a likely starting point. Raised by Casey on 2026-10-31.
  - **Effort**: ~1 sprint
  - **Value**: Removes the most manual step left in the workflow

- [ ] **Load meshes off the UI thread**
  - `AppWindow::ImportMorphMeshFromDisk` / `ImportTargetMeshFromDisk` call `Mesh::Load` on the UI thread, so large meshes freeze the window. AGENTS.md requires worker threads for mesh loading. Load on a worker, show progress, and upload to the GPU on the UI thread when done. Found while planning Sprint 11.
  - **Effort**: ~1–2 days

- [ ] **Ctrl+Y redo shortcut**
  - The Keyboard Shortcuts dialog lists Ctrl+Y for redo, but `HandleGlobalShortcut` only handles Ctrl+Shift+Z. Bind it or drop it from the dialog. Found in Sprint 9.
  - **Effort**: ~0.5 hour

- [ ] **Release mesh renderer cache on project switch**
  - Renderer caches GPU buffers by `Mesh*`; entries for the previous project's meshes are never released on New/Open Project, so GPU memory grows and a recycled address could reuse a stale entry. Found in Sprint 9 (target-mesh replacement is fixed).
  - **Effort**: ~0.5 day

- [ ] **Loaded-mesh UI smoke fixture**
  - Add a small local fixture for Stage 3 Process/Accept and Stage 4 sculpt/undo automation, retaining a bounded runtime.
  - **Effort**: ~1-2 days
  - **Value**: Covers the remaining mesh-dependent UI paths beyond standalone startup, navigation, and brush selection

- [ ] **Standalone release package smoke checklist**
  - Verify a clean Windows machine can launch an extracted release package, locate shaders/brush previews/default head, and complete a local export without service configuration.
  - **Effort**: ~0.5 day
  - **Value**: Catches packaging and runtime DLL/asset gaps before distributing the open-source build

- [ ] **Texture thumbnail preview modal**
  - v2.0 allows unlimited texture uploads with small thumbnails. Clicking the thumbnail should open a full-size preview modal.
  - **Effort**: ~1 day
  - **Value**: Quality-of-life polish for texture workflow

- [ ] **Advanced sculpting brushes**
  - Crease, pinch, and masking brushes beyond the v1.0 smooth/grab/push-pull/inflate-deflate set.
  - **Effort**: ~1 week
  - **Value**: Closes the feature gap with commercial sculpting tools for Stage 4 refinement

---

## 🟢 Lower Priority

- [ ] **ML landmark solver (evaluate)**
  - A second `LandmarkSolver` that renders the target and runs a local 2D face-landmark model (for example via ONNX Runtime), then projects the results onto the mesh. Build it only if the geometric solver from the Auto Point Reference work falls short on realistic heads. It must stay offline, with a model license compatible with MIT.
  - **Effort**: ~2–3 weeks (model selection, runtime integration, accuracy comparison)
  - **Value**: May improve accuracy on realistic human faces; unlikely to help on stylized heads

- [ ] **Batch processing**
  - Queue multiple target meshes against a single morph mesh with shared point correspondences.
  - **Effort**: ~1–2 weeks
  - **Value**: Studio workflows creating character rosters

- [ ] **Real-time deformation preview**
  - Preview morph result while dragging parameter sliders, without re-running the full NRICP pipeline.
  - **Effort**: ~2 weeks (requires incremental deformation approach)
  - **Value**: Tightens the parameter-tuning feedback loop

---

## 🎯 Future Phases

- [ ] **macOS and Linux support** — cross-platform build and distribution
- [ ] **Hair and body mesh morphing** — extend the pipeline beyond the head mesh
- [ ] **Direct Unreal Engine integration** — plugin that imports morphed meshes without manual FBX export
- [ ] **Plugin architecture** — third-party brushes, preset packs, export targets
- [ ] **Animation / blend shape support** — morph across a pose set rather than a single rest pose
- [ ] **VR sculpting support** — Stage 4 refinement in a VR viewport

---

## 📝 Notes

**Current direction**: Sprint 8 restores the standalone open-source application.
Commercial/web follow-ups are retired; historical details are retained in the changelog.

**Prioritization Criteria**:
1. **User impact** — does it directly improve the core morph-to-export workflow?
2. **Strategic value** — does it improve the standalone open-source workflow or unlock a new user segment?
3. **Implementation effort** — time and complexity to build on the post-migration ImGui/GLFW stack
4. **Maintenance burden** — ongoing support cost, especially for ML-dependent features

---

## 🔗 Related Documents

- [Backlog Changelog](./backlog_changelog.md) — Completed items archive
- [Product Vision](../product-vision.md) — Strategic goals and current state
- [Sprint Plan](../sprints/SPRINT_PLAN.md) — v2.0 sprint plan (aggregated)
