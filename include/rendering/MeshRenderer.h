#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <QOpenGLFunctions_4_3_Core>
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
    void Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, const Transform& transform,
                ShadingMode mode, const Vector3& color);
    void Clear();

    bool HasMesh() const { return vao_ != 0; }

private:
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ibo_;
    size_t indexCount_;
    size_t vertexCount_;
};

} // namespace MetaVisage

#endif // MESHRENDERER_H
