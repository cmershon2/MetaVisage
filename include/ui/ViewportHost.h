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
    // When regionPos/regionSize are non-zero, restricts rendering to that
    // screen-coord rect (used to cut the viewport out of a docked sidebar's
    // region); otherwise falls back to the main viewport's WorkPos/WorkSize.
    void BuildLayout(int fbWidth, int fbHeight,
                     float regionX = 0.0f, float regionY = 0.0f,
                     float regionW = 0.0f, float regionH = 0.0f);

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

    // --- Input routing (Story 3.5.4) ---
    // Coordinates are in screen-coord pixels (GLFW cursor space), top-left
    // origin. Callers (AppWindow GLFW callbacks) must gate on ImGui capture
    // flags before forwarding. Drag capture: once a mouse button goes down
    // over a viewport, subsequent moves/release route to the same viewport
    // even if the cursor leaves its rect.
    void ForwardMouseButton(int button, int action, int mods, double sx, double sy);
    void ForwardMouseMove(double sx, double sy);
    void ForwardScroll(double xOffset, double yOffset, double sx, double sy);
    void ForwardKey(int key, int scancode, int action, int mods);

    // True while at least one mouse button is held down over a viewport. Used
    // by AppWindow callbacks to bypass the ImGui WantCaptureMouse gate for
    // move + release events — otherwise a drag that crosses over the debug
    // HUD permanently strands the viewport in orbit/pan state.
    bool IsDragging() const { return capturedViewport_ != nullptr; }

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

    // Rects captured during BuildLayout in GLFW screen-coord pixels. Used for
    // cursor hit-testing (GLFW cursor pos isn't framebuffer-scaled).
    float leftScrX_ = 0.0f, leftScrY_ = 0.0f, leftScrW_ = 0.0f, leftScrH_ = 0.0f;
    float rightScrX_ = 0.0f, rightScrY_ = 0.0f, rightScrW_ = 0.0f, rightScrH_ = 0.0f;

    // Drag capture: set on mouse-press, cleared on all-buttons-released.
    Viewport* capturedViewport_ = nullptr;
    int pressedButtons_ = 0;

    Viewport* HitTest(double sx, double sy) const;
    void ToLocal(Viewport* vp, double sx, double sy, float& outX, float& outY) const;
};

} // namespace MetaVisage

#endif // METAVISAGE_VIEWPORTHOST_H
