# Campaign Workflow

Use this rule when the user approves a multi-sprint Campaign Autopilot run.
A campaign is the unit of long-range mandate, autonomy, repeated sprint loops,
handoff compaction, and reassessment.

## Campaign Inputs

Campaign Autopilot can start from either:

- A generalized PRD, roadmap, or product mandate.
- A prewritten series of sprint plans in `.agents/context/sprints/`.

For PRD or roadmap inputs, derive the product backlog and initial sprint
sequence before execution. For prewritten sprint inputs, preserve the supplied
order unless new evidence makes a different order clearly better for the
approved campaign goal.

## Campaign Approval

- Create a campaign file in `.agents/context/campaigns/campaign-{slug}.md`.
- Record the campaign mandate, source input, approval, stop conditions, and
  initial sprint sequence.
- Run a Campaign Readiness Check before approval or first execution, and link
  the readiness file from the campaign file.
- Record campaign-wide quality gates, including any required design system,
  navigation map, loading/empty/error-state guidance, live smoke tests,
  screenshot strategy, rendered image checks, and link audits.
- Campaign approval authorizes Codex to plan and execute multiple sprint loops
  without returning for approval between sprints, as long as work remains inside
  the approved mandate and every completed sprint has retrospective closeout
  evidence marked ready.

Record approval as:

```markdown
**Approval**: Approved by [name] on [YYYY-MM-DD]
```

## Campaign Readiness

Before Campaign Autopilot starts execution, use `$campaign-readiness` to infer
and check permissions, auth, MCP/app connectors, CLIs, environment variables,
external services, and secrets needed for uninterrupted work.

- Use only safe, read-only probes.
- Never request, print, or store secret values.
- Record only presence, absence, unknown state, setup instructions, and
  workarounds.
- Missing optional access should not block the campaign.
- Missing critical access should mark the campaign readiness as `Blocked`.
- Refresh readiness when new external access needs are discovered.
- Include quality specification readiness: design/style guidance, navigation
  map, loading/empty/error-state guidance, live smoke needs, screenshot
  strategy, rendered image checks, and link-audit strategy when relevant.

Readiness files are saved as:

```text
.agents/context/campaigns/readiness-{campaign-slug}.md
```

## Autonomy Rules

Campaign Autopilot is intentionally broad and practical.

- Execute the campaign goal, not only the literal first draft of the plan.
- Do tangential work inline when it improves the current sprint or campaign
  outcome, including nearby refactors, tests, docs, validation helpers, context
  updates, or small architecture adjustments.
- Record notable tangential work in the sprint and campaign logs.
- Reassess after every sprint using the handoff, backlog, retrospective action
  items, validation results, and current repo state.
- Continue to the next sprint automatically while work remains within the
  approved campaign mandate and the latest retrospective records complete
  Sprint Quality Evidence with `Campaign Loop Readiness: Ready for next sprint`.

## Stop Conditions

Pause and ask the user only for high-risk boundaries:

- Destructive git or history rewriting.
- Secrets, credentials, billing, or paid external services.
- Production or real-user-impacting actions.
- Legal, compliance, or security-sensitive decisions.
- Requirements that cannot reasonably be inferred from the campaign mandate.
- Missing or incomplete required Sprint Quality Evidence in the latest
  retrospective.
- `Campaign Loop Readiness` marked `Needs reassessment` or `Blocked`.
- Campaign-end checkpoints or named human-review checkpoints in the approved
  campaign.

When blocked, record the blocker in the campaign file and preserve enough
handoff context for the next session to resume cleanly.

## Repeated Sprint Loop

Each sprint inside a campaign follows this loop:

1. Confirm campaign readiness is `Ready` or `Ready with gaps`.
2. Plan the next sprint from the campaign mandate, backlog, and latest handoff.
3. Execute with loose autonomy inside the sprint and campaign goal.
4. Verify with the sprint validation commands or project test guidance.
5. Update sprint outcome, backlog status, and campaign progress.
6. Run the sprint retrospective, including complete Sprint Quality Evidence.
7. Stop if the retrospective is missing required evidence or marks Campaign
   Loop Readiness as `Needs reassessment` or `Blocked`.
8. Update project context with `$update-context`, using the retrospective as a
   first-class input.
9. Commit one focused sprint commit with `$commit`.
10. Write a compact handoff summary in `.agents/context/campaigns/`.
11. Reassess priorities and plan the next sprint only if the retrospective and
   handoff support advancement.

## Handoff Compaction

After each sprint commit, write a handoff file named:

```text
.agents/context/campaigns/handoff-{campaign-slug}-sprint-{number}.md
```

The handoff must include:

- What changed.
- Current product and repo state.
- Completed and carried-over work.
- Tangential work performed.
- Validation results.
- Sprint Quality Evidence status from the retrospective.
- Unresolved retrospective action items and known issues.
- Remaining campaign priorities.
- Recommended next sprint goal.

Use the latest handoff as the starting context for the next sprint so the
campaign can resume from compact, current state instead of carrying excessive
conversation history.
