#include "rendering/PointRenderer.h"

namespace MetaVisage {

PointRenderer::PointRenderer()
    : vao_(0),
      vbo_(0),
      pointCount_(0),
      initialized_(false) {
}

PointRenderer::~PointRenderer() {
    Clear();
}

void PointRenderer::Initialize() {
    if (initialized_) return;
    initializeOpenGLFunctions();
    initialized_ = true;
}

void PointRenderer::UpdatePoints(const std::vector<PointMarkerData>& points) {
    if (!initialized_) return;

    pointCount_ = static_cast<int>(points.size());

    if (pointCount_ == 0) {
        Clear();
        return;
    }

    // Interleave position(3) + color(3) per point
    std::vector<float> vertexData;
    vertexData.reserve(points.size() * 6);

    for (const auto& point : points) {
        vertexData.push_back(point.position.x);
        vertexData.push_back(point.position.y);
        vertexData.push_back(point.position.z);
        vertexData.push_back(point.color.x);
        vertexData.push_back(point.color.y);
        vertexData.push_back(point.color.z);
    }

    if (vao_ == 0) {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PointRenderer::Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, float pointSize) {
    if (vao_ == 0 || pointCount_ == 0) return;

    glUseProgram(shaderProgram);

    int vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    int sizeLoc = glGetUniformLocation(shaderProgram, "uPointSize");

    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());
    glUniform1f(sizeLoc, pointSize);

    // Enable point sprites and program point size
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Enable blending for soft edges
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing but keep depth testing so points render on top of mesh
    // but still have correct depth ordering relative to each other
    glDepthMask(GL_FALSE);

    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, pointCount_);
    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void PointRenderer::Clear() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    pointCount_ = 0;
}

} // namespace MetaVisage
