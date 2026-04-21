# Product Backlog

**Project**: MetaVisage
**Last Updated**: April 20, 2026

> Active v2.0 work is tracked in `.claude/context/sprints/SPRINT_PLAN.md` and individual sprint files seeded via `/sprint-planner`. This backlog holds items **not yet committed to a sprint** — candidates pulled from v1.0.0 feedback that fell out of v2.0 scope, and post-v2.0 enhancements from the PRD.

---

## 🔥 High Priority

- [ ] **Investigate "cottage cheese face" morph artifacts**
  - Users report geometry stretching at correspondence point locations after morphing. Needs diagnostic work: determine whether this is an NRICP parameter-tuning issue, a point-density issue, or a normal-recomputation bug.
  - **Effort**: ~1–2 weeks (investigation + fix)
  - **Value**: Directly affects output quality — the feature users pay for

- [ ] **Auto-save and crash recovery**
  - Periodic project snapshots to a `.mvautosave` sidecar file. On launch after unclean shutdown, offer to restore.
  - **Effort**: ~3–5 days
  - **Value**: Data loss during long morph sessions is the highest-severity user complaint category not addressed in v2.0

---

## 🟡 Medium Priority

- [ ] **Custom wrap preset saving**
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
