#include "ui/ViewportHost.h"
#include "ui/Viewport.h"
#include "core/Camera.h"
#include "core/Project.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace MetaVisage {

ViewportHost::ViewportHost()
    : left_(std::make_unique<Viewport>()),
      right_(std::make_unique<Viewport>()) {}

ViewportHost::~ViewportHost() = default;

bool ViewportHost::Initialize() {
    if (!left_->Initialize()) return false;
    if (!right_->Initialize()) return false;
    WireCameraSync();
    return true;
}

void ViewportHost::WireCameraSync() {
    left_->OnCameraChanged = [this]() {
        if (syncing_ || !dualMode_) return;
        syncing_ = true;
        left_->SetActive(true);
        right_->SetActive(false);
        right_->SyncCameraFrom(*left_->GetCamera());
        syncing_ = false;
    };
    right_->OnCameraChanged = [this]() {
        if (syncing_ || !dualMode_) return;
        syncing_ = true;
        right_->SetActive(true);
        left_->SetActive(false);
        left_->SyncCameraFrom(*right_->GetCamera());
        syncing_ = false;
    };
}

void ViewportHost::SetProject(Project* project) {
    left_->SetProject(project);
    right_->SetProject(project);
}

void ViewportHost::SetDualMode(bool dual) {
    dualMode_ = dual;
    if (dual) {
        left_->SetRenderFilter(RenderFilter::TargetOnly);
        left_->SetViewportLabel(QStringLiteral("Target Mesh"));
        right_->SetRenderFilter(RenderFilter::MorphOnly);
        right_->SetViewportLabel(QStringLiteral("Morph Mesh"));
        right_->SyncCameraFrom(*left_->GetCamera());
        left_->SetActive(true);
        right_->SetActive(false);
    } else {
        left_->SetRenderFilter(RenderFilter::All);
        left_->SetViewportLabel(QString());
        left_->SetActive(false);
        right_->SetActive(false);
    }
}

Camera* ViewportHost::GetActiveCamera() {
    return left_->GetCamera();
}

void ViewportHost::SetSelectedPointIndex(int index) {
    left_->SetSelectedPointIndex(index);
    right_->SetSelectedPointIndex(index);
}

void ViewportHost::SetPointSize(float size) {
    left_->SetPointSize(size);
    right_->SetPointSize(size);
}

void ViewportHost::BuildLayout(int fbWidth, int fbHeight,
                               float regionX, float regionY,
                               float regionW, float regionH) {
    lastLeftW_ = 0.0f;
    lastRightW_ = 0.0f;

    ImGuiViewport* mainVp = ImGui::GetMainViewport();
    ImVec2 hostPos;
    ImVec2 hostSize;
    if (regionW > 0.0f && regionH > 0.0f) {
        hostPos  = ImVec2(regionX, regionY);
        hostSize = ImVec2(regionW, regionH);
    } else {
        hostPos  = mainVp->WorkPos;
        hostSize = mainVp->WorkSize;
    }
    ImGui::SetNextWindowPos(hostPos);
    ImGui::SetNextWindowSize(hostSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // NoInputs is critical — without it, the fullscreen host window claims the
    // cursor, forcing WantCaptureMouse high and blocking all viewport input.
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("##ViewportHost", nullptr, flags)) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();

        float dpiX = (mainVp->Size.x > 0.0f) ? (float)fbWidth / mainVp->Size.x : 1.0f;
        float dpiY = (mainVp->Size.y > 0.0f) ? (float)fbHeight / mainVp->Size.y : 1.0f;

        float px = pos.x * dpiX;
        float py = pos.y * dpiY;
        float pw = avail.x * dpiX;
        float ph = avail.y * dpiY;

        if (dualMode_) {
            float half = pw * 0.5f;
            lastLeftX_ = px;         lastLeftY_ = py;
            lastLeftW_ = half;       lastLeftH_ = ph;
            lastRightX_ = px + half; lastRightY_ = py;
            lastRightW_ = pw - half; lastRightH_ = ph;

            float scrHalf = avail.x * 0.5f;
            leftScrX_ = pos.x;            leftScrY_ = pos.y;
            leftScrW_ = scrHalf;          leftScrH_ = avail.y;
            rightScrX_ = pos.x + scrHalf; rightScrY_ = pos.y;
            rightScrW_ = avail.x - scrHalf; rightScrH_ = avail.y;

            // Overlays (ImGui draw list + windows) use logical/screen coords so
            // they align with the menu bar, shading combo, and mouse input
            // on high-DPI displays. The GL pass (RenderGL) uses the scaled
            // framebuffer coords stored in lastLeft*/lastRight*.
            left_->DrawOverlays(leftScrX_, leftScrY_, leftScrW_, leftScrH_);
            right_->DrawOverlays(rightScrX_, rightScrY_, rightScrW_, rightScrH_);
        } else {
            lastLeftX_ = px; lastLeftY_ = py;
            lastLeftW_ = pw; lastLeftH_ = ph;
            leftScrX_ = pos.x; leftScrY_ = pos.y;
            leftScrW_ = avail.x; leftScrH_ = avail.y;
            rightScrW_ = 0.0f;
            left_->DrawOverlays(leftScrX_, leftScrY_, leftScrW_, leftScrH_);
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
}

Viewport* ViewportHost::HitTest(double sx, double sy) const {
    if (leftScrW_ > 0.0f &&
        sx >= leftScrX_ && sx < leftScrX_ + leftScrW_ &&
        sy >= leftScrY_ && sy < leftScrY_ + leftScrH_) {
        return left_.get();
    }
    if (dualMode_ && rightScrW_ > 0.0f &&
        sx >= rightScrX_ && sx < rightScrX_ + rightScrW_ &&
        sy >= rightScrY_ && sy < rightScrY_ + rightScrH_) {
        return right_.get();
    }
    return nullptr;
}

void ViewportHost::ToLocal(Viewport* vp, double sx, double sy,
                           float& outX, float& outY) const {
    if (vp == left_.get()) {
        outX = static_cast<float>(sx - leftScrX_);
        outY = static_cast<float>(sy - leftScrY_);
    } else if (vp == right_.get()) {
        outX = static_cast<float>(sx - rightScrX_);
        outY = static_cast<float>(sy - rightScrY_);
    } else {
        outX = static_cast<float>(sx);
        outY = static_cast<float>(sy);
    }
}

void ViewportHost::ForwardMouseButton(int button, int action, int mods,
                                      double sx, double sy) {
    if (action == GLFW_PRESS) {
        Viewport* target = capturedViewport_ ? capturedViewport_ : HitTest(sx, sy);
        if (!target) return;
        capturedViewport_ = target;
        pressedButtons_ |= (1 << button);

        if (dualMode_) {
            left_->SetActive(target == left_.get());
            right_->SetActive(target == right_.get());
        }

        float lx, ly; ToLocal(target, sx, sy, lx, ly);
        target->OnMouseButton(button, action, mods, lx, ly);
    } else if (action == GLFW_RELEASE) {
        Viewport* target = capturedViewport_ ? capturedViewport_ : HitTest(sx, sy);
        if (!target) return;
        float lx, ly; ToLocal(target, sx, sy, lx, ly);
        target->OnMouseButton(button, action, mods, lx, ly);

        pressedButtons_ &= ~(1 << button);
        if (pressedButtons_ == 0) capturedViewport_ = nullptr;
    }
}

void ViewportHost::ForwardMouseMove(double sx, double sy) {
    Viewport* target = capturedViewport_ ? capturedViewport_ : HitTest(sx, sy);
    if (!target) return;
    float lx, ly; ToLocal(target, sx, sy, lx, ly);
    target->OnMouseMove(lx, ly);
}

void ViewportHost::ForwardScroll(double xOffset, double yOffset,
                                 double sx, double sy) {
    Viewport* target = HitTest(sx, sy);
    if (!target) return;
    target->OnScroll(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void ViewportHost::ForwardKey(int key, int scancode, int action, int mods) {
    // Keyboard routes to the active viewport when one is set (dual mode), or
    // to the primary viewport otherwise. Single-viewport mode always routes
    // to left_.
    Viewport* target = left_.get();
    if (dualMode_ && right_->IsActive()) target = right_.get();
    target->OnKey(key, scancode, action, mods);
}

void ViewportHost::RenderGL(int fbHeight) {
    if (lastLeftW_ > 0.0f) {
        left_->RenderGL(lastLeftX_, lastLeftY_, lastLeftW_, lastLeftH_, fbHeight);
    }
    if (dualMode_ && lastRightW_ > 0.0f) {
        right_->RenderGL(lastRightX_, lastRightY_, lastRightW_, lastRightH_, fbHeight);
    }
}

} // namespace MetaVisage
