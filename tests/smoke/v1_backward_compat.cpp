// Story 4.1 — Backward-compat regression for v1.0.0 .mmproj files.
//
// v1.0.0 wrote `morph.algorithm` as one of:
//   0 = RBF_TPS         1 = RBF_GAUSSIAN
//   2 = RBF_MULTIQUADRIC 3 = ARAP    4 = NRICP
//
// Sprint 4 collapsed `DeformationAlgorithm` to a single member (NRICP) and
// the serializer must silently coerce any of the discontinued ordinals back
// to NRICP so old projects keep loading without warnings.
//
// Each fixture exercises one historical ordinal. The test asserts:
//   * Load succeeds (no error message)
//   * No warnings raised (silent coercion)
//   * Final algorithm == NRICP
//
// Each numbered step exits with a unique non-zero code so failure logs name
// the broken stage immediately.

#include "core/Project.h"
#include "io/ProjectSerializer.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {

std::string LocateFixture(const std::string& exeDir, const std::string& filename) {
    namespace fs = std::filesystem;
    // Fixtures live alongside the source tree at tests/fixtures/. The test
    // runs from build/<config>/ so we walk up to find the repo root.
    const fs::path candidates[] = {
        fs::path(exeDir) / "fixtures" / filename,
        fs::path(exeDir) / ".." / "tests" / "fixtures" / filename,
        fs::path(exeDir) / ".." / ".." / "tests" / "fixtures" / filename,
        fs::path(exeDir) / ".." / ".." / ".." / "tests" / "fixtures" / filename,
        fs::path(exeDir) / ".." / ".." / ".." / ".." / "tests" / "fixtures" / filename,
    };
    for (const auto& p : candidates) {
        std::error_code ec;
        if (fs::exists(p, ec)) return fs::canonical(p, ec).generic_string();
    }
    return {};
}

struct Fixture {
    const char* filename;
    const char* label;
    int historicalOrdinal;
};

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace MetaVisage;
    Logger::Instance().SetFileLogging(false); // keep CI stdout clean

    const std::string exeDir = Paths::ExecutableDir();

    const Fixture fixtures[] = {
        {"v1_project_rbf_tps.mmproj",          "RBF_TPS",          0},
        {"v1_project_rbf_gaussian.mmproj",     "RBF_GAUSSIAN",     1},
        {"v1_project_rbf_multiquadric.mmproj", "RBF_MULTIQUADRIC", 2},
        {"v1_project_arap.mmproj",             "ARAP",             3},
    };

    int stepBase = 1;
    for (const auto& fx : fixtures) {
        const std::string path = LocateFixture(exeDir, fx.filename);
        if (path.empty()) {
            std::fprintf(stderr, "v1_compat: fixture not found: %s (exeDir=%s)\n",
                         fx.filename, exeDir.c_str());
            return stepBase + 0;
        }

        Project project;
        ProjectSerializer serializer;
        auto result = serializer.Load(project, path);
        if (!result.success) {
            std::fprintf(stderr, "v1_compat: Load failed for %s (%s): %s\n",
                         fx.label, fx.filename, result.errorMessage.c_str());
            return stepBase + 1;
        }
        if (!result.warnings.empty()) {
            std::fprintf(stderr, "v1_compat: %s loaded with %zu warning(s) — first: %s\n",
                         fx.label, result.warnings.size(), result.warnings[0].c_str());
            return stepBase + 2;
        }
        if (project.GetMorphData().algorithm != DeformationAlgorithm::NRICP) {
            std::fprintf(stderr, "v1_compat: %s did not coerce to NRICP\n", fx.label);
            return stepBase + 3;
        }

        if (!project.GetSourceProfileId().empty()) {
            std::fprintf(stderr, "v1_compat: %s loaded a non-empty source profile id\n", fx.label);
            return stepBase + 4;
        }

        std::printf("v1_compat: %-18s (ordinal %d) -> NRICP OK\n",
                    fx.label, fx.historicalOrdinal);
        stepBase += 10;
    }

    // Sprint 11 — format v2 (pre-Source-Profile) fixture. Loads with an empty
    // profile id, and the vertex mask plus the optimization / rigidity /
    // sampling parameters survive the load (they were dropped before
    // Sprint 11).
    {
        const std::string path = LocateFixture(exeDir, "v2_project.mmproj");
        if (path.empty()) {
            std::fprintf(stderr, "v1_compat: fixture not found: v2_project.mmproj\n");
            return 100;
        }
        Project project;
        ProjectSerializer serializer;
        auto result = serializer.Load(project, path);
        if (!result.success || !result.warnings.empty()) {
            std::fprintf(stderr, "v1_compat: v2 fixture load failed or warned: %s\n",
                         result.success ? result.warnings[0].c_str() : result.errorMessage.c_str());
            return 101;
        }
        if (!project.GetSourceProfileId().empty()) {
            std::fprintf(stderr, "v1_compat: v2 fixture has a non-empty source profile id\n");
            return 102;
        }
        const MorphData& m = project.GetMorphData();
        const std::vector<bool> expectedMask = {false, false, true, true, true, false};
        if (m.vertexMask != expectedMask) {
            std::fprintf(stderr, "v1_compat: v2 fixture vertex mask not restored (size %zu)\n",
                         m.vertexMask.size());
            return 103;
        }
        if (m.nricpOptimizationIterations != 3 || m.nricpGammaInitial != 2.0f ||
            m.nricpSamplingFinal != 0.05f || m.nricpNormalizeSampling) {
            std::fprintf(stderr, "v1_compat: v2 fixture wrap parameters not restored\n");
            return 104;
        }
        if (project.GetPointReferenceData().correspondences.size() != 1) {
            std::fprintf(stderr, "v1_compat: v2 fixture points not restored\n");
            return 105;
        }
        // Sprint 13 — files without the auto-map fields load as manual points.
        const PointReferenceData& refData = project.GetPointReferenceData();
        const PointCorrespondence& old = refData.correspondences[0];
        if (!old.landmarkId.empty() || old.isAuto || old.confidence >= 0.0f ||
            old.midlineUnlocked || refData.hasTargetPlane || !refData.mirrorDrag ||
            !refData.showConfidence) {
            std::fprintf(stderr, "v1_compat: v2 fixture point picked up auto-map fields\n");
            return 106;
        }
        std::printf("v1_compat: v2 fixture -> mask, parameters, points OK\n");
    }

    // Sprint 11 — format v3 round-trip of sourceProfileId and the mask.
    {
        namespace fs = std::filesystem;
        const fs::path tmp = fs::temp_directory_path() / "metavisage_v3_roundtrip.mmproj";
        Project original;
        original.SetSourceProfileId("metahuman-head-lod0");
        original.GetMorphData().vertexMask = {true, false, false, true};
        // Sprint 13 — landmark fields on points, and the auto-map plane.
        PointReferenceData& refData = original.GetPointReferenceData();
        PointCorrespondence autoPoint;
        autoPoint.pointID = 1;
        autoPoint.landmarkId = "nose_tip";
        autoPoint.isAuto = true;
        autoPoint.confidence = 0.625f;
        autoPoint.midlineUnlocked = true;
        PointCorrespondence manualPoint;
        manualPoint.pointID = 2;
        refData.correspondences = {autoPoint, manualPoint};
        refData.hasTargetPlane = true;
        refData.targetPlanePoint = Vector3(1.0f, 2.0f, 3.0f);
        refData.targetPlaneNormal = Vector3(0.0f, 0.0f, 1.0f);
        refData.mirrorDrag = false;
        refData.showConfidence = false;
        ProjectSerializer serializer;
        if (!serializer.Save(original, tmp.string()).success) {
            std::fprintf(stderr, "v1_compat: v3 save failed\n");
            return 110;
        }
        Project reloaded;
        const auto result = serializer.Load(reloaded, tmp.string());
        std::error_code ec;
        fs::remove(tmp, ec);
        if (!result.success) {
            std::fprintf(stderr, "v1_compat: v3 reload failed: %s\n", result.errorMessage.c_str());
            return 111;
        }
        if (reloaded.GetSourceProfileId() != "metahuman-head-lod0" ||
            reloaded.GetMorphData().vertexMask != original.GetMorphData().vertexMask) {
            std::fprintf(stderr, "v1_compat: v3 round-trip lost the profile id or mask\n");
            return 112;
        }
        const PointReferenceData& back = reloaded.GetPointReferenceData();
        if (back.correspondences.size() != 2 || !back.hasTargetPlane || back.mirrorDrag || back.showConfidence ||
            back.targetPlanePoint.y != 2.0f || back.targetPlaneNormal.z != 1.0f) {
            std::fprintf(stderr, "v1_compat: v3 round-trip lost the auto-map plane or settings\n");
            return 113;
        }
        const PointCorrespondence& a = back.correspondences[0];
        const PointCorrespondence& m = back.correspondences[1];
        if (a.landmarkId != "nose_tip" || !a.isAuto || a.confidence != 0.625f || !a.midlineUnlocked ||
            !m.landmarkId.empty() || m.isAuto || m.confidence >= 0.0f || m.midlineUnlocked) {
            std::fprintf(stderr, "v1_compat: v3 round-trip lost point landmark fields\n");
            return 114;
        }
        std::printf("v1_compat: v3 round-trip -> profile id, mask, landmark fields OK\n");
    }

    return 0;
}
