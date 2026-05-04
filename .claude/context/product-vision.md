---
name: MetaVisage Product Vision
description: Strategic vision, target users, and current state for MetaVisage — the MetaHuman mesh morphing desktop application.
---

# Product Vision

**Project**: MetaVisage
**Last Updated**: 2026-05-04

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
- **Sprint 4 (Wrap Engine Redesign — Algorithm & Preset System) complete** as of 2026-04-29, 22/27 velocity, merged to `development` via [PR #22](https://github.com/cmershon2/MetaVisage/pull/22). RBFInterpolator and the legacy RBF / TPS / Gaussian / Multiquadric / ARAP enum members removed (~350 LoC of dead code gone); user-visible algorithm strings rebranded to "Wrap Engine"; v1.0.0 `.mmproj` files load without warning via silent enum coercion (regression-tested by the new `smoke_v1_backward_compat` CTest). New `core/WrapPreset` data model + `io/PresetStore` persistence layer (`%APPDATA%\MetaVisage\presets\<id>.mvpreset.json`, `schema_version: 1`). Two read-only built-ins (Quick Wrap = defaults + γ=1.0, Fantasy Wrap = defaults) iterate from `WrapPreset::BuiltIns()`; user-saved presets render after the built-ins with a right-click context menu (Rename / Replace / Delete). New Save / Rename modal with a 12-glyph Lucide picker. Behavioural-parity smoke (`smoke_wrap_parity`) catches the bug class Sprint 3.8.9 first shipped. `/sprint-retrospective` skill mechanically gates on the DoD checklist (Story 4.7). Story 4.5 (`imgui_test_engine` smoke-UI lane) deferred to Sprint 5 with Path A confirmed.
- **Sprint 5 (Advanced Parameters, Cottage-Cheese Mitigation, Stage 4 Polish & smoke_ui Lane) complete** as of 2026-04-30, 22/22 velocity (100% on commitment), open against `development` via [PR #24](https://github.com/cmershon2/MetaVisage/pull/24) with all `/review-pr` non-blocking nits addressed, Phase D manual UI smoke logged, and [retrospective](./retrospectives/retro-sprint-05.md) closed (zero open carryover from previous sprints — first time in five sprints). Five stories shipped: (5.1) Stage 3 inspector restructured into a hierarchical collapsible parameter tree — General with Interior Geometry sub-section, Advanced (default-collapsed) with Optimization / Rigidity / Control Node Sampling sub-sections, Vertex Mask promoted to top, Reset to Defaults always-visible; collapsed/expanded state persists in `%APPDATA%\MetaVisage\config.json` under `ui_state.collapsed_sections`; new `MvInspectorPrimitives::DrawSubSectionLabel` primitive. (5.2) Cottage-cheese-face Outcome B stopgap — amber inspector warning when Landmark Weight / Stiffness (end) > 0.5 plus a per-vertex residual CSV dump gated by `METAVISAGE_DUMP_RESIDUALS=1`; root-cause spike filed as a High-priority backlog item with three audit-flagged hypothesis sites named. (5.3) Stage 4 brush selector polished end-to-end — brush-switch hotkeys 1/2/3/4 in `Viewport::OnKey`, `[N]` discoverability badges in card sub-labels, PNG brush previews authored to MetaVisage palette specs and wired through `MvStyle::Asset.BrushPreviewTextureIds` into both the inspector cards AND the Stage 4 ToolRail buttons (active state switches to MvBlue500 border ring so the PNG's blue arrow stays legible). (5.4) `imgui_test_engine` smoke-UI lane finally landed after a four-retro carry — `vcpkg.json` declares `imgui[test-engine]`, `AppWindow::TickOnce()` extracted from `Run()`'s body, viewport toolbar window ID stabilized to `##VpToolbar_<role>`, new `tests/smoke/ui_smoke.cpp` with 3 scenarios (welcome modal dismiss / dockspace window-existence / viewport shading-mode toggles) running in 2.13s under `ctest -L ui`. (5.5) `/sprint-retrospective` Clause B closes the bypass case Sprint 4's retro surfaced — gate now also fires when all stories are `[x]` but Status is left at `Planned`/`In Progress` with validation DoD items unchecked.
- **Sprint 6 (Licensing Infrastructure — Core Integration) complete** as of 2026-05-04, 26/26 velocity (100% on commitment), open against `development` via [PR #25](https://github.com/cmershon2/MetaVisage/pull/25) with the `/review-pr` blocking finding closed (`StartRefreshThread` joinable-guard fix in commit `104b809`) and 3 of 6 non-blocking findings addressed inline (the other 3 explicitly tracked to the backlogged License Status redesign). Seven stories shipped: (6.1) `LicenseManager` singleton + `Unauthenticated`/`Authenticating`/`Verifying`/`Authenticated`/`Expired`/`Inactive` state machine; LoginScreen promoted from Sprint 3.8.12 dev preview to mandatory gate (Sign Out / Esc dev escape hatches removed, `Help > Login Screen (Dev)` toggle gone); compile-time licensing config baked at configure time via `cmake/LicenseConfig.h.in` from `.env.<profile>`. (6.2) BCryptGenRandom 32-byte hex state token + `OpenUrl` browser launch + `HKCU\Software\Classes\metavisage` first-run scheme registration + cold-launch + already-running deep-link interception (named-pipe forwarding via existing `SingleInstance`) + 5-min timeout watchdog + URI parser tolerating Windows / Chromium `auth/` host normalization. (6.3) Worker-thread `/api/desktop/exchange` POST with retry budget (≤4 attempts, ≤30s wall-clock, 429 honors Retry-After, 5xx exponential backoff) + RS256 JWT verification via `jwt-cpp` with the nlohmann_json traits header (`iss=metavisage-web`, `aud=metavisage-desktop`, 60s clock-skew leeway) + DPAPI-encrypted token persistence via `Config::SetTokens` / `SetLicenseJwt` + cold-launch fast-path verification. New vcpkg deps: `jwt-cpp` + `openssl` (libcurl on Windows ships Schannel so the OpenSSL backend gets nothing transitively). (6.4) 24h `cv::wait_for` refresh thread with `NotifyRefreshNow` early-wake; Supabase direct refresh (`{SUPABASE_URL}/auth/v1/token?grant_type=refresh_token` per QA Round 2 Decision 2 — Option A); 403/404 → `Inactive`; 401 + invalid refresh-token → re-auth; transport-fail kept silent for the offline grace path; tier/license_key/conversions parsed from JWT payload (Sprint 6 follow-up); `RequestRefresh()` exposed for the deferred manual-refresh button. (6.5) Amber `Offline` status-bar pill on transport-fail when the cached JWT is still within `exp`; clears silently on the next successful refresh; expired-cache blocking modal deliberately deferred to a follow-up backlog item per Story 6.5 commit body. (6.6) `LicenseModalState` + `RenderLicenseModal` rendering account email + tier badge + `Online`/`Offline`/`Inactive` validation status + conversions-used / limit / reset date; `Manage Subscription` opens `/manage-subscription` (the portal's Stripe billing-portal route handler per QA Round 2 Decision 8); `Sign Out` confirmation modal → `LicenseManager::SignOut`; status-bar `Tier FREE/INDIE/PRO` chip. (6.7) `login_screen_cycle` scenario in `smoke_ui` exercising `Sign In → Cancel`; `LicenseManager::SetTestState` enables hermetic test mode (suppresses browser launch + HKCU writes). Closes Sprint 5 retro AI #1 (Login Screen portion); Stage Stepper portion deferred to Sprint 7 alongside Story 7.x stage-progression integration. **Late UI/UX feedback** added a `Verifying` auth-state intermediate (bridges deep-link arrival → exchange completion so the 1-2s loading window can't be interrupted by a Retry click), an `AllowSetForegroundWindow(ASFW_ANY)` from the second-instance side to defeat Windows' foreground-lock when bringing the running first instance forward, a confirm-modal centering fix (`Cond_Always` instead of `Cond_Appearing` because `AlwaysAutoResize` makes the popup size unknown on the first frame), and a Retry-button fix (Cancel-then-Begin so the in-flight state token is cleared before the fresh launch). Smoke 4/4 green throughout (`smoke_ui` runs `welcome_modal_dismiss` / `dockspace_window_smoke` / `viewport_shading_modes` / `login_screen_cycle` in ~3s). Backlog: cottage-cheese-face root-cause spike (deferred deliberately, carries to Sprint 7), expired-cache blocking modal, License Status modal redesign + title-bar account chip per UI/UX feedback (Medium Priority, ~325 LoC, every data hook plumbed).
- **Sprint 7** is next — Licensing Infrastructure: Feature Gating & Conversion Tracking per the master `SPRINT_PLAN.md`. Carryover into Sprint 7: cottage-cheese-face root-cause spike (Sprint 5 retro AI #2 → Sprint 6 → Sprint 7), Stage Stepper `smoke_ui` scenario (Sprint 5 retro AI #1 partial), and the broader Stage 1 → 2 → 3 → 4 → Export Phase D pass that wasn't part of Sprint 6's licensing-only scope.
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
