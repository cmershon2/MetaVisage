---
name: write-pr
description: Create a GitHub pull request from the current branch by summarizing the diff, writing a concise title/body, and calling gh.
---

# Write PR

Create a pull request for the current branch. Treat a branch name in the user's prompt as the base branch; default to `main`.

## Step 1 - Gather Context

```bash
git branch --show-current
git log <base>...HEAD --oneline
git diff <base>...HEAD --stat
git diff <base>...HEAD
```

Check the branch name and recent commits for issue or ticket references.

## Step 2 - Analyze the Changes

Extract:

- What changed
- Why it changed
- Risk areas for reviewers
- Tests or verification performed
- Any unrelated changes that should be split before opening the PR

## Step 3 - Write the PR

Title format:

```text
<type>(<scope>): <short summary under 70 chars>
```

Use a concise body:

```markdown
## Summary
- [what changed]

## Why
[motivation, link issue if applicable]

## Changes
- `path/to/file` - [what changed]

## Test Plan
- [ ] [verification step]
- [ ] Tests pass: `[test command]`

## Notes for Reviewers
[optional]
```

Omit sections that have no useful content.

## Step 4 - Create the PR

```bash
gh pr create --title "<title>" --body "<body>" --base "<base>"
```

After creating, print the PR URL.

## Guidelines

- Keep small PRs small.
- Never include secrets, credentials, or internal-only hostnames in the PR body.
- If `gh` is not authenticated, stop and tell the user to run `gh auth login`.
