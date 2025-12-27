#include "ui/ViewportWidget.h"
#include "rendering/Renderer.h"
#include <QDebug>

namespace MetaVisage {

ViewportWidget::ViewportWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      camera_(std::make_unique<Camera>()),
      renderer_(nullptr),
      project_(nullptr),
      isOrbiting_(false),
      isPanning_(false) {

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

ViewportWidget::~ViewportWidget() {
}

void ViewportWidget::initializeGL() {
    initializeOpenGLFunctions();

    qDebug() << "OpenGL Version:" << (const char*)glGetString(GL_VERSION);
    qDebug() << "GLSL Version:" << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "Renderer:" << (const char*)glGetString(GL_RENDERER);

    // Set clear color to dark gray
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Disable backface culling so we can see the mesh from any angle
    glDisable(GL_CULL_FACE);

    // Create renderer
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->Initialize()) {
        qWarning() << "Failed to initialize renderer!";
    }
}

void ViewportWidget::paintGL() {
    if (renderer_) {
        renderer_->Render(*camera_, width(), height(), project_);
    }
}

void ViewportWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void ViewportWidget::mousePressEvent(QMouseEvent *event) {
    lastMousePos_ = event->pos();

    if (event->button() == Qt::MiddleButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            isPanning_ = true;
        } else {
            isOrbiting_ = true;
        }
    }
}

void ViewportWidget::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta = event->pos() - lastMousePos_;
    lastMousePos_ = event->pos();

    if (isOrbiting_) {
        camera_->Orbit(delta.x(), delta.y());
        update();
    } else if (isPanning_) {
        camera_->Pan(delta.x(), delta.y());
        update();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        isOrbiting_ = false;
        isPanning_ = false;
    }
}

void ViewportWidget::wheelEvent(QWheelEvent *event) {
    float delta = event->angleDelta().y() / 120.0f;
    camera_->Zoom(delta);
    update();
}

void ViewportWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Home:
            camera_->Reset();
            update();
            break;

        case Qt::Key_1:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicFront);
                camera_->SetPosition(Vector3(0.0f, 0.0f, 5.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
            }
            break;

        case Qt::Key_3:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicRight);
                camera_->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
            }
            break;

        case Qt::Key_7:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicTop);
                camera_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
            }
            break;

        case Qt::Key_5:
            if (event->modifiers() == Qt::NoModifier) {
                if (camera_->GetProjectionMode() == ProjectionMode::Perspective) {
                    camera_->SetProjectionMode(ProjectionMode::OrthographicFront);
                } else {
                    camera_->SetProjectionMode(ProjectionMode::Perspective);
                }
                update();
            }
            break;

        case Qt::Key_F:
            // Focus on selection - for now just reset camera
            camera_->Reset();
            update();
            break;

        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

} // namespace MetaVisage
