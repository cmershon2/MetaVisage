---
name: sprint-planner
description: Agile sprint planning with story estimation, capacity planning, and sprint goal setting. Use when planning sprints, estimating stories with story points, defining sprint goals, managing sprint backlogs, or setting sprint capacity.
---
# Sprint Planner

You are an expert scrum master who facilitates effective sprint planning for agile teams.

## File Location

All sprint plans are saved to:

```
.agents/context/sprints/sprint-{number}.md
```

**Naming convention**: Use zero-padded two-digit numbers: `sprint-01.md`, `sprint-02.md`, etc.

When creating a new sprint, check the `.agents/context/sprints/` folder to determine the next sprint number.

## Required Sprint Workflow

Sprint work is governed by `.agents/rules/sprint-workflow.md`.
Multi-sprint Campaign Autopilot work is also governed by
`.agents/rules/campaign-workflow.md`.

Required loop:

1. Create the sprint plan in `.agents/context/sprints/sprint-{number}.md`.
2. Get explicit approval before implementation begins.
3. Execute only the approved sprint scope.
4. Test the completed work and record the commands run.
5. Confirm the sprint is done with the user or product owner.
6. Run the retrospective in `.agents/context/retrospectives/`.
7. Use `update-context` to refresh project context.
8. Use `commit` to create the final focused sprint commit.

Do not begin implementation until approval is recorded in the sprint plan, or
until the sprint is part of an approved Campaign Autopilot mandate.

## Integration with Product Backlog

Sprint stories are pulled from the product backlog:

```
.agents/context/development/product_backlog.md
```

When selecting stories for a sprint:
1. Review the product backlog for items in the target priority sections
2. Reference items by their exact **bold title** from the backlog
3. Mark selected items as `[/]` (in-progress) in the product backlog during the sprint
4. For Campaign Autopilot, use the latest campaign handoff and retrospective
   action items when choosing or reordering the next sprint.

## Quality Gates

Every sprint plan must declare the quality evidence needed to close the sprint.
Choose gates based on the work, not habit. Examples:

- Unit, integration, E2E, privacy, accessibility, performance, or compliance
  tests.
- Fixture contract tests for normalized boundaries.
- Live smoke tests for external APIs or services whose runtime behavior matters.
- Rendered image checks for image-heavy pages.
- Broken-link audits for route, nav, or public-page changes.
- Screenshots or visual audits for user-facing UI work.
- Impeccable critique, audit, or polish for frontend work when `impeccable` is
  installed or enabled separately.
- Loading, empty, and error-state checks for async UI.

If a gate is not applicable, mark it `Not required` and explain why. If a gate
is required but cannot be run in the sprint, mark it as a risk or carryover and
do not treat Campaign Loop Readiness as ready without an explicit workaround.

### Sprint Completion

When a sprint is completed:
1. Mark all completed stories as `[x]` in the product backlog
2. Use the **product-backlog** skill to move completed items to the Backlog Changelog (`.agents/context/development/backlog_changelog.md`)
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
**Execution Mode**: Classic / Campaign Autopilot
**Campaign**: [campaign-{slug}.md or "None"]
**Approval**: Pending

---

## Sprint Backlog

| Story | Points | Owner | Dependencies | Status |
|-------|--------|-------|--------------|--------|
| [Title from backlog] | [Pts] | [Name] | [None/Story titles] | [ ] |

---

## Sprint Context (Key Affected Files)
- [List specific core files that will need to be ingested or modified to achieve the sprint goals, e.g., `apps/backend/main.py`]

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
- [ ] Sprint Quality Evidence recorded in the retrospective
- [ ] Deployed to staging
- [ ] PO approval
- [ ] Sprint outcome confirmed
- [ ] Retrospective saved
- [ ] Context updated

---

## Sprint Outcome
**Status**: [Planned / In Progress / Complete]
**Velocity (Actual)**: [Points completed]
**Carryover**: [Any items not completed]
**Validation**: [Commands run and results]
**Quality Evidence**: [Retrospective section reference and readiness status]
**Notes**: [Retrospective reference, key learnings]
**Campaign Handoff**: [handoff path, or "N/A"]
```

---
