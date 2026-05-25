#!/usr/bin/env python3
"""PostToolUse hook: run lightweight advisory checks and campaign reminders."""

from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
import subprocess
import sys


MAX_OUTPUT = 4000


def run(cmd: list[str], cwd: pathlib.Path, timeout: int = 30) -> tuple[int, str]:
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except Exception as exc:
        return 1, str(exc)

    output = "\n".join(part for part in ((result.stdout or "").strip(), (result.stderr or "").strip()) if part)
    return result.returncode, output


def has_npm_lint(root: pathlib.Path) -> bool:
    package_json = root / "package.json"
    if not package_json.exists() or shutil.which("npm") is None:
        return False
    try:
        data = json.loads(package_json.read_text(encoding="utf-8"))
    except Exception:
        return False
    return "lint" in (data.get("scripts") or {})


def detect_checks(root: pathlib.Path) -> list[list[str]]:
    checks: list[list[str]] = []

    if has_npm_lint(root):
        checks.append(["npm", "run", "lint"])

    if shutil.which("ruff") and any((root / name).exists() for name in ("pyproject.toml", "ruff.toml", ".ruff.toml")):
        checks.append(["ruff", "check", "."])

    if shutil.which("go") and (root / "go.mod").exists():
        checks.append(["go", "vet", "./..."])

    if shutil.which("cargo") and (root / "Cargo.toml").exists():
        checks.append(["cargo", "check"])

    return checks[:1]


def git_root(cwd: pathlib.Path) -> pathlib.Path:
    code, output = run(["git", "rev-parse", "--show-toplevel"], cwd, timeout=8)
    return pathlib.Path(output.splitlines()[0]) if code == 0 and output else cwd


def command_from(data: dict) -> str:
    tool_input = data.get("tool_input") or data.get("input") or {}
    if isinstance(tool_input, dict):
        return str(tool_input.get("command") or tool_input.get("cmd") or "")
    return ""


def is_git_commit(command: str) -> bool:
    return bool(re.search(r"\bgit\s+commit\b", command)) and "--dry-run" not in command


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except Exception:
        return 0

    cwd = pathlib.Path(data.get("cwd") or os.getcwd())
    root = git_root(cwd)
    messages: list[str] = []
    command = command_from(data)

    if command:
        if is_git_commit(command):
            messages.append(
                "Campaign handoff reminder: if this commit closes a Campaign "
                "Autopilot sprint, write or refresh "
                "`.agents/context/campaigns/handoff-{campaign-slug}-sprint-{number}.md` "
                "before reassessing or planning the next sprint."
            )
    else:
        for cmd in detect_checks(root):
            code, output = run(cmd, root)
            if code != 0 or output:
                text = output[:MAX_OUTPUT]
                messages.append(f"`{' '.join(cmd)}` exited {code}:\n{text}")

    if messages:
        print(json.dumps({"systemMessage": "Advisory hook:\n\n" + "\n\n".join(messages)}))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
