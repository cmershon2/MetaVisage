#include "ui/ViewportWidget.h"
#include "rendering/Renderer.h"
#include "core/Project.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace MetaVisage {

ViewportWidget::ViewportWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      camera_(std::make_unique<Camera>()),
      renderer_(nullptr),
      project_(nullptr),
      isOrbiting_(false),
      isPanning_(false),
      transformMode_(TransformMode::None),
      axisConstraint_(AxisConstraint::None),
      isTransforming_(false) {

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
    // Ensure viewport has focus for keyboard input
    setFocus();

    lastMousePos_ = event->pos();

    // Left click starts transform if in a transform mode
    if (event->button() == Qt::LeftButton && transformMode_ != TransformMode::None) {
        if (project_ && project_->GetTargetMesh().isLoaded &&
            project_->GetCurrentStage() == WorkflowStage::Alignment) {
            isTransforming_ = true;
            transformStartPos_ = event->pos();
        }
    }

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

    if (isTransforming_) {
        ApplyTransform(delta);
        update();
    } else if (isOrbiting_) {
        camera_->Orbit(delta.x(), delta.y());
        update();
    } else if (isPanning_) {
        camera_->Pan(delta.x(), delta.y());
        update();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (isTransforming_) {
            isTransforming_ = false;
            // Keep the transform mode active so user can continue transforming
        }
    }

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
    // Check if we're in the Alignment stage for transform tools
    bool inAlignmentStage = project_ && project_->GetCurrentStage() == WorkflowStage::Alignment;
    bool hasTargetMesh = project_ && project_->GetTargetMesh().isLoaded;

    qDebug() << "Key pressed:" << event->key()
             << "inAlignmentStage:" << inAlignmentStage
             << "hasTargetMesh:" << hasTargetMesh;

    switch (event->key()) {
        // Transform tool keys (only in Alignment stage with target mesh loaded)
        case Qt::Key_G:
            if (inAlignmentStage && hasTargetMesh) {
                qDebug() << "Activating Move mode";
                SetTransformMode(TransformMode::Move);
            } else {
                qDebug() << "Cannot activate Move - stage or mesh check failed";
            }
            break;

        case Qt::Key_R:
            if (inAlignmentStage && hasTargetMesh) {
                qDebug() << "Activating Rotate mode";
                SetTransformMode(TransformMode::Rotate);
            } else {
                qDebug() << "Cannot activate Rotate - stage or mesh check failed";
            }
            break;

        case Qt::Key_S:
            if (inAlignmentStage && hasTargetMesh) {
                qDebug() << "Activating Scale mode";
                SetTransformMode(TransformMode::Scale);
            } else {
                qDebug() << "Cannot activate Scale - stage or mesh check failed";
            }
            break;

        // Axis constraint keys (only when a transform mode is active)
        case Qt::Key_X:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::X) ? AxisConstraint::None : AxisConstraint::X;
                emit TransformModeChanged(transformMode_, axisConstraint_);
            }
            break;

        case Qt::Key_Y:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::Y) ? AxisConstraint::None : AxisConstraint::Y;
                emit TransformModeChanged(transformMode_, axisConstraint_);
            }
            break;

        case Qt::Key_Z:
            if (transformMode_ != TransformMode::None) {
                axisConstraint_ = (axisConstraint_ == AxisConstraint::Z) ? AxisConstraint::None : AxisConstraint::Z;
                emit TransformModeChanged(transformMode_, axisConstraint_);
            }
            break;

        // Escape cancels current transform
        case Qt::Key_Escape:
            if (transformMode_ != TransformMode::None) {
                CancelTransform();
            }
            break;

        // Camera shortcuts
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

void ViewportWidget::SetTransformMode(TransformMode mode) {
    // If clicking the same mode, toggle it off
    if (transformMode_ == mode) {
        transformMode_ = TransformMode::None;
        axisConstraint_ = AxisConstraint::None;
    } else {
        transformMode_ = mode;
        axisConstraint_ = AxisConstraint::None;  // Reset axis constraint when changing mode
    }
    isTransforming_ = false;
    emit TransformModeChanged(transformMode_, axisConstraint_);
}

void ViewportWidget::CancelTransform() {
    transformMode_ = TransformMode::None;
    axisConstraint_ = AxisConstraint::None;
    isTransforming_ = false;
    emit TransformModeChanged(transformMode_, axisConstraint_);
}

void ViewportWidget::ApplyTransform(const QPoint& delta) {
    if (!project_ || !project_->GetTargetMesh().isLoaded) {
        return;
    }

    Transform& transform = project_->GetTargetMesh().transform;

    // Sensitivity factors for each transform type
    const float moveSensitivity = 0.01f;
    const float rotateSensitivity = 0.5f;  // degrees per pixel
    const float scaleSensitivity = 0.005f;

    switch (transformMode_) {
        case TransformMode::Move: {
            Vector3 translation(0.0f, 0.0f, 0.0f);

            // Calculate translation based on mouse movement
            // Horizontal mouse = X axis, Vertical mouse = Y axis (inverted for natural feel)
            float dx = delta.x() * moveSensitivity;
            float dy = -delta.y() * moveSensitivity;  // Invert Y for natural up/down

            switch (axisConstraint_) {
                case AxisConstraint::X:
                    translation.x = dx;
                    break;
                case AxisConstraint::Y:
                    translation.y = dy;
                    break;
                case AxisConstraint::Z:
                    // Use horizontal movement for Z when constrained
                    translation.z = dx;
                    break;
                case AxisConstraint::None:
                default:
                    translation.x = dx;
                    translation.y = dy;
                    break;
            }

            transform.Translate(translation);
            break;
        }

        case TransformMode::Rotate: {
            // Use horizontal movement for rotation
            float angle = delta.x() * rotateSensitivity;

            Vector3 axis(0.0f, 1.0f, 0.0f);  // Default: rotate around Y axis

            switch (axisConstraint_) {
                case AxisConstraint::X:
                    axis = Vector3(1.0f, 0.0f, 0.0f);
                    break;
                case AxisConstraint::Y:
                    axis = Vector3(0.0f, 1.0f, 0.0f);
                    break;
                case AxisConstraint::Z:
                    axis = Vector3(0.0f, 0.0f, 1.0f);
                    break;
                case AxisConstraint::None:
                default:
                    // Without constraint, use Y axis for horizontal movement
                    // and X axis for vertical movement
                    if (std::abs(delta.x()) >= std::abs(delta.y())) {
                        axis = Vector3(0.0f, 1.0f, 0.0f);
                        angle = delta.x() * rotateSensitivity;
                    } else {
                        axis = Vector3(1.0f, 0.0f, 0.0f);
                        angle = delta.y() * rotateSensitivity;
                    }
                    break;
            }

            Quaternion rotation = Quaternion::FromAxisAngle(axis, angle);
            transform.Rotate(rotation);
            break;
        }

        case TransformMode::Scale: {
            // Use horizontal movement for scaling
            float scaleFactor = 1.0f + delta.x() * scaleSensitivity;

            // Clamp scale factor to prevent negative or zero scales
            scaleFactor = std::max(0.1f, std::min(2.0f, scaleFactor));

            Vector3 scaleVec(1.0f, 1.0f, 1.0f);

            switch (axisConstraint_) {
                case AxisConstraint::X:
                    scaleVec.x = scaleFactor;
                    break;
                case AxisConstraint::Y:
                    scaleVec.y = scaleFactor;
                    break;
                case AxisConstraint::Z:
                    scaleVec.z = scaleFactor;
                    break;
                case AxisConstraint::None:
                default:
                    // Uniform scaling
                    scaleVec.x = scaleFactor;
                    scaleVec.y = scaleFactor;
                    scaleVec.z = scaleFactor;
                    break;
            }

            transform.Scale(scaleVec);
            break;
        }

        case TransformMode::None:
        default:
            break;
    }

    emit TargetTransformChanged();
}

} // namespace MetaVisage
