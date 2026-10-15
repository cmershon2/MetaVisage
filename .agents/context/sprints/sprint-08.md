# Sprint 8 - Standalone Open-Source Transition

**Date**: 2026-10-15
**Status**: Complete
**Campaign**: None
**Branch**: `feat/open-source-standalone`
**Base / merge target**: `development`
**Approval**: Approved by Casey on 2026-10-15 in the request to remove login,
account, payment gates, and all web integrations, followed by approval to review,
fix, commit, push, merge into development, and close out the sprint.

This record consolidates the previously authorized direct implementation task.
Implementation began as a task outside Sprint 7, which was already complete;
this plan was recorded during the requested closeout, not before implementation.
It replaces the archived roadmap's commercial distribution/auto-update Sprint 8.

## Sprint Goal

Restore MetaVisage as an offline, standalone open-source application with every
implemented tool available without authentication, accounts, payment gates,
usage limits, or runtime web integrations. Preserve the local mesh workflow.

## Sprint Backlog

No story-point estimate was made before implementation; velocity is unpointed.

| Story | Scope | Status |
|-------|-------|--------|
| 8.1 | Remove login, account UI, license state machine, token persistence, browser/deep-link auth | Complete |
| 8.2 | Remove feature gates, brush clamps, conversion limits and reporting; expose all existing local tools | Complete |
| 8.3 | Remove HTTP/JWT/crypto build dependencies, web profiles/key configuration, remote help/update actions | Complete |
| 8.4 | Validate standalone startup, stage navigation, brushes, mesh compatibility/export and wrap parity; update guidance and closeout | Complete |

## Quality Gates & Evidence

- MSVC Release build through the helper script, retaining existing toolchain fixes.
- Full four-target CTest suite: mesh round-trip/export, v1 compatibility, wrap parity, live UI.
- UI smoke uses no auth fixture and verifies every brush hotkey and viewport selection.
- CTest must load staged shaders from the selected configuration's executable directory.
- Review runtime source/manifests for residual web endpoints, gates, and dependencies.
- PowerShell parser checks, executable dependency inspection, and `git diff --check`.
- Review changed UI navigation/layout against the existing design system. No new
  imagery or external links are introduced; image generation/link-service probes
  and stored screenshots are not required for this removal task.
- Existing Welcome/empty workspace, stage prerequisites and export errors retain
  their existing paths. Full loaded-mesh sculpt/undo UI automation remains deferred.

## Definition of Done

- [x] Implement the approved removal scope and preserve local mesh functionality.
- [x] User confirmed implementation completion and requested sprint closeout.
- [x] Review findings resolved and final validation evidence recorded.
- [x] Retrospective, backlog/changelog, and durable context refreshed.
- [x] Changes reviewed and prepared for the authorized implementation/closeout commits and PR into development.

GitHub delivery is tracked by the PR; merge is verified after the final push,
without claiming it occurred before this document was committed.

## Sprint Outcome

All 4 stories complete. MSVC Release and all 4 CTest targets pass (37.39 s),
including 5/5 live UI scenarios without shader warnings. Review findings resolved.
See [retrospective](../retrospectives/retro-sprint-08.md) and
[review](../reviews/open-source-standalone.md). No planned functional scope carries over.
MinGW validation and broader loaded-mesh UI automation are outside this sprint.

**Pull request**: [#27](https://github.com/cmershon2/MetaVisage/pull/27), targeting `development`.
