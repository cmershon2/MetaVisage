# Codex Agent Boilerplate

Project-agnostic starter and project-local runtime plugin for Codex. Use it to install visible/editable Codex guidance into a project, start a new project from a PRD, and upgrade older boilerplate installs without overwriting project-owned context.

## How To Use

- New project: install or enable the plugin, run `Use $codex-agent-boilerplate:install to install this boilerplate`, restart Codex, add a PRD, then prompt Codex with `Use $project-onboarding with path/to/prd.md`.
- Existing project: install or enable the plugin, run `Use $codex-agent-boilerplate:install to install this boilerplate`, restart Codex, then prompt Codex with `Use $project-onboarding to onboard this existing project`.
- Existing boilerplate install: prompt Codex with `Use $codex-agent-boilerplate:upgrade to check this project`, review the report, then apply safe managed updates.
- Reusable install: use the Benchmarke Codex Plugins marketplace to install `codex-agent-boilerplate`, then run the plugin install skill for this project.
- Frontend design work: install or enable `impeccable` separately from the same
  marketplace. When it is installed, prefer `$impeccable teach`,
  `$impeccable document`, `$impeccable shape`, `$impeccable critique`,
  `$impeccable audit`, and `$impeccable polish` for UI/UX design quality.

## Repository Structure

```text
.
├── AGENTS.md
├── README.md
├── .agents/
│   ├── context/
│   ├── boilerplate-manifest.json
│   ├── boilerplate.lock.json
│   ├── plugins/
│   ├── rules/
│   ├── skills/
│   └── vendor/
├── .codex/
│   ├── agents/
│   ├── hooks/
│   ├── config.toml
│   └── hooks.json
└── .codex-plugin/
    └── plugin.json
```

## Standards

Keep changes simple, focused, and reversible. Prefer the existing project style over new abstractions. Validate inputs at system boundaries, keep secrets out of git, and ask before destructive or history-rewriting operations.

Detailed standards live in:

- `.agents/rules/code-style.md`
- `.agents/rules/testing.md`
- `.agents/rules/security.md`
- `.agents/rules/git-workflow.md`
- `.agents/rules/sprint-workflow.md`
- `.agents/rules/campaign-workflow.md`
- `.agents/rules/design-workflow.md`

Sprint work follows the required loop in `.agents/rules/sprint-workflow.md`:
create sprint plan in `.agents/context/sprints/`, get approval, execute, test, confirm done, run retrospective, update context, then commit.

Campaign Autopilot follows `.agents/rules/campaign-workflow.md` for approved
multi-sprint work: plan, execute, verify, retro, update context, commit, write a
compact handoff, reassess, then continue to the next sprint while inside the
campaign mandate.

## Skills

The installed plugin exposes only control-plane skills. Codex reads day-to-day boilerplate skills from the project-local `.agents/skills` directory after installation. Invoke them with `$` or let Codex choose them from the description.

- `codex-agent-boilerplate:install` - install project-local runtime files from the plugin payload
- `codex-agent-boilerplate:upgrade` - check or apply plugin payload updates to project-local runtime files
- `project-onboarding` - create a new project from a PRD or onboard an existing project
- `boilerplate-upgrade` - check or apply plugin-backed upgrades while preserving project context
- `commit` - stage focused changes and create a detailed, bulleted Conventional Commits message
- `write-pr` - create a GitHub PR from the current branch
- `review-pr` - review a PR, branch, staged diff, or working tree
- `product-backlog` - maintain backlog and changelog files
- `sprint-planner` - plan sprints from backlog items
- `campaign-readiness` - check campaign permissions, auth, connectors, tools, and environment needs before autopilot execution
- `campaign-executor` - execute approved multi-sprint campaigns across repeated sprint loops
- `sprint-retrospective` - facilitate and save retrospectives
- `update-context` - refresh `AGENTS.md`, product vision, backlog context, and README

Use built-in `$skill-creator`, `$skill-installer`, and `$plugin-creator` for creating or installing new Codex capabilities.

For frontend work, Impeccable is an optional companion plugin. The boilerplate
should recommend it for design context, critique, audit, polish, and visual
iteration when it is installed, but should not auto-install it or block non-UI
work when it is absent.

## Custom Agents

Project-scoped custom agents live in `.codex/agents`.

- `code-reviewer` - read-only review specialist
- `architect` - read-only codebase navigator
- `test-writer` - writes tests matching project patterns
- `dependency-auditor` - read-only package security auditor

Codex only spawns subagents when explicitly asked. Use them for parallel review, architecture exploration, dependency audit, or targeted test-writing work.

## Hooks

`.codex/hooks.json` enables advisory automation:

- `SessionStart` injects branch, recent commits, active sprint context, active campaign readiness, active campaign context, and latest handoff context.
- `PreToolUse` blocks clearly risky shell commands and warns on commit-message, staged-secret, or sprint workflow gate risks.
- `PostToolUse` runs one lightweight project check after edits when project tooling is detected and reminds Codex to write campaign handoff context after sprint commits.

Hooks require project-local `.codex/` config to be trusted by Codex.
