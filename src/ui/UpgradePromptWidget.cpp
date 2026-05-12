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
        // PushTextWrapPos takes a WINDOW-LOCAL x; 0.0f wraps at the
        // window's right edge minus WindowPadding. Previously we were
        // passing a screen-space value, which caused the body to extend
        // past the popup right edge and clip.
        ImGui::PushTextWrapPos(0.0f);
        const std::string body = BuildBody(copy);
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    // Tier-requirement rendered as a pill, not an eyebrow row. Matches the
    // chip's tier-pill styling (small rounded rect, tier-coloured fill, mono
    // text). No upgrade link in the hover card -- the sidebar pill (which
    // hosts the hover) already carries the click affordance.
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        std::string tier(copy.minimumTier);
        for (auto& c : tier) c = static_cast<char>(
            std::toupper(static_cast<unsigned char>(c)));

        const ImVec4 pillCol = (tier == "PRO")   ? MvStyle::MvWarning
                             : (tier == "INDIE") ? MvStyle::MvBlue500
                                                 : MvStyle::MvFg4;
        constexpr float kPillPadX   = 10.0f;
        constexpr float kPillHeight = 18.0f;

        float pillTextW = 0.0f;
        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            pillTextW = ImGui::CalcTextSize(tier.c_str()).x;
        }
        const float pillW = pillTextW + kPillPadX * 2.0f;

        const ImVec2 pillMin = ImGui::GetCursorScreenPos();
        const ImVec2 pillMax(pillMin.x + pillW, pillMin.y + kPillHeight);
        ImDrawList* dl2 = ImGui::GetWindowDrawList();
        dl2->AddRectFilled(pillMin, pillMax,
                           ImGui::ColorConvertFloat4ToU32(pillCol),
                           kPillHeight * 0.5f);
        if (MvStyle::Font.MonoSm) {
            const float tx = pillMin.x + kPillPadX;
            const float ty = pillMin.y +
                (kPillHeight - MvStyle::TypeSize::MonoSm) * 0.5f;
            dl2->AddText(MvStyle::Font.MonoSm,
                         MvStyle::TypeSize::MonoSm,
                         ImVec2(tx, ty),
                         ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg1),
                         tier.c_str());
        }
        // Advance the layout cursor past the pill so subsequent items
        // sit below it.
        ImGui::Dummy(ImVec2(pillW, kPillHeight));
    }

    ImGui::Unindent(kSidePad);
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));
}

// Sidebar-relative tooltip position: anchored to the LEFT of the sidebar's
// outer edge so the hover card sits adjacent to the panel rather than at
// the mouse cursor. Live-test feedback round 4: the prior cursor-anchored
// positioning placed the card on top of the sidebar (or overlapping the
// pill that triggered it), which read as a tooltip rather than as a side-
// panel preview.
//
// `sidebarLeftX` is captured by the caller from the parent window's left
// edge BEFORE the pill's BeginChild scope opens; the disabled-control
// variant (DrawUpgradePromptTooltip) captures it on entry.
// `anchorY` is the pill's vertical center so the card vertically tracks
// what's being hovered.
void OpenHoverCardTooltip(const UpgradePromptCopy& copy,
                          float sidebarLeftX,
                          float anchorY) {
    constexpr float kTipWidth = 320.0f;
    constexpr float kGap      = 12.0f;
    constexpr float kEstHeight = 280.0f;

    // Clamp to the visible viewport so the popup never extends off-screen
    // -- a sidebar at the far left of the viewport falls back to a "right
    // of cursor" placement so the card still shows something useful.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    float tipX = sidebarLeftX - kGap - kTipWidth;
    if (tipX < vp->Pos.x + 4.0f) {
        // Not enough room on the left; place to the right of the cursor.
        tipX = ImGui::GetMousePos().x + 16.0f;
    }

    // Vertically centre on the anchor with viewport clamping.
    float tipY = anchorY - kEstHeight * 0.5f;
    const float maxY = vp->Pos.y + vp->Size.y - kEstHeight - 4.0f;
    const float minY = vp->Pos.y + 4.0f;
    if (tipY > maxY) tipY = maxY;
    if (tipY < minY) tipY = minY;

    ImGui::SetNextWindowPos(ImVec2(tipX, tipY), ImGuiCond_Always);
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

    // Capture the sidebar's left edge BEFORE opening BeginChild so the
    // hover-card placement (to the left of the sidebar) can be anchored
    // accurately. Inside the child, GetWindowPos returns the child's
    // own position, not the sidebar's.
    const float sidebarLeftX = ImGui::GetWindowPos().x;

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
    const ImVec2 pillMin   = ImGui::GetItemRectMin();
    const ImVec2 pillMax   = ImGui::GetItemRectMax();
    ImGui::PopID();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    // Round-5 feedback: the whole sidebar pill is now the click target.
    // The inner "Upgrade at metavisage.io ->" text was already clickable,
    // but the pill body wasn't -- meaning users hovering and trying to
    // click outside the link missed the action. IsMouseClicked + the
    // pillHovered guard gives us "click anywhere on the pill = upgrade".
    if (pillHovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            OpenPricingPage();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S1));

    // Hover state -- reveal the rich card to the LEFT of the sidebar
    // (not at the cursor). Anchor vertical position on the pill's centre
    // so the card stays visually associated with what's being hovered.
    if (pillHovered) {
        const float anchorY = (pillMin.y + pillMax.y) * 0.5f;
        OpenHoverCardTooltip(copy, sidebarLeftX, anchorY);
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
    // Disabled-control variant -- capture the hover target's bounds for
    // the anchor + the parent window's left edge for the position. The
    // window-of-record at this point is the sidebar that hosts the
    // disabled control.
    const float sidebarLeftX = ImGui::GetWindowPos().x;
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    const float anchorY  = (itemMin.y + itemMax.y) * 0.5f;
    OpenHoverCardTooltip(copy, sidebarLeftX, anchorY);
}

}  // namespace MetaVisage::ui
