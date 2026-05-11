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

// Clickable "Upgrade at metavisage.io ->" link in MvBlue500 text. Caller
// is responsible for placement; this draws a single text run as a hot item.
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

// Render the hover-card body (image / glyph header + description + REQUIRES
// row + upgrade link). Reused by DrawUpgradePromptTooltip and by the
// hover state of the inline pill.
void RenderHoverCardContents(const UpgradePromptCopy& copy, float tipWidth) {
    constexpr float kHeaderH = 180.0f;  // matches recommended 320x180 PNG
    constexpr float kIconHeaderH = 56.0f;  // fallback header when no image
    constexpr float kSidePad = 14.0f;

    const ImVec2 hdrMin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (copy.heroImage != 0) {
        // Hero PNG header (16:9 = 320x180). The card width is tipWidth;
        // image height scales to maintain the aspect ratio.
        const float imgH = tipWidth * (kHeaderH / 320.0f);
        const ImVec2 hdrMax(hdrMin.x + tipWidth, hdrMin.y + imgH);
        dl->AddImageRounded(copy.heroImage, hdrMin, hdrMax,
                            ImVec2(0, 0), ImVec2(1, 1),
                            IM_COL32_WHITE, 6.0f,
                            ImDrawFlags_RoundCornersTop);
        ImGui::SetCursorScreenPos(ImVec2(hdrMin.x, hdrMax.y));
    } else {
        // Gradient header strip with the feature glyph centred vertically
        // and the feature name to its right. Reads as an "image header"
        // without requiring a PNG asset.
        const ImVec2 hdrMax(hdrMin.x + tipWidth, hdrMin.y + kIconHeaderH);
        dl->AddRectFilled(hdrMin, hdrMax,
                          ImGui::ColorConvertFloat4ToU32(WarnFill(0.22f)),
                          6.0f, ImDrawFlags_RoundCornersTop);

        const std::string glyphStorage =
            copy.featureGlyph.empty() ? std::string(MV_ICON_ZAP)
                                      : std::string(copy.featureGlyph);
        if (MvStyle::Font.Icon) {
            const float glyphSize = 28.0f;
            dl->AddText(MvStyle::Font.Icon, glyphSize,
                        ImVec2(hdrMin.x + kSidePad,
                               hdrMin.y + (kIconHeaderH - glyphSize) * 0.5f),
                        ImGui::ColorConvertFloat4ToU32(MvStyle::MvWarning),
                        glyphStorage.c_str());
        }
        {
            MvStyle::ScopedFont f(MvStyle::Font.Heading);
            const ImVec2 ts = ImGui::CalcTextSize(
                std::string(copy.feature).c_str());
            ImGui::SetCursorScreenPos(
                ImVec2(hdrMin.x + kSidePad + 28.0f + 12.0f,
                       hdrMin.y + (kIconHeaderH - ts.y) * 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
            ImGui::TextUnformatted(std::string(copy.feature).c_str());
            ImGui::PopStyleColor();
        }
        ImGui::SetCursorScreenPos(ImVec2(hdrMin.x, hdrMax.y));
    }

    ImGui::Dummy(ImVec2(tipWidth, MvStyle::Spacing::S2));
    ImGui::Indent(kSidePad);

    // When the PNG header is in play, repeat the feature name beneath the
    // image so the title is always rendered in body type.
    if (copy.heroImage != 0) {
        MvStyle::ScopedFont f(MvStyle::Font.Heading);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
        ImGui::TextUnformatted(std::string(copy.feature).c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
    }

    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        ImGui::PushTextWrapPos(hdrMin.x + tipWidth - kSidePad);
        const std::string body = BuildBody(copy);
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
        std::string tier(copy.minimumTier);
        for (auto& c : tier) c = static_cast<char>(
            std::toupper(static_cast<unsigned char>(c)));
        const std::string label = std::format("REQUIRES {}", tier);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    DrawUpgradeLink();
    ImGui::Unindent(kSidePad);
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));
}

void OpenHoverCardTooltip(const UpgradePromptCopy& copy) {
    constexpr float kTipWidth = 320.0f;
    ImGui::SetNextWindowSize(ImVec2(kTipWidth, 0.0f), ImGuiCond_Always);
    if (!ImGui::BeginTooltip()) return;
    RenderHoverCardContents(copy, kTipWidth);
    ImGui::EndTooltip();
}

}  // namespace

// =============================================================================
// Inline callout -- rounded amber box matching the Starlight "Caution" aside.
// Auto-resizes its height to fit the wrapped body text. Hovering reveals the
// rich hover card (RenderHoverCardContents) so the user can drill into the
// feature without having to find a disabled control to mouse over.
// =============================================================================
void DrawUpgradePrompt(const UpgradePromptCopy& copy) {
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));

    constexpr float kRounding = 8.0f;
    constexpr float kBorderPx = 1.0f;

    const float availW = ImGui::GetContentRegionAvail().x;

    // BeginChild with AutoResizeY measures content height for us, so the
    // body's TextWrapped runs the full width of the child and the child
    // grows to fit. ChildBg / Border get the amber tokens.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, WarnFill(0.14f));
    ImGui::PushStyleColor(ImGuiCol_Border, WarnFill(0.55f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,   kRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, kBorderPx);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(12.0f, 10.0f));

    // PushID prevents the child name from colliding across multiple
    // upgrade prompts in the same sidebar (e.g. textures + masking
    // both rendering pills).
    ImGui::PushID(std::string(copy.feature).c_str());
    ImGui::BeginChild("##premiumPill",
                      ImVec2(availW, 0.0f),
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        const std::string body = BuildBody(copy);
        // Inside a BeginChild, TextWrapped wraps at the child's right edge
        // (minus WindowPadding) automatically; no explicit PushTextWrapPos
        // needed.
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    DrawUpgradeLink();

    ImGui::EndChild();
    const bool pillHovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S1));

    // Hover state -- reveal the rich card to the side of the sidebar.
    // Side-effect-free if not hovering; otherwise BeginTooltip auto-
    // positions the popup near the mouse cursor.
    if (pillHovered) {
        OpenHoverCardTooltip(copy);
    }
}

// =============================================================================
// Hover card -- richer side-popup for disabled controls. Image-style header
// strip with a 320x180 hero image when supplied, otherwise an amber-tinted
// band with the feature glyph + name. Description body, REQUIRES <TIER>
// eyebrow, upgrade link.
// =============================================================================
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        return;
    OpenHoverCardTooltip(copy);
}

}  // namespace MetaVisage::ui
