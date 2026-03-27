#include "baking/TextureBaker.h"
#include "core/Mesh.h"
#include "utils/BVH.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace MetaVisage {

TextureBaker::TextureBaker() {}
TextureBaker::~TextureBaker() {}

Vector3 TextureBaker::TransformPoint(const Matrix4x4& mat, const Vector3& p) {
    // Column-major 4x4 matrix * (p.x, p.y, p.z, 1)
    float x = mat.m[0] * p.x + mat.m[4] * p.y + mat.m[8]  * p.z + mat.m[12];
    float y = mat.m[1] * p.x + mat.m[5] * p.y + mat.m[9]  * p.z + mat.m[13];
    float z = mat.m[2] * p.x + mat.m[6] * p.y + mat.m[10] * p.z + mat.m[14];
    return Vector3(x, y, z);
}

Vector3 TextureBaker::TransformDirection(const Matrix4x4& mat, const Vector3& d) {
    // Column-major 4x4 matrix * (d.x, d.y, d.z, 0) — no translation
    float x = mat.m[0] * d.x + mat.m[4] * d.y + mat.m[8]  * d.z;
    float y = mat.m[1] * d.x + mat.m[5] * d.y + mat.m[9]  * d.z;
    float z = mat.m[2] * d.x + mat.m[6] * d.y + mat.m[10] * d.z;
    return Vector3(x, y, z);
}

Vector3 TextureBaker::ComputeBarycentric3D(const Vector3& p,
                                            const Vector3& a, const Vector3& b, const Vector3& c) {
    Vector3 v0 = b - a;
    Vector3 v1 = c - a;
    Vector3 v2 = p - a;
    float d00 = v0.Dot(v0);
    float d01 = v0.Dot(v1);
    float d11 = v1.Dot(v1);
    float d20 = v2.Dot(v0);
    float d21 = v2.Dot(v1);
    float denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-12f) {
        return Vector3(1.0f, 0.0f, 0.0f); // Degenerate triangle
    }
    float invDenom = 1.0f / denom;
    float bary1 = (d11 * d20 - d01 * d21) * invDenom;
    float bary2 = (d00 * d21 - d01 * d20) * invDenom;
    float bary0 = 1.0f - bary1 - bary2;
    return Vector3(bary0, bary1, bary2);
}

Vector3 TextureBaker::ComputeBarycentric2D(const Vector2& p,
                                            const Vector2& a, const Vector2& b, const Vector2& c) {
    float denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    if (std::abs(denom) < 1e-12f) {
        return Vector3(1.0f, 0.0f, 0.0f); // Degenerate
    }
    float invDenom = 1.0f / denom;
    float bary0 = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) * invDenom;
    float bary1 = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) * invDenom;
    float bary2 = 1.0f - bary0 - bary1;
    return Vector3(bary0, bary1, bary2);
}

void TextureBaker::RasterizeTriangleUV(
    const Vector2& uv0, const Vector2& uv1, const Vector2& uv2,
    int resolution,
    const std::function<void(int, int, float, float, float)>& visitor) {

    float res = static_cast<float>(resolution);

    // Convert UVs to pixel coordinates
    float px0 = uv0.x * res, py0 = uv0.y * res;
    float px1 = uv1.x * res, py1 = uv1.y * res;
    float px2 = uv2.x * res, py2 = uv2.y * res;

    // Bounding box in pixel space
    int minX = std::max(0, static_cast<int>(std::floor(std::min({px0, px1, px2}))));
    int maxX = std::min(resolution - 1, static_cast<int>(std::ceil(std::max({px0, px1, px2}))));
    int minY = std::max(0, static_cast<int>(std::floor(std::min({py0, py1, py2}))));
    int maxY = std::min(resolution - 1, static_cast<int>(std::ceil(std::max({py0, py1, py2}))));

    // Precompute barycentric denominator
    float denom = (py1 - py2) * (px0 - px2) + (px2 - px1) * (py0 - py2);
    if (std::abs(denom) < 1e-8f) return; // Degenerate triangle
    float invDenom = 1.0f / denom;

    for (int y = minY; y <= maxY; ++y) {
        float cy = static_cast<float>(y) + 0.5f; // Pixel center
        for (int x = minX; x <= maxX; ++x) {
            float cx = static_cast<float>(x) + 0.5f;

            // Barycentric coordinates
            float b0 = ((py1 - py2) * (cx - px2) + (px2 - px1) * (cy - py2)) * invDenom;
            float b1 = ((py2 - py0) * (cx - px2) + (px0 - px2) * (cy - py2)) * invDenom;
            float b2 = 1.0f - b0 - b1;

            // Check if pixel center is inside triangle (with small epsilon for edge coverage)
            const float eps = -1e-4f;
            if (b0 >= eps && b1 >= eps && b2 >= eps) {
                visitor(x, y, b0, b1, b2);
            }
        }
    }
}

bool TextureBaker::FindFaceForTriangle(const Mesh& mesh,
                                        unsigned int i0, unsigned int i1, unsigned int i2,
                                        int& outFaceIdx, int& outCorner0, int& outCorner1, int& outCorner2) {
    const auto& faces = mesh.GetFaces();
    for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
        const auto& face = faces[fi];
        const auto& vi = face.vertexIndices;
        if (vi.size() < 3) continue;

        // Fan triangulation: triangle (vi[0], vi[k], vi[k+1]) for k=1..n-2
        for (size_t k = 1; k + 1 < vi.size(); ++k) {
            if (vi[0] == i0 && vi[k] == i1 && vi[k + 1] == i2) {
                outFaceIdx = fi;
                outCorner0 = 0;
                outCorner1 = static_cast<int>(k);
                outCorner2 = static_cast<int>(k + 1);
                return true;
            }
        }
    }
    return false;
}

Vector2 TextureBaker::InterpolateTargetUV(
    const std::vector<Face>& targetFaces,
    const std::vector<Vector2>& targetUVs,
    bool targetHasSeparateUVs,
    int faceIndex, int corner0, int corner1, int corner2,
    unsigned int i0, unsigned int i1, unsigned int i2,
    const Vector3& bary) {

    Vector2 tuv0, tuv1, tuv2;

    if (faceIndex >= 0 && faceIndex < static_cast<int>(targetFaces.size())) {
        const auto& face = targetFaces[faceIndex];
        if (targetHasSeparateUVs && !face.uvIndices.empty() &&
            corner0 < static_cast<int>(face.uvIndices.size()) &&
            corner1 < static_cast<int>(face.uvIndices.size()) &&
            corner2 < static_cast<int>(face.uvIndices.size())) {
            unsigned int ui0 = face.uvIndices[corner0];
            unsigned int ui1 = face.uvIndices[corner1];
            unsigned int ui2 = face.uvIndices[corner2];
            if (ui0 < targetUVs.size()) tuv0 = targetUVs[ui0];
            if (ui1 < targetUVs.size()) tuv1 = targetUVs[ui1];
            if (ui2 < targetUVs.size()) tuv2 = targetUVs[ui2];
        } else {
            if (i0 < targetUVs.size()) tuv0 = targetUVs[i0];
            if (i1 < targetUVs.size()) tuv1 = targetUVs[i1];
            if (i2 < targetUVs.size()) tuv2 = targetUVs[i2];
        }
    } else {
        // Fallback: use vertex indices directly
        if (i0 < targetUVs.size()) tuv0 = targetUVs[i0];
        if (i1 < targetUVs.size()) tuv1 = targetUVs[i1];
        if (i2 < targetUVs.size()) tuv2 = targetUVs[i2];
    }

    float u = bary.x * tuv0.x + bary.y * tuv1.x + bary.z * tuv2.x;
    float v = bary.x * tuv0.y + bary.y * tuv1.y + bary.z * tuv2.y;
    return Vector2(u, v);
}

void TextureBaker::ApplySeamBleeding(std::vector<uint8_t>& pixels, std::vector<bool>& coverage,
                                      int width, int height, int paddingPixels) {
    // Simple dilation: for each iteration, expand covered pixels into uncovered neighbors
    for (int iter = 0; iter < paddingPixels; ++iter) {
        std::vector<bool> newCoverage = coverage;
        std::vector<uint8_t> newPixels = pixels;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                if (coverage[idx]) continue; // Already covered

                // Check 4-connected neighbors
                int count = 0;
                int rSum = 0, gSum = 0, bSum = 0, aSum = 0;

                auto sample = [&](int nx, int ny) {
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) return;
                    int ni = ny * width + nx;
                    if (!coverage[ni]) return;
                    rSum += pixels[ni * 4 + 0];
                    gSum += pixels[ni * 4 + 1];
                    bSum += pixels[ni * 4 + 2];
                    aSum += pixels[ni * 4 + 3];
                    count++;
                };

                sample(x - 1, y);
                sample(x + 1, y);
                sample(x, y - 1);
                sample(x, y + 1);

                if (count > 0) {
                    newPixels[idx * 4 + 0] = static_cast<uint8_t>(rSum / count);
                    newPixels[idx * 4 + 1] = static_cast<uint8_t>(gSum / count);
                    newPixels[idx * 4 + 2] = static_cast<uint8_t>(bSum / count);
                    newPixels[idx * 4 + 3] = static_cast<uint8_t>(aSum / count);
                    newCoverage[idx] = true;
                }
            }
        }

        pixels = std::move(newPixels);
        coverage = std::move(newCoverage);
    }
}

BakeResult TextureBaker::Bake(
    const Mesh& morphMesh,
    const Mesh& targetMesh,
    const Transform& morphTransform,
    const Transform& targetTransform,
    const TextureData* targetAlbedo,
    const TextureData* targetNormalMap,
    const BakeSettings& settings,
    std::function<void(float)> progressCallback) {

    BakeResult result;
    int res = settings.resolution;

    // Validate inputs
    if (morphMesh.GetVertices().empty() || morphMesh.GetFaces().empty()) {
        result.errorMessage = "Morph mesh has no geometry";
        return result;
    }
    if (targetMesh.GetVertices().empty() || targetMesh.GetFaces().empty()) {
        result.errorMessage = "Target mesh has no geometry";
        return result;
    }
    if (settings.bakeAlbedo && (!targetAlbedo || !targetAlbedo->IsLoaded())) {
        result.errorMessage = "No albedo texture provided for baking";
        return result;
    }
    if (morphMesh.GetUVs().empty()) {
        result.errorMessage = "Morph mesh has no UV coordinates";
        return result;
    }
    if (targetMesh.GetUVs().empty()) {
        result.errorMessage = "Target mesh has no UV coordinates";
        return result;
    }

    MV_LOG_INFO(QString("Starting texture bake: %1x%1, albedo=%2, normal=%3")
        .arg(res).arg(settings.bakeAlbedo).arg(settings.bakeNormalMap));

    // Compute the morph-to-target coordinate transform:
    //   morphLocal -> world (via morphTransform) -> targetLocal (via inverse targetTransform)
    // This transforms points from the deformed morph mesh's local space into
    // the target mesh's local space, so BVH queries work correctly.
    Matrix4x4 morphToWorld = morphTransform.GetMatrix();
    Matrix4x4 worldToTarget = targetTransform.GetMatrix().Inverse();
    Matrix4x4 morphToTarget = worldToTarget * morphToWorld;

    MV_LOG_INFO("Coordinate transform computed: morphLocal -> world -> targetLocal");

    // Ensure target mesh BVH is built
    BVH* targetBVH = targetMesh.GetBVH();
    if (!targetBVH || !targetBVH->IsBuilt()) {
        result.errorMessage = "Failed to build BVH for target mesh";
        return result;
    }

    const auto& morphVertices = morphMesh.GetVertices();
    const auto& morphNormals = morphMesh.GetNormals();
    const auto& morphUVs = morphMesh.GetUVs();
    const auto& morphFaces = morphMesh.GetFaces();
    const auto& targetVertices = targetMesh.GetVertices();
    const auto& targetUVs = targetMesh.GetUVs();
    const auto& targetFaces = targetMesh.GetFaces();

    bool hasMorphNormals = !morphNormals.empty();

    // Build target triangle-to-face map (matching BVH::Build's fan triangulation order)
    struct TriFaceInfo {
        int faceIndex;
        int corner0, corner1, corner2; // Corners within the face for UV lookup
    };

    std::unordered_map<int, TriFaceInfo> targetTriToFace;
    {
        int triIdx = 0;
        for (int fi = 0; fi < static_cast<int>(targetFaces.size()); ++fi) {
            const auto& face = targetFaces[fi];
            if (face.vertexIndices.size() < 3) continue;
            for (size_t k = 1; k + 1 < face.vertexIndices.size(); ++k) {
                TriFaceInfo info;
                info.faceIndex = fi;
                info.corner0 = 0;
                info.corner1 = static_cast<int>(k);
                info.corner2 = static_cast<int>(k + 1);
                targetTriToFace[triIdx] = info;
                triIdx++;
            }
        }
    }

    // Allocate output textures
    std::shared_ptr<TextureData> bakedAlbedo;
    std::vector<bool> albedoCoverage;
    if (settings.bakeAlbedo) {
        bakedAlbedo = TextureData::Create(res, res);
        albedoCoverage.resize(res * res, false);
    }

    std::shared_ptr<TextureData> bakedNormal;
    std::vector<bool> normalCoverage;
    if (settings.bakeNormalMap && targetNormalMap && targetNormalMap->IsLoaded()) {
        bakedNormal = TextureData::Create(res, res);
        normalCoverage.resize(res * res, false);
    }

    // Build morph mesh triangle info for UV lookup
    struct MorphTriInfo {
        int faceIndex;
        int corner0, corner1, corner2;
        unsigned int vi0, vi1, vi2;
    };

    std::vector<MorphTriInfo> morphTriangles;
    {
        for (int fi = 0; fi < static_cast<int>(morphFaces.size()); ++fi) {
            const auto& face = morphFaces[fi];
            if (face.vertexIndices.size() < 3) continue;
            for (size_t k = 1; k + 1 < face.vertexIndices.size(); ++k) {
                MorphTriInfo info;
                info.faceIndex = fi;
                info.corner0 = 0;
                info.corner1 = static_cast<int>(k);
                info.corner2 = static_cast<int>(k + 1);
                info.vi0 = face.vertexIndices[0];
                info.vi1 = face.vertexIndices[k];
                info.vi2 = face.vertexIndices[k + 1];
                morphTriangles.push_back(info);
            }
        }
    }

    bool morphHasSeparateUVs = morphMesh.HasSeparateUVIndices();
    bool targetHasSeparateUVs = targetMesh.HasSeparateUVIndices();

    int totalTriangles = static_cast<int>(morphTriangles.size());
    int processedTriangles = 0;

    // Compute auto max ray distance if not set: use target mesh bounding box diagonal
    float maxDist = settings.maxRayDistance;
    if (maxDist <= 0.0f) {
        BoundingBox targetBounds = targetMesh.GetBounds();
        Vector3 diag = targetBounds.Size();
        maxDist = std::sqrt(diag.x * diag.x + diag.y * diag.y + diag.z * diag.z) * 0.5f;
        MV_LOG_INFO(QString("Auto max ray distance: %1").arg(maxDist));
    }

    // Stats for debugging
    int rayHits = 0;
    int closestPointFallbacks = 0;
    int totalTexels = 0;

    // Process each morph mesh triangle
    for (int ti = 0; ti < totalTriangles; ++ti) {
        const auto& mt = morphTriangles[ti];
        const auto& morphFace = morphFaces[mt.faceIndex];

        // Get morph mesh UVs for this triangle's corners
        Vector2 muv0, muv1, muv2;
        if (morphHasSeparateUVs && !morphFace.uvIndices.empty()) {
            unsigned int uvIdx0 = morphFace.uvIndices[mt.corner0];
            unsigned int uvIdx1 = morphFace.uvIndices[mt.corner1];
            unsigned int uvIdx2 = morphFace.uvIndices[mt.corner2];
            if (uvIdx0 < morphUVs.size()) muv0 = morphUVs[uvIdx0];
            if (uvIdx1 < morphUVs.size()) muv1 = morphUVs[uvIdx1];
            if (uvIdx2 < morphUVs.size()) muv2 = morphUVs[uvIdx2];
        } else {
            if (mt.vi0 < morphUVs.size()) muv0 = morphUVs[mt.vi0];
            if (mt.vi1 < morphUVs.size()) muv1 = morphUVs[mt.vi1];
            if (mt.vi2 < morphUVs.size()) muv2 = morphUVs[mt.vi2];
        }

        // Get morph mesh 3D positions for this triangle (in morph-local space)
        Vector3 mv0 = morphVertices[mt.vi0];
        Vector3 mv1 = morphVertices[mt.vi1];
        Vector3 mv2 = morphVertices[mt.vi2];

        // Get morph mesh normals for this triangle (if available)
        Vector3 mn0, mn1, mn2;
        if (hasMorphNormals) {
            if (mt.vi0 < morphNormals.size()) mn0 = morphNormals[mt.vi0];
            if (mt.vi1 < morphNormals.size()) mn1 = morphNormals[mt.vi1];
            if (mt.vi2 < morphNormals.size()) mn2 = morphNormals[mt.vi2];
        } else {
            // Compute face normal as fallback
            Vector3 faceNormal = (mv1 - mv0).Cross(mv2 - mv0).Normalized();
            mn0 = mn1 = mn2 = faceNormal;
        }

        // Rasterize this triangle in morph UV space
        RasterizeTriangleUV(muv0, muv1, muv2, res,
            [&](int px, int py, float b0, float b1, float b2) {
                totalTexels++;

                // Interpolate 3D position on morph mesh surface (morph-local space)
                Vector3 morphPos;
                morphPos.x = b0 * mv0.x + b1 * mv1.x + b2 * mv2.x;
                morphPos.y = b0 * mv0.y + b1 * mv1.y + b2 * mv2.y;
                morphPos.z = b0 * mv0.z + b1 * mv1.z + b2 * mv2.z;

                // Interpolate normal on morph mesh surface (morph-local space)
                Vector3 morphNormal;
                morphNormal.x = b0 * mn0.x + b1 * mn1.x + b2 * mn2.x;
                morphNormal.y = b0 * mn0.y + b1 * mn1.y + b2 * mn2.y;
                morphNormal.z = b0 * mn0.z + b1 * mn1.z + b2 * mn2.z;
                morphNormal = morphNormal.Normalized();

                // Transform position and normal from morph-local to target-local space
                Vector3 targetSpacePos = TransformPoint(morphToTarget, morphPos);
                Vector3 targetSpaceNormal = TransformDirection(morphToTarget, morphNormal).Normalized();

                // === Primary: Ray trace along normal in both directions ===
                bool foundHit = false;
                int hitTriIndex = -1;
                unsigned int hitI0 = 0, hitI1 = 0, hitI2 = 0;
                Vector3 hitPoint;
                float hitDist = maxDist;

                // Cast ray in +normal direction
                {
                    Ray ray(targetSpacePos, targetSpaceNormal);
                    float t;
                    int triIndex;
                    unsigned int ri0, ri1, ri2;
                    if (targetBVH->RayIntersect(ray, targetVertices, t, triIndex, ri0, ri1, ri2)) {
                        if (t > 0.0f && t < hitDist) {
                            hitDist = t;
                            hitTriIndex = triIndex;
                            hitI0 = ri0;
                            hitI1 = ri1;
                            hitI2 = ri2;
                            hitPoint = ray.PointAt(t);
                            foundHit = true;
                        }
                    }
                }

                // Cast ray in -normal direction
                {
                    Ray ray(targetSpacePos, targetSpaceNormal * -1.0f);
                    float t;
                    int triIndex;
                    unsigned int ri0, ri1, ri2;
                    if (targetBVH->RayIntersect(ray, targetVertices, t, triIndex, ri0, ri1, ri2)) {
                        if (t > 0.0f && t < hitDist) {
                            hitDist = t;
                            hitTriIndex = triIndex;
                            hitI0 = ri0;
                            hitI1 = ri1;
                            hitI2 = ri2;
                            hitPoint = ray.PointAt(t);
                            foundHit = true;
                        }
                    }
                }

                // === Fallback: Closest point on target surface ===
                if (!foundHit) {
                    BVHClosestPointResult closest = targetBVH->FindClosestPoint(targetSpacePos, targetVertices);
                    if (!closest.found) return;

                    // Apply distance limit
                    float dist = std::sqrt(closest.distanceSq);
                    if (maxDist > 0.0f && dist > maxDist) return;

                    hitTriIndex = closest.triangleIndex;
                    hitI0 = closest.i0;
                    hitI1 = closest.i1;
                    hitI2 = closest.i2;
                    hitPoint = closest.point;
                    closestPointFallbacks++;
                } else {
                    rayHits++;
                }

                // Compute barycentric coordinates on the hit target triangle
                const Vector3& tv0 = targetVertices[hitI0];
                const Vector3& tv1 = targetVertices[hitI1];
                const Vector3& tv2 = targetVertices[hitI2];
                Vector3 targetBary = ComputeBarycentric3D(hitPoint, tv0, tv1, tv2);

                // Clamp barycentrics to valid range
                targetBary.x = std::max(0.0f, targetBary.x);
                targetBary.y = std::max(0.0f, targetBary.y);
                targetBary.z = std::max(0.0f, targetBary.z);
                float barySum = targetBary.x + targetBary.y + targetBary.z;
                if (barySum > 0.0f) {
                    targetBary.x /= barySum;
                    targetBary.y /= barySum;
                    targetBary.z /= barySum;
                }

                // Look up face/corner info for UV interpolation
                int faceIdx = -1;
                int c0 = 0, c1 = 1, c2 = 2;
                auto it = targetTriToFace.find(hitTriIndex);
                if (it != targetTriToFace.end()) {
                    faceIdx = it->second.faceIndex;
                    c0 = it->second.corner0;
                    c1 = it->second.corner1;
                    c2 = it->second.corner2;
                }

                // Interpolate target UV
                Vector2 targetUV = InterpolateTargetUV(
                    targetFaces, targetUVs, targetHasSeparateUVs,
                    faceIdx, c0, c1, c2,
                    hitI0, hitI1, hitI2,
                    targetBary);

                int pixelIdx = py * res + px;

                // Sample and write albedo
                if (settings.bakeAlbedo && targetAlbedo) {
                    Vector3 color = targetAlbedo->SampleBilinear(targetUV.x, targetUV.y);
                    bakedAlbedo->SetPixel(px, py,
                        static_cast<uint8_t>(std::min(255.0f, color.x * 255.0f)),
                        static_cast<uint8_t>(std::min(255.0f, color.y * 255.0f)),
                        static_cast<uint8_t>(std::min(255.0f, color.z * 255.0f)),
                        255);
                    albedoCoverage[pixelIdx] = true;
                }

                // Sample and write normal map
                if (bakedNormal && targetNormalMap) {
                    Vector3 normal = targetNormalMap->SampleBilinear(targetUV.x, targetUV.y);
                    bakedNormal->SetPixel(px, py,
                        static_cast<uint8_t>(std::min(255.0f, normal.x * 255.0f)),
                        static_cast<uint8_t>(std::min(255.0f, normal.y * 255.0f)),
                        static_cast<uint8_t>(std::min(255.0f, normal.z * 255.0f)),
                        255);
                    normalCoverage[pixelIdx] = true;
                }
            }
        );

        processedTriangles++;
        if (progressCallback && (processedTriangles % 100 == 0 || processedTriangles == totalTriangles)) {
            progressCallback(static_cast<float>(processedTriangles) / static_cast<float>(totalTriangles));
        }
    }

    MV_LOG_INFO(QString("Bake stats: %1 total texels, %2 ray hits, %3 closest-point fallbacks")
        .arg(totalTexels).arg(rayHits).arg(closestPointFallbacks));

    // Apply seam bleeding
    if (settings.paddingPixels > 0) {
        if (bakedAlbedo) {
            MV_LOG_INFO("Applying seam bleeding to albedo...");
            ApplySeamBleeding(bakedAlbedo->GetPixels(), albedoCoverage, res, res, settings.paddingPixels);
        }
        if (bakedNormal) {
            MV_LOG_INFO("Applying seam bleeding to normal map...");
            ApplySeamBleeding(bakedNormal->GetPixels(), normalCoverage, res, res, settings.paddingPixels);
        }
    }

    result.success = true;
    result.bakedAlbedo = bakedAlbedo;
    result.bakedNormalMap = bakedNormal;

    MV_LOG_INFO(QString("Texture bake completed: %1x%1").arg(res));
    return result;
}

} // namespace MetaVisage
