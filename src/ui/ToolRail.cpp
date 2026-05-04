#include "ui/ToolRail.h"

#include "licensing/LicenseManager.h"
#include "ui/MvIcons.h"
#include "ui/MvStyle.h"
#include "ui/Viewport.h"
#include "core/Project.h"
#include "core/Types.h"

#include <imgui.h>

#include <cstdio>
#include <iterator>

namespace MetaVisage::ToolRail {

// =============================================================================
// Icons — sourced from the Lucide font (assets/fonts/lucide.ttf) via the
// MV_ICON_* glyph constants in ui/MvIcons.h. Drawn through MvStyle::Font.Icon
// which is a 16 px-baked Lucide rasterization. The earlier ImDrawList
// primitive-drawing path has been retired now that the font ships.
// =============================================================================
namespace {

ImU32 Col(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

// -----------------------------------------------------------------------------
// Tool descriptors

struct Tool {
    const char* tooltip;
    const char* hotkey;   // nullptr → no hotkey badge
    int         kind;     // cast of TransformMode or BrushType depending on stage
    const char* glyph;    // UTF-8 Lucide glyph from MvIcons.h
};

constexpr Tool kStage1Tools[] = {
    { "Move (W)",   "W", static_cast<int>(TransformMode::Move),   MV_ICON_MOVE_3D    },
    { "Rotate (E)", "E", static_cast<int>(TransformMode::Rotate), MV_ICON_ROTATE_3D  },
    { "Scale (R)",  "R", static_cast<int>(TransformMode::Scale),  MV_ICON_SCALE_3D   },
};

constexpr Tool kStage4Tools[] = {
    // Brush mappings (informed by lucide.dev visual review):
    //   Smooth    → paintbrush       (sweeping stroke)
    //   Grab      → hand              (grasping cursor metaphor)
    //   Push/Pull → move-vertical     (bidirectional vertical arrow)
    //   Inflate   → expand            (outward radial arrows)
    { "Smooth",    nullptr, static_cast<int>(BrushType::Smooth),   MV_ICON_BRUSH         },
    { "Grab",      nullptr, static_cast<int>(BrushType::Grab),     MV_ICON_HAND          },
    { "Push/Pull", nullptr, static_cast<int>(BrushType::PushPull), MV_ICON_MOVE_VERTICAL },
    { "Inflate",   nullptr, static_cast<int>(BrushType::Inflate),  MV_ICON_EXPAND        },
};

// PR #24 review nit — compile-time guard against the implicit ordering
// coupling between kStage4Tools and MvStyle::Asset.BrushPreviewTextureIds.
// The Render() loop indexes BrushPreviewTextureIds[i] by the same `i`
// that walks kStage4Tools; if either array's size drifts, the next
// build fails loudly instead of the toolrail silently mis-mapping (or
// out-of-bounds reading) brush thumbnails.
static_assert(std::size(kStage4Tools) == std::size(MvStyle::Asset.BrushPreviewTextureIds),
              "kStage4Tools and MvStyle::Asset.BrushPreviewTextureIds must "
              "stay aligned in size and brush order");

constexpr float kButtonSize = 40.0f;
constexpr float kButtonGap  = 4.0f;

// Returns true if the user clicked the button this frame. Consumes one
// 40 px-tall row at the supplied yPos within the rail's BeginChild.
//
// `previewTex` is optional (Story 5.3 follow-up): when non-zero, an RGBA
// texture is rendered in place of the Lucide glyph at 32 px centred in
// the 40 px button. Active-state rendering switches from a filled blue
// rectangle to a blue border ring — the PNG mesh+arrow shouldn't fight
// an MvBlue500 overlay (the arrow itself is MvBlue500 and would
// disappear into a flat-blue button bg). Used by the Stage 4 brush
// rail to mirror the brush-preview thumbnails from the sidebar cards.
bool RenderToolButton(ImDrawList* dl, ImVec2 railOrigin, float yPos,
                      const Tool& tool, bool active, bool stub,
                      ImTextureID previewTex = 0) {
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
        if (previewTex) {
            // Border-ring active treatment for preview buttons — keeps
            // the PNG legible against the rail's normal MvBg2 fill while
            // still signalling "selected" via the blue stroke. Matches
            // the inspector preset card's active style at
            // StageSidebar.cpp::DrawPresetCard.
            dl->AddRectFilled(btnMin, btnMax, Col(MvStyle::MvBg2), 6.0f);
            dl->AddRect(btnMin, btnMax, Col(MvStyle::MvBlue500), 6.0f, 0, 1.5f);
        } else {
            dl->AddRectFilled(btnMin, btnMax, Col(MvStyle::MvBlue500), 6.0f);
        }
    } else if (hovered && !stub) {
        dl->AddRectFilled(btnMin, btnMax, Col(MvStyle::MvBg3), 6.0f);
    }

    const ImVec2 iconCenter((btnMin.x + btnMax.x) * 0.5f,
                            (btnMin.y + btnMax.y) * 0.5f);
    ImU32 iconCol;
    if (active && !previewTex) iconCol = IM_COL32(0x05, 0x13, 0x22, 0xFF); // dark on bright blue
    else if (stub)             iconCol = Col(MvStyle::MvFg4);
    else                       iconCol = Col(MvStyle::MvFg2);

    if (previewTex) {
        // PNG preview path. Renders at 32 px centred — keeps a 4 px
        // breathing margin to the 40 px button edge so the active
        // border ring stays readable.
        constexpr float kPreviewSize = 32.0f;
        const ImVec2 imgMin(iconCenter.x - kPreviewSize * 0.5f,
                            iconCenter.y - kPreviewSize * 0.5f);
        const ImVec2 imgMax(imgMin.x + kPreviewSize, imgMin.y + kPreviewSize);
        const ImU32 tint = stub ? IM_COL32(255, 255, 255, 128) : IM_COL32_WHITE;
        dl->AddImage(previewTex, imgMin, imgMax,
                     ImVec2(0, 0), ImVec2(1, 1), tint);
    } else if (tool.glyph && MvStyle::Font.Icon) {
        // Draw the Lucide glyph centred in the button. The font is
        // baked at 16 px (MvStyle::TypeSize::* — see MvStyle.cpp);
        // CalcTextSizeA measures the rasterized glyph so we can centre
        // exactly. Falls back to nothing if the icon font failed to
        // load — the button still takes its hit area, just without
        // the glyph.
        constexpr float IconSize = 18.0f;
        ImFont* font = MvStyle::Font.Icon;
        const ImVec2 ts = font->CalcTextSizeA(IconSize, FLT_MAX, 0.0f, tool.glyph);
        dl->AddText(font, IconSize,
                    ImVec2(iconCenter.x - ts.x * 0.5f,
                           iconCenter.y - ts.y * 0.5f),
                    iconCol, tool.glyph);
    }

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

    // Story 7.1 gate -- Stage 4 touchup_tools tier mapping. Mirrors the
    // gating in StageSidebar's brush-card grid; both surfaces must be
    // gated together (architect audit observation).
    const auto touchUpToolset =
        MetaVisage::LicenseManager::Instance().GetTouchUpToolset();
    auto isBrushAllowed = [&](BrushType b) {
        switch (touchUpToolset) {
            case FeatureFlags::TouchUpToolset::Full:
                return true;
            case FeatureFlags::TouchUpToolset::Basic:
                return b == BrushType::Smooth || b == BrushType::Grab;
            case FeatureFlags::TouchUpToolset::BasicSmooth:
            default:
                return b == BrushType::Smooth;
        }
    };

    constexpr float HeaderHeight = 32.0f; // eyebrow + breathing room
    float y = HeaderHeight;
    for (int i = 0; i < toolCount; ++i) {
        const bool active = (tools[i].kind == activeKind);
        // Story 5.3 follow-up — when on Stage 4, the loop iteration
        // index `i` aligns with MvStyle::Asset.BrushPreviewTextureIds[]
        // (the kStage4Tools array is ordered Smooth/Grab/PushPull/
        // Inflate to match the texture array). Zero handle means
        // "load failed" and falls back to the Lucide glyph.
        ImTextureID previewTex = 0;
        if (isStage4 && MvStyle::Asset.BrushPreviewTextureIds[i]) {
            previewTex = (ImTextureID)(intptr_t)
                         MvStyle::Asset.BrushPreviewTextureIds[i];
        }
        // Story 7.1 -- gate-disabled Stage 4 brushes render via the
        // existing `stub=true` path so they look like the Stage 1
        // "coming soon" tools. Stage 1 transform tools are always
        // enabled (no tier gating).
        const bool gated = isStage4 &&
            !isBrushAllowed(static_cast<BrushType>(tools[i].kind));
        if (RenderToolButton(dl, origin, y, tools[i], active,
                             /*stub=*/gated, previewTex)
            && viewport && !gated) {
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
                               0, MV_ICON_FLIP_HORIZ };
        const Tool kPivStub  { "Pivot snap (coming in a future update)", nullptr,
                               0, MV_ICON_CROSSHAIR };
        RenderToolButton(dl, origin, y, kSymStub, /*active=*/false, /*stub=*/true);
        y += kButtonSize + kButtonGap;
        RenderToolButton(dl, origin, y, kPivStub, /*active=*/false, /*stub=*/true);
        y += kButtonSize + kButtonGap;
    } else {
        // Stage 4 — Symmetry toggle wired to SculptState.symmetryEnabled.
        const bool sym = project && project->GetSculptState().symmetryEnabled;
        const Tool kSymTool { sym ? "Symmetry X (on)" : "Symmetry X (off)",
                              nullptr, 0, MV_ICON_FLIP_HORIZ };
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
        const Tool kHelpTool { "Keyboard shortcuts", nullptr, 0, MV_ICON_HELP };
        if (RenderToolButton(dl, origin, helpY, kHelpTool,
                             /*active=*/false, /*stub=*/false)) {
            out.helpRequested = true;
        }
    }

    ImGui::EndChild();
    return out;
}

} // namespace MetaVisage::ToolRail
