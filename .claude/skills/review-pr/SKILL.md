---
name: review-pr
description: Review a pull request for correctness, security, performance, and adherence to project conventions. Works with GitHub PR numbers or local diffs.
user-invocable: true
context: fork
agent: code-reviewer
argument-hint: "[pr-number or branch-name]"
---

# PR Review

Review the pull request or branch diff at `$ARGUMENTS`.

## Fetch the Diff

If `$ARGUMENTS` is a PR number:
```bash
gh pr diff $ARGUMENTS
gh pr view $ARGUMENTS
```

If `$ARGUMENTS` is a branch name:
```bash
git diff main...$ARGUMENTS
git log main...$ARGUMENTS --oneline
```

If no argument, review uncommitted staged changes:
```bash
git diff --staged
```

## Review Checklist

Go through each changed file and evaluate:

### Correctness
- [ ] Logic is correct for stated purpose
- [ ] Edge cases handled (nulls, empty collections, boundary values)
- [ ] No obvious bugs or off-by-one errors
- [ ] Error handling is appropriate

### Security (see @.claude/rules/security.md)
- [ ] No secrets, credentials, or sensitive data in code
- [ ] User input validated at boundaries
- [ ] No injection vectors (SQL, shell, XSS)
- [ ] Authorization checks present where needed

### Performance
- [ ] No N+1 queries or avoidable repeated work in loops
- [ ] Appropriate indexing considered for database changes
- [ ] No blocking calls in async/event-driven code paths

### Code Quality (see @.claude/rules/code-style.md)
- [ ] Functions are focused and appropriately sized
- [ ] No unnecessary complexity or premature abstraction
- [ ] Dead code removed
- [ ] Comments explain *why*, not *what*

### Tests (see @.claude/rules/testing.md)
- [ ] New behavior is tested
- [ ] Tests cover edge cases and error conditions
- [ ] No test-only mocks hiding real integration issues

### Git
- [ ] Commit messages follow Conventional Commits
- [ ] No unrelated changes bundled in

## Output Format

Provide a structured review:

```
## Summary
[1-2 sentence overview of the change]

## Issues
### Blocking
- [file:line] — [issue description and suggested fix]

### Non-blocking
- [file:line] — [suggestion or nit]

## Positives
- [anything done particularly well]

## Verdict
[ ] Approve  [ ] Request changes  [ ] Needs discussion
```

Only include sections that have content. Be specific — reference file and line numbers.
