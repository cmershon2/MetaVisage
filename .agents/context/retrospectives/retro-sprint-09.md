# Sprint 9 Retrospective

**Date**: 2026-10-30
**Sprint**: [Sprint 9: Alignment Stage Usability Fixes](../sprints/sprint-09.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 10 points
- **Velocity (Actual)**: 10 points
- **Sprint Goal Achievement**: 100%
- **Stories Completed**: 5 of 5
- **Carryover Items**: None. Hand-testing the Numpad 1/3/7 views moves to Sprint 10.3 (View menu), because the owner's keyboard has no numpad.
- **Tangential Work**: Ctrl+Z defers to active text fields; transform group IDs scoped per group; replaced target renderer released; viewport hint G/R/S; Numpad 7 top-view fix (camera up derived from yaw/pitch, numpad views keep target and distance); `HasMeshTexture` query; UI harness `RunOnMainThread` for GL work.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | `tests/smoke/ui_smoke.cpp`: 5 new scenarios (`alignment_transform_hotkeys`, `alignment_gizmo_tracks_cursor`, `alignment_gizmo_all_views`, `alignment_texture_survives_target_swap`, `alignment_typed_transform_entry`) | Texture test confirmed failing with the 9.4 fix reverted |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK; `ctest --test-dir build -C Release --output-on-failure` 4/4 passed (40.6 s); `smoke_ui` 11/11 | `git diff --check` clean |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Pass: Casey confirmed textures survive target swap | Save/reopen uses the same refresh helper |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | Yes | Stage 1 rail screenshot supplied by Casey (three tools, G/R/S badges, no stubs); typed entry confirmed by hand | Screenshot shared in session, not committed |
| Loading / empty / error states | Yes | Esc cancels typed entry with no undo; texture refresh is a no-op without a loaded target or albedo | Covered by smoke test and code path |
| Known issues | Yes | Numpad views not hand-tested (no numpad; automated only); Ctrl+Y redo unbound; renderer cache not released on project switch | Last two are in the backlog |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 4 | Every story has a UI smoke scenario; gizmo checked in four camera setups |
| Visual consistency | 4 | Rail screenshot; typing mode uses MvStyle tokens |
| Loading behavior | N/A | No new async work |
| Navigation completeness | 4 | Full Stage 1 -> 4 -> Export run by Casey |
| Error handling | 4 | Typed-entry cancel and focus-loss paths; failed target import unchanged |

---

## Retrospective

### START (Start doing)
- Turning manual checks into smoke scenarios when they can be stated as numbers. The all-views gizmo test found the broken Numpad 7 top view, which predated this sprint.
- Giving every keyboard-only feature a menu or toolbar path, so it can be tested on keyboards without a numpad.

### STOP (Stop doing)
- Calling GL from test bodies. They run on the engine's coroutine thread, and uploads there fail silently. Use `RunOnMainThread`.

### CONTINUE (Keep doing)
- Recording the root cause with a file reference before a story is approved. All five fixes went where the plan said they would.
- Checking that a new regression test fails when its fix is reverted.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Hand-check the orthographic views through the new View menu items in Sprint 10.3 | Casey / Sprint 10 implementer | Sprint 10 | [ ] Recorded in sprint-10.md |
| 2 | Open a PR to `development` for Sprint 9 and run `$review-pr` before Sprint 10 starts | Casey / implementer | Before Sprint 10 | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 8) | Status |
|---|---------------------------|--------|
| 1 | Loaded-mesh fixture for Process/Accept and sculpt/undo UI automation | 🔄 Ongoing: Sprint 9 added in-memory and temp-OBJ target fixtures; loaded-asset Process/Accept still open in the backlog |
| 2 | Clean-machine release package smoke checklist | ❌ Not started; still in the backlog |

---

## Key Metrics
- **Velocity Trend**: Stable (first pointed sprint; 10/10)
- **Team Morale**: High
- **Process Health**: Plan, approval, implementation, tests and owner verification all done in order. PR review still to do.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. The only gap, hand-testing the numpad views, has automated coverage and is assigned to Sprint 10.3.
