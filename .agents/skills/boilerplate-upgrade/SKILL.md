---
name: boilerplate-upgrade
description: Check or apply plugin-backed upgrades for project-local codex-agent-boilerplate runtime files while preserving project-owned context.
---

# Boilerplate Upgrade

Use this skill when the user asks to check, upgrade, sync, migrate, or apply
updates from `codex-agent-boilerplate` to an existing project.

The workflow is review-first. The plugin is a control plane and source package;
projects keep the active runtime files locally so users can inspect and edit
instructions, skills, rules, hooks, and subagent definitions. Never overwrite
project-owned context. In check mode, produce a report only. In apply mode,
refresh reference baselines and update only safe active files.

## Inputs

Infer the mode from the prompt:

- **Check mode**: prompts like "check", "report", "what would change", or no
  explicit apply instruction.
- **Apply mode**: prompts like "apply", "upgrade this project", "sync managed
  files", or explicit approval after a previous report.

Use the current repository as the target project unless the user gives another
path.

## Step 1 - Locate Upgrade Metadata

Read the upstream metadata from the boilerplate plugin or payload:

- `.codex-plugin/plugin.json` for the available plugin version.
- `payload/.agents/boilerplate-manifest.json`, or
  `.agents/boilerplate-manifest.json` in a development checkout, for artifact
  source, target, ownership, activation, and upgrade policy.

Read the target project's lock file when it exists:

- `.agents/boilerplate.lock.json`

If the lock file is missing, treat the project as a legacy or first-sync
project and report local modifications as `unknown baseline` instead of safe.

When this skill is available from a local plugin checkout, prefer the helper
script for the file comparison:

```bash
python .agents/skills/boilerplate-upgrade/scripts/upgrade.py --mode check --project-root .
```

If running from a target project with a separate plugin checkout, pass the
plugin path explicitly:

```bash
python path/to/plugin/payload/.agents/skills/boilerplate-upgrade/scripts/upgrade.py --mode check --project-root . --plugin-root path/to/plugin
```

## Step 2 - Classify Files

Use the manifest artifact policy:

- **Reference**: files under `.agents/vendor/codex-agent-boilerplate/`. These
  are visible upstream baselines used for transparency and comparison.
- **Project-active**: files in `AGENTS.md`, `.agents/skills/`, `.agents/rules/`,
  `.codex/agents/`, `.codex/hooks/`, `.codex/hooks.json`, and
  `.codex/config.toml`. Codex reads runtime behavior from these project files.
- **Managed**: project-active files that may be updated from the plugin when
  unchanged locally, such as skills, rules, custom agents, and the manifest.
- **Merge**: project-facing files. Compare and report differences, but do not
  replace automatically.
- **Preserve**: project-owned durable context. Never overwrite, even if the
  plugin contains a newer template or example nearby.

For managed files, compare:

- upstream file hash,
- target file hash,
- lock-file hash from the last sync, when present.

Classify each managed file as:

- `current`: target matches upstream.
- `missing`: target file does not exist.
- `update available`: target differs from upstream and still matches the lock
  baseline.
- `local modification`: target differs from both upstream and the lock baseline.
- `merge required`: the file is project-facing and needs human/project-aware
  review.

## Step 3 - Check Mode Report

In check mode, do not edit files. Output:

- current target boilerplate version from `.agents/boilerplate.lock.json`, or
  `untracked` when missing,
- available plugin version from `.codex-plugin/plugin.json`,
- reference baseline files that are current, missing, updateable, or locally
  modified,
- active project files that are current, missing, updateable, locally modified,
  or merge-required,
- preserved paths that will not be overwritten,
- recommended commit plan.

Make the recommendation explicit:

- Apply managed updates only when there are no local modifications in those
  files, or when the user confirms replacing them.
- Review merge files manually.
- Keep preserved planning context untouched.
- Keep project-local active agent config editable; local modifications are a
  first-class review signal, not an error.

## Step 4 - Apply Mode

Before mutating in apply mode, summarize the planned reference baseline and
active-file writes, then confirm the user has approved this apply step unless
the prompt already contains explicit approval.

Apply only these changes:

- Refresh missing or updateable reference files under `.agents/vendor/`.
- Copy plugin versions of missing managed active files.
- Replace updateable managed active files.
- Copy missing preserved seed files only during first install; never replace
  existing preserved files.
- Skip local modifications and report them for manual review.
- Do not write merge files unless the user explicitly asks for a specific merge.
- Do not install confirm-required active files, such as hooks, unless the user
  explicitly approves that activation.
- Do not write preserved files.

The helper script can apply the safe managed-file portion:

```bash
python .agents/skills/boilerplate-upgrade/scripts/upgrade.py --mode apply --project-root .
```

After managed-file writes, update `.agents/boilerplate.lock.json` with:

- `schemaVersion`,
- `plugin`,
- `version`,
- `syncedAt` as `YYYY-MM-DD`,
- `manifest`,
- `vendorRoot`,
- `managedFiles` checksums for reference and active target files,
- preserved path patterns from the manifest.

## Step 5 - Validation

Run lightweight validation after apply mode:

- JSON parse check for `.agents/boilerplate-manifest.json`,
  `.agents/boilerplate.lock.json`, `.codex-plugin/plugin.json`, and
  `.agents/plugins/marketplace.json` when present.
- If hook scripts changed, run a Python syntax compile check for
  `.codex/hooks/*.py`.
- Run `git status --short` and summarize changed files.

Do not claim tests passed unless they were run.

## Final Output

Summarize:

- target version before and after,
- managed files updated, skipped, or already current,
- merge files needing review,
- preserved project context,
- validation commands and results,
- recommended focused commit message.
