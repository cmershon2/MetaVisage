---
name: campaign-executor
description: Execute approved multi-sprint Campaign Autopilot runs from a PRD, roadmap, or sprint series through repeated sprint loops with handoff compaction.
---

# Campaign Executor

Use this skill when the user asks Codex to run a generalized PRD, roadmap, or
series of sprints across multiple sprint loops with minimal user involvement.

Campaign execution is governed by:

```text
.agents/rules/campaign-workflow.md
.agents/rules/sprint-workflow.md
```

## Files

Campaign state lives in:

```text
.agents/context/campaigns/campaign-{slug}.md
.agents/context/campaigns/readiness-{campaign-slug}.md
.agents/context/campaigns/handoff-{campaign-slug}-sprint-{number}.md
```

Use `.agents/context/campaigns/campaign-template.md` and
`.agents/context/campaigns/handoff-template.md` for new files. Use
`.agents/context/campaigns/readiness-template.md` for readiness preflight.
Sprint retrospective closeout evidence lives in
`.agents/context/retrospectives/retro-sprint-{number}.md`.

## Inputs

Campaign Autopilot accepts either:

- A PRD, roadmap, or product mandate that should be decomposed into backlog
  items and sprint loops.
- A prewritten sprint series in `.agents/context/sprints/`.

If the campaign file does not exist, create it and record the mandate, source,
approval status, sprint sequence, autonomy boundaries, and stop conditions.
Run `$campaign-readiness` before campaign approval or first execution. Do not
begin execution until campaign approval is recorded and readiness is `Ready` or
`Ready with gaps`.

## Readiness Preflight

Before execution:

1. Read the campaign mandate, PRD, roadmap, sprint series, backlog, repo docs,
   manifests, `.env.example`, deployment config, and known run/test commands.
2. Infer required permissions, auth, MCP/app connectors, CLIs, environment
   variables, external services, and secrets.
3. Use safe, read-only probes only.
4. Record available, missing, unknown, deferred, and workaround states in
   `readiness-{campaign-slug}.md`.
5. Record quality specification readiness for design system/style guide,
   navigation map, loading/empty/error-state guidance, live smoke needs,
   screenshot strategy, rendered image checks, and link-audit strategy when
   relevant.
6. Mark readiness as `Ready`, `Ready with gaps`, or `Blocked`.
7. Update the campaign file's `Readiness` and `Readiness Check` fields.

If readiness is `Blocked`, pause before execution and report the missing
critical access. If readiness is `Ready with gaps`, continue with documented
workarounds unless the gap later becomes critical.

## Execution Loop

After campaign approval, repeat this loop until the campaign is complete or a
high-risk stop condition is reached:

1. Read the campaign file, readiness file, latest handoff, latest
   retrospective, product backlog, product vision, and current sprint
   documents.
2. Confirm readiness is not `Blocked`.
3. Plan or select the next sprint.
4. Execute the sprint with broad autonomy inside the campaign mandate.
5. Perform tangential work inline when it improves the sprint or campaign
   outcome.
6. Verify with the sprint validation commands or project test guidance.
7. Update the sprint outcome, backlog status, campaign progress, and tangential
   work log.
8. Run `$sprint-retrospective`, including the required Sprint Quality Evidence
   section.
9. Confirm the retrospective is complete and `Campaign Loop Readiness` is
   `Ready for next sprint`. If the retro is missing, incomplete, `Blocked`, or
   `Needs reassessment`, pause instead of advancing.
10. Run `$update-context`; it must read the latest retrospective and carry
   durable findings, unresolved action items, and readiness status forward.
11. Run `$commit` for one focused sprint commit.
12. Write the campaign handoff summary, including quality evidence status and
   unresolved retro action items.
13. Reassess remaining work and continue to the next sprint only if the latest
   retrospective and handoff both support advancement.

## Autonomy

Campaign Autopilot is intentionally permissive.

- Choose implementation details without asking.
- Refactor supporting code when useful.
- Add or adjust tests, docs, tooling, and context files.
- Fix nearby defects discovered while completing the campaign.
- Reorder future sprint work when validation or discoveries make that clearly
  better for the campaign goal.

Pause only for destructive git/history changes, secrets or credentials,
billing or paid services, production-impacting actions, legal/security-sensitive
decisions, requirements that cannot reasonably be inferred, missing required
quality evidence, or retrospective readiness marked `Blocked` or `Needs
reassessment`. Named human-review checkpoints and campaign-end checkpoints are
hard stops even when all tests are green.

## Handoff

After every sprint commit, write a compact handoff containing:

- What changed.
- Current product and repo state.
- Completed and carried-over work.
- Tangential work performed.
- Validation results.
- Sprint Quality Evidence status from the retrospective.
- Unresolved retrospective action items or known issues.
- Remaining campaign priorities.
- Recommended next sprint goal.

Use the handoff as the first context source for the next loop.

## Final Output

When the campaign completes or pauses, report:

- Campaign status.
- Completed sprints and commits.
- Validation summary.
- Remaining backlog or carryover.
- Latest handoff path.
