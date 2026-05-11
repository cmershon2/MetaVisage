#include "ui/UpgradePromptWidget.h"

#include <format>
#include <string>

#include <imgui.h>

#include "licensing/LicenseConfig.h"
#include "ui/MvIcons.h"
#include "ui/MvStyle.h"
#include "utils/OpenUrl.h"

namespace MetaVisage::ui {

namespace {

// "Upgrade at metavisage.io" link target. The pricing page is the public
// upgrade entry point -- distinct from /manage-subscription which is the
// post-purchase Stripe portal (already used by the License Status modal).
constexpr const char* kUpgradePathSuffix = "/pricing";
constexpr const char* kArrowRight = "\xE2\x86\x92";  // U+2192

void OpenPricingPage() {
    const std::string url =
        std::string(MetaVisage::LicenseConfig::kApiBaseUrl) + kUpgradePathSuffix;
    MetaVisage::OpenUrl(url);
}

// Build the body line. If the call site provided a description, use it
// verbatim; otherwise fall back to a generic "Available on the <Tier> plan"
// so older call sites keep working.
std::string BuildBody(const UpgradePromptCopy& copy) {
    if (!copy.description.empty()) {
        return std::string(copy.description);
    }
    return std::format("{} -- available on the {} plan.",
                       std::string(copy.feature),
                       std::string(copy.minimumTier));
}

// MvStyle::MvWarning at a low alpha for the callout background fill.
ImVec4 WarnFill(float alpha) {
    return ImVec4(MvStyle::MvWarning.x,
                  MvStyle::MvWarning.y,
                  MvStyle::MvWarning.z,
                  alpha);
}

// Hovering text reads as a link affordance.
void DrawUpgradeLink() {
    MvStyle::ScopedFont f(MvStyle::Font.BodySm);
    const std::string linkText = std::format("Upgrade at metavisage.io {}",
                                             kArrowRight);
    ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue500);
    ImGui::TextUnformatted(linkText.c_str());
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        OpenPricingPage();
    }
}

}  // namespace

// =============================================================================
// Inline callout -- rounded amber box that visually matches the Starlight
// "Caution" aside. Icon + "PREMIUM FEATURE" title, body line, upgrade link.
// Rendered inline at gate sites where a control is disabled or a section is
// hidden.
// =============================================================================
void DrawUpgradePrompt(const UpgradePromptCopy& copy) {
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));

    constexpr float kPadX     = 12.0f;
    constexpr float kPadY     = 10.0f;
    constexpr float kRounding = 8.0f;
    constexpr float kBorderPx = 1.0f;

    // Reserve the bounding rect up-front so the border + fill can be drawn
    // beneath the content. Width = available; height computed from the wrapped
    // text run we know we'll emit (3 visible lines: title, body, link).
    const float availW = ImGui::GetContentRegionAvail().x;
    const ImVec2 boxMin = ImGui::GetCursorScreenPos();

    // Push the cursor in by the padding for the inner content. The box
    // height is finalized after we measure the content, so we capture
    // the starting cursor and re-rect after.
    ImGui::SetCursorScreenPos(ImVec2(boxMin.x + kPadX, boxMin.y + kPadY));
    ImGui::BeginGroup();
    {
        // Title row -- zap icon + "PREMIUM FEATURE" mono uppercase.
        {
            MvStyle::ScopedFont f(MvStyle::Font.IconSm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvWarning);
            ImGui::TextUnformatted(MV_ICON_ZAP);
            ImGui::PopStyleColor();
        }
        ImGui::SameLine(0.0f, MvStyle::Spacing::S2);
        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvWarning);
            ImGui::TextUnformatted("PREMIUM FEATURE");
            ImGui::PopStyleColor();
        }

        // Body line in muted foreground so the title still reads as the
        // call-out header.
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
            // Account for the kPadX inset on the right side of the wrap.
            ImGui::PushTextWrapPos(boxMin.x + availW - kPadX);
            const std::string body = BuildBody(copy);
            ImGui::TextWrapped("%s", body.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        DrawUpgradeLink();
    }
    ImGui::EndGroup();

    // Finalize the box rect from the group's size, then draw the bg fill
    // + border *behind* the content (ChannelsSplit would be cleaner but
    // is overkill here -- the content runs after this and ImGui paints
    // it in z-order over the draw list we just added to).
    const ImVec2 endPos = ImGui::GetCursorScreenPos();
    const ImVec2 boxMax(boxMin.x + availW, endPos.y + kPadY - MvStyle::Spacing::S2);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(boxMin, boxMax,
                      ImGui::ColorConvertFloat4ToU32(WarnFill(0.14f)),
                      kRounding);
    dl->AddRect(boxMin, boxMax,
                ImGui::ColorConvertFloat4ToU32(WarnFill(0.55f)),
                kRounding, 0, kBorderPx);

    // Advance the layout cursor past the box so the next item lays out
    // below the callout cleanly.
    ImGui::Dummy(ImVec2(availW, 0.0f));
    ImGui::Dummy(ImVec2(0.0f, kPadY));
}

// =============================================================================
// Hover card -- richer side-popup for disabled controls. Image-style header
// strip with the feature glyph + feature name in display weight; description
// body; tier line; upgrade link.
// =============================================================================
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        return;

    // Constrain the tooltip width so wrapped descriptions read cleanly.
    constexpr float kTipWidth   = 280.0f;
    constexpr float kHeaderH    = 56.0f;

    ImGui::SetNextWindowSize(ImVec2(kTipWidth, 0.0f), ImGuiCond_Always);
    if (!ImGui::BeginTooltip()) return;

    // Header strip -- amber-tinted band with the feature glyph (large) on
    // the left and the feature name to its right. Reads as an "image
    // header" without requiring per-feature PNG art.
    const ImVec2 hdrMin = ImGui::GetCursorScreenPos();
    const ImVec2 hdrMax(hdrMin.x + kTipWidth, hdrMin.y + kHeaderH);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(hdrMin, hdrMax,
                      ImGui::ColorConvertFloat4ToU32(WarnFill(0.22f)),
                      6.0f, ImDrawFlags_RoundCornersTop);

    // Large glyph centred vertically in the header. Hold the glyph
    // string by value so the c_str() pointer outlives the AddText call.
    const std::string glyphStorage =
        copy.featureGlyph.empty() ? std::string(MV_ICON_ZAP)
                                  : std::string(copy.featureGlyph);
    {
        const float glyphSize = 28.0f;
        if (MvStyle::Font.Icon) {
            dl->AddText(MvStyle::Font.Icon,
                        glyphSize,
                        ImVec2(hdrMin.x + 14.0f,
                               hdrMin.y + (kHeaderH - glyphSize) * 0.5f),
                        ImGui::ColorConvertFloat4ToU32(MvStyle::MvWarning),
                        glyphStorage.c_str());
        }
    }
    // Feature name to the right of the glyph, vertically centred.
    {
        MvStyle::ScopedFont f(MvStyle::Font.Heading);
        const ImVec2 ts = ImGui::CalcTextSize(std::string(copy.feature).c_str());
        ImGui::SetCursorScreenPos(ImVec2(hdrMin.x + 14.0f + 28.0f + 12.0f,
                                         hdrMin.y + (kHeaderH - ts.y) * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
        ImGui::TextUnformatted(std::string(copy.feature).c_str());
        ImGui::PopStyleColor();
    }

    // Push the layout cursor past the header so subsequent content lays
    // out below it.
    ImGui::SetCursorScreenPos(ImVec2(hdrMin.x, hdrMin.y + kHeaderH));
    ImGui::Dummy(ImVec2(kTipWidth, MvStyle::Spacing::S2));

    // Body padding mirrors the inline pill -- 12 px inset.
    ImGui::Indent(12.0f);
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        ImGui::PushTextWrapPos(hdrMin.x + kTipWidth - 12.0f);
        const std::string body = BuildBody(copy);
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
        const std::string tier = std::format("REQUIRES {}",
                                             std::string(copy.minimumTier));
        // Uppercase for the eyebrow look.
        std::string upper = tier;
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        ImGui::TextUnformatted(upper.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    DrawUpgradeLink();
    ImGui::Unindent(12.0f);
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));

    ImGui::EndTooltip();
}

}  // namespace MetaVisage::ui
