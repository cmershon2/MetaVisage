# Product Backlog

**Project**: MetaVisage
**Last Updated**: October 30, 2026

> Active v2.0 work is tracked in `.agents/context/sprints/SPRINT_PLAN.md` and individual sprint files seeded via `$sprint-planner`. Sprint 8 closes the standalone transition. This backlog holds items **not yet committed to a sprint** — candidates pulled from v1.0.0 feedback that fell out of v2.0 scope, and post-v2.0 enhancements from the PRD.

---

## 🔥 High Priority

### Feedback triage 2026-10-29 — planned for Sprint 10

The Sprint 9 items from this triage are complete and archived in `backlog_changelog.md`.

- [ ] **Debug HUD hidden by default** — Sprint 10
- [ ] **Remove stub Mesh menu** — Sprint 10
- [ ] **Working View menu** — Sprint 10
- [ ] **Menu spacing pass** — Sprint 10
- [ ] **MetaVisage icon on the executable and window** — Sprint 10
- [ ] **Reset morph result to source mesh** — Sprint 10 (includes fixing "Reset to defaults" wiping the result)
- [ ] **Vertex mask in its own Morph section** — Sprint 10

### Other

- [ ] **Auto-save and crash recovery**
  - Periodic project snapshots to a `.mvautosave` sidecar file. On launch after unclean shutdown, offer to restore.
  - **Effort**: ~3–5 days
  - **Value**: Data loss during long morph sessions is the highest-severity user complaint category not addressed in v2.0

---

## 🟡 Medium Priority

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
