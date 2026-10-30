#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "core/WrapPreset.h"
#include "ui/Modals.h"
#include "ui/StageSidebar.h"

#include <imgui.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct GLFWwindow;

namespace MetaVisage {

class ViewportHost;
class Project;
class UndoStack;
class MeshDeformer;
class Mesh;

class AppWindow {
public:
    enum class PendingAction {
        None,
        OpenProject,
        SaveProject,      // Ctrl+S — prompts only on the first save
        SaveProjectAs,    // Ctrl+Shift+S — always prompts for a new path
        ImportMorphMesh,
        ImportTargetMesh,
        ExportMesh,
        ShowKeyboardShortcuts,
        ShowAbout
    };

    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();

    // Story 5.4 — single-frame entry point used by Run() and by the
    // imgui_test_engine smoke-UI harness. Caller is responsible for polling
    // GLFW events and querying the framebuffer size; everything else (ImGui
    // frame setup, app shell, viewport, sidebar, modals, GL render, swap)
    // happens here.
    void TickOnce(int fbWidth, int fbHeight);

    ViewportHost* GetViewportHost() { return viewportHost_.get(); }
    Project* GetProject() { return project_.get(); }
    UndoStack* GetUndoStack() { return undoStack_.get(); }

    // Story 5.4 — exposed so the imgui_test_engine smoke harness can
    // query framebuffer size + drive its own tick loop. Production code
    // should not touch the GLFW handle directly; AppWindow remains the
    // owner.
    GLFWwindow* GetWindow() { return window_; }

    // Thread-safe request to focus the running app after a second launch.
    static void RequestForegroundFocus();

private:
    friend struct AppWindowTestAccess;

    void RenderFrame(int fbWidth, int fbHeight);

    void RenderAppShell();
    void RenderTitleBar();              // Story 3.8.2 — 36px chrome above the menu bar
    void RenderMainMenuBar();
    void RenderStatusBar();
    void RenderIconExplorer();          // Dev tool — identifies Lucide glyph codepoints
    void SyncOSWindowTitle();           // Mirror filename + stage breadcrumb to glfwSetWindowTitle
    void RenderAboutModal();
    void RenderModals();
    void DispatchPendingAction();
    bool HandleGlobalShortcut(int key, int action, int mods);

    void OnFileNewProject();
    void OnFileExit();
    void OnEditUndo();
    void OnEditRedo();

    // Story 3.6.3 handlers
    void OpenProjectFromDisk(const std::string& path);
    void SaveProjectToDisk(const std::string& path);
    void ImportMorphMeshFromDisk(const std::string& path);
    void ImportTargetMeshFromDisk(const std::string& path);
    void UseDefaultMorphMesh();
    void ImportTextureForSlot(const std::string& slot);
    // Sprint 9.4 — uploads the project's Albedo to the current target mesh's
    // renderer. Textures live on the project but the GPU copy is per mesh, so
    // this runs after texture import, target mesh replacement and project open.
    void RefreshTargetTexturePreview();
    void DoExport();

    // Keep ViewportHost dual-mode flag in sync with the current stage — dual
    // split-screen is required by Stage 2 (Point Reference) so the user can
    // place matching points on the morph and target meshes side-by-side.
    void ApplyStageViewportMode();

    // Stage 3 — Morph worker. StartMorph spawns a thread that runs the
    // MeshDeformer; CancelMorph sets the deformer's cancel flag;
    // PollMorphCompletion joins the thread and commits results into the
    // project when the worker finishes (called each frame from Run()).
    void StartMorph();
    void CancelMorph();
    void PollMorphCompletion();
    void AcceptMorph();
    // Sprint 10.6 — discard the processed morph and show the source mesh.
    // Request confirms first when the result was accepted (Touch Up work
    // lives on the deformed mesh); Reset pushes a MorphUndoAction.
    void RequestResetMorphResult();
    void ResetMorphResult();

    GLFWwindow* window_ = nullptr;
    bool imguiContextCreated_ = false;
    bool imguiGlfwInitialized_ = false;
    bool imguiOpenGLInitialized_ = false;
    std::unique_ptr<ViewportHost> viewportHost_;

    std::unique_ptr<Project> project_;
    std::unique_ptr<UndoStack> undoStack_;
    std::string projectPath_;

    // Story 4.4 — user-saved Wrap Engine presets, loaded once on Initialize
    // from %APPDATA%\MetaVisage\presets and refreshed after every Save /
    // Rename / Delete flow. Built-in presets (Quick / Fantasy) are NOT in
    // this vector — they live in WrapPreset::BuiltIns(). The sidebar is
    // handed a const reference each Render() call.
    std::vector<WrapPreset> userPresets_;

    PendingAction pendingAction_ = PendingAction::None;
    std::string statusMessage_;

    // Stage 2 — index-aware point delete, shared by the Delete-key path
    // (reads the active viewport's selected index) and the sidebar's per-
    // row X click (passes the row index directly).
    std::function<void(int)> deletePointAt_;

    // Story 3.8.2 — chrome state.
    // lastSavedAt_ drives the menu bar's "Saved Ns ago" indicator. Set every
    // time SaveProjectToDisk completes; nullopt means "no save this session".
    std::optional<std::chrono::steady_clock::time_point> lastSavedAt_;
    // GL_RENDERER captured once at startup for the status bar GPU readout.
    std::string gpuRenderer_;
    // Last value passed to glfwSetWindowTitle so SyncOSWindowTitle skips
    // the syscall when nothing changed frame-to-frame.
    std::string lastOSTitle_;

    // Modal state — every struct is inert until its Open() is called.
    bool showAbout_ = false;
    bool showIconExplorer_ = false;        // Dev tool, gated by Help menu
    bool showDebugHud_ = false;            // Dev tool, gated by Help menu

    ConfirmModalState confirmModal_;
    ErrorModalState errorModal_;
    WelcomeModalState welcomeModal_;
    ExportModalState exportModal_;
    ShortcutsModalState shortcutsModal_;

    // Story 4.4 — Save / Rename Wrap preset modal. Single state struct
    // covers both flows; editingPresetId distinguishes new (empty) from
    // rename (id of the user preset being edited).
    SavePresetModalState savePresetModal_;
    bool welcomeShownOnLaunch_ = false;

    StageSidebar stageSidebar_;

    // Central (viewport) node rect of the dockspace, captured each frame so
    // ViewportHost scissors the GL pass to the visible area instead of the
    // full work area (where the sidebar would cover it).
    ImVec2 centralRectPos_  = ImVec2(0, 0);
    ImVec2 centralRectSize_ = ImVec2(0, 0);

    // Morph worker state. The deformer runs on a background thread;
    // `morphProgress_` is updated from the worker, read from the UI each
    // frame. `morphResult_*` are populated when the worker finishes and
    // consumed by PollMorphCompletion on the main thread.
    std::unique_ptr<MeshDeformer> morphDeformer_;
    std::uint64_t projectGeneration_ = 0;
    std::uint64_t morphProjectGeneration_ = 0;
    std::thread morphThread_;
    std::atomic<bool> morphRunning_{false};
    std::atomic<bool> morphFinished_{false};
    std::atomic<float> morphProgress_{0.0f};
    std::mutex morphResultMutex_;
    bool morphResultSuccess_ = false;
    std::string morphResultError_;
    std::shared_ptr<Mesh> morphResultMesh_;
    std::vector<float> morphResultDisplacements_;
    float morphResultMaxDisp_ = 0.0f;
    float morphResultAvgDisp_ = 0.0f;
};

} // namespace MetaVisage

#endif // APPWINDOW_H
