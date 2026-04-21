#include "ui/ViewportHost.h"
#include "ui/Viewport.h"
#include "core/Camera.h"
#include "core/Project.h"

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

void ViewportHost::BuildLayout(int fbWidth, int fbHeight) {
    lastLeftW_ = 0.0f;
    lastRightW_ = 0.0f;

    ImGuiViewport* mainVp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainVp->WorkPos);
    ImGui::SetNextWindowSize(mainVp->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

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

            left_->DrawOverlays(lastLeftX_, lastLeftY_, lastLeftW_, lastLeftH_);
            right_->DrawOverlays(lastRightX_, lastRightY_, lastRightW_, lastRightH_);
        } else {
            lastLeftX_ = px; lastLeftY_ = py;
            lastLeftW_ = pw; lastLeftH_ = ph;
            left_->DrawOverlays(lastLeftX_, lastLeftY_, lastLeftW_, lastLeftH_);
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
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
