#ifndef TEXTUREBAKER_H
#define TEXTUREBAKER_H

#include "core/Types.h"
#include "core/Transform.h"
#include "core/TextureData.h"
#include <functional>
#include <memory>

namespace MetaVisage {

class Mesh;
class BVH;

struct BakeSettings {
    int resolution;        // Output texture resolution (e.g. 1024, 2048, 4096)
    int paddingPixels;     // Seam bleeding radius
    bool bakeAlbedo;
    bool bakeNormalMap;
    float maxRayDistance;   // Max distance for ray tracing (0 = unlimited)

    BakeSettings()
        : resolution(2048), paddingPixels(4),
          bakeAlbedo(true), bakeNormalMap(false),
          maxRayDistance(0.0f) {}
};

struct BakeResult {
    bool success;
    QString errorMessage;
    std::shared_ptr<TextureData> bakedAlbedo;
    std::shared_ptr<TextureData> bakedNormalMap;

    BakeResult() : success(false) {}
};

class TextureBaker {
public:
    TextureBaker();
    ~TextureBaker();

    // Bake textures from target mesh UV space to morph mesh UV space.
    // morphMesh: the deformed morph mesh (MetaHuman UVs)
    // targetMesh: the custom head mesh (source texture UVs)
    // morphTransform: transform applied to morph mesh (for coordinate space conversion)
    // targetTransform: transform applied to target mesh (for coordinate space conversion)
    // targetAlbedo: albedo texture to remap (may be null if !settings.bakeAlbedo)
    // targetNormalMap: normal map to remap (may be null if !settings.bakeNormalMap)
    // progressCallback: called with [0,1] progress
    BakeResult Bake(
        const Mesh& morphMesh,
        const Mesh& targetMesh,
        const Transform& morphTransform,
        const Transform& targetTransform,
        const TextureData* targetAlbedo,
        const TextureData* targetNormalMap,
        const BakeSettings& settings,
        std::function<void(float)> progressCallback = nullptr
    );

private:
    // Transform a point by a 4x4 matrix (position, w=1)
    static Vector3 TransformPoint(const Matrix4x4& mat, const Vector3& p);

    // Transform a direction by a 4x4 matrix (direction, w=0, no translation)
    static Vector3 TransformDirection(const Matrix4x4& mat, const Vector3& d);

    // Compute barycentric coordinates of point P in triangle (A, B, C) in 3D
    static Vector3 ComputeBarycentric3D(const Vector3& p,
                                         const Vector3& a, const Vector3& b, const Vector3& c);

    // Compute barycentric coordinates of point P in triangle (A, B, C) in 2D
    static Vector3 ComputeBarycentric2D(const Vector2& p,
                                         const Vector2& a, const Vector2& b, const Vector2& c);

    // Rasterize a triangle in UV space, calling visitor for each covered texel.
    // visitor(px, py, bary0, bary1, bary2) where bary are barycentric weights for uv0, uv1, uv2
    static void RasterizeTriangleUV(
        const Vector2& uv0, const Vector2& uv1, const Vector2& uv2,
        int resolution,
        const std::function<void(int, int, float, float, float)>& visitor
    );

    // Apply seam bleeding (dilation) to fill uncovered pixels near UV island edges
    static void ApplySeamBleeding(std::vector<uint8_t>& pixels, std::vector<bool>& coverage,
                                   int width, int height, int paddingPixels);

    // For a given triangle (by vertex indices i0,i1,i2), find the face and corner indices
    // to look up UVs from face.uvIndices
    static bool FindFaceForTriangle(const Mesh& mesh, unsigned int i0, unsigned int i1, unsigned int i2,
                                     int& outFaceIdx, int& outCorner0, int& outCorner1, int& outCorner2);

    // Look up target UV at a hit point given triangle info
    static Vector2 InterpolateTargetUV(
        const std::vector<Face>& targetFaces,
        const std::vector<Vector2>& targetUVs,
        bool targetHasSeparateUVs,
        int faceIndex, int corner0, int corner1, int corner2,
        unsigned int i0, unsigned int i1, unsigned int i2,
        const Vector3& barycentrics);
};

} // namespace MetaVisage

#endif // TEXTUREBAKER_H
