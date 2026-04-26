#include "rendering/MeshRenderer.h"
#include "core/TextureData.h"
#include <vector>
#include <map>
#include <unordered_set>
#include <cstdint>

namespace MetaVisage {

MeshRenderer::MeshRenderer()
    : vao_(0),
      vbo_(0),
      ibo_(0),
      colorVBO_(0),
      textureId_(0),
      indexCount_(0),
      vertexCount_(0),
      hasVertexColors_(false) {
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

    // Build a quad-aware edge index list — one pair of indices per face edge
    // (closing back to the first corner), so GL_LINES draws the original
    // polygon perimeters in wireframe mode without showing fan-triangulation
    // diagonals. Edges are deduplicated via a min/max canonical key.
    std::vector<unsigned int> edgeIndices;
    {
        edgeIndices.reserve(faces.size() * 8);
        std::unordered_set<uint64_t> seenEdges;
        seenEdges.reserve(faces.size() * 4);

        auto pushEdge = [&](unsigned int a, unsigned int b) {
            if (a == b) return;
            unsigned int lo = a < b ? a : b;
            unsigned int hi = a < b ? b : a;
            uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
            if (!seenEdges.insert(key).second) return;
            edgeIndices.push_back(a);
            edgeIndices.push_back(b);
        };

        if (hasSeparateUVs_) {
            // Recover GPU index for each (vertexIdx, uvIdx) corner using the
            // same cornerMap layout that built the triangle buffer.
            std::map<std::pair<unsigned int, unsigned int>, unsigned int> cornerMap;
            for (size_t gi = 0; gi < gpuToMeshVertex_.size(); ++gi) {
                cornerMap[{gpuToMeshVertex_[gi], gpuToMeshUV_[gi]}] =
                    static_cast<unsigned int>(gi);
            }
            for (const auto& face : faces) {
                const size_t n = face.vertexIndices.size();
                if (n < 2 || face.uvIndices.size() != n) continue;
                for (size_t i = 0; i < n; ++i) {
                    size_t j = (i + 1) % n;
                    auto itA = cornerMap.find({face.vertexIndices[i], face.uvIndices[i]});
                    auto itB = cornerMap.find({face.vertexIndices[j], face.uvIndices[j]});
                    if (itA == cornerMap.end() || itB == cornerMap.end()) continue;
                    pushEdge(itA->second, itB->second);
                }
            }
        } else {
            for (const auto& face : faces) {
                const size_t n = face.vertexIndices.size();
                if (n < 2) continue;
                for (size_t i = 0; i < n; ++i) {
                    size_t j = (i + 1) % n;
                    pushEdge(face.vertexIndices[i], face.vertexIndices[j]);
                }
            }
        }
    }
    edgeIndexCount_ = static_cast<GLsizei>(edgeIndices.size());

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

    // Edge IBO for quad-aware wireframe rendering. Upload it WITHOUT the
    // VAO bound — otherwise the GL_ELEMENT_ARRAY_BUFFER binding would be
    // recorded into the VAO and clobber the triangle ibo_ that solid mode
    // relies on. Wireframe paths bind/restore edgeIBO_ explicitly per draw.
    glBindVertexArray(0);

    if (!edgeIndices.empty()) {
        glGenBuffers(1, &edgeIBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     edgeIndices.size() * sizeof(unsigned int),
                     edgeIndices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void MeshRenderer::Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                          ShadingMode mode, const Vector3& color, const Vector3& viewPos) {
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
    // Light direction follows the camera: illuminate from the viewer's position
    Vector3 lightDir = viewPos.Normalized();
    glUniform3f(lightDirLoc, lightDir.x, lightDir.y, lightDir.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);

    glBindVertexArray(vao_);

    // Render based on shading mode
    switch (mode) {
        case ShadingMode::Solid:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            break;

        case ShadingMode::Wireframe:
            // Quad-aware wireframe: draw face perimeters via the edge buffer
            // with GL_LINES so quads/n-gons display as quads (no fan
            // diagonals). Falls back to triangulated outline if edges weren't
            // built — should never happen for a valid mesh.
            if (edgeIBO_ != 0 && edgeIndexCount_ > 0) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO_);
                glDrawElements(GL_LINES, edgeIndexCount_, GL_UNSIGNED_INT, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;

        case ShadingMode::SolidWireframe:
            // Solid first
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            // Quad-aware wireframe overlay
            glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
            if (edgeIBO_ != 0 && edgeIndexCount_ > 0) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO_);
                glDrawElements(GL_LINES, edgeIndexCount_, GL_UNSIGNED_INT, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;

        case ShadingMode::Textured:
            if (textureId_ != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId_);
                int texLoc = glGetUniformLocation(shaderProgram, "uAlbedoTex");
                if (texLoc >= 0) glUniform1i(texLoc, 0);
                int hasTexLoc = glGetUniformLocation(shaderProgram, "uHasTexture");
                if (hasTexLoc >= 0) glUniform1i(hasTexLoc, 1);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            if (textureId_ != 0) {
                glBindTexture(GL_TEXTURE_2D, 0);
                int hasTexLoc2 = glGetUniformLocation(shaderProgram, "uHasTexture");
                if (hasTexLoc2 >= 0) glUniform1i(hasTexLoc2, 0);
            }
            break;
    }

    glBindVertexArray(0);
}

void MeshRenderer::RenderWithAlpha(unsigned int shaderProgram, const Matrix4x4& viewProjection,
                                    const Transform& transform, ShadingMode /*mode*/,
                                    const Vector3& color, float alpha, const Vector3& viewPos) {
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
    Vector3 lightDirAlpha = viewPos.Normalized();
    glUniform3f(lightDirLoc, lightDirAlpha.x, lightDirAlpha.y, lightDirAlpha.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);
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
                                  const Transform& transform, ShadingMode mode, const Vector3& viewPos) {
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

    Vector3 lightDirHeat = viewPos.Normalized();
    glUniform3f(lightDirLoc, lightDirHeat.x, lightDirHeat.y, lightDirHeat.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);

    glBindVertexArray(vao_);

    switch (mode) {
        case ShadingMode::Solid:
        case ShadingMode::Textured:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            break;

        case ShadingMode::Wireframe:
            if (edgeIBO_ != 0 && edgeIndexCount_ > 0) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO_);
                glDrawElements(GL_LINES, edgeIndexCount_, GL_UNSIGNED_INT, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;

        case ShadingMode::SolidWireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
            // Quad-aware wireframe overlay
            if (edgeIBO_ != 0 && edgeIndexCount_ > 0) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO_);
                glDrawElements(GL_LINES, edgeIndexCount_, GL_UNSIGNED_INT, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;
    }

    glBindVertexArray(0);
}

void MeshRenderer::UploadTexture(const TextureData& texture) {
    ClearTexture();

    if (!texture.IsLoaded()) return;

    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 texture.GetWidth(), texture.GetHeight(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texture.GetPixels().data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void MeshRenderer::ClearTexture() {
    if (textureId_ != 0) {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }
}

void MeshRenderer::Clear() {
    ClearTexture();
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
    if (edgeIBO_ != 0) {
        glDeleteBuffers(1, &edgeIBO_);
        edgeIBO_ = 0;
    }
    edgeIndexCount_ = 0;
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
