#!/usr/bin/env bash
# SessionStart hook — inject git and sprint context into every session.
# stdout is automatically added to Claude's context (exit 0).

# Skip if not a git repo
git rev-parse --git-dir > /dev/null 2>&1 || exit 0

BRANCH=$(git branch --show-current 2>/dev/null || echo "detached HEAD")
RECENT=$(git log --oneline -5 2>/dev/null)

printf "## Session Context\n\n"
printf "**Branch**: %s\n\n" "$BRANCH"

if [ -n "$RECENT" ]; then
  printf "**Recent commits**:\n"
  echo "$RECENT" | sed 's/^/  /'
  printf "\n"
fi

# Load the most recent non-template sprint file if one exists
SPRINT=$(ls .claude/context/sprints/sprint-*.md 2>/dev/null | grep -v template | sort -r | head -1)
if [ -n "$SPRINT" ]; then
  SPRINT_NAME=$(basename "$SPRINT")
  printf "**Active sprint**: %s\n" "$SPRINT_NAME"
  grep -E "(Sprint Goal|Status|Committed):" "$SPRINT" 2>/dev/null | sed 's/^/  /'
fi

exit 0
