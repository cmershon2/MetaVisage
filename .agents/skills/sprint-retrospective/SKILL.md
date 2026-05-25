---
name: sprint-retrospective
description: Facilitate effective sprint retrospectives for continuous team improvement. Use when conducting team retrospectives, identifying improvements, or reviewing sprint outcomes. Supports Start-Stop-Continue, Mad-Sad-Glad, and 4Ls formats.
---

# Sprint Retrospective

## File Location

All retrospective documents are saved to:

```
.agents/context/retrospectives/retro-sprint-{number}.md
```

**Naming convention**: Match the sprint number from `.agents/context/sprints/`. Example: if closing Sprint 3, the file is `retro-sprint-03.md`.

When creating a new retrospective, check the corresponding sprint file in `.agents/context/sprints/` for context on what was accomplished.

## Integration with Sprint Planner

The retrospective references the sprint plan and its outcomes:

```
Sprint Plan:     .agents/context/sprints/sprint-{number}.md
Retrospective:   .agents/context/retrospectives/retro-sprint-{number}.md
Product Backlog: .agents/context/development/product_backlog.md
Changelog:       .agents/context/development/backlog_changelog.md
Campaign:        .agents/context/campaigns/campaign-{slug}.md, when applicable
```

Before starting a retrospective:
1. Review the sprint plan for committed stories, velocity, and outcome
2. Check which items were carried over vs. completed
3. Review the sprint plan's declared quality gates and validation requirements
4. Reference any action items from the previous retrospective
5. For Campaign Autopilot, review tangential work and whether the campaign is
   ready for the next sprint loop.

## When to Use This Skill

- **End of sprint**: at the end of each sprint
- **Project milestone**: after major releases
- **Team issues**: when an immediate retrospective is needed

## Retrospective Formats

### Format 1: Start-Stop-Continue

```markdown
### START (Start doing)
- [Things the team should begin doing]

### STOP (Stop doing)
- [Things the team should stop doing]

### CONTINUE (Keep doing)
- [Things that are working well]
```

### Format 2: Mad-Sad-Glad

```markdown
### MAD (What frustrated us)
- [Pain points and frustrations]

### SAD (What we wished went better)
- [Missed opportunities or shortcomings]

### GLAD (What made us happy)
- [Wins and positive moments]
```

### Format 3: 4Ls (Liked-Learned-Lacked-Longed For)

```markdown
### LIKED (What we liked)
- [Positive experiences]

### LEARNED (What we learned)
- [New knowledge gained]

### LACKED (What we lacked)
- [Missing tools, resources, or skills]

### LONGED FOR (What we longed for)
- [Improvements we wish we had]
```

## Output Format

Each retrospective file follows this template:

```markdown
# Sprint [N] Retrospective

**Date**: [YYYY-MM-DD]
**Sprint**: [Sprint number and name, linked to sprint file]
**Campaign**: [campaign-{slug}.md or "None"]
**Format**: [Start-Stop-Continue / Mad-Sad-Glad / 4Ls]

---

## Sprint Summary
- **Velocity (Planned)**: [points]
- **Velocity (Actual)**: [points]
- **Sprint Goal Achievement**: [percentage]
- **Stories Completed**: [X of Y]
- **Carryover Items**: [list or "None"]
- **Tangential Work**: [summary or "None"]

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes / No | [paths and counts, or "None"] | [notes] |
| Tests run | Yes | [commands and pass/fail] | [notes] |
| Live smoke tests | Yes / No | [endpoints/services and result] | [notes] |
| Rendered image checks | Yes / No | [result] | [notes] |
| Link audit | Yes / No | [result] | [notes] |
| Screenshots / visual audit | Yes / No | [paths or result] | [notes] |
| Loading / empty / error states | Yes / No | [result] | [notes] |
| Known issues | Yes | [none, or list] | [impact/carryover] |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | [score] | [evidence] |
| Visual consistency | [score or N/A] | [evidence] |
| Loading behavior | [score or N/A] | [evidence] |
| Navigation completeness | [score or N/A] | [evidence] |
| Error handling | [score or N/A] | [evidence] |

`Campaign Loop Readiness` must be based on this evidence. If required evidence
is missing or red, readiness is `Blocked` or `Needs reassessment`.

---

## Retrospective

[Selected format sections go here]

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | [Specific, actionable item] | [Name] | [Date] | [ ] |
| 2 | [Specific, actionable item] | [Name] | [Date] | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint [N-1]) | Status |
|---|---------------------------|--------|
| 1 | [Previous action item] | ✅ Done / 🔄 Ongoing / ❌ Not started |

---

## Key Metrics
- **Velocity Trend**: [Increasing / Stable / Decreasing]
- **Team Morale**: [High / Medium / Low]
- **Process Health**: [notes]
- **Campaign Loop Readiness**: [Ready for next sprint / Needs reassessment / Blocked]
- **Readiness Rationale**: [why the evidence supports this status]
```

## Constraints

### Required Rules (MUST)

1. **Safe Space**: a blame-free environment
2. **Action Items**: must be specific and actionable (limit 2-3)
3. **Follow-up**: check progress of previous action items in the next retrospective
4. **Quality Evidence**: required gates must be recorded before a campaign can
   advance

### Prohibited (MUST NOT)

1. **Personal attacks**: improve the process, not the person
2. **Too many actions**: limit to 2-3 focused improvements

## Best Practices

1. **Time-box**: within 1 hour
2. **Rotate Facilitator**: team members take turns facilitating
3. **Celebrate Wins**: acknowledge successes, not just problems
4. **Track Trends**: compare velocity and morale across sprints

## End of Sprint Context Update

The sprint retrospective is the final evidence checkpoint before context update.
It closes the sprint only when the Sprint Quality Evidence section is complete.
Once the retrospective document has been generated and saved, use the
`update-context` skill to synchronize `AGENTS.md`,
`.agents/context/product-vision.md`, and `README.md` with the new project state.

After context is updated, use the `commit` skill to create the final focused sprint commit. Do not commit sprint work before the retrospective and context update are complete unless the user explicitly asks for an interim checkpoint commit.

For Campaign Autopilot, the sprint retrospective is followed by context update,
one focused sprint commit, campaign handoff creation, reassessment, and the next
sprint loop.

## References

- [Retrospective Formats](https://retromat.org/)
- [Agile Retrospectives](https://www.amazon.com/Agile-Retrospectives-Making-Teams-Great/dp/0977616649)
