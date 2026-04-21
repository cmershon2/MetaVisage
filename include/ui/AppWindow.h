#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <memory>

struct GLFWwindow;

namespace MetaVisage {

class ViewportHost;

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();

    ViewportHost* GetViewportHost() { return viewportHost_.get(); }

private:
    void ApplyDarkTheme();
    void RenderFrame(int fbWidth, int fbHeight);

    GLFWwindow* window_ = nullptr;
    // Init progress — Shutdown() consults each flag so a partial failure in
    // Initialize() still unwinds only what actually succeeded.
    bool imguiContextCreated_ = false;
    bool imguiGlfwInitialized_ = false;
    bool imguiOpenGLInitialized_ = false;
    std::unique_ptr<ViewportHost> viewportHost_;
};

} // namespace MetaVisage

#endif // APPWINDOW_H
