---
name: Sprint 6 Story 6.6 Pre-Implementation Audit
description: License Status modal + status bar tier badge — modal pattern, ConfirmModal API, DrawSectionLabel, status bar right cluster, dispatch sites, available glyphs, OpenUrl
type: project
---

## Modal pattern (ShortcutsModal is the simplest template)

- `ShortcutsModalState::Open()` at Modals.cpp:849 — sets `pendingOpen = true` only
- `RenderShortcutsModal(state)` at Modals.cpp:851 calls `HandlePendingOpen("Keyboard Shortcuts", state.pendingOpen)` which calls `ImGui::OpenPopup` + clears flag (Modals.cpp:19-24)
- `ImGui::BeginPopupModal("Keyboard Shortcuts", nullptr, ImGuiWindowFlags_NoSavedSettings)` gates the body
- nullptr p_open = no X button; click-outside and Escape still close (ImGui default)
- Centered via `SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, {0.5,0.5})` + `SetNextWindowSize(..., ImGuiCond_Appearing)`
- Explicit close only: `ImGui::Button("Close") -> ImGui::CloseCurrentPopup()` at line 910
- Called unconditionally from `AppWindow::RenderModals()` at AppWindow.cpp:1899
- No special PushStyleColor wrappers — relies on global MvStyle::Apply() theme

## ConfirmModalState API (for Sign Out confirmation)

- `ConfirmModalState::Open(title, message, std::function<void()> confirm, confirmLbl, cancelLbl)` at Modals.cpp:40
- Confirm fires `onConfirm()`, clears it, calls CloseCurrentPopup (lines 61-63)
- Cancel clears `onConfirm` and closes (lines 65-67)
- No return value — callback IS the result mechanism
- Call: `confirmModal_.Open("Sign Out", "...", [this]{ LicenseManager::Instance().SignOut(); }, "Sign Out", "Cancel")`

## DrawSectionLabel

- Signature at MvInspectorPrimitives.h:40: `void DrawSectionLabel(const char* title)`
- Namespace alias `I` = `MetaVisage::MvInspector`, typical call: `I::DrawSectionLabel("SOURCE MESH")`
- Draws: MvLine2 hairline above + 8px padding + 6x6 MvBlue500 square dot + 10px MonoSm uppercase title in MvFg3

## Status bar right-aligned cluster (AppWindow.cpp:1642-1654)

- Currently contains ONLY the GPU label: `std::format("GPU {}", gpuRenderer_)`
- Right cluster math: `rightCluster = gpuW + SidePad; rightStartX = startCursor.x + fullW - rightCluster`
- There is NO existing Save indicator / lastSavedAt_ chip in the status bar — the comment at 1642 says it was dropped
- Tier badge belongs in the right cluster alongside GPU label; increase rightCluster width accordingly
- Inline semantic coloring uses flat `drawSection(text, color)` lambda — no rounded-rect pill primitive exists in status bar today
- Established pill rounding across the codebase is 6.0f (StageStepper disc rects, Modals card rects)
- Offline segment at lines 1607-1611 uses MvStyle::MvWarning for color — same pattern for badge semantic colors

## OpenAccountLicense dispatch sites (only 2)

- AppWindow.cpp:1509 — `ImGui::MenuItem("Account & License")` sets `pendingAction_ = PendingAction::OpenAccountLicense`
- AppWindow.cpp:1733-1737 — switch case calls `OpenUrl("https://metavisage.io/account")` — REPLACE this with `licenseModal_.Open()`
- No other callers. Add `LicenseModalState licenseModal_` to AppWindow and a `RenderLicenseModal` call in `RenderModals()` at AppWindow.cpp:1894

## Available Lucide glyphs for tier icons (MvIcons.h — full list)

- `MV_ICON_SPARKLES` (U+E412) — best existing candidate for premium/Pro badge
- `MV_ICON_LOCK` (U+E10B) — locked/restricted tier
- `MV_ICON_CHECK_CIRCLE` (U+E226) — active/validated status
- `MV_ICON_ALERT` (U+E193) — Inactive/Expired states
- NO crown, shield, star, gem, or award glyphs exist — must add new MV_ICON_* constant from assets/fonts/info.json if needed

## OpenUrl for Manage Subscription button

- `OpenUrl::Open` whitelist (OpenUrl.cpp:29-33): https://, http://, mailto: — `https://metavisage.io/manage-subscription` passes without special handling
- Wrap: `if (!OpenUrl(...)) { errorModal_.Open("..."); }` per AppWindow.cpp:1729-1731 pattern
