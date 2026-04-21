#include "ui/Viewport.h"

#include "rendering/Renderer.h"
#include "core/Project.h"
#include "core/TextureData.h"
#include "sculpting/SculptBrush.h"
#include "sculpting/SmoothBrush.h"
#include "sculpting/GrabBrush.h"
#include "sculpting/PushPullBrush.h"
#include "sculpting/InflateBrush.h"
#include "sculpting/MaskBrush.h"
#include "utils/Logger.h"
#include "utils/RayCaster.h"

#include <GL/glew.h>
#include <imgui.h>

#include <cmath>
#include <cstdio>

namespace MetaVisage {

Viewport::Viewport()
    : camera_(std::make_unique<Camera>()),
      smoothBrush_(std::make_unique<SmoothBrush>()),
      grabBrush_(std::make_unique<GrabBrush>()),
      pushPullBrush_(std::make_unique<PushPullBrush>()),
      inflateBrush_(std::make_unique<InflateBrush>()),
      maskBrush_(std::make_unique<MaskBrush>()) {}

Viewport::~Viewport() = default;

bool Viewport::Initialize() {
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->Initialize()) {
        MV_LOG_CRITICAL("Renderer initialization failed");
        return false;
    }
    renderer_->SetRenderFilter(renderFilter_);
    return true;
}

// --- Setters ---

void Viewport::SetShadingMode(ShadingMode mode) {
    if (renderer_) renderer_->SetShadingMode(mode);
}

ShadingMode Viewport::GetShadingMode() const {
    if (renderer_) return renderer_->GetShadingMode();
    return ShadingMode::Solid;
}

void Viewport::SetRenderFilter(RenderFilter filter) {
    renderFilter_ = filter;
    if (renderer_) renderer_->SetRenderFilter(filter);
}

void Viewport::SyncCameraFrom(const Camera& other) {
    camera_->CopyStateFrom(other);
    // Intentionally no OnCameraChanged callback — mirrors v1.0 behaviour to
    // break dual-viewport sync loops.
}

void Viewport::SetTransformMode(TransformMode mode) {
    transformMode_ = mode;
    axisConstraint_ = AxisConstraint::None;
    isTransforming_ = false;
    activeGizmoAxis_ = GizmoAxis::None;
    if (OnTransformModeChanged) OnTransformModeChanged(transformMode_, axisConstraint_);
}

void Viewport::CancelTransform() {
    if (!isTransforming_) return;
    isTransforming_ = false;
    activeGizmoAxis_ = GizmoAxis::None;
    axisConstraint_ = AxisConstraint::None;
    // 3.5.4 restores transformBeforeState_ on the target mesh.
}

void Viewport::SetSelectedPointIndex(int index) {
    selectedPointIndex_ = index;
    if (renderer_) renderer_->SetSelectedPointIndex(index);
}

void Viewport::SetPointSize(float size) {
    if (renderer_) renderer_->SetPointSize(size);
}

void Viewport::SetMorphPreviewMode(MorphPreviewMode mode) {
    if (renderer_) renderer_->SetMorphPreviewMode(mode);
}

void Viewport::InvalidateMesh(const Mesh* mesh) {
    if (renderer_) renderer_->InvalidateMesh(mesh);
}

void Viewport::UploadHeatMapColors(const Mesh* mesh,
                                   const std::vector<float>& displacements,
                                   float maxDisplacement) {
    if (renderer_) renderer_->UploadHeatMapColors(mesh, displacements, maxDisplacement);
}

void Viewport::UploadMeshTexture(const Mesh* mesh, const TextureData& texture) {
    if (renderer_) renderer_->UploadMeshTexture(mesh, texture);
}

void Viewport::SetBrushType(BrushType type) { activeBrushType_ = type; }

static SculptBrush* PickActiveBrush(BrushType type,
                                    SmoothBrush* smooth,
                                    GrabBrush* grab,
                                    PushPullBrush* pushPull,
                                    InflateBrush* inflate) {
    switch (type) {
        case BrushType::Smooth:   return smooth;
        case BrushType::Grab:     return grab;
        case BrushType::PushPull: return pushPull;
        case BrushType::Inflate:  return inflate;
        default: return nullptr;
    }
}

void Viewport::SetBrushRadius(float radius) {
    if (auto* b = PickActiveBrush(activeBrushType_, smoothBrush_.get(),
                                  grabBrush_.get(), pushPullBrush_.get(),
                                  inflateBrush_.get())) {
        b->SetRadius(radius);
    }
}

void Viewport::SetBrushStrength(float strength) {
    if (auto* b = PickActiveBrush(activeBrushType_, smoothBrush_.get(),
                                  grabBrush_.get(), pushPullBrush_.get(),
                                  inflateBrush_.get())) {
        b->SetStrength(strength);
    }
}

void Viewport::SetBrushFalloff(FalloffType falloff) {
    if (smoothBrush_)   smoothBrush_->SetFalloff(falloff);
    if (grabBrush_)     grabBrush_->SetFalloff(falloff);
    if (pushPullBrush_) pushPullBrush_->SetFalloff(falloff);
    if (inflateBrush_)  inflateBrush_->SetFalloff(falloff);
}

float Viewport::GetBrushRadius() const {
    switch (activeBrushType_) {
        case BrushType::Smooth:   return smoothBrush_ ? smoothBrush_->GetRadius() : 0.5f;
        case BrushType::Grab:     return grabBrush_ ? grabBrush_->GetRadius() : 0.5f;
        case BrushType::PushPull: return pushPullBrush_ ? pushPullBrush_->GetRadius() : 0.5f;
        case BrushType::Inflate:  return inflateBrush_ ? inflateBrush_->GetRadius() : 0.5f;
        default: return 0.5f;
    }
}

float Viewport::GetBrushStrength() const {
    switch (activeBrushType_) {
        case BrushType::Smooth:   return smoothBrush_ ? smoothBrush_->GetStrength() : 0.5f;
        case BrushType::Grab:     return grabBrush_ ? grabBrush_->GetStrength() : 0.5f;
        case BrushType::PushPull: return pushPullBrush_ ? pushPullBrush_->GetStrength() : 0.5f;
        case BrushType::Inflate:  return inflateBrush_ ? inflateBrush_->GetStrength() : 0.5f;
        default: return 0.5f;
    }
}

void Viewport::RefreshMeshGPUData(Mesh* mesh) {
    if (!mesh || !renderer_) return;
    renderer_->UpdateMeshVertices(mesh);
}

void Viewport::SetSculptSymmetry(bool enabled, Axis axis) {
    sculptSymmetryEnabled_ = enabled;
    sculptSymmetryAxis_ = axis;
}

void Viewport::SetMaskBrushRadius(float radius) {
    if (maskBrush_) maskBrush_->SetRadius(radius);
}

void Viewport::SetMaskEraseMode(bool erase) {
    if (maskBrush_) maskBrush_->SetEraseMode(erase);
}

void Viewport::SetMaskData(std::vector<bool>* mask) {
    if (maskBrush_) maskBrush_->SetMaskData(mask);
}

void Viewport::UploadMaskColors(const Mesh* mesh, const std::vector<bool>& mask) {
    if (renderer_) renderer_->UploadMaskColors(mesh, mask);
}

void Viewport::SetShowMask(bool show) {
    if (renderer_) renderer_->SetShowMask(show);
}

// --- Input stubs (bodies land in Story 3.5.4) ---
void Viewport::OnMouseButton(int, int, int, float, float) {}
void Viewport::OnMouseMove(float, float) {}
void Viewport::OnScroll(float, float) {}
void Viewport::OnKey(int, int, int, int) {}

// --- Gizmo helpers ---

bool Viewport::IsGizmoVisible() const {
    if (!project_ || transformMode_ == TransformMode::None) return false;
    if (project_->GetCurrentStage() != WorkflowStage::Alignment) return false;
    if (!project_->GetTargetMesh().isLoaded) return false;
    return true;
}

Vector3 Viewport::GetGizmoPosition() const {
    if (!project_ || !project_->GetTargetMesh().isLoaded) return Vector3();
    const MeshReference& target = project_->GetTargetMesh();
    Vector3 meshCenter = target.mesh->GetBounds().Center();
    Matrix4x4 m = target.transform.GetMatrix();
    return Vector3(
        m.m[0]*meshCenter.x + m.m[4]*meshCenter.y + m.m[8]*meshCenter.z  + m.m[12],
        m.m[1]*meshCenter.x + m.m[5]*meshCenter.y + m.m[9]*meshCenter.z  + m.m[13],
        m.m[2]*meshCenter.x + m.m[6]*meshCenter.y + m.m[10]*meshCenter.z + m.m[14]);
}

// --- Render ---

void Viewport::DrawOverlays(float x, float y, float w, float h) {
    lastX_ = x; lastY_ = y; lastW_ = w; lastH_ = h;
    if (w <= 0.0f || h <= 0.0f) return;
    DrawOrientationGizmoOverlay(x, y, w);
    DrawViewportLabelOverlay(x, y);
    DrawPointLabelsOverlay(x, y, w, h);
    DrawActiveBorderOverlay(x, y, w, h);
}

void Viewport::RenderGL(float x, float y, float w, float h, int fbHeight) {
    if (w <= 0.0f || h <= 0.0f) return;

    // Convert ImGui top-left screen rect (Y-down) to GL bottom-left viewport.
    GLint glX = static_cast<GLint>(std::floor(x));
    GLint glY = static_cast<GLint>(std::floor(fbHeight - (y + h)));
    GLsizei glW = static_cast<GLsizei>(std::ceil(w));
    GLsizei glH = static_cast<GLsizei>(std::ceil(h));

    // --- Save GL state that ImGui depends on ---
    GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend   = glIsEnabled(GL_BLEND);
    GLboolean prevDepth   = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCull    = glIsEnabled(GL_CULL_FACE);
    GLint prevVp[4];       glGetIntegerv(GL_VIEWPORT, prevVp);
    GLint prevScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, prevScissorBox);
    GLint prevVao = 0;     glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    GLint prevProgram = 0; glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    // --- 3D pass state ---
    glEnable(GL_SCISSOR_TEST);
    glScissor(glX, glY, glW, glH);
    glViewport(glX, glY, glW, glH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (renderer_) {
        int iw = static_cast<int>(std::ceil(w));
        int ih = static_cast<int>(std::ceil(h));
        renderer_->Render(*camera_, iw, ih, project_);
        if (IsGizmoVisible()) {
            renderer_->RenderTransformGizmo(
                transformMode_, GetGizmoPosition(),
                hoveredGizmoAxis_, activeGizmoAxis_,
                *camera_, iw, ih);
        }
    }

    // --- Restore state ImGui expects ---
    glBindVertexArray(static_cast<GLuint>(prevVao));
    glUseProgram(static_cast<GLuint>(prevProgram));
    if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glScissor(prevScissorBox[0], prevScissorBox[1], prevScissorBox[2], prevScissorBox[3]);
    if (prevBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prevCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
}

// --- Overlays ---

static inline ImU32 PackColor(int r, int g, int b, int a = 255) {
    return IM_COL32(r, g, b, a);
}

void Viewport::DrawViewportLabelOverlay(float x, float y) {
    if (viewportLabel_.isEmpty()) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    std::string label = viewportLabel_.toStdString();
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label.c_str());

    ImVec2 tl(x + 8.0f, y + 40.0f);
    ImVec2 br(tl.x + textSize.x + 16.0f, tl.y + textSize.y + 8.0f);
    dl->AddRectFilled(tl, br, PackColor(0, 0, 0, 128), 4.0f);
    dl->AddText(font, fontSize,
                ImVec2(tl.x + 8.0f, tl.y + 4.0f),
                PackColor(255, 255, 255, 200),
                label.c_str());
}

void Viewport::DrawOrientationGizmoOverlay(float x, float y, float w) {
    const float gizmoSize = 80.0f;
    const float padding = 10.0f;
    float gx = x + w - gizmoSize - padding;
    float gy = y + padding;

    Vector3 camPos = camera_->GetPosition();
    Vector3 camTarget = camera_->GetTarget();
    Vector3 viewDir = (camTarget - camPos).Normalized();
    Vector3 eye = viewDir * -2.0f;
    Matrix4x4 view = Matrix4x4::LookAt(eye, Vector3(0, 0, 0), camera_->GetUp());
    Matrix4x4 proj = Matrix4x4::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 10.0f);
    Matrix4x4 vp = proj * view;

    const float axisLen = 0.7f;
    struct AxisLabel { Vector3 tip; ImU32 color; const char* label; };
    AxisLabel axes[3] = {
        {Vector3(axisLen, 0, 0), PackColor(230, 50, 50),  "X"},
        {Vector3(0, axisLen, 0), PackColor(50, 200, 50),  "Y"},
        {Vector3(0, 0, axisLen), PackColor(75, 130, 230), "Z"},
    };

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    for (const auto& axis : axes) {
        float cx = vp.m[0]*axis.tip.x + vp.m[4]*axis.tip.y + vp.m[8]*axis.tip.z  + vp.m[12];
        float cy = vp.m[1]*axis.tip.x + vp.m[5]*axis.tip.y + vp.m[9]*axis.tip.z  + vp.m[13];
        float cw = vp.m[3]*axis.tip.x + vp.m[7]*axis.tip.y + vp.m[11]*axis.tip.z + vp.m[15];
        if (std::abs(cw) < 0.0001f) continue;
        float ndcX = cx / cw;
        float ndcY = cy / cw;
        float sx = gx + (ndcX * 0.5f + 0.5f) * gizmoSize;
        float sy = gy + (1.0f - (ndcY * 0.5f + 0.5f)) * gizmoSize;
        dl->AddText(font, fontSize, ImVec2(sx - 4.0f, sy - fontSize * 0.5f),
                    axis.color, axis.label);
    }
}

void Viewport::DrawPointLabelsOverlay(float x, float y, float w, float h) {
    if (!project_) return;
    if (project_->GetCurrentStage() != WorkflowStage::PointReference) return;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    int iw = static_cast<int>(std::ceil(w));
    int ih = static_cast<int>(std::ceil(h));

    for (size_t i = 0; i < correspondences.size(); ++i) {
        const auto& corr = correspondences[i];
        int corrIdx = static_cast<int>(i);
        bool isSelected = (corrIdx == selectedPointIndex_);

        Vector3 worldPos;
        bool shouldDraw = false;
        if (renderFilter_ != RenderFilter::MorphOnly && corr.targetMeshVertexIndex >= 0) {
            worldPos = corr.targetMeshPosition;
            shouldDraw = true;
        } else if (renderFilter_ != RenderFilter::TargetOnly && corr.morphMeshVertexIndex >= 0) {
            worldPos = corr.morphMeshPosition;
            shouldDraw = true;
        }
        if (!shouldDraw) continue;

        Vector3 screenPos = RayCaster::WorldToScreen(worldPos, iw, ih, *camera_);
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) continue;

        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", corr.pointID);
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, buf);

        float sx = x + screenPos.x;
        float sy = y + screenPos.y - 12.0f;
        ImVec2 tl(sx - textSize.x * 0.5f - 3.0f, sy - textSize.y * 0.5f - 2.0f);
        ImVec2 br(sx + textSize.x * 0.5f + 3.0f, sy + textSize.y * 0.5f + 2.0f);

        ImU32 bgColor = isSelected ? PackColor(230, 126, 34, 200)
                                   : PackColor(0, 0, 0, 160);
        dl->AddRectFilled(tl, br, bgColor, 3.0f);
        dl->AddText(font, fontSize,
                    ImVec2(tl.x + 3.0f, tl.y + 2.0f),
                    PackColor(255, 255, 255, 230), buf);
    }
}

void Viewport::DrawActiveBorderOverlay(float x, float y, float w, float h) {
    if (!isActive_) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
                PackColor(52, 152, 219), 0.0f, 0, 2.0f);
}

} // namespace MetaVisage
