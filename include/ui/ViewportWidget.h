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

namespace MetaVisage {

class Renderer;
class Project;

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

signals:
    // Signal emitted when transform mode changes
    void TransformModeChanged(TransformMode mode, AxisConstraint axis);
    // Signal emitted when target mesh transform is modified
    void TargetTransformChanged();
    // Signal emitted when camera state changes (for synchronization)
    void CameraChanged();

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
};

} // namespace MetaVisage

#endif // VIEWPORTWIDGET_H
