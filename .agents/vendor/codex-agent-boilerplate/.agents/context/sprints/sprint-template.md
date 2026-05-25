# Sprint [N]: [Name]

**Sprint Goal**: [Clear, single-sentence objective]
**Duration**: [Start Date] — [End Date]
**Capacity**: [Total story points available]
**Committed**: [Story points pulled from backlog]
**Execution Mode**: Classic / Campaign Autopilot
**Campaign**: [campaign-{slug}.md or "None"]
**Approval**: Pending

---

## Workflow Loop

- [ ] Sprint plan created in `.agents/context/sprints/`
- [ ] Sprint plan approved before execution
- [ ] Approved scope executed
- [ ] Tests run and validation recorded
- [ ] Sprint completion confirmed
- [ ] Retrospective saved in `.agents/context/retrospectives/`
- [ ] Context updated with `$update-context`
- [ ] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **[Story title from backlog]** | [pts] | None | [ ] |

---

## Sprint Context (Key Affected Files)

- [List specific files that will need to be ingested or modified to achieve the sprint goals]

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| [Inline support work, or "None"] | [Sprint/campaign benefit] | [ ] |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes / No | [paths/counts or "None"] | [why] |
| Tests run | Yes | [commands and pass/fail] | [why] |
| Live smoke tests | Yes / No | [services/endpoints and result] | [why] |
| Rendered image checks | Yes / No | [expected domains/dimensions/load result] | [why] |
| Link audit | Yes / No | [crawl scope and result] | [why] |
| Screenshots / visual audit | Yes / No | [paths and summary] | [why] |
| Impeccable design pass | Yes / No | [$impeccable critique/audit/polish result, or "Not installed"] | [why] |
| Loading / empty / error states | Yes / No | [scenarios and result] | [why] |

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| [Risk description] | Low/Med/High | Low/Med/High | [Plan] |

---

## Definition of Done

- [ ] Code reviewed
- [ ] Tests passing
- [ ] Required quality evidence captured in the retrospective
- [ ] Deployed to staging
- [ ] PO approval
- [ ] Sprint outcome confirmed
- [ ] Retrospective saved
- [ ] Context updated

---

## Sprint Outcome

**Status**: Planned / In Progress / Complete
**Velocity (Actual)**: [Points completed]
**Carryover**: [Items not completed, or "None"]
**Validation**: [Commands run and results]
**Quality Evidence**: [Retrospective section reference and readiness status]
**Notes**: [Key learnings, retrospective reference]
**Campaign Handoff**: [handoff path, or "N/A"]
