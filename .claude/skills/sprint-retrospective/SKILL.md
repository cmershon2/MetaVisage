---
name: sprint-retrospective
description: Facilitate effective sprint retrospectives for continuous team improvement. Use when conducting team retrospectives, identifying improvements, or reviewing sprint outcomes. Supports Start-Stop-Continue, Mad-Sad-Glad, and 4Ls formats.
user-invocable: true
allowed-tools: Read, Write, Edit, Glob
---

# Sprint Retrospective

## Pre-Flight DoD Check (Story 4.7)

**This check runs first, before any other step in this skill.** It exists to
break a recurring anti-pattern flagged in the Sprint 3.6 / 3.7 / 3.8 retros:
sprint outcomes were marked "Complete" or "Implementation Complete" before the
PR review and manual UI smoke had actually run, so the retro analyzed an
outcome that hadn't been validated.

### Steps

1. Locate the active sprint file at `.claude/context/sprints/sprint-{number}.md`.
   The number is supplied either by the user (`/sprint-retrospective 04`) or
   inferred from the highest-numbered sprint file present.
2. Read the file's `## Sprint Outcome` section. Parse the `**Status**:` line.
3. **If `Status` is `Planned` or `In Progress`**: the retro is being run mid-
   sprint or before close-out. Skip the rest of this pre-flight check and
   proceed normally. (Ad-hoc retros on incomplete sprints are still
   supported.)
4. **If `Status` is `Complete` or `Implementation Complete`**: read the
   `## Definition of Done` checklist. Scan every list item that begins with
   `- [ ]` (unchecked) for any of these substrings, case-insensitive:
   - `"PR opened"` *or* `"/review-pr"` *or* `"blocking findings resolved"`
   - `"manual UI smoke"` *or* `"Phase D"` *or* `"manual smoke"`
   - `"sprint outcome notes updated"`
5. **If any of those substrings appear in an unchecked DoD item**, REFUSE to
   proceed. Print a message naming each blocking item verbatim and stop
   execution. The user must either (a) tick the boxes once they're actually
   done, or (b) revert the Status field to `In Progress` to bypass the
   check legitimately for an ad-hoc retro.

### Refusal message template

```
Cannot run /sprint-retrospective on sprint-{N}.md (Status: Complete) — the
following Definition of Done items are still unchecked:

  - [ ] {first blocking item, copied verbatim}
  - [ ] {second blocking item}
  ...

Either tick those boxes once the work is genuinely done, or set Status back
to "In Progress" if this is an ad-hoc mid-sprint retro. The /sprint-retro-
spective skill refuses to run on a "Complete" sprint with unchecked DoD
items because the Sprint 3.6 / 3.7 / 3.8 retros all flagged this as a
recurring anti-pattern (Sprint 4 Action Item #3).
```

Only after this check passes (or is skipped because Status is not Complete)
do you proceed to the retrospective generation steps below.

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
