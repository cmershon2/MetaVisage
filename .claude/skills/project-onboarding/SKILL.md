---
name: project-onboarding
description: Initialize a new project from a PRD. Analyzes requirements, selects tech stack, scaffolds directory structure, generates project-specific CLAUDE.md, and sets up version control.
user-invocable: true
allowed-tools: Read, Write, Edit, Bash, Glob, Grep, WebSearch, WebFetch, Agent, TodoWrite
argument-hint: "[path/to/prd.md]"
---

# Project Onboarding

You are initializing a new project from a PRD. Follow these steps precisely. Use TodoWrite to track progress and mark each step complete as you go.

## Input

The PRD is at `$ARGUMENTS`. If no argument is provided, ask the user to either paste the PRD content or provide a file path before proceeding.

---

## Step 1 — Read and Analyze the PRD

Read the PRD and extract:

- **Project name** — derive a kebab-case slug if not explicit
- **Project type** — web app, REST/GraphQL API, CLI tool, library/package, mobile app, data pipeline, or other
- **Primary language and framework** — use what is specified; if unspecified, recommend based on project type (see defaults below)
- **Database** — if applicable
- **Key features** — top 5-10 bullet points from the requirements
- **Non-functional requirements** — performance, security, scalability, compliance constraints
- **Deployment target** — cloud provider, containerized, serverless, etc. (if mentioned)

**Default tech stack recommendations by project type (use when not specified):**

| Type | Language | Framework | Testing |
|------|----------|-----------|---------|
| Web app (full-stack) | TypeScript | Next.js + Tailwind | Jest + Playwright |
| Web app (frontend only) | TypeScript | React + Vite + Tailwind | Vitest |
| REST API | TypeScript | Node.js + Fastify | Jest |
| REST API (Python) | Python | FastAPI | pytest |
| CLI | Go | cobra | testing (stdlib) |
| Library (JS) | TypeScript | — | Vitest |
| Library (Python) | Python | — | pytest |
| Data pipeline | Python | — | pytest |

---

## Step 2 — Present Summary and Confirm

Display a confirmation block before creating any files:

```
Project:    [name]
Type:       [type]
Language:   [language]
Framework:  [framework]
Database:   [database or "none"]
Testing:    [framework]
Deploy:     [target or "not specified"]

Key features:
  - [feature 1]
  - [feature 2]
  ...

Proceed? (y to confirm, or describe changes)
```

Wait for the user to confirm or request changes before continuing. If they request changes, update your plan and show the summary again.

---

## Step 3 — Scaffold Project Structure

Create the project structure based on the confirmed tech stack. Follow the conventions in the appropriate template:

- **TypeScript/Node.js projects**: See @.claude/skills/project-onboarding/templates/node-ts.md
- **Python projects**: See @.claude/skills/project-onboarding/templates/python.md
- **Go projects**: See @.claude/skills/project-onboarding/templates/go.md
- **Generic / other**: See @.claude/skills/project-onboarding/templates/generic.md

Always create:
- Language-appropriate package/dependency manifest
- `.env.example` listing all required environment variables with descriptions
- `src/` (or equivalent) with a minimal entry point
- `tests/` directory with one example test file
- `docs/` directory (can be empty with `.gitkeep`)

---

## Step 4 — Generate Project CLAUDE.md

**Replace** the contents of `CLAUDE.md` with a project-specific version. Preserve the `.claude/` rules imports. Include:

```markdown
# [Project Name]

[1-3 sentence description from the PRD]

## Setup

\`\`\`bash
[install command]
\`\`\`

## Development

| Command | Description |
|---------|-------------|
| `[run command]` | Start development server |
| `[test command]` | Run tests |
| `[build command]` | Build for production |
| `[lint command]` | Run linter |

## Architecture

[Brief overview of key directories and their purpose, 5-10 bullets]

## Environment Variables

See `.env.example` for required variables. Copy to `.env` and fill in values before running.

## Key Conventions

[3-5 project-specific conventions that Claude should follow — derived from the PRD and tech stack]

## Standards

See @.claude/rules/code-style.md for coding conventions.
See @.claude/rules/testing.md for testing requirements.
See @.claude/rules/security.md for security guidelines.
See @.claude/rules/git-workflow.md for git workflow.

## Agents

- `code-reviewer` — Invoked by `/review-pr`. Read-only, learns project conventions over time.
- `architect` — Answers questions about how the codebase works. Read-only, builds a structural map in memory.
- `test-writer` — Writes tests for existing code, matching project test patterns.
- `dependency-auditor` — Scans dependencies for vulnerabilities. Read-only, tracks deferred issues in memory.

## Available Skills

- `/commit` — Generate a semantic commit message
- `/review-pr [pr-number]` — Review a pull request
```

---

## Step 5 — Configure Project Hooks

Update `.claude/settings.json` to add a `PostToolUse` lint hook for the confirmed tech stack.

This hook runs after every `Edit` or `Write` and surfaces lint output as context for Claude — advisory only (always exit 0 via pipe) so it never blocks editing.

Add the following to the `"hooks"` object in `.claude/settings.json`, choosing the block that matches the tech stack:

**TypeScript / Node.js** (ESLint):
```json
"PostToolUse": [
  {
    "matcher": "Edit|Write",
    "hooks": [{ "type": "command", "command": "npm run lint 2>&1 | head -30", "timeout": 30 }]
  }
]
```

**Python** (ruff):
```json
"PostToolUse": [
  {
    "matcher": "Edit|Write",
    "hooks": [{ "type": "command", "command": "ruff check . 2>&1 | head -30", "timeout": 30 }]
  }
]
```

**Go**:
```json
"PostToolUse": [
  {
    "matcher": "Edit|Write",
    "hooks": [{ "type": "command", "command": "go vet ./... 2>&1 | head -30", "timeout": 30 }]
  }
]
```

**Other / no linter configured**: Skip this step — do not add a placeholder.

The `head -30` pipe ensures the command always exits 0 regardless of lint errors, so output flows to Claude as context rather than blocking. Claude will address issues it sees without the user needing to ask.

---

## Step 6 — Initialize Git

```bash
git init
git add .
git commit -m "feat: initialize [project-name] from PRD"
```

---

## Step 7 — Final Summary

Print a clean summary:

```
✓ Project scaffolded: [project-name]
✓ CLAUDE.md generated
✓ Lint hook configured in .claude/settings.json
✓ Git initialized with initial commit

Next steps:
  1. [Install dependencies command]
  2. Copy .env.example → .env and fill in values
  3. [Any framework-specific setup step]
  4. [Start development command]

Optionally, create a GitHub repo:
  gh repo create [project-name] --public --source=. --remote=origin --push
```

---

## Guidelines

- Match complexity to the PRD — do not over-engineer
- Prefer widely-used, well-documented packages
- Keep the generated CLAUDE.md under 150 lines
- Preserve the `.claude/` directory and all its contents — only replace `CLAUDE.md`
- If the PRD is ambiguous about the tech stack, ask rather than assume for anything that significantly affects architecture
