#!/usr/bin/env python3
"""
PreToolUse hook — scan staged changes for secrets before git commit.
Fires before any `git commit` command. Exits 0 with a warning in stdout
(context injected for Claude) rather than hard-blocking, since pattern
matches can be false positives. Claude will see the warning and confirm
with the user before proceeding.
"""
import json
import re
import subprocess
import sys

# Patterns: (regex, label)
SECRET_PATTERNS = [
    (r'(?i)(api[_-]?key|apikey)\s*[:=]\s*["\']?[A-Za-z0-9_\-]{20,}', "API key"),
    (r'(?i)(secret[_-]?key|client_secret)\s*[:=]\s*["\']?[A-Za-z0-9_\-]{16,}', "Secret key"),
    (r'(?i)(password|passwd)\s*[:=]\s*["\']?[^\s$\'\"]{8,}', "Password"),
    (r'(?i)(access_token|auth_token|bearer)\s*[:=]\s*["\']?[A-Za-z0-9_\-\.]{16,}', "Auth token"),
    (r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----', "Private key"),
    (r'(?:sk|pk)-[A-Za-z0-9]{32,}', "OpenAI/Stripe/Anthropic key"),
    (r'ghp_[A-Za-z0-9]{36}', "GitHub personal access token"),
    (r'AKIA[0-9A-Z]{16}', "AWS access key ID"),
    (r'(?i)database_url\s*[:=]\s*["\']?[a-z]+://[^@\s]+:[^@\s]+@', "Database URL with credentials"),
]

data = json.load(sys.stdin)
command = data.get("input", {}).get("command", "")

if not re.search(r'\bgit commit\b', command):
    sys.exit(0)

try:
    result = subprocess.run(
        ["git", "diff", "--cached", "--unified=0"],
        capture_output=True, text=True, timeout=10
    )
    diff = result.stdout
except Exception:
    sys.exit(0)

if not diff:
    sys.exit(0)

findings = []
for line in diff.splitlines():
    # Only check added lines, skip diff headers
    if not line.startswith("+") or line.startswith("+++"):
        continue
    for pattern, label in SECRET_PATTERNS:
        if re.search(pattern, line):
            findings.append(label)
            break  # One finding per line is enough

if findings:
    unique = list(dict.fromkeys(findings))  # Deduplicate, preserve order
    labels = ", ".join(unique)
    # Exit 0 — advisory only. stdout is injected into Claude's context.
    # Claude will surface this to the user before proceeding.
    print(
        f"⚠️  Secret scan: possible {labels} detected in staged changes.\n"
        f"Review the diff before committing. If these are false positives "
        f"(e.g. example values, test fixtures), it is safe to proceed."
    )

sys.exit(0)
