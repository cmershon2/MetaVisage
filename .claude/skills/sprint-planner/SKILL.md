---
name: sprint-planner
description: Agile sprint planning with story estimation, capacity planning, and sprint goal setting. Use when planning sprints, estimating stories with story points, defining sprint goals, managing sprint backlogs, or setting sprint capacity.
user-invocable: true
allowed-tools: Read, Write, Edit, Glob
---

# Sprint Planner

You are an expert scrum master who facilitates effective sprint planning for agile teams.

## File Location

All sprint plans are saved to:

```
.claude/context/sprints/sprint-{number}.md
```

**Naming convention**: Use zero-padded two-digit numbers: `sprint-01.md`, `sprint-02.md`, etc.

When creating a new sprint, check the `.claude/context/sprints/` folder to determine the next sprint number.

## Integration with Product Backlog

Sprint stories are pulled from the product backlog:

```
.claude/context/development/product_backlog.md
```

When selecting stories for a sprint:
1. Review the product backlog for items in the target priority sections
2. Reference items by their exact **bold title** from the backlog
3. Mark selected items as `[/]` (in-progress) in the product backlog during the sprint

### Sprint Completion

When a sprint is completed:
1. Mark all completed stories as `[x]` in the product backlog
2. Use the **product-backlog** skill to move completed items to the Backlog Changelog (`.claude/context/development/backlog_changelog.md`)
3. Update the sprint document with final velocity and completion status

## When to Apply

Use this skill when:
- Planning sprint iterations
- Estimating user stories with story points
- Defining sprint goals
- Managing sprint capacity
- Prioritizing backlog items
- Identifying sprint dependencies and risks

## Sprint Planning Framework

Story Points: Use Modified Fibonacci: 1, 2, 3, 5, 8, 13, 20
Team Capacity: (Team × Days × Hours × Focus Factor 0.6-0.8)
Velocity: Average points completed in past 3-5 sprints

## Output Format

Each sprint file follows this template:

```markdown
# Sprint [Number]: [Name]

**Sprint Goal**: [Clear objective]
**Duration**: [Start Date] — [End Date]
**Capacity**: [Points]
**Committed**: [Points from backlog]

---

## Sprint Backlog

| Story | Points | Owner | Dependencies | Status |
|-------|--------|-------|--------------|--------|
| [Title from backlog] | [Pts] | [Name] | [None/Story titles] | [ ] |

---

## Sprint Context (Key Affected Files)
- [List specific core files that will need to be ingested or modified to achieve the sprint goals, e.g., `apps/backend/main.py`]

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| [Risk description] | Low/Med/High | Low/Med/High | [Plan] |

---

## Definition of Done
- [ ] Code reviewed
- [ ] Tests passing
- [ ] Deployed to staging
- [ ] PO approval

> **Mechanical guard (Sprint 4 Action Item #3 + Sprint 5 Story 5.5):** the
> `/sprint-retrospective` skill refuses to run in two cases — (Clause A) when
> this sprint's `Sprint Outcome` Status is `Complete`/`Implementation Complete`
> and any DoD item containing `"PR opened"`, `"/review-pr"`, `"manual UI smoke"`,
> or `"Phase D"` is still unchecked, *and* (Clause B) when every committed
> backlog story is shipped (`[x]`) but Status is still `Planned`/`In Progress`
> with the same validation DoD items unchecked. Don't flip Status to `Complete`
> until those boxes are genuinely ticked, and don't try to slip past Clause A
> by leaving Status at `Planned`/`In Progress` — Clause B closes that bypass.

---

## Sprint Outcome
**Status**: [Planned / In Progress / Complete]
**Velocity (Actual)**: [Points completed]
**Carryover**: [Any items not completed]
**Notes**: [Retrospective reference, key learnings]
```

---

