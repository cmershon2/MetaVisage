#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "ui/Modals.h"
#include "ui/StageSidebar.h"

#include <imgui.h>

#include <QString>
#include <memory>
#include <string>

struct GLFWwindow;

namespace MetaVisage {

class ViewportHost;
class Project;
class UndoStack;

class AppWindow {
public:
    enum class PendingAction {
        None,
        OpenProject,
        SaveProjectAs,
        ImportMorphMesh,
        ImportTargetMesh,
        ExportMesh,
        ShowKeyboardShortcuts,
        ShowAbout,
        OpenDocumentation,
        OpenAccountLicense,
        CheckForUpdates
    };

    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();

    ViewportHost* GetViewportHost() { return viewportHost_.get(); }
    Project* GetProject() { return project_.get(); }
    UndoStack* GetUndoStack() { return undoStack_.get(); }

private:
    void ApplyDarkTheme();
    void RenderFrame(int fbWidth, int fbHeight);

    void RenderAppShell();
    void RenderMainMenuBar();
    void RenderStatusBar();
    void RenderAboutModal();
    void RenderModals();
    void DispatchPendingAction();
    bool HandleGlobalShortcut(int key, int action, int mods);

    void OnFileNewProject();
    void OnFileExit();
    void OnEditUndo();
    void OnEditRedo();

    // Story 3.6.3 handlers
    void OpenProjectFromDisk(const QString& path);
    void SaveProjectToDisk(const QString& path);
    void ImportMorphMeshFromDisk(const QString& path);
    void ImportTargetMeshFromDisk(const QString& path);
    void UseDefaultMorphMesh();
    void ImportTextureForSlot(const QString& slot);
    void DoExport();

    // Keep ViewportHost dual-mode flag in sync with the current stage — dual
    // split-screen is required by Stage 2 (Point Reference) so the user can
    // place matching points on the morph and target meshes side-by-side.
    void ApplyStageViewportMode();

    GLFWwindow* window_ = nullptr;
    bool imguiContextCreated_ = false;
    bool imguiGlfwInitialized_ = false;
    bool imguiOpenGLInitialized_ = false;
    std::unique_ptr<ViewportHost> viewportHost_;

    std::unique_ptr<Project> project_;
    std::unique_ptr<UndoStack> undoStack_;
    QString projectPath_;

    PendingAction pendingAction_ = PendingAction::None;
    std::string statusMessage_;

    // Modal state — every struct is inert until its Open() is called.
    bool showAbout_ = false;
    ConfirmModalState confirmModal_;
    ErrorModalState errorModal_;
    WelcomeModalState welcomeModal_;
    ExportModalState exportModal_;
    ShortcutsModalState shortcutsModal_;
    bool welcomeShownOnLaunch_ = false;

    StageSidebar stageSidebar_;

    // Central (viewport) node rect of the dockspace, captured each frame so
    // ViewportHost scissors the GL pass to the visible area instead of the
    // full work area (where the sidebar would cover it).
    ImVec2 centralRectPos_  = ImVec2(0, 0);
    ImVec2 centralRectSize_ = ImVec2(0, 0);
};

} // namespace MetaVisage

#endif // APPWINDOW_H
