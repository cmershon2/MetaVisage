---
name: update-context
description: Refresh AGENTS.md, product vision, backlog context, and README after meaningful project changes or sprint completion.
---

# Update Context

Synchronize the primary Codex context files so future sessions start with accurate project knowledge.

## Step 1 - Review Current State

1. Read the most recent sprint document in `.agents/context/sprints/`.
2. Read the matching latest retrospective in `.agents/context/retrospectives/`.
3. Read the active campaign, readiness file, and latest handoff in `.agents/context/campaigns/`, if present.
4. Read `.agents/context/development/product_backlog.md`.
5. Read `.agents/context/product-vision.md`.
6. For frontend projects, read `PRODUCT.md` and `DESIGN.md` when present and
   note whether Impeccable has been used or is recommended.
7. Inspect manifests, README, and recent source/test layout changes.
8. Summarize what changed, what is in progress, what quality evidence was
   recorded, what unresolved retro action items remain, and what is next.

## Step 2 - Update Product Vision

Update `.agents/context/product-vision.md` with current strategic goals and project state. Keep it concise; this file is product context, not a changelog.
Reflect only durable retrospective findings that affect product direction,
quality posture, or next priorities.

## Step 3 - Update AGENTS.md

Refresh:

- Setup, build, lint, test, and run commands
- Repository structure
- Architecture notes
- Environment variable guidance
- Project-specific conventions
- Active skills and custom agents
- Active campaign workflow, when the project uses Campaign Autopilot
- Durable frontend design context, including `PRODUCT.md`, `DESIGN.md`, and
  whether `impeccable` is an installed or recommended companion plugin

Keep detailed historical notes in `.agents/context`, not in `AGENTS.md`.
Do not copy sprint-specific retrospective detail into `AGENTS.md`; carry forward
only durable conventions, current state, required commands, active blockers, and
campaign readiness.

## Step 4 - Update README.md

Ensure README setup, usage, requirements, and project structure match the real project.
Update README only for durable user/developer-facing changes. Do not turn it
into a sprint closeout log.
For frontend projects, mention `PRODUCT.md` / `DESIGN.md` and Impeccable only
when they are part of the real workflow or recommended team workflow.

## Step 5 - Cleanup and Verify

Remove temporary files created during discovery. Run lightweight validation commands when available, and report commands that were not run.

## Step 6 - Prepare for Commit

For completed sprint work, verify the sprint workflow rule has been satisfied before committing:

- Sprint plan exists and records approval.
- Sprint outcome, validation, and backlog completion state are current.
- Retrospective exists for the completed sprint and includes complete Sprint
  Quality Evidence.
- Unresolved retrospective action items are moved to the backlog, campaign
  risks, next-sprint planning context, or explicitly deferred.
- Context files have been refreshed.
- For Campaign Autopilot, campaign progress is current and the handoff will be written after the sprint commit.
- For Campaign Autopilot, readiness status is current or intentionally deferred.
- For Campaign Autopilot, do not describe the campaign as ready to continue if
  the latest retrospective says `Blocked` or `Needs reassessment`.

After these checks, use the `commit` skill for the focused final sprint commit.
