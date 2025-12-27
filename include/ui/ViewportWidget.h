#ifndef VIEWPORTWIDGET_H
#define VIEWPORTWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <memory>
#include "core/Camera.h"

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
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Renderer> renderer_;
    Project* project_;

    // Mouse state
    QPoint lastMousePos_;
    bool isOrbiting_;
    bool isPanning_;
};

} // namespace MetaVisage

#endif // VIEWPORTWIDGET_H
