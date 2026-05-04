#ifndef METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
#define METAVISAGE_UI_UPGRADEPROMPTWIDGET_H

#include <string_view>

namespace MetaVisage::ui {

// Sprint 7 Story 7.3 — reusable inline upgrade prompt for tier-gated UI.
//
// Two variants share the same copy struct so a tier-rename (Indie → Studio,
// etc.) is a single-line change at the call site:
//
//   DrawUpgradePrompt          — amber inline pill rendered in a sidebar
//                                section after a hidden / disabled control
//   DrawUpgradePromptTooltip   — hover tooltip variant; called immediately
//                                after a disabled ImGui control. Internally
//                                checks IsItemHovered with the
//                                AllowWhenDisabled flag so it works through
//                                BeginDisabled scopes.
//
// Both variants render a Lucide lock glyph + the body copy + a clickable
// "Upgrade at metavisage.io" link that opens the public pricing page in the
// system browser.

struct UpgradePromptCopy {
    std::string_view feature;       // e.g. "Vertex masking", "Custom mesh upload"
    std::string_view minimumTier;   // e.g. "Indie", "Pro"
};

void DrawUpgradePrompt(const UpgradePromptCopy& copy);
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy);

}  // namespace MetaVisage::ui

#endif  // METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
