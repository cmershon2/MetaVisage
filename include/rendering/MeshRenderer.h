#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <vector>
#include "core/Mesh.h"
#include "core/Camera.h"
#include "core/Transform.h"

namespace MetaVisage {

class ShaderManager;

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

private:
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ibo_;
    unsigned int colorVBO_;
    size_t indexCount_;
    size_t vertexCount_;
    bool hasVertexColors_;
};

} // namespace MetaVisage

#endif // MESHRENDERER_H
