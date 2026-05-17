#include "ui/UpgradePromptWidget.h"

#include <algorithm>
#include <format>
#include <string>

#include <imgui.h>

#include "licensing/LicenseConfig.h"
#include "ui/MvIcons.h"
#include "ui/MvStyle.h"
#include "utils/OpenUrl.h"

namespace MetaVisage::ui {

namespace {

constexpr const char* kUpgradePathSuffix = "/pricing";

void OpenPricingPage() {
    const std::string url =
        std::string(MetaVisage::LicenseConfig::kApiBaseUrl) + kUpgradePathSuffix;
    MetaVisage::OpenUrl(url);
}

std::string BuildBody(const UpgradePromptCopy& copy) {
    if (!copy.description.empty()) {
        return std::string(copy.description);
    }
    return std::format("{} -- available on the {} plan.",
                       std::string(copy.feature),
                       std::string(copy.minimumTier));
}

// MvStyle::MvWarning at a low alpha for the callout backgrounds.
ImVec4 WarnFill(float alpha) {
    return ImVec4(MvStyle::MvWarning.x,
                  MvStyle::MvWarning.y,
                  MvStyle::MvWarning.z,
                  alpha);
}

// Multiply each colour channel by `factor` (alpha unchanged) -- used for the
// upgrade button's pressed / icon-box darker amber accent.
ImVec4 Darken(const ImVec4& c, float factor) {
    return ImVec4(c.x * factor, c.y * factor, c.z * factor, c.w);
}

ImU32 ColU32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

// =============================================================================
// Hover card body -- image / glyph header, description, REQUIRES <tier> pill.
// Reused by every entry point that opens the side-anchored popup.
// =============================================================================
void RenderHoverCardContents(const UpgradePromptCopy& copy, float tipWidth) {
    constexpr float kHeaderH = 180.0f;   // 320 x 180 hero PNG
    constexpr float kIconHeaderH = 56.0f;  // glyph fallback band
    constexpr float kSidePad = 14.0f;

    const ImVec2 hdrMin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (copy.heroImage != 0) {
        const float imgH = tipWidth * (kHeaderH / 320.0f);
        const ImVec2 hdrMax(hdrMin.x + tipWidth, hdrMin.y + imgH);
        dl->AddImageRounded(copy.heroImage, hdrMin, hdrMax,
                            ImVec2(0, 0), ImVec2(1, 1),
                            IM_COL32_WHITE, 6.0f,
                            ImDrawFlags_RoundCornersTop);
        ImGui::SetCursorScreenPos(ImVec2(hdrMin.x, hdrMax.y));
    } else {
        const ImVec2 hdrMax(hdrMin.x + tipWidth, hdrMin.y + kIconHeaderH);
        dl->AddRectFilled(hdrMin, hdrMax,
                          ColU32(WarnFill(0.22f)),
                          6.0f, ImDrawFlags_RoundCornersTop);

        const std::string glyphStorage =
            copy.featureGlyph.empty() ? std::string(MV_ICON_ZAP)
                                      : std::string(copy.featureGlyph);
        if (MvStyle::Font.Icon) {
            const float glyphSize = 28.0f;
            dl->AddText(MvStyle::Font.Icon, glyphSize,
                        ImVec2(hdrMin.x + kSidePad,
                               hdrMin.y + (kIconHeaderH - glyphSize) * 0.5f),
                        ColU32(MvStyle::MvWarning),
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
        ImGui::PushTextWrapPos(tipWidth - kSidePad);
        const std::string body = BuildBody(copy);
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    // Tier requirement as a coloured pill, matching the chip's tier-pill
    // styling. No upgrade link in the card -- the section's Upgrade
    // button (rendered in the sidebar by the caller) is the action.
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
                           ColU32(pillCol),
                           kPillHeight * 0.5f);
        if (MvStyle::Font.MonoSm) {
            const float tx = pillMin.x + kPillPadX;
            const float ty = pillMin.y +
                (kPillHeight - MvStyle::TypeSize::MonoSm) * 0.5f;
            dl2->AddText(MvStyle::Font.MonoSm,
                         MvStyle::TypeSize::MonoSm,
                         ImVec2(tx, ty),
                         ColU32(MvStyle::MvFg1),
                         tier.c_str());
        }
        ImGui::Dummy(ImVec2(pillW, kPillHeight));
    }

    ImGui::Unindent(kSidePad);
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));
}

// =============================================================================
// OpenHoverCardTooltip -- anchored to the LEFT of the sidebar (not the
// cursor) so the card reads as a side panel preview rather than a tooltip
// hovering over the trigger.
// =============================================================================
void OpenHoverCardTooltip(const UpgradePromptCopy& copy,
                          float sidebarLeftX,
                          float anchorY) {
    constexpr float kTipWidth = 320.0f;
    constexpr float kGap      = 12.0f;
    constexpr float kEstHeight = 280.0f;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    float tipX = sidebarLeftX - kGap - kTipWidth;
    if (tipX < vp->Pos.x + 4.0f) {
        tipX = ImGui::GetMousePos().x + 16.0f;
    }

    float tipY = anchorY - kEstHeight * 0.5f;
    const float maxY = vp->Pos.y + vp->Size.y - kEstHeight - 4.0f;
    const float minY = vp->Pos.y + 4.0f;
    if (tipY > maxY) tipY = maxY;
    if (tipY < minY) tipY = minY;

    // Zero WindowPadding so the hero image is edge-to-edge into the
    // rounded top corners; body content keeps its breathing room via
    // Indent / PushTextWrapPos inside RenderHoverCardContents.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowPos(ImVec2(tipX, tipY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kTipWidth, 0.0f), ImGuiCond_Always);
    const bool open = ImGui::BeginTooltip();
    ImGui::PopStyleVar();
    if (!open) return;
    RenderHoverCardContents(copy, kTipWidth);
    ImGui::EndTooltip();
}

}  // namespace

// =============================================================================
// Gated-section hover scope -- captures the sidebar's left edge + the top of
// the gated block, then on End() tests whether the mouse is anywhere inside
// the [topY, currentCursorY] vertical range (across the full sidebar width)
// and fires the hover card if so. Lets a whole section behave as one large
// hover surface without wrapping its contents in an InvisibleButton or
// BeginChild (which would interfere with the disabled-control layout the
// caller is already responsible for).
// =============================================================================
GatedSectionScope BeginGatedSectionHover(const UpgradePromptCopy& copy) {
    GatedSectionScope scope;
    scope.copy         = copy;
    scope.sidebarLeftX = ImGui::GetWindowPos().x;
    scope.topY         = ImGui::GetCursorScreenPos().y;
    scope.active       = true;
    return scope;
}

void EndGatedSectionHover(const GatedSectionScope& scope) {
    if (!scope.active) return;
    const float bottomY = ImGui::GetCursorScreenPos().y;
    const float winW    = ImGui::GetWindowWidth();
    const ImVec2 rectMin(scope.sidebarLeftX,           scope.topY);
    const ImVec2 rectMax(scope.sidebarLeftX + winW,    bottomY);
    if (ImGui::IsMouseHoveringRect(rectMin, rectMax, /*clip=*/true)) {
        const float anchorY = (scope.topY + bottomY) * 0.5f;
        OpenHoverCardTooltip(scope.copy, scope.sidebarLeftX, anchorY);
    }
}

// =============================================================================
// DrawUpgradeButton -- full-width amber CTA that matches the round-7 mockup:
// rounded rectangle, "Upgrade" body text on the left, a darker amber square
// trailing on the right with the zap glyph centred inside.
// =============================================================================
void DrawUpgradeButton(const UpgradePromptCopy& copy) {
    (void)copy;  // copy is here for symmetry / future per-tier theming hooks

    constexpr float kButtonH    = 36.0f;
    constexpr float kIconBoxW   = 28.0f;
    constexpr float kRounding   = 8.0f;
    constexpr float kIconBoxPad = 4.0f;   // inset of the dark-amber accent
    constexpr float kIconBoxRoundFactor = 0.65f;

    const float availW = ImGui::GetContentRegionAvail().x;
    if (availW < 1.0f) return;

    const ImVec2 btnMin = ImGui::GetCursorScreenPos();
    const ImVec2 btnMax(btnMin.x + availW, btnMin.y + kButtonH);

    ImGui::PushID(std::string(copy.feature).c_str());
    const bool clicked = ImGui::InvisibleButton("##upgradeBtn",
                                                ImVec2(availW, kButtonH));
    const bool hovered = ImGui::IsItemHovered();
    const bool held    = ImGui::IsItemActive();
    ImGui::PopID();

    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (clicked) OpenPricingPage();

    // Visual states -- subtle hover lift, pressed dim.
    ImVec4 baseCol = MvStyle::MvWarning;
    if (held)        baseCol = Darken(baseCol, 0.85f);
    else if (hovered) baseCol = Darken(baseCol, 1.05f);  // slight lighten; clamps via saturation

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(btnMin, btnMax, ColU32(baseCol), kRounding);

    // Darker amber icon-box on the right -- inset by kIconBoxPad, rounded
    // a touch less than the outer button so the boxes nest cleanly.
    const ImVec2 iconBoxMin(btnMax.x - kIconBoxW - kIconBoxPad,
                            btnMin.y + kIconBoxPad);
    const ImVec2 iconBoxMax(btnMax.x - kIconBoxPad,
                            btnMax.y - kIconBoxPad);
    dl->AddRectFilled(iconBoxMin, iconBoxMax,
                      ColU32(Darken(baseCol, 0.78f)),
                      kRounding * kIconBoxRoundFactor);

    // "Upgrade" label centred in the left portion (between btnMin.x and
    // iconBoxMin.x).
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodyMd);
        const ImVec2 ts = ImGui::CalcTextSize("Upgrade");
        const float zoneL = btnMin.x;
        const float zoneR = iconBoxMin.x;
        const float tx = (zoneL + zoneR) * 0.5f - ts.x * 0.5f;
        const float ty = btnMin.y + (kButtonH - ts.y) * 0.5f;
        dl->AddText(MvStyle::Font.BodyMd
                        ? MvStyle::Font.BodyMd
                        : ImGui::GetFont(),
                    MvStyle::TypeSize::Body,
                    ImVec2(tx, ty),
                    ColU32(MvStyle::MvFg1),
                    "Upgrade");
    }

    // Zap glyph centred in the icon box.
    if (MvStyle::Font.Icon) {
        const float gs = 16.0f;
        const ImVec2 iconPos(
            (iconBoxMin.x + iconBoxMax.x) * 0.5f - gs * 0.5f,
            (iconBoxMin.y + iconBoxMax.y) * 0.5f - gs * 0.5f);
        dl->AddText(MvStyle::Font.Icon, gs,
                    iconPos,
                    ColU32(MvStyle::MvFg1),
                    MV_ICON_ZAP);
    }
}

// =============================================================================
// DrawUpgradePromptTooltip -- disabled-control hover variant. Still used by
// Stage 4 brush cards where each disabled brush card is a click target and
// hovering it should reveal that brush's specific upgrade preview.
// =============================================================================
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        return;
    const float sidebarLeftX = ImGui::GetWindowPos().x;
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    const float anchorY  = (itemMin.y + itemMax.y) * 0.5f;
    OpenHoverCardTooltip(copy, sidebarLeftX, anchorY);
}

}  // namespace MetaVisage::ui
