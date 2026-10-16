# Campaign: [Name]

**Campaign Goal**: [One sentence describing the multi-sprint outcome]
**Source**: [PRD / roadmap / sprint series path]
**Status**: Planned / In Progress / Complete / Blocked
**Readiness**: Pending / Ready / Ready with gaps / Blocked
**Readiness Check**: readiness-{campaign-slug}.md
**Approval**: Pending
**Active Sprint**: [sprint-0N.md or "Not started"]
**Next Action**: [Run readiness / Plan next sprint / Execute active sprint / Verify / Retro / Update context / Commit / Write handoff / Reassess]

---

## Mandate

[Summarize the approved PRD, roadmap, or sprint series. This is the boundary for autonomous execution.]

---

## Readiness Summary

- [Readiness decision, critical gaps, optional gaps, and workarounds]

---

## Campaign Quality Gates

| Gate | Required? | Source / Strategy | Advancement Rule |
|------|-----------|-------------------|------------------|
| Design system / style guide | Yes / No | [tokens, primitives, references, or rationale] | [required before UI sprint / deferred / N/A] |
| Impeccable design pass | Yes / No | [$impeccable teach/document/critique/audit/polish, or "not installed"] | [required before UI sprint / deferred / N/A] |
| Navigation map | Yes / No | [route map or rationale] | [required before route sprint / deferred / N/A] |
| Loading / empty / error-state guidance | Yes / No | [spec path or rationale] | [required before async UI sprint / deferred / N/A] |
| Live smoke tests | Yes / No | [services and cadence] | [hard stop on missing/red / deferred / N/A] |
| Screenshot / visual audit strategy | Yes / No | [tool and scope] | [hard stop on missing/red / deferred / N/A] |
| Rendered image checks | Yes / No | [domains/views] | [hard stop on missing/red / deferred / N/A] |
| Broken-link audit | Yes / No | [crawl scope] | [hard stop on missing/red / deferred / N/A] |

---

## Sprint Sequence

| # | Sprint | Goal | Status | Notes |
|---|--------|------|--------|-------|
| 1 | [sprint-01.md] | [Goal] | Planned | [Notes] |

---

## Autonomy Boundaries

- Codex may execute multiple sprint loops after campaign approval.
- Codex may perform tangential work inline when it improves the sprint or campaign outcome.
- Codex should stop only for destructive git/history changes, secrets or credentials, billing or paid services, production-impacting actions, legal/security-sensitive decisions, or requirements that cannot be reasonably inferred.
- Codex must stop when required sprint retrospective quality evidence is
  missing/red, or when Campaign Loop Readiness is `Needs reassessment` or
  `Blocked`.
- Campaign-end checkpoints are hard stops for human review.

---

## Progress Log

| Date | Sprint | Summary | Commit | Handoff |
|------|--------|---------|--------|---------|
| [YYYY-MM-DD] | [sprint-0N.md] | [Summary] | [hash] | [handoff path] |

---

## Tangential Work Log

| Date | Sprint | Work | Why It Helped |
|------|--------|------|---------------|
| [YYYY-MM-DD] | [sprint-0N.md] | [Work performed] | [Campaign/sprint benefit] |

---

## Risks, Assumptions, And Open Decisions

### Risks

- [Risk and mitigation]

### Assumptions

- [Assumption]

### Open Decisions

- [Decision needed, or "None"]
