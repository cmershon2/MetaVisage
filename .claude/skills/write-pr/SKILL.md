---
name: write-pr
description: Create a pull request for the current branch — writes a clear title and description from the diff and commit history, then opens the PR on GitHub.
user-invocable: true
allowed-tools: Bash, Read, Grep, Glob
argument-hint: "[base-branch (default: main)]"
---

# Write PR

Create a pull request for the current branch against `$ARGUMENTS` (default: `main`).

## Step 1 — Gather Context

```bash
# Identify current branch and base
git branch --show-current
git log main...HEAD --oneline         # replace main with $ARGUMENTS if provided
git diff main...HEAD --stat           # files changed
git diff main...HEAD                  # full diff
```

Also check for a linked issue or ticket number in the branch name or recent commits — include it in the PR if found.

## Step 2 — Analyze the Changes

From the diff and commit history, extract:

- **What changed** — the concrete set of modifications (files, features, fixes)
- **Why it changed** — the motivation or problem being solved
- **Scope** — is this a focused change or does it touch multiple concerns?
- **Risk areas** — anything that warrants reviewer attention (migrations, API changes, logic-heavy code, security-sensitive areas)

## Step 3 — Write the PR

Use this format:

**Title**: `<type>(<scope>): <short summary under 70 chars>`

Follow the same Conventional Commits types as commits: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `ci`, `perf`. Use imperative mood.

**Body**:

```markdown
## Summary

- [Bullet 1 — what this PR does]
- [Bullet 2 — if more than one thing changed]

## Why

[One to three sentences on the motivation. Link to issue/ticket if applicable: Closes #123]

## Changes

- `path/to/file` — [what changed and why]
- `path/to/file` — [what changed and why]

## Test Plan

- [ ] [How to manually verify the change works]
- [ ] [Edge case or regression to check]
- [ ] Tests pass: `[test command]`

## Notes for Reviewers

[Optional — highlight anything tricky, a deliberate trade-off, or a decision that needs discussion. Omit if nothing warrants it.]
```

Omit any section that has nothing meaningful to say. Keep the body concise — reviewers should be able to understand the PR in under two minutes.

## Step 4 — Create the PR

```bash
gh pr create \
  --title "<title>" \
  --body "<body>" \
  --base "${ARGUMENTS:-main}"
```

After creating, print the PR URL.

## Guidelines

- Do not inflate the description — if the change is small, the description should be small
- Never include secrets, internal hostnames, or credentials in the PR body
- If the branch has multiple unrelated changes, flag this to the user before creating — it may be worth splitting
- If `gh` is not authenticated, stop and tell the user to run `gh auth login`
