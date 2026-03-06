#ifndef VIEWPORTWIDGET_H
#define VIEWPORTWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <memory>
#include "core/Camera.h"
#include "core/Types.h"
#include "sculpting/BrushStroke.h"

namespace MetaVisage {

class Renderer;
class Project;
class Transform;
class SculptBrush;
class SmoothBrush;
class GrabBrush;

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core {
    Q_OBJECT

public:
    explicit ViewportWidget(QWidget *parent = nullptr);
    ~ViewportWidget();

    void SetProject(Project* project) { project_ = project; }
    Camera* GetCamera() { return camera_.get(); }

    // Transform mode accessors
    TransformMode GetTransformMode() const { return transformMode_; }
    AxisConstraint GetAxisConstraint() const { return axisConstraint_; }
    void SetTransformMode(TransformMode mode);
    void CancelTransform();

    // Render filter for dual viewport mode
    void SetRenderFilter(RenderFilter filter);
    RenderFilter GetRenderFilter() const { return renderFilter_; }

    // Camera synchronization - copies state without emitting CameraChanged
    void SyncCameraFrom(const Camera& other);

    // Active viewport indicator
    void SetActive(bool active);
    bool IsActive() const { return isActive_; }

    // Viewport label (shown in corner, e.g. "Target Mesh" or "Morph Mesh")
    void SetViewportLabel(const QString& label) { viewportLabel_ = label; }

    // Point selection
    void SetSelectedPointIndex(int index);
    int GetSelectedPointIndex() const { return selectedPointIndex_; }

    // Point rendering settings
    void SetPointSize(float size);

    // Morph stage
    void SetMorphPreviewMode(MorphPreviewMode mode);
    void InvalidateMesh(const Mesh* mesh);
    void UploadHeatMapColors(const Mesh* mesh, const std::vector<float>& displacements, float maxDisplacement);

    // Sculpting (Touch Up stage)
    void SetBrushType(BrushType type);
    void SetBrushRadius(float radius);
    void SetBrushStrength(float strength);
    void SetBrushFalloff(FalloffType falloff);
    BrushType GetBrushType() const { return activeBrushType_; }
    float GetBrushRadius() const;
    float GetBrushStrength() const;

signals:
    // Signal emitted when transform mode changes
    void TransformModeChanged(TransformMode mode, AxisConstraint axis);
    // Signal emitted when target mesh transform is modified
    void TargetTransformChanged();
    // Signal emitted when camera state changes (for synchronization)
    void CameraChanged();
    // Signal emitted when a point is placed on the mesh
    void PointPlaced(PointSide side, Vector3 position, int vertexIndex);
    // Signal emitted when a point is selected
    void PointSelected(int correspondenceIndex);
    // Signal emitted when delete key is pressed to remove selected point
    void PointDeleteRequested();
    // Signal emitted when brush radius changes (from [ ] keys)
    void BrushRadiusChanged(float radius);

protected:
    // OpenGL functions
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    // Input handling
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // Apply transform based on mouse delta
    void ApplyTransform(const QPoint& delta);

    // Point reference helpers
    void HandlePointClick(QMouseEvent *event);
    bool TrySelectExistingPoint(float screenX, float screenY);
    PointSide GetViewportPointSide() const;
    void DrawPointLabels(QPainter& painter);

    // Sculpting helpers
    void HandleSculptPress(QMouseEvent *event);
    void HandleSculptMove(QMouseEvent *event, const QPoint& delta);
    void HandleSculptRelease();
    void UpdateBrushCursor(float screenX, float screenY);
    SculptBrush* GetActiveBrush();
    Mesh* GetSculptMesh();
    Transform GetSculptTransform() const;
    Vector3 ComputeWorldNormal(const Mesh& mesh, const Transform& transform, int vertexIndex);

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Renderer> renderer_;
    Project* project_;

    // Mouse state
    QPoint lastMousePos_;
    QPoint transformStartPos_;  // Mouse position when transform started
    bool isOrbiting_;
    bool isPanning_;

    // Transform tool state
    TransformMode transformMode_;
    AxisConstraint axisConstraint_;
    bool isTransforming_;  // True when actively dragging to transform

    // Render filter
    RenderFilter renderFilter_;

    // Active viewport state
    bool isActive_;
    QString viewportLabel_;

    // Point reference state
    int selectedPointIndex_;

    // Sculpting state
    BrushType activeBrushType_;
    std::unique_ptr<SmoothBrush> smoothBrush_;
    std::unique_ptr<GrabBrush> grabBrush_;
    bool isSculpting_;
    BrushStroke currentStroke_;
    Vector3 lastBrushWorldPos_;
    bool brushOnMesh_;
};

} // namespace MetaVisage

#endif // VIEWPORTWIDGET_H
