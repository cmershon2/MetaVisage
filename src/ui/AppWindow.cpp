#include "ui/AppWindow.h"
#include "ui/ViewportHost.h"
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
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // don't persist imgui.ini in cwd

    ApplyDarkTheme();

    // install_callbacks=false — we install our own GLFW callbacks below and
    // forward to ImGui_ImplGlfw_* inside them so we can gate viewport input on
    // ImGui's WantCapture flags without fighting the backend's chain logic.
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, false)) {
        MV_LOG_CRITICAL("ImGui_ImplGlfw_InitForOpenGL failed");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 430 core")) {
        MV_LOG_CRITICAL("ImGui_ImplOpenGL3_Init failed");
        return false;
    }
    imguiInitialized_ = true;

    MV_LOG_INFO(QString("AppWindow initialized: %1x%2, GL %3, GLSL %4")
        .arg(width).arg(height)
        .arg(reinterpret_cast<const char*>(glGetString(GL_VERSION)))
        .arg(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));

    viewportHost_ = std::make_unique<ViewportHost>();
    if (!viewportHost_->Initialize()) {
        MV_LOG_CRITICAL("ViewportHost initialization failed");
        return false;
    }

    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowFocusCallback(window_, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(window_, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCharCallback(window_, ImGui_ImplGlfw_CharCallback);

    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureMouse) return;
        double sx, sy; glfwGetCursorPos(w, &sx, &sy);
        self->viewportHost_->ForwardMouseButton(button, action, mods, sx, sy);
    });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double sx, double sy) {
        ImGui_ImplGlfw_CursorPosCallback(w, sx, sy);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureMouse) return;
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
        if (!self || !self->viewportHost_) return;
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

        if (viewportHost_) viewportHost_->BuildLayout(fbWidth, fbHeight);
        RenderFrame(fbWidth, fbHeight);

        ImGui::Render();

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (viewportHost_) viewportHost_->RenderGL(fbHeight);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }
}

void AppWindow::RenderFrame(int /*fbWidth*/, int /*fbHeight*/) {
    // Sprint 3.5.3: minimal debug HUD. Sidebars / menus / dialogs return in
    // Sprint 3.6. The viewport itself is driven from Run() via ViewportHost.
    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 110.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("MetaVisage — debug HUD");
    ImGui::Text("Frame: %.3f ms (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Text("GL:   %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    ImGui::Text("GLSL: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    ImGui::End();
}

void AppWindow::Shutdown() {
    if (imguiInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized_ = false;
    }
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

} // namespace MetaVisage
