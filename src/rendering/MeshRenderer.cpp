#include "rendering/MeshRenderer.h"
#include <vector>
#include <map>

namespace MetaVisage {

MeshRenderer::MeshRenderer()
    : vao_(0),
      vbo_(0),
      ibo_(0),
      colorVBO_(0),
      indexCount_(0),
      vertexCount_(0),
      hasVertexColors_(false) {
    initializeOpenGLFunctions();
}

MeshRenderer::~MeshRenderer() {
    Clear();
}

void MeshRenderer::UploadMesh(const Mesh& mesh) {
    Clear();

    const auto& vertices = mesh.GetVertices();
    const auto& normals = mesh.GetNormals();
    const auto& uvs = mesh.GetUVs();
    const auto& faces = mesh.GetFaces();

    if (vertices.empty() || faces.empty()) {
        return;
    }

    hasSeparateUVs_ = mesh.HasSeparateUVIndices();

    std::vector<float> vertexData;
    std::vector<unsigned int> indices;

    if (hasSeparateUVs_) {
        // Separate UV index path: build GPU vertices from unique (vertexIdx, uvIdx) pairs
        // This re-splits at UV seams for the GPU while the mesh data stays merged
        gpuToMeshVertex_.clear();
        gpuToMeshUV_.clear();
        std::map<std::pair<unsigned int, unsigned int>, unsigned int> cornerMap;

        for (const auto& face : faces) {
            if (face.vertexIndices.size() < 3 || face.uvIndices.size() != face.vertexIndices.size()) continue;

            // Fan triangulation
            for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
                size_t corners[] = {0, i, i + 1};
                for (size_t c : corners) {
                    unsigned int vi = face.vertexIndices[c];
                    unsigned int ti = face.uvIndices[c];
                    auto key = std::make_pair(vi, ti);

                    auto it = cornerMap.find(key);
                    if (it != cornerMap.end()) {
                        indices.push_back(it->second);
                    } else {
                        unsigned int gpuIdx = static_cast<unsigned int>(gpuToMeshVertex_.size());
                        cornerMap[key] = gpuIdx;
                        gpuToMeshVertex_.push_back(vi);
                        gpuToMeshUV_.push_back(ti);

                        vertexData.push_back(vertices[vi].x);
                        vertexData.push_back(vertices[vi].y);
                        vertexData.push_back(vertices[vi].z);

                        if (vi < normals.size()) {
                            vertexData.push_back(normals[vi].x);
                            vertexData.push_back(normals[vi].y);
                            vertexData.push_back(normals[vi].z);
                        } else {
                            vertexData.push_back(0.0f);
                            vertexData.push_back(1.0f);
                            vertexData.push_back(0.0f);
                        }

                        if (ti < uvs.size()) {
                            vertexData.push_back(uvs[ti].x);
                            vertexData.push_back(uvs[ti].y);
                        } else {
                            vertexData.push_back(0.0f);
                            vertexData.push_back(0.0f);
                        }

                        indices.push_back(gpuIdx);
                    }
                }
            }
        }

        gpuVertexCount_ = gpuToMeshVertex_.size();
        vertexCount_ = gpuVertexCount_;
    } else {
        // Original path: vertices and UVs share the same index
        vertexCount_ = vertices.size();
        vertexData.reserve(vertices.size() * 8);

        for (size_t i = 0; i < vertices.size(); ++i) {
            vertexData.push_back(vertices[i].x);
            vertexData.push_back(vertices[i].y);
            vertexData.push_back(vertices[i].z);

            if (i < normals.size()) {
                vertexData.push_back(normals[i].x);
                vertexData.push_back(normals[i].y);
                vertexData.push_back(normals[i].z);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);
            }

            if (i < uvs.size()) {
                vertexData.push_back(uvs[i].x);
                vertexData.push_back(uvs[i].y);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }

        for (const auto& face : faces) {
            if (face.vertexIndices.size() >= 3) {
                for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
                    indices.push_back(face.vertexIndices[0]);
                    indices.push_back(face.vertexIndices[i]);
                    indices.push_back(face.vertexIndices[i + 1]);
                }
            }
        }
    }

    indexCount_ = static_cast<GLsizei>(indices.size());

    // Create VAO
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Create VBO
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Create IBO
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void MeshRenderer::Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                          ShadingMode mode, const Vector3& color) {
    if (vao_ == 0 || indexCount_ == 0) return;

    glUseProgram(shaderProgram);

    // Set uniforms
    Matrix4x4 model = transform.GetMatrix();
    int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    int vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    int normalMatrixLoc = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    int colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "uLightDir");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "uLightColor");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.Data());
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    // Normal matrix is the transpose of the inverse of the model matrix
    Matrix4x4 normalMatrix = model.Inverse().Transpose();
    glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, normalMatrix.Data());

    glUniform3f(colorLoc, color.x, color.y, color.z);
    glUniform3f(lightDirLoc, 0.3f, -0.5f, 0.8f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, 0.0f, 0.0f, 5.0f);

    glBindVertexArray(vao_);

    // Render based on shading mode
    switch (mode) {
        case ShadingMode::Solid:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            break;

        case ShadingMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case ShadingMode::SolidWireframe:
            // Solid first
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            // Wireframe overlay
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case ShadingMode::Textured:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            break;
    }

    glBindVertexArray(0);
}

void MeshRenderer::RenderWithAlpha(unsigned int shaderProgram, const Matrix4x4& viewProjection,
                                    const Transform& transform, ShadingMode /*mode*/,
                                    const Vector3& color, float alpha) {
    if (vao_ == 0 || indexCount_ == 0) return;

    glUseProgram(shaderProgram);

    Matrix4x4 model = transform.GetMatrix();
    int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    int vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    int normalMatrixLoc = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    int colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "uLightDir");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "uLightColor");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");
    int alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.Data());
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    Matrix4x4 normalMatrix = model.Inverse().Transpose();
    glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, normalMatrix.Data());

    glUniform3f(colorLoc, color.x, color.y, color.z);
    glUniform3f(lightDirLoc, 0.3f, -0.5f, 0.8f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, 0.0f, 0.0f, 5.0f);
    if (alphaLoc >= 0) {
        glUniform1f(alphaLoc, alpha);
    }

    // Enable alpha blending, disable depth test so overlay renders on top
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(vao_);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void MeshRenderer::UpdateVertexData(const Mesh& mesh) {
    if (vao_ == 0 || vbo_ == 0) return;

    const auto& vertices = mesh.GetVertices();
    const auto& normals = mesh.GetNormals();
    const auto& uvs = mesh.GetUVs();

    if (vertices.empty()) return;

    std::vector<float> vertexData;

    if (hasSeparateUVs_ && !gpuToMeshVertex_.empty()) {
        // Separate UV path: use stored mapping to rebuild GPU buffer
        vertexData.reserve(gpuVertexCount_ * 8);

        for (size_t i = 0; i < gpuVertexCount_; ++i) {
            unsigned int vi = gpuToMeshVertex_[i];
            unsigned int ti = gpuToMeshUV_[i];

            vertexData.push_back(vertices[vi].x);
            vertexData.push_back(vertices[vi].y);
            vertexData.push_back(vertices[vi].z);

            if (vi < normals.size()) {
                vertexData.push_back(normals[vi].x);
                vertexData.push_back(normals[vi].y);
                vertexData.push_back(normals[vi].z);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);
            }

            if (ti < uvs.size()) {
                vertexData.push_back(uvs[ti].x);
                vertexData.push_back(uvs[ti].y);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }
    } else {
        // Original path: vertices and UVs share same index
        vertexData.reserve(vertices.size() * 8);

        for (size_t i = 0; i < vertices.size(); ++i) {
            vertexData.push_back(vertices[i].x);
            vertexData.push_back(vertices[i].y);
            vertexData.push_back(vertices[i].z);

            if (i < normals.size()) {
                vertexData.push_back(normals[i].x);
                vertexData.push_back(normals[i].y);
                vertexData.push_back(normals[i].z);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);
            }

            if (i < uvs.size()) {
                vertexData.push_back(uvs[i].x);
                vertexData.push_back(uvs[i].y);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }
    }

    // Re-upload vertex data to existing VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MeshRenderer::UploadVertexColors(const std::vector<Vector3>& colors) {
    if (vao_ == 0) return;

    // Pack color data, mapping mesh vertex colors to GPU vertices
    std::vector<float> colorData;

    if (hasSeparateUVs_ && !gpuToMeshVertex_.empty()) {
        // Map mesh vertex colors to GPU vertices using stored mapping
        colorData.reserve(gpuVertexCount_ * 3);
        for (size_t i = 0; i < gpuVertexCount_; ++i) {
            unsigned int vi = gpuToMeshVertex_[i];
            if (vi < colors.size()) {
                colorData.push_back(colors[vi].x);
                colorData.push_back(colors[vi].y);
                colorData.push_back(colors[vi].z);
            } else {
                colorData.push_back(0.0f);
                colorData.push_back(0.0f);
                colorData.push_back(0.0f);
            }
        }
    } else {
        if (colors.size() != vertexCount_) return;
        colorData.reserve(colors.size() * 3);
        for (const auto& c : colors) {
            colorData.push_back(c.x);
            colorData.push_back(c.y);
            colorData.push_back(c.z);
        }
    }

    glBindVertexArray(vao_);

    if (colorVBO_ == 0) {
        glGenBuffers(1, &colorVBO_);
    }

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO_);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(float), colorData.data(), GL_DYNAMIC_DRAW);

    // Vertex color attribute at location 3
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    hasVertexColors_ = true;
}

void MeshRenderer::RenderHeatMap(unsigned int shaderProgram, const Matrix4x4& viewProjection,
                                  const Transform& transform, ShadingMode mode) {
    if (vao_ == 0 || indexCount_ == 0 || !hasVertexColors_) return;

    glUseProgram(shaderProgram);

    Matrix4x4 model = transform.GetMatrix();
    int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    int vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    int normalMatrixLoc = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "uLightDir");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "uLightColor");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.Data());
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, viewProjection.Data());

    Matrix4x4 normalMatrix = model.Inverse().Transpose();
    glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, normalMatrix.Data());

    glUniform3f(lightDirLoc, 0.3f, -0.5f, 0.8f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, 0.0f, 0.0f, 5.0f);

    glBindVertexArray(vao_);

    switch (mode) {
        case ShadingMode::Solid:
        case ShadingMode::Textured:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            break;

        case ShadingMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case ShadingMode::SolidWireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            // Wireframe overlay in dark color
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // Use a default dark vertex color for wireframe - just draw lines
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }

    glBindVertexArray(0);
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
    if (colorVBO_ != 0) {
        glDeleteBuffers(1, &colorVBO_);
        colorVBO_ = 0;
    }
    indexCount_ = 0;
    vertexCount_ = 0;
    hasVertexColors_ = false;
    hasSeparateUVs_ = false;
    gpuVertexCount_ = 0;
    gpuToMeshVertex_.clear();
    gpuToMeshUV_.clear();
}

} // namespace MetaVisage
