#ifndef APPWINDOW_H
#define APPWINDOW_H

struct GLFWwindow;

namespace MetaVisage {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool Initialize(int width, int height, const char* title);
    void Run();
    void Shutdown();

private:
    void ApplyDarkTheme();
    void RenderFrame();

    GLFWwindow* window_ = nullptr;
    bool imguiInitialized_ = false;
};

} // namespace MetaVisage

#endif // APPWINDOW_H
