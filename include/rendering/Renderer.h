#ifndef RENDERER_H
#define RENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <memory>
#include <map>
#include <vector>
#include "core/Camera.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include "rendering/PointRenderer.h"

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

    void SetRenderFilter(RenderFilter filter) { renderFilter_ = filter; }
    RenderFilter GetRenderFilter() const { return renderFilter_; }

    // Point rendering
    void SetPointSize(float size) { pointSize_ = size; }
    float GetPointSize() const { return pointSize_; }
    void SetSelectedPointIndex(int index) { selectedPointIndex_ = index; }

    // Morph stage
    void SetMorphPreviewMode(MorphPreviewMode mode) { morphPreviewMode_ = mode; }
    MorphPreviewMode GetMorphPreviewMode() const { return morphPreviewMode_; }
    void InvalidateMesh(const Mesh* mesh);
    void UploadHeatMapColors(const Mesh* mesh, const std::vector<float>& displacements, float maxDisplacement);

private:
    void RenderGrid(const Matrix4x4& viewProjection);
    void RenderMesh(const Mesh& mesh, const Transform& transform,
                   const Vector3& color, const Matrix4x4& viewProjection);
    void RenderMeshOverlay(const Mesh& mesh, const Transform& transform,
                           const Vector3& color, float alpha, const Matrix4x4& viewProjection);
    void RenderMeshHeatMap(const Mesh& mesh, const Transform& transform,
                           const Matrix4x4& viewProjection);
    void RenderPoints(const Camera& camera, int width, int height, Project* project);
    void RenderMorphStage(const Camera& camera, int width, int height, Project* project,
                          const Matrix4x4& viewProjection);

    MeshRenderer* GetOrCreateMeshRenderer(const Mesh& mesh);

    std::unique_ptr<ShaderManager> shaderManager_;
    std::map<const Mesh*, std::unique_ptr<MeshRenderer>> meshRenderers_;
    std::unique_ptr<PointRenderer> pointRenderer_;

    bool showGrid_;
    ShadingMode shadingMode_;
    RenderFilter renderFilter_;
    MorphPreviewMode morphPreviewMode_;
    unsigned int gridVAO_;
    unsigned int gridVBO_;
    int gridVertexCount_;

    // Point rendering state
    float pointSize_;
    int selectedPointIndex_;
};

} // namespace MetaVisage

#endif // RENDERER_H
