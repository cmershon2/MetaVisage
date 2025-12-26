#include "rendering/Renderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/MeshRenderer.h"
#include <vector>

namespace MetaVisage {

Renderer::Renderer()
    : shaderManager_(nullptr),
      meshRenderer_(nullptr),
      showGrid_(true),
      gridVAO_(0),
      gridVBO_(0) {
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
    meshRenderer_ = std::make_unique<MeshRenderer>();

    // Load shaders
    shaderManager_->LoadShader("grid", "assets/shaders/grid.vert", "assets/shaders/grid.frag");
    shaderManager_->LoadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");

    // Create grid geometry
    const int gridSize = 10;
    const float gridSpacing = 1.0f;
    std::vector<float> gridVertices;

    // Grid lines parallel to X axis
    for (int i = -gridSize; i <= gridSize; ++i) {
        float z = i * gridSpacing;
        float color = (i == 0) ? 0.6f : 0.3f;

        gridVertices.push_back(-gridSize * gridSpacing); gridVertices.push_back(0.0f); gridVertices.push_back(z);
        gridVertices.push_back(color); gridVertices.push_back(color); gridVertices.push_back(color);

        gridVertices.push_back(gridSize * gridSpacing); gridVertices.push_back(0.0f); gridVertices.push_back(z);
        gridVertices.push_back(color); gridVertices.push_back(color); gridVertices.push_back(color);
    }

    // Grid lines parallel to Z axis
    for (int i = -gridSize; i <= gridSize; ++i) {
        float x = i * gridSpacing;
        float color = (i == 0) ? 0.6f : 0.6f;

        gridVertices.push_back(x); gridVertices.push_back(0.0f); gridVertices.push_back(-gridSize * gridSpacing);
        gridVertices.push_back(color); gridVertices.push_back(color); gridVertices.push_back(color);

        gridVertices.push_back(x); gridVertices.push_back(0.0f); gridVertices.push_back(gridSize * gridSpacing);
        gridVertices.push_back(color); gridVertices.push_back(color); gridVertices.push_back(color);
    }

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

void Renderer::Render(const Camera& camera, int width, int height) {
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = width / (float)height;
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 projection = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProjection = projection * view;

    if (showGrid_) {
        RenderGrid(viewProjection);
    }

    // TODO: Render meshes in Sprint 2
}

void Renderer::RenderGrid(const Matrix4x4& viewProjection) {
    if (gridVAO_ == 0) return;

    shaderManager_->UseShader("grid");
    unsigned int program = shaderManager_->GetShader("grid");

    int vpLoc = glGetUniformLocation(program, "uViewProjection");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    glBindVertexArray(gridVAO_);
    glDrawArrays(GL_LINES, 0, 84);
    glBindVertexArray(0);
}

} // namespace MetaVisage
