# Sprint Workflow

Use this rule for planned sprint work. The sprint is the unit of planning,
approval, execution, validation, learning, context refresh, and commit.

For multi-sprint autonomous work, also follow
`.agents/rules/campaign-workflow.md`. Campaign Autopilot repeats this sprint
loop across multiple sprints after one approved campaign mandate.

## Required Loop

Classic sprint work must follow this order:

1. Create a sprint plan in `.agents/context/sprints/sprint-{number}.md`.
2. Get explicit user or product-owner approval for the sprint plan.
3. Execute only the approved sprint scope, updating the sprint document as work progresses.
4. Test the completed work with the commands listed in the sprint plan or project context.
5. Confirm the sprint is done by updating the sprint outcome, definition of done, and backlog status.
6. Run a retrospective with complete Sprint Quality Evidence and save it to `.agents/context/retrospectives/retro-sprint-{number}.md`.
7. Update project context with `$update-context`, using the retrospective as a first-class input.
8. Commit the focused sprint changes with `$commit`.

Campaign Autopilot sprint work follows the same loop, but the campaign approval
authorizes Codex to plan and execute later sprint loops automatically while the
work remains inside the campaign mandate.

## Approval Gate

- Do not begin implementation until the sprint plan exists and approval is recorded.
- Record approval in the sprint plan as `**Approval**: Approved by [name] on [YYYY-MM-DD]`, or link to the approving message when available.
- If the user changes scope after approval, update the sprint plan and get approval for the revised scope.

## Execution Rules

- Keep implementation aligned to the approved sprint backlog.
- In Classic mode, move newly discovered work to the product backlog unless the user approves adding it to the current sprint.
- In Campaign Autopilot mode, perform tangential work inline when it improves the sprint or campaign outcome, and record notable tangents in the sprint and campaign logs.
- Keep the sprint plan current when story status, risks, carryover, or affected files change.
- Keep the sprint plan's Quality Gates & Evidence section current when required
  validation changes.
- Use the project testing rule for validation, and record the exact checks run.

## Completion Gate

Before a sprint can be treated as complete:

- Sprint backlog items are marked complete or carried over.
- Product backlog items are updated, and completed work is moved to the backlog changelog.
- Definition of done is checked off or exceptions are documented.
- Tests and validation results are recorded.
- The retrospective includes complete Sprint Quality Evidence for all required
  gates declared in the sprint plan.
- The user confirms the sprint is done.

## Retrospective and Context Gate

- Run the retrospective after completion confirmation, not before.
- Use the retrospective as both the learning artifact and the closeout evidence
  artifact.
- Capture 2-3 concrete action items in the retrospective.
- Campaign Autopilot may continue only when the latest retrospective's Campaign
  Loop Readiness is `Ready for next sprint`.
- After saving the retrospective, run `$update-context` so `AGENTS.md`, README, product vision, and backlog context reflect the finished sprint and any durable retro findings.
- Commit only after the sprint plan, backlog/changelog, retrospective, and context updates are complete.
- For Campaign Autopilot, write the compact campaign handoff after the sprint commit, then reassess and plan the next sprint.
