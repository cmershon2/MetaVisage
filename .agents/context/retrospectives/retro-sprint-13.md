# Sprint 13 Retrospective

**Date**: 2026-11-03
**Sprint**: [Sprint 13: Stage 2 Auto-map Workflow](../sprints/sprint-13.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 13 points, plus a 2-point stretch
- **Velocity (Actual)**: 15 points (stretch 13.6 included)
- **Sprint Goal Achievement**: 100%
- **Stories Completed**: 6 of 6
- **Carryover Items**: None
- **Tangential Work**:
  - Keyboard shortcuts reference gains a Stage 2 group (place, drag, Alt + drag, Delete).
  - Re-running "Fill missing" with nothing missing no longer adds an empty undo step.
  - Two rounds of fixes from Casey's hands-on review: mirrored partners and locked midline points land on the exact surface instead of the nearest vertex; confidence rings are target-only with a "Show confidence rings" setting; locked midline drags follow the cursor on targets scaled in Stage 1.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | `ui_smoke.cpp`: new `auto_map_workflow` scenario (prompt, apply, undo/redo, mirrored drag at the exact reflection, Alt drag, locked midline on the plane within 0.01, unlock, a locked drag on a 2.5x-scaled target, Replace keeps a moved point, Cancel, project switch mid-run). `v1_backward_compat.cpp`: older files load points as manual; point landmark fields, the auto-map plane and both Stage 2 settings round-trip in v3 | Uses the bundled head as source and target, so the solve is fast and the answers are known |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK, no new warnings. `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed (78 s). `smoke_ui` 15/15 (19 s) | `git diff --check` clean. `.\build-mingw.ps1` not run (shares `build\`; backlog) |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Casey auto-mapped and dragged points on dataset heads in the app across three rounds; final verdict: "buttery smooth now - exactly what we are looking for" | Facescan (~900k vertices) included |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | Yes | Casey reviewed the prompt, rings, sidebar list and dragging live in the app | Screenshots were not captured or committed |
| Impeccable design pass | No | Not installed | `MvStyle` tokens and existing components used throughout |
| Loading / empty / error states | Yes | Automated: cancel, project switch mid-run, Fill with nothing missing. By design: no profile or no target disables Auto-map with a tooltip; a refused solve shows the solver's message | A refused solve is covered by the solver smoke, not the UI scenario |
| Known issues | Yes | Global shortcuts (Ctrl+Z, Ctrl+N) still work while the progress modal is open; safe, since results apply against the current points and a project switch drops them. The external user docs site has not been updated for auto-map | See action items |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 5 | Every story has automated checks, including the scaled-target case Casey found |
| Visual consistency | 4 | `MvStyle` tokens, existing modal and sidebar patterns, Lucide lock icons; no screenshot review committed |
| Loading behavior | 5 | Solve on a worker with progress and Cancel; the UI keeps drawing |
| Navigation completeness | 5 | Prompt on Continue, Auto-map button for re-runs, undo for every edit |
| Error handling | 4 | Refusals, cancel and project switches leave points unchanged; the refused-solve path is not exercised in the UI scenario |

---

## Retrospective

### START (Start doing)
- Testing with a scaled target from the start. Every synthetic and UI fixture used the bundled head at scale 1, so a local-versus-world unit mix-up in the midline pick shipped to Casey's review. Real heads are usually scaled in Stage 1.
- Checking that a new test fails against the old bug before trusting it. The first scaled-drag test passed even with the bug put back, which showed the fix belonged in removing the filter, not patching it.

### STOP (Stop doing)
- Snapping to the nearest vertex by default for derived positions (mirror partners, plane-locked points). Placement snaps to a vertex index, but the visible position should follow the surface; Casey felt the difference at once.

### CONTINUE (Keep doing)
- Short hands-on review loops with Casey. Three rounds, each with a precise report ("hit or miss, better from the side"), turned feel problems into concrete causes within the sprint.
- Keeping all drag geometry in one place (AppWindow) with the viewport only reporting press, ray and release. The fixes touched one function each time.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Update the MetaVisage docs site for auto-map, confidence rings and point dragging | Casey / docs | Before the next release | [ ] |
| 2 | Plan Sprint 14 (custom source profiles), including which landmarks a custom profile needs for auto-map | Sprint 14 planner | Sprint 14 planning | [ ] |
| 3 | Decide whether to move the profile's cheekbone, jaw middle and ear-top points (carried from Sprint 12) | Casey | Before Sprint 14 planning | [ ] |
| 4 | Grow the auto-map dataset with heads kept out of tuning (carried from Sprint 12) | Casey | Before further solver work | [ ] |
| 5 | Measure an Unreal or Maya FBX export's fingerprint (carried since Sprint 11) | Casey / implementer | Before Sprint 14 | [ ] |
| 6 | Give `build-mingw.ps1` its own build directory (carried since Sprint 10) | Next sprint implementer | Next sprint | [ ] Backlog |

---

## Previous Action Items Review

| # | Action (from Sprint 12) | Status |
|---|---------------------------|--------|
| 1 | Decide whether to move the profile's cheekbone, jaw middle and ear-top points | ⏸ Deferred at Sprint 13 approval (carried as action 3) |
| 2 | Plan click-and-drag point refinement into Sprint 13 | ✅ Done (13.4 and 13.5) |
| 3 | Grow the dataset with heads kept out of tuning | ❌ Not started (carried as action 4) |
| 4 | Measure an Unreal or Maya FBX export's fingerprint | ❌ Not started (carried as action 5) |
| 5 | Give `build-mingw.ps1` its own build directory | ❌ Not started (carried as action 6) |

---

## Key Metrics
- **Velocity Trend**: Up (13 -> 15 points, stretch delivered)
- **Team Morale**: High; Casey: "exactly what we are looking for"
- **Process Health**: All steps in order: plan, approval, implementation, three hands-on review rounds with fixes, code review, retro.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. Sprint 14 planning should take action items 2 and 3 into account.
