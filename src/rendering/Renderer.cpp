#include "rendering/Renderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/MeshRenderer.h"
#include "core/Project.h"
#include <vector>

namespace MetaVisage {

Renderer::Renderer()
    : shaderManager_(nullptr),
      showGrid_(true),
      shadingMode_(ShadingMode::Solid),
      renderFilter_(RenderFilter::All),
      gridVAO_(0),
      gridVBO_(0),
      gridVertexCount_(0) {
}

Renderer::~Renderer() {
    if (gridVAO_ != 0) {
        glDeleteVertexArrays(1, &gridVAO_);
    }
    if (gridVBO_ != 0) {
        glDeleteBuffers(1, &gridVBO_);
    }
}

bool Renderer::Initialize() {
    initializeOpenGLFunctions();

    shaderManager_ = std::make_unique<ShaderManager>();

    // Load shaders
    shaderManager_->LoadShader("grid", "assets/shaders/grid.vert", "assets/shaders/grid.frag");
    shaderManager_->LoadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");

    // Create grid geometry - sized for typical 3D models (1-10 unit scale)
    // Grid extends 10 units in each direction with lines every 1 unit
    const int gridLines = 10;          // Number of lines on each side of center
    const float gridSpacing = 1.0f;    // Grid lines every 1 unit
    const float gridExtent = gridLines * gridSpacing; // Total extent: 10 units each direction
    std::vector<float> gridVertices;

    // Grid lines parallel to X axis (running along X, varying Z)
    for (int i = -gridLines; i <= gridLines; ++i) {
        float z = i * gridSpacing;
        // Center axis (Z=0) is blue-ish, others are gray
        float r = (i == 0) ? 0.3f : 0.35f;
        float g = (i == 0) ? 0.3f : 0.35f;
        float b = (i == 0) ? 0.6f : 0.35f;

        // Line start
        gridVertices.push_back(-gridExtent);  // x
        gridVertices.push_back(0.0f);         // y
        gridVertices.push_back(z);            // z
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);

        // Line end
        gridVertices.push_back(gridExtent);   // x
        gridVertices.push_back(0.0f);         // y
        gridVertices.push_back(z);            // z
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);
    }

    // Grid lines parallel to Z axis (running along Z, varying X)
    for (int i = -gridLines; i <= gridLines; ++i) {
        float x = i * gridSpacing;
        // Center axis (X=0) is red-ish, others are gray
        float r = (i == 0) ? 0.6f : 0.35f;
        float g = (i == 0) ? 0.3f : 0.35f;
        float b = (i == 0) ? 0.3f : 0.35f;

        // Line start
        gridVertices.push_back(x);            // x
        gridVertices.push_back(0.0f);         // y
        gridVertices.push_back(-gridExtent);  // z
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);

        // Line end
        gridVertices.push_back(x);            // x
        gridVertices.push_back(0.0f);         // y
        gridVertices.push_back(gridExtent);   // z
        gridVertices.push_back(r);
        gridVertices.push_back(g);
        gridVertices.push_back(b);
    }

    // Each axis has (gridLines * 2 + 1) lines, each line has 2 vertices
    // Total vertices = 2 axes * (gridLines * 2 + 1) lines * 2 vertices
    gridVertexCount_ = 2 * (gridLines * 2 + 1) * 2;

    // Create VAO and VBO for grid
    glGenVertexArrays(1, &gridVAO_);
    glGenBuffers(1, &gridVBO_);

    glBindVertexArray(gridVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void Renderer::Render(const Camera& camera, int width, int height, Project* project) {
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Ensure depth testing is enabled with proper settings
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
        // Render morph mesh (blue color, locked in place)
        if (renderFilter_ != RenderFilter::TargetOnly) {
            const MeshReference& morphMeshRef = project->GetMorphMesh();
            if (morphMeshRef.isLoaded && morphMeshRef.mesh) {
                Vector3 morphColor(0.3f, 0.5f, 0.9f); // Blue
                RenderMesh(*morphMeshRef.mesh, morphMeshRef.transform, morphColor, viewProjection);
            }
        }

        // Render target mesh (orange color, transformable)
        if (renderFilter_ != RenderFilter::MorphOnly) {
            const MeshReference& targetMeshRef = project->GetTargetMesh();
            if (targetMeshRef.isLoaded && targetMeshRef.mesh) {
                Vector3 targetColor(0.9f, 0.6f, 0.2f); // Orange
                RenderMesh(*targetMeshRef.mesh, targetMeshRef.transform, targetColor, viewProjection);
            }
        }
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

void Renderer::RenderMesh(const Mesh& mesh, const Transform& transform,
                          const Vector3& color, const Matrix4x4& viewProjection) {
    // Get or create mesh renderer for this mesh
    const Mesh* meshPtr = &mesh;
    if (meshRenderers_.find(meshPtr) == meshRenderers_.end()) {
        meshRenderers_[meshPtr] = std::make_unique<MeshRenderer>();
        meshRenderers_[meshPtr]->UploadMesh(mesh);
    }

    MeshRenderer* renderer = meshRenderers_[meshPtr].get();
    if (renderer->HasMesh()) {
        unsigned int program = shaderManager_->GetShader("basic");
        renderer->Render(program, viewProjection, transform, shadingMode_, color);
    }
}

} // namespace MetaVisage
