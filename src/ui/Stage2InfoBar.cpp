#include "ui/Stage2InfoBar.h"

#include "core/Mesh.h"
#include "core/Project.h"
#include "ui/MvStyle.h"

#include <imgui.h>

#include <cstdio>
#include <filesystem>
#include <string>

namespace MetaVisage::Stage2InfoBar {

namespace {

inline ImU32 ColU(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

inline ImVec4 WithAlpha(const ImVec4& v, float a) {
    return ImVec4(v.x, v.y, v.z, a);
}

std::string FileName(const std::string& path) {
    if (path.empty()) return {};
    return std::filesystem::path(path).filename().string();
}

// Small "SOURCE" / "TARGET" badge pill. Tinted bg + border + foreground using
// the supplied accent colour. Returns the pill's right edge in screen X so
// the caller can SameLine the next element flush against it.
float DrawSidePill(ImDrawList* dl, ImVec2 pos, const char* label,
                   const ImVec4& accent) {
    MetaVisage::MvStyle::ScopedFont f(MetaVisage::MvStyle::Font.MonoSm);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    constexpr float padX = 8.0f;
    constexpr float padY = 3.0f;
    const float fs = ImGui::GetFontSize();
    const float pillW = ts.x + padX * 2.0f;
    const float pillH = fs   + padY * 2.0f;
    const ImVec2 pmin = pos;
    const ImVec2 pmax(pmin.x + pillW, pmin.y + pillH);
    dl->AddRectFilled(pmin, pmax, ColU(WithAlpha(accent, 0.06f)), 3.0f);
    dl->AddRect      (pmin, pmax, ColU(WithAlpha(accent, 0.40f)), 3.0f, 0, 1.0f);
    dl->AddText(ImVec2(pmin.x + padX, pmin.y + padY), ColU(accent), label);
    return pmax.x;
}

// Inline mono text segment in `color` at the given screen position. Returns
// the right edge so spans can be chained.
float DrawMonoText(ImDrawList* dl, ImVec2 pos, const char* text,
                   const ImVec4& color, ImFont* font, float size) {
    if (!font) return pos.x;
    const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    dl->AddText(font, size, pos, ColU(color), text);
    return pos.x + ts.x;
}

} // namespace

void Render(const Project* project, float x, float y, float w) {
    if (w <= 0.0f) return;

    char wid[64];
    std::snprintf(wid, sizeof(wid), "##Stage2InfoBar_%p",
                  static_cast<const void*>(project));

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, HeightPx));
    ImGui::SetNextWindowBgAlpha(1.0f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, MvStyle::MvBg1);

    if (ImGui::Begin(wid, nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetWindowPos();

        // Bottom hairline so the bar reads as a separator from the viewports.
        dl->AddLine(ImVec2(origin.x,        origin.y + HeightPx - 1.0f),
                    ImVec2(origin.x + w,    origin.y + HeightPx - 1.0f),
                    ColU(MvStyle::MvLine2));

        // Pull the data we need. Defaults are sensible when project is null
        // or meshes aren't loaded.
        std::string srcFile, tgtFile;
        int srcVerts = 0, tgtVerts = 0;
        int paired = 0, srcOnly = 0, tgtOnly = 0;
        if (project) {
            const auto& morph  = project->GetMorphMesh();
            const auto& target = project->GetTargetMesh();
            srcFile = FileName(morph.filepath);
            tgtFile = FileName(target.filepath);
            if (morph.isLoaded  && morph.mesh)  srcVerts = static_cast<int>(morph.mesh->GetVertices().size());
            if (target.isLoaded && target.mesh) tgtVerts = static_cast<int>(target.mesh->GetVertices().size());
            for (const auto& c : project->GetPointReferenceData().correspondences) {
                const bool hasM = c.morphMeshVertexIndex >= 0;
                const bool hasT = c.targetMeshVertexIndex >= 0;
                if (hasM && hasT) ++paired;
                else if (hasM)    ++srcOnly;
                else if (hasT)    ++tgtOnly;
            }
        }
        const int srcPts = paired + srcOnly;
        const int tgtPts = paired + tgtOnly;

        // Format strings up front so we can measure for left/right alignment.
        char srcMeta[128];
        std::snprintf(srcMeta, sizeof(srcMeta), "  %d verts \xC2\xB7 %d pts",
                      srcVerts, srcPts);
        char tgtMeta[128];
        std::snprintf(tgtMeta, sizeof(tgtMeta), "  %d verts \xC2\xB7 %d pts",
                      tgtVerts, tgtPts);
        char paireBuf[64];
        std::snprintf(paireBuf, sizeof(paireBuf), "%d pts paired", paired);

        // Vertical centring uses the body font's metrics so the labels share
        // a baseline regardless of which fonts are loaded.
        constexpr float SidePad = 14.0f;

        // Font sizes pulled from MvStyle::TypeSize so they stay in sync
        // with what was passed to AddFontFromFileTTF at LoadFonts time —
        // ImGui 1.92+ removed ImFont::FontSize (dynamic baking; sizes are
        // per-bake now) so probing the font at runtime no longer works.
        constexpr float BodyH   = MvStyle::TypeSize::Body;     // 14
        constexpr float MonoH   = MvStyle::TypeSize::Mono;     // 11
        constexpr float MonoSmH = MvStyle::TypeSize::MonoSm;   // 10

        ImFont* body   = MvStyle::Font.BodyMd;
        ImFont* mono   = MvStyle::Font.Mono;
        ImFont* monoSm = MvStyle::Font.MonoSm;

        // ---- Left side: SOURCE pill + filename + meta ------------------
        {
            const float yMid = origin.y + HeightPx * 0.5f;
            // Pill — small mono badge in MvWarning amber (Source).
            const float pillH = MonoSmH + 6.0f;
            const float pillX = origin.x + SidePad;
            const float pillY = yMid - pillH * 0.5f;
            const float pillRight = DrawSidePill(dl, ImVec2(pillX, pillY),
                                                 "SOURCE", MvStyle::MvWarning);

            // Filename — Body Md, MvFg1.
            float cx = pillRight + 10.0f;
            const float textY = yMid - BodyH * 0.5f;
            const char* nameText = srcFile.empty() ? "\xE2\x80\x94" : srcFile.c_str();
            cx = DrawMonoText(dl, ImVec2(cx, textY), nameText,
                              MvStyle::MvFg1, body, BodyH);

            // Meta — mono, MvFg3.
            const float monoY = yMid - MonoH * 0.5f;
            DrawMonoText(dl, ImVec2(cx, monoY), srcMeta,
                         MvStyle::MvFg3, mono, MonoH);
        }

        // ---- Right side: TARGET pill + filename + meta -----------------
        // Compute right cluster width first so we can right-align it.
        {
            const float yMid = origin.y + HeightPx * 0.5f;
            const float pillH = MonoSmH + 6.0f;
            const ImVec2 pillTs = monoSm
                ? monoSm->CalcTextSizeA(MonoSmH, FLT_MAX, 0.0f, "TARGET")
                : ImGui::CalcTextSize("TARGET");
            const float pillW = pillTs.x + 16.0f;

            const char* nameText = tgtFile.empty() ? "\xE2\x80\x94" : tgtFile.c_str();
            const ImVec2 nameSize = body
                ? body->CalcTextSizeA(BodyH, FLT_MAX, 0.0f, nameText)
                : ImGui::CalcTextSize(nameText);
            const ImVec2 metaSize = mono
                ? mono->CalcTextSizeA(MonoH, FLT_MAX, 0.0f, tgtMeta)
                : ImGui::CalcTextSize(tgtMeta);

            const float clusterW = pillW + 10.0f + nameSize.x + metaSize.x;
            const float clusterRight = origin.x + w - SidePad;
            const float clusterLeft  = clusterRight - clusterW;

            const float pillY = yMid - pillH * 0.5f;
            DrawSidePill(dl, ImVec2(clusterLeft, pillY),
                         "TARGET", MvStyle::MvBlue400);

            float cx = clusterLeft + pillW + 10.0f;
            const float textY = yMid - BodyH * 0.5f;
            cx = DrawMonoText(dl, ImVec2(cx, textY), nameText,
                              MvStyle::MvFg1, body, BodyH);
            const float monoY = yMid - MonoH * 0.5f;
            DrawMonoText(dl, ImVec2(cx, monoY), tgtMeta,
                         MvStyle::MvFg3, mono, MonoH);
        }

        // ---- Centred paired indicator ---------------------------------
        {
            const ImVec2 ts = mono
                ? mono->CalcTextSizeA(MonoH, FLT_MAX, 0.0f, paireBuf)
                : ImGui::CalcTextSize(paireBuf);
            const float yMid = origin.y + HeightPx * 0.5f;
            const float cx = origin.x + (w - ts.x) * 0.5f;
            const ImVec4 col = paired > 0 ? MvStyle::MvBlue400 : MvStyle::MvFg4;
            DrawMonoText(dl, ImVec2(cx, yMid - MonoH * 0.5f),
                         paireBuf, col, mono, MonoH);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

} // namespace MetaVisage::Stage2InfoBar
