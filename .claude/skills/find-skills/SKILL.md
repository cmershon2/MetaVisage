---
name: find-skills
description: Discover and install community skills from the open agent skills ecosystem (skills.sh). Use when looking for existing skills for a domain or capability, or wanting to extend Claude Code with community-built skills.
user-invocable: true
allowed-tools: Bash, Glob
argument-hint: "[search query]"
---

# Find Skills

Helps discover and install skills from the open agent skills ecosystem.

## When to Use

- User asks "find a skill for X" or "is there a skill that can..."
- User asks "how do I do X" where X might have an existing community skill
- User wants to extend Claude Code with specialized domain knowledge

## Skills CLI

The Skills CLI (`npx skills`) is the package manager for the open agent skills ecosystem.

**Key commands:**

- `npx skills find [query]` — Search for skills by keyword
- `npx skills add <package>` — Install a skill
- `npx skills check` — Check for skill updates
- `npx skills update` — Update all installed skills

**Browse at:** https://skills.sh/

## Workflow

### Step 1: Search

Run with a relevant query based on what the user needs:

```bash
npx skills find [query]
```

Examples:
- "how do I make my React app faster?" → `npx skills find react performance`
- "help with PR reviews?" → `npx skills find pr review`
- "create a changelog?" → `npx skills find changelog`

### Step 2: Present Options

Show results with skill name, description, and install command. Example:

```
Found: "vercel-react-best-practices" — React and Next.js performance optimization guidelines.

Install command:
  npx skills add vercel-labs/agent-skills@vercel-react-best-practices

Learn more: https://skills.sh/vercel-labs/agent-skills/vercel-react-best-practices
```

### Step 3: Install

If the user wants to proceed, install and move to Claude Code's skills directory:

```bash
npx skills add <owner/repo@skill>
```

The CLI installs files to `.agents/skills/`. Move them to Claude Code's skills location and clean up:

```bash
# Unix/macOS/Git Bash
cp -r .agents/skills/<skill-name> .claude/skills/<skill-name>
rm -rf .agents/
```

```powershell
# Windows PowerShell
Copy-Item -Path ".agents\skills\<skill-name>" -Destination ".claude\skills\<skill-name>" -Recurse
Remove-Item -Path ".agents" -Recurse -Force
```

### Step 4: Verify

Confirm the skill landed correctly:

```bash
ls .claude/skills/<skill-name>/
```

The skill directory should contain a `SKILL.md` file.

## When No Skills Are Found

Acknowledge the gap and offer to help directly. Suggest creating a custom skill:

```bash
npx skills init my-skill-name
```

## Common Categories

| Category | Example Queries |
|----------|-----------------|
| Web Development | react, nextjs, typescript, tailwind |
| Testing | testing, jest, playwright, e2e |
| DevOps | deploy, docker, kubernetes, ci-cd |
| Documentation | docs, readme, changelog, api-docs |
| Code Quality | review, lint, refactor, best-practices |
| Design | ui, ux, design-system, accessibility |
| Productivity | workflow, automation, git |
