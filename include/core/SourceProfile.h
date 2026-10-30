#ifndef METAVISAGE_CORE_SOURCEPROFILE_H
#define METAVISAGE_CORE_SOURCEPROFILE_H

#include "core/Transform.h"
#include "core/Types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace MetaVisage {

class Mesh;
class Project;

// Sprint 11 — identifies the topology a Source Profile's vertex indices
// refer to. Two meshes with the same fingerprint have the same vertex count,
// face count and face vertex indices in the same order, so a landmark or
// mask index means the same vertex on both. Computed on the merged
// (post-UV-seam-weld) vertex list that Mesh::Load produces.
struct TopologyFingerprint {
    std::size_t   vertexCount = 0;
    std::size_t   faceCount   = 0;
    std::uint64_t faceHash    = 0;   // FNV-1a over each face's size + vertex indices

    static TopologyFingerprint FromMesh(const Mesh& mesh);

    bool IsValid() const { return vertexCount > 0 && faceCount > 0; }

    // 16 lower-case hex digits; the on-disk form of faceHash.
    std::string HashHex() const;
    static bool ParseHashHex(const std::string& hex, std::uint64_t& out);

    bool operator==(const TopologyFingerprint& o) const {
        return vertexCount == o.vertexCount && faceCount == o.faceCount &&
               faceHash == o.faceHash;
    }
    bool operator!=(const TopologyFingerprint& o) const { return !(*this == o); }
};

// A named source vertex the auto-map solver (Sprint 12) finds on the target.
struct ProfileLandmark {
    std::string id;           // stable key, e.g. "mouth_corner_l"
    std::string name;         // display, e.g. "Mouth corner (L)"
    int         vertexIndex = -1;
    std::string mirrorId;     // partner landmark id; empty on the midline
    std::string region;       // brow / eye / nose / mouth / jaw / cheek / ear / cranium / neck
};

// Inclusive [first, last] runs of masked (excluded) vertex indices.
using MaskRange = std::pair<int, int>;

namespace MaskRanges {
std::vector<MaskRange> FromMask(const std::vector<bool>& mask);
// Indices outside [0, vertexCount) are ignored.
std::vector<bool> ToMask(const std::vector<MaskRange>& ranges, std::size_t vertexCount);
} // namespace MaskRanges

// A reusable source mesh configuration: the mesh, its topology fingerprint,
// default transform, symmetry plane, landmark set and default vertex mask.
// Built-in profiles ship under assets/profiles/; user profiles live under
// %APPDATA%\MetaVisage\profiles\ (see io/ProfileStore.h). A project records
// only the profile id — points and masks are copied into the project, so a
// deleted profile never stops a project from opening.
struct SourceProfile {
    std::string id;
    std::string name;
    std::string description;
    bool        isBuiltIn = false;

    // As written in the profile file: relative to assets/ for built-ins,
    // relative to the profile's own folder for user profiles.
    std::string meshPath;
    // Absolute path resolved by ProfileStore on load. Not serialized.
    std::string resolvedMeshPath;

    TopologyFingerprint fingerprint;
    Transform           defaultTransform;
    Axis                symmetryAxis        = Axis::X;
    float               symmetryPlaneOffset = 0.0f;

    std::vector<ProfileLandmark> landmarks;
    std::vector<MaskRange>       maskRanges;
    std::string                  defaultWrapPresetId;   // empty = leave params alone

    bool HasLandmarks() const { return !landmarks.empty(); }
    bool HasMask() const { return !maskRanges.empty(); }

    // Default vertex mask sized to the fingerprint's vertex count.
    std::vector<bool> BuildMask() const;

    // Returns one message per problem: landmark indices out of range,
    // duplicate ids, mirror ids that don't resolve or aren't reciprocal,
    // mask ranges out of range. Empty means the profile is consistent.
    std::vector<std::string> Validate() const;

    // The canonical landmark set (14 midline + 19 left/right pairs). Vertex
    // indices are -1; a profile assigns them. Order is the placement order
    // the dev export expects: midline first, then each pair left before
    // right.
    static const std::vector<ProfileLandmark>& CanonicalLandmarks();

    // Dev tool (Help > Export Source Profile Draft). Builds a profile from the
    // project's source mesh: fingerprint, current source transform, Stage 2
    // symmetry axis, source-side points as landmarks, and the vertex mask.
    // When the points are exactly 14 unpaired + 19 symmetric pairs, they are
    // named from CanonicalLandmarks() in placement order (pair sides by the
    // vertex's sign on the symmetry axis: positive = L). Otherwise they get
    // generic point_<id> names. The mesh path is made relative to
    // `assetsDir` when the mesh lives under it.
    static SourceProfile DraftFromProject(const Project& project, const std::string& assetsDir);
};

} // namespace MetaVisage

#endif
