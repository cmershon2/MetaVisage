#ifndef GIZMORENDERER_H
#define GIZMORENDERER_H

#include <GL/glew.h>
#include <vector>
#include <map>
#include <string>
#include "core/Types.h"
#include "core/Camera.h"

namespace MetaVisage {

// Identifies which part of a gizmo the mouse is over
enum class GizmoAxis {
    None,
    X,
    Y,
    Z,
    Center
};

// A single sub-mesh loaded from the gizmo OBJ (one per named object)
struct GizmoSubMesh {
    std::string name;           // "axis_x", "axis_y", "axis_z", "center"
    GizmoAxis axis;             // Mapped from name
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ibo = 0;
    GLsizei indexCount = 0;
    // CPU-side geometry for ray casting
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
};

// A complete gizmo (Move, Rotate, or Scale) composed of sub-meshes
struct GizmoModel {
    std::vector<GizmoSubMesh> subMeshes;
};

class GizmoRenderer {
public:
    GizmoRenderer();
    ~GizmoRenderer();

    // Load all gizmo OBJ files. Call once after GL context is ready.
    bool Initialize();

    // Render the active gizmo at the given world position
    void Render(TransformMode mode, const Vector3& position,
                GizmoAxis hoveredAxis, GizmoAxis activeAxis,
                const Camera& camera, int viewportWidth, int viewportHeight,
                unsigned int shaderProgram);

    // Hit test the gizmo. Returns which axis (if any) the screen-space ray hits.
    GizmoAxis HitTest(TransformMode mode, const Vector3& gizmoPosition,
                      const Ray& ray, const Camera& camera,
                      int viewportWidth, int viewportHeight) const;

private:
    bool LoadGizmoModel(const std::string& filepath, GizmoModel& model);
    GizmoAxis NameToAxis(const std::string& name) const;
    Vector3 GetAxisColor(GizmoAxis axis, GizmoAxis hoveredAxis, GizmoAxis activeAxis) const;

    // Compute the model matrix for constant screen-size gizmo rendering
    Matrix4x4 ComputeGizmoModelMatrix(const Vector3& position,
                                       const Camera& camera,
                                       int viewportWidth, int viewportHeight) const;

    GizmoModel moveGizmo_;
    GizmoModel rotateGizmo_;
    GizmoModel scaleGizmo_;
    bool initialized_ = false;
};

} // namespace MetaVisage

#endif // GIZMORENDERER_H
