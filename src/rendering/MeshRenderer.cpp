#include "rendering/MeshRenderer.h"

namespace MetaVisage {

MeshRenderer::MeshRenderer()
    : vao_(0),
      vbo_(0),
      ibo_(0),
      indexCount_(0) {
    initializeOpenGLFunctions();
}

MeshRenderer::~MeshRenderer() {
    Clear();
}

void MeshRenderer::UploadMesh(const Mesh& mesh) {
    // TODO: Implement mesh upload in Sprint 2
}

void MeshRenderer::Render(const Camera& camera) {
    // TODO: Implement mesh rendering in Sprint 2
}

void MeshRenderer::Clear() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ibo_ != 0) {
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }
    indexCount_ = 0;
}

} // namespace MetaVisage
