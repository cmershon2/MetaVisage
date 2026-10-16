---
name: review-pr
description: Review a pull request, branch diff, staged diff, or working tree for correctness, security, performance, tests, and project conventions.
---

# PR Review

Review the PR, branch, staged diff, or local changes named in the user's prompt. If the user explicitly asks for parallel review, use the `code-reviewer` custom agent or focused subagents for separate review tracks.

## Fetch the Diff

If the prompt names a PR number:

```bash
gh pr diff <number>
gh pr view <number>
```

If the prompt names a branch:

```bash
git diff main...<branch>
git log main...<branch> --oneline
```

If no target is provided:

```bash
git diff --staged
git diff
```

## Review Checklist

Check only for issues that materially matter:

- Correctness: logic errors, missed edge cases, bad error handling, concurrency bugs
- Security: secrets, injection, path traversal, authz gaps, sensitive logs
- Performance: N+1 queries, blocking async work, avoidable repeated work, missing indexes
- Code quality: unnecessary complexity, dead code, risky abstractions, unclear ownership
- Tests: missing coverage for new behavior, edge cases, regressions, or integration boundaries
- Git hygiene: unrelated changes or misleading commit messages
- UI quality, when the diff touches frontend surfaces: accessibility,
  responsive behavior, visual hierarchy, loading/empty/error states, text
  overflow, and design-system consistency.

Use `.agents/rules/security.md`, `.agents/rules/testing.md`, `.agents/rules/code-style.md`, and `.agents/rules/git-workflow.md` as supporting standards.
Use `.agents/rules/design-workflow.md` for frontend review support. When
`impeccable` is installed or enabled separately, suggest `$impeccable audit`
for technical UI quality or `$impeccable critique` for UX/design review if the
diff needs deeper visual assessment.

## Output Format

Lead with findings, ordered by severity:

```markdown
## Findings
- [severity] `path/file.ext:line` - Issue and recommended fix.

## Open Questions
- [question, if any]

## Summary
[brief change summary]

## Verdict
[Approve | Request changes | Needs discussion]
```

Omit empty sections. If no issues are found, say that clearly and mention any remaining test gaps or residual risk.
