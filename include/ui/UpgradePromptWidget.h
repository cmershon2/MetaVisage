#ifndef METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
#define METAVISAGE_UI_UPGRADEPROMPTWIDGET_H

#include <string_view>

#include <imgui.h>

namespace MetaVisage::ui {

// Sprint 7 Story 7.3 (round-7 redesign) -- premium-feature gating widgets.
//
// The gated-section pattern (see Begin/EndGatedSectionHover + DrawUpgrade
// Button below) greys out a sidebar section, marks its title with a zap
// glyph, and renders an amber Upgrade CTA below the disabled content;
// hovering anywhere in the section's vertical range opens a rich side-
// panel card with the feature's hero image + description + tier pill.
//
// DrawUpgradePromptTooltip remains as a disabled-control hover variant
// for Stage 4 brush cards, where each card carries its own per-brush
// preview.
//
// Both surfaces are themed amber (MvStyle::MvWarning) with the Lucide
// zap glyph (MV_ICON_ZAP) as the brand-consistent gate indicator.

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

// Sprint 7 round-7 -- gated-section pattern (replaces the inline amber
// pill). Caller layout, in order:
//   1. MvInspector::DrawSectionLabel(title, /*gated=*/true)  -- zap-marked title
//   2. auto scope = ui::BeginGatedSectionHover(copy);
//      ImGui::BeginDisabled(true);
//      ... render the section's gated content (collapsibles + sliders + etc) ...
//      ImGui::EndDisabled();
//   3. ui::DrawUpgradeButton(copy);  -- full-width amber CTA
//   4. ui::EndGatedSectionHover(scope);  -- tests hover across the captured rect
//
// The hover-scope captures the section's vertical range, so anywhere in
// the gated block (label-to-button) shows the rich hover-card on
// hover. The card content comes from copy; the card itself opens to the
// LEFT of the sidebar.

struct GatedSectionScope {
    float topY = 0.0f;
    float sidebarLeftX = 0.0f;
    UpgradePromptCopy copy;
    bool active = false;
};

GatedSectionScope BeginGatedSectionHover(const UpgradePromptCopy& copy);
void EndGatedSectionHover(const GatedSectionScope& scope);

// Full-width amber rounded button with the zap glyph trailing on the
// right (inset darker amber square). Click opens the pricing page.
// Visually matches the Sprint 7 round-7 mockup.
void DrawUpgradeButton(const UpgradePromptCopy& copy);

// Disabled-control tooltip variant -- still used by Stage 4 brush cards
// where each disabled brush gets its own hover card.
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy);

}  // namespace MetaVisage::ui

#endif  // METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
