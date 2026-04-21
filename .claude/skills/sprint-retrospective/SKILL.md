---
name: sprint-retrospective
description: Facilitate effective sprint retrospectives for continuous team improvement. Use when conducting team retrospectives, identifying improvements, or reviewing sprint outcomes. Supports Start-Stop-Continue, Mad-Sad-Glad, and 4Ls formats.
user-invocable: true
allowed-tools: Read, Write, Edit, Glob
---

# Sprint Retrospective

## File Location

All retrospective documents are saved to:

```
.claude/context/retrospectives/retro-sprint-{number}.md
```

**Naming convention**: Match the sprint number from `.claude/context/sprints/`. Example: if closing Sprint 3, the file is `retro-sprint-03.md`.

When creating a new retrospective, check the corresponding sprint file in `.claude/context/sprints/` for context on what was accomplished.

## Integration with Sprint Planner

The retrospective references the sprint plan and its outcomes:

```
Sprint Plan:     .claude/context/sprints/sprint-{number}.md
Retrospective:   .claude/context/retrospectives/retro-sprint-{number}.md
Product Backlog: .claude/context/development/product_backlog.md
Changelog:       .claude/context/development/backlog_changelog.md
```

Before starting a retrospective:
1. Review the sprint plan for committed stories, velocity, and outcome
2. Check which items were carried over vs. completed
3. Reference any action items from the previous retrospective

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
**Format**: [Start-Stop-Continue / Mad-Sad-Glad / 4Ls]

---

## Sprint Summary
- **Velocity (Planned)**: [points]
- **Velocity (Actual)**: [points]
- **Sprint Goal Achievement**: [percentage]
- **Stories Completed**: [X of Y]
- **Carryover Items**: [list or "None"]

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
```

## Constraints

### Required Rules (MUST)

1. **Safe Space**: a blame-free environment
2. **Action Items**: must be specific and actionable (limit 2-3)
3. **Follow-up**: check progress of previous action items in the next retrospective

### Prohibited (MUST NOT)

1. **Personal attacks**: improve the process, not the person
2. **Too many actions**: limit to 2-3 focused improvements

## Best Practices

1. **Time-box**: within 1 hour
2. **Rotate Facilitator**: team members take turns facilitating
3. **Celebrate Wins**: acknowledge successes, not just problems
4. **Track Trends**: compare velocity and morale across sprints

## End of Sprint Context Update

The completion of a sprint retrospective signifies the absolute end of the sprint. Once the retrospective document has been generated and saved, you MUST execute the `/update-context` workflow to synchronize `CLAUDE.md`, `.claude/context/product-vision.md`, and `README.md` with the new project state.

## References

- [Retrospective Formats](https://retromat.org/)
- [Agile Retrospectives](https://www.amazon.com/Agile-Retrospectives-Making-Teams-Great/dp/0977616649)
