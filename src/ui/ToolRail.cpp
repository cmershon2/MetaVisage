#include "ui/ToolRail.h"

#include "ui/MvStyle.h"
#include "ui/Viewport.h"
#include "core/Project.h"
#include "core/Types.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <numbers>

namespace MetaVisage::ToolRail {

// =============================================================================
// Icon primitives — single-stroke Lucide-inspired glyphs drawn via ImDrawList.
// Each takes a centre, a half-extent `s` (typical 9 px for an 18 px icon),
// and a packed colour. Stroke weight is 1.6 px to match the design.
//
// Drawing every icon as line primitives is verbose but keeps Story 3.8.4
// asset-free. If the Story 3.8.5+ icon explosion makes this tedious, the
// migration target is a Lucide TTF icon font — every icon becomes a single
// `dl->AddText(font, glyph)` call once that lands.
// =============================================================================
namespace {

ImU32 Col(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

constexpr float kPi = std::numbers::pi_v<float>;

void DrawMove(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr float t = 1.6f;
    constexpr float ah = 3.0f;
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), col, t);
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x - ah, c.y - s + ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x + ah, c.y - s + ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y + s), ImVec2(c.x - ah, c.y + s - ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y + s), ImVec2(c.x + ah, c.y + s - ah), col, t);
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x - s + ah, c.y - ah), col, t);
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x - s + ah, c.y + ah), col, t);
    dl->AddLine(ImVec2(c.x + s, c.y), ImVec2(c.x + s - ah, c.y - ah), col, t);
    dl->AddLine(ImVec2(c.x + s, c.y), ImVec2(c.x + s - ah, c.y + ah), col, t);
}

void DrawRotate(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    // Open arc, ~270° starting from 12 o'clock and sweeping clockwise. The
    // open gap sits at the top so the arrowhead reads as "the arrow points
    // back to where the rotation started."
    constexpr int N = 24;
    ImVec2 pts[N + 1];
    for (int i = 0; i <= N; ++i) {
        const float t = static_cast<float>(i) / N;
        const float angle = -kPi * 0.4f + t * kPi * 1.7f;
        pts[i] = ImVec2(c.x + std::cos(angle) * s, c.y + std::sin(angle) * s);
    }
    dl->AddPolyline(pts, N + 1, col, ImDrawFlags_None, 1.6f);
    // Arrowhead at the start (top-right of arc).
    const ImVec2 tip = pts[0];
    dl->AddLine(tip, ImVec2(tip.x - 3.5f, tip.y - 1.5f), col, 1.6f);
    dl->AddLine(tip, ImVec2(tip.x - 1.5f, tip.y + 3.5f), col, 1.6f);
}

void DrawScale(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr float t = 1.6f;
    dl->AddLine(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), col, t);
    // NW corner bracket
    dl->AddLine(ImVec2(c.x - s, c.y - s), ImVec2(c.x - s + 4.0f, c.y - s), col, t);
    dl->AddLine(ImVec2(c.x - s, c.y - s), ImVec2(c.x - s, c.y - s + 4.0f), col, t);
    // SE corner bracket
    dl->AddLine(ImVec2(c.x + s, c.y + s), ImVec2(c.x + s - 4.0f, c.y + s), col, t);
    dl->AddLine(ImVec2(c.x + s, c.y + s), ImVec2(c.x + s, c.y + s - 4.0f), col, t);
}

void DrawSmooth(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr int N = 24;
    ImVec2 pts[N + 1];
    for (int i = 0; i <= N; ++i) {
        const float t = static_cast<float>(i) / N;
        const float x = c.x + (t - 0.5f) * s * 2.0f;
        const float y = c.y - std::sin(t * kPi * 2.0f) * s * 0.45f;
        pts[i] = ImVec2(x, y);
    }
    dl->AddPolyline(pts, N + 1, col, ImDrawFlags_None, 1.6f);
}

void DrawGrab(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    // Stylised hand: palm rectangle + extended index finger.
    dl->AddRect(ImVec2(c.x - s * 0.65f, c.y - s * 0.30f),
                ImVec2(c.x + s * 0.65f, c.y + s),
                col, 2.0f, 0, 1.6f);
    dl->AddRect(ImVec2(c.x - s * 0.55f, c.y - s),
                ImVec2(c.x - s * 0.20f, c.y - s * 0.30f),
                col, 1.0f, 0, 1.6f);
}

void DrawPushPull(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr float t = 1.6f;
    constexpr float ah = 3.5f;
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x - ah, c.y - s + ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x + ah, c.y - s + ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y + s), ImVec2(c.x - ah, c.y + s - ah), col, t);
    dl->AddLine(ImVec2(c.x, c.y + s), ImVec2(c.x + ah, c.y + s - ah), col, t);
}

void DrawInflate(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddCircle(c, s * 0.42f, col, 16, 1.6f);
    const float r1 = s * 0.62f;
    const float r2 = s * 0.98f;
    for (int i = 0; i < 8; ++i) {
        const float angle = static_cast<float>(i) * kPi * 0.25f;
        const float dx = std::cos(angle);
        const float dy = std::sin(angle);
        dl->AddLine(ImVec2(c.x + dx * r1, c.y + dy * r1),
                    ImVec2(c.x + dx * r2, c.y + dy * r2), col, 1.6f);
    }
}

void DrawSymmetry(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr float t = 1.6f;
    // Vertical mirror axis (dashed for visual weight).
    dl->AddLine(ImVec2(c.x, c.y - s),         ImVec2(c.x, c.y - s * 0.4f),  col, t);
    dl->AddLine(ImVec2(c.x, c.y - s * 0.15f), ImVec2(c.x, c.y + s * 0.15f), col, t);
    dl->AddLine(ImVec2(c.x, c.y + s * 0.4f),  ImVec2(c.x, c.y + s),         col, t);
    // Inward-pointing arrows on either side
    dl->AddLine(ImVec2(c.x - s,        c.y), ImVec2(c.x - s * 0.25f, c.y), col, t);
    dl->AddLine(ImVec2(c.x - s * 0.25f, c.y), ImVec2(c.x - s * 0.55f, c.y - 3.0f), col, t);
    dl->AddLine(ImVec2(c.x - s * 0.25f, c.y), ImVec2(c.x - s * 0.55f, c.y + 3.0f), col, t);
    dl->AddLine(ImVec2(c.x + s,        c.y), ImVec2(c.x + s * 0.25f, c.y), col, t);
    dl->AddLine(ImVec2(c.x + s * 0.25f, c.y), ImVec2(c.x + s * 0.55f, c.y - 3.0f), col, t);
    dl->AddLine(ImVec2(c.x + s * 0.25f, c.y), ImVec2(c.x + s * 0.55f, c.y + 3.0f), col, t);
}

void DrawPivot(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    constexpr float t = 1.6f;
    dl->AddCircleFilled(c, 2.2f, col);
    dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x - 4.0f, c.y), col, t);
    dl->AddLine(ImVec2(c.x + 4.0f, c.y), ImVec2(c.x + s, c.y), col, t);
    dl->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y - 4.0f), col, t);
    dl->AddLine(ImVec2(c.x, c.y + 4.0f), ImVec2(c.x, c.y + s), col, t);
}

void DrawHelp(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
    dl->AddCircle(c, s, col, 16, 1.6f);
    // Question mark hook — reverse-arc that ends pointing down.
    constexpr int N = 14;
    ImVec2 pts[N + 1];
    for (int i = 0; i <= N; ++i) {
        const float t = static_cast<float>(i) / N;
        const float angle = kPi * 1.30f - t * kPi * 1.40f;
        pts[i] = ImVec2(c.x + std::cos(angle) * s * 0.35f,
                        c.y + std::sin(angle) * s * 0.35f - s * 0.30f);
    }
    dl->AddPolyline(pts, N + 1, col, ImDrawFlags_None, 1.6f);
    dl->AddLine(pts[N], ImVec2(c.x, c.y + s * 0.10f), col, 1.6f);
    dl->AddCircleFilled(ImVec2(c.x, c.y + s * 0.50f), 1.3f, col);
}

// -----------------------------------------------------------------------------
// Tool descriptors

using IconFn = void (*)(ImDrawList*, ImVec2, float, ImU32);

struct Tool {
    const char* tooltip;
    const char* hotkey;   // nullptr → no hotkey badge
    int         kind;     // cast of TransformMode or BrushType depending on stage
    IconFn      icon;
};

constexpr Tool kStage1Tools[] = {
    { "Move (W)",   "W", static_cast<int>(TransformMode::Move),   &DrawMove   },
    { "Rotate (E)", "E", static_cast<int>(TransformMode::Rotate), &DrawRotate },
    { "Scale (R)",  "R", static_cast<int>(TransformMode::Scale),  &DrawScale  },
};

constexpr Tool kStage4Tools[] = {
    { "Smooth",    nullptr, static_cast<int>(BrushType::Smooth),   &DrawSmooth   },
    { "Grab",      nullptr, static_cast<int>(BrushType::Grab),     &DrawGrab     },
    { "Push/Pull", nullptr, static_cast<int>(BrushType::PushPull), &DrawPushPull },
    { "Inflate",   nullptr, static_cast<int>(BrushType::Inflate),  &DrawInflate  },
};

constexpr float kButtonSize = 40.0f;
constexpr float kButtonGap  = 4.0f;

// Returns true if the user clicked the button this frame. Consumes one
// 40 px-tall row at the supplied yPos within the rail's BeginChild.
bool RenderToolButton(ImDrawList* dl, ImVec2 railOrigin, float yPos,
                      const Tool& tool, bool active, bool stub) {
    const float xPad = (WidthPx - kButtonSize) * 0.5f;
    ImGui::SetCursorPos(ImVec2(xPad, yPos));
    const ImVec2 btnMin = ImGui::GetCursorScreenPos();
    const ImVec2 btnMax(btnMin.x + kButtonSize, btnMin.y + kButtonSize);

    ImGui::PushID(tool.tooltip);
    ImGui::BeginDisabled(stub);
    const bool clicked = ImGui::InvisibleButton("##tool", ImVec2(kButtonSize, kButtonSize));
    ImGui::EndDisabled();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    if (active) {
        dl->AddRectFilled(btnMin, btnMax, Col(MvStyle::MvBlue500), 6.0f);
    } else if (hovered && !stub) {
        dl->AddRectFilled(btnMin, btnMax, Col(MvStyle::MvBg3), 6.0f);
    }

    const ImVec2 iconCenter((btnMin.x + btnMax.x) * 0.5f,
                            (btnMin.y + btnMax.y) * 0.5f);
    ImU32 iconCol;
    if (active)      iconCol = IM_COL32(0x05, 0x13, 0x22, 0xFF); // dark on bright blue
    else if (stub)   iconCol = Col(MvStyle::MvFg4);
    else             iconCol = Col(MvStyle::MvFg2);
    if (tool.icon) tool.icon(dl, iconCenter, 9.0f, iconCol);

    if (tool.hotkey) {
        constexpr float BadgeW = 14.0f;
        constexpr float BadgeH = 12.0f;
        const ImVec2 badgeMin(btnMax.x - BadgeW - 2.0f, btnMax.y - BadgeH - 2.0f);
        const ImVec2 badgeMax(btnMax.x - 2.0f,           btnMax.y - 2.0f);
        if (active) {
            // No bg/border on active state — just the letter in dark.
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            const ImVec2 ts = ImGui::CalcTextSize(tool.hotkey);
            dl->AddText(ImVec2((badgeMin.x + badgeMax.x - ts.x) * 0.5f,
                               (badgeMin.y + badgeMax.y - ts.y) * 0.5f),
                        iconCol, tool.hotkey);
        } else {
            dl->AddRectFilled(badgeMin, badgeMax, Col(MvStyle::MvBg1), 2.0f);
            dl->AddRect(badgeMin, badgeMax, Col(MvStyle::MvLine2), 2.0f, 0, 1.0f);
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            const ImVec2 ts = ImGui::CalcTextSize(tool.hotkey);
            dl->AddText(ImVec2((badgeMin.x + badgeMax.x - ts.x) * 0.5f,
                               (badgeMin.y + badgeMax.y - ts.y) * 0.5f),
                        Col(MvStyle::MvFg3), tool.hotkey);
        }
    }

    if (hovered && !stub) {
        ImGui::SetTooltip("%s", tool.tooltip);
    }
    (void)railOrigin; // currently unused; reserved for future ImDrawList anchoring

    return clicked;
}

} // namespace

// -----------------------------------------------------------------------------
// Public entry point

Result Render(WorkflowStage stage, Project* project, Viewport* viewport,
              float height) {
    Result out;
    const bool isStage1 = (stage == WorkflowStage::Alignment);
    const bool isStage4 = (stage == WorkflowStage::TouchUp);
    if (!isStage1 && !isStage4) {
        return out;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, MvStyle::MvBg2);

    ImGui::BeginChild("##MvToolRail", ImVec2(WidthPx, height), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // 1 px right border so the rail visually separates from the dockspace.
    dl->AddLine(ImVec2(origin.x + WidthPx - 1.0f, origin.y),
                ImVec2(origin.x + WidthPx - 1.0f, origin.y + height),
                Col(MvStyle::MvLine2));

    // Eyebrow header — XFORM / BRUSH ----------------------------------------
    const char* eyebrowText = isStage1 ? "XFORM" : "BRUSH";
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const ImVec2 ts = ImGui::CalcTextSize(eyebrowText);
        ImGui::SetCursorPos(ImVec2((WidthPx - ts.x) * 0.5f, 12.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
        ImGui::TextUnformatted(eyebrowText);
        ImGui::PopStyleColor();
    }

    // Active tool / brush lookup --------------------------------------------
    int activeKind = -1;
    if (viewport) {
        activeKind = isStage1
            ? static_cast<int>(viewport->GetTransformMode())
            : static_cast<int>(viewport->GetBrushType());
    }

    // Stage-specific primary tool buttons -----------------------------------
    const Tool* tools     = isStage1 ? kStage1Tools : kStage4Tools;
    const int   toolCount = isStage1 ? 3            : 4;

    constexpr float HeaderHeight = 32.0f; // eyebrow + breathing room
    float y = HeaderHeight;
    for (int i = 0; i < toolCount; ++i) {
        const bool active = (tools[i].kind == activeKind);
        if (RenderToolButton(dl, origin, y, tools[i], active, /*stub=*/false) && viewport) {
            if (isStage1) {
                viewport->SetTransformMode(static_cast<TransformMode>(tools[i].kind));
            } else {
                const auto kind = static_cast<BrushType>(tools[i].kind);
                viewport->SetBrushType(kind);
                if (project) project->GetSculptState().activeBrush = kind;
            }
        }
        y += kButtonSize + kButtonGap;
    }

    // Divider between primary tools and stage-specific extras ---------------
    y += 6.0f;
    {
        constexpr float DividerW = 24.0f;
        dl->AddLine(ImVec2(origin.x + (WidthPx - DividerW) * 0.5f, origin.y + y),
                    ImVec2(origin.x + (WidthPx + DividerW) * 0.5f, origin.y + y),
                    Col(MvStyle::MvLine2));
    }
    y += 10.0f;

    // Stage-specific secondary buttons --------------------------------------
    if (isStage1) {
        // Stage 1 — Symmetry / Pivot are visual scaffolding; the underlying
        // transform-symmetry concept doesn't exist in the codebase yet.
        // Disabled with a "coming soon" tooltip so users know they're real.
        const Tool kSymStub  { "Symmetry X (coming in a future update)", nullptr,
                               0, &DrawSymmetry };
        const Tool kPivStub  { "Pivot snap (coming in a future update)", nullptr,
                               0, &DrawPivot };
        RenderToolButton(dl, origin, y, kSymStub, /*active=*/false, /*stub=*/true);
        y += kButtonSize + kButtonGap;
        RenderToolButton(dl, origin, y, kPivStub, /*active=*/false, /*stub=*/true);
        y += kButtonSize + kButtonGap;
    } else {
        // Stage 4 — Symmetry toggle wired to SculptState.symmetryEnabled.
        const bool sym = project && project->GetSculptState().symmetryEnabled;
        const Tool kSymTool { sym ? "Symmetry X (on)" : "Symmetry X (off)",
                              nullptr, 0, &DrawSymmetry };
        if (RenderToolButton(dl, origin, y, kSymTool, /*active=*/sym, /*stub=*/false)
            && project) {
            project->GetSculptState().symmetryEnabled = !sym;
        }
        y += kButtonSize + kButtonGap;
    }

    // Help button anchored to the bottom of the rail ------------------------
    {
        constexpr float HelpBottomPad = 12.0f;
        const float helpY = height - kButtonSize - HelpBottomPad;
        const Tool kHelpTool { "Keyboard shortcuts", nullptr, 0, &DrawHelp };
        if (RenderToolButton(dl, origin, helpY, kHelpTool,
                             /*active=*/false, /*stub=*/false)) {
            out.helpRequested = true;
        }
    }

    ImGui::EndChild();
    return out;
}

} // namespace MetaVisage::ToolRail
