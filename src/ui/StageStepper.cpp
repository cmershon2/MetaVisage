#include "ui/StageStepper.h"

#include "ui/MvStyle.h"
#include "core/Project.h"
#include "core/Types.h"   // WorkflowStage

#include <imgui.h>

#include <array>
#include <cstdio>

namespace MetaVisage::StageStepper {

namespace {

struct StageInfo {
    const char* name;
};

// Names match the design file (`STAGES` array in app.jsx) and the
// `StageDisplayName` helper in AppWindow.cpp. We don't share the latter
// because the design uses a richer label for Stage 4 ("Touch up & Export"
// vs the existing "Touch Up").
constexpr std::array<StageInfo, 4> kStages = {{
    { "Alignment" },
    { "Point Reference" },
    { "Morphing" },
    { "Touch up & Export" },
}};

ImU32 Col(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

ImVec4 WithAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

// Two-line checkmark glyph centered on `center`, sized to fit a `size`-square
// disc. Used for completed stages.
void DrawCheckmark(ImDrawList* dl, ImVec2 center, float size, ImU32 col) {
    const float s = size * 0.30f;
    const ImVec2 a = ImVec2(center.x - s,        center.y + s * 0.05f);
    const ImVec2 b = ImVec2(center.x - s * 0.20f, center.y + s * 0.65f);
    const ImVec2 c = ImVec2(center.x + s,        center.y - s * 0.55f);
    dl->AddLine(a, b, col, 2.4f);
    dl->AddLine(b, c, col, 2.4f);
}

// Lightweight `>` between steps. Drawn in MvFg4.
void DrawChevron(ImDrawList* dl, ImVec2 center, ImU32 col) {
    const float s = 5.0f;
    dl->AddLine(ImVec2(center.x - s * 0.5f, center.y - s),
                ImVec2(center.x + s * 0.5f, center.y),
                col, 1.6f);
    dl->AddLine(ImVec2(center.x + s * 0.5f, center.y),
                ImVec2(center.x - s * 0.5f, center.y + s),
                col, 1.6f);
}

} // namespace

std::optional<int> Render(const Project* project, float width) {
    constexpr float StepperH    = HeightPx;
    constexpr float DiscSize    = 36.0f;
    constexpr float DiscPad     = 16.0f;
    constexpr float ChevronGap  = 24.0f;
    constexpr float TextLeftPad = 14.0f;

    std::optional<int> result;

    const int currentStage = project ? static_cast<int>(project->GetCurrentStage()) : 0;
    const bool canAdvance  = project && project->CanProceedToNextStage();

    // BeginChild gives us a clipped rect with its own background fill — the
    // chrome doesn't bleed past the stepper region into the dockspace below.
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, MvStyle::MvBg1);

    ImGui::BeginChild("##MvStageStepper", ImVec2(width, StepperH), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // 1 px hairline divider along the bottom of the stepper, matching the
    // title bar / menu bar borders for visual consistency.
    dl->AddLine(ImVec2(origin.x,         origin.y + StepperH - 1.0f),
                ImVec2(origin.x + width, origin.y + StepperH - 1.0f),
                Col(MvStyle::MvLine2));

    const int N = static_cast<int>(kStages.size());
    const float chevronTotal = static_cast<float>(N - 1) * ChevronGap;
    const float stepW = (width - chevronTotal) / static_cast<float>(N);

    float xCursor = 0.0f;
    for (int i = 0; i < N; ++i) {
        const ImVec2 stepMin = ImVec2(origin.x + xCursor, origin.y);
        const ImVec2 stepMax = ImVec2(stepMin.x + stepW, stepMin.y + StepperH);

        const bool isActive   = (i == currentStage);
        const bool isComplete = (i <  currentStage);

        // Forward-jump gating mirrors StageSidebar's `Next >` button: any
        // backward jump is allowed; one forward step is allowed iff the
        // current stage's exit gate is satisfied. Skipping further forward
        // is an end-run around per-stage validation, so we ignore those.
        const bool clickable = (i < currentStage) ||
                               (i == currentStage + 1 && canAdvance);

        // Hit area for the whole step, not just the disc — matches the
        // design (the step button is a single click target spanning the
        // full row width).
        ImGui::SetCursorScreenPos(stepMin);
        ImGui::PushID(i);
        const bool clicked = ImGui::InvisibleButton(
            "##stepBtn", ImVec2(stepW, StepperH));
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        if (hovered && !isActive && clickable) {
            dl->AddRectFilled(stepMin, stepMax, Col(MvStyle::MvBg2));
        }

        // Active-step underline — 2 px MvBlue500 along the bottom edge, drawn
        // on top of the bottom hairline so it dominates visually.
        if (isActive) {
            dl->AddRectFilled(
                ImVec2(stepMin.x, stepMax.y - 2.0f),
                stepMax,
                Col(MvStyle::MvBlue500));
        }

        // Disc (36×36, rounding=6) ----------------------------------------
        const ImVec2 discCenter = ImVec2(stepMin.x + DiscPad + DiscSize * 0.5f,
                                         stepMin.y + StepperH * 0.5f);
        const ImVec2 discMin    = ImVec2(discCenter.x - DiscSize * 0.5f,
                                         discCenter.y - DiscSize * 0.5f);
        const ImVec2 discMax    = ImVec2(discCenter.x + DiscSize * 0.5f,
                                         discCenter.y + DiscSize * 0.5f);

        ImVec4 discFill, discBorder, discIconColor, statusColor, nameColor;
        const char* statusText;
        if (isActive) {
            discFill      = MvStyle::MvBlue500;
            discBorder    = MvStyle::MvBlue300;
            // Dark text on bright blue — matches the `#051322` foreground used
            // for the primary button label in the design system.
            discIconColor = ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f);
            statusColor   = MvStyle::MvBlue500;
            nameColor     = MvStyle::MvFg1;
            statusText    = "IN PROGRESS";
        } else if (isComplete) {
            discFill      = WithAlpha(MvStyle::MvSuccess, 0.12f);
            discBorder    = WithAlpha(MvStyle::MvSuccess, 0.40f);
            discIconColor = MvStyle::MvSuccess;
            statusColor   = MvStyle::MvSuccess;
            nameColor     = MvStyle::MvFg2;
            statusText    = "COMPLETE";
        } else {
            discFill      = MvStyle::MvBg2;
            discBorder    = MvStyle::MvLine2;
            discIconColor = MvStyle::MvFg4;
            statusColor   = MvStyle::MvFg4;
            nameColor     = MvStyle::MvFg3;
            statusText    = "UPCOMING";
        }

        dl->AddRectFilled(discMin, discMax, Col(discFill),   6.0f);
        dl->AddRect      (discMin, discMax, Col(discBorder), 6.0f, 0, 1.0f);

        if (isComplete) {
            DrawCheckmark(dl, discCenter, DiscSize, Col(discIconColor));
        } else {
            // "01" / "02" / "03" / "04" mono — Mono (11px) is the closest
            // handle we have; it's a touch small for a 36px disc but reads
            // cleanly. A dedicated 13px mono would match design exactly;
            // can add later if the visual review flags it.
            char buf[8];
            std::snprintf(buf, sizeof(buf), "0%d", i + 1);
            MvStyle::ScopedFont f(MvStyle::Font.Mono);
            const ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(discCenter.x - ts.x * 0.5f,
                               discCenter.y - ts.y * 0.5f),
                        Col(discIconColor), buf);
        }

        // Text block (eyebrow + name) -------------------------------------
        // Eyebrow sits just above the vertical centre line; name sits just
        // below. Pre-measured offsets keep the layout stable across font
        // load / fallback (eyebrow 10px, name 14px Medium).
        const float textX = discMax.x + TextLeftPad;
        const float midY  = stepMin.y + StepperH * 0.5f;

        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            const float h = ImGui::GetFontSize();
            dl->AddText(ImVec2(textX, midY - h - 2.0f),
                        Col(statusColor), statusText);
        }
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodyMd);
            dl->AddText(ImVec2(textX, midY + 3.0f),
                        Col(nameColor), kStages[i].name);
        }

        if (clicked && clickable && !isActive) {
            result = i;
        }

        xCursor += stepW;

        // Chevron between steps — drawn after the step so it sits in the gap
        // and doesn't get clipped by the next step's hover background.
        if (i < N - 1) {
            const ImVec2 chevCenter = ImVec2(origin.x + xCursor + ChevronGap * 0.5f,
                                             origin.y + StepperH * 0.5f);
            DrawChevron(dl, chevCenter, Col(MvStyle::MvFg4));
            xCursor += ChevronGap;
        }
    }

    ImGui::EndChild();
    return result;
}

} // namespace MetaVisage::StageStepper
