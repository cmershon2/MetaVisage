#include "ui/UpgradePromptWidget.h"

#include <algorithm>
#include <cctype>
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

ImU32 ColU32(const ImVec4& c) {
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImVec4 WithAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

ImVec4 TierColor(std::string_view tier) {
    std::string upper(tier);
    for (char& c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (upper == "PRO") return MvStyle::MvWarning;
    if (upper == "INDIE") return MvStyle::MvBlue500;
    return MvStyle::MvFg3;
}

std::string TierUpper(std::string_view tier) {
    std::string upper(tier);
    for (char& c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return upper;
}

std::string BuildBody(const UpgradePromptCopy& copy) {
    if (!copy.description.empty()) {
        return std::string(copy.description);
    }
    return std::format("{} requires {}.",
                       std::string(copy.feature),
                       std::string(copy.minimumTier));
}

void OpenPricingPage() {
    const std::string url =
        std::string(MetaVisage::LicenseConfig::kApiBaseUrl) + kUpgradePathSuffix;
    MetaVisage::OpenUrl(url);
}

void DrawHatch(ImDrawList* dl,
               ImVec2 min,
               ImVec2 max,
               ImU32 color,
               float spacing = 7.0f) {
    const float h = max.y - min.y;
    dl->PushClipRect(min, max, true);
    for (float x = min.x - h; x < max.x + h; x += spacing) {
        dl->AddLine(ImVec2(x, max.y), ImVec2(x + h, min.y), color, 1.0f);
    }
    dl->PopClipRect();
}

void DrawTierBadgeAt(ImDrawList* dl,
                     ImVec2 pos,
                     std::string_view tier,
                     float height = 16.0f) {
    const std::string upper = TierUpper(tier);
    const ImVec4 tierCol = TierColor(tier);
    float textW = 0.0f;
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        textW = ImGui::CalcTextSize(upper.c_str()).x;
    }

    const float w = textW + MvStyle::Spacing::S3;
    const ImVec2 max(pos.x + w, pos.y + height);
    dl->AddRectFilled(pos, max, ColU32(MvStyle::MvBg1), 3.0f);
    dl->AddRect(pos, max, ColU32(tierCol), 3.0f, 0, 1.0f);

    if (MvStyle::Font.MonoSm) {
        const float ty = pos.y + (height - MvStyle::TypeSize::MonoSm) * 0.5f;
        dl->AddText(MvStyle::Font.MonoSm,
                    MvStyle::TypeSize::MonoSm,
                    ImVec2(pos.x + MvStyle::Spacing::S2 * 0.5f, ty),
                    ColU32(MvStyle::MvFg1),
                    upper.c_str());
    }
}

float TierBadgeWidth(std::string_view tier) {
    const std::string upper = TierUpper(tier);
    MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
    return ImGui::CalcTextSize(upper.c_str()).x + MvStyle::Spacing::S3;
}

void DrawCompactCardContents(const UpgradePromptCopy& copy,
                             float width,
                             bool showUpgradeLink) {
    const float pad = MvStyle::Spacing::S3;
    const ImVec4 tierCol = TierColor(copy.minimumTier);
    const std::string eyebrow =
        std::format("{} {}",
                    TierUpper(copy.minimumTier),
                    copy.kind == UpgradePromptKind::Brush ? "BRUSH" : "FEATURE");

    ImGui::Dummy(ImVec2(width, pad - 2.0f));
    ImGui::Indent(pad);

    const float contentW = width - pad * 2.0f;
    if (MvStyle::Font.IconSm) {
        MvStyle::ScopedFont fi(MvStyle::Font.IconSm);
        ImGui::PushStyleColor(ImGuiCol_Text, tierCol);
        ImGui::TextUnformatted(MV_ICON_LOCK);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, MvStyle::Spacing::S1);
    }
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        ImGui::PushStyleColor(ImGuiCol_Text, tierCol);
        ImGui::TextUnformatted(eyebrow.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodyMd);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + contentW);
        ImGui::TextWrapped("%s", std::string(copy.feature).c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + contentW);
        const std::string body = BuildBody(copy);
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    if (showUpgradeLink) {
        ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S1));
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue500);
        ImGui::TextUnformatted("Upgrade ->");
        ImGui::PopStyleColor();
    }

    ImGui::Unindent(pad);
    ImGui::Dummy(ImVec2(width, pad - 2.0f));
}

void OpenCompactHoverCard(const UpgradePromptCopy& copy,
                          float x,
                          float y,
                          bool showUpgradeLink) {
    const float cardW = copy.kind == UpgradePromptKind::Brush ? 248.0f : 236.0f;
    const float estH  = copy.kind == UpgradePromptKind::Brush ? 126.0f : 150.0f;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    x = std::clamp(x, vp->Pos.x + 4.0f, vp->Pos.x + vp->Size.x - cardW - 4.0f);
    y = std::clamp(y, vp->Pos.y + 4.0f, vp->Pos.y + vp->Size.y - estH - 4.0f);

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(cardW, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, MvStyle::MvBg2);
    ImGui::PushStyleColor(ImGuiCol_Border, MvStyle::MvLine3);
    const bool open = ImGui::BeginTooltip();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    if (!open) return;
    DrawCompactCardContents(copy, cardW, showUpgradeLink);
    ImGui::EndTooltip();
}

}  // namespace

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
    const ImVec2 rectMin(scope.sidebarLeftX,        scope.topY);
    const ImVec2 rectMax(scope.sidebarLeftX + winW, bottomY);
    const bool hovered =
        ImGui::IsMouseHoveringRect(rectMin, rectMax, /*clip=*/true);
    if (!hovered) return;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        OpenPricingPage();
    }

    const float x = scope.sidebarLeftX + MvStyle::Spacing::S3;
    const float y = std::min(ImGui::GetMousePos().y + MvStyle::Spacing::S2,
                             bottomY + MvStyle::Spacing::S1);
    OpenCompactHoverCard(scope.copy, x, y, /*showUpgradeLink=*/true);
}

void DrawLockedFeatureField(const char* label,
                            const char* value,
                            const UpgradePromptCopy& copy,
                            const char* rightText) {
    constexpr float kRowH = 28.0f;
    constexpr float kFieldRounding = 3.0f;

    const float availW = ImGui::GetContentRegionAvail().x;
    if (availW < 1.0f) return;

    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax(rowMin.x + availW, rowMin.y + kRowH);
    ImGui::PushID(label ? label : "lockedFeature");
    const bool clicked = ImGui::InvisibleButton("##lockedFeature",
                                                ImVec2(availW, kRowH));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (clicked) OpenPricingPage();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float labelZoneW = std::min(136.0f, availW * 0.44f);
    const float fieldX = rowMin.x + labelZoneW;
    const ImVec2 fieldMin(fieldX, rowMin.y);
    const ImVec2 fieldMax(rowMax.x, rowMax.y);

    if (label && label[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(rowMin.x, rowMin.y + (kRowH - fs) * 0.5f),
                    ColU32(MvStyle::MvFg2),
                    label);
    }

    const float badgeW = TierBadgeWidth(copy.minimumTier);
    const float badgeX = std::min(fieldX - badgeW - MvStyle::Spacing::S2,
                                  rowMin.x + labelZoneW - badgeW);
    if (badgeX > rowMin.x + 24.0f) {
        DrawTierBadgeAt(dl,
                        ImVec2(badgeX, rowMin.y + (kRowH - 16.0f) * 0.5f),
                        copy.minimumTier);
    }

    const ImU32 bgCol = ColU32(hovered ? MvStyle::MvBg2 : MvStyle::MvBg1);
    const ImU32 borderCol = ColU32(hovered ? MvStyle::MvBlue500
                                           : MvStyle::MvLine2);
    dl->AddRectFilled(fieldMin, fieldMax, bgCol, kFieldRounding);
    DrawHatch(dl, fieldMin, fieldMax,
              ColU32(WithAlpha(MvStyle::MvLine2, hovered ? 0.45f : 0.28f)));
    dl->AddRect(fieldMin, fieldMax, borderCol, kFieldRounding, 0,
                hovered ? 1.2f : 1.0f);

    const char* fieldText = (value && value[0]) ? value : "Locked";
    {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        const float fs = ImGui::GetFontSize();
        dl->PushClipRect(fieldMin, fieldMax, true);
        dl->AddText(ImVec2(fieldMin.x + MvStyle::Spacing::S2,
                           fieldMin.y + (kRowH - fs) * 0.5f),
                    ColU32(MvStyle::MvFg4),
                    fieldText);
        if (rightText && rightText[0]) {
            const ImVec2 ts = ImGui::CalcTextSize(rightText);
            dl->AddText(ImVec2(fieldMax.x - ts.x - MvStyle::Spacing::S2,
                               fieldMin.y + (kRowH - fs) * 0.5f),
                        ColU32(MvStyle::MvFg3),
                        rightText);
        }
        dl->PopClipRect();
    }

    if (hovered) {
        OpenCompactHoverCard(copy,
                             rowMin.x,
                             rowMax.y + MvStyle::Spacing::S1,
                             /*showUpgradeLink=*/true);
    }
}

void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        return;
    }

    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    const bool showUpgradeLink = copy.kind != UpgradePromptKind::Brush;
    const float x = copy.kind == UpgradePromptKind::Brush
        ? itemMin.x
        : ImGui::GetWindowPos().x + MvStyle::Spacing::S3;
    const float y = itemMax.y + MvStyle::Spacing::S1;
    OpenCompactHoverCard(copy, x, y, showUpgradeLink);
}

}  // namespace MetaVisage::ui
