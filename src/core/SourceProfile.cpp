#include "core/SourceProfile.h"
#include "core/Mesh.h"
#include "core/Project.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <unordered_map>

namespace MetaVisage {

namespace {

constexpr std::uint64_t kFnvOffset = 0xcbf29ce484222325ull;
constexpr std::uint64_t kFnvPrime  = 0x100000001b3ull;

void FnvMix(std::uint64_t& h, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        h ^= static_cast<std::uint8_t>(value >> (i * 8));
        h *= kFnvPrime;
    }
}

} // namespace

TopologyFingerprint TopologyFingerprint::FromMesh(const Mesh& mesh) {
    TopologyFingerprint fp;
    fp.vertexCount = mesh.GetVertexCount();
    fp.faceCount   = mesh.GetFaceCount();
    std::uint64_t h = kFnvOffset;
    FnvMix(h, static_cast<std::uint32_t>(fp.vertexCount));
    FnvMix(h, static_cast<std::uint32_t>(fp.faceCount));
    for (const Face& f : mesh.GetFaces()) {
        // Mixing the face size keeps a quad and two triangles over the same
        // indices from hashing alike.
        FnvMix(h, static_cast<std::uint32_t>(f.vertexIndices.size()));
        for (unsigned int idx : f.vertexIndices) FnvMix(h, idx);
    }
    fp.faceHash = h;
    return fp;
}

std::string TopologyFingerprint::HashHex() const {
    return std::format("{:016x}", faceHash);
}

bool TopologyFingerprint::ParseHashHex(const std::string& hex, std::uint64_t& out) {
    if (hex.empty() || hex.size() > 16) return false;
    std::uint64_t v = 0;
    for (char c : hex) {
        v <<= 4;
        if (c >= '0' && c <= '9')      v |= static_cast<std::uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') v |= static_cast<std::uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= static_cast<std::uint64_t>(c - 'A' + 10);
        else return false;
    }
    out = v;
    return true;
}

namespace MaskRanges {

std::vector<MaskRange> FromMask(const std::vector<bool>& mask) {
    std::vector<MaskRange> ranges;
    const int n = static_cast<int>(mask.size());
    int i = 0;
    while (i < n) {
        if (!mask[i]) { ++i; continue; }
        const int first = i;
        while (i < n && mask[i]) ++i;
        ranges.emplace_back(first, i - 1);
    }
    return ranges;
}

std::vector<bool> ToMask(const std::vector<MaskRange>& ranges, std::size_t vertexCount) {
    std::vector<bool> mask(vertexCount, false);
    const int n = static_cast<int>(vertexCount);
    for (const auto& [first, last] : ranges) {
        const int lo = first < 0 ? 0 : first;
        const int hi = last >= n ? n - 1 : last;
        for (int i = lo; i <= hi; ++i) mask[i] = true;
    }
    return mask;
}

} // namespace MaskRanges

std::vector<bool> SourceProfile::BuildMask() const {
    return MaskRanges::ToMask(maskRanges, fingerprint.vertexCount);
}

std::vector<std::string> SourceProfile::Validate() const {
    std::vector<std::string> problems;
    const int vertexCount = static_cast<int>(fingerprint.vertexCount);

    std::unordered_map<std::string, const ProfileLandmark*> byId;
    for (const auto& lm : landmarks) {
        if (lm.id.empty()) {
            problems.push_back("landmark with empty id");
            continue;
        }
        if (!byId.emplace(lm.id, &lm).second) {
            problems.push_back(std::format("duplicate landmark id '{}'", lm.id));
        }
        if (lm.vertexIndex < 0 || lm.vertexIndex >= vertexCount) {
            problems.push_back(std::format("landmark '{}' vertex {} out of range [0, {})",
                                           lm.id, lm.vertexIndex, vertexCount));
        }
    }
    for (const auto& lm : landmarks) {
        if (lm.mirrorId.empty()) continue;
        auto it = byId.find(lm.mirrorId);
        if (it == byId.end()) {
            problems.push_back(std::format("landmark '{}' mirrors unknown id '{}'",
                                           lm.id, lm.mirrorId));
        } else if (it->second->mirrorId != lm.id) {
            problems.push_back(std::format("landmark '{}' mirror '{}' is not reciprocal",
                                           lm.id, lm.mirrorId));
        }
    }
    for (const auto& [first, last] : maskRanges) {
        if (first < 0 || last < first || last >= vertexCount) {
            problems.push_back(std::format("mask range [{}, {}] out of range [0, {})",
                                           first, last, vertexCount));
        }
    }
    return problems;
}

const std::vector<ProfileLandmark>& SourceProfile::CanonicalLandmarks() {
    static const std::vector<ProfileLandmark> kLandmarks = [] {
        std::vector<ProfileLandmark> out;
        struct Entry { const char* id; const char* name; const char* region; };
        const Entry midline[] = {
            {"cranium_top",        "Cranium top",        "cranium"},
            {"forehead_center",    "Forehead center",    "brow"},
            {"glabella",           "Glabella",           "brow"},
            {"nasion",             "Nasion",             "nose"},
            {"nose_bridge",        "Nose bridge",        "nose"},
            {"nose_tip",           "Nose tip",           "nose"},
            {"subnasale",          "Subnasale",          "nose"},
            {"upper_lip_center",   "Upper lip center",   "mouth"},
            {"lip_contact_center", "Lip contact center", "mouth"},
            {"lower_lip_center",   "Lower lip center",   "mouth"},
            {"chin_point",         "Chin point",         "jaw"},
            {"chin_bottom",        "Chin bottom",        "jaw"},
            {"occiput",            "Occiput",            "cranium"},
            {"nape",               "Nape",               "neck"},
        };
        const Entry pairs[] = {
            {"brow_inner",        "Brow inner",          "brow"},
            {"brow_peak",         "Brow peak",           "brow"},
            {"brow_outer",        "Brow outer",          "brow"},
            {"eye_inner_corner",  "Eye inner corner",    "eye"},
            {"eye_outer_corner",  "Eye outer corner",    "eye"},
            {"upper_lid_center",  "Upper lid center",    "eye"},
            {"lower_lid_center",  "Lower lid center",    "eye"},
            {"nostril_wing",      "Nostril wing",        "nose"},
            {"nasolabial_top",    "Nasolabial crease top", "cheek"},
            {"upper_lip_peak",    "Upper lip peak",      "mouth"},
            {"mouth_corner",      "Mouth corner",        "mouth"},
            {"cheekbone",         "Cheekbone",           "cheek"},
            {"jaw_angle",         "Jaw angle",           "jaw"},
            {"jaw_middle",        "Jaw middle",          "jaw"},
            {"temple",            "Temple",              "cranium"},
            {"ear_top",           "Ear top",             "ear"},
            {"tragus",            "Tragus",              "ear"},
            {"ear_lobe_bottom",   "Ear lobe bottom",     "ear"},
            {"ear_back",          "Ear back",            "ear"},
        };
        for (const auto& e : midline) {
            out.push_back({e.id, e.name, -1, "", e.region});
        }
        for (const auto& e : pairs) {
            const std::string l = std::string(e.id) + "_l";
            const std::string r = std::string(e.id) + "_r";
            out.push_back({l, std::string(e.name) + " (L)", -1, r, e.region});
            out.push_back({r, std::string(e.name) + " (R)", -1, l, e.region});
        }
        return out;
    }();
    return kLandmarks;
}

SourceProfile SourceProfile::DraftFromProject(const Project& project, const std::string& assetsDir) {
    namespace fs = std::filesystem;
    SourceProfile draft;
    const MeshReference& ref = project.GetMorphMesh();
    if (!ref.mesh) return draft;
    const Mesh& mesh = *ref.mesh;

    // Id and name from the file stem: "MetaHumanHeadLod0" -> "metahumanheadlod0".
    const fs::path meshFile(ref.filepath);
    const std::string stem = meshFile.stem().string();
    for (char c : stem) {
        const char lc = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        const bool keep = (lc >= 'a' && lc <= 'z') || (lc >= '0' && lc <= '9');
        if (keep) draft.id += lc;
        else if (!draft.id.empty() && draft.id.back() != '-') draft.id += '-';
    }
    if (draft.id.empty()) draft.id = "source";
    draft.name        = stem.empty() ? std::string("Source") : stem;
    draft.description = "Draft exported from MetaVisage. Review names, mesh path and "
                        "default_wrap_preset before shipping.";

    draft.meshPath = meshFile.generic_string();
    if (!assetsDir.empty() && !ref.filepath.empty()) {
        std::error_code ec;
        const fs::path rel = fs::relative(meshFile, fs::path(assetsDir), ec);
        if (!ec && !rel.empty() && *rel.begin() != "..") draft.meshPath = rel.generic_string();
    }

    draft.fingerprint      = TopologyFingerprint::FromMesh(mesh);
    draft.defaultTransform = ref.transform;
    const PointReferenceData& points = project.GetPointReferenceData();
    draft.symmetryAxis        = points.symmetryAxis;
    draft.symmetryPlaneOffset = points.symmetryPlaneOffset;

    // Source-side points in placement (pointID) order.
    std::vector<const PointCorrespondence*> placed;
    for (const auto& c : points.correspondences) {
        if (c.morphMeshVertexIndex >= 0 &&
            c.morphMeshVertexIndex < static_cast<int>(mesh.GetVertexCount())) {
            placed.push_back(&c);
        }
    }
    std::sort(placed.begin(), placed.end(),
              [](const PointCorrespondence* a, const PointCorrespondence* b) {
                  return a->pointID < b->pointID;
              });
    std::unordered_map<int, const PointCorrespondence*> byPointId;
    for (const auto* c : placed) byPointId[c->pointID] = c;
    auto partnerOf = [&](const PointCorrespondence* c) -> const PointCorrespondence* {
        if (!c->isSymmetric || c->symmetricPairID < 0) return nullptr;
        auto it = byPointId.find(c->symmetricPairID);
        return it == byPointId.end() ? nullptr : it->second;
    };

    std::vector<const PointCorrespondence*> midline;
    std::vector<std::pair<const PointCorrespondence*, const PointCorrespondence*>> pairs;
    for (const auto* c : placed) {
        const auto* partner = partnerOf(c);
        if (!partner) midline.push_back(c);
        else if (c->pointID < partner->pointID) pairs.emplace_back(c, partner);
    }

    auto axisValue = [&](int vertexIndex) {
        const Vector3& v = mesh.GetVertices()[vertexIndex];
        switch (draft.symmetryAxis) {
            case Axis::Y: return v.y;
            case Axis::Z: return v.z;
            default:      return v.x;
        }
    };

    const auto& canonical = CanonicalLandmarks();
    constexpr size_t kMidline = 14;
    constexpr size_t kPairs   = 19;
    if (midline.size() == kMidline && pairs.size() == kPairs) {
        for (size_t i = 0; i < kMidline; ++i) {
            ProfileLandmark lm = canonical[i];
            lm.vertexIndex = midline[i]->morphMeshVertexIndex;
            draft.landmarks.push_back(std::move(lm));
        }
        for (size_t i = 0; i < kPairs; ++i) {
            auto [a, b] = pairs[i];
            // Positive side of the symmetry plane is the character's left.
            if (axisValue(a->morphMeshVertexIndex) < axisValue(b->morphMeshVertexIndex)) {
                std::swap(a, b);
            }
            ProfileLandmark left  = canonical[kMidline + i * 2];
            ProfileLandmark right = canonical[kMidline + i * 2 + 1];
            left.vertexIndex  = a->morphMeshVertexIndex;
            right.vertexIndex = b->morphMeshVertexIndex;
            draft.landmarks.push_back(std::move(left));
            draft.landmarks.push_back(std::move(right));
        }
    } else {
        for (const auto* c : placed) {
            ProfileLandmark lm;
            lm.id          = std::format("point_{}", c->pointID);
            lm.name        = std::format("Point {}", c->pointID);
            lm.vertexIndex = c->morphMeshVertexIndex;
            if (const auto* partner = partnerOf(c)) {
                lm.mirrorId = std::format("point_{}", partner->pointID);
            }
            draft.landmarks.push_back(std::move(lm));
        }
    }

    const std::vector<bool>& mask = project.GetMorphData().vertexMask;
    if (mask.size() == mesh.GetVertexCount()) {
        draft.maskRanges = MaskRanges::FromMask(mask);
    }
    return draft;
}

} // namespace MetaVisage
