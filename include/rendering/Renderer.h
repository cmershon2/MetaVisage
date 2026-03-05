#ifndef RENDERER_H
#define RENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <memory>
#include <map>
#include "core/Camera.h"
#include "core/Mesh.h"
#include "core/Transform.h"

namespace MetaVisage {

class ShaderManager;
class MeshRenderer;
class Project;

class Renderer : protected QOpenGLFunctions_4_3_Core {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Render(const Camera& camera, int width, int height, Project* project);

    void SetShowGrid(bool show) { showGrid_ = show; }
    bool GetShowGrid() const { return showGrid_; }

    void SetShadingMode(ShadingMode mode) { shadingMode_ = mode; }
    ShadingMode GetShadingMode() const { return shadingMode_; }

private:
    void RenderGrid(const Matrix4x4& viewProjection);
    void RenderMesh(const Mesh& mesh, const Transform& transform,
                   const Vector3& color, const Matrix4x4& viewProjection);

    std::unique_ptr<ShaderManager> shaderManager_;
    std::map<const Mesh*, std::unique_ptr<MeshRenderer>> meshRenderers_;

    bool showGrid_;
    ShadingMode shadingMode_;
    unsigned int gridVAO_;
    unsigned int gridVBO_;
    int gridVertexCount_;
};

} // namespace MetaVisage

#endif // RENDERER_H
