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

    // Mask visualization (Morph stage)
    void UploadMaskColors(const Mesh* mesh, const std::vector<bool>& mask);
    void SetShowMask(bool show) { showMask_ = show; }
    bool GetShowMask() const { return showMask_; }

    // Sculpting (Touch Up stage)
    void SetBrushCursor(const Vector3& position, const Vector3& normal, float radius, float innerRadius, bool visible);
    void UpdateMeshVertices(const Mesh* mesh);
    void SetShowTargetOverlay(bool show) { showTargetOverlay_ = show; }

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
    void RenderTouchUpStage(const Camera& camera, int width, int height, Project* project,
                            const Matrix4x4& viewProjection);
    void RenderBrushCursor(const Matrix4x4& viewProjection);

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

    // Brush cursor state (parameters stored from SetBrushCursor, geometry built in RenderBrushCursor)
    unsigned int brushCursorVAO_;
    unsigned int brushCursorVBO_;
    bool brushCursorVisible_;
    int brushCursorSegments_;
    bool brushCursorDirty_;       // True when parameters changed and geometry needs rebuild
    Vector3 brushCursorPosition_;
    Vector3 brushCursorNormal_;
    float brushCursorRadius_;
    float brushCursorInnerRadius_;

    // Target overlay in Touch Up stage
    bool showTargetOverlay_;

    // Mask visualization
    bool showMask_;
};

} // namespace MetaVisage

#endif // RENDERER_H
