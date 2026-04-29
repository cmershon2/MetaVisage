// Story 4.6 — Wrap Engine behavioural-parity smoke.
//
// Loads the bundled MetaHuman head as both source and target, applies a
// deterministic Y-translation to the target Transform, places four
// correspondence anchors on hard-coded vertex indices, then runs the wrap
// for Quick Wrap and Fantasy Wrap. Asserts:
//
//   * Wrap converges (DeformationResult::success == true)
//   * Output vertex count matches input (catches retopology regressions)
//   * Output face count matches input
//   * maxDisplacement is positive (something happened)
//   * maxDisplacement is bounded — at most 5× the offset (catches wraps that
//     blow up to garbage geometry)
//   * avgDisplacement <= maxDisplacement (sanity)
//
// Catches the bug class Sprint 3.8.9 first shipped — preset numbers that
// look reasonable in the inspector but produce a visibly broken wrap. If
// either preset's params drift to a degenerate configuration, this smoke
// fires before the user opens the app.
//
// Each numbered step exits with a unique non-zero code so failure logs
// name the broken stage immediately.

#include "core/Mesh.h"
#include "core/Project.h"
#include "core/WrapPreset.h"
#include "deformation/MeshDeformer.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string LocateAsset(const std::string& exeDir, const std::string& relative) {
    namespace fs = std::filesystem;
    const fs::path candidates[] = {
        fs::path(exeDir) / relative,
        fs::path(exeDir) / ".." / ".." / "bin" / "Release" / relative,
        fs::path(exeDir) / ".." / ".." / "bin" / "Debug" / relative,
        fs::path(exeDir) / ".." / "bin" / "Release" / relative,
        fs::path(exeDir) / ".." / "bin" / "Debug" / relative,
    };
    for (const auto& p : candidates) {
        std::error_code ec;
        if (fs::exists(p, ec)) return fs::canonical(p, ec).generic_string();
    }
    return {};
}

// Vertex indices used as correspondence anchors. Mirrors
// tests/fixtures/wrap_smoke_points.json — kept in sync manually since the
// fixture is documentation; the test code is authoritative for build-time
// stability.
constexpr int kCorrespondenceVertexIndices[] = {0, 1500, 7500, 15000};

// Y-translation applied to the target mesh's Transform. Mesh-local units —
// the bundled head loads in centimetres so this is ~5 mm.
constexpr float kTargetOffsetY = 0.5f;

// Fail if maxDisplacement exceeds this multiple of the offset. A converged
// wrap should produce displacements close to the offset for landmarked
// vertices; a multiplier this generous accommodates non-landmark vertices
// that the algorithm might pull slightly farther in low-rigidity passes.
constexpr float kMaxDisplacementMultiplier = 5.0f;

struct PresetCase {
    const char* label;
    const MetaVisage::WrapPreset& (*GetPreset)();
};

int RunPreset(const std::string& meshPath,
              std::shared_ptr<MetaVisage::Mesh> head,
              const PresetCase& presetCase,
              int errorCodeBase) {
    using namespace MetaVisage;

    // Build a Project so the deformer setup matches StartMorph's path.
    Project project;
    project.SetName("smoke_wrap_parity");

    // Both meshes share the same Mesh shared_ptr. The deformer reads vertex
    // positions from each independently and applies their respective
    // Transforms — using the same ptr is fine because Deform() only reads.
    auto& morphRef = project.GetMorphMesh();
    morphRef.filepath = meshPath;
    morphRef.mesh = head;
    morphRef.isLoaded = true;
    // morphRef.transform = identity (default)

    auto& targetRef = project.GetTargetMesh();
    targetRef.filepath = meshPath;
    targetRef.mesh = head;
    targetRef.isLoaded = true;
    Transform targetXform;
    targetXform.SetPosition(Vector3(0.0f, kTargetOffsetY, 0.0f));
    targetRef.transform = targetXform;

    // Apply the preset to MorphData; this is the bug class the smoke is
    // built to catch — if WrapPreset::QuickWrap() / FantasyWrap() ever drift
    // to bad params, the wrap diverges and we fail here.
    presetCase.GetPreset().ApplyTo(project.GetMorphData());

    // Place four correspondence anchors on the same vertex indices on both
    // sides — landmarked vertices that the wrap should align almost exactly
    // to the target's translated position.
    auto& corrs = project.GetPointReferenceData().correspondences;
    corrs.clear();
    const auto& verts = head->GetVertices();
    int pointId = 0;
    for (int idx : kCorrespondenceVertexIndices) {
        if (idx < 0 || idx >= static_cast<int>(verts.size())) {
            std::fprintf(stderr,
                         "wrap_parity[%s]: vertex index %d out of range (max=%zu)\n",
                         presetCase.label, idx, verts.size() - 1);
            return errorCodeBase + 1;
        }
        PointCorrespondence c;
        c.pointID = pointId++;
        c.morphMeshVertexIndex = idx;
        c.morphMeshPosition    = verts[idx];
        c.targetMeshVertexIndex = idx;
        c.targetMeshPosition    = verts[idx]; // local-space; targetTransform offsets it
        c.isSymmetric = false;
        c.symmetricPairID = -1;
        corrs.push_back(c);
    }

    // Build NRICPParams from MorphData — mirrors AppWindow::StartMorph.
    const MorphData& m = project.GetMorphData();
    NRICPParams params;
    params.stiffnessSteps          = m.nricpStiffnessSteps;
    params.alphaInitial            = m.nricpAlphaInitial;
    params.alphaFinal              = m.nricpAlphaFinal;
    params.icpIterations           = m.nricpIcpIterations;
    params.normalThreshold         = m.nricpNormalThreshold;
    params.landmarkWeight          = m.nricpLandmarkWeight;
    params.epsilon                 = m.nricpEpsilon;
    params.enableBoundaryExclusion = m.nricpEnableBoundaryExclusion;
    params.boundaryExclusionHops   = m.nricpBoundaryExclusionHops;
    params.optimizationIterations  = m.nricpOptimizationIterations;
    params.dpInitial               = m.nricpDpInitial;
    params.dpFinal                 = m.nricpDpFinal;
    params.gammaInitial            = m.nricpGammaInitial;
    params.gammaFinal              = m.nricpGammaFinal;
    params.samplingInitial         = m.nricpSamplingInitial;
    params.samplingFinal           = m.nricpSamplingFinal;
    params.normalizeSampling       = m.nricpNormalizeSampling;

    MeshDeformer deformer;
    deformer.SetSourceMesh(head);
    deformer.SetTargetMesh(head);
    deformer.SetMorphTransform(morphRef.transform);
    deformer.SetTargetTransform(targetRef.transform);
    deformer.SetCorrespondences(corrs);
    deformer.SetStiffness(m.stiffness);
    deformer.SetSmoothness(m.smoothness);
    deformer.SetNRICPParams(params);

    const auto t0 = std::chrono::steady_clock::now();
    DeformationResult result = deformer.Deform();
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsedSec =
        std::chrono::duration<double>(t1 - t0).count();

    if (!result.success || !result.deformedMesh) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: Deform() failed: %s\n",
                     presetCase.label, result.errorMessage.c_str());
        return errorCodeBase + 2;
    }

    if (result.deformedMesh->GetVertexCount() != head->GetVertexCount()) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: vertex count drift %zu -> %zu\n",
                     presetCase.label,
                     head->GetVertexCount(),
                     result.deformedMesh->GetVertexCount());
        return errorCodeBase + 3;
    }

    if (result.deformedMesh->GetFaceCount() != head->GetFaceCount()) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: face count drift %zu -> %zu\n",
                     presetCase.label,
                     head->GetFaceCount(),
                     result.deformedMesh->GetFaceCount());
        return errorCodeBase + 4;
    }

    if (result.maxDisplacement <= 0.0f) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: maxDisplacement was %.4f (expected positive)\n",
                     presetCase.label, result.maxDisplacement);
        return errorCodeBase + 5;
    }

    const float maxAllowed = kTargetOffsetY * kMaxDisplacementMultiplier;
    if (result.maxDisplacement > maxAllowed) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: maxDisplacement=%.4f exceeded ceiling %.4f\n",
                     presetCase.label, result.maxDisplacement, maxAllowed);
        return errorCodeBase + 6;
    }

    if (result.avgDisplacement > result.maxDisplacement) {
        std::fprintf(stderr,
                     "wrap_parity[%s]: avg=%.4f exceeded max=%.4f (sanity)\n",
                     presetCase.label, result.avgDisplacement, result.maxDisplacement);
        return errorCodeBase + 7;
    }

    std::printf(
        "wrap_parity: %-13s converged in %.2fs — verts=%zu faces=%zu "
        "max=%.4f avg=%.4f\n",
        presetCase.label, elapsedSec,
        result.deformedMesh->GetVertexCount(),
        result.deformedMesh->GetFaceCount(),
        result.maxDisplacement,
        result.avgDisplacement);
    return 0;
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace MetaVisage;
    Logger::Instance().SetFileLogging(false); // keep CI stdout clean

    const std::string exeDir = Paths::ExecutableDir();
    const std::string headPath = LocateAsset(exeDir, "assets/models/MetaHumanHeadLod0.obj");
    if (headPath.empty()) {
        std::fprintf(stderr, "wrap_parity: bundled MetaHuman head not found near %s\n",
                     exeDir.c_str());
        return 1;
    }

    auto head = std::make_shared<Mesh>();
    if (!head->Load(headPath)) {
        std::fprintf(stderr, "wrap_parity: Mesh::Load failed for %s\n", headPath.c_str());
        return 2;
    }
    if (head->GetVertexCount() == 0 || head->GetFaceCount() == 0) {
        std::fprintf(stderr, "wrap_parity: loaded mesh has empty geometry\n");
        return 3;
    }

    const PresetCase presets[] = {
        {"Quick Wrap",   &WrapPreset::QuickWrap},
        {"Fantasy Wrap", &WrapPreset::FantasyWrap},
    };

    int code = 100;
    for (const auto& p : presets) {
        if (int rc = RunPreset(headPath, head, p, code); rc != 0) return rc;
        code += 10;
    }

    return 0;
}
