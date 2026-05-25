#!/usr/bin/env python3
"""Check and apply codex-agent-boilerplate project-runtime upgrades."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import pathlib
import shutil
import sys
from dataclasses import dataclass
from typing import Any


LOCK_PATH = pathlib.PurePosixPath(".agents/boilerplate.lock.json")


@dataclass
class TargetStatus:
    artifact_type: str
    role: str
    source: str
    target: str
    ownership: str
    activation: str
    install_policy: str
    upgrade_policy: str
    state: str
    source_hash: str
    target_hash: str
    lock_hash: str
    reason: str


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_json(path: pathlib.Path, default: Any = None) -> Any:
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: pathlib.Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=False) + "\n", encoding="utf-8")


def plugin_root_from_script() -> pathlib.Path:
    for parent in pathlib.Path(__file__).resolve().parents:
        if (parent / ".codex-plugin/plugin.json").exists():
            return parent
    return pathlib.Path(__file__).resolve().parents[4]


def source_root_for(plugin_root: pathlib.Path) -> pathlib.Path:
    payload_root = plugin_root / "payload"
    if (payload_root / ".agents/boilerplate-manifest.json").exists():
        return payload_root
    return plugin_root


def manifest_path_for(plugin_root: pathlib.Path, source_root: pathlib.Path) -> pathlib.Path:
    payload_manifest = source_root / ".agents/boilerplate-manifest.json"
    if payload_manifest.exists():
        return payload_manifest
    return plugin_root / ".agents/boilerplate-manifest.json"


def normalize(path: pathlib.Path, root: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def is_ignored_artifact(path: pathlib.Path) -> bool:
    parts = set(path.parts)
    return "__pycache__" in parts or path.suffix in {".pyc", ".pyo"}


def glob_base(pattern: str) -> pathlib.PurePosixPath:
    parts: list[str] = []
    for part in pathlib.PurePosixPath(pattern).parts:
        if any(char in part for char in "*?["):
            break
        parts.append(part)
    return pathlib.PurePosixPath(*parts) if parts else pathlib.PurePosixPath(".")


def map_target(source_pattern: str, target_pattern: str, source_rel: str) -> str:
    source_base = glob_base(source_pattern)
    target_base = glob_base(target_pattern)
    rel_path = pathlib.PurePosixPath(source_rel)
    if source_base == pathlib.PurePosixPath("."):
        suffix = rel_path
    else:
        suffix = rel_path.relative_to(source_base)
    return (target_base / suffix).as_posix()


def expand_sources(source_root: pathlib.Path, source_pattern: str) -> list[str]:
    paths: set[str] = set()
    for path in source_root.glob(source_pattern):
        if path.is_file() and not is_ignored_artifact(path):
            paths.add(normalize(path, source_root))
    return sorted(paths)


def lock_hash_for(lock: dict[str, Any], rel_path: str) -> str:
    value = ((lock.get("managedFiles") or {}).get(rel_path)) if lock else None
    if isinstance(value, str):
        return value
    if isinstance(value, dict):
        return str(value.get("sha256") or "")
    return ""


def classify_state(
    target_exists: bool,
    target_hash: str,
    source_hash: str,
    baseline_hash: str,
    ownership: str,
    role: str,
) -> str:
    if not target_exists:
        return "missing"
    if target_hash == source_hash:
        return "current"
    if baseline_hash and target_hash != baseline_hash:
        return "local modification"
    if ownership == "preserve" and role == "active":
        return "preserved"
    if ownership == "merge" and role == "active":
        return "merge required"
    if not baseline_hash:
        return "local modification"
    return "update available"


def build_statuses(
    source_root: pathlib.Path,
    project_root: pathlib.Path,
    manifest: dict[str, Any],
    lock: dict[str, Any],
) -> list[TargetStatus]:
    statuses: list[TargetStatus] = []
    for artifact in manifest.get("artifacts") or []:
        source_pattern = artifact["source"]
        ownership = str(artifact.get("ownership") or "managed")
        activation = str(artifact.get("activation") or "project")
        install_policy = str(artifact.get("installPolicy") or "copy-if-missing")
        upgrade_policy = str(artifact.get("upgradePolicy") or "safe-managed")
        artifact_type = str(artifact.get("type") or "file")
        reason = str(artifact.get("reason") or "")

        for source_rel in expand_sources(source_root, source_pattern):
            source_path = source_root / source_rel
            source_hash = sha256(source_path)
            for role, target_pattern in (
                ("reference", artifact.get("referenceTarget")),
                ("active", artifact.get("activeTarget")),
            ):
                if not target_pattern:
                    continue
                target_rel = map_target(source_pattern, str(target_pattern), source_rel)
                target_path = project_root / target_rel
                target_exists = target_path.exists()
                target_hash = sha256(target_path) if target_exists else ""
                baseline_hash = lock_hash_for(lock, target_rel)
                target_ownership = "managed" if role == "reference" else ownership
                target_activation = "reference" if role == "reference" else activation
                state = classify_state(
                    target_exists,
                    target_hash,
                    source_hash,
                    baseline_hash,
                    target_ownership,
                    role,
                )
                statuses.append(
                    TargetStatus(
                        artifact_type=artifact_type,
                        role=role,
                        source=source_rel,
                        target=target_rel,
                        ownership=target_ownership,
                        activation=target_activation,
                        install_policy=install_policy,
                        upgrade_policy=upgrade_policy,
                        state=state,
                        source_hash=source_hash,
                        target_hash=target_hash,
                        lock_hash=baseline_hash,
                        reason=reason,
                    )
                )
    return statuses


def load_versions(plugin_root: pathlib.Path, lock: dict[str, Any]) -> tuple[str, str]:
    plugin = read_json(plugin_root / ".codex-plugin/plugin.json", {})
    return str(lock.get("version") or "untracked"), str(plugin.get("version") or "unknown")


def grouped_counts(statuses: list[TargetStatus], role: str) -> list[str]:
    by_state: dict[str, list[TargetStatus]] = {}
    for item in statuses:
        if item.role == role:
            by_state.setdefault(item.state, []).append(item)

    lines: list[str] = []
    for state in ("current", "missing", "update available", "merge required", "local modification", "preserved"):
        values = by_state.get(state, [])
        if not values:
            continue
        lines.append(f"- {state}: {len(values)}")
        for value in values:
            lines.append(
                f"  - `{value.target}` from `{value.source}` "
                f"({value.artifact_type}, {value.ownership})"
            )
    if not lines:
        lines.append("- No files found.")
    return lines


def report(
    current_version: str,
    available_version: str,
    statuses: list[TargetStatus],
    preserve_patterns: list[str],
    lock_exists: bool,
) -> str:
    confirm_required = sorted(
        {
            item.target
            for item in statuses
            if item.role == "active" and item.install_policy == "confirm"
        }
    )
    local_mods = sorted({item.target for item in statuses if item.state == "local modification"})

    lines = [
        "# Boilerplate Upgrade Report",
        "",
        f"- Current target version: `{current_version}`",
        f"- Available plugin version: `{available_version}`",
        f"- Lock file: `{'present' if lock_exists else 'missing'}`",
        "- Activation model: plugin controls install/upgrade only; project files are active at runtime.",
        "",
        "## Reference Baseline",
        "- Location: `.agents/vendor/codex-agent-boilerplate/`",
        *grouped_counts(statuses, "reference"),
        "",
        "## Active Project Files",
        *grouped_counts(statuses, "active"),
        "",
        "## Confirm-Required Active Files",
    ]
    if confirm_required:
        lines.extend(f"- `{path}`" for path in confirm_required)
    else:
        lines.append("- None.")

    lines.extend(["", "## Local Modifications"])
    if local_mods:
        lines.extend(f"- `{path}`" for path in local_mods)
    else:
        lines.append("- None detected.")

    lines.extend(["", "## Preserved Project Context"])
    lines.extend(f"- `{pattern}`" for pattern in preserve_patterns)

    lines.extend(
        [
            "",
            "## Recommended Commit Plan",
            "- Refresh reference baselines first.",
            "- Apply safe active-file updates where the project copy is missing or unchanged.",
            "- Review merge-required files manually, especially hooks and AGENTS.md.",
            "- Preserve backlog, sprint, campaign, retrospective, and product vision history.",
        ]
    )
    return "\n".join(lines) + "\n"


def should_apply(item: TargetStatus, include_confirm: bool) -> bool:
    if item.state not in {"missing", "update available"}:
        return False
    if item.role == "reference":
        return True
    if item.ownership == "preserve":
        return item.state == "missing"
    if item.ownership == "managed":
        return True
    if item.ownership == "merge":
        if item.install_policy == "confirm" and not include_confirm:
            return False
        return item.state == "missing"
    return False


def apply_updates(
    source_root: pathlib.Path,
    project_root: pathlib.Path,
    statuses: list[TargetStatus],
    include_confirm: bool,
) -> list[str]:
    changed: list[str] = []
    for item in statuses:
        if not should_apply(item, include_confirm):
            continue
        source = source_root / item.source
        target = project_root / item.target
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        changed.append(item.target)
    return changed


def update_lock(
    project_root: pathlib.Path,
    plugin_name: str,
    version: str,
    statuses: list[TargetStatus],
    preserve_patterns: list[str],
) -> None:
    managed_files: dict[str, dict[str, str]] = {}
    for item in statuses:
        target = project_root / item.target
        if target.exists():
            managed_files[item.target] = {
                "sha256": sha256(target),
                "sourceVersion": version,
                "source": item.source,
                "role": item.role,
                "activation": item.activation,
                "ownership": item.ownership,
            }

    lock = {
        "schemaVersion": 2,
        "plugin": plugin_name,
        "version": version,
        "syncedAt": dt.date.today().isoformat(),
        "manifest": ".agents/boilerplate-manifest.json",
        "vendorRoot": ".agents/vendor/codex-agent-boilerplate",
        "managedFiles": managed_files,
        "preserved": preserve_patterns,
    }
    write_json(project_root / LOCK_PATH, lock)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("check", "apply"), default="check")
    parser.add_argument("--project-root", default=".", help="Target project root")
    parser.add_argument("--plugin-root", default=None, help="Boilerplate plugin root")
    parser.add_argument(
        "--include-confirm",
        action="store_true",
        help="Install missing active files whose manifest policy requires confirmation.",
    )
    args = parser.parse_args(argv)

    project_root = pathlib.Path(args.project_root).resolve()
    plugin_root = pathlib.Path(args.plugin_root).resolve() if args.plugin_root else plugin_root_from_script()
    source_root = source_root_for(plugin_root)

    manifest = read_json(manifest_path_for(plugin_root, source_root))
    if not manifest:
        raise SystemExit("Missing .agents/boilerplate-manifest.json in plugin payload")

    lock_path = project_root / LOCK_PATH
    lock = read_json(lock_path, {}) or {}
    plugin_info = read_json(plugin_root / ".codex-plugin/plugin.json", {}) or {}
    plugin_name = str(plugin_info.get("name") or manifest.get("plugin", {}).get("name") or "codex-agent-boilerplate")

    preserve_patterns = [entry["glob"] for entry in manifest.get("preserve") or []]
    statuses = build_statuses(source_root, project_root, manifest, lock)
    current_version, available_version = load_versions(plugin_root, lock)

    if args.mode == "apply":
        changed = apply_updates(source_root, project_root, statuses, args.include_confirm)
        update_lock(project_root, plugin_name, available_version, statuses, preserve_patterns)
        print(f"Applied {len(changed)} file update(s).")
        for path in changed:
            print(f"- {path}")

        skipped = sorted(
            {
                item.target
                for item in statuses
                if item.state in {"local modification", "merge required"}
                or (item.role == "active" and item.install_policy == "confirm" and not args.include_confirm)
            }
        )
        if skipped:
            print("\nSkipped or review-required files:")
            for path in skipped:
                print(f"- {path}")
        return 0

    print(
        report(
            current_version,
            available_version,
            statuses,
            preserve_patterns,
            lock_path.exists(),
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
