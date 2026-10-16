---
name: commit
description: Create a focused git commit by reviewing changes, staging only relevant files, and writing a detailed, bulleted Conventional Commits message.
---

# Commit

Generate a well-formed commit for the current working changes. Treat any extra wording in the user's prompt as optional scope or context.

## Steps

1. Run `git status`, `git diff --staged`, and `git diff` to understand what changed.
2. Run `git log --oneline -10` if history exists, so the new message matches the repo's style.
3. Determine the commit type, optional scope, active sprint number if any, and concise imperative summary.
4. For completed sprint work, verify the sprint workflow rule has reached the
   commit gate: approved sprint plan or campaign approval, validation recorded,
   completion confirmed, retrospective saved with complete Sprint Quality
   Evidence, and context updated from the retrospective.
5. Stage specific relevant files. Avoid `git add -A` near secrets or personal config.
6. Create the commit using:

```text
<type>(<scope>): <sprint N - summary under 72 chars>

- Change: <specific change made>
- Change: <additional relevant change, when applicable>
- Why: <reason this change is needed>
- Validation: <tests, checks, or review performed>
```

Allowed types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `ci`, `perf`, `build`, `revert`.

Omit the scope only when no meaningful scope exists. Omit `sprint N -` only when the work is not tied to a sprint or no active sprint number is available from the user, branch, or project context.

## Rules

- Use imperative mood: `add`, not `added`.
- Do not end the summary with a period.
- If the work belongs to a sprint, include the sprint number in the summary line as `sprint N - ...`.
- Always include a commit body.
- Format the body as a bulleted list only; do not use plain paragraphs.
- Make the body detailed enough to explain what changed, why it changed, and how it was validated.
- Include at least one `Change`, one `Why`, and one `Validation` bullet.
- Add `!` after the type or scope for breaking changes.
- Never use `--no-verify` to skip hooks.
- If hooks fail, stop and report the failure instead of retrying blindly.
- If possible secrets are present, stop and ask before staging or committing them.
- For sprint work, do not create the final sprint commit until
  `.agents/rules/sprint-workflow.md` has been satisfied, including complete
  retrospective Sprint Quality Evidence, unless the user explicitly asks for an
  interim checkpoint commit.
- For Campaign Autopilot, create one focused commit per completed sprint, then write the campaign handoff before reassessing or planning the next sprint.
- If the latest retrospective is missing, lacks required quality evidence, or
  marks Campaign Loop Readiness as `Blocked` or `Needs reassessment`, stop
  before the final sprint commit unless the user explicitly asks for an interim
  checkpoint commit.

## Output

After committing, print the commit hash and commit message.
