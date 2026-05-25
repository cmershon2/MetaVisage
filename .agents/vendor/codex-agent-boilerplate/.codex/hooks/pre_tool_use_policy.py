#!/usr/bin/env python3
"""PreToolUse hook: block clearly risky shell commands and warn on commit risks."""

from __future__ import annotations

import json
import os
import pathlib
import re
import subprocess
import sys


CONVENTIONAL_TYPES = "feat|fix|docs|style|refactor|test|chore|ci|perf|build|revert"
COMMIT_PATTERN = re.compile(
    rf"^({CONVENTIONAL_TYPES})(\([A-Za-z0-9_/\-]+\))?(!)?: .{{1,72}}$"
)

BLOCK_PATTERNS: list[tuple[re.Pattern[str], str]] = [
    (
        re.compile(r"\bgit\s+reset\s+--hard\b", re.IGNORECASE),
        "git reset --hard is destructive. Ask the user to confirm a safer recovery plan first.",
    ),
    (
        re.compile(r"\bgit\s+push\b.*\s--force(?:-with-lease)?\b", re.IGNORECASE),
        "Force pushing is blocked by repository policy. Ask the user before rewriting remote history.",
    ),
    (
        re.compile(r"\brm\s+-[^\n;]*r[^\n;]*f[^\n;]*\s/(?:\s|$)", re.IGNORECASE),
        "Recursive deletion of the filesystem root is blocked.",
    ),
    (
        re.compile(r"\bRemove-Item\b[^\n;]*\s-(?:Recurse|r)\b[^\n;]*(?:\s[A-Za-z]:\\|\s/|\s~)", re.IGNORECASE),
        "Broad recursive Remove-Item commands are blocked by repository policy.",
    ),
]

SECRET_PATTERNS: list[tuple[re.Pattern[str], str]] = [
    (re.compile(r"(?i)(api[_-]?key|apikey)\s*[:=]\s*[\"']?[A-Za-z0-9_\-]{20,}"), "API key"),
    (re.compile(r"(?i)(secret[_-]?key|client_secret)\s*[:=]\s*[\"']?[A-Za-z0-9_\-]{16,}"), "secret key"),
    (re.compile(r"(?i)(password|passwd)\s*[:=]\s*[\"']?[^\s$'\"]{8,}"), "password"),
    (re.compile(r"(?i)(access_token|auth_token|bearer)\s*[:=]\s*[\"']?[A-Za-z0-9_\-.]{16,}"), "auth token"),
    (re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"), "private key"),
    (re.compile(r"(?:sk|pk)-[A-Za-z0-9]{32,}"), "provider key"),
    (re.compile(r"ghp_[A-Za-z0-9]{36}"), "GitHub token"),
    (re.compile(r"AKIA[0-9A-Z]{16}"), "AWS access key ID"),
    (re.compile(r"(?i)database_url\s*[:=]\s*[\"']?[a-z]+://[^@\s]+:[^@\s]+@"), "database URL with credentials"),
]


def json_out(payload: dict) -> None:
    print(json.dumps(payload))


def command_from(data: dict) -> str:
    tool_input = data.get("tool_input") or data.get("input") or {}
    if isinstance(tool_input, dict):
        return str(tool_input.get("command") or "")
    return ""


def deny(reason: str) -> None:
    json_out(
        {
            "hookSpecificOutput": {
                "hookEventName": "PreToolUse",
                "permissionDecision": "deny",
                "permissionDecisionReason": reason,
            }
        }
    )


def git_diff_cached(cwd: pathlib.Path) -> str:
    try:
        result = subprocess.run(
            ["git", "diff", "--cached", "--unified=0"],
            cwd=str(cwd),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=10,
            check=False,
        )
    except Exception:
        return ""
    return result.stdout or ""


def git_root(cwd: pathlib.Path) -> pathlib.Path:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            cwd=str(cwd),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=8,
            check=False,
        )
    except Exception:
        return cwd

    root = (result.stdout or "").strip()
    return pathlib.Path(root) if result.returncode == 0 and root else cwd


def extract_commit_message(command: str) -> str:
    match = re.search(r"-m\s+(?:\"((?:[^\"\\]|\\.)*)\"|'((?:[^'\\]|\\.)*)')", command)
    if not match:
        return ""
    return (match.group(1) or match.group(2) or "").strip().splitlines()[0]


def latest_sprint(root: pathlib.Path) -> pathlib.Path | None:
    sprint_dir = root / ".agents" / "context" / "sprints"
    if not sprint_dir.exists():
        return None

    candidates = [
        path
        for path in sprint_dir.glob("sprint-*.md")
        if "template" not in path.name.lower()
    ]
    if not candidates:
        return None

    return sorted(candidates, key=lambda path: path.name, reverse=True)[0]


def has_approved_campaign(root: pathlib.Path) -> bool:
    campaign_dir = root / ".agents" / "context" / "campaigns"
    if not campaign_dir.exists():
        return False

    for path in campaign_dir.glob("campaign-*.md"):
        if "template" in path.name.lower():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            continue
        if "**Approval**:" in text and "**Approval**: Pending" not in text:
            return True
    return False


def sprint_commit_warnings(root: pathlib.Path) -> list[str]:
    sprint = latest_sprint(root)
    if sprint is None:
        return []

    try:
        text = sprint.read_text(encoding="utf-8")
    except OSError:
        return []

    missing: list[str] = []
    campaign_mode = "Campaign Autopilot" in text
    campaign_approved = campaign_mode and has_approved_campaign(root)
    if ("**Approval**: Pending" in text or "**Approval**:" not in text) and not campaign_approved:
        missing.append("approval")
    if "**Status**: Planned" in text or "**Status**: In Progress" in text:
        missing.append("complete sprint status")
    if "**Validation**:" not in text:
        missing.append("recorded validation")

    sprint_number = re.search(r"sprint-(\d+)\.md$", sprint.name, re.IGNORECASE)
    if sprint_number:
        retro = root / ".agents" / "context" / "retrospectives" / f"retro-sprint-{sprint_number.group(1)}.md"
        if not retro.exists():
            missing.append("retrospective")

    if not missing:
        return []

    details = ", ".join(dict.fromkeys(missing))
    return [
        f"Sprint workflow gate: {sprint.name} is missing {details}. "
        "For final sprint commits, complete the sprint plan or campaign approval -> "
        "execution -> tests -> confirmation -> retrospective -> context-update loop first. "
        "Proceed only for an explicit interim checkpoint commit."
    ]


def warnings_for_commit(command: str, cwd: pathlib.Path) -> list[str]:
    if not re.search(r"\bgit\s+commit\b", command):
        return []

    warnings: list[str] = []
    root = git_root(cwd)
    first_line = extract_commit_message(command)
    if first_line and not COMMIT_PATTERN.match(first_line):
        warnings.append(
            "Commit message does not follow Conventional Commits. "
            "Expected `<type>(<scope>): <summary>` with a summary under 72 characters."
        )

    diff = git_diff_cached(root)
    findings: list[str] = []
    for line in diff.splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        for pattern, label in SECRET_PATTERNS:
            if pattern.search(line):
                findings.append(label)
                break

    if findings:
        labels = ", ".join(dict.fromkeys(findings))
        warnings.append(
            f"Possible {labels} found in staged changes. Review before committing; "
            "continue only if these are placeholders or safe test fixtures."
        )

    warnings.extend(sprint_commit_warnings(root))

    return warnings


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except Exception:
        return 0

    command = command_from(data)
    if not command:
        return 0

    for pattern, reason in BLOCK_PATTERNS:
        if pattern.search(command):
            deny(reason)
            return 0

    cwd = pathlib.Path(data.get("cwd") or os.getcwd())
    warnings = warnings_for_commit(command, cwd)
    if warnings:
        json_out({"systemMessage": "\n".join(f"Advisory: {warning}" for warning in warnings)})

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
