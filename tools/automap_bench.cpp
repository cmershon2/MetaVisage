// Sprint 12 — auto-map dataset benchmark.
//
//   metavisage_automap_bench <dataset folder> [--out <folder>]
//
// Every .mmproj under the dataset folder is a case: a project whose source is
// a source profile (normally the built-in MetaHuman head) and whose target is
// aligned in Stage 1. If the project also has target points placed for the
// profile's landmarks, those are the reference answer and the report includes
// errors. For each case the tool writes <case>.automap.mmproj (the project
// with the solved points, to inspect in Stage 2) and a row in report.md and
// report.csv; per-landmark rows go to landmarks.csv. See
// tests/datasets/README.md.

#include "core/Mesh.h"
#include "core/Project.h"
#include "core/SourceProfile.h"
#include "deformation/TemplateFitSolver.h"
#include "io/ProfileStore.h"
#include "io/ProjectSerializer.h"
#include "utils/Logger.h"
#include "utils/Paths.h"
#include "utils/RayCaster.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace MetaVisage;
namespace fs = std::filesystem;

namespace {

struct CaseRow {
    std::string name;
    bool solved = false;
    std::string error;
    double seconds = 0.0;
    int high = 0, review = 0, low = 0;
    float meanConfidence = 0.0f;
    bool hasTruth = false;
    float median = 0.0f, p90 = 0.0f, worst = 0.0f;  // percent of target diagonal
    float medianUnits = 0.0f;
    std::string worstNames;
    float scale = 1.0f, rotation = 0.0f, residual = 0.0f, mirror = 0.0f;
    bool symmetric = false;
};

float Percentile(std::vector<float> v, float q) {
    if (v.empty()) return 0.0f;
    std::sort(v.begin(), v.end());
    const size_t i = std::min(v.size() - 1, static_cast<size_t>(q * (v.size() - 1) + 0.5f));
    return v[i];
}

std::string Band(float c) { return c >= 0.75f ? "high" : (c >= 0.4f ? "review" : "low"); }

} // namespace

int main(int argc, char** argv) {
    Logger::Instance().SetFileLogging(false);
    if (argc < 2) {
        std::fprintf(stderr, "usage: metavisage_automap_bench <dataset folder> [--out <folder>]\n");
        return 2;
    }
    const fs::path dataset = fs::path(argv[1]);
    fs::path outDir = dataset / "automap-results";
    TemplateFitSolver::Params solverParams;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) outDir = fs::path(argv[++i]);
        else if (arg == "--no-refit") solverParams.featureRefit = false;
        else if (arg == "--no-snap") solverParams.featureSnap = false;
        else if (arg == "--regions" && i + 1 < argc) {
            solverParams.refitRegions.clear();
            std::stringstream list(argv[++i]);
            for (std::string r; std::getline(list, r, ',');) solverParams.refitRegions.push_back(r);
        }
        else if (arg == "--set" && i + 1 < argc) {
            // Tuning only: --set name=value for a few float parameters.
            const std::string kv = argv[++i];
            const size_t eq = kv.find('=');
            const std::string key = kv.substr(0, eq);
            char* end = nullptr;
            const std::string text = eq == std::string::npos ? std::string() : kv.substr(eq + 1);
            const float value = std::strtof(text.c_str(), &end);
            if (text.empty() || *end != '\0') {
                std::fprintf(stderr, "automap_bench: --set needs name=number, got %s\n", kv.c_str());
                return 2;
            }
            const std::map<std::string, float*> fields = {
                {"snapRadius", &solverParams.snapRadius},
                {"snapScale", &solverParams.snapScale},
                {"snapMinDepth", &solverParams.snapMinDepth},
                {"snapDistancePenalty", &solverParams.snapDistancePenalty},
                {"regionPatchScale", &solverParams.regionPatchScale},
                {"maxRegionStretch", &solverParams.maxRegionStretch},
                {"maxRegionShift", &solverParams.maxRegionShift},
                {"regionRegularization", &solverParams.regionRegularization},
            };
            const auto it = fields.find(key);
            if (it == fields.end()) {
                std::fprintf(stderr, "automap_bench: unknown parameter %s\n", key.c_str());
                return 2;
            }
            *it->second = value;
        }
    }
    std::error_code ec;
    if (!fs::is_directory(dataset, ec)) {
        std::fprintf(stderr, "automap_bench: %s is not a folder\n", dataset.string().c_str());
        return 2;
    }
    fs::create_directories(outDir, ec);

    std::vector<SourceProfile> profiles = ProfileStore::LoadAll();
    if (profiles.empty()) {
        std::fprintf(stderr, "automap_bench: no source profiles found next to the executable\n");
        return 3;
    }
    std::map<std::string, std::shared_ptr<Mesh>> profileMeshes;

    std::vector<fs::path> projects;
    for (const auto& entry : fs::recursive_directory_iterator(dataset, ec)) {
        const fs::path& p = entry.path();
        if (!entry.is_regular_file() || p.extension() != ".mmproj") continue;
        if (p.filename().string().find(".automap.") != std::string::npos) continue;
        if (fs::equivalent(p.parent_path(), outDir, ec)) continue;
        projects.push_back(p);
    }
    std::sort(projects.begin(), projects.end());
    if (projects.empty()) {
        std::fprintf(stderr, "automap_bench: no .mmproj files under %s\n", dataset.string().c_str());
        return 4;
    }

    std::ofstream lmCsv(outDir / "landmarks.csv");
    lmCsv << "case,landmark,confidence,band,error_units,error_pct_diag\n";
    std::vector<CaseRow> rows;
    std::map<std::string, std::vector<float>> errorByBand;

    for (const fs::path& projectPath : projects) {
        CaseRow row;
        row.name = projectPath.stem().string();
        std::printf("automap_bench: %s\n", row.name.c_str());

        Project project;
        ProjectSerializer serializer;
        const SerializationResult loaded = serializer.Load(project, projectPath.string());
        const MeshReference& tgtRef = project.GetTargetMesh();
        const SourceProfile* profile = ProfileStore::FindById(profiles, project.GetSourceProfileId());
        if (!loaded.success) row.error = "project failed to load: " + loaded.errorMessage;
        else if (!tgtRef.isLoaded || !tgtRef.mesh) row.error = "target mesh not loaded";
        else if (!profile) row.error = "source profile '" + project.GetSourceProfileId() + "' not installed";
        else if (!profile->HasLandmarks()) row.error = "source profile has no landmarks";
        if (!row.error.empty()) {
            std::printf("  skipped: %s\n", row.error.c_str());
            rows.push_back(row);
            continue;
        }

        auto& srcMesh = profileMeshes[profile->id];
        if (!srcMesh) {
            srcMesh = std::make_shared<Mesh>();
            if (!srcMesh->Load(profile->resolvedMeshPath)) srcMesh.reset();
        }
        if (!srcMesh || TopologyFingerprint::FromMesh(*srcMesh) != profile->fingerprint) {
            row.error = "profile mesh missing or does not match its fingerprint";
            rows.push_back(row);
            continue;
        }

        LandmarkSolveInput in;
        in.source = srcMesh;
        in.sourceTransform = project.GetMorphMesh().transform;
        in.target = tgtRef.mesh;
        in.targetTransform = tgtRef.transform;
        in.landmarks = profile->landmarks;
        in.symmetryAxis = profile->symmetryAxis;
        in.symmetryPlaneOffset = profile->symmetryPlaneOffset;
        in.mask = project.GetMorphData().vertexMask.size() == srcMesh->GetVertexCount()
            ? project.GetMorphData().vertexMask : profile->BuildMask();

        TemplateFitSolver solver(solverParams);
        std::atomic<bool> cancel{false};
        const LandmarkSolveResult result = solver.Solve(in, nullptr, cancel);
        row.seconds = result.seconds;
        row.scale = result.alignmentScale;
        row.rotation = result.alignmentRotationDeg;
        row.residual = result.alignmentResidual * 100.0f;
        row.mirror = result.targetMirrorError * 100.0f;
        row.symmetric = result.symmetryApplied;
        if (!result.success) {
            row.error = result.errorMessage;
            std::printf("  failed: %s\n", row.error.c_str());
            rows.push_back(row);
            continue;
        }
        row.solved = true;

        // Reference answer: target points paired with a profile landmark's source vertex.
        std::unordered_map<int, Vector3> truth;
        for (const auto& c : project.GetPointReferenceData().correspondences) {
            if (c.morphMeshVertexIndex >= 0 && c.targetMeshVertexIndex >= 0) {
                truth[c.morphMeshVertexIndex] = c.targetMeshPosition;
            }
        }
        const Matrix4x4 tgtM = tgtRef.transform.GetMatrix();
        Vector3 lo = RayCaster::TransformPoint(tgtRef.mesh->GetVertices()[0], tgtM), hi = lo;
        for (const auto& v : tgtRef.mesh->GetVertices()) {
            const Vector3 w = RayCaster::TransformPoint(v, tgtM);
            lo = {std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z)};
            hi = {std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z)};
        }
        const float diag = (hi - lo).Length();

        std::unordered_map<std::string, std::string> names;
        for (const auto& lm : profile->landmarks) names[lm.id] = lm.name;
        std::vector<float> errors, errorUnits;
        std::vector<std::pair<float, std::string>> ranked;
        float confSum = 0.0f;
        for (const auto& s : result.landmarks) {
            confSum += s.confidence;
            const std::string band = Band(s.confidence);
            if (band == "high") ++row.high; else if (band == "review") ++row.review; else ++row.low;
            std::string errUnits, errPct;
            if (const auto it = truth.find(s.sourceVertex); it != truth.end()) {
                const float e = (s.targetPosition - it->second).Length();
                errors.push_back(e / diag * 100.0f);
                errorUnits.push_back(e);
                ranked.push_back({e / diag * 100.0f, names[s.landmarkId]});
                errorByBand[band].push_back(e / diag * 100.0f);
                errUnits = std::format("{:.4f}", e);
                errPct = std::format("{:.4f}", e / diag * 100.0f);
            }
            lmCsv << std::format("{},{},{:.3f},{},{},{}\n", row.name, s.landmarkId, s.confidence, band,
                                 errUnits, errPct);
        }
        row.meanConfidence = confSum / static_cast<float>(result.landmarks.size());
        if (!errors.empty()) {
            row.hasTruth = true;
            row.median = Percentile(errors, 0.5f);
            row.p90 = Percentile(errors, 0.9f);
            row.worst = Percentile(errors, 1.0f);
            row.medianUnits = Percentile(errorUnits, 0.5f);
            std::sort(ranked.rbegin(), ranked.rend());
            for (size_t i = 0; i < std::min<size_t>(3, ranked.size()); ++i) {
                row.worstNames += std::format("{}{} ({:.2f}%)", i ? ", " : "", ranked[i].second, ranked[i].first);
            }
        }

        // The project with the solved points, for inspection in Stage 2.
        std::unordered_map<std::string, int> pointIdByLandmark;
        for (size_t k = 0; k < result.landmarks.size(); ++k) {
            pointIdByLandmark[result.landmarks[k].landmarkId] = static_cast<int>(k) + 1;
        }
        const Matrix4x4 srcM = in.sourceTransform.GetMatrix();
        std::vector<PointCorrespondence> points;
        for (size_t k = 0; k < result.landmarks.size(); ++k) {
            const auto& s = result.landmarks[k];
            PointCorrespondence pc;
            pc.pointID = static_cast<int>(k) + 1;
            pc.morphMeshVertexIndex = s.sourceVertex;
            pc.morphMeshPosition = RayCaster::TransformPoint(srcMesh->GetVertices()[s.sourceVertex], srcM);
            pc.targetMeshVertexIndex = s.targetVertex;
            pc.targetMeshPosition = s.targetPosition;
            const std::string& mirror = profile->landmarks[k].mirrorId;
            if (const auto it = pointIdByLandmark.find(mirror); !mirror.empty() && it != pointIdByLandmark.end()) {
                pc.isSymmetric = true;
                pc.symmetricPairID = it->second;
            }
            points.push_back(pc);
        }
        project.GetPointReferenceData().correspondences = points;
        project.GetMorphMesh().mesh = srcMesh;
        project.GetMorphMesh().filepath = profile->resolvedMeshPath;
        project.SetCurrentStage(WorkflowStage::PointReference);
        const fs::path outProject = outDir / (row.name + ".automap.mmproj");
        const SerializationResult saved = serializer.Save(project, outProject.string());
        if (!saved.success) std::printf("  could not write %s\n", outProject.string().c_str());

        std::printf("  %.1f s  confidence %d high / %d review / %d low", row.seconds, row.high, row.review, row.low);
        if (row.hasTruth) std::printf("  error median %.2f%%  p90 %.2f%%  worst %.2f%%", row.median, row.p90, row.worst);
        std::printf("\n");
        rows.push_back(row);
    }

    std::ofstream csv(outDir / "report.csv");
    csv << "case,solved,seconds,high,review,low,mean_confidence,median_pct,p90_pct,worst_pct,median_units,"
           "scale,rotation_deg,residual_pct,mirror_pct,symmetric,error\n";
    std::ofstream md(outDir / "report.md");
    md << "# Auto-map benchmark\n\n"
       << "Errors are distances from each solved point to the hand-placed reference, as a percentage "
          "of the target's bounding-box diagonal. Confidence bands: high >= 0.75, review 0.4-0.75, low < 0.4.\n\n"
       << "| Case | Time | High / review / low | Median | P90 | Worst | Worst landmarks | Scale | Rotation | Symmetric |\n"
       << "|---|---|---|---|---|---|---|---|---|---|\n";
    for (const auto& r : rows) {
        csv << std::format("{},{},{:.2f},{},{},{},{:.3f},{:.3f},{:.3f},{:.3f},{:.4f},{:.4f},{:.2f},{:.3f},{:.3f},{},\"{}\"\n",
                           r.name, r.solved ? 1 : 0, r.seconds, r.high, r.review, r.low, r.meanConfidence,
                           r.median, r.p90, r.worst, r.medianUnits, r.scale, r.rotation, r.residual, r.mirror,
                           r.symmetric ? 1 : 0, r.error);
        if (!r.solved) {
            md << std::format("| {} | — | failed: {} | | | | | | | |\n", r.name, r.error);
            continue;
        }
        md << std::format("| {} | {:.1f} s | {} / {} / {} | {} | {} | {} | {} | {:.3f} | {:.1f}° | {} |\n",
                          r.name, r.seconds, r.high, r.review, r.low,
                          r.hasTruth ? std::format("{:.2f}%", r.median) : "—",
                          r.hasTruth ? std::format("{:.2f}%", r.p90) : "—",
                          r.hasTruth ? std::format("{:.2f}%", r.worst) : "—",
                          r.worstNames, r.scale, r.rotation, r.symmetric ? "yes" : "no");
    }
    md << "\n## Error by confidence band\n\n| Band | Landmarks | Median error | P90 error |\n|---|---|---|---|\n";
    for (const char* band : {"high", "review", "low"}) {
        const auto& v = errorByBand[band];
        md << std::format("| {} | {} | {:.2f}% | {:.2f}% |\n", band, v.size(), Percentile(v, 0.5f), Percentile(v, 0.9f));
    }
    std::printf("automap_bench: wrote %s\n", (outDir / "report.md").string().c_str());
    return 0;
}
