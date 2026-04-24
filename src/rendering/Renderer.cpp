#include "rendering/Renderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/MeshRenderer.h"
#include "rendering/GizmoRenderer.h"
#include "core/Project.h"
#include "core/TextureData.h"
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
      showMask_(false),
      gizmoVAO_(0),
      gizmoVBO_(0),
      gizmoRenderer_(nullptr) {
}

Renderer::~Renderer() {
    if (gridVAO_ != 0) {
        glDeleteVertexArrays(1, &gridVAO_);
    }
    if (brushCursorVAO_ != 0) {
        glDeleteVertexArrays(1, &brushCursorVAO_);
    }
    if (brushCursorVBO_ != 0) {
        glDeleteBuffers(1, &brushCursorVBO_);
    }
    if (gizmoVAO_ != 0) {
        glDeleteVertexArrays(1, &gizmoVAO_);
    }
    if (gizmoVBO_ != 0) {
        glDeleteBuffers(1, &gizmoVBO_);
    }
}

bool Renderer::Initialize() {
    shaderManager_ = std::make_unique<ShaderManager>();

    // Load shaders
    shaderManager_->LoadShader("grid", "assets/shaders/grid.vert", "assets/shaders/grid.frag");
    shaderManager_->LoadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    shaderManager_->LoadShader("point", "assets/shaders/point.vert", "assets/shaders/point.frag");
    shaderManager_->LoadShader("heatmap", "assets/shaders/heatmap.vert", "assets/shaders/heatmap.frag");
    shaderManager_->LoadShader("overlay", "assets/shaders/basic.vert", "assets/shaders/overlay.frag");
    shaderManager_->LoadShader("matcap", "assets/shaders/matcap.vert", "assets/shaders/matcap.frag");
    shaderManager_->LoadShader("textured", "assets/shaders/basic.vert", "assets/shaders/textured.frag");
    shaderManager_->LoadShader("gizmo", "assets/shaders/gizmo.vert", "assets/shaders/gizmo.frag");
    shaderManager_->LoadShader("gizmo_transform", "assets/shaders/gizmo_transform.vert", "assets/shaders/gizmo_transform.frag");

    // Initialize transform gizmo renderer
    gizmoRenderer_ = std::make_unique<GizmoRenderer>();
    gizmoRenderer_->Initialize();

    // Initialize point renderer
    pointRenderer_ = std::make_unique<PointRenderer>();
    pointRenderer_->Initialize();

    // Create empty VAO for fullscreen grid rendering (uses gl_VertexID, no vertex data)
    glGenVertexArrays(1, &gridVAO_);
    gridVertexCount_ = 3; // Fullscreen triangle

    // Create orientation gizmo geometry (3 axis lines + 3 arrowhead cones)
    // Each line: origin to axis tip, colored by axis
    // Format: position(3) + color(3) per vertex
    std::vector<float> gizmoVertices;
    float axisLen = 0.7f;
    float arrowLen = 0.15f;
    float arrowWidth = 0.06f;

    // Helper lambda to add a line segment
    auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                       float r, float g, float b) {
        gizmoVertices.insert(gizmoVertices.end(), {x1, y1, z1, r, g, b});
        gizmoVertices.insert(gizmoVertices.end(), {x2, y2, z2, r, g, b});
    };

    // X axis (red)
    addLine(0, 0, 0, axisLen, 0, 0, 0.9f, 0.2f, 0.2f);
    // X arrowhead
    addLine(axisLen, 0, 0, axisLen - arrowLen, arrowWidth, 0, 0.9f, 0.2f, 0.2f);
    addLine(axisLen, 0, 0, axisLen - arrowLen, -arrowWidth, 0, 0.9f, 0.2f, 0.2f);
    addLine(axisLen, 0, 0, axisLen - arrowLen, 0, arrowWidth, 0.9f, 0.2f, 0.2f);
    addLine(axisLen, 0, 0, axisLen - arrowLen, 0, -arrowWidth, 0.9f, 0.2f, 0.2f);

    // Y axis (green)
    addLine(0, 0, 0, 0, axisLen, 0, 0.2f, 0.8f, 0.2f);
    // Y arrowhead
    addLine(0, axisLen, 0, arrowWidth, axisLen - arrowLen, 0, 0.2f, 0.8f, 0.2f);
    addLine(0, axisLen, 0, -arrowWidth, axisLen - arrowLen, 0, 0.2f, 0.8f, 0.2f);
    addLine(0, axisLen, 0, 0, axisLen - arrowLen, arrowWidth, 0.2f, 0.8f, 0.2f);
    addLine(0, axisLen, 0, 0, axisLen - arrowLen, -arrowWidth, 0.2f, 0.8f, 0.2f);

    // Z axis (blue)
    addLine(0, 0, 0, 0, 0, axisLen, 0.3f, 0.5f, 0.9f);
    // Z arrowhead
    addLine(0, 0, axisLen, arrowWidth, 0, axisLen - arrowLen, 0.3f, 0.5f, 0.9f);
    addLine(0, 0, axisLen, -arrowWidth, 0, axisLen - arrowLen, 0.3f, 0.5f, 0.9f);
    addLine(0, 0, axisLen, 0, arrowWidth, axisLen - arrowLen, 0.3f, 0.5f, 0.9f);
    addLine(0, 0, axisLen, 0, -arrowWidth, axisLen - arrowLen, 0.3f, 0.5f, 0.9f);

    glGenVertexArrays(1, &gizmoVAO_);
    glGenBuffers(1, &gizmoVBO_);

    glBindVertexArray(gizmoVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO_);
    glBufferData(GL_ARRAY_BUFFER, gizmoVertices.size() * sizeof(float), gizmoVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void Renderer::Render(const Camera& camera, int width, int height, Project* project) {
    // Viewport origin is owned by the caller (Viewport::RenderGL) — in dual
    // split-screen mode the right pane's GL origin is (halfW, 0), so a reset
    // to (0, 0, w, h) here would re-anchor drawing to the bottom-left of the
    // full framebuffer and the scissor would clip every pixel out. Do NOT
    // call glViewport here. glClear still honors the caller's scissor rect.
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

    Vector3 viewPos = camera.GetPosition();

    if (project) {
        // Morph stage uses specialized rendering
        if (project->GetCurrentStage() == WorkflowStage::Morph) {
            RenderMorphStage(camera, width, height, project, viewProjection);
            // Orientation gizmo is drawn per-viewport by
            // Viewport::DrawOrientationGizmoOverlay (ImGui). The GL gizmo
            // here would clobber the viewport offset in dual-pane mode.
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
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            }
        }

        // Render target mesh (orange color, transformable)
        if (renderFilter_ != RenderFilter::MorphOnly) {
            const MeshReference& targetMeshRef = project->GetTargetMesh();
            if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
                Vector3 targetColor(0.9f, 0.6f, 0.2f);
                RenderMesh(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, viewProjection, viewPos);
            }
        }

        // Render correspondence points (only in PointReference stage)
        if (project->GetCurrentStage() == WorkflowStage::PointReference) {
            RenderPoints(camera, width, height, project);
        }
    }

    // Orientation gizmo is drawn per-viewport by
    // Viewport::DrawOrientationGizmoOverlay (ImGui), outside this GL pass.
}

void Renderer::RenderMorphStage(const Camera& camera, int /*width*/, int /*height*/, Project* project,
                                 const Matrix4x4& viewProjection) {
    const MorphData& morphData = project->GetMorphData();
    const MeshReference& morphMeshRef = project->GetMorphMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh) return;

    Vector3 morphColor(0.3f, 0.5f, 0.9f);
    Vector3 viewPos = camera.GetPosition();

    // If mask is being shown, render with mask colors using heat map shader
    if (showMask_ && !morphData.vertexMask.empty()) {
        // Render the morph mesh (original or deformed) with mask vertex colors
        const Mesh* meshToRender = morphMeshRef.mesh.get();
        if (morphData.isProcessed && morphData.deformedMorphMesh) {
            meshToRender = morphData.deformedMorphMesh.get();
        }
        RenderMeshHeatMap(*meshToRender, morphMeshRef.transform, viewProjection, viewPos);
        // Also render brush cursor for mask painting
        RenderBrushCursor(viewProjection);
        return;
    }

    switch (morphPreviewMode_) {
        case MorphPreviewMode::Deformed:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            }
            break;

        case MorphPreviewMode::Original:
            RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            break;

        case MorphPreviewMode::Overlay:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            }
            {
                const MeshReference& targetMeshRef = project->GetTargetMesh();
                if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
                    Vector3 targetColor(0.9f, 0.6f, 0.2f);
                    RenderMeshOverlay(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, 0.35f, viewProjection, viewPos);
                }
            }
            break;

        case MorphPreviewMode::HeatMap:
            if (morphData.isProcessed && morphData.deformedMorphMesh) {
                RenderMeshHeatMap(*morphData.deformedMorphMesh, morphMeshRef.transform, viewProjection, viewPos);
            } else {
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
            }
            break;
    }
}

void Renderer::RenderTouchUpStage(const Camera& camera, int /*width*/, int /*height*/,
                                   Project* project, const Matrix4x4& viewProjection) {
    const MorphData& morphData = project->GetMorphData();
    const MeshReference& morphMeshRef = project->GetMorphMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh) return;

    Vector3 morphColor(0.3f, 0.5f, 0.9f);
    Vector3 viewPos = camera.GetPosition();

    // Render the deformed mesh for sculpting
    if (morphData.deformedMorphMesh) {
        RenderMesh(*morphData.deformedMorphMesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
    } else {
        RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection, viewPos);
    }

    // Render target mesh overlay if enabled
    if (showTargetOverlay_) {
        const MeshReference& targetMeshRef = project->GetTargetMesh();
        if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
            Vector3 targetColor(0.9f, 0.6f, 0.2f);
            RenderMeshOverlay(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, 0.3f, viewProjection, viewPos);
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

    shaderManager_->UseShader("gizmo");
    unsigned int program = shaderManager_->GetShader("gizmo");

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
    if (gridVAO_ == 0) return;

    shaderManager_->UseShader("grid");
    unsigned int program = shaderManager_->GetShader("grid");

    // Set view-projection and its inverse for ray unprojection
    int vpLoc = glGetUniformLocation(program, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    Matrix4x4 invVP = viewProjection.Inverse();
    int invVPLoc = glGetUniformLocation(program, "uInvViewProjection");
    glUniformMatrix4fv(invVPLoc, 1, GL_FALSE, invVP.Data());

    int nearLoc = glGetUniformLocation(program, "uNearPlane");
    int farLoc = glGetUniformLocation(program, "uFarPlane");
    glUniform1f(nearLoc, 0.01f);
    glUniform1f(farLoc, 100000.0f);

    // Enable blending for smooth grid fade
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(gridVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Renderer::RenderOrientationGizmo(const Camera& camera, int viewportWidth, int viewportHeight) {
    if (gizmoVAO_ == 0) return;

    const int gizmoSize = 80;
    const int padding = 10;
    int gizmoX = viewportWidth - gizmoSize - padding;
    int gizmoY = viewportHeight - gizmoSize - padding; // OpenGL Y is bottom-up

    // Set up a small viewport in the top-right corner
    glViewport(gizmoX, gizmoY, gizmoSize, gizmoSize);
    glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so gizmo renders on top

    // Build a view matrix that only uses the camera's rotation (no translation)
    // Place the "eye" at a fixed distance looking at the origin
    Vector3 camPos = camera.GetPosition();
    Vector3 camTarget = camera.GetTarget();
    Vector3 viewDir = (camTarget - camPos).Normalized();
    Vector3 eye = viewDir * -2.0f; // Look from the opposite direction at distance 2
    Matrix4x4 view = Matrix4x4::LookAt(eye, Vector3(0, 0, 0), camera.GetUp());
    Matrix4x4 proj = Matrix4x4::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 10.0f);
    Matrix4x4 vp = proj * view;

    shaderManager_->UseShader("gizmo");
    unsigned int program = shaderManager_->GetShader("gizmo");

    int vpLoc = glGetUniformLocation(program, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, vp.Data());

    glBindVertexArray(gizmoVAO_);
    glLineWidth(2.5f);
    glDrawArrays(GL_LINES, 0, 30); // 3 axes * (1 shaft + 4 arrowhead lines) * 2 verts = 30
    glBindVertexArray(0);
    glLineWidth(1.0f);

    // Restore full viewport
    glViewport(0, 0, viewportWidth, viewportHeight);
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
                          const Vector3& color, const Matrix4x4& viewProjection, const Vector3& viewPos) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        // Use textured shader when in Textured mode and texture is available
        unsigned int program;
        if (shadingMode_ == ShadingMode::Textured && renderer->HasTexture()) {
            program = shaderManager_->GetShader("textured");
        } else {
            program = shaderManager_->GetShader("basic");
        }
        renderer->Render(program, viewProjection, transform, shadingMode_, color, viewPos);
    }
}

void Renderer::RenderMeshOverlay(const Mesh& mesh, const Transform& transform,
                                  const Vector3& color, float alpha, const Matrix4x4& viewProjection, const Vector3& viewPos) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("overlay");
        renderer->RenderWithAlpha(program, viewProjection, transform, shadingMode_, color, alpha, viewPos);
    }
}

void Renderer::RenderMeshHeatMap(const Mesh& mesh, const Transform& transform,
                                  const Matrix4x4& viewProjection, const Vector3& viewPos) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(mesh);
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("heatmap");
        renderer->RenderHeatMap(program, viewProjection, transform, shadingMode_, viewPos);
    }
}

void Renderer::InvalidateMesh(const Mesh* mesh) {
    auto it = meshRenderers_.find(mesh);
    if (it != meshRenderers_.end()) {
        meshRenderers_.erase(it);
    }
}

void Renderer::UploadMeshTexture(const Mesh* mesh, const TextureData& texture) {
    MeshRenderer* renderer = GetOrCreateMeshRenderer(*mesh);
    if (renderer->HasMesh()) {
        renderer->UploadTexture(texture);
    }
}

void Renderer::ClearMeshTexture(const Mesh* mesh) {
    auto it = meshRenderers_.find(mesh);
    if (it != meshRenderers_.end()) {
        it->second->ClearTexture();
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

void Renderer::RenderTransformGizmo(TransformMode mode, const Vector3& position,
                                     GizmoAxis hoveredAxis, GizmoAxis activeAxis,
                                     const Camera& camera, int viewportWidth, int viewportHeight) {
    if (!gizmoRenderer_ || mode == TransformMode::None) return;

    unsigned int program = shaderManager_->GetShader("gizmo_transform");
    if (program == 0) return;

    gizmoRenderer_->Render(mode, position, hoveredAxis, activeAxis,
                           camera, viewportWidth, viewportHeight, program);
}

GizmoAxis Renderer::GizmoHitTest(TransformMode mode, const Vector3& gizmoPosition,
                                  const Ray& ray, const Camera& camera,
                                  int viewportWidth, int viewportHeight) const {
    if (!gizmoRenderer_ || mode == TransformMode::None) return GizmoAxis::None;
    return gizmoRenderer_->HitTest(mode, gizmoPosition, ray, camera,
                                   viewportWidth, viewportHeight);
}

} // namespace MetaVisage
