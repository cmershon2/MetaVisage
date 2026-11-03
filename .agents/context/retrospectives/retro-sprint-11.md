# Sprint 11 Retrospective

**Date**: 2026-10-30
**Sprint**: [Sprint 11: Source Profiles](../sprints/sprint-11.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 13 points
- **Velocity (Actual)**: 13 points
- **Sprint Goal Achievement**: 100%
- **Stories Completed**: 5 of 5
- **Carryover Items**: None
- **Tangential Work**:
  - `ProjectSerializer::Load` now restores the vertex mask and the optimization, rigidity and sampling parameters. They were saved but never read back.
  - Changing the source mesh is one undo step and clears the stale `originalMorphMesh`, so Process no longer wraps the previous source after an import.
  - Help > Show Profile Landmarks (Dev) shows a profile's landmarks as named, source-only Stage 2 points, for reviewing authored placement.
  - Undo and redo now refresh the mask overlay. Before, the overlay's colors were only sent to the GPU while painting, so undoing a mask stroke or a source change kept showing the replaced mask.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | New `tests/smoke/source_profile.cpp` (`smoke_source_profile`): fingerprint, mask ranges, validation, canonical set, ProfileStore, draft export, built-in profile (including a check that every landmark is canonical), FBX round-trip. `v1_backward_compat.cpp`: v2 fixture and v3 round-trip. `ui_smoke.cpp`: `source_profile_picker`, including the landmark viewer | New fixture `tests/fixtures/v2_project.mmproj` |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK. `ctest --test-dir build -C Release --output-on-failure`: 5/5 passed. `smoke_ui`: 14/14 | `git diff --check` clean. The three C5054 warnings are in existing dock-flag code. `.\build-mingw.ps1` not run (shares `build\`; see backlog) |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Pass. Casey reviewed all 52 landmarks with Show Profile Landmarks (Dev) and the default mask in Stage 3 | Checked by script: midline points within 0.19 units of center, pairs mirror within 0.33, mask is one symmetric region (649 vertices) |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | Yes | Casey ran Stages 1–4 with the built-in profile on 2026-10-30 and approved the result | Screenshots were not captured or committed |
| Loading / empty / error states | Yes | Automated: malformed, unknown-schema and mis-filed profiles skipped; fingerprint mismatch refused with an error and no change; landmark viewer refused outside Stage 2. Manual: none extra | Save and reopen keeping the profile id is covered by the automated v3 round-trip, not a manual pass |
| Known issues | Yes | Meshes still load on the UI thread (backlog). FBX files exported by Maya or Unreal are unmeasured against the fingerprint. Landmark names on Stage 2 points are not saved (Sprint 13 adds landmark ids to the format) | None block Sprint 12 |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 5 | Every story has automated coverage; the built-in profile is checked against the bundled mesh and the canonical list on every run |
| Visual consistency | 4 | The picker uses `MvStyle` tokens and existing combo styling; no screenshot review of the picker itself |
| Loading behavior | 3 | Profile mesh loading is synchronous on the UI thread, like the import it replaced; tracked in the backlog |
| Navigation completeness | 5 | Casey ran Stage 1 → 4 with the built-in profile |
| Error handling | 5 | Fingerprint mismatch, missing mesh, malformed files and unknown preset ids are all handled without changing project state |

---

## Retrospective

### START (Start doing)
- Checking authored data by script as soon as it arrives. Measuring midline offsets, mirror error and mask symmetry turned "there is no way to get them exactly on it" into a concrete answer (the mesh has no vertex exactly on x = 0), and caught the jaw angle / jaw middle swap.
- Building the review tool together with the authoring tool. The landmark viewer came after the data was authored; with it ready from the start, Casey could have checked placement in one pass.

### STOP (Stop doing)
- Writing authoring steps that depend on one uninterrupted session. The dev export only names points when it sees exactly 14 midline points plus 19 pairs, and the steps didn't say so prominently, so the two separate exports needed hand-merging.

### CONTINUE (Keep doing)
- Fixing adjacent bugs found while building the feature (the mask not reloading, the stale `originalMorphMesh`, the mask overlay after undo), each with a test and a tangential log entry.
- Measuring risks instead of guessing. The FBX fingerprint risk was retired by a test that re-measures it every run.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Confirm design decisions 1 (geometric solver first) and 3 (auto-map refines alignment internally only) in `auto-point-reference.md` | Casey | Before Sprint 12 planning | [ ] |
| 2 | Export the MetaHuman head from Unreal (or Maya) as FBX and measure its fingerprint against the built-in profile, so we know whether such imports match the profile automatically | Casey / implementer | Before Sprint 13 | [ ] |
| 3 | Give `build-mingw.ps1` its own build directory (carried from Sprint 10) | Next sprint implementer | Next sprint | [ ] Backlog |

---

## Previous Action Items Review

| # | Action (from Sprint 10) | Status |
|---|---------------------------|--------|
| 1 | Give `build-mingw.ps1` its own build directory | ❌ Not started (still in the backlog; carried as action 3) |
| 2 | Merge cmershon2/MetaVisage#29, then open the Sprint 10 PR | ✅ Done (#29 and #30 both merged 2026-10-30) |

---

## Key Metrics
- **Velocity Trend**: Stable (12 -> 13 points, both fully delivered)
- **Team Morale**: High
- **Process Health**: All steps done in order: design, plan, approval, branch, implementation, owner authoring and review, code review, retro.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. Sprint 12 planning needs action item 1 (Casey's design decisions) first.
