#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include "core/Mesh.h"
#include "core/Camera.h"

namespace MetaVisage {

class MeshRenderer : protected QOpenGLFunctions_4_3_Core {
public:
    MeshRenderer();
    ~MeshRenderer();

    void UploadMesh(const Mesh& mesh);
    void Render(const Camera& camera);
    void Clear();

private:
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ibo_;
    size_t indexCount_;
};

} // namespace MetaVisage

#endif // MESHRENDERER_H
