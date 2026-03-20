#include "rendering/Renderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/MeshRenderer.h"
#include "core/Project.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace MetaVisage {

Renderer::Renderer()
    : shaderManager_(nullptr),
      showGrid_(true),
      shadingMode_(ShadingMode::Solid),
      renderFilter_(RenderFilter::All),
      morphPreviewMode_(MorphPreviewMode::Deformed),
      gridVAO_(0),
      gridVBO_(0),
      gridVertexCount_(0),
      pointSize_(12.0f),
      selectedPointIndex_(-1),
      brushCursorVAO_(0),
      brushCursorVBO_(0),
      brushCursorVisible_(false),
      brushCursorSegments_(0),
      brushCursorDirty_(false),
      brushCursorRadius_(0.0f),
      brushCursorInnerRadius_(0.0f),
      showTargetOverlay_(false),
      showMask_(false) {
}

Renderer::~Renderer() {
    if (gridVAO_ != 0) {
        glDeleteVertexArrays(1, &gridVAO_);
    }
    if (gridVBO_ != 0) {
        glDeleteBuffers(1, &gridVBO_);
    }
    if (brushCursorVAO_ != 0) {
        glDeleteVertexArrays(1, &brushCursorVAO_);
    }
    if (brushCursorVBO_ != 0) {
        glDeleteBuffers(1, &brushCursorVBO_);
    }
}

bool Renderer::Initialize() {
    initializeOpenGLFunctions();

    shaderManager_ = std::make_unique<ShaderManager>();

    // Load shaders
    shaderManager_->LoadShader("grid", "assets/shaders/grid.vert", "assets/shaders/grid.frag");
    shaderManager_->LoadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    shaderManager_->LoadShader("point", "assets/shaders/point.vert", "assets/shaders/point.frag");
    shaderManager_->LoadShader("heatmap", "assets/shaders/heatmap.vert", "assets/shaders/heatmap.frag");
    shaderManager_->LoadShader("overlay", "assets/shaders/basic.vert", "assets/shaders/overlay.frag");
    shaderManager_->LoadShader("matcap", "assets/shaders/matcap.vert", "assets/shaders/matcap.frag");

    // Initialize point renderer
    pointRenderer_ = std::make_unique<PointRenderer>();
    pointRenderer_->Initialize();

    // Create grid geometry - sized for typical 3D models (1-10 unit scale)
    const int gridLines = 10;
    const float gridSpacing = 1.0f;
    const float gridExtent = gridLines * gridSpacing;
    std::vector<float> gridVertices;

    // Grid lines parallel to X axis (running along X, varying Z)
    for (int i = -gridLines; i <= gridLines; ++i) {
        float z = i * gridSpacing;
        float r = (i == 0) ? 0.3f : 0.35f;
        float g = (i == 0) ? 0.3f : 0.35f;
        float b = (i == 0) ? 0.6f : 0.35f;

        gridVertices.push_back(-gridExtent);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(z);
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);

        gridVertices.push_back(gridExtent);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(z);
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);
    }

    // Grid lines parallel to Z axis (running along Z, varying X)
    for (int i = -gridLines; i <= gridLines; ++i) {
        float x = i * gridSpacing;
        float r = (i == 0) ? 0.6f : 0.35f;
        float g = (i == 0) ? 0.3f : 0.35f;
        float b = (i == 0) ? 0.3f : 0.35f;

        gridVertices.push_back(x);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(-gridExtent);
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);

        gridVertices.push_back(x);
        gridVertices.push_back(0.0f);
        gridVertices.push_back(gridExtent);
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);
    }

    gridVertexCount_ = 2 * (gridLines * 2 + 1) * 2;

    glGenVertexArrays(1, &gridVAO_);
    glGenBuffers(1, &gridVBO_);

    glBindVertexArray(gridVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void Renderer::Render(const Camera& camera, int width, int height, Project* project) {
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    float aspectRatio = width / (float)height;
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 projection = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProjection = projection * view;

    if (showGrid_) {
        RenderGrid(viewProjection);
    }

    if (project) {
        // Morph stage uses specialized rendering
        if (project->GetCurrentStage() == WorkflowStage::Morph) {
            RenderMorphStage(camera, width, height, project, viewProjection);
            return;
        }

        // Touch Up stage uses specialized rendering
        if (project->GetCurrentStage() == WorkflowStage::TouchUp) {
            RenderTouchUpStage(camera, width, height, project, viewProjection);
            return;
        }

        // Render morph mesh (blue color, locked in place)
        if (renderFilter_ != RenderFilter::TargetOnly) {
            const MeshReference& morphMeshRef = project->GetMorphMesh();
            if (morphMeshRef.isLoaded && morphMeshRef.mesh) {
                Vector3 morphColor(0.3f, 0.5f, 0.9f);
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            }
        }

        // Render target mesh (orange color, transformable)
        if (renderFilter_ != RenderFilter::MorphOnly) {
            const MeshReference& targetMeshRef = project->GetTargetMesh();
            if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
                Vector3 targetColor(0.9f, 0.6f, 0.2f);
                RenderMesh(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, viewProjection);
            }
        }

        // Render correspondence points (only in PointReference stage)
        if (project->GetCurrentStage() == WorkflowStage::PointReference) {
            RenderPoints(camera, width, height, project);
        }
    }
}

void Renderer::RenderMorphStage(const Camera& /*camera*/, int /*width*/, int /*height*/, Project* project,
                                 const Matrix4x4& viewProjection) {
    const MorphData& morphData = project->GetMorphData();
    const MeshReference& morphMeshRef = project->GetMorphMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh) return;

    Vector3 morphColor(0.3f, 0.5f, 0.9f);

    // If mask is being shown, render with mask colors using heat map shader
    if (showMask_ && !morphData.vertexMask.empty()) {
        // Render the morph mesh (original or deformed) with mask vertex colors
        const Mesh* meshToRender = morphMeshRef.mesh.get();
        if (morphData.isProcessed && morphData.deformedMorphMesh) {
            meshToRender = morphData.deformedMorphMesh.get();
        }
        RenderMeshHeatMap(*meshToRender, morphMeshRef.transform, viewProjection);
        // Also render brush cursor for mask painting
        RenderBrushCursor(viewProjection);
        return;
    }

    switch (morphPreviewMode_) {
        case MorphPreviewMode::Deformed:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            }
            break;

        case MorphPreviewMode::Original:
            RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            break;

        case MorphPreviewMode::Overlay:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            }
            {
                const MeshReference& targetMeshRef = project->GetTargetMesh();
                if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
                    Vector3 targetColor(0.9f, 0.6f, 0.2f);
                    RenderMeshOverlay(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, 0.35f, viewProjection);
                }
            }
            break;

        case MorphPreviewMode::HeatMap:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMeshHeatMap(*morphData.deformedMorphMesh, morphMeshRef.transform, viewProjection);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            }
            break;
    }
}

void Renderer::RenderTouchUpStage(const Camera& /*camera*/, int /*width*/, int /*height*/,
                                   Project* project, const Matrix4x4& viewProjection) {
    const MorphData& morphData = project->GetMorphData();
    const MeshReference& morphMeshRef = project->GetMorphMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh) return;

    Vector3 morphColor(0.3f, 0.5f, 0.9f);

    // Render the deformed mesh for sculpting
    if (morphData.deformedMorphMesh) {
        RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection);
    } else {
        RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
    }

    // Render target mesh overlay if enabled
    if (showTargetOverlay_) {
        const MeshReference& targetMeshRef = project->GetTargetMesh();
        if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
            Vector3 targetColor(0.9f, 0.6f, 0.2f);
            RenderMeshOverlay(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, 0.3f, viewProjection);
        }
    }

    // Render brush cursor on top
    RenderBrushCursor(viewProjection);
}

void Renderer::SetBrushCursor(const Vector3& position, const Vector3& normal, float radius, float innerRadius, bool visible) {
    // Only store parameters here - NO GL calls allowed since this is called from
    // mouse event handlers outside of a valid GL context. The actual geometry
    // generation and GPU upload happens in RenderBrushCursor() during paintGL().
    brushCursorVisible_ = visible;
    brushCursorPosition_ = position;
    brushCursorNormal_ = normal;
    brushCursorRadius_ = radius;
    brushCursorInnerRadius_ = innerRadius;
    brushCursorDirty_ = true;
}

void Renderer::RenderBrushCursor(const Matrix4x4& viewProjection) {
    if (!brushCursorVisible_ || brushCursorRadius_ <= 0.0f) return;

    bool hasInnerCircle = (brushCursorInnerRadius_ > 0.0f &&
                           brushCursorInnerRadius_ < brushCursorRadius_);

    // Rebuild circle geometry if parameters changed (we're now in a valid GL context)
    if (brushCursorDirty_) {
        brushCursorDirty_ = false;

        const int segments = 64;
        const float PI = 3.14159265359f;

        // Find tangent vectors perpendicular to the normal
        Vector3 tangent1, tangent2;
        Vector3 up(0.0f, 1.0f, 0.0f);
        if (std::abs(brushCursorNormal_.Dot(up)) > 0.9f) {
            up = Vector3(1.0f, 0.0f, 0.0f);
        }
        tangent1 = brushCursorNormal_.Cross(up).Normalized();
        tangent2 = brushCursorNormal_.Cross(tangent1).Normalized();

        // Slight offset along normal to avoid z-fighting
        Vector3 offset = brushCursorNormal_ * 0.003f;

        std::vector<float> circleData;
        int totalVerts = hasInnerCircle ? segments * 2 : segments;
        circleData.reserve(totalVerts * 6);

        // Outer circle - bright cyan (bold)
        float cr = 0.2f, cg = 0.85f, cb = 1.0f;
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * PI * i / segments;
            float cos_a = std::cos(angle);
            float sin_a = std::sin(angle);

            Vector3 point = brushCursorPosition_ + offset +
                            (tangent1 * cos_a + tangent2 * sin_a) * brushCursorRadius_;

            circleData.push_back(point.x);
            circleData.push_back(point.y);
            circleData.push_back(point.z);
            circleData.push_back(cr);
            circleData.push_back(cg);
            circleData.push_back(cb);
        }

        // Inner circle - dimmer cyan (thin falloff indicator)
        if (hasInnerCircle) {
            float icr = 0.15f, icg = 0.55f, icb = 0.7f;
            for (int i = 0; i < segments; ++i) {
                float angle = 2.0f * PI * i / segments;
                float cos_a = std::cos(angle);
                float sin_a = std::sin(angle);

                Vector3 point = brushCursorPosition_ + offset +
                                (tangent1 * cos_a + tangent2 * sin_a) * brushCursorInnerRadius_;

                circleData.push_back(point.x);
                circleData.push_back(point.y);
                circleData.push_back(point.z);
                circleData.push_back(icr);
                circleData.push_back(icg);
                circleData.push_back(icb);
            }
        }

        // Create or update GPU buffers
        if (brushCursorVAO_ == 0) {
            glGenVertexArrays(1, &brushCursorVAO_);
            glGenBuffers(1, &brushCursorVBO_);
        }

        glBindVertexArray(brushCursorVAO_);
        glBindBuffer(GL_ARRAY_BUFFER, brushCursorVBO_);
        glBufferData(GL_ARRAY_BUFFER, circleData.size() * sizeof(float),
                     circleData.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        brushCursorSegments_ = segments;
    }

    if (brushCursorVAO_ == 0 || brushCursorSegments_ == 0) return;

    shaderManager_->UseShader("grid");
    unsigned int program = shaderManager_->GetShader("grid");

    int vpLoc = glGetUniformLocation(program, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(brushCursorVAO_);

    // Draw outer circle (bold)
    glLineWidth(2.5f);
    glDrawArrays(GL_LINE_LOOP, 0, brushCursorSegments_);

    // Draw inner circle (thin falloff indicator)
    if (hasInnerCircle) {
        glLineWidth(1.0f);
        glDrawArrays(GL_LINE_LOOP, brushCursorSegments_, brushCursorSegments_);
    }

    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glLineWidth(1.0f);
}

void Renderer::UpdateMeshVertices(const Mesh* mesh) {
    auto it = meshRenderers_.find(mesh);
    if (it != meshRenderers_.end()) {
        it->second->UpdateVertexData(*mesh);
    }
}

void Renderer::RenderGrid(const Matrix4x4& viewProjection) {
    if (gridVAO_ == 0 || gridVertexCount_ == 0) return;

    shaderManager_->UseShader("grid");
    unsigned int program = shaderManager_->GetShader("grid");

    int vpLoc = glGetUniformLocation(program, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    glBindVertexArray(gridVAO_);
    glDrawArrays(GL_LINES, 0, gridVertexCount_);
    glBindVertexArray(0);
}

MeshRenderer* Renderer::GetOrCreateMeshRenderer(const Mesh& mesh) {
    const Mesh* meshPtr = &mesh;
    if (meshRenderers_.find(meshPtr) == meshRenderers_.end()) {
        meshRenderers_[meshPtr] = std::make_unique<MeshRenderer>();
        meshRenderers_[meshPtr]->UploadMesh(mesh);
    }
    return meshRenderers_[meshPtr].get();
}

void Renderer::RenderMesh(const Mesh& mesh, const Transform& transform,
                          const Vector3& color, const Matrix4x4& viewProjection) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("basic");
        renderer->Render(program, viewProjection, transform, shadingMode_, color);
    }
}

void Renderer::RenderMeshOverlay(const Mesh& mesh, const Transform& transform,
                                  const Vector3& color, float alpha, const Matrix4x4& viewProjection) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("overlay");
        renderer->RenderWithAlpha(program, viewProjection, transform, shadingMode_, color, alpha);
    }
}

void Renderer::RenderMeshHeatMap(const Mesh& mesh, const Transform& transform,
                                  const Matrix4x4& viewProjection) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("heatmap");
        renderer->RenderHeatMap(program, viewProjection, transform, shadingMode_);
    }
}

void Renderer::InvalidateMesh(const Mesh* mesh) {
    auto it = meshRenderers_.find(mesh);
    if (it != meshRenderers_.end()) {
        meshRenderers_.erase(it);
    }
}

void Renderer::UploadHeatMapColors(const Mesh* mesh, const std::vector<float>& displacements, float maxDisplacement) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(*mesh);
    if (!renderer->HasMesh()) return;

    std::vector<Vector3> colors;
    colors.reserve(displacements.size());

    float maxDisp = (maxDisplacement > 0.0f) ? maxDisplacement : 1.0f;

    for (float mag : displacements) {
        float t = std::min(mag / maxDisp, 1.0f);
        Vector3 color;

        if (t < 0.25f) {
            float s = t / 0.25f;
            color = Vector3(0.0f, s, 1.0f);
        } else if (t < 0.5f) {
            float s = (t - 0.25f) / 0.25f;
            color = Vector3(0.0f, 1.0f, 1.0f - s);
        } else if (t < 0.75f) {
            float s = (t - 0.5f) / 0.25f;
            color = Vector3(s, 1.0f, 0.0f);
        } else {
            float s = (t - 0.75f) / 0.25f;
            color = Vector3(1.0f, 1.0f - s, 0.0f);
        }

        colors.push_back(color);
    }

    renderer->UploadVertexColors(colors);
}

void Renderer::UploadMaskColors(const Mesh* mesh, const std::vector<bool>& mask) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(*mesh);
    if (!renderer->HasMesh()) return;

    std::vector<Vector3> colors;
    colors.reserve(mask.size());

    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i]) {
            colors.push_back(Vector3(0.9f, 0.2f, 0.2f));  // Red for masked
        } else {
            colors.push_back(Vector3(0.3f, 0.5f, 0.9f));  // Default mesh blue
        }
    }

    renderer->UploadVertexColors(colors);
}

void Renderer::RenderPoints(const Camera& camera, int width, int height, Project* project) {
    if (!pointRenderer_ || !project) return;

    const auto& correspondences = project->GetPointReferenceData().correspondences;
    if (correspondences.empty()) return;

    float aspectRatio = width / (float)height;
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 projection = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProjection = projection * view;

    std::vector<PointMarkerData> markers;

    Vector3 greenColor(0.18f, 0.8f, 0.44f);
    Vector3 orangeColor(0.9f, 0.49f, 0.13f);
    Vector3 yellowColor(1.0f, 0.84f, 0.0f);

    for (size_t i = 0; i < correspondences.size(); ++i) {
        const auto& corr = correspondences[i];
        int corrIdx = static_cast<int>(i);
        bool isSelected = (corrIdx == selectedPointIndex_);
        bool isLastPoint = (i == correspondences.size() - 1);

        if (renderFilter_ != RenderFilter::MorphOnly && corr.targetMeshVertexIndex >= 0) {
            PointMarkerData marker;
            marker.position = corr.targetMeshPosition;
            if (isSelected) marker.color = orangeColor;
            else if (isLastPoint) marker.color = yellowColor;
            else marker.color = greenColor;
            markers.push_back(marker);
        }

        if (renderFilter_ != RenderFilter::TargetOnly && corr.morphMeshVertexIndex >= 0) {
            PointMarkerData marker;
            marker.position = corr.morphMeshPosition;
            if (isSelected) marker.color = orangeColor;
            else if (isLastPoint) marker.color = yellowColor;
            else marker.color = greenColor;
            markers.push_back(marker);
        }
    }

    if (!markers.empty()) {
        pointRenderer_->UpdatePoints(markers);
        unsigned int program = shaderManager_->GetShader("point");
        pointRenderer_->Render(program, viewProjection, pointSize_);
    }
}

} // namespace MetaVisage
