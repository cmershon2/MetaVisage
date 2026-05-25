#!/usr/bin/env python3
"""SessionStart hook: add lightweight git, sprint, and campaign context."""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys


def run(cmd: list[str], cwd: pathlib.Path) -> str:
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=8,
            check=False,
        )
    except Exception:
        return ""
    return (result.stdout or "").strip()


def git_root(cwd: pathlib.Path) -> pathlib.Path:
    root = run(["git", "rev-parse", "--show-toplevel"], cwd)
    return pathlib.Path(root) if root else cwd


def modified_time(path: pathlib.Path) -> float:
    try:
        return path.stat().st_mtime
    except OSError:
        return 0.0


def latest_sprint(root: pathlib.Path) -> str:
    sprint_dir = root / ".agents" / "context" / "sprints"
    if not sprint_dir.exists():
        return ""

    candidates = [
        path
        for path in sprint_dir.glob("sprint-*.md")
        if "template" not in path.name.lower()
    ]
    if not candidates:
        return ""

    sprint = sorted(candidates, key=lambda p: p.name, reverse=True)[0]
    lines = []
    try:
        for line in sprint.read_text(encoding="utf-8").splitlines():
            if any(
                key in line
                for key in (
                    "Sprint Goal",
                    "Status",
                    "Committed",
                    "Execution Mode",
                    "Campaign",
                )
            ):
                lines.append(f"  {line}")
    except OSError:
        return f"**Active sprint**: {sprint.name}"

    detail = "\n".join(lines)
    return f"**Active sprint**: {sprint.name}" + (f"\n{detail}" if detail else "")


def latest_campaign(root: pathlib.Path) -> str:
    campaign_dir = root / ".agents" / "context" / "campaigns"
    if not campaign_dir.exists():
        return ""

    candidates = [
        path
        for path in campaign_dir.glob("campaign-*.md")
        if "template" not in path.name.lower()
    ]
    if not candidates:
        return ""

    campaign = sorted(candidates, key=modified_time, reverse=True)[0]
    lines = []
    try:
        for line in campaign.read_text(encoding="utf-8").splitlines():
            if line.startswith(
                (
                    "**Campaign Goal**",
                    "**Status**",
                    "**Readiness**",
                    "**Readiness Check**",
                    "**Approval**",
                    "**Active Sprint**",
                    "**Next Action**",
                )
            ):
                lines.append(f"  {line}")
    except OSError:
        return f"**Active campaign**: {campaign.name}"

    detail = "\n".join(lines)
    return f"**Active campaign**: {campaign.name}" + (f"\n{detail}" if detail else "")


def latest_handoff(root: pathlib.Path) -> str:
    campaign_dir = root / ".agents" / "context" / "campaigns"
    if not campaign_dir.exists():
        return ""

    candidates = [
        path
        for path in campaign_dir.glob("handoff-*.md")
        if "template" not in path.name.lower()
    ]
    if not candidates:
        return ""

    handoff = sorted(candidates, key=modified_time, reverse=True)[0]
    lines = []
    try:
        for line in handoff.read_text(encoding="utf-8").splitlines():
            if line.startswith(
                (
                    "**Campaign**",
                    "**Sprint**",
                    "**Commit**",
                    "**Status**",
                    "**Recommended Next Sprint Goal**",
                )
            ):
                lines.append(f"  {line}")
    except OSError:
        return f"**Latest campaign handoff**: {handoff.name}"

    detail = "\n".join(lines)
    return f"**Latest campaign handoff**: {handoff.name}" + (f"\n{detail}" if detail else "")


def latest_readiness(root: pathlib.Path) -> str:
    campaign_dir = root / ".agents" / "context" / "campaigns"
    if not campaign_dir.exists():
        return ""

    candidates = [
        path
        for path in campaign_dir.glob("readiness-*.md")
        if "template" not in path.name.lower()
    ]
    if not candidates:
        return ""

    readiness = sorted(candidates, key=modified_time, reverse=True)[0]
    lines = []
    try:
        for line in readiness.read_text(encoding="utf-8").splitlines():
            if line.startswith(
                (
                    "**Campaign**",
                    "**Status**",
                    "**Final Decision**",
                )
            ):
                lines.append(f"  {line}")
    except OSError:
        return f"**Campaign readiness**: {readiness.name}"

    summary = f"**Campaign readiness**: {readiness.name}"
    detail = "\n".join(lines)
    if detail:
        summary += f"\n{detail}"

    if any("Blocked" in line or "Ready with gaps" in line for line in lines):
        summary += "\n  Advisory: resolve critical blockers or follow documented workarounds before uninterrupted autopilot execution."

    return summary


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except Exception:
        data = {}

    cwd = pathlib.Path(data.get("cwd") or os.getcwd())
    root = git_root(cwd)

    sections: list[str] = []
    branch = run(["git", "branch", "--show-current"], root)
    if branch:
        sections.append(f"**Branch**: {branch}")

    recent = run(["git", "log", "--oneline", "-5"], root)
    if recent:
        indented = "\n".join(f"  {line}" for line in recent.splitlines())
        sections.append(f"**Recent commits**:\n{indented}")

    sprint = latest_sprint(root)
    if sprint:
        sections.append(sprint)

    campaign = latest_campaign(root)
    if campaign:
        sections.append(campaign)

    readiness = latest_readiness(root)
    if readiness:
        sections.append(readiness)

    handoff = latest_handoff(root)
    if handoff:
        sections.append(handoff)

    if not sections:
        return 0

    additional_context = "## Session Context\n\n" + "\n\n".join(sections)
    print(
        json.dumps(
            {
                "hookSpecificOutput": {
                    "hookEventName": "SessionStart",
                    "additionalContext": additional_context,
                }
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
