#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <memory>
#include <string>

struct GLFWwindow;

namespace MetaVisage {

class ViewportHost;
class Project;
class UndoStack;

class AppWindow {
public:
    // Menu-bar intents that require system services not yet online in this
    // story (NFD file pickers, ShellExecute, dialog modals). Each frame we
    // service any pending intent by rendering a placeholder modal; Sprint
    // 3.6.3 and 3.6.4 replace these branches with real behaviour.
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

    // Story 3.6.1 — app shell
    void RenderAppShell();
    void RenderMainMenuBar();
    void RenderStatusBar();
    void RenderPendingActionPlaceholder();
    bool HandleGlobalShortcut(int key, int action, int mods);

    // File-menu handlers that can execute today without NFD / modals.
    void OnFileNewProject();
    void OnFileExit();

    // Edit-menu handlers — safe today (UndoStack is already owned).
    void OnEditUndo();
    void OnEditRedo();

    GLFWwindow* window_ = nullptr;
    // Init progress — Shutdown() consults each flag so a partial failure in
    // Initialize() still unwinds only what actually succeeded.
    bool imguiContextCreated_ = false;
    bool imguiGlfwInitialized_ = false;
    bool imguiOpenGLInitialized_ = false;
    std::unique_ptr<ViewportHost> viewportHost_;

    std::unique_ptr<Project> project_;
    std::unique_ptr<UndoStack> undoStack_;

    PendingAction pendingAction_ = PendingAction::None;
    std::string statusMessage_;
};

} // namespace MetaVisage

#endif // APPWINDOW_H
