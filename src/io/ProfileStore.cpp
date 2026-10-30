#include "io/ProfileStore.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <sstream>
#include <system_error>

namespace MetaVisage {

namespace {

using OrderedJson = nlohmann::ordered_json;
namespace fs = std::filesystem;

constexpr int kSchemaVersion = 1;
constexpr const char* kFileExtension   = ".mvprofile.json";
constexpr const char* kUserProfileFile = "profile.mvprofile.json";

template <typename T>
T GetOr(const OrderedJson& obj, const char* key, T fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    try { return it->get<T>(); } catch (...) { return fallback; }
}

std::string GetStr(const OrderedJson& obj, const char* key, const std::string& fallback = {}) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) return fallback;
    return it->get<std::string>();
}

// Built-in ids are lower-case words joined by hyphens. They may not use the
// user- prefix, so the two namespaces can never collide.
bool IsValidBuiltInId(const std::string& id) {
    if (id.empty() || id.size() > 64) return false;
    if (id.rfind("user-", 0) == 0) return false;
    return std::all_of(id.begin(), id.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}

bool HasSuffix(const std::string& s, const char* suffix) {
    const std::string_view sv(suffix);
    return s.size() > sv.size() && s.compare(s.size() - sv.size(), sv.size(), sv) == 0;
}

const char* AxisName(Axis a) {
    switch (a) {
        case Axis::Y: return "Y";
        case Axis::Z: return "Z";
        default:      return "X";
    }
}

Axis ParseAxis(const std::string& s) {
    if (s == "Y") return Axis::Y;
    if (s == "Z") return Axis::Z;
    return Axis::X;
}

OrderedJson Vec3Json(const Vector3& v) { return OrderedJson::array({v.x, v.y, v.z}); }

bool ReadVec3(const OrderedJson& obj, const char* key, Vector3& out) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_array() || it->size() != 3) return false;
    try {
        out = Vector3((*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>());
    } catch (...) { return false; }
    return true;
}

std::string IsoNow() {
    using namespace std::chrono;
    const auto t = system_clock::to_time_t(system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buf;
}

OrderedJson ToJson(const SourceProfile& p) {
    OrderedJson root;
    root["schema_version"] = kSchemaVersion;
    root["id"]             = p.id;
    root["name"]           = p.name;
    root["description"]    = p.description;
    root["mesh"]           = p.meshPath;
    root["fingerprint"]    = {
        {"vertex_count", p.fingerprint.vertexCount},
        {"face_count",   p.fingerprint.faceCount},
        {"face_hash",    p.fingerprint.HashHex()},
    };
    const Quaternion& q = p.defaultTransform.GetRotation();
    root["default_transform"] = {
        {"position", Vec3Json(p.defaultTransform.GetPosition())},
        {"rotation", OrderedJson::array({q.x, q.y, q.z, q.w})},
        {"scale",    Vec3Json(p.defaultTransform.GetScale())},
    };
    root["symmetry"] = {
        {"axis",         AxisName(p.symmetryAxis)},
        {"plane_offset", p.symmetryPlaneOffset},
    };
    OrderedJson landmarks = OrderedJson::array();
    for (const auto& lm : p.landmarks) {
        OrderedJson j;
        j["id"]     = lm.id;
        j["name"]   = lm.name;
        j["vertex"] = lm.vertexIndex;
        if (!lm.mirrorId.empty()) j["mirror"] = lm.mirrorId;
        j["region"] = lm.region;
        landmarks.push_back(std::move(j));
    }
    root["landmarks"] = std::move(landmarks);
    OrderedJson ranges = OrderedJson::array();
    for (const auto& [first, last] : p.maskRanges) {
        ranges.push_back(OrderedJson::array({first, last}));
    }
    root["mask_ranges"] = std::move(ranges);
    root["default_wrap_preset"] = p.defaultWrapPresetId;
    return root;
}

// Fills isBuiltIn and resolvedMeshPath; mesh paths are relative to `meshRoot`.
void Resolve(SourceProfile& p, bool builtIn, const fs::path& meshRoot) {
    p.isBuiltIn = builtIn;
    const fs::path mesh(p.meshPath);
    p.resolvedMeshPath = mesh.is_absolute()
        ? mesh.generic_string()
        : (meshRoot / mesh).lexically_normal().generic_string();
}

void SortByName(std::vector<SourceProfile>& profiles) {
    std::stable_sort(profiles.begin(), profiles.end(),
                     [](const SourceProfile& a, const SourceProfile& b) {
                         return a.name < b.name;
                     });
}

} // namespace

std::string ProfileStore::BuiltInDir() {
    const std::string exeDir = Paths::ExecutableDir();
    if (exeDir.empty()) return {};
    return (fs::path(exeDir) / "assets" / "profiles").generic_string();
}

std::string ProfileStore::UserProfilesDir() {
    const std::string root = Paths::AppDataDir();
    if (root.empty()) return {};
    const fs::path dir = fs::path(root) / "profiles";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir.generic_string();
}

std::vector<SourceProfile> ProfileStore::LoadAll() {
    std::vector<SourceProfile> out = LoadBuiltIns(BuiltInDir());
    const std::string userDir = UserProfilesDir();
    if (userDir.empty()) {
        MV_LOG_WARNING("ProfileStore: APPDATA root unavailable; user profiles disabled");
        return out;
    }
    for (auto& p : LoadUserProfiles(userDir)) {
        if (FindById(out, p.id)) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping user profile '{}' (id taken)", p.id));
            continue;
        }
        out.push_back(std::move(p));
    }
    return out;
}

std::vector<SourceProfile> ProfileStore::LoadBuiltIns(const std::string& dir) {
    std::vector<SourceProfile> out;
    if (dir.empty()) return out;
    std::error_code ec;
    auto it = fs::directory_iterator(dir, ec);
    if (ec) {
        MV_LOG_WARNING(std::format("ProfileStore: built-in profile dir unavailable: {}", dir));
        return out;
    }
    const fs::path meshRoot = fs::path(dir).parent_path();   // assets/
    for (const auto& entry : it) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (!HasSuffix(name, kFileExtension)) continue;
        std::string error;
        auto profile = ReadFile(entry.path().string(), &error);
        if (!profile) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping {} ({})", name, error));
            continue;
        }
        if (!IsValidBuiltInId(profile->id)) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping {} (invalid built-in id '{}')",
                                       name, profile->id));
            continue;
        }
        if (fs::path(profile->meshPath).is_absolute()) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping {} (built-in mesh path must be relative)",
                                       name));
            continue;
        }
        Resolve(*profile, /*builtIn=*/true, meshRoot);
        out.push_back(std::move(*profile));
    }
    SortByName(out);
    return out;
}

std::vector<SourceProfile> ProfileStore::LoadUserProfiles(const std::string& dir) {
    std::vector<SourceProfile> out;
    if (dir.empty()) return out;
    std::error_code ec;
    auto it = fs::directory_iterator(dir, ec);
    if (ec) return out;
    for (const auto& entry : it) {
        if (!entry.is_directory()) continue;
        const fs::path file = entry.path() / kUserProfileFile;
        std::error_code existsEc;
        if (!fs::exists(file, existsEc)) continue;
        const std::string folder = entry.path().filename().string();
        std::string error;
        auto profile = ReadFile(file.string(), &error);
        if (!profile) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping profile {} ({})", folder, error));
            continue;
        }
        // The folder name is the id; a mismatch means a hand-moved folder.
        if (!IsValidUserProfileId(profile->id) || profile->id != folder) {
            MV_LOG_WARNING(std::format("ProfileStore: skipping profile {} (id '{}' invalid "
                                       "or does not match its folder)", folder, profile->id));
            continue;
        }
        Resolve(*profile, /*builtIn=*/false, entry.path());
        out.push_back(std::move(*profile));
    }
    SortByName(out);
    return out;
}

std::optional<SourceProfile> ProfileStore::ReadFile(const std::string& path, std::string* error) {
    auto fail = [error](std::string msg) -> std::optional<SourceProfile> {
        if (error) *error = std::move(msg);
        return std::nullopt;
    };

    std::ifstream in(fs::path(path), std::ios::binary);
    if (!in.is_open()) return fail("cannot open file");
    std::stringstream buf;
    buf << in.rdbuf();

    OrderedJson root;
    try {
        root = OrderedJson::parse(buf.str());
    } catch (const std::exception& e) {
        return fail(std::format("malformed JSON: {}", e.what()));
    }
    if (!root.is_object()) return fail("root is not an object");

    const int version = GetOr<int>(root, "schema_version", 0);
    if (version != kSchemaVersion) {
        return fail(std::format("schema_version={}; expected {}", version, kSchemaVersion));
    }

    SourceProfile p;
    p.id          = GetStr(root, "id");
    p.name        = GetStr(root, "name");
    p.description = GetStr(root, "description");
    p.meshPath    = GetStr(root, "mesh");
    p.defaultWrapPresetId = GetStr(root, "default_wrap_preset");
    if (p.id.empty() || p.name.empty() || p.meshPath.empty()) {
        return fail("missing id, name or mesh");
    }

    auto fpIt = root.find("fingerprint");
    if (fpIt == root.end() || !fpIt->is_object()) return fail("missing fingerprint");
    p.fingerprint.vertexCount = GetOr<std::size_t>(*fpIt, "vertex_count", 0);
    p.fingerprint.faceCount   = GetOr<std::size_t>(*fpIt, "face_count", 0);
    if (!TopologyFingerprint::ParseHashHex(GetStr(*fpIt, "face_hash"), p.fingerprint.faceHash) ||
        !p.fingerprint.IsValid()) {
        return fail("invalid fingerprint");
    }

    if (auto tIt = root.find("default_transform"); tIt != root.end() && tIt->is_object()) {
        Vector3 v;
        if (ReadVec3(*tIt, "position", v)) p.defaultTransform.SetPosition(v);
        if (ReadVec3(*tIt, "scale", v))    p.defaultTransform.SetScale(v);
        auto rIt = tIt->find("rotation");
        if (rIt != tIt->end() && rIt->is_array() && rIt->size() == 4) {
            try {
                p.defaultTransform.SetRotation(Quaternion(
                    (*rIt)[0].get<float>(), (*rIt)[1].get<float>(),
                    (*rIt)[2].get<float>(), (*rIt)[3].get<float>()).Normalized());
            } catch (...) {
                return fail("invalid default_transform.rotation");
            }
        }
    }

    if (auto sIt = root.find("symmetry"); sIt != root.end() && sIt->is_object()) {
        p.symmetryAxis        = ParseAxis(GetStr(*sIt, "axis", "X"));
        p.symmetryPlaneOffset = GetOr<float>(*sIt, "plane_offset", 0.0f);
    }

    if (auto lIt = root.find("landmarks"); lIt != root.end() && lIt->is_array()) {
        for (const auto& j : *lIt) {
            if (!j.is_object()) return fail("landmark entry is not an object");
            ProfileLandmark lm;
            lm.id          = GetStr(j, "id");
            lm.name        = GetStr(j, "name", lm.id);
            lm.vertexIndex = GetOr<int>(j, "vertex", -1);
            lm.mirrorId    = GetStr(j, "mirror");
            lm.region      = GetStr(j, "region");
            p.landmarks.push_back(std::move(lm));
        }
    }

    if (auto mIt = root.find("mask_ranges"); mIt != root.end() && mIt->is_array()) {
        for (const auto& j : *mIt) {
            if (!j.is_array() || j.size() != 2 ||
                !j[0].is_number_integer() || !j[1].is_number_integer()) {
                return fail("invalid mask range");
            }
            p.maskRanges.emplace_back(j[0].get<int>(), j[1].get<int>());
        }
    }

    const auto problems = p.Validate();
    if (!problems.empty()) {
        return fail(std::format("{} problem(s), first: {}", problems.size(), problems.front()));
    }
    return p;
}

bool ProfileStore::WriteFile(const SourceProfile& profile, const std::string& path) {
    OrderedJson root = ToJson(profile);

    // Preserve created_at across rewrites so a future "sort by date" list
    // reflects the real creation time (same rule as PresetStore::Save).
    std::string createdAt;
    {
        std::ifstream existing(fs::path(path), std::ios::binary);
        if (existing.is_open()) {
            std::stringstream buf;
            buf << existing.rdbuf();
            try {
                createdAt = GetStr(OrderedJson::parse(buf.str()), "created_at");
            } catch (...) {
            }
        }
    }
    root["created_at"] = createdAt.empty() ? IsoNow() : createdAt;

    std::ofstream out(fs::path(path), std::ios::binary);
    if (!out.is_open()) {
        MV_LOG_ERROR(std::format("ProfileStore: cannot open {} for writing", path));
        return false;
    }
    out << root.dump(2) << '\n';
    out.flush();
    if (out.fail()) {
        MV_LOG_ERROR(std::format("ProfileStore: write failed for {} (disk full or I/O error)", path));
        return false;
    }
    return true;
}

bool ProfileStore::Save(const SourceProfile& profile) {
    return Save(profile, UserProfilesDir());
}

bool ProfileStore::Save(const SourceProfile& profile, const std::string& dir) {
    if (profile.isBuiltIn) {
        MV_LOG_ERROR(std::format("ProfileStore::Save: refusing to write built-in '{}'", profile.id));
        return false;
    }
    if (!IsValidUserProfileId(profile.id)) {
        MV_LOG_ERROR(std::format("ProfileStore::Save: refusing id '{}' (fails user-XXXXXXXX check)",
                                 profile.id));
        return false;
    }
    if (dir.empty()) {
        MV_LOG_ERROR("ProfileStore::Save: profile directory unavailable");
        return false;
    }
    const fs::path folder = fs::path(dir) / profile.id;
    std::error_code ec;
    fs::create_directories(folder, ec);
    if (ec) {
        MV_LOG_ERROR(std::format("ProfileStore::Save: cannot create {}: {}",
                                 folder.generic_string(), ec.message()));
        return false;
    }
    return WriteFile(profile, (folder / kUserProfileFile).string());
}

bool ProfileStore::Delete(const std::string& profileId) {
    return Delete(profileId, UserProfilesDir());
}

bool ProfileStore::Delete(const std::string& profileId, const std::string& dir) {
    // The id check is the path-traversal guard: only user-XXXXXXXX folders
    // directly under `dir` can be removed.
    if (!IsValidUserProfileId(profileId)) {
        MV_LOG_ERROR(std::format("ProfileStore::Delete: refusing id '{}'", profileId));
        return false;
    }
    if (dir.empty()) return false;
    std::error_code ec;
    fs::remove_all(fs::path(dir) / profileId, ec);
    if (ec) {
        MV_LOG_ERROR(std::format("ProfileStore::Delete: filesystem error: {}", ec.message()));
        return false;
    }
    return true;
}

std::string ProfileStore::GenerateUserProfileId() {
    using namespace std::chrono;
    const auto t = high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    const uint32_t mix = static_cast<uint32_t>(t) ^ rd();
    return std::format("user-{:08x}", mix);
}

bool ProfileStore::IsValidUserProfileId(const std::string& id) {
    if (id.size() != 13 || id.compare(0, 5, "user-") != 0) return false;
    return std::all_of(id.begin() + 5, id.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

const SourceProfile* ProfileStore::FindById(const std::vector<SourceProfile>& profiles,
                                            const std::string& id) {
    if (id.empty()) return nullptr;
    for (const auto& p : profiles) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const SourceProfile* ProfileStore::FindByFingerprint(const std::vector<SourceProfile>& profiles,
                                                     const TopologyFingerprint& fp) {
    if (!fp.IsValid()) return nullptr;
    for (const auto& p : profiles) {
        if (p.fingerprint == fp) return &p;
    }
    return nullptr;
}

} // namespace MetaVisage
