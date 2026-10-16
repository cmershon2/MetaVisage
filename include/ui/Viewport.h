#ifndef METAVISAGE_VIEWPORT_H
#define METAVISAGE_VIEWPORT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/Camera.h"
#include "core/Types.h"
#include "core/Transform.h"
#include "rendering/GizmoRenderer.h"
#include "sculpting/BrushStroke.h"

namespace MetaVisage {

class Renderer;
class Project;
class TextureData;
class Mesh;
class SculptBrush;
class SmoothBrush;
class GrabBrush;
class PushPullBrush;
class InflateBrush;
class MaskBrush;

// Plain C++ viewport. Hosted inside an ImGui region by ViewportHost; the
// Render() call runs the 3D pass in the caller's current GL context, scissored
// to the pixel rectangle provided. Input handlers are stubs until Story 3.5.4
// wires GLFW callbacks. Sculpt/mask/point business logic bodies return in
// Sprint 3.6 when the StageManager and sidebar UI come back.
class Viewport {
public:
    Viewport();
    ~Viewport();

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    // Must be called once the GL context is current (AppWindow after GLEW init).
    bool Initialize();

    // Queue ImGui overlays (label, axis gizmo labels, point labels, active
    // border) for the given pixel rect. Called during ImGui frame build.
    void DrawOverlays(float x, float y, float w, float h);

    // Execute the 3D pass inside the pixel rectangle (x,y from top-left of the
    // framebuffer, w/h in pixels). fbHeight is the full framebuffer height for
    // the GL Y-flip. Called after ImGui::Render, before the ImGui draw pass.
    void RenderGL(float x, float y, float w, float h, int fbHeight);

    // --- Project / render config ---
    void SetProject(Project* project) { project_ = project; }
    Camera* GetCamera() { return camera_.get(); }
    const Camera& GetCameraConst() const { return *camera_; }

    // Story 5.4 — stable role identifier embedded in the viewport's
    // toolbar window ID so imgui_test_engine can address it without
    // knowing a runtime pointer. ViewportHost sets the secondary pane
    // to "secondary"; the default ("primary") covers single-pane stages
    // and the primary pane in dual mode. Two viewports must NEVER share
    // the same role at the same time — ImGui would collide on the
    // toolbar window's persistent ID.
    void SetRoleId(const char* role) { roleId_ = role; }
    const char* GetRoleId() const { return roleId_; }

    void SetShadingMode(ShadingMode mode);
    ShadingMode GetShadingMode() const;

    void SetRenderFilter(RenderFilter filter);
    RenderFilter GetRenderFilter() const { return renderFilter_; }

    void SyncCameraFrom(const Camera& other);

    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    void SetViewportLabel(const std::string& label) { viewportLabel_ = label; }

    // --- Transform mode (Alignment stage) ---
    TransformMode GetTransformMode() const { return transformMode_; }
    AxisConstraint GetAxisConstraint() const { return axisConstraint_; }
    GizmoAxis GetHoveredGizmoAxis() const { return hoveredGizmoAxis_; }
    GizmoAxis GetActiveGizmoAxis() const { return activeGizmoAxis_; }
    void SetTransformMode(TransformMode mode);
    void CancelTransform();

    // --- Point reference (Stage 2) ---
    void SetSelectedPointIndex(int index);
    int GetSelectedPointIndex() const { return selectedPointIndex_; }
    void SetPointSize(float size);

    // --- Morph stage ---
    void SetMorphPreviewMode(MorphPreviewMode mode);
    void InvalidateMesh(const Mesh* mesh);
    void UploadHeatMapColors(const Mesh* mesh,
                             const std::vector<float>& displacements,
                             float maxDisplacement);
    void UploadMeshTexture(const Mesh* mesh, const TextureData& texture);

    // --- Sculpting (Touch Up) ---
    void SetBrushType(BrushType type);
    void SetBrushRadius(float radius);
    void SetBrushStrength(float strength);
    void SetBrushFalloff(FalloffType falloff);
    BrushType GetBrushType() const { return activeBrushType_; }
    float GetBrushRadius() const;
    float GetBrushStrength() const;
    void RefreshMeshGPUData(Mesh* mesh);
    void SetSculptSymmetry(bool enabled, Axis axis);

    // --- Mask painting (Morph) ---
    void SetMaskPaintingMode(bool active) { isMaskPainting_ = active; }
    bool IsMaskPainting() const { return isMaskPainting_; }
    void SetMaskBrushRadius(float radius);
    void SetMaskEraseMode(bool erase);
    void SetMaskData(std::vector<bool>* mask);
    void UploadMaskColors(const Mesh* mesh, const std::vector<bool>& mask);
    void SetShowMask(bool show);

    // --- Target overlay (Touch Up) ---
    void SetShowTargetOverlay(bool show);

    // --- Input (stubs — Story 3.5.4 wires bodies) ---
    // All coordinates are viewport-local pixels (0..width, 0..height), Y-down
    // to match GLFW cursor coordinates. Callers must filter on
    // !io.WantCaptureMouse / !io.WantCaptureKeyboard before forwarding.
    void OnMouseButton(int button, int action, int mods, float x, float y);
    void OnMouseMove(float x, float y);
    void OnScroll(float xOffset, float yOffset);
    void OnKey(int key, int scancode, int action, int mods);

    // --- Callbacks (replace Qt signals) ---
    std::function<void(TransformMode, AxisConstraint)> OnTransformModeChanged;
    std::function<void()> OnTargetTransformChanged;
    std::function<void()> OnCameraChanged;
    std::function<void(PointSide, Vector3, int)> OnPointPlaced;
    std::function<void(int)> OnPointSelected;
    std::function<void()> OnPointDeleteRequested;
    std::function<void(float)> OnBrushRadiusChanged;
    std::function<void(Transform, Transform)> OnTransformApplied;
    std::function<void(BrushStroke)> OnSculptStrokeCompleted;
    // Fires once at the end of a Stage 3 mask paint stroke, with the mask
    // vector as it was *before* the first paint of this stroke. AppWindow
    // pushes a MaskStrokeUndoAction with that snapshot vs the post-stroke
    // mask so Ctrl+Z reverts a whole gesture, not one paint frame.
    std::function<void(std::vector<bool>)> OnMaskStrokeCompleted;

    // --- Stage 3 morph state (drives the Stage 3 toolbar status indicator) ---
    // Mirrored from AppWindow each frame:
    //   Processing → morphRunning_.load() == true
    //   Complete   → !running && project.morph.isProcessed
    //   Ready      → otherwise
    enum class MorphStatus { Ready, Processing, Complete };
    void SetMorphStatus(MorphStatus s) { morphStatus_ = s; }

private:
    // Overlay drawing (ImGui draw list)
    void DrawViewportLabelOverlay(float x, float y, float w, float h);
    void DrawOrientationGizmoOverlay(float x, float y, float w);
    void DrawPointLabelsOverlay(float x, float y, float w, float h);
    void DrawActiveBorderOverlay(float x, float y, float w, float h);

    // Sprint 3.8.5 — viewport chrome
    void DrawTopCenterToolbar(float x, float y, float w, float h);
    void DrawTopRightPills   (float x, float y, float w, float h);
    void DrawBottomLeftHud   (float x, float y, float w, float h);

    // Sprint 3.8.9 — bottom-left result stats panel (Stage 3 only, post-process)
    void DrawResultStatsOverlay(float x, float y, float w, float h);

    // Sprint 3.8.10 — bottom-left mesh stats panel (Stage 4 only)
    void DrawMeshStatsOverlay(float x, float y, float w, float h);

    bool IsGizmoVisible() const;
    Vector3 GetGizmoPosition() const;

    // Stage 2 point-placement helpers (ported from Qt HandlePointClick).
    PointSide GetViewportPointSide() const;
    bool TrySelectExistingPoint(float screenX, float screenY);
    void HandlePointClick(float screenX, float screenY);

    // Stage 3 mask painting helpers (ported from Qt HandleMask*).
    Mesh* GetMaskMesh();
    Transform GetMaskTransform() const;
    void HandleMaskPress(float screenX, float screenY);
    void HandleMaskMove(float screenX, float screenY);
    void HandleMaskRelease();
    void UpdateMaskBrushCursor(float screenX, float screenY);

    // Stage 4 sculpting helpers (ported from Qt HandleSculpt*).
    SculptBrush* GetActiveBrush();
    Mesh* GetSculptMesh();
    Transform GetSculptTransform() const;
    Vector3 ComputeWorldNormal(const Mesh& mesh, const Transform& transform,
                               int vertexIndex) const;
    void HandleSculptPress(float screenX, float screenY);
    void HandleSculptMove(float screenX, float screenY);
    void HandleSculptRelease();
    void UpdateSculptBrushCursor(float screenX, float screenY);

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Renderer> renderer_;
    Project* project_ = nullptr;

    // Transform tool state
    TransformMode transformMode_ = TransformMode::None;
    AxisConstraint axisConstraint_ = AxisConstraint::None;
    bool isTransforming_ = false;
    Transform transformBeforeState_;

    // Camera drag state (Story 3.5.4 — wired via GLFW callbacks routed through
    // ViewportHost). Coordinates are screen-coord pixels (local to the
    // viewport rect), matching GLFW cursor coordinates — not framebuffer px.
    bool isOrbiting_ = false;
    bool isPanning_ = false;
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;

    // Render filter + active/label
    RenderFilter renderFilter_ = RenderFilter::All;
    MorphStatus morphStatus_ = MorphStatus::Ready;  // Story 3.8.5 — Stage 3 toolbar status pill
    bool isActive_ = false;
    std::string viewportLabel_;
    // Story 5.4 — see SetRoleId comment.
    const char* roleId_ = "primary";

    // Point reference
    int selectedPointIndex_ = -1;

    // Sculpt brushes (kept so Setters can tune them; strokes re-wire in 3.5.4)
    BrushType activeBrushType_ = BrushType::Smooth;
    std::unique_ptr<SmoothBrush> smoothBrush_;
    std::unique_ptr<GrabBrush> grabBrush_;
    std::unique_ptr<PushPullBrush> pushPullBrush_;
    std::unique_ptr<InflateBrush> inflateBrush_;
    bool isSculpting_ = false;
    BrushStroke currentStroke_;
    Vector3 lastBrushWorldPos_;
    bool brushOnMesh_ = false;
    bool sculptSymmetryEnabled_ = false;
    Axis sculptSymmetryAxis_ = Axis::X;

    // Mask painting
    std::unique_ptr<MaskBrush> maskBrush_;
    bool isMaskPainting_ = false;
    bool isMaskStroking_ = false;
    // Snapshot of MorphData::vertexMask at the start of an active stroke,
    // used by OnMaskStrokeCompleted to push a single MaskStrokeUndoAction
    // covering the whole drag (not one per paint frame).
    std::vector<bool> maskBeforeStroke_;

    // Target overlay in Touch Up
    bool showTargetOverlay_ = false;

    // Gizmo hover/active
    GizmoAxis hoveredGizmoAxis_ = GizmoAxis::None;
    GizmoAxis activeGizmoAxis_ = GizmoAxis::None;

    // Last render rect (for overlay-only redraws & input hit testing)
    float lastX_ = 0.0f;
    float lastY_ = 0.0f;
    float lastW_ = 0.0f;
    float lastH_ = 0.0f;
};

} // namespace MetaVisage

#endif // METAVISAGE_VIEWPORT_H
