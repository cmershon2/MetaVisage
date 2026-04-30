# Product Backlog

**Project**: MetaVisage
**Last Updated**: April 20, 2026

> Active v2.0 work is tracked in `.claude/context/sprints/SPRINT_PLAN.md` and individual sprint files seeded via `/sprint-planner`. This backlog holds items **not yet committed to a sprint** — candidates pulled from v1.0.0 feedback that fell out of v2.0 scope, and post-v2.0 enhancements from the PRD.

---

## 🔥 High Priority

- [x] **Re-wire Undo/Redo on the ImGui port** *(Sprint 3.7, complete)*
  - Sprint 3.5/3.6 stood up `UndoStack` and the Edit menu but the ImGui port does not push any actions onto it. Point placement (`AppWindow::onPointPlaced`), point deletion (`AppWindow::onPointDelete`), gizmo drag (`Viewport::OnMouseButton` release path — `OnTransformApplied` callback is not wired in AppWindow), and transform spinboxes (`StageSidebar::RenderAlignment`) all mutate state directly. The Qt era pushed `PointPlacementUndoAction` / transform undos at each of these sites (see `MainWindow::OnPointPlaced` line 1274 and `ApplyTransform`).
  - **Effort**: ~1–2 days
  - **Value**: Restores a v1.0 feature users rely on for iterative point placement; required before NRICP work in Sprint 4

- [x] **Qt6Core removal (QString / QFile / QJson / QDateTime → std)** *(Sprint 3.7, complete)*
  - Sprint 3.5 removed Qt Gui/Widgets/OpenGL/OpenGLWidgets but kept `Qt6::Core` linked as a transitional dependency. `QString` is embedded in the public API of `core/Mesh`, `core/Project`, `core/TextureData`, `core/UndoStack`, `core/UndoableAction`, `io/MeshExporter`, `io/ProjectSerializer`, and pervasively in `MV_LOG_*(QString(...).arg(...))` call sites across `deformation/*`, `baking/*`, and `utils/*`. Port to `std::string` / `std::filesystem` / `std::chrono` / `nlohmann::json` / `std::fstream`. `Qt6::Core` is LGPL so not a licensing blocker, but keeping it means ~25 MB of runtime DLLs shipped with the installer and a transitive build-system footprint on Qt tooling.
  - **Effort**: ~15–20 points (~1.5 weeks solo)
  - **Value**: Completes the Qt exit begun in Sprint 3.5; shrinks installer; removes final Qt build-tool dependency

- [/] **Investigate "cottage cheese face" morph artifacts** *(Sprint 5, in progress — Story 5.2 shipped Outcome B stopgap)*
  - Users report geometry stretching at correspondence point locations after morphing. Needs diagnostic work: determine whether this is an NRICP parameter-tuning issue, a point-density issue, or a normal-recomputation bug.
  - **Effort**: ~1–2 weeks (investigation + fix)
  - **Value**: Directly affects output quality — the feature users pay for

- [ ] **Cottage-cheese-face — root-cause spike** *(follow-up to Sprint 5 Story 5.2 Outcome B)*
  - Sprint 5 Story 5.2 shipped the stopgap UI warning (landmark-weight / stiffness-final ratio > 0.5) plus diagnostic dump scaffolding gated by `METAVISAGE_DUMP_RESIDUALS=1` (writes `%TEMP%\metavisage_residuals_<epochMs>_step<N>.csv` per stiffness step). The full root-cause spike still needs to run: (1) reproduce reliably with dense landmark clusters, (2) inspect residual dumps for landmark-vs-boundary correlation, (3) test the three audit-flagged hypotheses — landmark-weight floor relative to alpha at [src/deformation/NRICPDeformer.cpp:764–790](../../../src/deformation/NRICPDeformer.cpp#L764), UV-seam BFS over-exclusion at [NRICPDeformer.cpp:198–286](../../../src/deformation/NRICPDeformer.cpp#L198), excluded-vertex landmark contribution mismatch at [NRICPDeformer.cpp:350, 387](../../../src/deformation/NRICPDeformer.cpp#L350), (4) ship the fix + a regression case in `smoke_wrap_parity` covering the dense-landmark reproduction, (5) tighten or remove the inspector warning threshold once the underlying bug is closed.
  - **Effort**: ~1 week (investigation + fix + regression test)
  - **Value**: Closes the root cause and lets us drop the stopgap warning; restores Quick Wrap as a no-warning default for typical landmark-weight values.

- [ ] **Auto-save and crash recovery**
  - Periodic project snapshots to a `.mvautosave` sidecar file. On launch after unclean shutdown, offer to restore.
  - **Effort**: ~3–5 days
  - **Value**: Data loss during long morph sessions is the highest-severity user complaint category not addressed in v2.0

---

## 🟡 Medium Priority

- [/] **Custom wrap preset saving** *(Sprint 4, in progress — Story 4.4)*
  - v2.0 ships with Quick Wrap and Fantasy Wrap presets. Users have asked to save their own tuned parameter sets as presets with thumbnails.
  - **Effort**: ~3–4 days
  - **Value**: Power-user retention; reduces re-tuning effort across projects

- [ ] **Texture thumbnail preview modal**
  - v2.0 allows unlimited texture uploads with small thumbnails. Clicking the thumbnail should open a full-size preview modal.
  - **Effort**: ~1 day
  - **Value**: Quality-of-life polish for texture workflow

- [ ] **Stage 4 procedural brush-effect previews** *(follow-up to Sprint 5 Story 5.3)*
  - Sprint 5 Story 5.3 shipped brush hotkeys (1–4) and `[N]` indicators in the card sub-labels but deferred the brush-effect previews from the master plan (PNG thumbnails or procedural ImDrawList shapes per [SPRINT_PLAN.md § Sprint 5](../sprints/SPRINT_PLAN.md)). The deferred work: replace the centred Lucide glyph in each Stage 4 brush card with a small (~28×28px) procedural visualization showing the brush effect — e.g. Smooth = wavy line straightening, Grab = horizontal line with one point pulled, Push/Pull = bumpy line with displaced section, Inflate = small circle to large. Path A (PNG assets at `assets/brushes/{smooth,grab,pushpull,inflate}_preview.png` loaded via stb_image) and Path B (procedural `ImDrawList::Add*` calls inside `DrawPresetCard` or a new `DrawBrushCard` sibling) are both viable per the Sprint 5 plan; pick whichever is faster after this lands.
  - **Effort**: ~1 day (Path B procedural) or ~half day plus asset turnaround (Path A).
  - **Value**: Closes Story 5.3's master-plan DoD; users see at-a-glance what each brush does before committing to a stroke.

- [ ] **Advanced sculpting brushes**
  - Crease, pinch, and masking brushes beyond the v1.0 smooth/grab/push-pull/inflate-deflate set.
  - **Effort**: ~1 week
  - **Value**: Closes the feature gap with commercial sculpting tools for Stage 4 refinement

---

## 🟢 Lower Priority

- [ ] **Automated point placement (ML)**
  - Use a facial-landmark model to pre-place correspondence points, letting the user adjust rather than place from scratch.
  - **Effort**: ~3–4 weeks (model selection, integration, UX)
  - **Value**: Significant time savings for Stage 2; differentiates from commercial tools

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
- [ ] **Cloud rendering** — offload long morph computations to a remote worker

---

## 📝 Notes

**Prioritization Criteria**:
1. **User impact** — does it directly improve the core morph-to-export workflow?
2. **Strategic value** — does it advance the v2.0 commercial goals or unlock a new user segment?
3. **Implementation effort** — time and complexity to build on the post-migration ImGui/GLFW stack
4. **Maintenance burden** — ongoing support cost, especially for ML-dependent features

---

## 🔗 Related Documents

- [Backlog Changelog](./backlog_changelog.md) — Completed items archive
- [Product Vision](../product-vision.md) — Strategic goals and current state
- [Sprint Plan](../sprints/SPRINT_PLAN.md) — v2.0 sprint plan (aggregated)
