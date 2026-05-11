#include "ui/Modals.h"
#include "licensing/LicenseManager.h"
#include "ui/MvIcons.h"
#include "ui/MvStyle.h"
#include "utils/Config.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>

namespace MetaVisage {

namespace {

// Open the popup if `pendingOpen` was set this frame. Callers MUST wrap their
// ImGui::BeginPopupModal in a block gated by the returned bool.
void HandlePendingOpen(const char* imguiId, bool& pendingOpen) {
    if (pendingOpen) {
        ImGui::OpenPopup(imguiId);
        pendingOpen = false;
    }
}

void RenderKeyValueRow(const char* key, const char* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(key);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(value);
}

} // namespace

// ---------------------------------------------------------------------------
// Confirm
// ---------------------------------------------------------------------------

void ConfirmModalState::Open(const std::string& t, const std::string& msg,
                             std::function<void()> confirm,
                             const std::string& confirmLbl,
                             const std::string& cancelLbl) {
    title = t;
    message = msg;
    onConfirm = std::move(confirm);
    confirmLabel = confirmLbl;
    cancelLabel = cancelLbl;
    pendingOpen = true;
}

void RenderConfirmModal(ConfirmModalState& state) {
    HandlePendingOpen("##ConfirmModal", state.pendingOpen);
    // PR #25 review feedback fix (round 2): the confirm popup is
    // ImGuiWindowFlags_AlwaysAutoResize, so its width/height aren't
    // known on the first frame after OpenPopup. With Cond_Appearing,
    // the pivot calculation `pos - size * 0.5` runs once with size=0
    // and places the window's top-left at center -- the user sees
    // the popup hugging the lower-right of the screen midpoint.
    // Switching to Cond_Always re-applies the centered pivot every
    // frame using the current measured size, so by frame 2 the popup
    // is genuinely centered. No perf concern for a small fixed popup.
    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("##ConfirmModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(state.title.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", state.message.c_str());
        ImGui::Spacing();
        if (ImGui::Button(state.confirmLabel.c_str(), ImVec2(120, 0))) {
            if (state.onConfirm) state.onConfirm();
            state.onConfirm = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(state.cancelLabel.c_str(), ImVec2(120, 0))) {
            state.onConfirm = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// Error
// ---------------------------------------------------------------------------

void ErrorModalState::Open(const std::string& msg, const std::string& t) {
    title = t;
    message = msg;
    isError = true;
    pendingOpen = true;
}

void ErrorModalState::OpenInfo(const std::string& msg, const std::string& t) {
    title = t;
    message = msg;
    isError = false;
    pendingOpen = true;
}

void RenderErrorModal(ErrorModalState& state) {
    HandlePendingOpen("##ErrorModal", state.pendingOpen);
    if (ImGui::BeginPopupModal("##ErrorModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const ImVec4 errorColor(0.95f, 0.45f, 0.45f, 1.0f);
        const ImVec4 infoColor (0.40f, 0.85f, 0.50f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, state.isError ? errorColor : infoColor);
        ImGui::TextUnformatted(state.title.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::TextWrapped("%s", state.message.c_str());
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// Welcome
// ---------------------------------------------------------------------------

void WelcomeModalState::Open() {
    pendingOpen = true;
    action = WelcomeAction::None;
    selectedRecent.clear();
}

namespace {

// Format a filesystem::last_write_time as "N hours ago" / "yesterday" /
// "N days ago" — relative-time string for the recent-projects list.
std::string FormatRelativeTime(const std::filesystem::path& path) {
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec) return "unknown";

    using namespace std::chrono;
    // file_clock to system_clock conversion is C++20; fall back to a
    // direct cast through duration_cast for the elapsed window we care
    // about (relative seconds, not absolute wall time).
    auto sftime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    auto now = system_clock::now();
    auto delta = duration_cast<seconds>(now - sftime).count();

    if (delta < 60)            return "just now";
    if (delta < 60 * 60)       { char b[32]; std::snprintf(b, sizeof(b), "%lld min ago", static_cast<long long>(delta / 60)); return b; }
    if (delta < 60 * 60 * 24)  { char b[32]; std::snprintf(b, sizeof(b), "%lld hr ago",  static_cast<long long>(delta / 3600)); return b; }
    if (delta < 60 * 60 * 48)  return "yesterday";
    if (delta < 60 * 60 * 24 * 7) {
        char b[32]; std::snprintf(b, sizeof(b), "%lld days ago",
                                  static_cast<long long>(delta / (60 * 60 * 24)));
        return b;
    }
    char b[32]; std::snprintf(b, sizeof(b), "%lld weeks ago",
                              static_cast<long long>(delta / (60 * 60 * 24 * 7)));
    return b;
}

// "184.2 MB" / "12 KB" / "1.4 GB" — size as a single mono span.
std::string FormatFileSize(const std::filesystem::path& path) {
    std::error_code ec;
    const auto bytes = std::filesystem::file_size(path, ec);
    if (ec) return "—";
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    char buf[32];
    if (bytes >= GB)      std::snprintf(buf, sizeof(buf), "%.1f GB", static_cast<double>(bytes) / GB);
    else if (bytes >= MB) std::snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / MB);
    else if (bytes >= KB) std::snprintf(buf, sizeof(buf), "%.0f KB", static_cast<double>(bytes) / KB);
    else                  std::snprintf(buf, sizeof(buf), "%llu B",  static_cast<unsigned long long>(bytes));
    return buf;
}

inline ImU32 ColU(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }
inline ImVec4 WithAlpha(const ImVec4& v, float a) { return ImVec4(v.x, v.y, v.z, a); }

// Big-action button used twice in the header: New Project (primary blue
// tint) and Open Project... (ghost). Returns true on click. Owns its own
// PushID so the two calls don't collide.
bool DrawWelcomeAction(const char* idStr, const char* iconGlyph,
                       const char* label, const char* sub,
                       bool isPrimary) {
    constexpr float CardH = 60.0f;
    ImGui::PushID(idStr);
    const ImVec2 pmin = ImGui::GetCursorScreenPos();
    const float availW = ImGui::GetContentRegionAvail().x;
    const ImVec2 pmax(pmin.x + availW, pmin.y + CardH);
    const bool clicked = ImGui::InvisibleButton("##wact", ImVec2(availW, CardH));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (isPrimary) {
        ImVec4 fill = MetaVisage::MvStyle::MvBlue500;
        fill.w = hovered ? 0.18f : 0.10f;
        dl->AddRectFilled(pmin, pmax, ColU(fill), 6.0f);
        dl->AddRect(pmin, pmax,
                    ColU(MetaVisage::MvStyle::MvBlue500),
                    6.0f, 0, 1.0f);
    } else {
        const ImU32 bg = hovered
            ? ColU(MetaVisage::MvStyle::MvBg3)
            : ColU(MetaVisage::MvStyle::MvBg1);
        dl->AddRectFilled(pmin, pmax, bg, 6.0f);
        dl->AddRect(pmin, pmax,
                    ColU(MetaVisage::MvStyle::MvLine2),
                    6.0f, 0, 1.0f);
    }

    // 36×36 icon container on the left.
    constexpr float IconBoxSize = 36.0f;
    const ImVec2 iconMin(pmin.x + 14.0f, pmin.y + (CardH - IconBoxSize) * 0.5f);
    const ImVec2 iconMax(iconMin.x + IconBoxSize, iconMin.y + IconBoxSize);
    const ImVec4 iconBg = isPrimary
        ? WithAlpha(MetaVisage::MvStyle::MvBlue500, 0.18f)
        : MetaVisage::MvStyle::MvBg2;
    dl->AddRectFilled(iconMin, iconMax, ColU(iconBg), 6.0f);
    dl->AddRect(iconMin, iconMax,
                ColU(isPrimary ? MetaVisage::MvStyle::MvBlue400
                               : MetaVisage::MvStyle::MvLine2),
                6.0f, 0, 1.0f);
    if (iconGlyph && MetaVisage::MvStyle::Font.Icon) {
        ImFont* iconFont = MetaVisage::MvStyle::Font.Icon;
        constexpr float iconSize = 18.0f;
        const ImVec2 ts = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, iconGlyph);
        const ImU32 iconCol = ColU(isPrimary
            ? MetaVisage::MvStyle::MvBlue400
            : MetaVisage::MvStyle::MvFg2);
        dl->AddText(iconFont, iconSize,
                    ImVec2((iconMin.x + iconMax.x - ts.x) * 0.5f,
                           (iconMin.y + iconMax.y - ts.y) * 0.5f),
                    iconCol, iconGlyph);
    }

    // Label + sub-label stack.
    const float textX = iconMax.x + 12.0f;
    {
        MetaVisage::MvStyle::ScopedFont f(MetaVisage::MvStyle::Font.BodyMd);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(textX, pmin.y + (CardH * 0.5f) - fs - 1.0f),
                    ColU(MetaVisage::MvStyle::MvFg1), label);
    }
    if (sub && sub[0]) {
        MetaVisage::MvStyle::ScopedFont f(MetaVisage::MvStyle::Font.MonoSm);
        dl->AddText(ImVec2(textX, pmin.y + (CardH * 0.5f) + 3.0f),
                    ColU(MetaVisage::MvStyle::MvFg3), sub);
    }

    // Right chevron.
    if (MetaVisage::MvStyle::Font.IconSm) {
        ImFont* iconFont = MetaVisage::MvStyle::Font.IconSm;
        constexpr float chevSize = 14.0f;
        const ImVec2 ts = iconFont->CalcTextSizeA(chevSize, FLT_MAX, 0.0f,
                                                  MV_ICON_CHEVRON_RIGHT);
        const ImU32 chevCol = ColU(isPrimary
            ? MetaVisage::MvStyle::MvBlue400
            : MetaVisage::MvStyle::MvFg3);
        dl->AddText(iconFont, chevSize,
                    ImVec2(pmax.x - ts.x - 12.0f,
                           pmin.y + (CardH - ts.y) * 0.5f),
                    chevCol, MV_ICON_CHEVRON_RIGHT);
    }

    return clicked;
}

} // namespace

void RenderWelcomeModal(WelcomeModalState& state) {
    HandlePendingOpen("Welcome to MetaVisage", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, MvStyle::MvBg2);
    ImGui::PushStyleColor(ImGuiCol_Border,  MvStyle::MvLine3);

    if (ImGui::BeginPopupModal("Welcome to MetaVisage", nullptr,
                               ImGuiWindowFlags_NoSavedSettings |
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoTitleBar |
                               ImGuiWindowFlags_NoResize)) {
        state.isOpen = true;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        constexpr float ModalW = 560.0f;
        const ImVec2 wpos = ImGui::GetWindowPos();

        // ---- Header — sized to fit subtitle + tagline -------------------
        // Bumped from 92 → 112 px to make room for the subtitle below the
        // Saira heading per user feedback.
        constexpr float HeaderH = 112.0f;
        constexpr float SidePad = 28.0f;
        {
            const ImVec2 hMin = wpos;
            const ImVec2 hMax(wpos.x + ModalW, wpos.y + HeaderH);
            // Subtle radial-style highlight via two-tone fill — frosted dark
            // navy at the bottom blending up into MvBg2.
            dl->AddRectFilled(hMin, hMax, ColU(MvStyle::MvBg2));
            dl->AddLine(ImVec2(hMin.x, hMax.y - 1.0f),
                        ImVec2(hMax.x, hMax.y - 1.0f),
                        ColU(MvStyle::MvLine2));

            // 56×56 app-mark backdrop. The bundled logo PNG is itself a
            // blue silhouette, so the previous MvBlue500→MvBlue700 gradient
            // made the mark disappear into the box. Use a dark navy
            // backdrop (MvBg3 → MvBg4 diagonal) with a thin MvBlue400 hair
            // border — the bright-blue logo pops against the dark surface.
            constexpr float MarkSize = 56.0f;
            const ImVec2 mMin(hMin.x + SidePad, hMin.y + (HeaderH - MarkSize) * 0.5f);
            const ImVec2 mMax(mMin.x + MarkSize, mMin.y + MarkSize);
            dl->AddRectFilledMultiColor(mMin, mMax,
                ColU(MvStyle::MvBg3), ColU(MvStyle::MvBg3),
                ColU(MvStyle::MvBg4), ColU(MvStyle::MvBg4));
            dl->AddRect(mMin, mMax,
                        ColU(MvStyle::MvBlue400),
                        8.0f, 0, 1.0f);
            if (MvStyle::Asset.LogoTextureId) {
                constexpr float LogoSize = 36.0f;
                const ImVec2 logoMin((mMin.x + mMax.x - LogoSize) * 0.5f,
                                     (mMin.y + mMax.y - LogoSize) * 0.5f);
                const ImVec2 logoMax(logoMin.x + LogoSize, logoMin.y + LogoSize);
                dl->AddImage((ImTextureID)(intptr_t)MvStyle::Asset.LogoTextureId,
                             logoMin, logoMax);
            }

            // Right side text block — eyebrow, heading, subtitle.
            const float textX = mMax.x + 18.0f;
            {
                MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
                dl->AddText(ImVec2(textX, hMin.y + 24.0f),
                            ColU(MvStyle::MvBlue400),
                            "V 2.0.0 \xC2\xB7 EARLY ACCESS");
            }
            {
                MvStyle::ScopedFont f(MvStyle::Font.Display);
                // "Welcome to" in MvFg1 + "MetaVisage" in MvBlue400 — split
                // span so the colour switch lands mid-sentence.
                const char* prefix = "Welcome to ";
                const ImVec2 ts1 = ImGui::CalcTextSize(prefix);
                dl->AddText(ImVec2(textX, hMin.y + 40.0f),
                            ColU(MvStyle::MvFg1), prefix);
                dl->AddText(ImVec2(textX + ts1.x, hMin.y + 40.0f),
                            ColU(MvStyle::MvBlue400), "MetaVisage");
            }
            {
                // Subtitle — wraps below the Saira heading, BodySm MvFg2.
                MvStyle::ScopedFont f(MvStyle::Font.BodySm);
                dl->AddText(ImVec2(textX, hMin.y + 76.0f),
                            ColU(MvStyle::MvFg2),
                            "MetaHuman mesh morphing tool");
            }

            // Top-right close button.
            constexpr float CloseW = 28.0f;
            const ImVec2 closeMin(hMax.x - CloseW - 10.0f, hMin.y + 10.0f);
            ImGui::SetCursorScreenPos(closeMin);
            ImGui::PushID("##wcloseBtn");
            if (ImGui::InvisibleButton("##c", ImVec2(CloseW, CloseW))) {
                ImGui::CloseCurrentPopup();
            }
            const bool closeHovered = ImGui::IsItemHovered();
            ImGui::PopID();
            const ImU32 closeCol = closeHovered
                ? ColU(MvStyle::MvFg1)
                : ColU(MvStyle::MvFg3);
            if (MvStyle::Font.IconSm) {
                ImFont* iconFont = MvStyle::Font.IconSm;
                constexpr float xSize = 14.0f;
                const ImVec2 ts = iconFont->CalcTextSizeA(xSize, FLT_MAX, 0.0f, MV_ICON_X);
                dl->AddText(iconFont, xSize,
                            ImVec2(closeMin.x + (CloseW - ts.x) * 0.5f,
                                   closeMin.y + (CloseW - ts.y) * 0.5f),
                            closeCol, MV_ICON_X);
            }
        }
        // Advance the cursor past the header so the rest of the body lands
        // below it.
        ImGui::SetCursorScreenPos(ImVec2(wpos.x, wpos.y + HeaderH));

        // ---- Action row (2-column grid) ---------------------------------
        ImGui::Dummy(ImVec2(0.0f, 18.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SidePad, 0.0f));
        ImGui::Indent(SidePad);
        // Explicit outer_size with negative right offset = "available width
        // minus SidePad" — keeps the right edge of the Open Project card
        // from hitting the modal border (Indent only pads the left side).
        if (ImGui::BeginTable("##welcomeActions", 2,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadOuterX,
                              ImVec2(-SidePad, 0.0f))) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (DrawWelcomeAction("new", MV_ICON_FILE,
                                  "New project",
                                  "Start a fresh morph",
                                  /*isPrimary=*/true)) {
                state.action = WelcomeAction::NewProject;
                ImGui::CloseCurrentPopup();
            }

            ImGui::TableNextColumn();
            ImGui::Dummy(ImVec2(8.0f, 0.0f));
            ImGui::SameLine();
            if (DrawWelcomeAction("open", MV_ICON_FOLDER,
                                  "Open project\xE2\x80\xA6",
                                  "Continue from a .mmproj",
                                  /*isPrimary=*/false)) {
                state.action = WelcomeAction::OpenProject;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(SidePad);
        ImGui::PopStyleVar();

        // ---- Recent projects --------------------------------------------
        ImGui::Dummy(ImVec2(0.0f, 14.0f));
        ImGui::Indent(SidePad);
        {
            const auto& recent = Config::Instance().RecentProjects();

            // Section label row — mono uppercase + file count on the right.
            const ImVec2 lblPos = ImGui::GetCursorScreenPos();
            const float lblW = ImGui::GetContentRegionAvail().x - SidePad;
            constexpr float DotSize = 6.0f;
            {
                MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
                const float fs = ImGui::GetFontSize();
                // Blue square dot prefix.
                const float dotY = lblPos.y + (fs - DotSize) * 0.5f + 1.0f;
                dl->AddRectFilled(ImVec2(lblPos.x, dotY),
                                  ImVec2(lblPos.x + DotSize, dotY + DotSize),
                                  ColU(MvStyle::MvBlue500));
                dl->AddText(ImVec2(lblPos.x + DotSize + 8.0f, lblPos.y),
                            ColU(MvStyle::MvFg3), "RECENT PROJECTS");

                char fileCount[32];
                std::snprintf(fileCount, sizeof(fileCount), "%zu files",
                              recent.size());
                const ImVec2 fcSize = ImGui::CalcTextSize(fileCount);
                dl->AddText(ImVec2(lblPos.x + lblW - fcSize.x, lblPos.y),
                            ColU(MvStyle::MvFg3), fileCount);
                ImGui::Dummy(ImVec2(lblW, fs));
            }
            ImGui::Dummy(ImVec2(0.0f, 8.0f));

            if (recent.empty()) {
                MvStyle::ScopedFont f(MvStyle::Font.BodySm);
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
                ImGui::TextWrapped("No recent projects yet — open or save "
                                   "a project to populate this list.");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, MvStyle::MvBg1);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                // Cap visible rows at ~5 (≈220 px) so the modal stays compact.
                constexpr float RowH = 44.0f;
                const float visibleRows = std::min<float>(5.0f,
                    static_cast<float>(recent.size()));
                const float listH = visibleRows * RowH + 4.0f;

                if (ImGui::BeginChild("##recent_projects",
                                      ImVec2(-SidePad, listH),
                                      true,
                                      ImGuiWindowFlags_None)) {
                    ImDrawList* listDl = ImGui::GetWindowDrawList();
                    for (size_t i = 0; i < recent.size(); ++i) {
                        const std::filesystem::path p(recent[i]);
                        const std::string filename = p.filename().string();
                        const std::string when = FormatRelativeTime(p);
                        const std::string sizeStr = FormatFileSize(p);

                        ImGui::PushID(static_cast<int>(i));
                        const ImVec2 rowMin = ImGui::GetCursorScreenPos();
                        const float rowW = ImGui::GetContentRegionAvail().x;
                        const ImVec2 rowMax(rowMin.x + rowW, rowMin.y + RowH);
                        const bool clicked = ImGui::InvisibleButton(
                            "##rrow", ImVec2(rowW, RowH));
                        const bool hovered = ImGui::IsItemHovered();
                        if (hovered) {
                            ImGui::SetTooltip("%s", recent[i].c_str());
                        }
                        ImGui::PopID();

                        if (i > 0) {
                            listDl->AddLine(ImVec2(rowMin.x, rowMin.y),
                                            ImVec2(rowMax.x, rowMin.y),
                                            ColU(MvStyle::MvLine2));
                        }
                        if (hovered) {
                            ImVec4 bg = MvStyle::MvBlue500; bg.w = 0.06f;
                            listDl->AddRectFilled(rowMin, rowMax, ColU(bg));
                        }

                        // 28×28 file icon container.
                        constexpr float IconSlot = 28.0f;
                        const ImVec2 iconMin(rowMin.x + 12.0f,
                                             rowMin.y + (RowH - IconSlot) * 0.5f);
                        const ImVec2 iconMax(iconMin.x + IconSlot, iconMin.y + IconSlot);
                        listDl->AddRectFilled(iconMin, iconMax,
                                              ColU(hovered
                                                  ? WithAlpha(MvStyle::MvBlue500, 0.12f)
                                                  : MvStyle::MvBg2),
                                              4.0f);
                        listDl->AddRect(iconMin, iconMax,
                                        ColU(MvStyle::MvLine2), 4.0f, 0, 1.0f);
                        if (MvStyle::Font.IconSm) {
                            ImFont* iconFont = MvStyle::Font.IconSm;
                            constexpr float iconSize = 13.0f;
                            const ImVec2 ts = iconFont->CalcTextSizeA(
                                iconSize, FLT_MAX, 0.0f, MV_ICON_FILE_TEXT);
                            const ImU32 iconCol = ColU(hovered
                                ? MvStyle::MvBlue400
                                : MvStyle::MvFg3);
                            listDl->AddText(iconFont, iconSize,
                                            ImVec2((iconMin.x + iconMax.x - ts.x) * 0.5f,
                                                   (iconMin.y + iconMax.y - ts.y) * 0.5f),
                                            iconCol, MV_ICON_FILE_TEXT);
                        }

                        // Filename + sub-line stack.
                        const float textX = iconMax.x + 12.0f;
                        {
                            MvStyle::ScopedFont f(MvStyle::Font.Mono);
                            const float fs = ImGui::GetFontSize();
                            listDl->AddText(ImVec2(textX, rowMin.y + (RowH * 0.5f) - fs - 1.0f),
                                            ColU(MvStyle::MvFg1),
                                            filename.c_str());
                        }
                        {
                            char metaBuf[128];
                            std::snprintf(metaBuf, sizeof(metaBuf), "%s \xC2\xB7 %s",
                                          when.c_str(), sizeStr.c_str());
                            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
                            listDl->AddText(ImVec2(textX, rowMin.y + (RowH * 0.5f) + 3.0f),
                                            ColU(MvStyle::MvFg3),
                                            metaBuf);
                        }

                        // Right chevron — MvFg3 dim until hovered.
                        if (MvStyle::Font.IconSm) {
                            ImFont* iconFont = MvStyle::Font.IconSm;
                            constexpr float chevSize = 13.0f;
                            const ImVec2 ts = iconFont->CalcTextSizeA(chevSize, FLT_MAX, 0.0f,
                                                                     MV_ICON_CHEVRON_RIGHT);
                            const ImU32 chevCol = hovered
                                ? ColU(MvStyle::MvBlue400)
                                : ColU(MvStyle::MvFg4);
                            listDl->AddText(iconFont, chevSize,
                                            ImVec2(rowMax.x - ts.x - 12.0f,
                                                   rowMin.y + (RowH - ts.y) * 0.5f),
                                            chevCol, MV_ICON_CHEVRON_RIGHT);
                        }

                        if (clicked) {
                            state.action = WelcomeAction::OpenRecent;
                            state.selectedRecent = recent[i];
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
        }
        ImGui::Unindent(SidePad);

        // ---- Footer (MvBg1 strip) ---------------------------------------
        ImGui::Dummy(ImVec2(0.0f, 14.0f));
        const ImVec2 footerStart = ImGui::GetCursorScreenPos();
        constexpr float FooterH = 48.0f;
        const ImVec2 fMin(wpos.x, footerStart.y);
        const ImVec2 fMax(wpos.x + ModalW, footerStart.y + FooterH);
        dl->AddRectFilled(fMin, fMax, ColU(MvStyle::MvBg1));
        dl->AddLine(ImVec2(fMin.x, fMin.y),
                    ImVec2(fMax.x, fMin.y),
                    ColU(MvStyle::MvLine2));

        ImGui::SetCursorScreenPos(ImVec2(fMin.x + SidePad, fMin.y + 12.0f));
        ImGui::Checkbox("Show this on startup", &state.showOnStartup);

        // Right-aligned Close ghost button.
        const float closeBtnW = 90.0f;
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(fMax.x - SidePad - closeBtnW, fMin.y + 8.0f));
        if (ImGui::Button("Close", ImVec2(closeBtnW, 30.0f))) {
            ImGui::CloseCurrentPopup();
        }

        // Reserve the footer's vertical space so EndPopup sees it.
        ImGui::SetCursorScreenPos(ImVec2(wpos.x, fMax.y));
        ImGui::Dummy(ImVec2(ModalW, 0.0f));

        if (!ImGui::IsPopupOpen("Welcome to MetaVisage")) state.isOpen = false;
        ImGui::EndPopup();
    } else {
        state.isOpen = false;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

void ExportModalState::Open(bool hasTextures) {
    pendingOpen = true;
    action = ExportAction::None;
    texturesAvailable = hasTextures;
    if (hasTextures) exportBakedTextures = true;
}

void RenderExportModal(ExportModalState& state) {
    HandlePendingOpen("Export Mesh", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Export Mesh", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        // Force a 1px border on every input/combo/checkbox in this modal — the
        // dark theme's default FrameBorderSize=0 makes empty fields nearly
        // invisible against the popup background, which users have flagged.
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        ImGui::TextUnformatted("Output File");
        ImGui::Separator();
        char pathBuf[1024];
        std::snprintf(pathBuf, sizeof(pathBuf), "%s", state.filepath.c_str());
        ImGui::PushItemWidth(-100);
        if (ImGui::InputText("##filepath", pathBuf, sizeof(pathBuf))) {
            state.filepath = pathBuf;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            // AppWindow consumes this flag each frame, runs the NFD save
            // dialog, and writes the chosen path back to `filepath`. The
            // modal stays open so the user can still tweak options.
            state.browseRequested = true;
        }
        ImGui::Dummy(ImVec2(0, 6));

        ImGui::TextUnformatted("Export Options");
        ImGui::Separator();
        static const char* kFormats[] = {"FBX", "OBJ", "GLTF"};
        ImGui::Combo("Format", &state.formatIndex, kFormats, IM_ARRAYSIZE(kFormats));
        ImGui::Checkbox("Include materials", &state.includeMaterials);
        ImGui::Checkbox("Bake transform into vertices", &state.applyTransform);
        ImGui::Checkbox("Y-Up coordinate system (Unreal Engine)", &state.convertToYUp);
        ImGui::Checkbox("MetaHuman compatible (preserves original mesh topology)",
                        &state.metaHumanCompatible);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Rewrites the original morph mesh file with deformed positions,\n"
                              "preserving the exact vertex count and face connectivity required\n"
                              "by UE5 MetaHuman Conform.");
        }
        ImGui::InputFloat("Scale Factor", &state.scaleFactor, 0.1f, 1.0f, "%.3f");
        char prefixBuf[64];
        std::snprintf(prefixBuf, sizeof(prefixBuf), "%s", state.ueNamingPrefix.c_str());
        if (ImGui::InputText("UE Name Prefix", prefixBuf, sizeof(prefixBuf))) {
            state.ueNamingPrefix = prefixBuf;
        }

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextUnformatted("Texture Baking");
        ImGui::Separator();
        ImGui::BeginDisabled(!state.texturesAvailable);
        ImGui::Checkbox("Bake and export textures", &state.exportBakedTextures);
        static const char* kRes[] = {"1024x1024", "2048x2048", "4096x4096"};
        ImGui::Combo("Resolution", &state.bakeResolutionIndex, kRes, IM_ARRAYSIZE(kRes));
        static const char* kFmt[] = {"PNG", "TGA"};
        ImGui::Combo("Texture Format", &state.textureFormatIndex, kFmt, IM_ARRAYSIZE(kFmt));
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(0, 10));
        const bool canExport = !state.filepath.empty();
        if (!canExport) ImGui::BeginDisabled();
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            ExportOptions& o = state.options;
            o.format = static_cast<ExportFormat>(state.formatIndex);
            o.triangulate = false;
            o.includeMaterials = state.includeMaterials;
            o.applyTransform = state.applyTransform;
            o.convertToYUp = state.convertToYUp;
            o.metaHumanCompatible = state.metaHumanCompatible;
            o.undoUVFlip = state.metaHumanCompatible;
            o.scaleFactor = state.scaleFactor;
            o.ueNamingPrefix = state.ueNamingPrefix;
            o.exportBakedTextures = state.exportBakedTextures && state.texturesAvailable;
            static const int kResValues[] = {1024, 2048, 4096};
            o.bakeResolution = kResValues[state.bakeResolutionIndex];
            o.textureFormat = (state.textureFormatIndex == 0) ? "png" : "tga";
            state.action = ExportAction::Export;
            ImGui::CloseCurrentPopup();
        }
        if (!canExport) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            state.action = ExportAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar(); // FrameBorderSize
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// Save / Rename Wrap Engine preset (Story 4.4)
// ---------------------------------------------------------------------------

namespace {

std::string TrimSpaces(const std::string& s) {
    auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return {};
    auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}

// 12 glyphs hand-picked from MvIcons.h for the picker grid (4 wide × 3 tall).
// Story 4.5's icon-glyph regression smoke (when it lands) should walk this
// array and verify each codepoint has a non-empty bitmap in the atlas.
constexpr std::array<const char*, 12> kPresetGlyphPicker = {
    MV_ICON_TARGET,    MV_ICON_SPARKLES,  MV_ICON_WAND,      MV_ICON_CIRCLE,
    MV_ICON_SQUARE,    MV_ICON_BRUSH,     MV_ICON_SLIDERS,   MV_ICON_CROSSHAIR,
    MV_ICON_MOVE_3D,   MV_ICON_CAMERA,    MV_ICON_IMAGE,     MV_ICON_SETTINGS,
};

} // namespace

void SavePresetModalState::Open(const std::string& t,
                                const std::string& editingId,
                                const std::string& initName,
                                const std::string& initDesc,
                                const std::string& initGlyph,
                                std::vector<std::string> taken) {
    title           = t;
    editingPresetId = editingId;
    name            = initName;
    description     = initDesc;
    iconGlyph       = initGlyph.empty() ? std::string(MV_ICON_TARGET) : initGlyph;
    takenNames      = std::move(taken);
    confirmed       = false;
    pendingOpen     = true;
}

void RenderSavePresetModal(SavePresetModalState& state) {
    HandlePendingOpen("##SavePresetModal", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("##SavePresetModal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    // Title bar
    ImGui::TextUnformatted(state.title.empty() ? "Save Preset" : state.title.c_str());
    ImGui::Separator();

    // Name field
    ImGui::TextUnformatted("Name");
    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", state.name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::InputText("##presetName", nameBuf, sizeof(nameBuf))) {
        state.name = nameBuf;
    }
    ImGui::PopItemWidth();

    // Validation feedback
    const std::string trimmedName = TrimSpaces(state.name);
    const bool nameMissing = trimmedName.empty();
    bool nameTaken = false;
    for (const auto& taken : state.takenNames) {
        if (taken == trimmedName) { nameTaken = true; break; }
    }
    if (nameMissing) {
        ImGui::TextColored(MvStyle::MvDanger, "Name is required.");
    } else if (nameTaken) {
        ImGui::TextColored(MvStyle::MvDanger,
                           "A preset with that name already exists.");
    }

    // Description (optional)
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::TextUnformatted("Description (optional)");
    char descBuf[256];
    std::snprintf(descBuf, sizeof(descBuf), "%s", state.description.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::InputText("##presetDesc", descBuf, sizeof(descBuf))) {
        state.description = descBuf;
    }
    ImGui::PopItemWidth();

    // Icon picker — 4×3 grid of Lucide glyphs. Selected glyph gets the
    // primary-blue button background.
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::TextUnformatted("Icon");
    if (ImGui::BeginTable("##iconPicker", 4,
                          ImGuiTableFlags_SizingStretchSame |
                          ImGuiTableFlags_NoPadOuterX)) {
        for (int i = 0; i < static_cast<int>(kPresetGlyphPicker.size()); ++i) {
            if (i % 4 == 0) ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char* glyph = kPresetGlyphPicker[i];
            const bool selected = (state.iconGlyph == glyph);
            ImGui::PushID(i);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, MvStyle::MvBlue500);
            if (MvStyle::Font.Icon) ImGui::PushFont(MvStyle::Font.Icon);
            if (ImGui::Button(glyph, ImVec2(-FLT_MIN, 36.0f))) {
                state.iconGlyph = glyph;
            }
            if (MvStyle::Font.Icon) ImGui::PopFont();
            if (selected) ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // Footer
    ImGui::Dummy(ImVec2(0, 10));
    const bool canConfirm = !nameMissing && !nameTaken && !state.iconGlyph.empty();
    if (!canConfirm) ImGui::BeginDisabled();
    if (ImGui::Button("Save", ImVec2(120, 0))) {
        state.name = trimmedName;       // canonicalize on the way out
        state.confirmed = true;
        ImGui::CloseCurrentPopup();
    }
    if (!canConfirm) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        state.confirmed = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::PopStyleVar(); // FrameBorderSize
    ImGui::EndPopup();
}

// ---------------------------------------------------------------------------
// Shortcuts
// ---------------------------------------------------------------------------

void ShortcutsModalState::Open() { pendingOpen = true; }

void RenderShortcutsModal(ShortcutsModalState& state) {
    HandlePendingOpen("Keyboard Shortcuts", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Keyboard Shortcuts", nullptr,
                               ImGuiWindowFlags_NoSavedSettings)) {
        struct Row { const char* shortcut; const char* description; };
        struct Group { const char* name; std::initializer_list<Row> rows; };
        static const Group kGroups[] = {
            {"File", {
                {"Ctrl+N", "New project"},
                {"Ctrl+O", "Open project"},
                {"Ctrl+S", "Save project"},
                {"Ctrl+Shift+S", "Save project as"},
                {"Ctrl+E", "Export mesh (Touch Up stage)"},
            }},
            {"Edit", {
                {"Ctrl+Z", "Undo"},
                {"Ctrl+Shift+Z / Ctrl+Y", "Redo"},
            }},
            {"Viewport", {
                {"MMB drag", "Orbit camera"},
                {"Shift + MMB drag", "Pan camera"},
                {"Scroll", "Zoom"},
                {"Numpad 1/3/7", "Front / Side / Top view"},
                {"Numpad 5", "Toggle perspective / orthographic"},
            }},
            {"Transform (Alignment stage)", {
                {"G", "Move gizmo"},
                {"R", "Rotate gizmo"},
                {"S", "Scale gizmo"},
                {"X / Y / Z", "Constrain to axis"},
            }},
            {"Brushes (Touch Up stage)", {
                {"[ / ]", "Decrease / increase brush radius"},
                {"Shift + LMB", "Smooth"},
                {"Ctrl + LMB", "Invert brush effect"},
            }},
        };
        if (ImGui::BeginChild("##shortcuts_scroll", ImVec2(0, 420), true)) {
            for (const auto& group : kGroups) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
                ImGui::TextUnformatted(group.name);
                ImGui::PopStyleColor();
                ImGui::Separator();
                if (ImGui::BeginTable("##tbl", 2,
                                      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
                    for (const auto& row : group.rows) {
                        RenderKeyValueRow(row.shortcut, row.description);
                    }
                    ImGui::EndTable();
                }
                ImGui::Spacing();
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Close", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// License status -- Sprint 6 Story 6.6
// ---------------------------------------------------------------------------

namespace {

std::string Capitalize(std::string s) {
    if (s.empty()) return s;
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

// Render a "LABEL value" row inside the license modal -- label muted, value
// emphasized. Uses MonoSm for the label per the inspector convention.
void LicenseRow(const char* label, const std::string& value) {
    ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
    {
        MvStyle::ScopedFont mono(MvStyle::Font.MonoSm);
        ImGui::TextUnformatted(label);
    }
    ImGui::PopStyleColor();
    ImGui::SameLine(140.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
    ImGui::TextUnformatted(value.empty() ? "\xE2\x80\x94" : value.c_str());
    ImGui::PopStyleColor();
}

// Format the validation status pill text + color from LicenseManager state.
void LicenseStatusPill(const LicenseManager& lm) {
    using S = LicenseManager::AuthState;
    const auto state = lm.GetState();
    const char* label = "\xE2\x80\x94";
    ImVec4 color = MvStyle::MvFg3;
    switch (state) {
        case S::Authenticated:
            if (lm.IsOfflineMode()) {
                label = "Offline -- using cached license";
                color = MvStyle::MvWarning;
            } else {
                label = "Online";
                color = MvStyle::MvBlue400;
            }
            break;
        case S::Inactive:
            label = "Subscription inactive";
            color = MvStyle::MvWarning;
            break;
        case S::Expired:
            label = "Session expired -- please sign in again";
            color = MvStyle::MvWarning;
            break;
        case S::Authenticating:
        case S::Unauthenticated:
            label = "Not signed in";
            color = MvStyle::MvFg3;
            break;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
    {
        MvStyle::ScopedFont mono(MvStyle::Font.MonoSm);
        ImGui::TextUnformatted("STATUS");
    }
    ImGui::PopStyleColor();
    ImGui::SameLine(140.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}

}  // namespace

void LicenseModalState::Open() { pendingOpen = true; }

void RenderLicenseModal(LicenseModalState& state) {
    HandlePendingOpen("Account & License", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Account & License", nullptr,
                               ImGuiWindowFlags_NoSavedSettings)) {
        const auto& lm = LicenseManager::Instance();

        // ---- Account ---------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted("ACCOUNT");
        ImGui::PopStyleColor();
        ImGui::Separator();
        LicenseRow("EMAIL", lm.GetUserEmail());
        ImGui::Spacing();
        ImGui::Spacing();

        // ---- License ---------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted("LICENSE");
        ImGui::PopStyleColor();
        ImGui::Separator();
        LicenseRow("TIER", Capitalize(lm.GetTier()));
        LicenseStatusPill(lm);
        ImGui::Spacing();
        ImGui::Spacing();

        // ---- Usage -----------------------------------------------------
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted("USAGE THIS MONTH");
        ImGui::PopStyleColor();
        ImGui::Separator();
        // Story 7.2 live-test feedback: surface offline-queued conversions
        // alongside the server-confirmed count so the user sees that
        // accepts performed offline are tracked.
        const int used    = lm.GetConversionsUsed();
        const int pending = lm.GetPendingConversionCount();
        const int limit   = lm.GetConversionsLimit();
        std::string usage;
        if (limit > 0) {
            usage = std::format("{} / {} conversions", used + pending, limit);
        } else {
            usage = std::format("{} conversions", used + pending);
        }
        LicenseRow("USED", usage);
        if (pending > 0) {
            LicenseRow("PENDING SYNC",
                       std::format("{} queued offline", pending));
        }
        // Story 7.4 -- the API returns conversions_reset_at as ISO 8601
        // (e.g. "2026-04-15T00:00:00.000Z"). Show only the YYYY-MM-DD
        // prefix so the dialog reads cleanly; fall back to the raw
        // string if the value is shorter than expected.
        std::string resetDisplay = lm.GetConversionsResetAt();
        if (resetDisplay.size() >= 10) {
            resetDisplay = resetDisplay.substr(0, 10);
        }
        LicenseRow("RESETS", resetDisplay);
        ImGui::Spacing();
        ImGui::Spacing();

        // ---- Footer ----------------------------------------------------
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Manage Subscription", ImVec2(180, 0))) {
            state.manageSubscriptionRequested = true;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        MvStyle::MvWarning);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MvStyle::MvWarning);
        if (ImGui::Button("Sign Out", ImVec2(120, 0))) {
            state.signOutRequested = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        // Right-aligned Close button.
        const float btnW = 100.0f;
        const float avail = ImGui::GetContentRegionAvail().x;
        if (avail > btnW) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btnW));
        if (ImGui::Button("Close", ImVec2(btnW, 0))) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

} // namespace MetaVisage
