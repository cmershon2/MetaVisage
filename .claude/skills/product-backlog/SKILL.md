---
name: product-backlog
description: Manage and maintain the product backlog and backlog changelog. Use when adding, updating, completing, or reorganizing backlog items. Enforces a standardized format for consistency. Handles moving completed items to the changelog with completion metadata.
---

# Product Backlog Management

This skill manages two files that work together to track product work:

1. **Product Backlog** (`product_backlog.md`) — Active and upcoming work items
2. **Backlog Changelog** (`backlog_changelog.md`) — Archive of completed items with completion metadata

## File Locations

Both files live in the agent context directory:

```
.claude/context/development/product_backlog.md
.claude/context/development/backlog_changelog.md
```

## When to Use This Skill

- User asks to add a feature, enhancement, or bug fix to the backlog
- User asks to update, reprioritize, or reorganize backlog items
- User asks to mark a backlog item as complete
- User asks to review the backlog or see what's been completed
- A sprint or task is finished and completed items should be archived
- User asks to clean up or standardize the backlog

## Product Backlog Format

The backlog uses a strict, standardized format. Every item must follow this structure.

### File Header

```markdown
# Product Backlog

**Project**: [Project Name]
**Last Updated**: [Month Day, Year]

---
```

**Rule**: Always update `Last Updated` when modifying the backlog.

### Priority Sections

Items are organized into four priority tiers:

```markdown
## 🔥 High Priority
## 🟡 Medium Priority
## 🟢 Lower Priority
## 🎯 Future Phases
```

### Epics (Optional Grouping)

Related items can be grouped under an epic heading within a priority section:

```markdown
### [Epic Name]
**Epic**: [One-line description of what this group of work achieves]
```

### Backlog Item Format

Every item MUST follow this exact structure:

```markdown
- [ ] **[Item Title]**
  - [Description line 1]
  - [Description line 2]
  - [Additional detail lines as needed]
  - **Effort**: [estimate, e.g. ~2 hours, ~1 day, ~1 sprint]
  - **Value**: [business value statement]
```

#### Required Fields

| Field | Description | Example |
|-------|-------------|---------|
| **Checkbox** | `- [ ]` for open, `- [/]` for in-progress | `- [ ]` |
| **Title** | Bold, concise feature name | `**Async SQLAlchemy Migration**` |
| **Description** | 1+ lines explaining what to build | Bullet points with details |
| **Effort** | Time estimate | `~2 hours`, `~1 day`, `~6-8 hours` |
| **Value** | Why this matters to the user/product | `Enables multi-agent concurrency` |

#### Optional Fields

| Field | When to Use | Example |
|-------|-------------|---------|
| **Why** | Complex items needing rationale | `**Why**: Current sync backend blocks...` |
| **Dependencies** | Items that must complete first or packages needed | `**Dependencies**: aiosqlite, asyncpg` |
| **Complexity** | Non-obvious difficulty | `**Complexity**: Medium-high` |
| **Blocked By** | If waiting on another item | `**Blocked By**: Async Migration` |

### Rules for the Active Backlog

1. **No completed items** — Items marked `[x]` must be moved to the Backlog Changelog
2. **In-progress items** use `[/]` — Indicates work has started but isn't finished
3. **Consistent field order** — Description → Effort → Value → (optional fields)
4. **No duplicate items** — Search before adding
5. **Update the date** — `Last Updated` must reflect the most recent change

### Footer

The backlog ends with a Notes section and Related Documents:

```markdown
---

## 📝 Notes

[Versioning notes, prioritization criteria, or other context]

---

## 🔗 Related Documents

- [Backlog Changelog](./backlog_changelog.md) - Completed items archive
```

## Backlog Changelog Format

The changelog is an append-only archive of completed backlog items, organized by completion date.

### File Structure

```markdown
# Backlog Changelog

**Project**: [Project Name]
**Purpose**: Archive of completed product backlog items

---

## [Month Year]

### [YYYY-MM-DD] — [Summary of what was completed]

- ✅ **[Item Title]**
  - [Original description from backlog]
  - **Effort (Estimated)**: [original estimate]
  - **Completed**: [date or sprint reference]
  - **Notes**: [optional — lessons learned, scope changes, or deviations]

---
```

### Rules for the Changelog

1. **Append-only** — Never delete or modify past entries
2. **Group by month** — Each month gets its own `## [Month Year]` section
3. **Date entries** — Each completion event gets a `### [YYYY-MM-DD]` heading
4. **Preserve original details** — Copy the item description as-is from the backlog
5. **Add completion metadata** — Date, optional notes about what changed during implementation

## Workflows

### Adding a New Backlog Item

1. Open `product_backlog.md`
2. Determine the appropriate priority section
3. Determine if it belongs under an existing epic or needs a new one
4. Add the item using the standardized format (see Backlog Item Format above)
5. Update `Last Updated` in the file header

### Completing a Backlog Item

1. Copy the completed item from `product_backlog.md`
2. Remove it from the backlog (delete the entire item block)
3. Open or create `backlog_changelog.md`
4. Add a new entry under the current month with today's date
5. Change `- [ ]` to `- ✅` and add completion metadata
6. Update `Last Updated` in the backlog header

### Reprioritizing Items

1. Move the item block to the new priority section
2. Preserve all item metadata
3. Update `Last Updated` in the header
4. Optionally add a note about why priority changed

### Sprint Completion Batch Archive

When a sprint finishes, multiple items may need to move at once:

1. Identify all `[x]` or completed items in the backlog
2. Create a single changelog entry with today's date and a sprint summary
3. Move all completed items into that entry
4. Remove them from the active backlog
5. Update `Last Updated`

## Standardization Checklist

When asked to clean up or standardize the backlog, verify:

- [ ] All items use `- [ ]` checkbox format (not `✅` or `- [x]`)
- [ ] All items have bold **Title**
- [ ] All items have **Effort** estimate
- [ ] All items have **Value** statement
- [ ] No completed (`[x]`) items remain in the active backlog
- [ ] Items are in the correct priority section
- [ ] `Last Updated` date is current
- [ ] Related Documents section includes link to Backlog Changelog
- [ ] Footer Notes section exists

## Tips

- When the user describes a feature casually, extract the key details and format it properly before adding
- When a large feature is described, consider breaking it into multiple smaller items with individual estimates
- If the user doesn't specify a priority, default to **🟡 Medium Priority** and confirm
- Always confirm with the user before moving items between priority sections
- When completing items, ask if there are any lessons learned to capture in the changelog Notes field
