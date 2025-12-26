#include "rendering/Renderer.h"
#include "rendering/ShaderManager.h"
#include "rendering/MeshRenderer.h"

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

    // TODO: Initialize grid geometry in Sprint 2

    return true;
}

void Renderer::Render(const Camera& camera, int width, int height) {
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (showGrid_) {
        RenderGrid();
    }

    // TODO: Render meshes in Sprint 2
}

void Renderer::RenderGrid() {
    // TODO: Implement grid rendering in Sprint 2
}

} // namespace MetaVisage
