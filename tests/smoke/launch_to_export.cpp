// Story 3.7.6 — Smoke-test fixture.
//
// Exercises the load → serialize → reload → export pipeline against the
// bundled MetaHuman head, with no GLFW/ImGui/OpenGL. The point is to catch
// regressions in the core engine layers that survived the Qt6::Core port —
// Mesh, Project, ProjectSerializer, MeshExporter — without needing a GL
// context or a UI thread. Each numbered step exits with a unique non-zero
// code so CI failure logs name the broken stage immediately.
//
// Asset resolution: the test resolves paths relative to its own executable
// (tests/<config>/metavisage_smoke.exe) and the assets folder staged under
// build/bin/<config>/assets by CMake's asset-staging step.

#include "core/Mesh.h"
#include "core/Project.h"
#include "io/MeshExporter.h"
#include "io/ProjectSerializer.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string LocateAsset(const std::string& exeDir, const std::string& relative) {
    namespace fs = std::filesystem;
    // Try alongside the test exe first (build/tests/<config>/assets/...),
    // then alongside the main executable (build/bin/<config>/assets/...).
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

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    using namespace MetaVisage;
    Logger::Instance().SetFileLogging(false); // keep CI stdout clean

    std::ostringstream logOutput;
    auto* previousOutput = std::cout.rdbuf(logOutput.rdbuf());
    Logger::Instance().Log(LogLevel::Info, "Wrap Engine diagnostic",
                           "deformation/NRICPDeformer.cpp", 1);
    std::cout.rdbuf(previousOutput);
    if (logOutput.str().find("NRICP") != std::string::npos ||
        logOutput.str().find("WrapEngine.cpp:1") == std::string::npos) {
        std::fprintf(stderr, "smoke: diagnostic metadata exposes an internal implementation name\n");
        return 12;
    }

    const std::string exeDir = Paths::ExecutableDir();
    const std::string morphPath = LocateAsset(exeDir, "assets/models/MetaHumanHeadLod0.obj");
    if (morphPath.empty()) {
        std::fprintf(stderr, "smoke: default MetaHuman head not found near %s\n", exeDir.c_str());
        return 1;
    }

    // 1. Load the bundled morph mesh.
    auto morph = std::make_shared<Mesh>();
    if (!morph->Load(morphPath)) {
        std::fprintf(stderr, "smoke: Mesh::Load failed for %s\n", morphPath.c_str());
        return 2;
    }
    if (morph->GetVertexCount() == 0) {
        std::fprintf(stderr, "smoke: loaded mesh has 0 vertices\n");
        return 3;
    }
    if (morph->GetFaceCount() == 0) {
        std::fprintf(stderr, "smoke: loaded mesh has 0 faces\n");
        return 4;
    }

    // 2. Save → reload via ProjectSerializer to exercise the nlohmann::ordered_json
    //    round-trip that replaced the QJsonDocument path.
    namespace fs = std::filesystem;
    const fs::path workDir = fs::temp_directory_path() / "metavisage_smoke";
    fs::create_directories(workDir);
    const std::string projectPath = (workDir / "smoke.mmproj").generic_string();

    Project project;
    project.SetName("smoke");
    auto& morphRef = project.GetMorphMesh();
    morphRef.filepath = morphPath;
    morphRef.mesh = morph;
    morphRef.isLoaded = true;

    {
        ProjectSerializer s;
        auto r = s.Save(project, projectPath);
        if (!r.success) {
            std::fprintf(stderr, "smoke: ProjectSerializer::Save failed: %s\n", r.errorMessage.c_str());
            return 5;
        }
    }

    Project reloaded;
    {
        ProjectSerializer s;
        auto r = s.Load(reloaded, projectPath);
        if (!r.success) {
            std::fprintf(stderr, "smoke: ProjectSerializer::Load failed: %s\n", r.errorMessage.c_str());
            return 6;
        }
    }

    if (reloaded.GetName() != "smoke") {
        std::fprintf(stderr, "smoke: reloaded project name mismatch (%s)\n",
                     reloaded.GetName().c_str());
        return 7;
    }
    if (!reloaded.GetMorphMesh().isLoaded || !reloaded.GetMorphMesh().mesh) {
        std::fprintf(stderr, "smoke: reloaded morph mesh not loaded\n");
        return 8;
    }
    if (reloaded.GetMorphMesh().mesh->GetVertexCount() != morph->GetVertexCount()) {
        std::fprintf(stderr, "smoke: vertex count mismatch after round-trip\n");
        return 9;
    }

    // 3. Export to FBX. This is the format users actually take to UE5; the
    //    Assimp exporter path is what end-of-Sprint smoke tests have caught
    //    breakage in historically.
    const std::string fbxPath = (workDir / "smoke.fbx").generic_string();
    {
        ExportOptions opts;
        opts.format = ExportFormat::FBX;
        opts.triangulate = false; // MetaHuman Conform requires quad topology
        opts.applyTransform = false;
        opts.includeMaterials = false;

        MeshExporter exp;
        const ExportResult er = exp.Export(*reloaded.GetMorphMesh().mesh, fbxPath, opts);
        if (!er.success) {
            std::fprintf(stderr, "smoke: MeshExporter::Export(FBX) failed: %s\n",
                         er.errorMessage.c_str());
            return 10;
        }
    }

    std::error_code ec;
    const auto fbxSize = fs::file_size(fbxPath, ec);
    if (ec || fbxSize < 1024) {
        std::fprintf(stderr, "smoke: exported FBX is missing or implausibly small (%llu bytes)\n",
                     static_cast<unsigned long long>(fbxSize));
        return 11;
    }

    std::printf("smoke: OK — verts=%zu faces=%zu fbx=%llu bytes\n",
                morph->GetVertexCount(), morph->GetFaceCount(),
                static_cast<unsigned long long>(fbxSize));
    return 0;
}
