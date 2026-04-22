#include "ui/AppWindow.h"
#include "ui/ViewportHost.h"
#include "core/Project.h"
#include "core/UndoStack.h"
#include "core/Types.h"
#include "utils/Logger.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <QString>
#include <cstdio>

namespace MetaVisage {

namespace {

void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
    MV_LOG_ERROR(QString("GLFW error %1: %2").arg(error).arg(description));
}

const char* StageDisplayName(WorkflowStage s) {
    switch (s) {
        case WorkflowStage::Alignment:      return "Alignment";
        case WorkflowStage::PointReference: return "Point Reference";
        case WorkflowStage::Morph:          return "Morph";
        case WorkflowStage::TouchUp:        return "Touch Up";
    }
    return "";
}

const char* PendingActionDisplayName(AppWindow::PendingAction a) {
    switch (a) {
        case AppWindow::PendingAction::OpenProject:           return "Open Project";
        case AppWindow::PendingAction::SaveProjectAs:         return "Save Project As";
        case AppWindow::PendingAction::ImportMorphMesh:       return "Import Morph Mesh";
        case AppWindow::PendingAction::ImportTargetMesh:      return "Import Target Mesh";
        case AppWindow::PendingAction::ExportMesh:            return "Export Mesh";
        case AppWindow::PendingAction::ShowKeyboardShortcuts: return "Keyboard Shortcuts";
        case AppWindow::PendingAction::ShowAbout:             return "About MetaVisage";
        case AppWindow::PendingAction::OpenDocumentation:     return "Documentation";
        case AppWindow::PendingAction::OpenAccountLicense:    return "Account & License";
        case AppWindow::PendingAction::CheckForUpdates:       return "Check for Updates";
        default:                                              return "";
    }
}

} // namespace

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    Shutdown();
}

bool AppWindow::Initialize(int width, int height, const char* title) {
    glfwSetErrorCallback(&GlfwErrorCallback);

    if (!glfwInit()) {
        MV_LOG_CRITICAL("glfwInit failed");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        MV_LOG_CRITICAL("glfwCreateWindow failed");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        MV_LOG_CRITICAL(QString("glewInit failed: %1")
            .arg(reinterpret_cast<const char*>(glewGetErrorString(glewStatus))));
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imguiContextCreated_ = true;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr; // don't persist imgui.ini in cwd

    ApplyDarkTheme();

    // install_callbacks=false — we install our own GLFW callbacks below and
    // forward to ImGui_ImplGlfw_* inside them so we can gate viewport input on
    // ImGui's WantCapture flags without fighting the backend's chain logic.
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, false)) {
        MV_LOG_CRITICAL("ImGui_ImplGlfw_InitForOpenGL failed");
        Shutdown();
        return false;
    }
    imguiGlfwInitialized_ = true;

    if (!ImGui_ImplOpenGL3_Init("#version 430 core")) {
        MV_LOG_CRITICAL("ImGui_ImplOpenGL3_Init failed");
        Shutdown();
        return false;
    }
    imguiOpenGLInitialized_ = true;

    MV_LOG_INFO(QString("AppWindow initialized: %1x%2, GL %3, GLSL %4")
        .arg(width).arg(height)
        .arg(reinterpret_cast<const char*>(glGetString(GL_VERSION)))
        .arg(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));

    viewportHost_ = std::make_unique<ViewportHost>();
    if (!viewportHost_->Initialize()) {
        MV_LOG_CRITICAL("ViewportHost initialization failed");
        viewportHost_.reset();
        Shutdown();
        return false;
    }

    project_ = std::make_unique<Project>();
    undoStack_ = std::make_unique<UndoStack>();
    viewportHost_->SetProject(project_.get());
    statusMessage_ = "Ready";

    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowFocusCallback(window_, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(window_, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCharCallback(window_, ImGui_ImplGlfw_CharCallback);

    // Mouse gating: the ImGui WantCaptureMouse gate applies only when no drag
    // is in progress. Once a button is held over a viewport, subsequent
    // move/release events bypass the gate — otherwise a drag that crosses
    // into an ImGui panel permanently strands the viewport in orbit/pan state
    // (no RELEASE reaches Viewport::OnMouseButton to clear isOrbiting_).
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        const bool gate = !self->viewportHost_->IsDragging();
        if (action == GLFW_PRESS && gate && ImGui::GetIO().WantCaptureMouse) return;
        double sx, sy; glfwGetCursorPos(w, &sx, &sy);
        self->viewportHost_->ForwardMouseButton(button, action, mods, sx, sy);
    });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double sx, double sy) {
        ImGui_ImplGlfw_CursorPosCallback(w, sx, sy);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        if (!self->viewportHost_->IsDragging() && ImGui::GetIO().WantCaptureMouse) return;
        self->viewportHost_->ForwardMouseMove(sx, sy);
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* w, double xOff, double yOff) {
        ImGui_ImplGlfw_ScrollCallback(w, xOff, yOff);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureMouse) return;
        double sx, sy; glfwGetCursorPos(w, &sx, &sy);
        self->viewportHost_->ForwardScroll(xOff, yOff, sx, sy);
    });

    glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        ImGui_ImplGlfw_KeyCallback(w, key, scancode, action, mods);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self) return;
        // Application-global shortcuts (Ctrl-modified) are intercepted before
        // the viewport so they fire regardless of keyboard focus. Text-editing
        // ImGui widgets still receive character input through the char callback
        // above, so Ctrl+Z inside a text field would need its own guard — but
        // no editable text widget currently exists in Sprint 3.6.1.
        if (self->HandleGlobalShortcut(key, action, mods)) return;
        if (!self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureKeyboard) return;
        self->viewportHost_->ForwardKey(key, scancode, action, mods);
    });

    return true;
}

void AppWindow::ApplyDarkTheme() {
    // Mirror the v1.0 Qt Fusion dark palette so ImGui doesn't flash a lighter
    // theme. Exact colors match src/main.cpp's pre-migration palette.
    ImGui::StyleColorsDark();

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 4.0f;
    s.FrameRounding = 3.0f;
    s.GrabRounding = 3.0f;

    auto rgb = [](int r, int g, int b, float a = 1.0f) {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
    };

    s.Colors[ImGuiCol_WindowBg]       = rgb(30, 30, 30);
    s.Colors[ImGuiCol_ChildBg]        = rgb(30, 30, 30);
    s.Colors[ImGuiCol_PopupBg]        = rgb(44, 62, 80);
    s.Colors[ImGuiCol_FrameBg]        = rgb(44, 62, 80);
    s.Colors[ImGuiCol_FrameBgHovered] = rgb(52, 73, 94);
    s.Colors[ImGuiCol_FrameBgActive]  = rgb(52, 152, 219);
    s.Colors[ImGuiCol_TitleBg]        = rgb(30, 30, 30);
    s.Colors[ImGuiCol_TitleBgActive]  = rgb(44, 62, 80);
    s.Colors[ImGuiCol_MenuBarBg]      = rgb(35, 40, 45);
    s.Colors[ImGuiCol_Button]         = rgb(44, 62, 80);
    s.Colors[ImGuiCol_ButtonHovered]  = rgb(52, 73, 94);
    s.Colors[ImGuiCol_ButtonActive]   = rgb(52, 152, 219);
    s.Colors[ImGuiCol_Header]         = rgb(44, 62, 80);
    s.Colors[ImGuiCol_HeaderHovered]  = rgb(52, 73, 94);
    s.Colors[ImGuiCol_HeaderActive]   = rgb(52, 152, 219);
    s.Colors[ImGuiCol_CheckMark]      = rgb(52, 152, 219);
    s.Colors[ImGuiCol_SliderGrab]     = rgb(52, 152, 219);
    s.Colors[ImGuiCol_SliderGrabActive] = rgb(41, 128, 185);
    s.Colors[ImGuiCol_Text]           = rgb(255, 255, 255);
    s.Colors[ImGuiCol_TextDisabled]   = rgb(127, 140, 141);
}

void AppWindow::Run() {
    if (!window_) return;

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // App shell (dockspace + menu bar + status bar) goes first so that
        // docked windows — the future sidebar and the viewport — can reserve
        // their dock slots before BuildLayout runs.
        RenderAppShell();

        if (viewportHost_) viewportHost_->BuildLayout(fbWidth, fbHeight);
        RenderFrame(fbWidth, fbHeight);
        RenderPendingActionPlaceholder();

        ImGui::Render();

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (viewportHost_) viewportHost_->RenderGL(fbHeight);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }
}

void AppWindow::RenderAppShell() {
    // Master host-window pattern: a single borderless window spans the main
    // viewport, contains the menu bar, reserves a status-bar strip at the
    // bottom, and hosts an ImGui DockSpace in between. Sidebar/viewport/etc.
    // dock into that space by matching the dock ID on Begin().
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##AppHostWindow", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    RenderMainMenuBar();

    // Reserve a row for the status bar at the bottom of the host window.
    const float statusH = ImGui::GetFrameHeight();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 dockSize(avail.x, avail.y - statusH);

    ImGuiID dockId = ImGui::GetID("AppDockSpace");
    ImGui::DockSpace(dockId, dockSize, ImGuiDockNodeFlags_PassthruCentralNode);

    // Status bar — rendered as the final child of the host window so it sits
    // flush at the bottom. No separate top-level window needed.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY());
    RenderStatusBar();

    ImGui::End();
}

void AppWindow::RenderMainMenuBar() {
    if (!ImGui::BeginMenuBar()) return;

    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    const bool inTouchUp = (stage == WorkflowStage::TouchUp);
    const bool canUndo = undoStack_ && undoStack_->CanUndo();
    const bool canRedo = undoStack_ && undoStack_->CanRedo();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project", "Ctrl+N")) OnFileNewProject();
        if (ImGui::MenuItem("Open Project...", "Ctrl+O")) pendingAction_ = PendingAction::OpenProject;
        ImGui::Separator();
        // Save becomes Save As until we persist a project path (Sprint 3.6.4).
        if (ImGui::MenuItem("Save Project", "Ctrl+S")) pendingAction_ = PendingAction::SaveProjectAs;
        if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S")) pendingAction_ = PendingAction::SaveProjectAs;
        ImGui::Separator();
        if (ImGui::MenuItem("Import Morph Mesh...")) pendingAction_ = PendingAction::ImportMorphMesh;
        if (ImGui::MenuItem("Import Target Mesh...")) pendingAction_ = PendingAction::ImportTargetMesh;
        ImGui::Separator();
        // Export is stage-gated — visible only in Touch Up, matching the
        // Qt build's behaviour from Sprint 1.2.
        ImGui::BeginDisabled(!inTouchUp);
        if (ImGui::MenuItem("Export Mesh...", "Ctrl+E")) pendingAction_ = PendingAction::ExportMesh;
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) OnFileExit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        ImGui::BeginDisabled(!canUndo);
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) OnEditUndo();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!canRedo);
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) OnEditRedo();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        // Shading mode, grid toggle, and camera view presets are stubbed for
        // 3.6.1. They wire into ViewportHost / Viewport APIs in 3.6.5 and the
        // follow-up sprint polish pass.
        if (ImGui::BeginMenu("Shading Mode")) {
            ImGui::MenuItem("Solid", nullptr, false, false);
            ImGui::MenuItem("Wireframe", nullptr, false, false);
            ImGui::MenuItem("Solid + Wireframe", nullptr, false, false);
            ImGui::MenuItem("Textured", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::MenuItem("Show Grid", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Front View (Numpad 1)", nullptr, false, false);
        ImGui::MenuItem("Right View (Numpad 3)", nullptr, false, false);
        ImGui::MenuItem("Top View (Numpad 7)", nullptr, false, false);
        ImGui::MenuItem("Perspective (Home)", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Documentation")) pendingAction_ = PendingAction::OpenDocumentation;
        if (ImGui::MenuItem("Account & License")) pendingAction_ = PendingAction::OpenAccountLicense;
        if (ImGui::MenuItem("Check for Updates...")) pendingAction_ = PendingAction::CheckForUpdates;
        ImGui::Separator();
        if (ImGui::MenuItem("Keyboard Shortcuts")) pendingAction_ = PendingAction::ShowKeyboardShortcuts;
        if (ImGui::MenuItem("About MetaVisage")) pendingAction_ = PendingAction::ShowAbout;
        ImGui::EndMenu();
    }

    // Right-side stage indicator — mirrors the sidebar's `N / 4` element but
    // is always visible even before the sidebar is docked. Removed once the
    // sidebar panel lands in 3.6.2.
    const char* stageName = StageDisplayName(stage);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Stage %d / 4 — %s",
                  static_cast<int>(stage) + 1, stageName);
    float textW = ImGui::CalcTextSize(buf).x;
    float available = ImGui::GetContentRegionAvail().x;
    if (available > textW + 16.0f) {
        ImGui::SameLine(ImGui::GetCursorPosX() + available - textW - 8.0f);
        ImGui::TextUnformatted(buf);
    }

    ImGui::EndMenuBar();
}

void AppWindow::RenderStatusBar() {
    // Status bar is the final row of the host window. Keep it single-line and
    // subdued — detailed tool / hint / tier-badge rendering lands as the
    // sidebar and licensing subsystems come online.
    ImGui::Separator();
    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    ImGui::Text("Stage %d / 4 — %s  |  %s",
                static_cast<int>(stage) + 1,
                StageDisplayName(stage),
                statusMessage_.c_str());
}

void AppWindow::RenderPendingActionPlaceholder() {
    // About is cheap and self-contained — render it inline without waiting
    // for the 3.6.3 dialog infrastructure.
    if (pendingAction_ == PendingAction::ShowAbout) {
        ImGui::OpenPopup("About MetaVisage");
        pendingAction_ = PendingAction::None;
    }
    if (ImGui::BeginPopupModal("About MetaVisage", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("MetaVisage");
        ImGui::Text("Version 2.0.0");
        ImGui::Separator();
        ImGui::TextWrapped("MetaHuman mesh morphing for Unreal Engine 5.x.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (pendingAction_ == PendingAction::None) return;
    if (pendingAction_ == PendingAction::ShowAbout) return; // already handled

    // Every other pending action depends on NFD / system services / modal
    // infrastructure that lands in Stories 3.6.3 and 3.6.4. Render a
    // placeholder so the menu wiring is verifiable before those ship.
    const char* label = PendingActionDisplayName(pendingAction_);
    ImGui::OpenPopup("##PendingActionPlaceholder");
    if (ImGui::BeginPopupModal("##PendingActionPlaceholder", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("%s", label);
        ImGui::Separator();
        ImGui::TextWrapped("This action requires file dialogs or modal UI "
                           "scheduled for Story 3.6.3 / 3.6.4. Menu wiring "
                           "confirmed — placeholder will be removed.");
        ImGui::Spacing();
        if (ImGui::Button("Dismiss", ImVec2(120, 0))) {
            pendingAction_ = PendingAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

bool AppWindow::HandleGlobalShortcut(int key, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return false;

    const bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (!ctrl) return false;

    // Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S / Ctrl+E / Ctrl+Z / Ctrl+Shift+Z
    // are handled here globally. Ctrl+E is stage-gated (Touch Up only) to
    // match the File menu's Export entry.
    switch (key) {
        case GLFW_KEY_N:
            if (!shift) { OnFileNewProject(); return true; }
            return false;
        case GLFW_KEY_O:
            if (!shift) { pendingAction_ = PendingAction::OpenProject; return true; }
            return false;
        case GLFW_KEY_S:
            pendingAction_ = PendingAction::SaveProjectAs;
            return true;
        case GLFW_KEY_E:
            if (!shift && project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp) {
                pendingAction_ = PendingAction::ExportMesh;
                return true;
            }
            return false;
        case GLFW_KEY_Z:
            if (shift) OnEditRedo(); else OnEditUndo();
            return true;
        default:
            return false;
    }
}

void AppWindow::OnFileNewProject() {
    project_ = std::make_unique<Project>();
    if (undoStack_) undoStack_->Clear();
    if (viewportHost_) viewportHost_->SetProject(project_.get());
    statusMessage_ = "New project created";
    MV_LOG_INFO("File > New Project");
}

void AppWindow::OnFileExit() {
    if (window_) glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void AppWindow::OnEditUndo() {
    if (!undoStack_ || !undoStack_->CanUndo()) return;
    undoStack_->Undo();
    statusMessage_ = "Undo";
}

void AppWindow::OnEditRedo() {
    if (!undoStack_ || !undoStack_->CanRedo()) return;
    undoStack_->Redo();
    statusMessage_ = "Redo";
}

void AppWindow::RenderFrame(int /*fbWidth*/, int /*fbHeight*/) {
    // Debug HUD retained behind a single dockable window — useful during the
    // 3.6 rebuild. Removed before Sprint 3.6 closes.
    ImGui::SetNextWindowSize(ImVec2(320.0f, 110.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug HUD")) {
        ImGui::Text("Frame: %.3f ms (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::Text("GL:   %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        ImGui::Text("GLSL: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    }
    ImGui::End();
}

void AppWindow::Shutdown() {
    // Unwind in reverse init order, consulting each flag so a partial
    // Initialize() failure still produces a clean teardown.
    if (viewportHost_) viewportHost_->SetProject(nullptr);
    undoStack_.reset();
    project_.reset();
    viewportHost_.reset();
    if (imguiOpenGLInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        imguiOpenGLInitialized_ = false;
    }
    if (imguiGlfwInitialized_) {
        ImGui_ImplGlfw_Shutdown();
        imguiGlfwInitialized_ = false;
    }
    if (imguiContextCreated_) {
        ImGui::DestroyContext();
        imguiContextCreated_ = false;
    }
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

} // namespace MetaVisage
