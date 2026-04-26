# Sprint 3.6 Retrospective

**Date**: 2026-04-26
**Sprint**: [Sprint 3.6 — Framework Migration: UI Layer (Sidebars, Dialogs & System Services)](../sprints/sprint-03.6.md)
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary

- **Velocity (Planned)**: 26 points
- **Velocity (Actual)**: 24 points (Stories 3.6.1 · 3.6.2 · 3.6.3 · 3.6.4 landed; 3.6.5 deferred)
- **Sprint Goal Achievement**: ~85% — full ImGui app shell, all four stage sidebars, every modal, NFD pickers, named-pipe single-instance, libcurl HTTP client, JSON config, and `MorphWorker` ported off `QThread`. Stage 1 → Stage 4 → Export workflow runs end-to-end on the new stack with parity to the end-of-Sprint-3 Qt build (verified via two rounds of live user testing).
- **Stories Completed**: 4 of 5
- **Carryover Items**: Story 3.6.5 (Shading-mode overlay & orientation gizmo ImGui port, 2 pts) — landed organically through PR-feedback cycles; technically delivered but never closed against its story ticket.

---

## Retrospective

### LIKED (What we liked)

- **`Project` as the per-frame source of truth removed an entire class of bug.** Sidebars read/write `Project` directly each frame, no signal/slot bus. The risk register flagged this as "high likelihood / high impact" — in practice it was the cleanest part of the port. Stage transitions, masking toggles, and morph progress all stayed coherent because there was nowhere for state to diverge.
- **Two-tiered modal pattern (state struct + `RenderXxx()` per frame)** scaled to seven dialogs (Welcome, Confirm, Error, Export, Texture Preview, Shortcuts, About) without ever hitting the nested-popup trap the risk register predicted. The `pendingOpen` → `OpenPopup` → `BeginPopupModal` discipline kept ImGui's modal stack well-behaved.
- **The `/review-pr` agent earned its keep again** — caught the `CancelTransform` Escape-commits-instead-of-reverts stub, the unresolved Sprint-3.5 `HeaderCallback` off-by-one (would have shipped a second time), the plaintext token storage spec violation, and the gizmo asset relative-path footgun. Four genuine defects, no style noise.
- **Three live user-testing rounds during the sprint surfaced 14 regressions before the sprint review** — Stage 1 gizmo axis-constraint, morph/mask save+reload, brush-hover outline, finalize-button bloat, View > Shading Mode redundancy, orientation-gizmo missing axis lines, default MetaHuman triangulated appearance + first-frame solid-render holes. Tight feedback loops worked.

### LEARNED (What we learned)

- **`GL_ELEMENT_ARRAY_BUFFER` binding is VAO-recorded state.** Binding a second IBO while the VAO was still bound silently overwrote `ibo_`'s recorded element binding, and the bug only manifested as "first solid render has holes; switching to wireframe and back fixes it" — a near-impossible-to-attribute symptom. Worth a memory: **never `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, …)` while a VAO you care about is bound, unless that's the binding you want recorded.**
- **Wireframe rendering must respect source topology, not GPU triangulation.** v1.0 Qt-era render path drew `GL_LINE` polygon mode against the fan-triangulated triangle buffer and showed every quad's diagonal — making meshes *look* triangulated even though `Mesh::faces_` always preserved quads. The fix (deduplicated edge IBO drawn with `GL_LINES`) is mesh-data-integrity work that belongs to the renderer, not the importer.
- **`metaHumanCompatible=true` + format=FBX + .obj source = silent format coercion.** The dispatch in `MeshExporter::Export` writes OBJ content regardless of the user's chosen format when the original is .obj. Discovered while auditing for triangulation; documenting now so it doesn't get re-discovered the same way next sprint.
- **NFD blocking the render loop on the main thread is the right answer.** The risk register flagged it as a Med/Med freeze risk; in practice users *expect* a modal picker to freeze the host. Trying to thread it would have been a two-day yak shave.
- **Sprint 3.5's `HeaderCallback` off-by-one re-shipped because it wasn't tracked.** Caught by review then, caught by review again now, never made it into a backlog item or a TODO comment. Review-found defects need an owner-and-deadline trail or they regress.

### LACKED (What we lacked)

- **Closed-loop on Sprint 3.5 action items.** Of the three action items from the prior retro: (1) hook absolute-path fix landed early in the sprint and held up; (2) the checked-in smoke-test project never got built; (3) the CMake guard against re-introduced Qt6::Gui/Widgets headers never got built. Items 2 and 3 were both scheduled "End of Sprint 3.6" — slipping two of three is a process signal, not a bandwidth signal.
- **No regression test for the four-stage workflow.** Every PR-feedback cycle re-tested the same Stage 1 → 2 → 3 → 4 → Export path manually. A scriptable end-to-end (load a fixture project, advance stages, export, diff result against a golden) would have caught the morph-not-visible-after-reload and mask-not-visible-after-reload bugs before user testing rather than during it.
- **Mid-sprint scope creep into rendering and persistence.** The sprint goal was UI-layer migration. The actual ship included a renderer change (quad-aware wireframe), an export-options safety default, two save/reload reconstruction paths, transform gizmo input drag logic, and Stage 2 point-symmetry mirror — all triggered by user testing. Real work, but none of it was on the sprint backlog.
- **The four `/review-pr` blockers landed *after* the sprint outcome was marked complete in the sprint file.** The doc says "Implementation Complete — manual parity smoke test pending"; the review found work that should have blocked that line. Sprint outcomes need to wait for review, not lead it.

### LONGED FOR (What we longed for)

- **A `mesh-integrity check` script.** Loads a fixture mesh, runs it through the morph + export pipeline, and asserts vertex count, face arity histogram, and UV layout match the source. Would have caught the triangulation question before the user did, and would catch any future Assimp post-process flag drift automatically.
- **Auto-tracking for `/review-pr` findings.** When the agent flags a non-blocking issue, it should land in `product_backlog.md` with a citation back to the PR. Today they live in chat output and only re-surface when the same code gets re-reviewed.
- **A "stage parity" smoke fixture** — the same ask as Sprint 3.5's "checked-in smoke-test project" action item. Now wanted twice, still not built. Promoting it from a deferred action item to a story in Sprint 3.7.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Fix the four `/review-pr` blockers (CancelTransform Escape revert, `HeaderCallback` `>=13`, gizmo asset paths via `Paths::ExecutableDir()`, plaintext token storage — at minimum file the encryption gap as a tracked product-backlog defect) | Casey | 2026-04-30 | [ ] |
| 2 | Promote the smoke-test project (Sprint 3.5 action item #2, still not built) into a Sprint 3.7 story: tiny `.mmproj` + small mesh + 2 correspondence points, loadable via `--open <path>`, used as the fixture for a CI mesh-integrity check | Casey | Mid Sprint 3.7 | [ ] |
| 3 | Land the CMake guard against `Qt6::Gui`/`Widgets`/`OpenGL`/`OpenGLWidgets` (Sprint 3.5 action item #3, still not built) — single-line CMake check so the regression cannot reappear silently | Casey | Start of Sprint 3.7 | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 3.5) | Status |
|---|--------------------------|--------|
| 1 | Fix PreToolUse hooks in `.claude/settings.json` to use `"$CLAUDE_PROJECT_DIR/..."` absolute paths | ✅ Done — hooks survived the entire sprint without cwd-drift breakage |
| 2 | Add a checked-in smoke-test project + `--open <path>` debug CLI flag for viewport parity verification | ❌ Not started — re-promoted as Sprint 3.7 action item #2 |
| 3 | Add a CMake/CI guard that fails the build if any `Qt6::Gui/Widgets/OpenGL/OpenGLWidgets` header reappears | ❌ Not started — re-promoted as Sprint 3.7 action item #3 |

---

## Key Metrics

- **Velocity Trend**: Slight decrease (26 → 24 of committed). One story deferred but its work landed informally through PR-feedback cycles. Real velocity counting that work is closer to 26.
- **Team Morale**: High — every stage of the four-stage workflow runs on the new stack; the Qt UI removal is durable.
- **Process Health**: Mixed. `/review-pr` is consistently catching real defects, the `Project` per-frame model paid off, and the modal pattern scaled cleanly. **But** action-item follow-through is poor (2/3 carryover from 3.5), sprint outcome was marked complete before review, and mid-sprint scope creep wasn't tracked. Action-item discipline and "review before outcome-complete" are the two process knobs to tighten in Sprint 3.7.
