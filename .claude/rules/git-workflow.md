# Git Workflow

## Commit Messages

Use Conventional Commits format:

```
<type>(<optional scope>): <short summary>

<optional body — explain why, not what>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `ci`, `perf`

- Summary line: imperative mood, under 72 characters, no period
- Body: explain motivation and contrast with previous behavior
- Breaking changes: add `!` after type, or `BREAKING CHANGE:` in footer

## Branching

- `main` / `master` — production-ready code; protected
- `dev` or `develop` — integration branch (optional, for larger teams)
- Feature branches: `feat/<short-description>`
- Bug fixes: `fix/<short-description>`
- Hotfixes: `hotfix/<short-description>`

## Pull Requests

- One focused change per PR — avoid mixing features with refactors
- PR description: what changed and why; link to the issue/PRD
- All CI checks must pass before merging
- At least one approval required on shared repos
- Squash or rebase to keep main history clean

## Safety Rules

- Never force push to `main` or shared branches
- Never commit directly to `main` — always use a PR
- Never use `--no-verify` to skip hooks — fix the underlying issue
- Stage specific files (`git add <file>`), not `git add .` near sensitive files
- Review `git diff --staged` before every commit

## Tags and Releases

- Use semantic versioning: `MAJOR.MINOR.PATCH`
- Tag releases: `git tag -a v1.2.3 -m "Release v1.2.3"`
- Document breaking changes in release notes
