#ifndef METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
#define METAVISAGE_UI_UPGRADEPROMPTWIDGET_H

#include <string_view>

#include <imgui.h>

namespace MetaVisage::ui {

// Sprint 7 Story 7.3 -- reusable Premium Feature callout for tier-gated UI.
//
// Two variants share the same copy struct so a tier-rename (Indie -> Studio,
// etc.) is a single-line change at the call site:
//
//   DrawUpgradePrompt          -- rounded amber callout rendered inline in
//                                 a sidebar section after a hidden / disabled
//                                 control. Visual matches the Starlight
//                                 "Caution" aside (rounded box, amber border,
//                                 icon + title row, body text, upgrade link).
//   DrawUpgradePromptTooltip   -- richer hover card variant; called
//                                 immediately after a disabled ImGui
//                                 control. Internally checks IsItemHovered
//                                 with the AllowWhenDisabled flag so it
//                                 works through BeginDisabled scopes.
//                                 Renders a side-popup with a large icon
//                                 header + feature name + description.
//
// Both variants are themed amber (MvStyle::MvWarning) with the Lucide zap
// glyph (MV_ICON_ZAP) marking the call-out as a premium-feature gate. A
// clickable "Upgrade -> metavisage.io" link opens the public pricing page
// in the system browser.

struct UpgradePromptCopy {
    std::string_view feature;        // e.g. "Vertex masking", "Custom MetaHuman head upload"
    std::string_view minimumTier;    // e.g. "Indie", "Pro"
    // Optional short body describing what the feature unlocks. Rendered in
    // the inline callout and the hover card; empty falls back to the
    // generic "Available on the <Tier> plan" line.
    std::string_view description = {};
    // Optional glyph for the hover card's image header when no heroImage
    // is supplied. Empty falls back to the zap glyph. Pass an MV_ICON_*
    // constant string.
    std::string_view featureGlyph = {};
    // Optional GL texture handle (round-tripped through ImTextureID) for a
    // hero image rendered at the top of the hover card. Recommended source
    // is a 320x180 PNG (16:9, ~50 KB). When zero, the card falls back to a
    // gradient header strip with the feature glyph.
    ImTextureID heroImage = 0;
};

void DrawUpgradePrompt(const UpgradePromptCopy& copy);
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy);

}  // namespace MetaVisage::ui

#endif  // METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
