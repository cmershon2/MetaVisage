#include "ui/Viewport.h"

#include "ui/MvIcons.h"
#include "ui/MvStyle.h"
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
#include "utils/BVH.h"
#include "utils/SpatialHash.h"

#include "core/Mesh.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

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
    if (project_ && project_->GetTargetMesh().isLoaded) {
        project_->GetTargetMesh().transform = transformBeforeState_;
        if (OnTargetTransformChanged) OnTargetTransformChanged();
    }
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

// Maps brush falloff curve to the inner-ring radius ratio used by the renderer
// overlay. The inner ring marks where the brush weight drops to ~50 %. Kept in
// sync with the Qt v1 implementation so sculpt cursors look identical.
static float GetFalloffInnerRatio(FalloffType falloff) {
    switch (falloff) {
        case FalloffType::Linear: return 0.5f;     // 1-t = 0.5 at t=0.5
        case FalloffType::Smooth: return 0.5f;     // hermite midpoint
        case FalloffType::Sharp:  return 0.794f;   // 1-t^3 = 0.5 at t≈0.794
    }
    return 0.5f;
}

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

void Viewport::SetShowTargetOverlay(bool show) {
    showTargetOverlay_ = show;
    // The renderer owns the actual overlay state; without forwarding the
    // checkbox in StageSidebar would just toggle a local bool that nothing
    // reads, which is what users hit before.
    if (renderer_) renderer_->SetShowTargetOverlay(show);
}

// --- Input (Story 3.5.4) ---
// Coordinates arrive in viewport-local screen-coord pixels. Sculpt/mask/point
// LMB handlers and gizmo drag are deferred to Sprint 3.6 — they early-return
// on null project anyway, and the project/mesh/stage infrastructure returns
// alongside the sidebars and dialogs.

void Viewport::OnMouseButton(int button, int action, int /*mods*/, float x, float y) {
    lastMouseX_ = x;
    lastMouseY_ = y;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.KeyShift) {
                isPanning_ = true;
            } else {
                isOrbiting_ = true;
            }
        } else if (action == GLFW_RELEASE) {
            isOrbiting_ = false;
            isPanning_ = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && project_) {
        const WorkflowStage stage = project_->GetCurrentStage();
        if (action == GLFW_PRESS) {
            // Stage 1 (Alignment): begin a gizmo drag if transform mode is
            // active and the target mesh is loaded. Axis constraint is derived
            // from the hovered gizmo handle if present; otherwise the current
            // keyboard-set constraint stays.
            if (stage == WorkflowStage::Alignment &&
                transformMode_ != TransformMode::None &&
                project_->GetTargetMesh().isLoaded) {
                if (hoveredGizmoAxis_ != GizmoAxis::None) {
                    activeGizmoAxis_ = hoveredGizmoAxis_;
                    switch (hoveredGizmoAxis_) {
                        case GizmoAxis::X: axisConstraint_ = AxisConstraint::X; break;
                        case GizmoAxis::Y: axisConstraint_ = AxisConstraint::Y; break;
                        case GizmoAxis::Z: axisConstraint_ = AxisConstraint::Z; break;
                        case GizmoAxis::Center: axisConstraint_ = AxisConstraint::None; break;
                        default: break;
                    }
                    if (OnTransformModeChanged) {
                        OnTransformModeChanged(transformMode_, axisConstraint_);
                    }
                }
                isTransforming_ = true;
                transformBeforeState_ = project_->GetTargetMesh().transform;
            }
            // Stage 2 (Point Reference): place / select a correspondence point.
            else if (stage == WorkflowStage::PointReference) {
                HandlePointClick(x, y);
            }
            // Stage 3 (Morph): paint/erase vertex mask when painting is active.
            // When painting is off the LMB is free for future gestures.
            else if (stage == WorkflowStage::Morph && isMaskPainting_) {
                HandleMaskPress(x, y);
            }
            // Stage 4 (Touch Up): begin a sculpt stroke.
            else if (stage == WorkflowStage::TouchUp) {
                HandleSculptPress(x, y);
            }
        } else if (action == GLFW_RELEASE) {
            if (isTransforming_) {
                isTransforming_ = false;
                activeGizmoAxis_ = GizmoAxis::None;
                if (OnTransformApplied) {
                    OnTransformApplied(transformBeforeState_,
                                       project_->GetTargetMesh().transform);
                }
            }
            if (isMaskStroking_) {
                HandleMaskRelease();
            }
            if (isSculpting_) {
                HandleSculptRelease();
            }
        }
    }
}

void Viewport::OnMouseMove(float x, float y) {
    float dx = x - lastMouseX_;
    float dy = y - lastMouseY_;
    lastMouseX_ = x;
    lastMouseY_ = y;

    if (isOrbiting_) {
        camera_->Orbit(dx, dy);
        if (OnCameraChanged) OnCameraChanged();
        return;
    }
    if (isPanning_) {
        camera_->Pan(dx, dy);
        if (OnCameraChanged) OnCameraChanged();
        return;
    }

    // Gizmo drag (Alignment stage only) — ported from Qt ApplyTransform.
    if (isTransforming_ && project_ && project_->GetTargetMesh().isLoaded) {
        Transform& transform = project_->GetTargetMesh().transform;

        const float moveSensitivity   = 0.01f;
        const float rotateSensitivity = 0.5f;  // deg/pixel
        const float scaleSensitivity  = 0.005f;

        switch (transformMode_) {
            case TransformMode::Move: {
                float ax =  dx * moveSensitivity;
                float ay = -dy * moveSensitivity;   // invert Y for natural feel
                Vector3 t(0.0f, 0.0f, 0.0f);
                switch (axisConstraint_) {
                    case AxisConstraint::X: t.x = ax; break;
                    case AxisConstraint::Y: t.y = ay; break;
                    case AxisConstraint::Z: t.z = -ax; break;
                    default:                t.x = ax; t.y = ay; break;
                }
                transform.Translate(t);
                break;
            }
            case TransformMode::Rotate: {
                float angle = dx * rotateSensitivity;
                Vector3 axis(0.0f, 1.0f, 0.0f);
                switch (axisConstraint_) {
                    case AxisConstraint::X:
                        axis = Vector3(1.0f, 0.0f, 0.0f); angle = -angle; break;
                    case AxisConstraint::Y:
                        axis = Vector3(0.0f, 1.0f, 0.0f); break;
                    case AxisConstraint::Z:
                        axis = Vector3(0.0f, 0.0f, 1.0f); angle = -angle; break;
                    default:
                        if (std::abs(dx) >= std::abs(dy)) {
                            axis = Vector3(0.0f, 1.0f, 0.0f);
                            angle = dx * rotateSensitivity;
                        } else {
                            axis = Vector3(1.0f, 0.0f, 0.0f);
                            angle = dy * rotateSensitivity;
                        }
                        break;
                }
                transform.Rotate(Quaternion::FromAxisAngle(axis, angle));
                break;
            }
            case TransformMode::Scale: {
                float sf    = std::max(0.1f, std::min(2.0f, 1.0f + dx * scaleSensitivity));
                float sfInv = std::max(0.1f, std::min(2.0f, 1.0f - dx * scaleSensitivity));
                Vector3 s(1.0f, 1.0f, 1.0f);
                switch (axisConstraint_) {
                    case AxisConstraint::X: s.x = sf; break;
                    case AxisConstraint::Y: s.y = sf; break;
                    case AxisConstraint::Z: s.z = sfInv; break;
                    default: s.x = sf; s.y = sf; s.z = sf; break;
                }
                transform.Scale(s);
                break;
            }
            case TransformMode::None:
            default: break;
        }
        if (OnTargetTransformChanged) OnTargetTransformChanged();
    }

    // Stage 1 gizmo hover. axisConstraint_ is set on PRESS from the hovered
    // axis, so the hover hit test must run on every move while the user has
    // a transform mode active — otherwise hoveredGizmoAxis_ is always None
    // and Move/Rotate/Scale never constrain to a single axis.
    if (!isTransforming_ && IsGizmoVisible() && renderer_) {
        const int vpW = static_cast<int>(std::ceil(lastW_));
        const int vpH = static_cast<int>(std::ceil(lastH_));
        if (vpW > 0 && vpH > 0) {
            Ray ray = RayCaster::ScreenToRay(x, y, vpW, vpH, *camera_);
            hoveredGizmoAxis_ = renderer_->GizmoHitTest(
                transformMode_, GetGizmoPosition(), ray, *camera_, vpW, vpH);
        }
    } else if (!isTransforming_) {
        hoveredGizmoAxis_ = GizmoAxis::None;
    }

    // Stage-specific brush cursor / stroke handling. The renderer draws the
    // ring overlay, so we hide it on stages that don't use a brush.
    if (project_) {
        const WorkflowStage stage = project_->GetCurrentStage();
        if (stage == WorkflowStage::Morph) {
            if (isMaskStroking_) {
                HandleMaskMove(x, y);
            } else if (isMaskPainting_) {
                UpdateMaskBrushCursor(x, y);
            } else if (renderer_) {
                renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
            }
        } else if (stage == WorkflowStage::TouchUp) {
            if (isSculpting_) HandleSculptMove(x, y);
            else              UpdateSculptBrushCursor(x, y);
        } else if (renderer_) {
            renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        }
    }
}

void Viewport::OnScroll(float /*xOffset*/, float yOffset) {
    if (yOffset == 0.0f) return;
    camera_->Zoom(yOffset);
    if (OnCameraChanged) OnCameraChanged();
}

void Viewport::OnKey(int key, int /*scancode*/, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    const bool noMods = (mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL |
                                  GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0;

    bool inAlignment = project_ && project_->GetCurrentStage() == WorkflowStage::Alignment;
    bool inPointRef  = project_ && project_->GetCurrentStage() == WorkflowStage::PointReference;
    bool inTouchUp   = project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp;
    bool hasTarget   = project_ && project_->GetTargetMesh().isLoaded;

    switch (key) {
        case GLFW_KEY_G:
            if (inAlignment && hasTarget) SetTransformMode(TransformMode::Move);
            break;
        case GLFW_KEY_R:
            if (inAlignment && hasTarget) SetTransformMode(TransformMode::Rotate);
            break;
        case GLFW_KEY_S:
            if (inAlignment && hasTarget) SetTransformMode(TransformMode::Scale);
            break;

        case GLFW_KEY_X:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::X)
                    ? AxisConstraint::None : AxisConstraint::X;
                if (OnTransformModeChanged) OnTransformModeChanged(transformMode_, axisConstraint_);
            }
            break;
        case GLFW_KEY_Y:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::Y)
                    ? AxisConstraint::None : AxisConstraint::Y;
                if (OnTransformModeChanged) OnTransformModeChanged(transformMode_, axisConstraint_);
            }
            break;
        case GLFW_KEY_Z:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::Z)
                    ? AxisConstraint::None : AxisConstraint::Z;
                if (OnTransformModeChanged) OnTransformModeChanged(transformMode_, axisConstraint_);
            }
            break;

        case GLFW_KEY_LEFT_BRACKET:
            if (inTouchUp) {
                float r = GetBrushRadius();
                float step = std::max(0.1f, r * 0.1f);
                float nr = std::max(0.1f, r - step);
                SetBrushRadius(nr);
                if (OnBrushRadiusChanged) OnBrushRadiusChanged(nr);
            }
            break;
        case GLFW_KEY_RIGHT_BRACKET:
            if (inTouchUp) {
                float r = GetBrushRadius();
                float step = std::max(0.1f, r * 0.1f);
                float nr = std::min(100.0f, r + step);
                SetBrushRadius(nr);
                if (OnBrushRadiusChanged) OnBrushRadiusChanged(nr);
            }
            break;

        case GLFW_KEY_ESCAPE:
            if (transformMode_ != TransformMode::None) {
                CancelTransform();
            } else if (inPointRef && selectedPointIndex_ >= 0) {
                SetSelectedPointIndex(-1);
                if (OnPointSelected) OnPointSelected(-1);
            }
            break;

        case GLFW_KEY_DELETE:
            if (inPointRef && selectedPointIndex_ >= 0) {
                if (OnPointDeleteRequested) OnPointDeleteRequested();
            }
            break;

        case GLFW_KEY_HOME:
            camera_->Reset();
            if (OnCameraChanged) OnCameraChanged();
            break;

        case GLFW_KEY_KP_1:
            if (noMods) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicFront);
                camera_->SetPosition(Vector3(0.0f, 0.0f, 5.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                if (OnCameraChanged) OnCameraChanged();
            }
            break;
        case GLFW_KEY_KP_3:
            if (noMods) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicRight);
                camera_->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                if (OnCameraChanged) OnCameraChanged();
            }
            break;
        case GLFW_KEY_KP_7:
            if (noMods) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicTop);
                camera_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                if (OnCameraChanged) OnCameraChanged();
            }
            break;
        case GLFW_KEY_KP_5:
            if (noMods) {
                camera_->SetProjectionMode(
                    camera_->GetProjectionMode() == ProjectionMode::Perspective
                        ? ProjectionMode::OrthographicFront
                        : ProjectionMode::Perspective);
                if (OnCameraChanged) OnCameraChanged();
            }
            break;

        case GLFW_KEY_F: {
            BoundingBox combined;
            bool has = false;
            auto addMesh = [&](const MeshReference& mr) {
                if (!mr.isLoaded || !mr.mesh) return;
                const BoundingBox& b = mr.mesh->GetBounds();
                Vector3 c = b.Center();
                Vector3 p = mr.transform.GetPosition();
                Vector3 tc(c.x + p.x, c.y + p.y, c.z + p.z);
                Vector3 hs((b.max.x - b.min.x) * 0.5f,
                           (b.max.y - b.min.y) * 0.5f,
                           (b.max.z - b.min.z) * 0.5f);
                BoundingBox tb;
                tb.min = Vector3(tc.x - hs.x, tc.y - hs.y, tc.z - hs.z);
                tb.max = Vector3(tc.x + hs.x, tc.y + hs.y, tc.z + hs.z);
                if (!has) { combined = tb; has = true; }
                else {
                    combined.min.x = std::min(combined.min.x, tb.min.x);
                    combined.min.y = std::min(combined.min.y, tb.min.y);
                    combined.min.z = std::min(combined.min.z, tb.min.z);
                    combined.max.x = std::max(combined.max.x, tb.max.x);
                    combined.max.y = std::max(combined.max.y, tb.max.y);
                    combined.max.z = std::max(combined.max.z, tb.max.z);
                }
            };
            if (project_) {
                addMesh(project_->GetMorphMesh());
                addMesh(project_->GetTargetMesh());
            }
            if (has) camera_->FocusOnBounds(combined);
            else     camera_->Reset();
            if (OnCameraChanged) OnCameraChanged();
            break;
        }
        default: break;
    }
}

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

    // Clip draw-list overlays to the viewport rect so labels and gizmo axes
    // never bleed over the sidebar, the other pane, or the main menu bar.
    // Floating ImGui windows (Sprint 3.8.5 toolbar / pills / HUD) are clipped
    // separately via their own window rects — they don't need this scope.
    ImDrawList* fgDl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
    fgDl->PushClipRect(ImVec2(x, y), ImVec2(x + w, y + h), true);

    DrawOrientationGizmoOverlay(x, y, w);
    DrawPointLabelsOverlay(x, y, w, h);
    DrawActiveBorderOverlay(x, y, w, h);
    // Viewport label is drawn inside the clip so it can't overlap the menu
    // bar even if the viewport rect is mis-sized.
    DrawViewportLabelOverlay(x, y, w, h);

    fgDl->PopClipRect();

    // Sprint 3.8.5 — three floating chrome windows anchored to the viewport
    // rect. Render last so they sit above the GL pass and the other overlays.
    DrawTopCenterToolbar(x, y, w, h);
    DrawTopRightPills   (x, y, w, h);
    DrawBottomLeftHud   (x, y, w, h);
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

    // Sprint 3.8.5 — viewport bg matches MvBg0 (deep navy) so the chrome
    // overlays sit on the same surface as the design's blueprint void.
    glClearColor(MvStyle::MvBg0.x, MvStyle::MvBg0.y, MvStyle::MvBg0.z, 1.0f);
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

void Viewport::DrawViewportLabelOverlay(float x, float y, float w, float h) {
    if (viewportLabel_.empty()) return;
    (void)w;
    ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
    const std::string& label = viewportLabel_;
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label.c_str());

    // Anchor to the bottom-left of the viewport rect. Keeps the label clear
    // of the main menu bar at the top of the window and the shading combo
    // at the top-right of the viewport.
    float rectH = textSize.y + 8.0f;
    float rectW = textSize.x + 16.0f;
    ImVec2 tl(x + 8.0f, y + h - rectH - 8.0f);
    ImVec2 br(tl.x + rectW, tl.y + rectH);
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
    // Sprint 3.8.5 — axis colours pulled from MvStyle (MvAxisX/Y/Z) so they
    // match the design's transform-input axis tint. RGB convention preserved.
    auto packStyle = [](const ImVec4& v) {
        return ImGui::ColorConvertFloat4ToU32(v);
    };
    AxisLabel axes[3] = {
        {Vector3(axisLen, 0, 0), packStyle(MvStyle::MvAxisX), "X"},
        {Vector3(0, axisLen, 0), packStyle(MvStyle::MvAxisY), "Y"},
        {Vector3(0, 0, axisLen), packStyle(MvStyle::MvAxisZ), "Z"},
    };

    ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    // Project the origin once — every axis line starts here.
    auto projectToScreen = [&](const Vector3& p, ImVec2& out) -> bool {
        float cx = vp.m[0]*p.x + vp.m[4]*p.y + vp.m[8]*p.z  + vp.m[12];
        float cy = vp.m[1]*p.x + vp.m[5]*p.y + vp.m[9]*p.z  + vp.m[13];
        float cw = vp.m[3]*p.x + vp.m[7]*p.y + vp.m[11]*p.z + vp.m[15];
        if (std::abs(cw) < 0.0001f) return false;
        float ndcX = cx / cw;
        float ndcY = cy / cw;
        out.x = gx + (ndcX * 0.5f + 0.5f) * gizmoSize;
        out.y = gy + (1.0f - (ndcY * 0.5f + 0.5f)) * gizmoSize;
        return true;
    };

    ImVec2 originScreen;
    if (!projectToScreen(Vector3(0, 0, 0), originScreen)) return;

    for (const auto& axis : axes) {
        ImVec2 tipScreen;
        if (!projectToScreen(axis.tip, tipScreen)) continue;
        dl->AddLine(originScreen, tipScreen, axis.color, 2.0f);
        dl->AddCircleFilled(tipScreen, 3.0f, axis.color);
        dl->AddText(font, fontSize,
                    ImVec2(tipScreen.x - 4.0f, tipScreen.y - fontSize * 0.5f - 10.0f),
                    axis.color, axis.label);
    }
}

void Viewport::DrawPointLabelsOverlay(float x, float y, float w, float h) {
    if (!project_) return;
    if (project_->GetCurrentStage() != WorkflowStage::PointReference) return;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
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

        // Sprint 3.8.5 — selected = MvBlue500 fill (matches design's selection
        // accent), unselected = MvBg0 with 70% alpha so it reads against the
        // mesh underneath. Text uses MvFg1.
        ImU32 bgColor;
        if (isSelected) {
            ImVec4 c = MvStyle::MvBlue500; c.w = 0.85f;
            bgColor = ImGui::ColorConvertFloat4ToU32(c);
        } else {
            ImVec4 c = MvStyle::MvBg0; c.w = 0.70f;
            bgColor = ImGui::ColorConvertFloat4ToU32(c);
        }
        const ImU32 textColor = ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg1);
        dl->AddRectFilled(tl, br, bgColor, 3.0f);
        dl->AddText(font, fontSize,
                    ImVec2(tl.x + 3.0f, tl.y + 2.0f),
                    textColor, buf);
    }
}

void Viewport::DrawActiveBorderOverlay(float x, float y, float w, float h) {
    if (!isActive_) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
    const ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(MvStyle::MvBlue500);
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), borderCol, 0.0f, 0, 2.0f);
}

// =============================================================================
// Story 3.8.5 — viewport chrome: top-centre toolbar, top-right pills, and
// bottom-left instruction HUD. Each is a borderless ImGui window pinned to
// the viewport rect by SetNextWindowPos. The dual-viewport scenario (Stage 2)
// renders these per pane; the per-pane window IDs include `(void*)this` so
// the primary and secondary viewports never share state.
// =============================================================================

namespace {

// Convert an ImVec4 to packed RGBA32 — used heavily by the new chrome.
inline ImU32 ColU(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

// Override the alpha of an MvStyle colour without mutating the source.
inline ImVec4 WithAlpha(const ImVec4& v, float a) {
    return ImVec4(v.x, v.y, v.z, a);
}

// 28×26 icon button — used by the top-centre toolbar. Returns true on click.
// Active state fills MvBlue500 with dark icon; hover state fills MvBg3.
bool ToolbarIconButton(const char* idGlyph, const char* glyph, bool active,
                       const char* tooltip) {
    constexpr float W = 28.0f;
    constexpr float H = 26.0f;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::PushID(idGlyph);
    const bool clicked = ImGui::InvisibleButton("##b", ImVec2(W, H));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pmax(pos.x + W, pos.y + H);
    if (active) {
        dl->AddRectFilled(pos, pmax, ColU(MetaVisage::MvStyle::MvBlue500), 4.0f);
    } else if (hovered) {
        dl->AddRectFilled(pos, pmax, ColU(MetaVisage::MvStyle::MvBg3), 4.0f);
    }

    const ImU32 iconCol = active
        ? IM_COL32(0x05, 0x13, 0x22, 0xFF)
        : ColU(MetaVisage::MvStyle::MvFg2);
    if (glyph && MetaVisage::MvStyle::Font.Icon) {
        ImFont* font = MetaVisage::MvStyle::Font.Icon;
        const float size = 14.0f;
        const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, glyph);
        dl->AddText(font, size,
                    ImVec2(pos.x + (W - ts.x) * 0.5f,
                           pos.y + (H - ts.y) * 0.5f),
                    iconCol, glyph);
    }

    if (hovered && tooltip) ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

// 1px MvLine2 vertical separator inside the toolbar. Advances the cursor by 9px.
void ToolbarSeparator() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    constexpr float gap = 9.0f;
    constexpr float h = 18.0f;
    const float yMid = pos.y + 13.0f;
    dl->AddLine(ImVec2(pos.x + 4.0f, yMid - h * 0.5f),
                ImVec2(pos.x + 4.0f, yMid + h * 0.5f),
                ColU(MetaVisage::MvStyle::MvLine2), 1.0f);
    ImGui::Dummy(ImVec2(gap, 26.0f));
}

// "PERSP" / "F · ⇧ axis" mono label, vertically centred on the 26 px button
// row so it shares the toolbar's baseline. The earlier Dummy(0,yPad) +
// SameLine pattern only advanced the cursor by yPad horizontally — it
// didn't actually shift the text's Y position, leaving the label visually
// floating above the row's mid-line. We now emit the glyph via ImDrawList
// at the correct Y and reserve a 26 px-tall layout dummy so SameLine
// continues to work for the next inline element.
void ToolbarMonoLabel(const char* text, const ImVec4& color) {
    MetaVisage::MvStyle::ScopedFont f(MetaVisage::MvStyle::Font.Mono);
    const float fs = ImGui::GetFontSize();
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(pos.x, pos.y + (26.0f - fs) * 0.5f),
                ColU(color), text);

    ImGui::Dummy(ImVec2(ts.x, 26.0f));
}

} // namespace

void Viewport::DrawTopCenterToolbar(float x, float y, float w, float h) {
    (void)h;
    if (w <= 0.0f) return;

    const WorkflowStage stage = project_ ? project_->GetCurrentStage()
                                         : WorkflowStage::Alignment;

    // Story 3.8.5 — Stage 2 (Point Reference) is the one stage that does
    // NOT show the centred toolbar. Story 3.8.8 will replace it with a
    // dedicated source/target info bar above the dual viewports. The HUD
    // and pills still render in Stage 2, just not this strip.
    if (stage == WorkflowStage::PointReference) return;

    const bool isStage3 = (stage == WorkflowStage::Morph);

    char wid[64];
    std::snprintf(wid, sizeof(wid), "##VpToolbar_%p", static_cast<void*>(this));

    ImGui::SetNextWindowPos(ImVec2(x + w * 0.5f, y + 14.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.85f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings |
        // NoFocusOnAppearing keeps the toolbar from grabbing keyboard focus
        // on its first frame; we deliberately omit NoBringToFrontOnFocus so
        // a click on a toolbar button raises the window above the host
        // window's central dockspace node — otherwise the click passes
        // through to the (transparent) viewport, never reaches the button,
        // and the toggle appears to do nothing. The pre-3.8.5
        // DrawShadingModeOverlay worked because it didn't set this flag.
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(5.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(2.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, MvStyle::MvBg1);
    ImGui::PushStyleColor(ImGuiCol_Border,   MvStyle::MvLine2);

    if (ImGui::Begin(wid, nullptr, flags)) {
        // PERSP label — projection mode indicator.
        ImGui::Dummy(ImVec2(6.0f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ToolbarMonoLabel("PERSP", MvStyle::MvBlue400);
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::Dummy(ImVec2(2.0f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ToolbarSeparator();

        // Shading mode toggles — Solid / Wireframe / Textured. Reads/writes
        // shadingMode_ directly. SolidWireframe enum value remains for legacy
        // callers; the toolbar only surfaces three modes.
        const ShadingMode sm = GetShadingMode();
        ImGui::SameLine(0.0f, 0.0f);
        if (ToolbarIconButton("solid", MV_ICON_CIRCLE,
                              sm == ShadingMode::Solid, "Solid")) {
            SetShadingMode(ShadingMode::Solid);
        }
        ImGui::SameLine(0.0f, 0.0f);
        if (ToolbarIconButton("wire", MV_ICON_BOX,
                              sm == ShadingMode::Wireframe, "Wireframe")) {
            SetShadingMode(ShadingMode::Wireframe);
        }
        ImGui::SameLine(0.0f, 0.0f);
        if (ToolbarIconButton("tex", MV_ICON_IMAGE,
                              sm == ShadingMode::Textured, "Textured")) {
            SetShadingMode(ShadingMode::Textured);
        }
        ImGui::SameLine(0.0f, 0.0f);
        ToolbarSeparator();

        if (isStage3) {
            // Stage 3 — Source / Target / Deformed view-mode toggles. The
            // canonical preview state lives in Project::MorphData::previewMode
            // (the sidebar combo writes there too); AppWindow's per-frame
            // mirror at AppWindow.cpp:597 pushes that into the renderer. So
            // we WRITE to project state, not directly to the renderer —
            // otherwise next frame's mirror clobbers the renderer back to
            // the stale project value and the toggle appears to do nothing.
            // We expose 3 of the 4 enum values; HeatMap stays in the
            // sidebar's Compare panel.
            //   Source   → Original
            //   Target   → Overlay      (deformed + semi-transparent target)
            //   Deformed → Deformed
            const MorphPreviewMode pm = project_ ? project_->GetMorphData().previewMode
                                                  : MorphPreviewMode::Deformed;
            auto setPreview = [&](MorphPreviewMode mode) {
                if (project_) project_->GetMorphData().previewMode = mode;
                SetMorphPreviewMode(mode);  // immediate, avoids 1-frame lag
            };
            ImGui::SameLine(0.0f, 0.0f);
            if (ToolbarIconButton("vmSrc", MV_ICON_CIRCLE_DASHED,
                                  pm == MorphPreviewMode::Original, "Source")) {
                setPreview(MorphPreviewMode::Original);
            }
            ImGui::SameLine(0.0f, 0.0f);
            if (ToolbarIconButton("vmTgt", MV_ICON_TARGET,
                                  pm == MorphPreviewMode::Overlay, "Target overlay")) {
                setPreview(MorphPreviewMode::Overlay);
            }
            ImGui::SameLine(0.0f, 0.0f);
            if (ToolbarIconButton("vmDef", MV_ICON_SPARKLES,
                                  pm == MorphPreviewMode::Deformed, "Deformed result")) {
                setPreview(MorphPreviewMode::Deformed);
            }
            ImGui::SameLine(0.0f, 0.0f);
            ToolbarSeparator();

            // Process-status pill — three states driven by AppWindow's
            // per-frame Viewport::SetMorphStatus call:
            //   Ready      blue   — no morph yet
            //   Processing amber  — worker running
            //   Complete   green  — morph isProcessed
            ImVec4 dotCol;
            const char* statusText;
            switch (morphStatus_) {
                case MorphStatus::Processing:
                    dotCol = MvStyle::MvWarning; statusText = "PROCESSING"; break;
                case MorphStatus::Complete:
                    dotCol = MvStyle::MvSuccess; statusText = "COMPLETE";   break;
                case MorphStatus::Ready:
                default:
                    dotCol = MvStyle::MvBlue400; statusText = "READY";      break;
            }
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 dotPos = ImGui::GetCursorScreenPos();
            dl->AddCircleFilled(ImVec2(dotPos.x + 8.0f, dotPos.y + 13.0f),
                                3.5f, ColU(dotCol));
            ImGui::Dummy(ImVec2(16.0f, 26.0f));
            ImGui::SameLine(0.0f, 0.0f);
            ToolbarMonoLabel(statusText, dotCol);
            ImGui::SameLine(0.0f, 6.0f);
        } else {
            // Stages 1, 2, 4 — Source / Target eye-toggle visibility. The
            // RenderFilter enum is mutually exclusive (All / MorphOnly /
            // TargetOnly), so we derive two booleans, flip the relevant one,
            // and recompose. Never let both go off — fall back to All.
            const RenderFilter rf = renderFilter_;
            const bool srcVis = (rf != RenderFilter::TargetOnly);
            const bool tgtVis = (rf != RenderFilter::MorphOnly);

            ImGui::SameLine(0.0f, 0.0f);
            if (ToolbarIconButton("vSrc",
                                  srcVis ? MV_ICON_EYE : MV_ICON_EYE_OFF,
                                  srcVis, "Show source")) {
                bool ns = !srcVis;
                bool nt = tgtVis;
                if (!ns && !nt) ns = true;
                RenderFilter newF = RenderFilter::All;
                if (ns && !nt)      newF = RenderFilter::MorphOnly;
                else if (!ns && nt) newF = RenderFilter::TargetOnly;
                SetRenderFilter(newF);
            }
            ImGui::SameLine(0.0f, 0.0f);
            if (ToolbarIconButton("vTgt",
                                  tgtVis ? MV_ICON_EYE : MV_ICON_EYE_OFF,
                                  tgtVis, "Show target")) {
                bool ns = srcVis;
                bool nt = !tgtVis;
                if (!ns && !nt) nt = true;
                RenderFilter newF = RenderFilter::All;
                if (ns && !nt)      newF = RenderFilter::MorphOnly;
                else if (!ns && nt) newF = RenderFilter::TargetOnly;
                SetRenderFilter(newF);
            }
            ImGui::SameLine(0.0f, 0.0f);
            ToolbarSeparator();

            // Hotkey strip — tiny mono mnemonic. Stage-specific text.
            const char* hotkeyStrip;
            switch (stage) {
                case WorkflowStage::PointReference: hotkeyStrip = "M mirror \xC2\xB7 DEL clear"; break;
                case WorkflowStage::TouchUp:        hotkeyStrip = "[ ] radius \xC2\xB7 SHIFT smooth"; break;
                case WorkflowStage::Alignment:
                default:                            hotkeyStrip = "F focus \xC2\xB7 \xE2\x87\xA7 axis"; break;
            }
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::Dummy(ImVec2(6.0f, 0.0f));
            ImGui::SameLine(0.0f, 0.0f);
            ToolbarMonoLabel(hotkeyStrip, MvStyle::MvFg3);
            ImGui::SameLine(0.0f, 6.0f);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void Viewport::DrawTopRightPills(float x, float y, float w, float h) {
    (void)h;
    if (w <= 0.0f) return;

    const WorkflowStage stage = project_ ? project_->GetCurrentStage()
                                         : WorkflowStage::Alignment;

    char wid[64];
    std::snprintf(wid, sizeof(wid), "##VpPills_%p", static_cast<void*>(this));

    // Anchored top-right of the viewport, offset inward by the orientation
    // gizmo's footprint (~96 px) so it doesn't collide with it.
    constexpr float GizmoFootprint = 96.0f;
    ImGui::SetNextWindowPos(ImVec2(x + w - GizmoFootprint, y + 14.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings |
        // NoFocusOnAppearing keeps the toolbar from grabbing keyboard focus
        // on its first frame; we deliberately omit NoBringToFrontOnFocus so
        // a click on a toolbar button raises the window above the host
        // window's central dockspace node — otherwise the click passes
        // through to the (transparent) viewport, never reaches the button,
        // and the toggle appears to do nothing. The pre-3.8.5
        // DrawShadingModeOverlay worked because it didn't set this flag.
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));

    if (ImGui::Begin(wid, nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Single-pill draw helper — frosted dark MvBg1 fill, MvLine2 border,
        // optional dot prefix in `dotCol`. Returns the pill's rect width.
        auto drawPill = [&](const char* text, const ImVec4* dotColor) {
            MvStyle::ScopedFont f(MvStyle::Font.Mono);
            const ImVec2 ts = ImGui::CalcTextSize(text);
            const float dotPad = dotColor ? 12.0f : 0.0f;
            const float pillW = ts.x + dotPad + 14.0f;
            constexpr float pillH = 22.0f;

            const ImVec2 pos = ImGui::GetCursorScreenPos();
            const ImVec2 pmax(pos.x + pillW, pos.y + pillH);
            ImVec4 bg = MvStyle::MvBg1; bg.w = 0.85f;
            dl->AddRectFilled(pos, pmax, ColU(bg), 4.0f);
            dl->AddRect      (pos, pmax, ColU(MvStyle::MvLine2), 4.0f, 0, 1.0f);

            float textX = pos.x + 7.0f;
            if (dotColor) {
                dl->AddCircleFilled(ImVec2(pos.x + 8.0f, pos.y + pillH * 0.5f),
                                    3.0f, ColU(*dotColor));
                textX += dotPad;
            }
            const float textY = pos.y + (pillH - ImGui::GetFontSize()) * 0.5f;
            dl->AddText(ImVec2(textX, textY), ColU(MvStyle::MvFg2), text);

            ImGui::Dummy(ImVec2(pillW, pillH));
        };

        // LIVE pill — always shown.
        const ImVec4 liveDot = MvStyle::MvBlue400;
        drawPill("LIVE", &liveDot);

        // Stage 4 — strength % pill alongside LIVE.
        if (stage == WorkflowStage::TouchUp && project_) {
            const float strength = project_->GetSculptState().brushStrength;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "STRENGTH %d%%",
                          static_cast<int>(strength * 100.0f + 0.5f));
            ImGui::SameLine();
            drawPill(buf, nullptr);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void Viewport::DrawBottomLeftHud(float x, float y, float w, float h) {
    if (w <= 0.0f || h <= 0.0f) return;

    const WorkflowStage stage = project_ ? project_->GetCurrentStage()
                                         : WorkflowStage::Alignment;

    // Stage-specific copy. Kept terse so the HUD stays compact at 320 px.
    struct StageHud {
        const char* label;
        const char* instruction;
        const char* kbds[4];   // each "Key,Action"; nullptr-terminated entry stops
    };
    // MSVC C4701: the switch covers every WorkflowStage value but the
    // compiler can't prove enum exhaustivity at this site. Initialise to
    // Stage 1's copy as the safe default, then overwrite per case.
    StageHud hud = { "/ Stage 1 \xC2\xB7 Alignment",
                     "Position the source roughly over the MetaHuman target. "
                     "Hit F to focus, drag the gizmo handles to translate.",
                     {"W,move", "E,rotate", "R,scale", nullptr} };
    switch (stage) {
        case WorkflowStage::Alignment:
            // Already in default initializer.
            break;
        case WorkflowStage::PointReference:
            hud = { "/ Stage 2 \xC2\xB7 Point reference",
                    "Click each mesh at matching landmarks. Place at least one "
                    "matched pair before continuing.",
                    {"M,mirror", "DEL,remove", "F,focus", nullptr} };
            break;
        case WorkflowStage::Morph:
            hud = { "/ Stage 3 \xC2\xB7 Morph",
                    "Tune parameters and process the wrap. Compare Source / "
                    "Target / Deformed to verify the result.",
                    {"P,process", "ESC,cancel", nullptr, nullptr} };
            break;
        case WorkflowStage::TouchUp:
            hud = { "/ Stage 4 \xC2\xB7 Touch up",
                    "Pick a brush and scrub the mesh to refine the wrap. "
                    "Bracket keys adjust radius; Shift smooths.",
                    {"[,radius -", "],radius +", "SHIFT,smooth", nullptr} };
            break;
    }

    char wid[64];
    std::snprintf(wid, sizeof(wid), "##VpHud_%p", static_cast<void*>(this));

    constexpr float HudW = 320.0f;
    ImGui::SetNextWindowPos(ImVec2(x + 16.0f, y + h - 16.0f),
                            ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(HudW, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.88f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings |
        // NoFocusOnAppearing keeps the toolbar from grabbing keyboard focus
        // on its first frame; we deliberately omit NoBringToFrontOnFocus so
        // a click on a toolbar button raises the window above the host
        // window's central dockspace node — otherwise the click passes
        // through to the (transparent) viewport, never reaches the button,
        // and the toggle appears to do nothing. The pre-3.8.5
        // DrawShadingModeOverlay worked because it didn't set this flag.
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(14.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, MvStyle::MvBg1);
    ImGui::PushStyleColor(ImGuiCol_Border,   MvStyle::MvLine2);

    if (ImGui::Begin(wid, nullptr, flags)) {
        // Header — mono stage label on the left, "HINT" tag on the right.
        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            ImGui::TextUnformatted(hud.label);
            ImGui::PopStyleColor();

            const ImVec2 ts = ImGui::CalcTextSize("HINT");
            ImGui::SameLine(0.0f, ImGui::GetContentRegionAvail().x - ts.x - 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue400);
            ImGui::TextUnformatted("HINT");
            ImGui::PopStyleColor();
        }
        ImGui::Spacing();

        // Instruction — body text wrapped to the HUD width.
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
            ImGui::TextWrapped("%s", hud.instruction);
            ImGui::PopStyleColor();
        }

        // Hotkey strip — Kbd-style rounded chips. Each chip is "KEY label".
        ImGui::Spacing();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
        for (int i = 0; i < 4 && hud.kbds[i]; ++i) {
            const char* entry = hud.kbds[i];
            const char* comma = std::strchr(entry, ',');
            const std::string keyText = comma ? std::string(entry, comma) : std::string(entry);
            const std::string label   = comma ? std::string(comma + 1) : std::string();

            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            const float kFontH = ImGui::GetFontSize();

            const ImVec2 keySize = ImGui::CalcTextSize(keyText.c_str());
            constexpr float chipPadX = 6.0f;
            constexpr float chipPadY = 2.0f;
            const float chipW = keySize.x + chipPadX * 2.0f;
            const float chipH = kFontH + chipPadY * 2.0f;

            const ImVec2 pos = ImGui::GetCursorScreenPos();
            const ImVec2 pmax(pos.x + chipW, pos.y + chipH);
            dl->AddRectFilled(pos, pmax, ColU(MvStyle::MvBg3), 3.0f);
            dl->AddRect      (pos, pmax, ColU(MvStyle::MvLine2), 3.0f, 0, 1.0f);
            dl->AddText(ImVec2(pos.x + chipPadX, pos.y + chipPadY),
                        ColU(MvStyle::MvFg1), keyText.c_str());

            ImGui::Dummy(ImVec2(chipW, chipH));
            if (!label.empty()) {
                ImGui::SameLine(0.0f, 4.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
                ImGui::TextUnformatted(label.c_str());
                ImGui::PopStyleColor();
            }
            if (i + 1 < 4 && hud.kbds[i + 1]) {
                ImGui::SameLine(0.0f, 10.0f);
            }
        }
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

// --- Stage 2 point placement (ported from Qt ViewportWidget::HandlePointClick) ---

PointSide Viewport::GetViewportPointSide() const {
    if (renderFilter_ == RenderFilter::TargetOnly) return PointSide::Target;
    if (renderFilter_ == RenderFilter::MorphOnly)  return PointSide::Morph;
    // Default: single-viewport mode showing everything treats clicks as target.
    return PointSide::Target;
}

bool Viewport::TrySelectExistingPoint(float screenX, float screenY) {
    if (!project_) return false;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return false;

    const float selectRadius = 15.0f; // pixels
    float closestDist = selectRadius;
    int closestIdx = -1;

    PointSide side = GetViewportPointSide();
    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return false;

    for (size_t i = 0; i < correspondences.size(); ++i) {
        const auto& corr = correspondences[i];

        Vector3 worldPos;
        bool hasPoint = false;
        if (side == PointSide::Target && corr.targetMeshVertexIndex >= 0) {
            worldPos = corr.targetMeshPosition;
            hasPoint = true;
        } else if (side == PointSide::Morph && corr.morphMeshVertexIndex >= 0) {
            worldPos = corr.morphMeshPosition;
            hasPoint = true;
        }
        if (!hasPoint) continue;

        Vector3 screenPos = RayCaster::WorldToScreen(worldPos, vpW, vpH, *camera_);

        // Skip if behind camera / clipped.
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) continue;

        float dx = screenPos.x - screenX;
        float dy = screenPos.y - screenY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < closestDist) {
            closestDist = dist;
            closestIdx = static_cast<int>(i);
        }
    }

    if (closestIdx >= 0) {
        SetSelectedPointIndex(closestIdx);
        if (OnPointSelected) OnPointSelected(closestIdx);
        return true;
    }
    return false;
}

void Viewport::HandlePointClick(float screenX, float screenY) {
    if (!project_) return;

    // First, try to select an existing point near the click.
    if (TrySelectExistingPoint(screenX, screenY)) return;

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);

    PointSide side = GetViewportPointSide();
    RaycastHit hit;

    auto raycast = [&](MeshReference& mr) {
        if (!mr.isLoaded || !mr.mesh) return;
        BVH* bvh = mr.mesh->GetBVH();
        hit = bvh
            ? RayCaster::RayIntersectMeshBVH(ray, *mr.mesh, mr.transform, *bvh)
            : RayCaster::RayIntersectMesh   (ray, *mr.mesh, mr.transform);
    };

    if (side == PointSide::Target) {
        raycast(project_->GetTargetMesh());
    } else {
        raycast(project_->GetMorphMesh());
    }

    if (hit.hit) {
        if (OnPointPlaced) OnPointPlaced(side, hit.position, hit.vertexIndex);
    } else {
        // Click on empty space: deselect any current selection.
        SetSelectedPointIndex(-1);
        if (OnPointSelected) OnPointSelected(-1);
    }
}

// --- Stage 3 mask painting (ported from Qt HandleMask*) ---

Mesh* Viewport::GetMaskMesh() {
    if (!project_) return nullptr;
    // Prefer the processed deformed mesh so the mask aligns with what the user
    // sees. Before processing, fall back to the undeformed morph mesh so the
    // user can pre-paint regions before their first Process run.
    const MorphData& morphData = project_->GetMorphData();
    if (morphData.isProcessed && morphData.deformedMorphMesh) {
        return morphData.deformedMorphMesh.get();
    }
    auto& morphRef = project_->GetMorphMesh();
    if (morphRef.isLoaded && morphRef.mesh) return morphRef.mesh.get();
    return nullptr;
}

Transform Viewport::GetMaskTransform() const {
    if (project_) return project_->GetMorphMesh().transform;
    return Transform();
}

void Viewport::HandleMaskPress(float screenX, float screenY) {
    if (!project_ || !maskBrush_) return;

    Mesh* mesh = GetMaskMesh();
    if (!mesh) return;

    MorphData& morphData = project_->GetMorphData();
    const size_t vertCount = mesh->GetVertices().size();

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetMaskTransform();
    BVH* bvh = mesh->GetBVH();
    RaycastHit hit = bvh
        ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
        : RayCaster::RayIntersectMesh(ray, *mesh, transform);

    // Bail before allocating the mask buffer if the click missed — avoids
    // leaving a non-empty all-false mask that the renderer's Show-Mask
    // short-circuit would then treat as "user has a mask, draw mask colors".
    if (!hit.hit) return;

    // Snapshot for undo BEFORE we resize the buffer or apply paint, so a
    // first-ever stroke restores correctly to "no mask" on Ctrl+Z.
    maskBeforeStroke_ = morphData.vertexMask;

    // The mask buffer must match the mesh we're painting. Re-sizing is cheap
    // and idempotent — first-paint bootstraps the vector, later paints no-op.
    if (morphData.vertexMask.size() != vertCount) {
        morphData.vertexMask.assign(vertCount, false);
    }
    maskBrush_->SetMaskData(&morphData.vertexMask);

    isMaskStroking_ = true;
    maskBrush_->BeginStroke(*mesh, transform, hit.position);
    if (maskBrush_->Apply(*mesh, transform, hit.position,
                          Vector3(), Vector3(), 0.016f)) {
        if (renderer_) renderer_->UploadMaskColors(mesh, morphData.vertexMask);
    }
    if (renderer_) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        renderer_->SetBrushCursor(hit.position, worldNormal,
                                  maskBrush_->GetRadius(), 0.0f, true);
    }
}

void Viewport::HandleMaskMove(float screenX, float screenY) {
    if (!project_ || !maskBrush_) return;
    Mesh* mesh = GetMaskMesh();
    if (!mesh) return;

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetMaskTransform();
    BVH* bvh = mesh->GetBVH();
    RaycastHit hit = bvh
        ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
        : RayCaster::RayIntersectMesh(ray, *mesh, transform);
    if (!hit.hit) return;

    MorphData& morphData = project_->GetMorphData();
    if (maskBrush_->Apply(*mesh, transform, hit.position,
                          Vector3(), Vector3(), 0.016f)) {
        if (renderer_) renderer_->UploadMaskColors(mesh, morphData.vertexMask);
    }
    if (renderer_) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        renderer_->SetBrushCursor(hit.position, worldNormal,
                                  maskBrush_->GetRadius(), 0.0f, true);
    }
}

void Viewport::HandleMaskRelease() {
    if (!isMaskStroking_) return;
    // Hand the pre-stroke snapshot to AppWindow so it can push one
    // MaskStrokeUndoAction per gesture. Move-out clears maskBeforeStroke_.
    std::vector<bool> before = std::move(maskBeforeStroke_);
    if (maskBrush_) maskBrush_->EndStroke();
    isMaskStroking_ = false;
    if (OnMaskStrokeCompleted) OnMaskStrokeCompleted(std::move(before));
}

// --- Stage 4 sculpting (ported from Qt HandleSculpt*) ---

SculptBrush* Viewport::GetActiveBrush() {
    return PickActiveBrush(activeBrushType_, smoothBrush_.get(),
                           grabBrush_.get(), pushPullBrush_.get(),
                           inflateBrush_.get());
}

Mesh* Viewport::GetSculptMesh() {
    if (!project_) return nullptr;
    auto& morphData = project_->GetMorphData();
    if (morphData.deformedMorphMesh) return morphData.deformedMorphMesh.get();
    return nullptr;
}

Transform Viewport::GetSculptTransform() const {
    // Deformed morph is rendered with the morph mesh's transform (including
    // the default -90° Y rotation), so raycasting must use the same transform.
    if (project_ && project_->GetMorphMesh().isLoaded) {
        return project_->GetMorphMesh().transform;
    }
    return Transform();
}

Vector3 Viewport::ComputeWorldNormal(const Mesh& mesh, const Transform& transform,
                                     int vertexIndex) const {
    const auto& normals = mesh.GetNormals();
    if (vertexIndex < 0 || vertexIndex >= static_cast<int>(normals.size())) {
        return Vector3(0.0f, 1.0f, 0.0f);
    }
    const Vector3 localNormal = normals[vertexIndex];
    Matrix4x4 modelMatrix = transform.GetMatrix();
    Matrix4x4 normalMatrix = modelMatrix.Inverse().Transpose();
    Vector3 worldNormal(
        normalMatrix.m[0] * localNormal.x + normalMatrix.m[4] * localNormal.y + normalMatrix.m[8]  * localNormal.z,
        normalMatrix.m[1] * localNormal.x + normalMatrix.m[5] * localNormal.y + normalMatrix.m[9]  * localNormal.z,
        normalMatrix.m[2] * localNormal.x + normalMatrix.m[6] * localNormal.y + normalMatrix.m[10] * localNormal.z);
    return worldNormal.Normalized();
}

void Viewport::HandleSculptPress(float screenX, float screenY) {
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh || !project_) return;

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetSculptTransform();
    BVH* bvh = mesh->GetBVH();
    RaycastHit hit = bvh
        ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
        : RayCaster::RayIntersectMesh(ray, *mesh, transform);

    if (!hit.hit) return;

    isSculpting_ = true;
    lastBrushWorldPos_ = hit.position;
    currentStroke_ = BrushStroke();

    // Record all affected vertices' pre-stroke positions for undo.
    SpatialHash* sh = mesh->GetSpatialHash(brush->GetRadius() * 2.0f);
    auto affected = sh
        ? brush->GetAffectedVertices(*mesh, transform, hit.position, brush->GetRadius(), *sh)
        : brush->GetAffectedVertices(*mesh, transform, hit.position, brush->GetRadius());
    const auto& vertices = mesh->GetVertices();
    for (const auto& av : affected) {
        if (av.index >= 0 && av.index < static_cast<int>(vertices.size())) {
            currentStroke_.RecordVertex(av.index, vertices[av.index]);
        }
    }

    brush->BeginStroke(*mesh, transform, hit.position);

    // Smooth brush applies on press (doesn't need movement); Grab waits for
    // the first move sample to drag.
    if (activeBrushType_ == BrushType::Smooth) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        if (brush->Apply(*mesh, transform, hit.position, worldNormal,
                         Vector3(), 0.016f)) {
            mesh->CalculateNormals();
            if (renderer_) renderer_->UpdateMeshVertices(mesh);
        }

        if (sculptSymmetryEnabled_) {
            Vector3 mPos = hit.position;
            Vector3 mNorm = worldNormal;
            switch (sculptSymmetryAxis_) {
                case Axis::X: mPos.x = -mPos.x; mNorm.x = -mNorm.x; break;
                case Axis::Y: mPos.y = -mPos.y; mNorm.y = -mNorm.y; break;
                case Axis::Z: mPos.z = -mPos.z; mNorm.z = -mNorm.z; break;
            }
            auto mAffected = sh
                ? brush->GetAffectedVertices(*mesh, transform, mPos, brush->GetRadius(), *sh)
                : brush->GetAffectedVertices(*mesh, transform, mPos, brush->GetRadius());
            for (const auto& mav : mAffected) {
                if (mav.index >= 0 && mav.index < static_cast<int>(vertices.size())) {
                    currentStroke_.RecordVertex(mav.index, vertices[mav.index]);
                }
            }
            if (brush->Apply(*mesh, transform, mPos, mNorm, Vector3(), 0.016f)) {
                mesh->CalculateNormals();
                if (renderer_) renderer_->UpdateMeshVertices(mesh);
            }
        }
    }
}

void Viewport::HandleSculptMove(float screenX, float screenY) {
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh || !project_) return;

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetSculptTransform();

    Vector3 brushCenter = lastBrushWorldPos_;
    Vector3 worldNormal(0.0f, 1.0f, 0.0f);
    Vector3 mouseDelta;

    if (activeBrushType_ == BrushType::Grab) {
        // Grab: project the cursor onto a view-aligned plane through the last
        // brush position. Lets vertices be pulled off the mesh surface.
        Vector3 viewDir = (camera_->GetTarget() - camera_->GetPosition()).Normalized();
        Vector3 planeNormal = viewDir * -1.0f;
        float denom = planeNormal.Dot(ray.direction);
        if (std::abs(denom) > 1e-6f) {
            float t = planeNormal.Dot(lastBrushWorldPos_ - ray.origin) / denom;
            if (t > 0.0f) brushCenter = ray.PointAt(t);
        }
        mouseDelta = brushCenter - lastBrushWorldPos_;
        worldNormal = planeNormal;
    } else {
        // Other brushes: hug the mesh surface via BVH raycast.
        BVH* bvh = mesh->GetBVH();
        RaycastHit hit = bvh
            ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
            : RayCaster::RayIntersectMesh(ray, *mesh, transform);
        if (hit.hit) {
            brushCenter = hit.position;
            worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        }
        mouseDelta = brushCenter - lastBrushWorldPos_;

        // Record newly-affected vertices so the undo snapshot covers the whole
        // stroke footprint.
        SpatialHash* sh = mesh->GetSpatialHash(brush->GetRadius() * 2.0f);
        auto affected = sh
            ? brush->GetAffectedVertices(*mesh, transform, brushCenter, brush->GetRadius(), *sh)
            : brush->GetAffectedVertices(*mesh, transform, brushCenter, brush->GetRadius());
        const auto& vertices = mesh->GetVertices();
        for (const auto& av : affected) {
            if (av.index >= 0 && av.index < static_cast<int>(vertices.size())) {
                currentStroke_.RecordVertex(av.index, vertices[av.index]);
            }
        }
    }

    if (brush->Apply(*mesh, transform, brushCenter, worldNormal, mouseDelta, 0.016f)) {
        mesh->CalculateNormals();
        if (renderer_) renderer_->UpdateMeshVertices(mesh);
    }

    if (sculptSymmetryEnabled_) {
        Vector3 mCenter = brushCenter;
        Vector3 mNormal = worldNormal;
        Vector3 mDelta  = mouseDelta;
        switch (sculptSymmetryAxis_) {
            case Axis::X: mCenter.x = -mCenter.x; mNormal.x = -mNormal.x; mDelta.x = -mDelta.x; break;
            case Axis::Y: mCenter.y = -mCenter.y; mNormal.y = -mNormal.y; mDelta.y = -mDelta.y; break;
            case Axis::Z: mCenter.z = -mCenter.z; mNormal.z = -mNormal.z; mDelta.z = -mDelta.z; break;
        }
        SpatialHash* sh2 = mesh->GetSpatialHash(brush->GetRadius() * 2.0f);
        auto mAffected = sh2
            ? brush->GetAffectedVertices(*mesh, transform, mCenter, brush->GetRadius(), *sh2)
            : brush->GetAffectedVertices(*mesh, transform, mCenter, brush->GetRadius());
        const auto& verts = mesh->GetVertices();
        for (const auto& mav : mAffected) {
            if (mav.index >= 0 && mav.index < static_cast<int>(verts.size())) {
                currentStroke_.RecordVertex(mav.index, verts[mav.index]);
            }
        }
        if (brush->Apply(*mesh, transform, mCenter, mNormal, mDelta, 0.016f)) {
            mesh->CalculateNormals();
            if (renderer_) renderer_->UpdateMeshVertices(mesh);
        }
    }

    lastBrushWorldPos_ = brushCenter;

    // Keep the ring visible at the new brush center throughout the stroke.
    if (renderer_) {
        float innerRadius = brush->GetRadius() * GetFalloffInnerRatio(brush->GetFalloff());
        renderer_->SetBrushCursor(brushCenter, worldNormal,
                                  brush->GetRadius(), innerRadius, true);
    }
}

void Viewport::HandleSculptRelease() {
    if (!isSculpting_) return;
    if (SculptBrush* brush = GetActiveBrush()) brush->EndStroke();
    isSculpting_ = false;

    // Invalidate the BVH & spatial hash so the next stroke raycasts against
    // the current (post-edit) surface, not the pre-stroke cache.
    if (Mesh* mesh = GetSculptMesh()) {
        mesh->InvalidateAccelerationStructures();
    }

    if (!currentStroke_.IsEmpty() && OnSculptStrokeCompleted) {
        OnSculptStrokeCompleted(currentStroke_);
    }
}

// Hover-cursor updates — raycast the sculpt/mask mesh each move and hand the
// hit position + surface normal to the renderer so it can draw the outer
// radius ring and (for sculpt) the inner falloff ring.

void Viewport::UpdateSculptBrushCursor(float screenX, float screenY) {
    if (!renderer_) return;
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh) {
        renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        return;
    }

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetSculptTransform();
    BVH* bvh = mesh->GetBVH();
    RaycastHit hit = bvh
        ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
        : RayCaster::RayIntersectMesh(ray, *mesh, transform);

    if (hit.hit) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        float innerRadius = brush->GetRadius() * GetFalloffInnerRatio(brush->GetFalloff());
        renderer_->SetBrushCursor(hit.position, worldNormal,
                                  brush->GetRadius(), innerRadius, true);
        brushOnMesh_ = true;
        lastBrushWorldPos_ = hit.position;
    } else {
        renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        brushOnMesh_ = false;
    }
}

void Viewport::UpdateMaskBrushCursor(float screenX, float screenY) {
    if (!renderer_ || !maskBrush_) return;
    Mesh* mesh = GetMaskMesh();
    if (!mesh) {
        renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        return;
    }

    int vpW = static_cast<int>(std::ceil(lastW_));
    int vpH = static_cast<int>(std::ceil(lastH_));
    if (vpW <= 0 || vpH <= 0) return;

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, vpW, vpH, *camera_);
    Transform transform = GetMaskTransform();
    BVH* bvh = mesh->GetBVH();
    RaycastHit hit = bvh
        ? RayCaster::RayIntersectMeshBVH(ray, *mesh, transform, *bvh)
        : RayCaster::RayIntersectMesh(ray, *mesh, transform);

    if (hit.hit) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        // Mask has no falloff — inner ring = 0 suppresses the second ring.
        renderer_->SetBrushCursor(hit.position, worldNormal,
                                  maskBrush_->GetRadius(), 0.0f, true);
    } else {
        renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
    }
}

} // namespace MetaVisage
