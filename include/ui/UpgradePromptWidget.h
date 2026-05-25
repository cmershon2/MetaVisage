#ifndef METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
#define METAVISAGE_UI_UPGRADEPROMPTWIDGET_H

#include <string_view>

#include <imgui.h>

namespace MetaVisage::ui {

// Sprint 7 Story 7.3 -- premium-feature gating widgets.
//
// The UI/UX pass moved paid gates away from large amber CTA blocks. Locked
// features now stay quiet at rest: the disabled control shows a tier badge
// and diagonal hatch treatment, while hover reveals a compact card with the
// required tier, a short explanation, and the pricing link.

enum class UpgradePromptKind {
    Feature,
    Brush,
};

struct UpgradePromptCopy {
    std::string_view feature;        // e.g. "Vertex masking", "Custom MetaHuman head upload"
    std::string_view minimumTier;    // e.g. "Indie", "Pro"
    // Optional short body describing what the feature unlocks. Rendered in
    // the inline callout and the hover card; empty falls back to the
    // generic "Available on the <Tier> plan" line.
    std::string_view description = {};
    // Optional glyph for the hover card. Empty falls back to the lock glyph.
    std::string_view featureGlyph = {};
    UpgradePromptKind kind = UpgradePromptKind::Feature;
};

struct GatedSectionScope {
    float topY = 0.0f;
    float sidebarLeftX = 0.0f;
    UpgradePromptCopy copy;
    bool active = false;
};

GatedSectionScope BeginGatedSectionHover(const UpgradePromptCopy& copy);
void EndGatedSectionHover(const GatedSectionScope& scope);

// Compact locked row used by standard feature gates in the inspector. Click
// or hover the row to reveal the card; click opens the pricing page.
void DrawLockedFeatureField(const char* label,
                            const char* value,
                            const UpgradePromptCopy& copy,
                            const char* rightText = nullptr);

// Disabled-control hover variant for brush tiles / rail buttons.
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy);

}  // namespace MetaVisage::ui

#endif  // METAVISAGE_UI_UPGRADEPROMPTWIDGET_H
