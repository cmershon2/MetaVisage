#include "deformation/TemplateFitSolver.h"

#include "utils/BVH.h"
#include "utils/Logger.h"
#include "utils/RayCaster.h"

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <format>
#include <limits>
#include <unordered_map>

namespace MetaVisage {

NRICPParams TemplateFitCoarseParams() {
    NRICPParams p;
    p.stiffnessSteps = 3;
    p.alphaInitial = 60.0f;
    p.alphaFinal = 15.0f;
    p.icpIterations = 4;
    p.optimizationIterations = 1;
    p.dpInitial = 1.0f;
    p.dpFinal = 1.0f;
    p.gammaInitial = 1.0f;   // rigidity keeps features from sliding apart
    p.gammaFinal = 1.0f;
    p.samplingInitial = 0.06f;
    p.samplingFinal = 0.02f;
    p.normalizeSampling = true;
    return p;
}

namespace {

using Clock = std::chrono::steady_clock;

Eigen::Vector3d ToEigen(const Vector3& v) { return {v.x, v.y, v.z}; }
Vector3 FromEigen(const Eigen::Vector3d& v) {
    return {static_cast<float>(v.x()), static_cast<float>(v.y()), static_cast<float>(v.z())};
}

Vector3 Apply(const Eigen::Matrix4d& m, const Vector3& p) {
    const Eigen::Vector4d r = m * Eigen::Vector4d(p.x, p.y, p.z, 1.0);
    return {static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.z())};
}

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

struct SurfaceHit {
    Vector3 point;
    Vector3 normal;
    int vertex = -1;
    float distance = std::numeric_limits<float>::max();
    bool found = false;
};

SurfaceHit Closest(const BVH& bvh, const std::vector<Vector3>& verts, const Vector3& p) {
    SurfaceHit hit;
    const BVHClosestPointResult r = bvh.FindClosestPoint(p, verts);
    if (!r.found) return hit;
    hit.found = true;
    hit.point = r.point;
    hit.normal = r.normal;
    hit.distance = std::sqrt(r.distanceSq);
    // Snap to the triangle's nearest corner, as manual placement does.
    float best = std::numeric_limits<float>::max();
    for (unsigned int idx : {r.i0, r.i1, r.i2}) {
        const float d = (verts[idx] - r.point).Length();
        if (d < best) { best = d; hit.vertex = static_cast<int>(idx); }
    }
    return hit;
}

std::shared_ptr<Mesh> MakeMesh(const std::vector<Vector3>& verts, const std::vector<Face>& faces) {
    auto mesh = std::make_shared<Mesh>();
    mesh->SetVertices(verts);
    mesh->SetFaces(faces);
    mesh->CalculateNormals();
    mesh->CalculateBounds();
    return mesh;
}

Vector3 AxisUnit(Axis axis) {
    switch (axis) {
        case Axis::Y: return {0.0f, 1.0f, 0.0f};
        case Axis::Z: return {0.0f, 0.0f, 1.0f};
        default:      return {1.0f, 0.0f, 0.0f};
    }
}

LandmarkSolveResult Fail(const std::string& message) {
    LandmarkSolveResult r;
    r.errorMessage = message;
    return r;
}

// Uniform grid over a point set for radius queries.
class PointGrid {
public:
    PointGrid(const std::vector<Vector3>& pts, float cell) : pts_(pts), cell_(cell) {
        for (size_t i = 0; i < pts.size(); ++i) cells_[Key(Cell(pts[i]))].push_back(static_cast<int>(i));
    }

    template <class F>
    void ForEachWithin(const Vector3& p, float r, F&& f) const {
        const std::array<int, 3> lo = Cell(p - Vector3(r, r, r));
        const std::array<int, 3> hi = Cell(p + Vector3(r, r, r));
        const float r2 = r * r;
        for (int x = lo[0]; x <= hi[0]; ++x) {
            for (int y = lo[1]; y <= hi[1]; ++y) {
                for (int z = lo[2]; z <= hi[2]; ++z) {
                    const auto it = cells_.find(Key({x, y, z}));
                    if (it == cells_.end()) continue;
                    for (int i : it->second) {
                        const Vector3 d = pts_[i] - p;
                        if (d.Dot(d) <= r2) f(i);
                    }
                }
            }
        }
    }

private:
    std::array<int, 3> Cell(const Vector3& p) const {
        return {static_cast<int>(std::floor(p.x / cell_)), static_cast<int>(std::floor(p.y / cell_)),
                static_cast<int>(std::floor(p.z / cell_))};
    }
    static int64_t Key(const std::array<int, 3>& c) {
        return (static_cast<int64_t>(c[0] & 0x1FFFFF) << 42) | (static_cast<int64_t>(c[1] & 0x1FFFFF) << 21) |
               static_cast<int64_t>(c[2] & 0x1FFFFF);
    }

    const std::vector<Vector3>& pts_;
    float cell_;
    std::unordered_map<int64_t, std::vector<int>> cells_;
};

// Landmarks that sit at the deepest point of a crease, for the final snap.
// Keyed by the built-in profile's ids; custom profiles (Sprint 14) will carry
// it instead. Tested on the six-head dataset: crease snapping helped the mouth;
// tips, the subnasale and ear extremes did not, so they're left to the fit.
bool SnapsToCrease(const std::string& id) {
    return id == "mouth_corner_l" || id == "mouth_corner_r" || id == "lip_contact_center";
}

// Weighted, regularized affine fit of x onto y. Returns false when degenerate.
bool FitAffine(const std::vector<Eigen::Vector3d>& x, const std::vector<Eigen::Vector3d>& y,
               const std::vector<double>& w, double regularization, Eigen::Matrix4d& out) {
    double wsum = 0.0;
    Eigen::Vector3d mx = Eigen::Vector3d::Zero(), my = Eigen::Vector3d::Zero();
    for (size_t i = 0; i < x.size(); ++i) { wsum += w[i]; mx += w[i] * x[i]; my += w[i] * y[i]; }
    if (!(wsum > 0.0)) return false;
    mx /= wsum;
    my /= wsum;
    Eigen::Matrix3d sxx = Eigen::Matrix3d::Zero(), syx = Eigen::Matrix3d::Zero();
    for (size_t i = 0; i < x.size(); ++i) {
        const Eigen::Vector3d a = x[i] - mx, b = y[i] - my;
        sxx += w[i] * a * a.transpose();
        syx += w[i] * b * a.transpose();
    }
    const double lambda = regularization * sxx.trace() / 3.0;
    const Eigen::Matrix3d reg = lambda * Eigen::Matrix3d::Identity();
    const Eigen::Matrix3d denom = sxx + reg;
    if (std::abs(denom.determinant()) < 1e-18) return false;
    const Eigen::Matrix3d m = (syx + reg) * denom.inverse();
    out = Eigen::Matrix4d::Identity();
    out.block<3, 3>(0, 0) = m;
    out.block<3, 1>(0, 3) = my - m * mx;
    return true;
}

} // namespace

LandmarkSolveResult TemplateFitSolver::Solve(const LandmarkSolveInput& in,
                                             const LandmarkProgressCallback& progress,
                                             std::atomic<bool>& cancel) {
    const auto t0 = Clock::now();
    auto report = [&](float p, const std::string& msg) { if (progress) progress(p, msg); };
    auto cancelled = [&] {
        LandmarkSolveResult r = Fail("Auto-map cancelled");
        r.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
        return r;
    };

    if (!in.source || !in.target || in.source->GetVertexCount() == 0 ||
        in.target->GetVertexCount() == 0 || in.target->GetFaceCount() == 0) {
        return Fail("Auto-map needs both a source and a target mesh.");
    }
    if (in.landmarks.empty()) {
        return Fail("The source profile has no landmarks to place.");
    }
    const size_t srcCount = in.source->GetVertexCount();
    for (const auto& lm : in.landmarks) {
        if (lm.vertexIndex < 0 || static_cast<size_t>(lm.vertexIndex) >= srcCount) {
            return Fail(std::format("Landmark '{}' points past the end of the source mesh.", lm.name));
        }
    }
    const bool hasMask = in.mask.size() == srcCount;
    if (cancel.load()) return cancelled();

    report(0.0f, "Preparing meshes");

    // World-space source and target.
    const Matrix4x4 srcM = in.sourceTransform.GetMatrix();
    const Matrix4x4 tgtM = in.targetTransform.GetMatrix();
    std::vector<Vector3> srcW(srcCount);
    for (size_t i = 0; i < srcCount; ++i) {
        srcW[i] = RayCaster::TransformPoint(in.source->GetVertices()[i], srcM);
    }
    std::vector<Vector3> tgtW(in.target->GetVertexCount());
    for (size_t i = 0; i < tgtW.size(); ++i) {
        tgtW[i] = RayCaster::TransformPoint(in.target->GetVertices()[i], tgtM);
    }
    auto target = MakeMesh(tgtW, in.target->GetFaces());
    const BoundingBox& tb = target->GetBounds();
    const float diag = (tb.max - tb.min).Length();
    if (!(diag > 0.0f)) return Fail("The target mesh has no size.");
    const BVH& bvh = *target->GetBVH();
    const std::vector<Vector3>& tv = target->GetVertices();
    auto sourceMesh = MakeMesh(srcW, in.source->GetFaces());
    const std::vector<Vector3>& srcN = sourceMesh->GetNormals();
    if (cancel.load()) return cancelled();

    // Region of interest: the landmarks' bounding box, padded. Keeps the
    // shoulders and lower neck from pulling the fit.
    Vector3 lo(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
               std::numeric_limits<float>::max());
    Vector3 hi = lo * -1.0f;
    for (const auto& lm : in.landmarks) {
        const Vector3& p = srcW[lm.vertexIndex];
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    const Vector3 extent = hi - lo;
    const float pad = params_.roiPadding * std::max({extent.x, extent.y, extent.z});
    lo = lo - Vector3(pad, pad, pad);
    hi = hi + Vector3(pad, pad, pad);
    std::vector<bool> inRoi(srcCount);
    std::vector<int> eligible;
    for (size_t i = 0; i < srcCount; ++i) {
        const Vector3& p = srcW[i];
        inRoi[i] = p.x >= lo.x && p.x <= hi.x && p.y >= lo.y && p.y <= hi.y &&
                   p.z >= lo.z && p.z <= hi.z;
        if (inRoi[i] && !(hasMask && in.mask[i])) eligible.push_back(static_cast<int>(i));
    }
    if (eligible.size() < 16) return Fail("Too few source vertices to fit around the landmarks.");
    std::vector<int> samples;
    const size_t stride = std::max<size_t>(1, eligible.size() / params_.similaritySamples);
    for (size_t i = 0; i < eligible.size(); i += stride) samples.push_back(eligible[i]);

    // 1. Similarity refinement (trimmed ICP with scale).
    report(0.02f, "Refining alignment");
    const int sampleCount = static_cast<int>(samples.size());
    std::vector<SurfaceHit> hits(sampleCount);
    std::vector<Vector3> moved(sampleCount);
    enum class Fit { Ok, Cancelled, NoOverlap };
    auto runSimilarity = [&](Eigen::Matrix4d& sim, float& residual, bool showProgress) {
    float prevResidual = std::numeric_limits<float>::max();
    for (int iter = 0; iter < params_.similarityIterations; ++iter) {
        if (cancel.load()) return Fit::Cancelled;
        const Eigen::Matrix3d lin = sim.block<3, 3>(0, 0);
        #pragma omp parallel for schedule(dynamic, 64)
        for (int j = 0; j < sampleCount; ++j) {
            moved[j] = Apply(sim, srcW[samples[j]]);
            hits[j] = Closest(bvh, tv, moved[j]);
            if (hits[j].found) {
                const Vector3 n = FromEigen((lin * ToEigen(srcN[samples[j]])).normalized());
                // Facing away (back of a lip, inside of an ear): not a match.
                if (n.Dot(hits[j].normal) < 0.0f && std::abs(n.Dot(hits[j].normal)) > 0.5f) {
                    hits[j].found = false;
                }
            }
        }
        std::vector<int> valid;
        valid.reserve(sampleCount);
        for (int j = 0; j < sampleCount; ++j) if (hits[j].found) valid.push_back(j);
        if (valid.size() < 16) return Fit::NoOverlap;
        const size_t keep = std::max<size_t>(16, static_cast<size_t>(valid.size() * params_.trimFraction));
        std::nth_element(valid.begin(), valid.begin() + (keep - 1), valid.end(),
                         [&](int a, int b) { return hits[a].distance < hits[b].distance; });
        valid.resize(keep);

        Eigen::Matrix3Xd from(3, keep), to(3, keep);
        double sq = 0.0;
        for (size_t k = 0; k < keep; ++k) {
            from.col(k) = ToEigen(moved[valid[k]]);
            to.col(k) = ToEigen(hits[valid[k]].point);
            sq += static_cast<double>(hits[valid[k]].distance) * hits[valid[k]].distance;
        }
        residual = static_cast<float>(std::sqrt(sq / keep));
        sim = Eigen::umeyama(from, to, true) * sim;
        if (std::abs(prevResidual - residual) < 1e-5f * diag) break;
        prevResidual = residual;
        if (showProgress) {
            report(0.02f + 0.14f * static_cast<float>(iter + 1) / params_.similarityIterations,
                   "Refining alignment");
        }
    }
    return Fit::Ok;
    };

    Eigen::Matrix4d sim = Eigen::Matrix4d::Identity();
    float residual = 0.0f;
    switch (runSimilarity(sim, residual, true)) {
        case Fit::Cancelled: return cancelled();
        case Fit::NoOverlap: return Fail("The target doesn't overlap the source. Align it in Stage 1 first.");
        case Fit::Ok: break;
    }

    // A target facing backwards can still settle into a nearby fit (the face
    // on the back of the head). Fit again from a pose turned 180 degrees about
    // the vertical (world Y, the viewport's up); if that fits clearly better,
    // Stage 1 is wrong, so refuse. Upside-down poses aren't tried: a rounded
    // skull can match them deceptively well.
    {
        Eigen::Vector3d roiCenter = Eigen::Vector3d::Zero();
        for (int j : samples) roiCenter += ToEigen(srcW[j]);
        roiCenter /= static_cast<double>(samples.size());
        for (const Eigen::Vector3d& flipAxis : {Eigen::Vector3d::UnitY()}) {
            Eigen::Affine3d flip = Eigen::Translation3d(roiCenter) *
                                   Eigen::AngleAxisd(3.14159265358979, flipAxis) *
                                   Eigen::Translation3d(-roiCenter);
            Eigen::Matrix4d flipped = flip.matrix();
            float flippedResidual = 0.0f;
            const Fit fit = runSimilarity(flipped, flippedResidual, false);
            if (fit == Fit::Cancelled) return cancelled();
            if (fit == Fit::Ok && flippedResidual < params_.flipResidualRatio * residual) {
                LandmarkSolveResult r = Fail(
                    "The target seems to face a different way than the source. "
                    "Rotate it to match in Stage 1 and try again.");
                r.alignmentResidual = residual / diag;
                r.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
                return r;
            }
        }
    }
    report(0.18f, "Refining alignment");

    const Eigen::Matrix3d lin = sim.block<3, 3>(0, 0);
    const double scale = std::cbrt(lin.determinant());
    const Eigen::Matrix3d rot = lin / scale;
    const double cosAngle = std::clamp((rot.trace() - 1.0) * 0.5, -1.0, 1.0);
    const float angleDeg = static_cast<float>(std::acos(cosAngle) * 180.0 / 3.14159265358979);
    if (!(scale > 0.0) || scale > params_.maxScaleChange || scale < 1.0 / params_.maxScaleChange ||
        angleDeg > params_.maxRotationDeg) {
        LandmarkSolveResult r = Fail(
            "The target is too far from the Stage 1 alignment for auto-map. "
            "Align it more closely in Stage 1 and try again.");
        r.alignmentScale = static_cast<float>(scale);
        r.alignmentRotationDeg = angleDeg;
        r.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
        return r;
    }

    // 1b. Affine refinement on top of the similarity: lets proportions differ
    // (a wider face, a shallower skull) before the coarse fit, which handles
    // local shape better than a large global stretch.
    Eigen::Matrix4d aff = sim;
    float prevResidual = residual;
    for (int iter = 0; iter < params_.affineIterations; ++iter) {
        if (cancel.load()) return cancelled();
        const Eigen::Matrix3d normalMat = aff.block<3, 3>(0, 0).inverse().transpose();
        #pragma omp parallel for schedule(dynamic, 64)
        for (int j = 0; j < sampleCount; ++j) {
            moved[j] = Apply(aff, srcW[samples[j]]);
            hits[j] = Closest(bvh, tv, moved[j]);
            if (hits[j].found) {
                const Vector3 n = FromEigen((normalMat * ToEigen(srcN[samples[j]])).normalized());
                if (n.Dot(hits[j].normal) < 0.0f && std::abs(n.Dot(hits[j].normal)) > 0.5f) {
                    hits[j].found = false;
                }
            }
        }
        std::vector<int> valid;
        for (int j = 0; j < sampleCount; ++j) if (hits[j].found) valid.push_back(j);
        if (valid.size() < 16) break;
        const size_t keep = std::max<size_t>(16, static_cast<size_t>(valid.size() * params_.trimFraction));
        std::nth_element(valid.begin(), valid.begin() + (keep - 1), valid.end(),
                         [&](int a, int b) { return hits[a].distance < hits[b].distance; });
        valid.resize(keep);

        Eigen::Vector3d mx = Eigen::Vector3d::Zero(), my = Eigen::Vector3d::Zero();
        for (int j : valid) { mx += ToEigen(moved[j]); my += ToEigen(hits[j].point); }
        mx /= static_cast<double>(keep);
        my /= static_cast<double>(keep);
        Eigen::Matrix3d sxx = Eigen::Matrix3d::Zero(), syx = Eigen::Matrix3d::Zero();
        double sq = 0.0;
        for (int j : valid) {
            const Eigen::Vector3d x = ToEigen(moved[j]) - mx;
            const Eigen::Vector3d y = ToEigen(hits[j].point) - my;
            sxx += x * x.transpose();
            syx += y * x.transpose();
            sq += static_cast<double>(hits[j].distance) * hits[j].distance;
        }
        if (std::abs(sxx.determinant()) < 1e-12) break;
        const Eigen::Matrix3d m = syx * sxx.inverse();
        // Refuse steps that would squash or shear the head implausibly.
        const Eigen::JacobiSVD<Eigen::Matrix3d> svd(m * aff.block<3, 3>(0, 0) * lin.inverse());
        const Eigen::Vector3d sv = svd.singularValues();
        if (sv.maxCoeff() > params_.maxAffineStretch || sv.minCoeff() < 1.0 / params_.maxAffineStretch) break;
        Eigen::Matrix4d step = Eigen::Matrix4d::Identity();
        step.block<3, 3>(0, 0) = m;
        step.block<3, 1>(0, 3) = my - m * mx;
        aff = step * aff;
        const float stepResidual = static_cast<float>(std::sqrt(sq / keep));
        residual = stepResidual;
        if (iter > 0 && std::abs(prevResidual - stepResidual) < 1e-5f * diag) break;
        prevResidual = stepResidual;
    }

    // The aligned source, and the vertices the fits may use.
    std::vector<Vector3> aligned(srcCount);
    for (size_t i = 0; i < srcCount; ++i) aligned[i] = Apply(aff, srcW[i]);
    auto alignedMesh = MakeMesh(aligned, in.source->GetFaces());
    std::vector<bool> excluded(srcCount);
    for (size_t i = 0; i < srcCount; ++i) excluded[i] = !inRoi[i] || (hasMask && in.mask[i]);

    const std::vector<Vector3>& targetN = target->GetNormals();
    const Vector3 symAxis = AxisUnit(in.symmetryAxis);

    // 1c. Feature refit. The coarse fit is stiff on purpose, so a feature much
    // larger or smaller than the source's (cartoon eyes, a goblin's ears)
    // would keep roughly the source's size. Refit each one on its own with a
    // bounded affine first, so the coarse fit starts from it. Matching runs
    // both ways: patch to target pulls the patch onto the surface, target to
    // patch pulls it out over a larger feature.
    if (params_.featureRefit) {
        report(0.2f, "Refitting features");
        const std::vector<Vector3> coarseN = alignedMesh->GetNormals();
        const std::vector<Vector3> base = aligned;
        std::vector<float> bestWeight(srcCount, 0.0f);

        // Group by region; a region with no midline landmark (an eye, an ear)
        // splits into one group per side.
        std::vector<std::pair<std::string, std::vector<int>>> groups;
        for (const std::string& region : params_.refitRegions) {
            bool hasMidline = false;
            for (const auto& lm : in.landmarks) {
                if (lm.region == region && lm.mirrorId.empty()) hasMidline = true;
            }
            std::vector<int> left, right;
            for (const auto& lm : in.landmarks) {
                if (lm.region != region) continue;
                const float side = in.source->GetVertices()[lm.vertexIndex].Dot(symAxis) - in.symmetryPlaneOffset;
                (hasMidline || side >= 0.0f ? left : right).push_back(lm.vertexIndex);
            }
            if (!left.empty()) groups.emplace_back(region, std::move(left));
            if (!right.empty()) groups.emplace_back(region, std::move(right));
        }

        for (const auto& [region, lmVerts] : groups) {
            if (cancel.load()) return cancelled();
            Eigen::Vector3d c = Eigen::Vector3d::Zero();
            for (int v : lmVerts) c += ToEigen(base[v]);
            c /= static_cast<double>(lmVerts.size());
            double spread = 0.0;
            for (int v : lmVerts) spread = std::max(spread, (ToEigen(base[v]) - c).norm());
            const double R = std::max(spread * params_.regionPatchScale,
                                      static_cast<double>(params_.regionMinRadius * diag));

            std::vector<int> patch;
            for (size_t i = 0; i < srcCount; ++i) {
                if (excluded[i]) continue;
                if ((ToEigen(base[i]) - c).norm() <= R) patch.push_back(static_cast<int>(i));
            }
            if (patch.size() < 32) continue;
            std::vector<int> ps;
            const size_t pStride = std::max<size_t>(1, patch.size() / params_.regionSamples);
            for (size_t i = 0; i < patch.size(); i += pStride) ps.push_back(patch[i]);
            const int pn = static_cast<int>(ps.size());

            Eigen::Matrix4d A = Eigen::Matrix4d::Identity();
            std::vector<Vector3> px(pn), pnrm(pn);
            std::vector<SurfaceHit> fwd(pn);
            double prev = std::numeric_limits<double>::max();
            for (int iter = 0; iter < params_.regionIterations; ++iter) {
                const Eigen::Matrix3d nm = A.block<3, 3>(0, 0).inverse().transpose();
                #pragma omp parallel for schedule(dynamic, 64)
                for (int j = 0; j < pn; ++j) {
                    px[j] = Apply(A, base[ps[j]]);
                    pnrm[j] = FromEigen((nm * ToEigen(coarseN[ps[j]])).normalized());
                    fwd[j] = Closest(bvh, tv, px[j]);
                    if (fwd[j].found && pnrm[j].Dot(fwd[j].normal) < -0.5f) fwd[j].found = false;
                }

                // Target vertices around the current feature, matched back to the patch.
                const double s = std::cbrt(std::abs(A.block<3, 3>(0, 0).determinant()));
                const Eigen::Vector4d cc = A * Eigen::Vector4d(c.x(), c.y(), c.z(), 1.0);
                const Vector3 cw(static_cast<float>(cc.x()), static_cast<float>(cc.y()), static_cast<float>(cc.z()));
                const float rb = static_cast<float>(R * s);
                std::vector<int> tball;
                for (size_t t = 0; t < tv.size(); ++t) {
                    if ((tv[t] - cw).Length() <= rb) tball.push_back(static_cast<int>(t));
                }
                const size_t tStride = std::max<size_t>(1, tball.size() / params_.regionSamples);
                std::vector<int> ts;
                for (size_t i = 0; i < tball.size(); i += tStride) ts.push_back(tball[i]);
                const int tn = static_cast<int>(ts.size());
                std::vector<int> bwdIdx(tn, -1);
                std::vector<float> bwdDist(tn, std::numeric_limits<float>::max());
                #pragma omp parallel for schedule(dynamic, 64)
                for (int i = 0; i < tn; ++i) {
                    const Vector3& q = tv[ts[i]];
                    for (int j = 0; j < pn; ++j) {
                        const Vector3 d = px[j] - q;
                        const float d2 = d.Dot(d);
                        if (d2 < bwdDist[i] && pnrm[j].Dot(targetN[ts[i]]) >= -0.5f) {
                            bwdDist[i] = d2;
                            bwdIdx[i] = j;
                        }
                    }
                }

                std::vector<int> fv, bv;
                for (int j = 0; j < pn; ++j) if (fwd[j].found) fv.push_back(j);
                for (int i = 0; i < tn; ++i) if (bwdIdx[i] >= 0) bv.push_back(i);
                if (fv.size() < 16 || bv.size() < 16) break;
                auto trim = [&](std::vector<int>& idx, auto dist) {
                    const size_t keep = std::max<size_t>(16, static_cast<size_t>(idx.size() * params_.trimFraction));
                    std::nth_element(idx.begin(), idx.begin() + (keep - 1), idx.end(),
                                     [&](int a, int b) { return dist(a) < dist(b); });
                    idx.resize(keep);
                };
                trim(fv, [&](int j) { return fwd[j].distance; });
                trim(bv, [&](int i) { return bwdDist[i]; });

                std::vector<Eigen::Vector3d> xs, ys;
                std::vector<double> ws;
                double sq = 0.0;
                for (int j : fv) {
                    xs.push_back(ToEigen(px[j]));
                    ys.push_back(ToEigen(fwd[j].point));
                    ws.push_back(1.0 / fv.size());
                    sq += static_cast<double>(fwd[j].distance) * fwd[j].distance / fv.size();
                }
                for (int i : bv) {
                    xs.push_back(ToEigen(px[bwdIdx[i]]));
                    ys.push_back(ToEigen(tv[ts[i]]));
                    ws.push_back(1.0 / bv.size());
                    sq += static_cast<double>(bwdDist[i]) / bv.size();
                }
                Eigen::Matrix4d step;
                if (!FitAffine(xs, ys, ws, params_.regionRegularization, step)) break;
                const Eigen::Matrix4d next = step * A;
                const Eigen::JacobiSVD<Eigen::Matrix3d> svd(next.block<3, 3>(0, 0));
                const Eigen::Vector3d sv = svd.singularValues();
                const Eigen::Vector4d nc = next * Eigen::Vector4d(c.x(), c.y(), c.z(), 1.0);
                if (sv.maxCoeff() > params_.maxRegionStretch || sv.minCoeff() < 1.0 / params_.maxRegionStretch ||
                    (nc.head<3>() - c).norm() > params_.maxRegionShift * R ||
                    next.block<3, 3>(0, 0).determinant() <= 0.0) {
                    break;
                }
                A = next;
                const double rms = std::sqrt(sq * 0.5);
                if (std::abs(prev - rms) < 1e-5 * diag) break;
                prev = rms;
            }

            // Blend the refit into the coarse fit: full inside 70% of the
            // patch radius, fading to none at its edge.
            for (int i : patch) {
                const double d = (ToEigen(base[i]) - c).norm();
                const float w = static_cast<float>(std::clamp((R - d) / (0.3 * R), 0.0, 1.0));
                if (w <= bestWeight[i]) continue;
                bestWeight[i] = w;
                aligned[i] = base[i] + (Apply(A, base[i]) - base[i]) * w;
            }
        }
    }
    alignedMesh = MakeMesh(aligned, in.source->GetFaces());

    // 2. Coarse fit of the aligned source onto the target.
    report(0.25f, "Fitting the source to the target");
    NRICPDeformer coarse;
    coarse.SetSourceMesh(aligned, alignedMesh->GetNormals(), in.source->GetFaces());
    coarse.SetTargetMesh(target);
    coarse.SetParams(params_.coarse);
    coarse.SetUserExcludedVertices(excluded);
    coarse.SetCancellationFlag(&cancel);
    coarse.SetProgressCallback([&](float p, const std::string&) {
        report(0.25f + 0.65f * Clamp01(p), "Fitting the source to the target");
    });
    std::vector<Vector3> fitted = coarse.Solve();
    if (cancel.load()) return cancelled();
    if (fitted.size() != srcCount) return Fail("The coarse fit did not converge.");
    auto fittedMesh = MakeMesh(fitted, in.source->GetFaces());
    const std::vector<Vector3>& fittedN = fittedMesh->GetNormals();

    // 3. Projection.
    report(0.92f, "Placing landmarks");
    const size_t lmCount = in.landmarks.size();
    std::vector<SurfaceHit> placed(lmCount);
    std::vector<float> projDist(lmCount);
    std::vector<float> normalAgree(lmCount);
    for (size_t k = 0; k < lmCount; ++k) {
        const int v = in.landmarks[k].vertexIndex;
        placed[k] = Closest(bvh, tv, fitted[v]);
        if (!placed[k].found) return Fail("A landmark could not be placed on the target surface.");
        projDist[k] = placed[k].distance;
        normalAgree[k] = std::abs(fittedN[v].Normalized().Dot(placed[k].normal));
    }

    // 4. Symmetry.
    const Vector3 localP0 = symAxis * in.symmetryPlaneOffset;
    const Vector3 w0 = Apply(aff, RayCaster::TransformPoint(localP0, srcM));
    // Under the affine fit a plane's normal maps by the inverse transpose.
    Eigen::Matrix3d srcLin;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) srcLin(r, c) = srcM.m[c * 4 + r];
    }
    const Eigen::Matrix3d planeMat = (aff.block<3, 3>(0, 0) * srcLin).inverse().transpose();
    const Vector3 planeN = FromEigen((planeMat * ToEigen(symAxis)).normalized());
    auto reflect = [&](const Vector3& p) { return p - planeN * (2.0f * (p - w0).Dot(planeN)); };

    Vector3 flo(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());
    Vector3 fhi = flo * -1.0f;
    for (const auto& h : placed) {
        flo = {std::min(flo.x, h.point.x), std::min(flo.y, h.point.y), std::min(flo.z, h.point.z)};
        fhi = {std::max(fhi.x, h.point.x), std::max(fhi.y, h.point.y), std::max(fhi.z, h.point.z)};
    }
    std::vector<int> mirrorSamples;
    for (size_t i = 0; i < tv.size(); ++i) {
        const Vector3& p = tv[i];
        if (p.x >= flo.x && p.x <= fhi.x && p.y >= flo.y && p.y <= fhi.y && p.z >= flo.z && p.z <= fhi.z) {
            mirrorSamples.push_back(static_cast<int>(i));
        }
    }
    const size_t mStride = std::max<size_t>(1, mirrorSamples.size() / 3000);
    std::vector<float> mirrorDist;
    for (size_t i = 0; i < mirrorSamples.size(); i += mStride) {
        const SurfaceHit h = Closest(bvh, tv, reflect(tv[mirrorSamples[i]]));
        if (h.found) mirrorDist.push_back(h.distance);
    }
    float mirrorError = 1.0f;
    if (!mirrorDist.empty()) {
        std::nth_element(mirrorDist.begin(), mirrorDist.begin() + mirrorDist.size() / 2, mirrorDist.end());
        mirrorError = mirrorDist[mirrorDist.size() / 2] / diag;
    }
    const bool symmetric = mirrorError < params_.symmetryThreshold;

    // 3b. Feature snapping: nudge crease landmarks into the crease. The fit
    // only knows how far a point is from the surface, not where along it the
    // mouth's corners are.
    if (params_.featureSnap) {
        double edgeSum = 0.0;
        size_t edgeCount = 0;
        const auto& tf = target->GetFaces();
        const size_t fStride = std::max<size_t>(1, tf.size() / 5000);
        for (size_t f = 0; f < tf.size(); f += fStride) {
            const auto& vi = tf[f].vertexIndices;
            for (size_t e = 0; e < vi.size(); ++e) {
                edgeSum += (tv[vi[e]] - tv[vi[(e + 1) % vi.size()]]).Length();
                ++edgeCount;
            }
        }
        const float meanEdge = edgeCount ? static_cast<float>(edgeSum / edgeCount) : 0.0f;
        const float rho = std::max(params_.snapScale * diag, 1.5f * meanEdge);
        const float r = std::max(params_.snapRadius * diag, 2.0f * meanEdge);
        const PointGrid grid(tv, rho);
        // Positive in a crease: how far the neighbourhood sits above the
        // point along the outward direction.
        auto depth = [&](int t, const Vector3& out) {
            Vector3 sum;
            int n = 0;
            grid.ForEachWithin(tv[t], rho, [&](int i) { sum = sum + tv[i]; ++n; });
            if (n < 2) return 0.0f;
            return out.Dot(sum * (1.0f / static_cast<float>(n)) - tv[t]) / rho;
        };
        auto candidates = [&](const Vector3& centre, const Vector3& out) {
            std::vector<int> c;
            grid.ForEachWithin(centre, r, [&](int t) {
                if (targetN[t].Normalized().Dot(out) > 0.3f) c.push_back(t);
            });
            return c;
        };

        for (size_t k = 0; k < lmCount; ++k) {
            if (!SnapsToCrease(in.landmarks[k].id)) continue;
            const Vector3 out = fittedN[in.landmarks[k].vertexIndex].Normalized();
            const Vector3 p0 = placed[k].point;
            int best = -1;
            float bestScore = -std::numeric_limits<float>::max();
            for (int t : candidates(p0, out)) {
                const float dv = depth(t, out);
                if (dv < params_.snapMinDepth) continue;
                const float score = dv - params_.snapDistancePenalty * (tv[t] - p0).Length() / r;
                if (score > bestScore) { bestScore = score; best = t; }
            }
            if (best >= 0) {
                placed[k].point = tv[best];
                placed[k].normal = targetN[best];
                placed[k].vertex = best;
            }
        }
    }

    std::unordered_map<std::string, size_t> byId;
    for (size_t k = 0; k < lmCount; ++k) byId[in.landmarks[k].id] = k;
    std::vector<float> pairError(lmCount, 0.0f);
    std::vector<bool> paired(lmCount, false);
    for (size_t a = 0; a < lmCount; ++a) {
        const auto& lm = in.landmarks[a];
        const auto it = lm.mirrorId.empty() ? byId.end() : byId.find(lm.mirrorId);
        if (it == byId.end()) {
            if (symmetric && lm.mirrorId.empty()) {
                const Vector3 p = placed[a].point;
                const Vector3 onPlane = p - planeN * (p - w0).Dot(planeN);
                const SurfaceHit h = Closest(bvh, tv, onPlane);
                if (h.found) placed[a] = h;
            }
            continue;
        }
        const size_t b = it->second;
        if (b <= a) continue;  // each pair once
        const Vector3 pa = placed[a].point;
        const Vector3 pbMirrored = reflect(placed[b].point);
        pairError[a] = pairError[b] = (pa - pbMirrored).Length();
        paired[a] = paired[b] = true;
        if (symmetric) {
            const Vector3 avg = (pa + pbMirrored) * 0.5f;
            const SurfaceHit ha = Closest(bvh, tv, avg);
            const SurfaceHit hb = Closest(bvh, tv, reflect(avg));
            if (ha.found && hb.found) { placed[a] = ha; placed[b] = hb; }
        }
    }

    // 5. Confidence. The main signal is how well the fitted surface around
    // each landmark lies on the target: where the template couldn't conform
    // (an oversized ear, a missing feature) the neighbourhood stays off the
    // surface even if the landmark itself happens to project close.
    const float residualRel = residual / diag;
    const float alignFactor = Clamp01(1.0f - (residualRel - 0.02f) / 0.04f);
    const float radius = params_.confidenceRadius * diag;
    std::vector<float> localResidual(lmCount, 0.0f);
    std::vector<float> localNormal(lmCount, 1.0f);
    std::vector<float> normalSpread(lmCount, 1.0f);
    #pragma omp parallel for schedule(dynamic, 1)
    for (int k = 0; k < static_cast<int>(lmCount); ++k) {
        const Vector3 c = fitted[in.landmarks[k].vertexIndex];
        std::vector<int> near;
        Vector3 normalSum;
        for (size_t i = 0; i < srcCount; ++i) {
            if (hasMask && in.mask[i]) continue;
            if ((fitted[i] - c).Length() <= radius) {
                near.push_back(static_cast<int>(i));
                normalSum = normalSum + fittedN[i].Normalized();
            }
        }
        // On a nearly flat patch (top of the skull, temple, cheek) nothing
        // pins the landmark's position along the surface.
        if (!near.empty()) {
            normalSpread[k] = 1.0f - normalSum.Length() / static_cast<float>(near.size());
        }
        const size_t step = std::max<size_t>(1, near.size() / 64);
        double sq = 0.0, dots = 0.0;
        int n = 0;
        for (size_t j = 0; j < near.size(); j += step) {
            const SurfaceHit h = Closest(bvh, tv, fitted[near[j]]);
            if (!h.found) continue;
            sq += static_cast<double>(h.distance) * h.distance;
            dots += std::abs(fittedN[near[j]].Normalized().Dot(h.normal));
            ++n;
        }
        if (n > 0) {
            localResidual[k] = static_cast<float>(std::sqrt(sq / n));
            localNormal[k] = static_cast<float>(dots / n);
        }
    }

    LandmarkSolveResult result;
    result.landmarks.reserve(lmCount);
    for (size_t k = 0; k < lmCount; ++k) {
        const float cDist = Clamp01(1.0f - (std::max(projDist[k], localResidual[k]) / diag) /
                                               params_.confidenceResidualZero);
        const float cNormal = Clamp01((std::min(normalAgree[k], localNormal[k]) - 0.3f) / 0.6f);
        const float cShape = 0.5f + 0.5f * Clamp01(normalSpread[k] / params_.distinctSpread);
        const float cMirror = (paired[k] && symmetric)
            ? Clamp01(1.0f - (pairError[k] / diag) / 0.02f) : 1.0f;
        SolvedLandmark s;
        s.landmarkId = in.landmarks[k].id;
        s.sourceVertex = in.landmarks[k].vertexIndex;
        s.targetPosition = placed[k].point;
        s.targetVertex = placed[k].vertex;
        s.confidence = std::sqrt(cDist) * std::pow(cNormal, 0.25f) * std::pow(cMirror, 0.25f) *
                       cShape * alignFactor;
        result.landmarks.push_back(std::move(s));
    }

    result.success = true;
    result.alignmentScale = static_cast<float>(scale);
    result.alignmentRotationDeg = angleDeg;
    result.alignmentResidual = residualRel;
    result.targetMirrorError = mirrorError;
    result.targetPlanePoint = w0;
    result.targetPlaneNormal = planeN;
    result.symmetryApplied = symmetric;
    result.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
    report(1.0f, "Done");
    MV_LOG_INFO(std::format("Auto-map placed {} landmarks in {:.1f} s (scale {:.3f}, rotation {:.1f} deg, "
                            "residual {:.4f}, mirror {:.4f}{})",
                            lmCount, result.seconds, scale, angleDeg, residualRel, mirrorError,
                            symmetric ? ", symmetric" : ""));
    return result;
}

} // namespace MetaVisage
