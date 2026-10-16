---
name: campaign-readiness
description: Run a safe campaign-start readiness preflight for permissions, auth, MCP/app connectors, CLIs, environment variables, and external access needed by Campaign Autopilot.
---

# Campaign Readiness

Use this skill before Campaign Autopilot begins execution, and refresh it when a
campaign discovers new external access needs.

Campaign readiness is a front-loaded preflight, not a per-command gate. It helps
Codex run uninterrupted by recording which permissions, auth, connectors,
secrets, tools, and external services are available, missing, deferred, or out
of scope.

Readiness also checks whether the campaign has enough quality specification to
avoid turning subjective requirements into agent guesswork. For UI-heavy,
integration-heavy, or navigation-heavy campaigns, record whether the project has
mechanical guidance for design, navigation, loading states, live smoke tests,
screenshots, rendered image checks, and link audits before approving autonomy.
When `impeccable` is installed, it is the preferred companion for design
context, critique, audit, and polish on UI-heavy campaigns.

## Files

Readiness state lives in:

```text
.agents/context/campaigns/readiness-{campaign-slug}.md
```

Use `.agents/context/campaigns/readiness-template.md` for new readiness files.

## Inputs

Infer required access from:

- The campaign mandate, PRD, roadmap, or sprint series.
- Product backlog and product vision.
- Existing sprint plans and latest handoff.
- Repo manifests, docs, `.env.example`, deployment config, and test/run
  commands.

## Safe Checks

Use only safe, read-only probes. Never request, display, or store secret values.
Record only whether a variable, credential, connector, or command appears
available.

Check these categories when relevant:

- Local repo and filesystem access.
- Git branch, remotes, and clean/dirty worktree state.
- GitHub CLI/API auth for issues, PRs, releases, or repo metadata.
- Package manager and network install access.
- Browser automation or local dev server needs.
- Required env vars from `.env.example`, docs, deployment config, or PRD.
- Databases, cloud providers, deployment targets, billing-sensitive services.
- MCP/app connectors such as GitHub, Slack, Drive, Notion, Linear, email,
  calendar, or project-specific servers.
- Design system or style guide for taste-heavy UI work, including tokens,
  component primitives, visual references, and explicit anti-patterns.
- Impeccable readiness for UI-heavy work: whether `impeccable` is installed or
  enabled separately, whether `PRODUCT.md` / `DESIGN.md` exist, and whether
  `$impeccable teach`, `$impeccable document`, `$impeccable critique`, or
  `$impeccable audit` should run before or during the campaign.
- Navigation map or route inventory for route-heavy work, including expected
  anonymous, authenticated, and partial-state behavior.
- Loading, empty, and error-state guidance for async or data-heavy UI work.
- Live smoke-test needs for external APIs, webhooks, cloud storage, auth,
  payment, deployment, search, email, or other runtime integrations.
- Screenshot or visual audit strategy for user-facing UI work.
- Rendered image checks for pages whose value depends on images appearing from
  source CDNs or storage.
- Broken-link audit strategy for campaigns that add or change routes, nav, or
  public pages.

## Status Rules

- `Ready`: all critical capabilities are available or not required.
- `Ready with gaps`: optional capabilities are missing, or critical work has a
  documented local workaround.
- `Blocked`: critical access is missing and no safe workaround exists.

Missing optional access must not block the campaign. Missing critical access
should block execution until the user connects or configures it.

Missing quality specifications follow the same rule: optional gaps may be
recorded as `Ready with gaps`, but critical gaps that would force Codex to make
high-impact taste, navigation, or verification decisions should mark readiness
as `Blocked` until the project provides guidance or explicitly defers the gate.
Missing Impeccable installation alone is not a blocker. Treat it as a blocker
only when the campaign depends on high-impact UI judgment and there is no other
approved design guidance.

## Output

Create or update the readiness file with:

- Campaign name, date, status, owner, and final decision.
- Required capabilities table.
- Auth/connectors table.
- Environment and secrets table.
- Quality specification readiness table.
- Deferred items and workarounds.
- Clear next actions for missing critical access.

After updating readiness, update the linked campaign file's `Readiness` and
`Readiness Check` fields.
