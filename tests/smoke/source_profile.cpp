// Sprint 11 — Source Profile smoke test.
//
// Headless checks for the Source Profile model and ProfileStore:
//   1. Topology fingerprint: stable across positions, sensitive to topology.
//   2. Mask range round-trip.
//   3. Profile validation and the canonical landmark set.
//   4. ProfileStore save / load / delete in a temporary directory, including
//      malformed, unknown-schema and mis-filed profiles being skipped.
//   5. Dev draft export: generic and canonical landmark naming.
//   6. The shipped built-in MetaHuman profile loads and matches the bundled
//      head's topology.
//   7. Informational: whether an FBX round-trip of the head keeps the same
//      fingerprint (Sprint 11 risk measurement; does not fail the test).
//
// Each step exits with a unique non-zero code so failure logs name the
// broken stage immediately.

#include "core/Mesh.h"
#include "core/Project.h"
#include "core/SourceProfile.h"
#include "io/MeshExporter.h"
#include "io/ProfileStore.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace MetaVisage;
namespace fs = std::filesystem;

std::string LocateAsset(const std::string& exeDir, const std::string& relative) {
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

std::shared_ptr<Mesh> MakeQuadStrip(float offset) {
    // Two quads sharing an edge: 6 vertices, 2 faces.
    auto mesh = std::make_shared<Mesh>();
    mesh->SetVertices({Vector3(0 + offset, 0, 0), Vector3(1, 0, 0), Vector3(2, 0, 0),
                       Vector3(0, 1, 0), Vector3(1, 1, 0), Vector3(2, 1, 0)});
    std::vector<Face> faces(2);
    faces[0].vertexIndices = {0, 1, 4, 3};
    faces[1].vertexIndices = {1, 2, 5, 4};
    mesh->SetFaces(faces);
    return mesh;
}

SourceProfile MakeUserProfile(const std::string& id) {
    SourceProfile p;
    p.id = id;
    p.name = "Test head";
    p.meshPath = "head.obj";
    p.fingerprint = TopologyFingerprint::FromMesh(*MakeQuadStrip(0.0f));
    p.landmarks = {{"a_l", "A (L)", 0, "a_r", "eye"}, {"a_r", "A (R)", 2, "a_l", "eye"},
                   {"mid", "Mid", 4, "", "nose"}};
    p.maskRanges = {{1, 1}, {3, 5}};
    p.defaultWrapPresetId = "quick";
    return p;
}

void WriteText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    Logger::Instance().SetFileLogging(false);

    // 1. Fingerprint.
    {
        const auto a = TopologyFingerprint::FromMesh(*MakeQuadStrip(0.0f));
        const auto moved = TopologyFingerprint::FromMesh(*MakeQuadStrip(5.0f));
        if (!a.IsValid() || a != moved) {
            std::fprintf(stderr, "source_profile: fingerprint changed with positions only\n");
            return 1;
        }
        auto reordered = MakeQuadStrip(0.0f);
        std::vector<Face> faces = reordered->GetFaces();
        std::swap(faces[0], faces[1]);
        reordered->SetFaces(faces);
        if (TopologyFingerprint::FromMesh(*reordered) == a) {
            std::fprintf(stderr, "source_profile: fingerprint ignored face order\n");
            return 2;
        }
        auto triangulated = MakeQuadStrip(0.0f);
        std::vector<Face> tris(2);
        tris[0].vertexIndices = {0, 1, 4};
        tris[1].vertexIndices = {3, 1, 2, 5, 4};
        triangulated->SetFaces(tris);
        if (TopologyFingerprint::FromMesh(*triangulated) == a) {
            std::fprintf(stderr, "source_profile: fingerprint ignored face sizes\n");
            return 3;
        }
        std::uint64_t parsed = 0;
        if (!TopologyFingerprint::ParseHashHex(a.HashHex(), parsed) || parsed != a.faceHash ||
            TopologyFingerprint::ParseHashHex("xyz", parsed)) {
            std::fprintf(stderr, "source_profile: hash hex round-trip failed\n");
            return 4;
        }
    }

    // 2. Mask ranges.
    {
        const std::vector<bool> mask = {true, false, false, true, true, true, false, true};
        const auto ranges = MaskRanges::FromMask(mask);
        const std::vector<MaskRange> expected = {{0, 0}, {3, 5}, {7, 7}};
        if (ranges != expected || MaskRanges::ToMask(ranges, mask.size()) != mask) {
            std::fprintf(stderr, "source_profile: mask range round-trip failed\n");
            return 10;
        }
        const auto clamped = MaskRanges::ToMask({{-3, 1}, {6, 99}}, 8);
        const std::vector<bool> expectedClamped = {true, true, false, false, false, false, true, true};
        if (clamped != expectedClamped) {
            std::fprintf(stderr, "source_profile: mask ranges not clamped\n");
            return 11;
        }
    }

    // 3. Validation and canonical landmarks.
    {
        SourceProfile good = MakeUserProfile("user-0000abcd");
        if (!good.Validate().empty()) {
            std::fprintf(stderr, "source_profile: valid profile reported problems: %s\n",
                         good.Validate().front().c_str());
            return 20;
        }
        SourceProfile bad = good;
        bad.landmarks.push_back({"mid", "Duplicate", 99, "a_l", ""});
        bad.maskRanges.push_back({4, 6});
        // duplicate id + out-of-range vertex + non-reciprocal mirror + bad range
        if (bad.Validate().size() < 4) {
            std::fprintf(stderr, "source_profile: validation missed problems (%zu found)\n",
                         bad.Validate().size());
            return 21;
        }
        const auto& canonical = SourceProfile::CanonicalLandmarks();
        if (canonical.size() != 52) {
            std::fprintf(stderr, "source_profile: canonical set has %zu landmarks\n",
                         canonical.size());
            return 22;
        }
        SourceProfile full;
        full.fingerprint.vertexCount = 52;
        full.fingerprint.faceCount = 1;
        full.landmarks = canonical;
        for (size_t i = 0; i < full.landmarks.size(); ++i) {
            full.landmarks[i].vertexIndex = static_cast<int>(i);
        }
        if (!full.Validate().empty()) {
            std::fprintf(stderr, "source_profile: canonical set invalid: %s\n",
                         full.Validate().front().c_str());
            return 23;
        }
    }

    // 4. ProfileStore against a temporary user directory.
    const fs::path tmp = fs::temp_directory_path() / "metavisage_source_profile_smoke";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);
    {
        const std::string dir = tmp.generic_string();
        const std::string id = ProfileStore::GenerateUserProfileId();
        if (!ProfileStore::IsValidUserProfileId(id)) {
            std::fprintf(stderr, "source_profile: generated id '%s' fails its own check\n", id.c_str());
            return 30;
        }
        SourceProfile p = MakeUserProfile(id);
        if (!ProfileStore::Save(p, dir)) {
            std::fprintf(stderr, "source_profile: Save failed\n");
            return 31;
        }
        SourceProfile builtIn = p;
        builtIn.isBuiltIn = true;
        if (ProfileStore::Save(builtIn, dir) ||
            ProfileStore::Save(MakeUserProfile("../escape"), dir)) {
            std::fprintf(stderr, "source_profile: Save accepted a built-in or invalid id\n");
            return 32;
        }

        // Files the loader must skip.
        WriteText(tmp / "user-11111111" / "profile.mvprofile.json", "{ not json");
        WriteText(tmp / "user-22222222" / "profile.mvprofile.json",
                  "{\"schema_version\": 99, \"id\": \"user-22222222\"}");
        SourceProfile misfiled = MakeUserProfile("user-33333333");
        fs::create_directories(tmp / "user-44444444");
        if (!ProfileStore::WriteFile(misfiled,
                                     (tmp / "user-44444444" / "profile.mvprofile.json").string())) {
            std::fprintf(stderr, "source_profile: WriteFile failed\n");
            return 38;
        }

        const auto loaded = ProfileStore::LoadUserProfiles(dir);
        if (loaded.size() != 1 || loaded[0].id != id) {
            std::fprintf(stderr, "source_profile: expected exactly the saved profile, got %zu\n",
                         loaded.size());
            return 33;
        }
        const SourceProfile& r = loaded[0];
        const std::string expectedMesh = (tmp / id / "head.obj").lexically_normal().generic_string();
        if (r.isBuiltIn || r.resolvedMeshPath != expectedMesh || r.fingerprint != p.fingerprint ||
            r.landmarks.size() != 3 || r.landmarks[0].mirrorId != "a_r" ||
            r.maskRanges != p.maskRanges || r.defaultWrapPresetId != "quick" ||
            r.BuildMask() != std::vector<bool>({false, true, false, true, true, true})) {
            std::fprintf(stderr, "source_profile: saved profile did not round-trip "
                         "(mesh '%s')\n", r.resolvedMeshPath.c_str());
            return 34;
        }
        if (ProfileStore::FindById(loaded, id) != &loaded[0] ||
            ProfileStore::FindByFingerprint(loaded, p.fingerprint) != &loaded[0] ||
            ProfileStore::FindByFingerprint(loaded, TopologyFingerprint{}) != nullptr) {
            std::fprintf(stderr, "source_profile: lookup helpers failed\n");
            return 35;
        }

        if (ProfileStore::Delete("../user-11111111", dir) || !fs::exists(tmp / "user-11111111")) {
            std::fprintf(stderr, "source_profile: Delete accepted a traversal id\n");
            return 36;
        }
        if (!ProfileStore::Delete(id, dir) || fs::exists(tmp / id) ||
            !ProfileStore::Delete(id, dir)) {
            std::fprintf(stderr, "source_profile: Delete failed or is not idempotent\n");
            return 37;
        }
    }

    // 5. Dev draft export.
    {
        Project project;
        auto mesh = MakeQuadStrip(0.0f);
        project.GetMorphMesh().mesh = mesh;
        project.GetMorphMesh().filepath = (tmp / "My Head_01.obj").generic_string();
        project.GetMorphMesh().isLoaded = true;
        project.GetMorphData().vertexMask = {false, false, true, true, false, false};
        auto& corr = project.GetPointReferenceData().correspondences;
        PointCorrespondence c1; c1.pointID = 1; c1.morphMeshVertexIndex = 4;
        PointCorrespondence c2; c2.pointID = 2; c2.morphMeshVertexIndex = 0;
        c2.isSymmetric = true; c2.symmetricPairID = 3;
        PointCorrespondence c3; c3.pointID = 3; c3.morphMeshVertexIndex = 2;
        c3.isSymmetric = true; c3.symmetricPairID = 2;
        PointCorrespondence targetOnly; targetOnly.pointID = 4; targetOnly.targetMeshVertexIndex = 1;
        corr = {c3, c1, c2, targetOnly};

        const SourceProfile draft = SourceProfile::DraftFromProject(project, tmp.generic_string());
        if (draft.id != "my-head-01" || draft.meshPath != "My Head_01.obj" ||
            draft.fingerprint != TopologyFingerprint::FromMesh(*mesh) ||
            draft.landmarks.size() != 3 || draft.landmarks[0].id != "point_1" ||
            draft.landmarks[1].mirrorId != "point_3" ||
            draft.maskRanges != std::vector<MaskRange>({{2, 3}}) || !draft.Validate().empty()) {
            std::fprintf(stderr, "source_profile: generic draft wrong (id '%s', mesh '%s', %zu landmarks)\n",
                         draft.id.c_str(), draft.meshPath.c_str(), draft.landmarks.size());
            return 40;
        }

        // Canonical naming: 14 midline points, then 19 pairs placed with the
        // right-hand (negative X) side first to check L/R assignment.
        auto big = std::make_shared<Mesh>();
        std::vector<Vector3> verts;
        for (int i = 0; i < 14; ++i) verts.emplace_back(0.0f, static_cast<float>(i), 0.0f);
        for (int i = 0; i < 19; ++i) {
            verts.emplace_back(-1.0f, static_cast<float>(i), 0.0f);   // R
            verts.emplace_back(1.0f, static_cast<float>(i), 0.0f);    // L
        }
        big->SetVertices(verts);
        std::vector<Face> oneFace(1);
        oneFace[0].vertexIndices = {0, 1, 2};
        big->SetFaces(oneFace);
        Project canon;
        canon.GetMorphMesh().mesh = big;
        canon.GetMorphMesh().filepath = "head.obj";
        auto& cc = canon.GetPointReferenceData().correspondences;
        int id = 1;
        for (int i = 0; i < 14; ++i) {
            PointCorrespondence c; c.pointID = id++; c.morphMeshVertexIndex = i;
            cc.push_back(c);
        }
        for (int i = 0; i < 19; ++i) {
            PointCorrespondence r; r.pointID = id++; r.morphMeshVertexIndex = 14 + i * 2;
            PointCorrespondence l; l.pointID = id++; l.morphMeshVertexIndex = 15 + i * 2;
            r.isSymmetric = l.isSymmetric = true;
            r.symmetricPairID = l.pointID;
            l.symmetricPairID = r.pointID;
            cc.push_back(r);
            cc.push_back(l);
        }
        const SourceProfile named = SourceProfile::DraftFromProject(canon, "");
        if (named.landmarks.size() != 52 || named.landmarks[5].id != "nose_tip" ||
            named.landmarks[5].vertexIndex != 5 ||
            named.landmarks[14].id != "brow_inner_l" || named.landmarks[14].vertexIndex != 15 ||
            named.landmarks[15].id != "brow_inner_r" || named.landmarks[15].vertexIndex != 14 ||
            !named.Validate().empty()) {
            std::fprintf(stderr, "source_profile: canonical draft naming wrong\n");
            return 41;
        }
    }
    fs::remove_all(tmp, ec);

    // 6. Built-in MetaHuman profile.
    const std::string exeDir = Paths::ExecutableDir();
    const std::string headPath = LocateAsset(exeDir, "assets/models/MetaHumanHeadLod0.obj");
    const std::string profilesDir = LocateAsset(exeDir, "assets/profiles");
    if (headPath.empty() || profilesDir.empty()) {
        std::fprintf(stderr, "source_profile: bundled head or profiles dir not found near %s\n",
                     exeDir.c_str());
        return 50;
    }
    auto head = std::make_shared<Mesh>();
    if (!head->Load(headPath)) {
        std::fprintf(stderr, "source_profile: Mesh::Load failed for %s\n", headPath.c_str());
        return 51;
    }
    const auto headFp = TopologyFingerprint::FromMesh(*head);
    std::printf("source_profile: bundled head fingerprint verts=%zu faces=%zu hash=%s\n",
                headFp.vertexCount, headFp.faceCount, headFp.HashHex().c_str());
    {
        const auto builtIns = ProfileStore::LoadBuiltIns(profilesDir);
        const SourceProfile* mh = ProfileStore::FindById(builtIns, "metahuman-head-lod0");
        if (!mh || !mh->isBuiltIn) {
            std::fprintf(stderr, "source_profile: built-in metahuman-head-lod0 missing (%zu built-ins)\n",
                         builtIns.size());
            return 52;
        }
        if (mh->fingerprint != headFp) {
            std::fprintf(stderr, "source_profile: built-in fingerprint does not match the bundled head\n");
            return 53;
        }
        std::error_code exEc;
        if (!fs::exists(mh->resolvedMeshPath, exEc) ||
            fs::canonical(mh->resolvedMeshPath, exEc) != fs::canonical(headPath, exEc)) {
            std::fprintf(stderr, "source_profile: built-in mesh path resolves to %s\n",
                         mh->resolvedMeshPath.c_str());
            return 54;
        }
        if (ProfileStore::FindByFingerprint(builtIns, headFp) != mh) {
            std::fprintf(stderr, "source_profile: fingerprint lookup missed the built-in\n");
            return 55;
        }
        // Authored landmarks must come from the canonical set, so the solver
        // and the Stage 2 checklist can find them by id.
        const auto& canonical = SourceProfile::CanonicalLandmarks();
        for (const auto& lm : mh->landmarks) {
            const auto it = std::find_if(canonical.begin(), canonical.end(),
                                         [&](const ProfileLandmark& c) { return c.id == lm.id; });
            if (it == canonical.end() || it->name != lm.name || it->region != lm.region ||
                it->mirrorId != lm.mirrorId) {
                std::fprintf(stderr, "source_profile: built-in landmark '%s' does not match the canonical set\n",
                             lm.id.c_str());
                return 56;
            }
        }
        std::printf("source_profile: built-in '%s' OK (%zu landmarks, %zu mask ranges)\n",
                    mh->name.c_str(), mh->landmarks.size(), mh->maskRanges.size());
    }

    // 7. Informational FBX round-trip.
    {
        const fs::path fbx = fs::temp_directory_path() / "metavisage_source_profile_head.fbx";
        ExportOptions opts;
        opts.format = ExportFormat::FBX;
        opts.triangulate = false;
        opts.applyTransform = false;
        opts.includeMaterials = false;
        MeshExporter exporter;
        const auto er = exporter.Export(*head, fbx.generic_string(), opts);
        auto reloaded = std::make_shared<Mesh>();
        if (er.success && reloaded->Load(fbx.generic_string())) {
            const auto fbxFp = TopologyFingerprint::FromMesh(*reloaded);
            std::printf("source_profile: FBX round-trip fingerprint %s (verts=%zu faces=%zu hash=%s)\n",
                        fbxFp == headFp ? "MATCHES" : "DIFFERS",
                        fbxFp.vertexCount, fbxFp.faceCount, fbxFp.HashHex().c_str());
        } else {
            std::printf("source_profile: FBX round-trip skipped (%s)\n",
                        er.success ? "reload failed" : er.errorMessage.c_str());
        }
        fs::remove(fbx, ec);
    }

    std::printf("source_profile: OK\n");
    return 0;
}
