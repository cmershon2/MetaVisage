# Sprint 7 Retrospective

**Date**: 2026-05-25
**Sprint**: [Sprint 7 - Licensing Infrastructure: Feature Gating & Conversion Tracking](../sprints/sprint-07.md)
**Campaign**: None
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary

- **Velocity (Planned)**: 25 points
- **Velocity (Actual)**: 25 committed points completed; additional unpointed live-test polish shipped for the License Status modal/account chip and final paid-feature CTA redesign.
- **Sprint Goal Achievement**: 100% on committed scope.
- **Stories Completed**: 6 of 6.
- **Carryover Items**: None. Sprint 5 retro AI #1 Stage Stepper portion and Sprint 5/6 cottage-cheese spike are both closed.
- **Tangential Work**: Codex runtime migration, License Status modal redesign/title-bar account chip, final paid-feature CTA redesign, brush-preview restoration, and PR review fixes for conversion queue durability/JWT restrictive fallback.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | `tests/smoke/ui_smoke.cpp` | Added `stage_stepper_progression` and `touchup_brush_gate_clamp` scenarios. |
| Tests run | Yes | Pass | `.\build-msvc.ps1 -Profile development -Deploy`; `ctest -C Release --output-on-failure`; `git diff --check`. |
| Live smoke tests | Yes | Pass with caveat | Development profile live review covered paid-feature CTA rest/hover states and brush-preview restoration. Staging endpoints remain unavailable. |
| Rendered image checks | Yes | Pass | Brush preview PNGs remain in `assets/brushes/` and render through Stage 4 sidebar/tool rail paths. |
| Link audit | Yes | Pass by code review | Upgrade/manage links route to pricing/manage-subscription paths via `OpenUrl`; no browser click audit recorded. |
| Screenshots / visual audit | Yes | Pass | UI/UX comps drove the final CTA redesign; user live-reviewed the result. No stored screenshot artifact. |
| Loading / empty / error states | Yes | Pass | Login smoke covers unauthenticated/authenticating cycle; conversion-limit dialog and offline queue paths were code-reviewed and covered by state checks. |
| Known issues | Yes | Non-blocking | Staging web endpoints still pending; full deep-link exchange mock remains future work; known build warnings remain C4996 getenv and C5054 enum flag combines. |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 4 | Full CTest green, plus two new UI scenarios. Live network endpoint testing remains development-profile only. |
| Visual consistency | 5 | Final paid-feature CTA matches UI/UX comps and preserves brush previews. |
| Loading behavior | 4 | Login cycle and cached/offline license surfaces are covered; deep-link exchange mock still absent. |
| Navigation completeness | 4 | Pricing/manage links are wired; no automated external browser audit. |
| Error handling | 5 | Conversion limit, retry, offline queue, queue cap, durable queue, and malformed JWT fallback paths are handled. |

---

## Retrospective

### LIKED (What we liked)

- The feature-gating work landed as a full cross-surface slice: JWT parsing, typed getters, Stage 1/3/4 gates, ToolRail enforcement, viewport brush clamping, conversion warnings, and License Status usage all moved together.
- The late UI/UX feedback loop was productive. The final paid-feature CTA moved away from large amber blocks into compact locked rows and hover cards, and the brush previews were restored quickly after review.
- PR review caught two real shipment risks: conversion queue durability and stale paid entitlements after malformed claim parsing. Both were fixed before merge.
- The Stage Stepper and brush-gate smoke tests closed old retrospective carryover without needing brittle full-worker UI automation.

### LEARNED (What we learned)

- Offline conversion reports should stay durable until server confirmation. Clearing the queue before a worker result drains creates a quiet loss window and makes effective usage lie while work is in flight.
- Feature flags need stricter defaulting than passive display fields. It is acceptable to keep old tier/usage text visible for diagnostics, but authorization flags must reset restrictive before parsing.
- `smoke_wrap_parity` is a valid full-suite gate but no longer fits a 60-second budget on this machine. The test passed reliably once the CTest timeout matched observed runtime.
- UI gate affordances work best when they keep the shape of the underlying tool visible. Locked brush tiles still need brush imagery so users understand what the upgrade unlocks.

### LACKED (What we lacked)

- A hermetic license JWT fixture that exercises all feature-flag parsing from a signed token would have caught claim-shape regressions earlier than live review.
- We still do not have a mock portal/deep-link exchange lane, so auth network behavior is verified by code review and manual/dev-profile testing rather than automated CI.
- The conversion endpoint contract lacks an idempotency key. A crash after a successful POST but before local drain can still duplicate a queued conversion on next launch; normal shutdown now drains, but server-side idempotency would close the hard-crash edge.

### LONGED FOR (What we longed for)

- A small release-readiness checklist that includes screenshots or captured frames for every paid-feature gate would make late CTA feedback less memory-dependent.
- A web-team staging environment for `/api/desktop/token` and `/api/license/conversion` would turn the current development-profile validation into true release evidence.
- A future UI smoke fixture with a minimal loaded mesh would let us add the deferred Stage 3 process/Accept and Stage 4 sculpt/undo scenarios without paying the full worker cost every run.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Add a Sprint 8 backlog/planning item for conversion idempotency or account-scoped queue ownership with the web team. | Casey | Sprint 8 planning | [ ] |
| 2 | Create a compact paid-feature visual QA checklist with saved screenshots or captured frames for Free / Indie / Pro gates. | Casey | Before next UI-heavy PR | [ ] |
| 3 | Keep staging endpoint deployment visible as a release-readiness blocker until `/api/desktop/token` and `/api/license/conversion` are live. | Casey | Sprint 8 readiness | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 6) | Status |
|---|---------------------------|--------|
| 1 | Phase D pre-merge gate before PR #25 lands on `development`. | Done / superseded by Sprint 7 evidence. Development-profile live checks and full CTest are recorded; staging remains separately blocked by endpoint availability. |
| 2 | Cottage-cheese-face root-cause spike in Sprint 7. | Done. Closed with "Outcome B stopgap is the answer" in `cottage_cheese_face_decision.md`; no code change to deformation algorithm. |
| 3 | License Status modal redesign + title-bar account chip in Sprint 7 or 8. | Done. Pulled into Sprint 7 during live-test feedback. |

---

## Key Metrics

- **Velocity Trend**: Stable on committed work: Sprint 5 22/22, Sprint 6 26/26, Sprint 7 25/25.
- **Team Morale**: High. The review/fix loop produced better UX and caught real correctness issues before merge.
- **Process Health**: Good. The sprint closed two older retro carryovers, added targeted smoke coverage, and used PR review as a genuine quality gate.
- **Campaign Loop Readiness**: Ready for next sprint.
- **Readiness Rationale**: Required code/test evidence is green, known gaps are non-blocking and tracked as action items, and no committed Sprint 7 scope is carried over.
