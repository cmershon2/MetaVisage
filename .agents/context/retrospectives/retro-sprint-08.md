# Sprint 8 Retrospective

**Date**: 2026-10-15
**Sprint**: [Sprint 8 - Standalone Open-Source Transition](../sprints/sprint-08.md)
**Campaign**: None
**Format**: Start-Stop-Continue
**Completion confirmation**: Casey accepted the implementation and requested
review, fixes, commits/pushes, merge into development, and sprint closeout.

## Sprint Summary

- **Velocity (Planned)**: Unpointed; approved initially as a direct implementation task.
- **Velocity (Actual)**: Unpointed; no retrospective estimates invented.
- **Sprint Goal Achievement**: 100% of the approved functional scope.
- **Stories Completed**: 4 of 4.
- **Carryover Items**: None from committed scope.
- **Tangential Work**: Fix the UI CTest working directory and Help separator;
  reconcile completed backlog entries and retire superseded commercial follow-ups.

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added/updated | Yes | Pass | Replace login/tier fixtures with standalone startup and all-brush hotkey/viewport checks; five UI scenarios total. |
| Tests run | Yes | Pass | MSVC Release helper build; `ctest --test-dir build -C Release --output-on-failure`: 4/4, 37.39 s; `git diff --check`. |
| Live smoke tests | Yes | Pass | Real GLFW/ImGui harness: 5/5 scenarios, no auth fixture or shader warnings. Network probes are unnecessary because integrations are removed. |
| Rendered image checks | No | N/A | No new images; existing brush previews/assets preserved. No image-capture claim. |
| Link audit | Yes | Pass | Runtime endpoints/browser actions removed; source/manifests scanned for residual integrations. |
| Screenshots / visual audit | Yes | Pass with limitation | Changed Help/menu/status/brush paths reviewed in code and live UI harness; no stored screenshot or full visual signoff claimed. |
| Loading / empty / error states | Yes | Pass within scope | Welcome/empty workspace, all stage transitions and mesh compatibility/export smoke pass; local stage prerequisites retained. Full worker/sculpt UI paths deferred. |
| Known issues | Yes | Non-blocking | MinGW unvalidated; existing MSVC deprecation warnings; loaded-mesh UI and clean-machine package tests tracked in backlog. |
| Dependencies and scripts | Yes | Pass | Removed network/JWT/crypto dependencies; executable import inspection; both PowerShell build helpers parse. |
| PR review | Yes | Approve after fixes | See `../reviews/open-source-standalone.md`; no material unresolved findings. |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 4 | Four smoke targets plus standalone brush regression; loaded-mesh UI gap tracked. |
| Visual consistency | 4 | Existing MvStyle and preview treatment retained; removal inspected in source/live harness. |
| Loading behavior | 4 | Workspace mounts without login; shader directory fixed; no service wait path. |
| Navigation completeness | 5 | Four-stage workflow remains; removed account/remote actions have no dangling callbacks. |
| Error handling | 4 | Local mesh/compatibility/export tests pass; deleted service failures are no longer relevant. |

## Retrospective

### START

- Keep CTest working directories configuration-aware so standard tests load the same assets as the application.
- Validate distributed standalone packages on a clean Windows environment before publishing a release.

### STOP

- Carrying licensing, payment, and staging endpoint work into the active backlog after that product direction is retired.
- Treating a green UI assertion run with shader-load warnings as complete runtime asset evidence.

### CONTINUE

- Preserve topology and local workflow behavior through the full mesh smoke suite.
- Remove obsolete subsystems and dependencies together, then review the resulting executable imports.

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Add a small loaded-mesh fixture for Process/Accept and sculpt/undo UI automation. | Casey / next implementation owner | Next UI test expansion | Open; backlog item recorded |
| 2 | Create and execute the standalone clean-machine release package smoke checklist. | Casey / release owner | Before the next published release | Open; backlog item recorded |

## Previous Action Items Review

| # | Action from Sprint 7 | Status |
|---|----------------------|--------|
| 1 | Conversion idempotency/account-scoped queue follow-up | Retired: reporting and the queue are removed. |
| 2 | Paid-feature visual QA checklist | Retired: all paid gates are removed. |
| 3 | Staging auth/conversion endpoint deployment | Retired: no runtime service integration remains. |

## Key Metrics

- **Velocity Trend**: Not comparable; prior sprints used points, this transition was unpointed.
- **Team Morale**: Not assessed.
- **Process Health**: Completed task scope is documented truthfully at closeout; the plan was consolidated after implementation rather than backdated.
- **Campaign Loop Readiness**: Ready for next sprint.
- **Readiness Rationale**: Required build/test/review gates pass. Deferred validation is explicitly tracked; no campaign or next sprint is started by this closeout.
