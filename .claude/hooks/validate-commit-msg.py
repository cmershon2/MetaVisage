#!/usr/bin/env python3
"""
PreToolUse hook — validate commit messages follow Conventional Commits.
Fires before any `git commit` command. Blocks (exit 2) if the format is wrong.
"""
import json
import re
import sys

data = json.load(sys.stdin)
command = data.get("input", {}).get("command", "")

# Only act on git commit commands
if not re.search(r'\bgit commit\b', command):
    sys.exit(0)

# Try to extract the commit message from -m flag (single or double quotes)
msg_match = re.search(r'-m\s+(?:"((?:[^"\\]|\\.)*)"|\'((?:[^\'\\]|\\.)*)\')', command)
if not msg_match:
    # Can't extract message (editor-based commit, heredoc, etc.) — let it through
    sys.exit(0)

msg = (msg_match.group(1) or msg_match.group(2)).strip()
first_line = msg.splitlines()[0] if msg else ""

# Conventional Commits: type(scope)!: summary
TYPES = "feat|fix|docs|style|refactor|test|chore|ci|perf|build|revert"
pattern = rf'^({TYPES})(\([a-zA-Z0-9_/\-]+\))?(!)?: .{{1,72}}$'

if not re.match(pattern, first_line):
    print(
        f"Commit blocked: message does not follow Conventional Commits.\n\n"
        f"  Provided:  {first_line!r}\n\n"
        f"  Format:    <type>(<scope>): <summary>\n"
        f"  Types:     feat, fix, docs, style, refactor, test, chore, ci, perf, build, revert\n"
        f"  Examples:  feat(auth): add OAuth2 login\n"
        f"             fix: handle null user on checkout\n"
        f"             chore(deps): bump fastapi to 0.110",
        file=sys.stderr
    )
    sys.exit(2)

sys.exit(0)
