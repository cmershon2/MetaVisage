---
name: MetaVisage Product Vision
description: Strategic vision, target users, and current state for MetaVisage — the MetaHuman mesh morphing desktop application.
---

# Product Vision

**Project**: MetaVisage
**Last Updated**: 2026-04-28

---

## Vision Statement

MetaVisage is a Windows desktop application that automates wrapping MetaHuman head meshes to custom head models for Unreal Engine, giving indie developers and small studios a high-quality alternative to expensive enterprise mesh-wrapping tools.

---

## Target Users

- **Indie Game Developers** — solo developers and small teams creating custom characters
- **3D Artists** — mid to expert-level artists working with Unreal Engine MetaHumans
- **Virtual Production Studios** — small studios needing custom digital humans for 1–10 characters per project

Users are intermediate-to-expert 3D artists, comfortable with Blender/Maya workflows and familiar with the MetaHuman pipeline in Unreal Engine 5.x.

---

## Core Value Proposition

Commercial mesh-wrapping solutions (e.g., Faceform's Wrapped) cost ~$500/month and are out of reach for indie teams. MetaVisage delivers comparable deformation quality through a guided four-stage workflow (Alignment → Point Reference → Morph → Touch Up) with Blender-style viewport controls, Unreal-ready FBX export, and a transparent pricing model that keeps custom MetaHuman creation accessible.

---

## Strategic Goals

1. **Quality parity** — produce morph results indistinguishable from commercial tools for standard MetaHuman workflows; preserve topology, UVs, and normals end-to-end.
2. **Time to result** — an experienced 3D artist can complete a full morph-and-export workflow in under 30 minutes.
3. **Performant viewport** — maintain 30+ FPS for meshes under 100k vertices on the target hardware baseline.
4. **Sustainable commercial model** — ship v2.0 as a paid product with a licensing/subscription system that funds ongoing development while keeping a viable free tier.
5. **Frictionless distribution** — every user receives updates automatically via WinSparkle and installs via a single Inno Setup installer.

---

## Current State

- **v1.0.0** shipped under MIT license on Qt 6 with the four-stage workflow, FBX/OBJ/GLTF import-export, point correspondence with symmetry, RBF/TPS-based deformation, and basic sculpting brushes.
- **v2.0 in progress** on branch `sprint/Framework-Migration-UI-Layer`. The scope:
  - Address all v1.0.0 user feedback (see `.claude/context/Version 1.0.0 Feedback.md`)
  - Migrate off Qt onto Dear ImGui + GLFW to eliminate Qt commercial-license cost (Sprints 3.5 and 3.6)
  - Replace RBF with the new wrapping algorithm; introduce Quick Wrap / Fantasy Wrap presets (Sprint 4)
  - Add licensing infrastructure, tier enforcement, Inno Setup installer, and WinSparkle auto-update
- Sprints 1–3 complete.
- **Sprint 3.5 (Platform Layer migration) complete** as of 2026-04-21 — Qt Gui/Widgets/OpenGL/OpenGLWidgets removed, app re-hosted on GLFW + Dear ImGui + GLEW with camera/input parity. `Qt6::Core` retained transitionally (LGPL; full removal tracked in the product backlog).
- **Sprint 3.6 (UI Layer migration) complete** as of 2026-04-26 — full ImGui application shell (dockspace, menu bar, status bar, all four stage sidebars), every modal (Welcome, Confirm, Error, Export, Texture Preview, Shortcuts, About), NFD-backed file pickers, JSON config at `%APPDATA%\MetaVisage\config.json`, named-pipe single-instance + deep-link forwarding, libcurl HTTP client, and `MorphWorker` ported off `QThread`. Stage 1 → 2 → 3 → 4 → Export workflow runs end-to-end on the new stack. Story 3.6.5 (shading-mode overlay & orientation gizmo ImGui port) landed organically through PR-feedback cycles.
- **Sprint 3.7 (Qt Decoupling & Deferred Cleanup) complete** as of 2026-04-26, 26/26 velocity, merged to `development` via [PR #20](https://github.com/cmershon2/MetaVisage/pull/20). `Qt6::Core` removed entirely (QString/QFile/QJson/QDateTime/QFileInfo/QDir → std equivalents across core, utils, rendering, io, baking, deformation, ui); dead Qt-era UI files deleted. C++17 → C++20 bump for `std::format`. ProjectSerializer migrated to `nlohmann::ordered_json`. All four Sprint 3.6 `/review-pr` blockers closed. Sprint 3.5 deferred items landed (CMake configure-time Qt-purity guard + headless smoke-test fixture). Undo/Redo re-wired at every mutation site. `MetaVisage.exe` ships with zero Qt DLLs in its dependency chain.
- **Sprint 3.8 (UI/UX Redesign Implementation) complete** as of 2026-04-28, 32/32 velocity, 12/12 stories shipped on branch `sprint/UI-UX-Redesign-Implementation`. Full visual overhaul against the design team's HTML mockups: design-token system (`MvStyle` — palette, spacing, radii, shadows, eight font handles); Lucide icon font adopted mid-sprint (`MvIcons`, ~280 lines of hand-drawn ImDrawList primitives deleted from ToolRail); shared inspector primitives (`MvInspectorPrimitives` with `DragEditResult` struct for one-undo-per-gesture); custom title bar + breadcrumb + caption buttons; redesigned menu bar; `StageStepper` (4-step pill stepper with Lucide checks); status bar with Lucide chips; `ToolRail` (stage-aware: Stages 1+4); all four stage sidebars rewritten using primitives (per-row delete, palette badges, Stage 4 brush row uses ToolRail); viewport overlays (top-center toolbar with shading-mode toggles, top-right pills, bottom-left instruction HUD, Stage 3 result-stats overlay, Stage 4 mesh-stats overlay); Welcome modal redesign (header + actions + recent-list table + footer); full-screen `LoginScreen` (no offline path — sign-in required at least once per session); frameless OS window via Win32 `WM_NCHITTEST`/`WM_NCCALCSIZE` subclass (Aero Snap + edge resize + system menu preserved). Mid-sprint scope changes: dropped Stage2InfoBar in favor of a point list; Stage 4 promoted to use ToolRail; window mode promoted to fully-frameless (Option C). Three-state morph status (`Ready`/`Processing`/`Complete`) tracked in Viewport. All NRICP parameter labels and defaults preserved per CLAUDE.md (Quick Wrap = defaults + rigidity 1.0, Fantasy Wrap = defaults). Smoke green every commit.
- **Sprint 4 (Wrap Engine Redesign)** is next per `SPRINT_PLAN.md` — the algorithm rework, RBF/TPS removal, and Quick Wrap / Fantasy Wrap preset cards. Sprints 3.7 and 3.8 met the preconditions: reliable Undo/Redo, zero-Qt baseline, and the design-system foundation the preset cards will sit on.
- v2.0 transitions the product from open-source MIT to proprietary/closed-source.

---

## Key Constraints

- **Platform**: Windows 10/11 64-bit only for v2.0. Cross-platform is post-MVP.
- **Framework budget**: Cannot pay for a Qt commercial license — the Dear ImGui + GLFW migration is non-negotiable before v2.0 ship.
- **Unreal compatibility**: Exports must load cleanly in UE 5.x; breaking this blocks the release regardless of other progress.
- **Mesh data integrity**: Topology, UVs, and normals must survive the full pipeline unmodified — any regression here is treated as a P0 bug.
- **Algorithm secrecy**: NRICP is the core differentiator for v2.0; surface-facing copy, logs, and UI text must not reveal the algorithm by name.
- **External dependencies**: Licensing/auth flows depend on the web team's API contract — see `.claude/context/DESKTOP_APP_INTEGRATION.md`, `DESKTOP_AUTH_FLOW.md`, and `DESKTOP_UPDATE_FLOW.md`.

---

## Related Documents

- [PRD](./PRD.md) — full v1.0 product requirements
- [v1.0.0 Feedback](./Version 1.0.0 Feedback.md) — user feedback driving v2.0 scope
- [Sprint Plan](./sprints/SPRINT_PLAN.md) — aggregated v2.0 sprint plan
- [Desktop App Integration](./DESKTOP_APP_INTEGRATION.md) — licensing API contract
- [Desktop Auth Flow](./DESKTOP_AUTH_FLOW.md) — browser OAuth spec
- [Desktop Update Flow](./DESKTOP_UPDATE_FLOW.md) — WinSparkle update flow
