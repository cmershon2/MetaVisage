#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <GL/glew.h>
#include <vector>
#include "core/Mesh.h"
#include "core/Camera.h"
#include "core/Transform.h"

namespace MetaVisage {

class ShaderManager;
class TextureData;

class MeshRenderer {
public:
    MeshRenderer();
    ~MeshRenderer();

    void UploadMesh(const Mesh& mesh);
    void UpdateVertexData(const Mesh& mesh);
    void Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                ShadingMode mode, const Vector3& color, const Vector3& viewPos);
    void RenderWithAlpha(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                         ShadingMode mode, const Vector3& color, float alpha, const Vector3& viewPos);
    void Clear();

    bool HasMesh() const { return vao_ != 0; }

    // Heat map support: upload per-vertex colors and render with them
    void UploadVertexColors(const std::vector<Vector3>& colors);
    void RenderHeatMap(unsigned int shaderProgram, const Matrix4x4& viewProjection,
                       const Transform& transform, ShadingMode mode, const Vector3& viewPos);

    // Texture support
    void UploadTexture(const TextureData& texture);
    void ClearTexture();
    bool HasTexture() const { return textureId_ != 0; }
    unsigned int GetTextureId() const { return textureId_; }

private:
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ibo_;
    unsigned int colorVBO_;
    unsigned int textureId_;
    GLsizei indexCount_;
    size_t vertexCount_;
    bool hasVertexColors_;

    // Quad-aware edge index buffer. Drawn with GL_LINES in wireframe modes so
    // the user sees the original face perimeters (no fan-triangulation
    // diagonals). Built from Mesh::faces_ in UploadMesh; shared with the
    // existing VBO layout.
    unsigned int edgeIBO_ = 0;
    GLsizei edgeIndexCount_ = 0;

    // GPU-to-mesh mapping for separate UV indices (UV seam handling)
    bool hasSeparateUVs_ = false;
    size_t gpuVertexCount_ = 0;
    std::vector<unsigned int> gpuToMeshVertex_;  // GPU vertex idx → mesh vertex idx (position/normal)
    std::vector<unsigned int> gpuToMeshUV_;      // GPU vertex idx → mesh UV idx
};

} // namespace MetaVisage

#endif // MESHRENDERER_H
