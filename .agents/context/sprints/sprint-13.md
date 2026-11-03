# Sprint 13: Stage 2 Auto-map Workflow

**Sprint Goal**: From Stage 1, a user can auto-map the profile's landmarks in
one click, see which points need checking, and fix any point by dragging it
along the mesh, with mirrored pairs and midline points behaving sensibly.
**Duration**: 2026-11-04 — 2026-11-10
**Capacity**: ~13 points (Sprint 11 and 12 velocity: 13)
**Committed**: 13 points, plus a 2-point stretch story
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Approved by Casey on 2026-11-03 ("Approve as planned")

Source: [auto-point-reference.md](../design/auto-point-reference.md), "Stage 2
workflow (Sprint 13)", including Casey's 2026-11-03 drag-to-refine request.
Builds on the Sprint 12 `TemplateFitSolver` (headless, cancellable, never
changes Stage 1 transforms).

Branch: `feat/sprint-13-automap-workflow`, from `development` after the Sprint
12 merge (cmershon2/MetaVisage#32).

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [x] Sprint plan approved before execution
- [x] Approved scope executed
- [x] Tests run and validation recorded
- [x] Sprint completion confirmed (Casey, 2026-11-03: "Let's go ahead and close this out")
- [x] Retrospective saved in `.agents/context/retrospectives/`
- [x] Context updated with `$update-context`
- [x] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot (N/A: no campaign)

---

## Sprint Backlog

Backlog item: **Stage 2 auto-map workflow**.

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 13.1 | **Landmark data on points** | 2 | None | [x] |
| 13.2 | **Auto-map run: prompt, progress, apply** | 3 | 13.1 | [x] |
| 13.3 | **Confidence display and review list** | 2 | 13.2 | [x] |
| 13.4 | **Drag points along the surface** | 3 | 13.1 | [x] |
| 13.5 | **Midline lock and mirrored drag** | 3 | 13.4 | [x] |
| 13.6 | *Stretch:* **Re-run auto-map (replace or fill)** | 2 | 13.2 | [x] |

### 13.1 Landmark data on points

- [x] `PointCorrespondence` gains `landmarkId`, `confidence` (0–1, or none for
      manual points), `isAuto` and `midlineUnlocked`. The Sprint 11
      `landmarkName` field becomes a lookup from `landmarkId` through the
      project's source profile, so names survive save and reopen.
- [x] Serialized in project format v3 as optional fields. Older v3 files load
      unchanged (points without them are manual points). The v1/v2
      compatibility smoke stays green and gains a v3 round-trip of the new
      fields.
- [x] Editing a point (drag, or re-placing it) clears `isAuto` and its
      confidence: the user has checked it.
- [x] Help > Show Profile Landmarks (Dev) uses the same fields instead of the
      temporary name.

### 13.2 Auto-map run: prompt, progress, apply

- [x] **Prompt.** On Continue from Stage 1, if the project's source profile has
      landmarks and Stage 2 has no points, a modal offers **Auto-map points**
      (primary) or **Place manually**. With no profile or with existing points,
      Continue goes straight to Stage 2 as today.
- [x] **Worker.** The solver runs on a worker thread, as the Stage 3 morph
      does: inputs are copied or shared up front, and a result is dropped if
      the user opened or created another project meanwhile. A modal shows
      progress and Cancel; the UI keeps drawing.
- [x] **Apply.** On success, the 52 landmarks become paired Stage 2 points: the
      source side is the profile vertex, the target side is the solved
      position. Left/right landmarks are linked as symmetric pairs, so the
      existing delete-both-of-a-pair behaviour applies. The whole auto-map is
      one undo step ("Auto-map Points").
- [x] **Failure.** A refused solve (backwards target, too far from alignment)
      shows the solver's message in the error modal and leaves Stage 2 empty,
      ready for manual placement. Cancel leaves Stage 2 unchanged.
- [x] The disabled "Auto-detect" sidebar button becomes **Auto-map** and runs
      the same flow when Stage 2 is empty (re-running over existing points is
      13.6).

### 13.3 Confidence display and review list

- [x] Auto points show their confidence band on both viewports and in the
      sidebar list: high, review, low (bands from Sprint 12: ≥ 0.75,
      0.4–0.75, < 0.4), using `MvStyle` success / warning / danger tokens.
      See open question 1 for how.
- [x] The sidebar summary shows the counts per band, and the point list puts
      low and review points first so the ones to check are at the top.
      Edited and manual points show no band.
- [x] The selected point's viewport label shows the landmark name and, for an
      auto point, its band.

### 13.4 Drag points along the surface

- [x] Press on a placed point and move the mouse: the point slides along that
      viewport's mesh under the cursor (ray-cast each frame, snapped to the
      nearest vertex on release, as placement does). A press without moving
      still just selects, as today. Works on both viewports.
- [x] One drag is one undo step ("Move Point").
- [x] Dragging off the mesh keeps the point at its last surface position.
- [x] Viewport navigation is unchanged (MMB orbit, Shift+MMB pan, scroll zoom).

### 13.5 Midline lock and mirrored drag

- [x] **Midline lock.** A midline landmark (no mirror partner) stays on the
      symmetry plane while dragged: the cursor ray picks the closest point on
      the curve where the plane cuts the mesh. Source side uses the profile's
      plane; target side uses the plane the solver found (returned with the
      result and stored with the points), or the Stage 2 symmetry setting for
      projects without one.
- [x] **Unlock.** A per-point unlock (lock icon on the selected point's sidebar
      row) frees a midline point for asymmetrical features; a sidebar setting
      unlocks or relocks all.
- [x] **Mirrored drag.** Dragging one point of a symmetric pair moves its
      partner to the reflected position, snapped to the surface. A sidebar
      setting turns this on or off (default on), and holding a modifier while
      dragging (open question 2) moves one side only.
- [x] One undo step covers both points of a mirrored drag.

### 13.6 Stretch: Re-run auto-map (replace or fill)

- [x] With points already in Stage 2, **Auto-map** asks whether to replace all
      auto points or only fill in missing landmarks. Either way, points the
      user has edited or placed by hand are never moved. One undo step.

---

## Sprint Context (Key Affected Files)

- `include/core/Project.h`, `src/io/ProjectSerializer.cpp`: point fields,
  target symmetry plane
- `include/deformation/LandmarkSolver.h`, `src/deformation/TemplateFitSolver.cpp`:
  return the target symmetry plane with the result
- `src/ui/AppWindow.cpp` / `.h`: prompt, auto-map worker and modal, apply,
  drag and mirror callbacks, undo
- `src/ui/Viewport.cpp` / `.h`: press-drag-release on points, midline-locked
  picking, confidence rendering
- `src/ui/StageSidebar.cpp` / `.h`: Auto-map button, band counts, review
  ordering, lock control, mirrored-drag setting
- `include/core/UndoActions.h`: reuse `PointPlacementUndoAction` snapshots
- `tests/smoke/ui_smoke.cpp`, `tests/smoke/v1_backward_compat.cpp`
- `.agents/context/PRD.md` (FR-3.3–FR-3.6), `docs/KNOWN_ISSUES.md` if needed

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| Keyboard shortcuts reference gains a "Point reference (Stage 2)" group: click to place, drag to move, Alt + drag for one side, Delete | Makes the new drag gestures discoverable | [x] |
| Re-running "Fill missing" when every landmark already has a point adds no empty undo step | Found while writing the cancel check | [x] |
| **Casey's hands-on review (2026-11-03)**: mirrored partners and locked midline points snapped to the nearest vertex, which was worst on the midline where no vertex may lie on the plane. Partners now go to the closest surface point to the reflection; midline points slide along the exact plane cut (one segment per crossed face). Confidence rings are now target-side only, with a "Show confidence rings" setting (saved with the project) | Smooth mirrored and midline drags; rings only where placement is uncertain | [x] |
| **Casey's second review**: locked midline drags were still hit or miss. Cause: the midline pick filtered candidates by `RaycastHit::distance`, which is in the mesh's local units, so on a target scaled in Stage 1 it rejected most or all candidates depending on the view. The pick now scores distance from the cursor ray plus a small pull toward the surface under the cursor, all in world units, with no filter | Locked midline drags follow the cursor on scaled targets; a UI check drags on a 2.5x-scaled target | [x] |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes | UI smoke: prompt appears only when it should; auto-map applies 52 paired points and undoes in one step; cancel leaves Stage 2 unchanged; drag moves a point and undoes in one step; mirrored drag moves both; locked midline point stays on the plane; unlock frees it. Serializer: new point fields round-trip; older v3 files still load | Uses the bundled head as both source and target so the run is fast and deterministic |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy`; `ctest --test-dir build -C Release --output-on-failure` (six targets) | |
| Live smoke tests | Not required | | Offline desktop app |
| Rendered image checks | Yes | Casey runs auto-map on at least two dataset heads in the app and fixes a few points by dragging | The real check for this sprint |
| Link audit | Not required | | No links change |
| Screenshots / visual audit | Yes | Screenshots of the prompt, progress modal, confidence colours and sidebar review list | New UI surfaces |
| Impeccable design pass | Not required | | Not installed for this C++ ImGui app; `MvStyle` tokens and existing components are the design system |
| Loading / empty / error states | Yes | Solver refused, cancelled, project switched during a run, no profile, profile without landmarks | Automated where possible |

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Ray-casting every frame while dragging is slow on dense targets (~900k vertices) | Med | Med | The BVH ray cast is already used for placement; measure on the facescan head and throttle to cursor movement |
| Midline lock feels sticky on meshes whose plane cut is ragged | Med | Low | Pick the closest point on the cut to the cursor ray, not a projection; unlock is always one click away |
| A project switch or New while the solver runs | Low | High | Same generation guard as the Stage 3 morph; covered by a UI smoke scenario |
| Scope creep from the review UI | Med | Med | Re-run (13.6) is the stretch; "Load preset" stays disabled |

---

## Open Questions (resolved at approval)

Casey approved the plan as written, so each question takes its
recommendation: (1) keep the pair accent fill and add a confidence ring on
auto points; (2) Alt while dragging moves one point of a pair alone; (3) the
profile points stay as they are and 13.6 stays the stretch story.


1. **How to show confidence.** Points currently use a per-pair accent colour so
   a pair can be matched across the two viewports. Recommendation: keep the
   accent fill and add a coloured ring (green / amber / red) on auto points,
   so both pieces of information survive. The alternative is to colour the
   point itself by band.
2. **One-side drag modifier.** Recommendation: hold **Alt** while dragging to
   move one point of a pair alone. Alt is unused in Stage 2 today.
3. **Sprint 12 action item 1.** Should I also move the profile's cheekbone, jaw
   middle and ear-top vertices to where you click them? It's about 1 point
   and would replace the 13.6 stretch.

---

## Definition of Done

- [x] Code reviewed (`$review-pr`, 2026-11-03)
- [x] Tests passing (six CTest targets)
- [x] Casey has auto-mapped and fixed points on real heads in the app
- [x] PRD FR-3 updated for auto-map, confidence and dragging
- [x] Required quality evidence captured in the retrospective
- [x] PO approval
- [x] Sprint outcome confirmed
- [x] Retrospective saved
- [x] Context updated

---

## Sprint Outcome

**Status**: Complete (confirmed by Casey 2026-11-03)
**Velocity (Actual)**: 15 points (13 committed plus the 13.6 stretch)
**Carryover**: —
**Validation** (2026-11-03):
- `.\build-msvc.ps1 -Deploy`: builds; no new warnings (the three C5054 dock-flag warnings are existing).
- `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed (83 s). `smoke_ui` 15/15 scenarios (22 s, was 12 s).
- New `auto_map_workflow` UI scenario (141 checks), using the bundled head as source and target:
  - Continue from Stage 1 into an empty Stage 2 shows the prompt; Auto-map applies 52 paired, named auto points as one undo step; most land in the high band; undo and redo work
  - dragging a paired point moves it to the cursor and its partner to the mirrored landmark; both lose auto status; one undo step restores both
  - Alt drag leaves the partner alone
  - a locked midline point stays within 1 unit of the symmetry plane; once unlocked it follows the cursor
  - re-running with Replace keeps the user-moved point exactly and re-solves the rest without duplicates
  - Cancel and a project switch mid-run leave points unchanged
- `smoke_v1_backward_compat`: older files load points as manual; the new point fields, the auto-map plane and the mirrored-drag setting round-trip in format v3.
- `git diff --check` clean.
- After Casey's review fixes: 6/6 passed again (84 s). The UI scenario now requires a locked midline point within 0.01 units of the plane (was 1) and the mirrored partner within 0.3 of the exact reflection; the v3 round-trip covers the ring setting.
- **Closeout (2026-11-03)**: `$review-pr` found no blocking issues. Noted: global shortcuts still work while the progress modal is open, which is safe (results apply against the current points; a project switch drops them). Final run: `.\build-msvc.ps1 -Deploy` clean; `ctest` 6/6 passed (78 s; `smoke_ui` 15/15, 19 s); `git diff --check` clean. Casey's final hands-on verdict: "buttery smooth now - exactly what we are looking for."
**Quality Evidence**: [retro-sprint-13.md](../retrospectives/retro-sprint-13.md)
**Notes**: The external docs site still needs auto-map and dragging pages (retro action item 1).
**Campaign Handoff**: N/A
