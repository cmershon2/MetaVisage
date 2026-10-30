# Backlog Changelog

**Project**: MetaVisage
**Purpose**: Append-only archive of completed product backlog items

---

## April 2026

### 2026-04-30 — Stage 4 brush-effect previews shipped (Path A)

- ✅ **Stage 4 procedural brush-effect previews**
  - Sprint 5 Story 5.3 originally shipped brush hotkeys (1–4) and `[N]` indicators in the card sub-labels but deferred the brush-effect previews from the master plan. The deferral was filed as a Medium-priority backlog item.
  - **Effort (Estimated)**: ~1 day (Path B procedural) or ~half day plus asset turnaround (Path A).
  - **Completed**: 2026-04-30 (Sprint 5, Story 5.3 follow-up commit)
  - **Notes**: Path A (PNG assets) landed instead of Path B (procedural ImDrawList). Four brush previews authored externally and staged at `assets/brushes/{smooth,grab,pushpull,inflate}_preview.png`. New optional `ImTextureID thumbnail` parameter on `DrawPresetCard` defaults to 0 — Stage 3 preset cards continue to render Lucide glyphs unchanged; Stage 4 brush cards render the PNGs at 28×28 px with graceful fallback to the Lucide glyph if a load fails. `MvStyle::AssetHandles` extended with a 4-slot `BrushPreviewTextureIds` array; `LoadAssets` / `Shutdown` extended to manage their lifecycle. CMake's existing recursive `file(COPY assets DESTINATION ...)` step picks up `assets/brushes/` automatically on next configure.

---

## October 2026

### 2026-10-15 - Sprint 8 standalone open-source transition

- Completed removal of login, account, feature/payment gates, conversion limits/reporting, runtime web integrations, credential build profiles, and HTTP/JWT/crypto dependencies.
- **Effort (Estimated)**: Unpointed; originally authorized as a direct implementation task.
- **Completed**: Sprint 8, 2026-10-15.
- **Notes**: The owner confirmed completion and requested PR review, merge into development, and sprint closeout. Historical Sprint 7 follow-ups for licensing are retired rather than implemented.

### 2026-10-15 - Reconcile previously completed backlog entries

These entries were already shipped in earlier sprints. They are moved here during
Sprint 8 closeout without counting their effort toward Sprint 8. Custom preset
saving was stale as in-progress; save/rename/replace/delete are implemented and
documented in the existing application.

- Completed: **Re-wire Undo/Redo on the ImGui port** *(Sprint 3.7, complete)*
  - Sprint 3.5/3.6 stood up `UndoStack` and the Edit menu but the ImGui port does not push any actions onto it. Point placement (`AppWindow::onPointPlaced`), point deletion (`AppWindow::onPointDelete`), gizmo drag (`Viewport::OnMouseButton` release path — `OnTransformApplied` callback is not wired in AppWindow), and transform spinboxes (`StageSidebar::RenderAlignment`) all mutate state directly. The Qt era pushed `PointPlacementUndoAction` / transform undos at each of these sites (see `MainWindow::OnPointPlaced` line 1274 and `ApplyTransform`).
  - **Effort**: ~1–2 days
  - **Value**: Restores a v1.0 feature users rely on for iterative point placement; required before NRICP work in Sprint 4

  - **Archived**: 2026-10-15; delivered in an earlier sprint.

- Completed: **Qt6Core removal (QString / QFile / QJson / QDateTime → std)** *(Sprint 3.7, complete)*
  - Sprint 3.5 removed Qt Gui/Widgets/OpenGL/OpenGLWidgets but kept `Qt6::Core` linked as a transitional dependency. `QString` is embedded in the public API of `core/Mesh`, `core/Project`, `core/TextureData`, `core/UndoStack`, `core/UndoableAction`, `io/MeshExporter`, `io/ProjectSerializer`, and pervasively in `MV_LOG_*(QString(...).arg(...))` call sites across `deformation/*`, `baking/*`, and `utils/*`. Port to `std::string` / `std::filesystem` / `std::chrono` / `nlohmann::json` / `std::fstream`. `Qt6::Core` is LGPL so not a licensing blocker, but keeping it means ~25 MB of runtime DLLs shipped with the installer and a transitive build-system footprint on Qt tooling.
  - **Effort**: ~15–20 points (~1.5 weeks solo)
  - **Value**: Completes the Qt exit begun in Sprint 3.5; shrinks installer; removes final Qt build-tool dependency

  - **Archived**: 2026-10-15; delivered in an earlier sprint.

- Completed: **Investigate "cottage cheese face" morph artifacts** *(Sprint 7 complete - Outcome B stopgap decision)*
  - Users report geometry stretching at correspondence point locations after morphing. Sprint 7 closed the root-cause spike with the explicit "Outcome B stopgap is the answer" decision; the amber parameter warning and `METAVISAGE_DUMP_RESIDUALS=1` diagnostic path remain the supported response until a stable repro fixture and residual data justify reopening.
  - **Effort actual**: Sprint 5 stopgap + Sprint 7 decision spike
  - **Value**: Directly affects output quality - the feature users pay for

  - **Archived**: 2026-10-15; delivered in an earlier sprint.

- Completed: **Cottage-cheese-face — root-cause spike** *(Sprint 7 Story 7.5, closed with "Outcome B stopgap is the answer" decision)*
  - **Closed 2026-05-25 with explicit decision per Sprint 6 retro AI #2 framing.** See [cottage_cheese_face_decision.md](./cottage_cheese_face_decision.md) for the full close-out rationale + the residual-dump-backed criteria that would reopen the spike.
  - Three-sprint deferral closed without a code change to NRICPDeformer. Each of the three audit-flagged hypothesis sites was reviewed: Hypothesis 1 (constant landmark weight at [NRICPDeformer.cpp:764-794](../../../src/deformation/NRICPDeformer.cpp#L764)) is the standard NRICP formulation, not a bug; Hypothesis 2 (UV-seam BFS at [NRICPDeformer.cpp:198-285](../../../src/deformation/NRICPDeformer.cpp#L198)) cannot be validated without a stable repro fixture + residual-dump data; Hypothesis 3 (excluded-vertex landmark contribution at [NRICPDeformer.cpp:354, 391](../../../src/deformation/NRICPDeformer.cpp#L354)) is a precondition issue, not an indexing defect.
  - The Sprint 5 Outcome B stopgap (StageSidebar amber warning at `landmarkWeight / alphaFinal > 0.5` + the `METAVISAGE_DUMP_RESIDUALS=1` diagnostic-dump scaffolding) stays in place as the user-facing fix.
  - **Reopen criteria**: a stable repro fixture + residual-dump data correlating peaks against landmarks / seams / scattered. A backlog item should be filed if/when those preconditions exist; the decision document captures the workflow.

  - **Archived**: 2026-10-15; delivered in an earlier sprint.

- Completed: **License Status modal redesign + title-bar account chip** *(Sprint 7 live-test feedback, pulled in from Sprint 6 retro AI #3)*
  - Shipped in Sprint 7 instead of the originally-planned Sprint 8 deferral, after the user surfaced it during live testing. The redesigned modal renders a dedicated header band (sparkle glyph + title + X close button), three sectioned bodies (ACCOUNT / LICENSE / USAGE THIS MONTH with mono-uppercase eyebrows), the existing `LicenseRow` primitive for label/value pairs, a tier-coloured `TIER` badge, a `KEY` row in mono, the `STATUS` pill + "Last sync: Xm ago" via a new `LastSyncedLabel` helper, a `↻ Refresh license` button wired to `LicenseManager::RequestRefresh()`, a tier-aware progress bar (`ImGui::ProgressBar` recoloured amber within 2 of the cap), and a footer band carrying the `Manage subscription →` text link plus right-aligned `Sign out` (red) and `Close` (primary blue) buttons. The title-bar account chip renders to the right of the Autosave indicator: tier pill (FREE muted / INDIE blue / PRO amber), email (mono, truncated past 24 chars), tier-ringed initials avatar pulled from the email's local part. Click anywhere on the chip opens the License Status modal. Graceful degradation when the menu bar runs out of room — chip wins, save indicator drops first.
  - **The "ANNUAL" plan badge from the original mockup remains out of scope** — no contract field on the JWT or exchange response carries billing cadence.
  - **Effort actual**: ~290 LoC (modal redesign + chip render in `AppWindow::RenderMainMenuBar` + `LastSyncedLabel` helper).

  - **Archived**: 2026-10-15; delivered in an earlier sprint.

- Completed: **Custom wrap preset saving** *(Sprint 4, in progress — Story 4.4)*
  - v2.0 ships with Quick Wrap and Fantasy Wrap presets. Users have asked to save their own tuned parameter sets as presets with thumbnails.
  - **Effort**: ~3–4 days
  - **Value**: Power-user retention; reduces re-tuning effort across projects


  - **Archived**: 2026-10-15; delivered in an earlier sprint.
### 2026-10-15 - Retire commercial/web follow-ups

These items are superseded by the owner's standalone/offline mandate. Their
original descriptions are retained below as historical context; they were not
implemented or counted as completed delivery.

- [ ] **Staging licensing endpoint deployment for desktop release QA**
  - `/api/desktop/token` and `/api/license/conversion` need to be live in staging so desktop release testing can validate the same auth refresh, feature flag, conversion reporting, and limit-reached paths users will hit outside the localhost development portal.
  - **Effort**: Web-team deployment + desktop verification pass
  - **Value**: Converts Sprint 6/7 development-profile validation into release-readiness evidence

- [ ] **Conversion reporting idempotency / account-scoped queue ownership**
  - Sprint 7 hardened the local durable queue so reports are removed only after server confirmation, but a hard crash after a successful POST and before local drain can still duplicate a conversion on next launch. Coordinate with the web team on an idempotency key and/or persist queue ownership by account/license so queued reports cannot cross sessions incorrectly.
  - **Effort**: ~1–2 days desktop + web API coordination
  - **Value**: Protects billing accuracy and user trust around paid conversion limits

- [ ] **Paid-feature visual QA checklist**
  - Capture a compact checklist with screenshots or frames for Free / Indie / Pro gate states, including standard locked inspector rows, hover cards, ToolRail gates, and brush preview preservation.
  - **Effort**: ~0.5 day
  - **Value**: Keeps future UI-heavy PRs aligned with the paid-feature CTA standard without relying on memory from live review

- [ ] **Cloud rendering** — offload long morph computations to a remote worker

### 2026-10-30 - Sprint 9 Alignment stage usability fixes

- Completed: **Alignment gizmo moves at cursor speed**, **Alignment tool hotkeys match the tool rail** (kept Blender G/R/S; rail labels corrected), **Typed entry for target transform fields**, **Target textures persist across target mesh replacement**, **Remove placeholder Alignment rail tools**.
- **Effort (Estimated)**: 10 points.
- **Completed**: Sprint 9, 2026-10-30.
- **Notes**: Casey confirmed by hand, including a full Stage 1 -> 4 -> Export run. Numpad view checks are automated only; hand-testing them moves to the Sprint 10 View menu. Also fixed the degenerate Numpad 7 top view. See `retrospectives/retro-sprint-09.md`.

### 2026-10-30 - Sprint 10 app shell and Morph stage polish

- Completed: **Debug HUD hidden by default**, **Remove stub Mesh menu**, **Working View menu** (plus a viewport-toolbar projection toggle and Auto Perspective), **Menu spacing pass**, **MetaVisage icon on the executable and window**, **Reset morph result to source mesh** (undoable; "Reset to defaults" now resets parameters only), **Vertex mask in its own Morph section**.
- **Effort (Estimated)**: 12 points.
- **Completed**: Sprint 10, 2026-10-30.
- **Notes**: Casey confirmed by hand, including a full Stage 1 -> 4 -> Export run. This completes the 2026-10-29 feedback triage. See `retrospectives/retro-sprint-10.md`.

### 2026-10-30 - Sprint 11 Source Profiles

- ✅ **Source Profiles and built-in MetaHuman profile**
  - Added a Source Profile (source mesh, topology fingerprint, landmarks, default mask, default transform), a `ProfileStore`, and the authored built-in MetaHuman profile. The Stage 1 source picker selects a profile, and projects store `sourceProfileId` (format v3).
  - **Effort (Estimated)**: ~1 sprint (13 pts)
  - **Completed**: Sprint 11, 2026-10-30
  - **Notes**: Casey authored the 52 landmarks and the mouth-interior mask. Also fixed: reopened projects lost the vertex mask and newer wrap parameters; a new source import kept the old `originalMorphMesh`; undo did not refresh the mask overlay. See [retro-sprint-11.md](../retrospectives/retro-sprint-11.md).
