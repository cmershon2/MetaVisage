#ifndef METAVISAGE_UI_MODALS_H
#define METAVISAGE_UI_MODALS_H

#include "io/MeshExporter.h"

#include <functional>
#include <optional>
#include <string>

namespace MetaVisage {

// Each modal is driven by a small state struct that AppWindow owns. RenderXxx()
// is called every frame and no-ops until the matching Open() sets the pending
// flag. Separating "state" from "render" keeps the call sites (menu bar,
// sidebars) free of ImGui ID management.

// ---------------------------------------------------------------------------
// Confirm / Error modals — reusable helpers used by stage transitions,
// overwrite guards, and API-failure paths.
// ---------------------------------------------------------------------------

struct ConfirmModalState {
    bool pendingOpen = false;
    std::string title;
    std::string message;
    std::string confirmLabel = "OK";
    std::string cancelLabel = "Cancel";
    std::function<void()> onConfirm;

    void Open(const std::string& t, const std::string& msg,
              std::function<void()> confirm,
              const std::string& confirmLbl = "OK",
              const std::string& cancelLbl = "Cancel");
};
void RenderConfirmModal(ConfirmModalState& state);

// Single message-with-OK modal used both for errors and for one-shot
// confirmations (e.g. "Export Complete"). The `isError` flag drives the
// title color; everything else is shared so we don't ship two near-identical
// modal renderers.
struct ErrorModalState {
    bool pendingOpen = false;
    bool isError = true;
    std::string title = "Error";
    std::string message;

    void Open(const std::string& msg, const std::string& t = "Error");
    // Convenience for success / info notifications. Renders with a green
    // title color instead of the red error color.
    void OpenInfo(const std::string& msg, const std::string& t);
};
void RenderErrorModal(ErrorModalState& state);

// ---------------------------------------------------------------------------
// Welcome modal — shown on launch when no project is loaded. Offers New /
// Open / pick from recent projects. Returned action is consumed via `action`.
// ---------------------------------------------------------------------------

enum class WelcomeAction { None, NewProject, OpenProject, OpenRecent };

struct WelcomeModalState {
    bool pendingOpen = false;
    bool isOpen = false;
    WelcomeAction action = WelcomeAction::None;
    std::string selectedRecent;

    // Story 3.8.11 — footer checkbox. Default true mirrors the v1.0
    // behaviour where the welcome dialog appeared on every launch; the
    // user toggles it off to skip the modal next time. Persisted state
    // is the AppWindow's responsibility (Config or similar) — this struct
    // only holds the live UI value.
    bool showOnStartup = true;

    void Open();
};
void RenderWelcomeModal(WelcomeModalState& state);

// ---------------------------------------------------------------------------
// Export modal — mirrors the legacy ExportDialog. Sidebars call Open() on the
// Touch-Up stage; AppWindow consumes `action == Export` with `options` + `path`.
// ---------------------------------------------------------------------------

enum class ExportAction { None, Export };

struct ExportModalState {
    bool pendingOpen = false;
    ExportAction action = ExportAction::None;

    // Set to true when the user clicks Browse in the modal. AppWindow consumes
    // this flag, runs the NFD save dialog, and writes the chosen path back to
    // `filepath` (the modal stays open so the user can review options).
    bool browseRequested = false;

    // Inputs
    bool texturesAvailable = false;

    // Form state (persists across re-opens within a session)
    int formatIndex = 0;              // 0=FBX, 1=OBJ, 2=GLTF
    bool includeMaterials = true;
    bool applyTransform = true;
    bool convertToYUp = true;
    bool metaHumanCompatible = true;
    float scaleFactor = 1.0f;
    std::string ueNamingPrefix;
    bool exportBakedTextures = false;
    int bakeResolutionIndex = 1;      // 0=1024, 1=2048, 2=4096
    int textureFormatIndex = 0;       // 0=png, 1=tga
    std::string filepath;

    // Outputs, valid iff action == Export
    ExportOptions options;

    void Open(bool hasTextures);
};
void RenderExportModal(ExportModalState& state);

// ---------------------------------------------------------------------------
// Keyboard shortcuts reference — read-only table grouped by category.
// ---------------------------------------------------------------------------

struct ShortcutsModalState {
    bool pendingOpen = false;
    void Open();
};
void RenderShortcutsModal(ShortcutsModalState& state);

} // namespace MetaVisage

#endif
