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

        std::printf("v1_compat: %-18s (ordinal %d) -> NRICP OK\n",
                    fx.label, fx.historicalOrdinal);
        stepBase += 10;
    }

    return 0;
}
