#include "ui/ViewportWidget.h"
#include "rendering/Renderer.h"
#include "core/Project.h"
#include "utils/RayCaster.h"
#include "sculpting/SculptBrush.h"
#include "sculpting/SmoothBrush.h"
#include "sculpting/GrabBrush.h"
#include "sculpting/PushPullBrush.h"
#include "sculpting/InflateBrush.h"
#include <QDebug>
#include <QPainter>
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
      isTransforming_(false),
      renderFilter_(RenderFilter::All),
      isActive_(false),
      viewportLabel_(""),
      selectedPointIndex_(-1),
      activeBrushType_(BrushType::Smooth),
      smoothBrush_(std::make_unique<SmoothBrush>()),
      grabBrush_(std::make_unique<GrabBrush>()),
      pushPullBrush_(std::make_unique<PushPullBrush>()),
      inflateBrush_(std::make_unique<InflateBrush>()),
      isSculpting_(false),
      brushOnMesh_(false),
      sculptSymmetryEnabled_(false),
      sculptSymmetryAxis_(Axis::X),
      showTargetOverlay_(false) {

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // Default border style (inactive)
    setStyleSheet("border: 2px solid #555;");
}

ViewportWidget::~ViewportWidget() {
}

void ViewportWidget::SetRenderFilter(RenderFilter filter) {
    renderFilter_ = filter;
    if (renderer_) {
        renderer_->SetRenderFilter(filter);
    }
    update();
}

void ViewportWidget::SyncCameraFrom(const Camera& other) {
    camera_->CopyStateFrom(other);
    update();
    // Note: intentionally does NOT emit CameraChanged to prevent sync loops
}

void ViewportWidget::SetActive(bool active) {
    isActive_ = active;
    if (active) {
        setStyleSheet("border: 2px solid #3498DB;");
    } else {
        setStyleSheet("border: 2px solid #555;");
    }
}

void ViewportWidget::SetSelectedPointIndex(int index) {
    selectedPointIndex_ = index;
    if (renderer_) {
        renderer_->SetSelectedPointIndex(index);
    }
    update();
}

void ViewportWidget::SetPointSize(float size) {
    if (renderer_) {
        renderer_->SetPointSize(size);
    }
    update();
}

void ViewportWidget::SetMorphPreviewMode(MorphPreviewMode mode) {
    if (renderer_) {
        renderer_->SetMorphPreviewMode(mode);
    }
    update();
}

void ViewportWidget::InvalidateMesh(const Mesh* mesh) {
    if (renderer_) {
        renderer_->InvalidateMesh(mesh);
    }
}

void ViewportWidget::UploadHeatMapColors(const Mesh* mesh, const std::vector<float>& displacements, float maxDisplacement) {
    makeCurrent();
    if (renderer_) {
        renderer_->UploadHeatMapColors(mesh, displacements, maxDisplacement);
    }
    doneCurrent();
}

// --- Sculpting accessors ---

void ViewportWidget::SetBrushType(BrushType type) {
    activeBrushType_ = type;
}

void ViewportWidget::SetBrushRadius(float radius) {
    SculptBrush* brush = GetActiveBrush();
    if (brush) {
        brush->SetRadius(radius);
    }
}

void ViewportWidget::SetBrushStrength(float strength) {
    SculptBrush* brush = GetActiveBrush();
    if (brush) {
        brush->SetStrength(strength);
    }
}

void ViewportWidget::SetBrushFalloff(FalloffType falloff) {
    if (smoothBrush_) smoothBrush_->SetFalloff(falloff);
    if (grabBrush_) grabBrush_->SetFalloff(falloff);
    if (pushPullBrush_) pushPullBrush_->SetFalloff(falloff);
    if (inflateBrush_) inflateBrush_->SetFalloff(falloff);
}

void ViewportWidget::SetSculptSymmetry(bool enabled, Axis axis) {
    sculptSymmetryEnabled_ = enabled;
    sculptSymmetryAxis_ = axis;
}

void ViewportWidget::SetShowTargetOverlay(bool show) {
    showTargetOverlay_ = show;
    if (renderer_) {
        renderer_->SetShowTargetOverlay(show);
    }
    update();
}

float ViewportWidget::GetBrushRadius() const {
    if (activeBrushType_ == BrushType::Smooth && smoothBrush_) {
        return smoothBrush_->GetRadius();
    } else if (activeBrushType_ == BrushType::Grab && grabBrush_) {
        return grabBrush_->GetRadius();
    } else if (activeBrushType_ == BrushType::PushPull && pushPullBrush_) {
        return pushPullBrush_->GetRadius();
    } else if (activeBrushType_ == BrushType::Inflate && inflateBrush_) {
        return inflateBrush_->GetRadius();
    }
    return 0.5f;
}

float ViewportWidget::GetBrushStrength() const {
    if (activeBrushType_ == BrushType::Smooth && smoothBrush_) {
        return smoothBrush_->GetStrength();
    } else if (activeBrushType_ == BrushType::Grab && grabBrush_) {
        return grabBrush_->GetStrength();
    } else if (activeBrushType_ == BrushType::PushPull && pushPullBrush_) {
        return pushPullBrush_->GetStrength();
    } else if (activeBrushType_ == BrushType::Inflate && inflateBrush_) {
        return inflateBrush_->GetStrength();
    }
    return 0.5f;
}

SculptBrush* ViewportWidget::GetActiveBrush() {
    switch (activeBrushType_) {
        case BrushType::Smooth:   return smoothBrush_.get();
        case BrushType::Grab:     return grabBrush_.get();
        case BrushType::PushPull: return pushPullBrush_.get();
        case BrushType::Inflate:  return inflateBrush_.get();
        default: return nullptr;
    }
}

Mesh* ViewportWidget::GetSculptMesh() {
    if (!project_) return nullptr;

    // In Touch Up stage, we sculpt the deformed morph mesh
    auto& morphData = project_->GetMorphData();
    if (morphData.deformedMorphMesh) {
        return morphData.deformedMorphMesh.get();
    }

    return nullptr;
}

Transform ViewportWidget::GetSculptTransform() const {
    // The deformed mesh is rendered using the morph mesh's transform (which has the
    // -90° X rotation), so raycasting and brush operations must use the same transform.
    if (project_ && project_->GetMorphMesh().isLoaded) {
        return project_->GetMorphMesh().transform;
    }
    return Transform();
}

Vector3 ViewportWidget::ComputeWorldNormal(const Mesh& mesh, const Transform& transform,
                                            int vertexIndex) {
    const auto& normals = mesh.GetNormals();
    if (vertexIndex < 0 || vertexIndex >= static_cast<int>(normals.size())) {
        return Vector3(0.0f, 1.0f, 0.0f); // Default up
    }

    Vector3 localNormal = normals[vertexIndex];

    // Transform normal to world space using the normal matrix (inverse transpose of model)
    Matrix4x4 modelMatrix = transform.GetMatrix();
    Matrix4x4 normalMatrix = modelMatrix.Inverse().Transpose();

    Vector3 worldNormal;
    worldNormal.x = normalMatrix.m[0] * localNormal.x + normalMatrix.m[4] * localNormal.y + normalMatrix.m[8] * localNormal.z;
    worldNormal.y = normalMatrix.m[1] * localNormal.x + normalMatrix.m[5] * localNormal.y + normalMatrix.m[9] * localNormal.z;
    worldNormal.z = normalMatrix.m[2] * localNormal.x + normalMatrix.m[6] * localNormal.y + normalMatrix.m[10] * localNormal.z;

    return worldNormal.Normalized();
}

// Helper: compute inner radius ratio based on falloff type (where weight drops to 50%)
static float GetFalloffInnerRatio(FalloffType falloff) {
    switch (falloff) {
        case FalloffType::Linear: return 0.5f;    // 1-t = 0.5 at t=0.5
        case FalloffType::Smooth: return 0.5f;    // hermite midpoint
        case FalloffType::Sharp:  return 0.794f;  // 1-t^3 = 0.5 at t≈0.794
        default: return 0.5f;
    }
}

// --- Sculpting interaction ---

void ViewportWidget::HandleSculptPress(QMouseEvent *event) {
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh || !project_) return;

    float screenX = static_cast<float>(event->pos().x());
    float screenY = static_cast<float>(event->pos().y());

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, width(), height(), *camera_);

    // Use the morph mesh transform (matches how renderer draws the deformed mesh)
    Transform transform = GetSculptTransform();
    RaycastHit hit = RayCaster::RayIntersectMesh(ray, *mesh, transform);

    if (hit.hit) {
        isSculpting_ = true;
        lastBrushWorldPos_ = hit.position;

        // Start recording vertices for undo
        currentStroke_ = BrushStroke();

        // Record original positions of vertices that will be affected
        auto affected = brush->GetAffectedVertices(*mesh, transform, hit.position, brush->GetRadius());
        const auto& vertices = mesh->GetVertices();
        for (const auto& av : affected) {
            if (av.index >= 0 && av.index < static_cast<int>(vertices.size())) {
                currentStroke_.RecordVertex(av.index, vertices[av.index]);
            }
        }

        // Begin brush stroke
        brush->BeginStroke(*mesh, transform, hit.position);

        // Apply immediately for smooth brush (grab waits for movement)
        if (activeBrushType_ == BrushType::Smooth) {
            Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
            if (brush->Apply(*mesh, transform, hit.position, worldNormal, Vector3(), 0.016f)) {
                mesh->CalculateNormals();
                makeCurrent();
                if (renderer_) {
                    renderer_->UpdateMeshVertices(mesh);
                }
                doneCurrent();
                update();
            }

            // Apply symmetric stroke if symmetry enabled
            if (sculptSymmetryEnabled_) {
                Vector3 mirroredPos = hit.position;
                switch (sculptSymmetryAxis_) {
                    case Axis::X: mirroredPos.x = -mirroredPos.x; break;
                    case Axis::Y: mirroredPos.y = -mirroredPos.y; break;
                    case Axis::Z: mirroredPos.z = -mirroredPos.z; break;
                }
                Vector3 mirroredNormal = worldNormal;
                switch (sculptSymmetryAxis_) {
                    case Axis::X: mirroredNormal.x = -mirroredNormal.x; break;
                    case Axis::Y: mirroredNormal.y = -mirroredNormal.y; break;
                    case Axis::Z: mirroredNormal.z = -mirroredNormal.z; break;
                }
                // Record mirrored affected vertices
                auto mirroredAffected = brush->GetAffectedVertices(*mesh, transform, mirroredPos, brush->GetRadius());
                for (const auto& mav : mirroredAffected) {
                    if (mav.index >= 0 && mav.index < static_cast<int>(vertices.size())) {
                        currentStroke_.RecordVertex(mav.index, vertices[mav.index]);
                    }
                }
                brush->Apply(*mesh, transform, mirroredPos, mirroredNormal, Vector3(), 0.016f);
                mesh->CalculateNormals();
                makeCurrent();
                if (renderer_) {
                    renderer_->UpdateMeshVertices(mesh);
                }
                doneCurrent();
                update();
            }
        }
    }
}

void ViewportWidget::HandleSculptMove(QMouseEvent *event, const QPoint& delta) {
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh || !project_) return;

    float screenX = static_cast<float>(event->pos().x());
    float screenY = static_cast<float>(event->pos().y());

    if (isSculpting_) {
        // Active sculpting stroke
        Ray ray = RayCaster::ScreenToRay(screenX, screenY, width(), height(), *camera_);
        Transform transform = GetSculptTransform();

        Vector3 brushCenter = lastBrushWorldPos_;
        Vector3 worldNormal(0.0f, 1.0f, 0.0f);
        Vector3 mouseDelta;

        if (activeBrushType_ == BrushType::Grab) {
            // Grab brush: project mouse onto a view-aligned plane through the last
            // brush position. This allows pulling vertices away from the mesh surface
            // because movement is no longer constrained to the mesh.
            Vector3 viewDir = (camera_->GetTarget() - camera_->GetPosition()).Normalized();
            Vector3 planeNormal = viewDir * -1.0f; // plane faces toward camera

            float denom = planeNormal.Dot(ray.direction);
            if (std::abs(denom) > 1e-6f) {
                float t = planeNormal.Dot(lastBrushWorldPos_ - ray.origin) / denom;
                if (t > 0.0f) {
                    brushCenter = ray.PointAt(t);
                }
            }
            mouseDelta = brushCenter - lastBrushWorldPos_;
            // Use camera-facing normal for cursor orientation
            worldNormal = planeNormal;
        } else {
            // Smooth brush: raycast against mesh surface
            RaycastHit hit = RayCaster::RayIntersectMesh(ray, *mesh, transform);

            if (hit.hit) {
                brushCenter = hit.position;
                worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
            }
            mouseDelta = brushCenter - lastBrushWorldPos_;

            // Record any new vertices that may be affected
            auto affected = brush->GetAffectedVertices(*mesh, transform, brushCenter, brush->GetRadius());
            const auto& vertices = mesh->GetVertices();
            for (const auto& av : affected) {
                if (av.index >= 0 && av.index < static_cast<int>(vertices.size())) {
                    currentStroke_.RecordVertex(av.index, vertices[av.index]);
                }
            }
        }

        // Apply brush
        if (brush->Apply(*mesh, transform, brushCenter, worldNormal, mouseDelta, 0.016f)) {
            mesh->CalculateNormals();
            makeCurrent();
            if (renderer_) {
                renderer_->UpdateMeshVertices(mesh);
            }
            doneCurrent();
            update();
        }

        // Apply symmetric stroke if symmetry enabled
        if (sculptSymmetryEnabled_) {
            Vector3 mirroredCenter = brushCenter;
            switch (sculptSymmetryAxis_) {
                case Axis::X: mirroredCenter.x = -mirroredCenter.x; break;
                case Axis::Y: mirroredCenter.y = -mirroredCenter.y; break;
                case Axis::Z: mirroredCenter.z = -mirroredCenter.z; break;
            }
            Vector3 mirroredNormal = worldNormal;
            switch (sculptSymmetryAxis_) {
                case Axis::X: mirroredNormal.x = -mirroredNormal.x; break;
                case Axis::Y: mirroredNormal.y = -mirroredNormal.y; break;
                case Axis::Z: mirroredNormal.z = -mirroredNormal.z; break;
            }
            Vector3 mirroredDelta = mouseDelta;
            switch (sculptSymmetryAxis_) {
                case Axis::X: mirroredDelta.x = -mirroredDelta.x; break;
                case Axis::Y: mirroredDelta.y = -mirroredDelta.y; break;
                case Axis::Z: mirroredDelta.z = -mirroredDelta.z; break;
            }
            // Record mirrored affected vertices for undo
            auto mirroredAffected = brush->GetAffectedVertices(*mesh, transform, mirroredCenter, brush->GetRadius());
            const auto& verts = mesh->GetVertices();
            for (const auto& mav : mirroredAffected) {
                if (mav.index >= 0 && mav.index < static_cast<int>(verts.size())) {
                    currentStroke_.RecordVertex(mav.index, verts[mav.index]);
                }
            }
            if (brush->Apply(*mesh, transform, mirroredCenter, mirroredNormal, mirroredDelta, 0.016f)) {
                mesh->CalculateNormals();
                makeCurrent();
                if (renderer_) {
                    renderer_->UpdateMeshVertices(mesh);
                }
                doneCurrent();
                update();
            }
        }

        lastBrushWorldPos_ = brushCenter;

        // Update brush cursor
        if (renderer_) {
            float innerRadius = brush->GetRadius() * GetFalloffInnerRatio(brush->GetFalloff());
            renderer_->SetBrushCursor(brushCenter, worldNormal, brush->GetRadius(), innerRadius, true);
        }
    } else {
        // Not sculpting - just update brush cursor position on hover
        UpdateBrushCursor(screenX, screenY);
    }
}

void ViewportWidget::HandleSculptRelease() {
    if (!isSculpting_) return;

    SculptBrush* brush = GetActiveBrush();
    if (brush) {
        brush->EndStroke();
    }

    isSculpting_ = false;
    // currentStroke_ contains the undo data (can be used by undo system later)
}

void ViewportWidget::UpdateBrushCursor(float screenX, float screenY) {
    SculptBrush* brush = GetActiveBrush();
    Mesh* mesh = GetSculptMesh();
    if (!brush || !mesh || !renderer_) {
        if (renderer_) renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        return;
    }

    Ray ray = RayCaster::ScreenToRay(screenX, screenY, width(), height(), *camera_);
    Transform transform = GetSculptTransform();

    RaycastHit hit = RayCaster::RayIntersectMesh(ray, *mesh, transform);

    if (hit.hit) {
        Vector3 worldNormal = ComputeWorldNormal(*mesh, transform, hit.vertexIndex);
        float innerRadius = brush->GetRadius() * GetFalloffInnerRatio(brush->GetFalloff());
        renderer_->SetBrushCursor(hit.position, worldNormal, brush->GetRadius(), innerRadius, true);
        brushOnMesh_ = true;
        lastBrushWorldPos_ = hit.position;
    } else {
        renderer_->SetBrushCursor(Vector3(), Vector3(), 0.0f, 0.0f, false);
        brushOnMesh_ = false;
    }

    update();
}

// --- OpenGL ---

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
    renderer_->SetRenderFilter(renderFilter_);
}

void ViewportWidget::paintGL() {
    if (renderer_) {
        renderer_->Render(*camera_, width(), height(), project_);
    }

    // Draw overlays with QPainter
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw viewport label overlay if set
    if (!viewportLabel_.isEmpty()) {
        QFont font = painter.font();
        font.setPointSize(10);
        font.setBold(true);
        painter.setFont(font);

        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(viewportLabel_);
        textRect.adjust(-8, -4, 8, 4);
        textRect.moveTopLeft(QPoint(8, 8));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRoundedRect(textRect, 4, 4);

        painter.setPen(QColor(255, 255, 255, 200));
        painter.drawText(textRect, Qt::AlignCenter, viewportLabel_);
    }

    // Draw point labels in PointReference stage
    if (project_ && project_->GetCurrentStage() == WorkflowStage::PointReference) {
        DrawPointLabels(painter);
    }

    painter.end();
}

void ViewportWidget::DrawPointLabels(QPainter& painter) {
    if (!project_) return;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return;

    QFont labelFont = painter.font();
    labelFont.setPointSize(8);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    for (size_t i = 0; i < correspondences.size(); ++i) {
        const auto& corr = correspondences[i];
        int corrIdx = static_cast<int>(i);
        bool isSelected = (corrIdx == selectedPointIndex_);
        QString label = QString::number(corr.pointID);

        Vector3 worldPos;
        bool shouldDraw = false;

        // Determine which point to draw based on render filter
        if (renderFilter_ != RenderFilter::MorphOnly && corr.targetMeshVertexIndex >= 0) {
            worldPos = corr.targetMeshPosition;
            shouldDraw = true;
        } else if (renderFilter_ != RenderFilter::TargetOnly && corr.morphMeshVertexIndex >= 0) {
            worldPos = corr.morphMeshPosition;
            shouldDraw = true;
        }

        if (!shouldDraw) continue;

        // Project to screen
        Vector3 screenPos = RayCaster::WorldToScreen(worldPos, width(), height(), *camera_);

        // Skip if behind camera
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) continue;

        // Draw label above the point
        int sx = static_cast<int>(screenPos.x);
        int sy = static_cast<int>(screenPos.y) - 12;

        QFontMetrics fm(labelFont);
        QRect textRect = fm.boundingRect(label);
        textRect.adjust(-3, -2, 3, 2);
        textRect.moveCenter(QPoint(sx, sy));

        // Background
        QColor bgColor = isSelected ? QColor(230, 126, 34, 200) : QColor(0, 0, 0, 160);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawRoundedRect(textRect, 3, 3);

        // Text
        painter.setPen(QColor(255, 255, 255, 230));
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
}

void ViewportWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

// --- Input handling ---

void ViewportWidget::mousePressEvent(QMouseEvent *event) {
    // Ensure viewport has focus for keyboard input
    setFocus();

    lastMousePos_ = event->pos();

    bool inPointReferenceStage = project_ && project_->GetCurrentStage() == WorkflowStage::PointReference;
    bool inTouchUpStage = project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp;

    // Left click behavior depends on stage
    if (event->button() == Qt::LeftButton) {
        if (inTouchUpStage && activeBrushType_ != BrushType::None) {
            // Touch Up stage: start sculpting
            HandleSculptPress(event);
        } else if (inPointReferenceStage) {
            // In Point Reference stage: place or select points
            HandlePointClick(event);
        } else if (transformMode_ != TransformMode::None) {
            // In Alignment stage with transform mode: start transform
            if (project_ && project_->GetTargetMesh().isLoaded &&
                project_->GetCurrentStage() == WorkflowStage::Alignment) {
                isTransforming_ = true;
                transformStartPos_ = event->pos();
            }
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

void ViewportWidget::HandlePointClick(QMouseEvent *event) {
    if (!project_) return;

    float screenX = static_cast<float>(event->pos().x());
    float screenY = static_cast<float>(event->pos().y());

    // First, try to select an existing point near the click
    if (TrySelectExistingPoint(screenX, screenY)) {
        return;
    }

    // If no existing point near click, try to place a new point
    Ray ray = RayCaster::ScreenToRay(screenX, screenY, width(), height(), *camera_);

    PointSide side = GetViewportPointSide();
    RaycastHit hit;

    if (side == PointSide::Target && project_->GetTargetMesh().isLoaded && project_->GetTargetMesh().mesh) {
        hit = RayCaster::RayIntersectMesh(ray, *project_->GetTargetMesh().mesh,
                                           project_->GetTargetMesh().transform);
    } else if (side == PointSide::Morph && project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
        hit = RayCaster::RayIntersectMesh(ray, *project_->GetMorphMesh().mesh,
                                           project_->GetMorphMesh().transform);
    }

    if (hit.hit) {
        emit PointPlaced(side, hit.position, hit.vertexIndex);
    } else {
        // Click on empty space: deselect
        selectedPointIndex_ = -1;
        if (renderer_) {
            renderer_->SetSelectedPointIndex(-1);
        }
        emit PointSelected(-1);
        update();
    }
}

bool ViewportWidget::TrySelectExistingPoint(float screenX, float screenY) {
    if (!project_) return false;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return false;

    const float selectRadius = 15.0f; // pixels
    float closestDist = selectRadius;
    int closestIdx = -1;

    PointSide side = GetViewportPointSide();

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

        Vector3 screenPos = RayCaster::WorldToScreen(worldPos, width(), height(), *camera_);

        // Skip if behind camera
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
        selectedPointIndex_ = closestIdx;
        if (renderer_) {
            renderer_->SetSelectedPointIndex(closestIdx);
        }
        emit PointSelected(closestIdx);
        update();
        return true;
    }

    return false;
}

PointSide ViewportWidget::GetViewportPointSide() const {
    if (renderFilter_ == RenderFilter::TargetOnly) {
        return PointSide::Target;
    } else if (renderFilter_ == RenderFilter::MorphOnly) {
        return PointSide::Morph;
    }
    // Default: if in single viewport mode showing all, treat as target
    return PointSide::Target;
}

void ViewportWidget::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta = event->pos() - lastMousePos_;
    lastMousePos_ = event->pos();

    bool inTouchUpStage = project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp;

    if (inTouchUpStage && (isSculpting_ || (!isOrbiting_ && !isPanning_))) {
        // In Touch Up: handle sculpting move or hover cursor update
        HandleSculptMove(event, delta);
    } else if (isTransforming_) {
        ApplyTransform(delta);
        update();
    } else if (isOrbiting_) {
        camera_->Orbit(delta.x(), delta.y());
        update();
        emit CameraChanged();
    } else if (isPanning_) {
        camera_->Pan(delta.x(), delta.y());
        update();
        emit CameraChanged();
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (isSculpting_) {
            HandleSculptRelease();
        } else if (isTransforming_) {
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
    emit CameraChanged();
}

void ViewportWidget::keyPressEvent(QKeyEvent *event) {
    // Check if we're in the Alignment stage for transform tools
    bool inAlignmentStage = project_ && project_->GetCurrentStage() == WorkflowStage::Alignment;
    bool inPointReferenceStage = project_ && project_->GetCurrentStage() == WorkflowStage::PointReference;
    bool inTouchUpStage = project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp;
    bool hasTargetMesh = project_ && project_->GetTargetMesh().isLoaded;

    switch (event->key()) {
        // Transform tool keys (only in Alignment stage with target mesh loaded)
        case Qt::Key_G:
            if (inAlignmentStage && hasTargetMesh) {
                SetTransformMode(TransformMode::Move);
            }
            break;

        case Qt::Key_R:
            if (inAlignmentStage && hasTargetMesh) {
                SetTransformMode(TransformMode::Rotate);
            }
            break;

        case Qt::Key_S:
            if (inAlignmentStage && hasTargetMesh) {
                SetTransformMode(TransformMode::Scale);
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

        // Brush radius keys (Touch Up stage)
        case Qt::Key_BracketLeft:
            if (inTouchUpStage) {
                SculptBrush* brush = GetActiveBrush();
                if (brush) {
                    float step = std::max(0.1f, brush->GetRadius() * 0.1f);
                    float newRadius = std::max(0.1f, brush->GetRadius() - step);
                    brush->SetRadius(newRadius);
                    emit BrushRadiusChanged(newRadius);
                    update();
                }
            }
            break;

        case Qt::Key_BracketRight:
            if (inTouchUpStage) {
                SculptBrush* brush = GetActiveBrush();
                if (brush) {
                    float step = std::max(0.1f, brush->GetRadius() * 0.1f);
                    float newRadius = std::min(100.0f, brush->GetRadius() + step);
                    brush->SetRadius(newRadius);
                    emit BrushRadiusChanged(newRadius);
                    update();
                }
            }
            break;

        // Escape cancels current transform or deselects point
        case Qt::Key_Escape:
            if (transformMode_ != TransformMode::None) {
                CancelTransform();
            } else if (inPointReferenceStage && selectedPointIndex_ >= 0) {
                selectedPointIndex_ = -1;
                if (renderer_) renderer_->SetSelectedPointIndex(-1);
                emit PointSelected(-1);
                update();
            }
            break;

        // Delete key removes selected point
        case Qt::Key_Delete:
            if (inPointReferenceStage && selectedPointIndex_ >= 0) {
                emit PointDeleteRequested();
            }
            break;

        // Camera shortcuts
        case Qt::Key_Home:
            camera_->Reset();
            update();
            emit CameraChanged();
            break;

        case Qt::Key_1:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicFront);
                camera_->SetPosition(Vector3(0.0f, 0.0f, 5.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
                emit CameraChanged();
            }
            break;

        case Qt::Key_3:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicRight);
                camera_->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
                emit CameraChanged();
            }
            break;

        case Qt::Key_7:
            if (event->modifiers() == Qt::NoModifier) {
                camera_->SetProjectionMode(ProjectionMode::OrthographicTop);
                camera_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
                camera_->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
                update();
                emit CameraChanged();
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
                emit CameraChanged();
            }
            break;

        case Qt::Key_F:
            // Focus on selection - for now just reset camera
            camera_->Reset();
            update();
            emit CameraChanged();
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
