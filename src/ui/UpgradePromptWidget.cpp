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
// upgrade entry point — distinct from /manage-subscription which is the
// post-purchase Stripe portal (already used by the License Status modal).
constexpr const char* kUpgradePathSuffix = "/pricing";

// Right-arrow glyph appended to the upgrade link copy.
constexpr const char* kArrowRight = "\xE2\x86\x92";  // U+2192

void OpenPricingPage() {
    const std::string url =
        std::string(MetaVisage::LicenseConfig::kApiBaseUrl) + kUpgradePathSuffix;
    MetaVisage::OpenUrl(url);
}

// Render the lock icon + "<Feature> requires <Tier>" body in amber.
// Caller is responsible for laying out the row before/after.
void DrawIconAndBody(const UpgradePromptCopy& copy) {
    {
        MvStyle::ScopedFont f(MvStyle::Font.IconSm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvWarning);
        ImGui::TextUnformatted(MV_ICON_LOCK);
        ImGui::PopStyleColor();
    }
    ImGui::SameLine(0.0f, MvStyle::Spacing::S2);
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvWarning);
        const std::string body = std::format("{} requires {}",
                                             copy.feature, copy.minimumTier);
        ImGui::TextUnformatted(body.c_str());
        ImGui::PopStyleColor();
    }
}

}  // namespace

void DrawUpgradePrompt(const UpgradePromptCopy& copy) {
    ImGui::Dummy(ImVec2(0.0f, MvStyle::Spacing::S2));

    DrawIconAndBody(copy);

    // Right-aligned clickable "Upgrade at metavisage.io →" link in MvBlue500.
    // Rendered as plain text + IsItemClicked rather than a Button so it
    // reads as a link rather than a control. Hover swaps the cursor to the
    // hand cursor for affordance.
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    {
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
}

void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy) {
    // BeginDisabled blocks IsItemHovered by default; AllowWhenDisabled lets
    // the disabled control still surface a hover tooltip.
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        return;

    ImGui::BeginTooltip();
    DrawIconAndBody(copy);
    ImGui::EndTooltip();
}

}  // namespace MetaVisage::ui
