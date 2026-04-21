#ifndef METAVISAGE_VIEWPORTHOST_H
#define METAVISAGE_VIEWPORTHOST_H

#include <memory>

namespace MetaVisage {

class Viewport;
class Camera;
class Project;

// Hosts one or two Viewports inside the main ImGui frame. Replaces the v1.0
// QSplitter-based ViewportContainer. Renders a single fullscreen ImGui window
// (no title bar, no move, no resize) and splits its client area horizontally
// when dual mode is on.
class ViewportHost {
public:
    ViewportHost();
    ~ViewportHost();

    ViewportHost(const ViewportHost&) = delete;
    ViewportHost& operator=(const ViewportHost&) = delete;

    // Call once the GL context is current (AppWindow after GLEW init).
    bool Initialize();

    // Called during ImGui frame build (between NewFrame and Render). Places
    // the fullscreen window + per-viewport InvisibleButton regions, captures
    // each viewport's pixel rect for the GL pass, and queues overlay draws.
    void BuildLayout(int fbWidth, int fbHeight);

    // Called after ImGui::Render and the framebuffer clear, before
    // ImGui_ImplOpenGL3_RenderDrawData. Runs the scissored 3D pass for each
    // viewport using the rect captured during BuildLayout.
    void RenderGL(int fbHeight);

    void SetProject(Project* project);
    void SetDualMode(bool dual);
    bool IsDualMode() const { return dualMode_; }

    Viewport* GetPrimaryViewport() { return left_.get(); }
    Viewport* GetSecondaryViewport() { return right_.get(); }
    Camera* GetActiveCamera();

    void SetSelectedPointIndex(int index);
    void SetPointSize(float size);

private:
    void WireCameraSync();

    std::unique_ptr<Viewport> left_;
    std::unique_ptr<Viewport> right_;

    bool dualMode_ = false;
    bool syncing_ = false;

    // Rects captured during BuildLayout, consumed by RenderGL. In framebuffer
    // pixels, top-left origin. lastLeftW_ <= 0 means "no layout this frame."
    float lastLeftX_ = 0.0f, lastLeftY_ = 0.0f, lastLeftW_ = 0.0f, lastLeftH_ = 0.0f;
    float lastRightX_ = 0.0f, lastRightY_ = 0.0f, lastRightW_ = 0.0f, lastRightH_ = 0.0f;
};

} // namespace MetaVisage

#endif // METAVISAGE_VIEWPORTHOST_H
