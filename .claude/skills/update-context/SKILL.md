---
name: update-context
description: Synchronize CLAUDE.md, product-vision.md, and README.md with the current project state. Run at the end of each sprint or after significant changes to keep documentation accurate.
user-invocable: true
allowed-tools: Read, Write, Edit, Bash, Glob
---

# Update Context

Synchronize the three primary context files so project documentation stays accurate as the codebase evolves.

## Step 1 — Review Current Project Status

1. Read the most recent sprint document in `.claude/context/sprints/`
2. Read the active development backlog at `.claude/context/development/product_backlog.md`
3. Read `.claude/context/product-vision.md` if it exists
4. Synthesize the current state: what was recently completed, what is in progress, what the immediate goals are

## Step 2 — Update Product Vision

1. Review `.claude/context/product-vision.md`
2. Update strategic goals and current project state based on Step 1
3. Keep it concise — this is strategic context, not a changelog

## Step 3 — Generate Current Directory Tree

```bash
python -c "
import os, pathlib

def tree(d, prefix='', ignore={'.git','node_modules','venv','__pycache__','.turbo','.vscode','dist','build','.next','coverage'}, max_depth=3, depth=0, out=None):
    entries = [x for x in sorted(d.iterdir(), key=lambda y: (y.is_file(), y.name)) if x.name not in ignore]
    for i, entry in enumerate(entries):
        connector = '└── ' if i == len(entries) - 1 else '├── '
        out.write(prefix + connector + entry.name + ('/' if entry.is_dir() else '') + '\n')
        if entry.is_dir() and depth < max_depth:
            extension = '    ' if i == len(entries) - 1 else '│   '
            tree(entry, prefix + extension, ignore, max_depth, depth + 1, out)

root = pathlib.Path('.')
project_name = root.resolve().name
with open('tree.txt', 'w', encoding='utf-8') as f:
    f.write(project_name + '/\n')
    tree(root, out=f)
"
```

## Step 4 — Update CLAUDE.md

1. Read the newly generated `tree.txt`
2. Update the `Repository Structure` section in `CLAUDE.md` to reflect the current tree
3. Review commands, tech stack notes, and key conventions — update anything that has changed

## Step 5 — Update README.md

1. Update the `Project Structure` section (high-level overview only — not the full deep tree)
2. Update any prerequisites, start commands, or architecture descriptions that have shifted
3. Ensure listed commands still match the actual project setup

## Step 6 — Cleanup

```bash
rm -f tree.txt
```
