#ifndef METAVISAGE_UI_MODALS_H
#define METAVISAGE_UI_MODALS_H

#include "io/MeshExporter.h"

#include <QString>
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
    QString title;
    QString message;
    QString confirmLabel = "OK";
    QString cancelLabel = "Cancel";
    std::function<void()> onConfirm;

    void Open(const QString& t, const QString& msg,
              std::function<void()> confirm,
              const QString& confirmLbl = "OK",
              const QString& cancelLbl = "Cancel");
};
void RenderConfirmModal(ConfirmModalState& state);

struct ErrorModalState {
    bool pendingOpen = false;
    QString title = "Error";
    QString message;

    void Open(const QString& msg, const QString& t = "Error");
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
    QString selectedRecent;

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
    QString filepath;

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
