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

- [x] **Cottage-cheese-face — root-cause spike** *(Sprint 7 Story 7.5, closed with "Outcome B stopgap is the answer" decision)*
  - **Closed 2026-05-04 with explicit decision per Sprint 6 retro AI #2 framing.** See [cottage_cheese_face_decision.md](./cottage_cheese_face_decision.md) for the full close-out rationale + the residual-dump-backed criteria that would reopen the spike.
  - Three-sprint deferral closed without a code change to NRICPDeformer. Each of the three audit-flagged hypothesis sites was reviewed: Hypothesis 1 (constant landmark weight at [NRICPDeformer.cpp:764-794](../../../src/deformation/NRICPDeformer.cpp#L764)) is the standard NRICP formulation, not a bug; Hypothesis 2 (UV-seam BFS at [NRICPDeformer.cpp:198-285](../../../src/deformation/NRICPDeformer.cpp#L198)) cannot be validated without a stable repro fixture + residual-dump data; Hypothesis 3 (excluded-vertex landmark contribution at [NRICPDeformer.cpp:354, 391](../../../src/deformation/NRICPDeformer.cpp#L354)) is a precondition issue, not an indexing defect.
  - The Sprint 5 Outcome B stopgap (StageSidebar amber warning at `landmarkWeight / alphaFinal > 0.5` + the `METAVISAGE_DUMP_RESIDUALS=1` diagnostic-dump scaffolding) stays in place as the user-facing fix.
  - **Reopen criteria**: a stable repro fixture + residual-dump data correlating peaks against landmarks / seams / scattered. A backlog item should be filed if/when those preconditions exist; the decision document captures the workflow.

- [ ] **Auto-save and crash recovery**
  - Periodic project snapshots to a `.mvautosave` sidecar file. On launch after unclean shutdown, offer to restore.
  - **Effort**: ~3–5 days
  - **Value**: Data loss during long morph sessions is the highest-severity user complaint category not addressed in v2.0

---

## 🟡 Medium Priority

- [x] **License Status modal redesign + title-bar account chip** *(Sprint 7 live-test feedback, pulled in from Sprint 6 retro AI #3)*
  - Shipped in Sprint 7 instead of the originally-planned Sprint 8 deferral, after the user surfaced it during live testing. The redesigned modal renders a dedicated header band (sparkle glyph + title + X close button), three sectioned bodies (ACCOUNT / LICENSE / USAGE THIS MONTH with mono-uppercase eyebrows), the existing `LicenseRow` primitive for label/value pairs, a tier-coloured `TIER` badge, a `KEY` row in mono, the `STATUS` pill + "Last sync: Xm ago" via a new `LastSyncedLabel` helper, a `↻ Refresh license` button wired to `LicenseManager::RequestRefresh()`, a tier-aware progress bar (`ImGui::ProgressBar` recoloured amber within 2 of the cap), and a footer band carrying the `Manage subscription →` text link plus right-aligned `Sign out` (red) and `Close` (primary blue) buttons. The title-bar account chip renders to the right of the Autosave indicator: tier pill (FREE muted / INDIE blue / PRO amber), email (mono, truncated past 24 chars), tier-ringed initials avatar pulled from the email's local part. Click anywhere on the chip opens the License Status modal. Graceful degradation when the menu bar runs out of room — chip wins, save indicator drops first.
  - **The "ANNUAL" plan badge from the original mockup remains out of scope** — no contract field on the JWT or exchange response carries billing cadence.
  - **Effort actual**: ~290 LoC (modal redesign + chip render in `AppWindow::RenderMainMenuBar` + `LastSyncedLabel` helper).

- [/] **Custom wrap preset saving** *(Sprint 4, in progress — Story 4.4)*
  - v2.0 ships with Quick Wrap and Fantasy Wrap presets. Users have asked to save their own tuned parameter sets as presets with thumbnails.
  - **Effort**: ~3–4 days
  - **Value**: Power-user retention; reduces re-tuning effort across projects

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
