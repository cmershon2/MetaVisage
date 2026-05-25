---
name: project-onboarding
description: Initialize a new project from a PRD or onboard Codex to an existing project by inspecting requirements, stack, commands, context, and agent guidance.
---

# Project Onboarding

Use this skill when the user asks to start from a PRD, turn requirements into a project scaffold, or add this agent boilerplate to an existing codebase.

Prefer the project-runtime workflow: the `codex-agent-boilerplate` plugin is
the upstream install and upgrade source, while each target repository keeps the
visible, editable, active agent runtime files plus a
`.agents/boilerplate.lock.json` sync record.

## Inputs

Read the user's prompt for a PRD path, pasted PRD content, or a request to onboard the current repository. If the prompt does not include enough information, ask for the missing PRD path/content or confirm that this is an existing-project onboarding pass.

## Step 1 - Classify the Workspace

Determine which mode applies:

- **New project from PRD**: the workspace contains a PRD and no real application source yet. The boilerplate runtime files should already be installed locally by `$codex-agent-boilerplate:install`.
- **Existing project onboarding**: the workspace already contains source code, manifests, tests, or deployment configuration.

For existing projects, do not scaffold application files or overwrite manifests, README, source files, tests, or environment examples unless the user explicitly asks.
Do not copy this whole boilerplate repository as the target application's root.
Instead, install or update the project-local runtime files from the plugin
payload.

## Step 2 - Read and Analyze

For a PRD, extract:

- Project name and a kebab-case slug
- Project type: web app, API, CLI, library, mobile app, data pipeline, or other
- Primary language and framework
- Database or storage needs
- Top requirements and non-functional constraints
- Deployment target, if mentioned

For an existing project, inspect:

- Manifests and lockfiles
- Source and test layout
- Build, lint, test, and run commands
- Existing README and docs
- Existing `PRODUCT.md` and `DESIGN.md`, especially for frontend projects
- Environment variable examples
- Current git branch and recent commits, if available

Default stack recommendations when a PRD is silent:

| Type | Language | Framework | Testing |
|------|----------|-----------|---------|
| Full-stack web app | TypeScript | Next.js + Tailwind | Vitest/Jest + Playwright |
| Frontend app | TypeScript | React + Vite + Tailwind | Vitest |
| REST API | TypeScript | Node.js + Fastify | Vitest/Jest |
| REST API (Python) | Python | FastAPI | pytest |
| CLI | Go | Cobra | stdlib testing |
| JS/TS library | TypeScript | none | Vitest |
| Python library | Python | none | pytest |
| Data pipeline | Python | project-specific | pytest |

## Step 2.5 - Frontend Design Companion

If the PRD or existing repo indicates a frontend, full-stack web app, marketing
site, dashboard, mobile UI, onboarding flow, or design system, recommend
installing or enabling `impeccable` separately from the Benchmarke Codex
Plugins marketplace. This is advisory only; do not require it for onboarding
and do not install it automatically.

When Impeccable is installed, prefer:

- `$impeccable teach` to create or refresh `PRODUCT.md` with users, brand,
  tone, anti-references, and design principles.
- `$impeccable document` to create or refresh `DESIGN.md` from an existing UI.
- `$impeccable shape` before substantial UI implementation.

For UI projects, include the current state of `PRODUCT.md` and `DESIGN.md` in
the onboarding summary. If either file is missing, record it as a design
context follow-up rather than blocking non-design setup.

## Step 3 - Confirm Before Mutating

Before creating or changing files, show a concise confirmation block:

```text
Mode:       [new project | existing project onboarding]
Project:    [name]
Type:       [type]
Language:   [language]
Framework:  [framework]
Database:   [database or none]
Testing:    [framework]
Deploy:     [target or not specified]

Planned changes:
  - [change 1]
  - [change 2]
  - Use project-local active agent config under .agents/ and .codex/
  - Initialize .agents/boilerplate.lock.json for plugin-backed upgrades
  - For UI work, recommend Impeccable separately for design context and critique

Proceed? (yes, or describe changes)
```

Wait for user confirmation. If the user changes the plan, update the summary and confirm again.

## Step 4 - New Project Scaffold

Only in new-project mode, create a minimal idiomatic project from the selected template:

- TypeScript/Node.js: `.agents/skills/project-onboarding/templates/node-ts.md`
- Python: `.agents/skills/project-onboarding/templates/python.md`
- Go: `.agents/skills/project-onboarding/templates/go.md`
- Generic/other: `.agents/skills/project-onboarding/templates/generic.md`

Always create:

- Language-appropriate dependency manifest
- `.env.example` with required variables and descriptions
- `src/` or the idiomatic equivalent with a minimal entry point
- `tests/` with one example test
- `docs/` with `.gitkeep` if no docs exist yet

Ensure the active project-local agent runtime exists in the project:

- `.agents/boilerplate-manifest.json`
- `.agents/skills/`
- `.agents/rules/`
- `.agents/context/` templates and initial project-owned context
- `.codex/agents/`
- `.codex/hooks/`
- `.codex/hooks.json`
- `.codex/config.toml`
- `.agents/vendor/codex-agent-boilerplate/` reference baseline

For new projects, these files should be created by
`$codex-agent-boilerplate:install` from the plugin payload. For existing
projects, add missing files and treat existing local files as user/project
edits unless the user approves replacing them.

## Step 5 - Codex Context

Generate or update `AGENTS.md` with:

- Project purpose and current status
- Setup and development commands
- Repository structure
- Architecture overview
- Environment variable guidance
- Project-specific conventions
- A short inline summary of plugin or local `.agents/rules/*`
- Available plugin/repo skills and custom agents
- For frontend projects, whether Impeccable is installed or recommended and
  where `PRODUCT.md` / `DESIGN.md` live

Keep `AGENTS.md` concise enough to load as persistent instructions. Detailed process docs belong under `.agents/context` and `.agents/rules`.

For existing projects, preserve useful existing README language and only add missing Codex-specific context.

## Step 6 - Planning Context

Initialize or update:

- `.agents/context/product-vision.md`
- `.agents/context/development/product_backlog.md`
- `.agents/context/development/backlog_changelog.md`
- `.agents/context/campaigns/campaign-template.md`
- `.agents/context/campaigns/readiness-template.md`
- `.agents/context/campaigns/handoff-template.md`
- `.agents/context/sprints/sprint-template.md`
- `.agents/context/retrospectives/retro-template.md`

For existing projects, summarize current state rather than inventing future
work. For PRD projects, seed the backlog from requirements. For UI projects,
capture any design-context follow-up for `PRODUCT.md`, `DESIGN.md`, or an
Impeccable `shape` pass in the backlog.

## Step 6.5 - Transparent Vendoring Lock

Create or update `.agents/boilerplate.lock.json`:

```json
{
  "schemaVersion": 1,
  "plugin": "codex-agent-boilerplate",
  "version": "[version from .codex-plugin/plugin.json]",
  "syncedAt": "[YYYY-MM-DD]",
  "manifest": ".agents/boilerplate-manifest.json",
  "managedFiles": {},
  "preserved": [
    ".agents/context/product-vision.md",
    ".agents/context/development/**",
    ".agents/context/campaigns/**",
    ".agents/context/sprints/**",
    ".agents/context/retrospectives/**"
  ]
}
```

If the project has legacy copied skills, rules, hook scripts, or custom agent
definitions, record their current checksums as the first managed-file baseline
instead of overwriting them during onboarding. Future updates should use
`$boilerplate-upgrade`.

The lock is the bridge between transparency and upgrades: users may inspect and
edit local active agent config at any time, and the upgrader will later
distinguish unchanged upstream files from local modifications.

## Step 7 - Git

If this is a new project and there is no `.git` directory, ask before running:

```bash
git init
git add .
git commit -m "feat: initialize project from PRD" \
  -m "- Change: add initial project scaffold and agent context
- Why: seed the repo from the approved PRD
- Validation: review generated files and onboarding summary"
```

If this is an existing project, do not initialize git. Leave changed files unstaged unless the user asks for a commit.

## Final Output

Summarize:

- Files created or updated
- Detected stack and commands
- Boilerplate plugin version and lock-file status
- Any assumptions
- Suggested next commands to install dependencies, configure `.env`, run tests, and start development

Do not claim that tests or commands passed unless you actually ran them.
