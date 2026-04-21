---
name: commit
description: Generate a semantic commit message following Conventional Commits, stage relevant files, and create the commit.
user-invocable: true
allowed-tools: Bash, Read, Grep
argument-hint: "[optional: scope or extra context]"
---

# Commit

Generate a well-formed commit for the current working changes.

## Steps

1. Run `git status` and `git diff --staged` and `git diff` to understand what changed
2. Run `git log --oneline -10` to match the repo's existing commit style
3. Analyze the changes — determine the type, scope (optional), and a concise summary
4. Stage specific changed files (never `git add -A` or `git add .` near sensitive files):
   - Check `git status` for untracked files first
   - Stage files relevant to the change
   - Skip any files that look like secrets, credentials, or personal config
5. Create the commit using this format:

```
<type>(<scope>): <summary under 72 chars>

<body — explain WHY, not what. Only include if the diff doesn't make it obvious.>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `ci`, `perf`

## Rules

- Summary line: imperative mood ("add" not "added"), no period, under 72 chars
- If `$ARGUMENTS` was provided, use it as additional context or the scope
- Add `!` after type for breaking changes
- Never use `--no-verify` to skip hooks
- If pre-commit hooks fail, stop and report the failure — do not retry blindly

## Output

After the commit, print the commit hash and message so the user can review it.
