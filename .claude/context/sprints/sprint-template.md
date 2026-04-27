# Sprint [N]: [Name]

**Sprint Goal**: [Clear, single-sentence objective]
**Duration**: [Start Date] — [End Date]
**Capacity**: [Total story points available]
**Committed**: [Story points pulled from backlog]

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **[Story title from backlog]** | [pts] | None | [ ] |

---

## Sprint Context (Key Affected Files)

- [List specific files that will need to be ingested or modified to achieve the sprint goals]

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| [Risk description] | Low/Med/High | Low/Med/High | [Plan] |

---

## Definition of Done

- [ ] All committed stories implemented and merged to the sprint branch
- [ ] Build clean: `cmake --build . --config Release` succeeds with no new warnings
- [ ] CTest green: `ctest -C Release` passes (smoke test + any new test targets)
- [ ] PR opened against `development` and `/review-pr` agent run; **all blocking findings resolved**
- [ ] Phase D manual UI smoke completed on Windows — at minimum: Stage 1 → 2 → 3 → 4 → Export end-to-end with the bundled MetaHuman head, plus Undo/Redo verification on point placement, gizmo drag, mask paint, and sculpt brush
- [ ] Sprint outcome notes updated with actual velocity, carryover (or "None"), and a retrospective link

> **Process note (Sprint 3.7 retro action item #2):** Do NOT mark the Sprint Outcome
> as "Complete" or "Implementation Complete" until **both** the `/review-pr`
> blockers are closed **and** the manual UI smoke has run. The Sprint 3.6 and
> 3.7 retros both flagged this anti-pattern; the DoD checkbox above is the
> guard.

---

## Sprint Outcome

**Status**: Planned / In Progress / Complete
**Velocity (Actual)**: [Points completed]
**Carryover**: [Items not completed, or "None"]
**Notes**: [Key learnings, retrospective reference]
