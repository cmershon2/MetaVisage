#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <vector>
#include "core/Mesh.h"
#include "core/Camera.h"
#include "core/Transform.h"

namespace MetaVisage {

class ShaderManager;
class TextureData;

class MeshRenderer : protected QOpenGLFunctions_4_3_Core {
public:
    MeshRenderer();
    ~MeshRenderer();

    void UploadMesh(const Mesh& mesh);
    void UpdateVertexData(const Mesh& mesh);
    void Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                ShadingMode mode, const Vector3& color);
    void RenderWithAlpha(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                         ShadingMode mode, const Vector3& color, float alpha);
    void Clear();

    bool HasMesh() const { return vao_ != 0; }

    // Heat map support: upload per-vertex colors and render with them
    void UploadVertexColors(const std::vector<Vector3>& colors);
    void RenderHeatMap(unsigned int shaderProgram, const Matrix4x4& viewProjection,
                       const Transform& transform, ShadingMode mode);

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

    // GPU-to-mesh mapping for separate UV indices (UV seam handling)
    bool hasSeparateUVs_ = false;
    size_t gpuVertexCount_ = 0;
    std::vector<unsigned int> gpuToMeshVertex_;  // GPU vertex idx → mesh vertex idx (position/normal)
    std::vector<unsigned int> gpuToMeshUV_;      // GPU vertex idx → mesh UV idx
};

} // namespace MetaVisage

#endif // MESHRENDERER_H
