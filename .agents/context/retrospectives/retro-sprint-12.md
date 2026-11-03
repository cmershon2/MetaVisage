# Sprint 12 Retrospective

**Date**: 2026-11-03
**Sprint**: [Sprint 12: Auto-map Landmark Solver](../sprints/sprint-12.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 13 points
- **Velocity (Actual)**: 13 points
- **Sprint Goal Achievement**: 100% (the headless solver places all 52 landmarks with confidence; real-head accuracy is informational this sprint)
- **Stories Completed**: 6 of 6
- **Carryover Items**: None
- **Tangential Work**:
  - Affine refinement after the similarity fit (stretch case worst 3.2% -> 0.8%).
  - Facing check that refuses a target turned 180 degrees about the vertical.
  - Confidence from the neighbourhood fit and surface shape, not just the projection distance.
  - After Casey's Stage 2 review: per-feature refit for eyes and ears, crease snap for the mouth corners and lip contact, and bench flags (`--no-refit`, `--no-snap`, `--regions`, `--set`) to measure each step on its own.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | New `tests/smoke/landmark_solver.cpp` (`smoke_landmark_solver`): identity, units and origin, stretch, bulges, bulges + noise, misaligned, large ears (refit must beat no refit), rotated 180 (refused), cancel, inputs unchanged, no landmarks | Bulge cases carry a 0.6% median bar (measured 0.52%); all other cases use the planned 0.5% / 2% bars |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK, no new warnings. `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed (71 s; `smoke_landmark_solver` 23 s) | `git diff --check` clean. `.\build-mingw.ps1` not run (shares `build\`; backlog) |
| Accuracy report | Yes | Synthetic: medians 0.09–0.52%, worst 1.83%, about 2 s per case. Six real heads (vs Casey's clicks, % of diagonal): medians 0.56–1.70%, 3–6 s each | Full tables in the sprint outcome and the design doc |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Casey opened the `.automap.mmproj` results for all six heads in Stage 2, twice (before and after the refit and snap) | First review flagged lower lids, mouth corners, ears and the goblin's nose; second review: "about the same" |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | No | N/A | No new UI; Casey's review screenshots were not committed |
| Loading / empty / error states | Yes | Automated: no landmarks, cancelled solve, backwards target and far-off alignment all fail with a clear message and no points | User-facing messages never name the algorithm |
| Known issues | Yes | Cartoon-female lower lids 2.9% off; goblin ear tops ~13% and nose tip 4.7% off. Cheekbone, jaw middle and ear-top points are offset the same way on every head, and confidence can't detect that | Sprint 13's drag-to-refine editing covers the manual fix; the profile question is action item 1 |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 5 | Every pipeline step has a synthetic case with a measured bar, including one that fails if the feature refit stops helping |
| Visual consistency | N/A | No UI this sprint |
| Loading behavior | 4 | The solver takes a progress callback and cancel flag and is UI-free, ready for a worker in Sprint 13; not yet wired to the UI |
| Navigation completeness | N/A | No UI this sprint |
| Error handling | 5 | Divergence, facing, missing input and cancel are all refused cleanly; inputs are never modified |

---

## Retrospective

### START (Start doing)
- Building ablation switches into a tuning tool from the start. The first refit attempt changed nothing, and that only became clear once the bench could turn each step off; with the switches built in up front, the dead end would have shown up in the first run.
- Reporting real-head accuracy in terms the reviewer sees. Medians across 52 points hid that the specific points Casey looks at (lids, mouth corners, ears) were the worst; per-area numbers should lead.

### STOP (Stop doing)
- Tuning against six heads without a held-out set. Several tried steps helped about as many heads as they hurt; with only six heads, a gain on some of them is weak evidence that a step works in general. More heads, some kept out of tuning, should come before further tuning.
- Editing C++ string literals through Python here-strings. `\n` and `\b` were turned into real control characters three times, each costing a failed build or a broken doc line. Use the Edit tool for literals.

### CONTINUE (Keep doing)
- Comparing the solver against Casey's own hand-placed points. That comparison separated solver misses from places where the profile disagrees with Casey (cheekbones offset the same way on every head), which a synthetic test cannot do.
- Recording dropped attempts with their measured reason (mouth and nose refit, tip and ear snaps), so they aren't retried blindly.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Decide whether to move the profile's cheekbone, jaw middle and ear-top vertices to where Casey clicks them on targets | Casey | Before Sprint 13 planning | [ ] |
| 2 | Plan click-and-drag point refinement (midline lock with unlock, mirrored drag) into Sprint 13 | Sprint 13 planner | Sprint 13 planning | [ ] Backlog |
| 3 | Grow the dataset with a few heads kept out of tuning before tuning the solver further | Casey | Before further solver work | [ ] |
| 4 | Export the MetaHuman head from Unreal (or Maya) as FBX and measure its fingerprint (carried from Sprint 11) | Casey / implementer | Before Sprint 13 | [ ] |
| 5 | Give `build-mingw.ps1` its own build directory (carried since Sprint 10) | Next sprint implementer | Next sprint | [ ] Backlog |

---

## Previous Action Items Review

| # | Action (from Sprint 11) | Status |
|---|---------------------------|--------|
| 1 | Confirm design decisions 1 and 3 in `auto-point-reference.md` | ✅ Done (Casey, 2026-10-31) |
| 2 | Measure an Unreal or Maya FBX export's fingerprint against the built-in profile | ❌ Not started (carried as action 4) |
| 3 | Give `build-mingw.ps1` its own build directory | ❌ Not started (still in the backlog; carried as action 5) |

---

## Key Metrics
- **Velocity Trend**: Stable (13 -> 13 points, both fully delivered)
- **Team Morale**: Good. The solver works end to end, but Casey saw little visible change from the review round
- **Process Health**: All steps in order: design decisions, plan, approval, implementation, owner dataset and review, review follow-up, code review, retro.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. Sprint 13 planning should take action items 1 and 2 into account.
