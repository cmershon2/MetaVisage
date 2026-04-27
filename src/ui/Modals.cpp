#include "ui/Modals.h"
#include "utils/Config.h"

#include <imgui.h>

#include <array>
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

void RenderWelcomeModal(WelcomeModalState& state) {
    HandlePendingOpen("Welcome to MetaVisage", state.pendingOpen);

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Welcome to MetaVisage", nullptr,
                               ImGuiWindowFlags_NoSavedSettings)) {
        state.isOpen = true;
        ImGui::TextWrapped("MetaHuman mesh morphing for indie teams.");
        ImGui::Spacing();

        if (ImGui::Button("New Project", ImVec2(-FLT_MIN, 0))) {
            state.action = WelcomeAction::NewProject;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Open Project...", ImVec2(-FLT_MIN, 0))) {
            state.action = WelcomeAction::OpenProject;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Recent Projects");
        ImGui::Spacing();

        const auto& recent = Config::Instance().RecentProjects();
        if (recent.empty()) {
            ImGui::TextDisabled("(No recent projects)");
        } else {
            if (ImGui::BeginChild("##recent_projects", ImVec2(0, 180), true)) {
                for (const auto& path : recent) {
                    const std::string label = std::filesystem::path(path).filename().string();
                    ImGui::PushID(path.c_str());
                    if (ImGui::Selectable(label.c_str())) {
                        state.action = WelcomeAction::OpenRecent;
                        state.selectedRecent = path;
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", path.c_str());
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        if (!ImGui::IsPopupOpen("Welcome to MetaVisage")) state.isOpen = false;
        ImGui::EndPopup();
    } else {
        state.isOpen = false;
    }
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

} // namespace MetaVisage
