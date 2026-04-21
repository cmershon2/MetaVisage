---
name: create-skill
description: Scaffold a new Claude Code skill following Anthropic best practices. Use when a user wants to create a reusable slash command, automate a workflow, or package domain knowledge into a skill.
user-invocable: true
allowed-tools: Read, Write, Edit, Glob, Bash
argument-hint: "[description of what the skill should do]"
---

# Create Skill

Scaffold a new skill at `.claude/skills/<name>/SKILL.md` based on `$ARGUMENTS`.

If no argument is provided, ask the user to describe:
1. What the skill should do
2. When it should be triggered
3. What inputs it takes (if any)
4. What output or action it produces

---

## Step 1 — Review Existing Skills

Before writing anything, read the existing skills to match the project's style:

```bash
ls .claude/skills/
```

Read 1–2 existing `SKILL.md` files for reference. Use their structure and tone as a baseline.

---

## Step 2 — Design the Skill

Determine the following before writing a single line of SKILL.md:

### Name
- kebab-case, verb-first where possible (`create-X`, `review-X`, `write-X`, `run-X`)
- Should be obvious at a glance what it does

### Frontmatter fields

| Field | When to use |
|-------|-------------|
| `name` | Always — the slash command name |
| `description` | Always — front-load the use case; include trigger phrases |
| `user-invocable: true` | Default for slash commands users invoke explicitly |
| `user-invocable: false` | Background knowledge/context Claude reads automatically |
| `disable-model-invocation: true` | Actions with side effects (deploys, pushes, sends) — only the user can trigger |
| `allowed-tools` | List only the tools the skill actually needs — principle of least privilege |
| `argument-hint` | If the skill accepts `$ARGUMENTS`, describe the expected input |
| `context: fork` | If the skill should run in an isolated subagent (long tasks, separate context) |
| `agent: Explore` | To use a specific subagent type (`Explore`, `Plan`, etc.) |
| `paths` | Glob pattern — skill only activates for matching file paths (e.g., `src/**/*.ts`) |
| `shell` | Only if you need `powershell`; default is `bash` |

### String substitutions available in skill body
- `$ARGUMENTS` — everything the user typed after the slash command
- `$0`, `$1`, `$2` — individual space-separated arguments
- `${CLAUDE_SESSION_ID}` — current session ID
- `${CLAUDE_SKILL_DIR}` — absolute path to the skill's directory
- `` !`command` `` — inject live shell output into the skill at load time

### Tool selection guide

| Need | Tools |
|------|-------|
| Read files or search code | `Read`, `Grep`, `Glob` |
| Write or modify files | `Write`, `Edit` |
| Run shell commands, git, CLI tools | `Bash` |
| Fetch documentation or web content | `WebFetch`, `WebSearch` |
| Spawn subagents for complex tasks | `Agent`, `TodoWrite` |

### Skill type patterns

**Action skill** (does something, has side effects):
- Set `disable-model-invocation: true`
- Use `allowed-tools` to limit blast radius
- Include explicit confirmation step before irreversible actions

**Workflow skill** (multi-step process, writes files):
- Use `TodoWrite` in `allowed-tools` for progress tracking
- Break into numbered steps
- Each step should end with a clear deliverable

**Query/analysis skill** (reads and reports):
- No `disable-model-invocation` needed
- Prefer `Read`, `Grep`, `Glob` — avoid `Bash` unless necessary
- Output format section is important

**Background knowledge skill** (context Claude absorbs):
- Set `user-invocable: false`
- No steps needed — write as reference documentation
- Keep concise; it loads every session

---

## Step 3 — Write the SKILL.md

Create the file at `.claude/skills/<name>/SKILL.md`.

### Structure to follow

```markdown
---
name: <name>
description: <front-loaded description with trigger phrases>
user-invocable: true
allowed-tools: <minimum tools needed>
argument-hint: "<hint>"   # omit if no arguments
---

# <Title>

<One sentence: what this skill does and what input it works on.>

## Step 1 — <First action>

<Instructions. Include code blocks for any commands to run.>

## Step 2 — <Next action>

...

## Output Format

<Define exactly what Claude should produce — structure, sections, level of detail.>

## Guidelines

<3–7 rules that constrain behavior and prevent common mistakes.>
```

### Size limits
- Keep SKILL.md under **500 lines**
- If the skill needs reference material (API docs, format specs, examples), create a separate file:
  - `.claude/skills/<name>/reference.md` — detailed reference
  - `.claude/skills/<name>/examples.md` — worked examples
  - `.claude/skills/<name>/scripts/` — helper scripts
- Reference them from SKILL.md using `@.claude/skills/<name>/reference.md`

### Writing quality checklist
- [ ] Description front-loads the primary use case (not "this skill helps you...")
- [ ] Steps are numbered and sequential — no ambiguity about order
- [ ] Every `Bash` command is shown in a code block
- [ ] Output format section defines structure precisely
- [ ] Guidelines prevent the most likely mistakes
- [ ] No steps that say "use your judgment" without boundaries

---

## Step 4 — Update CLAUDE.md

Add the new skill to the `## Available Skills` section in `CLAUDE.md`:

```markdown
- `/<name> [args]` — <one-line description matching the skill's purpose>
```

---

## Step 5 — Confirm

Print:
```
✓ Created: .claude/skills/<name>/SKILL.md
✓ Updated: CLAUDE.md

Invoke with: /<name> [args]
```

Then ask: "Would you like me to test this skill now, or refine anything?"

---

## Guidelines

- Match the complexity of the skill to what was asked — a simple formatter doesn't need 8 steps
- Never add tools to `allowed-tools` that the skill doesn't use
- If the user's description is vague, ask one clarifying question before writing — don't guess at scope
- Prefer explicit output formats over "Claude will decide what to say"
- Skills that modify the filesystem or run git commands should always confirm before acting
- Do not create a skill for something that's already covered by an existing skill — check first
