#ifndef POINTRENDERER_H
#define POINTRENDERER_H

#include <QOpenGLFunctions_4_3_Core>
#include <vector>
#include "core/Types.h"

namespace MetaVisage {

struct PointMarkerData {
    Vector3 position;
    Vector3 color;
};

class PointRenderer : protected QOpenGLFunctions_4_3_Core {
public:
    PointRenderer();
    ~PointRenderer();

    void Initialize();

    // Upload point positions and colors to GPU
    void UpdatePoints(const std::vector<PointMarkerData>& points);

    // Render all uploaded points
    // shaderProgram: compiled point shader
    // viewProjection: combined VP matrix
    // pointSize: size in pixels
    void Render(unsigned int shaderProgram, const Matrix4x4& viewProjection, float pointSize);

    void Clear();

    int GetPointCount() const { return pointCount_; }

private:
    unsigned int vao_;
    unsigned int vbo_;
    int pointCount_;
    bool initialized_;
};

} // namespace MetaVisage

#endif // POINTRENDERER_H
