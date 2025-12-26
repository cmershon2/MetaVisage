#ifndef RENDERER_H
#define RENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <memory>
#include "core/Camera.h"
#include "core/Mesh.h"

namespace MetaVisage {

class ShaderManager;
class MeshRenderer;

class Renderer : protected QOpenGLFunctions_4_3_Core {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Render(const Camera& camera, int width, int height);

    void SetShowGrid(bool show) { showGrid_ = show; }
    bool GetShowGrid() const { return showGrid_; }

private:
    void RenderGrid(const Matrix4x4& viewProjection);

    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<MeshRenderer> meshRenderer_;

    bool showGrid_;
    unsigned int gridVAO_;
    unsigned int gridVBO_;
};

} // namespace MetaVisage

#endif // RENDERER_H
