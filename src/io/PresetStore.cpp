#include "io/PresetStore.h"
#include "utils/Logger.h"
#include "utils/Paths.h"

#include <nlohmann/json.hpp>

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

// Schema version stamped on every preset file. Bump only on incompatible
// breaks. Loader rejects unknown major versions with a logged warning so
// future-us can roll forward without a hard crash.
constexpr int kSchemaVersion = 1;

constexpr const char* kFileExtension = ".mvpreset.json";

// Strict whitelist for preset ids: "user-" followed by exactly 8 lower-case
// hex digits. Mirrors the format `GenerateUserPresetId` produces, so any
// value that doesn't match couldn't have come from us. Used as a path-
// traversal guard before constructing `<id>.mvpreset.json` filenames in
// the user presets directory — a hand-edited file with id="../etc/passwd"
// would otherwise let `Delete` / `Save` escape `UserPresetsDir`.
bool IsValidUserPresetId(const std::string& id) {
    if (id.size() != 13) return false;        // "user-" (5) + 8 hex
    if (id.compare(0, 5, "user-") != 0) return false;
    for (size_t i = 5; i < id.size(); ++i) {
        const char c = id[i];
        const bool isHex =
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f');
        if (!isHex) return false;
    }
    return true;
}

// Small read helper, mirrors ProjectSerializer::GetOr.
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

std::string IsoNow() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
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

OrderedJson SerializeParams(const WrapParams& p) {
    OrderedJson j;
    j["algorithm"]                       = static_cast<int>(p.algorithm);
    j["stiffness"]                       = p.stiffness;
    j["smoothness"]                      = p.smoothness;
    j["nricp_stiffness_steps"]           = p.nricpStiffnessSteps;
    j["nricp_alpha_initial"]             = p.nricpAlphaInitial;
    j["nricp_alpha_final"]               = p.nricpAlphaFinal;
    j["nricp_icp_iterations"]            = p.nricpIcpIterations;
    j["nricp_normal_threshold"]          = p.nricpNormalThreshold;
    j["nricp_landmark_weight"]           = p.nricpLandmarkWeight;
    j["nricp_epsilon"]                   = p.nricpEpsilon;
    j["nricp_enable_boundary_exclusion"] = p.nricpEnableBoundaryExclusion;
    j["nricp_boundary_exclusion_hops"]   = p.nricpBoundaryExclusionHops;
    j["nricp_optimization_iterations"]   = p.nricpOptimizationIterations;
    j["nricp_dp_initial"]                = p.nricpDpInitial;
    j["nricp_dp_final"]                  = p.nricpDpFinal;
    j["nricp_gamma_initial"]             = p.nricpGammaInitial;
    j["nricp_gamma_final"]               = p.nricpGammaFinal;
    j["nricp_sampling_initial"]          = p.nricpSamplingInitial;
    j["nricp_sampling_final"]            = p.nricpSamplingFinal;
    j["nricp_normalize_sampling"]        = p.nricpNormalizeSampling;
    return j;
}

WrapParams DeserializeParams(const OrderedJson& j) {
    WrapParams p;
    // Algorithm field is read-and-discarded — Sprint 4 collapsed the enum to a
    // single member. WrapParams::algorithm is always NRICP.
    (void)GetOr<int>(j, "algorithm", static_cast<int>(DeformationAlgorithm::NRICP));
    p.algorithm                     = DeformationAlgorithm::NRICP;
    p.stiffness                     = GetOr<float>(j, "stiffness", p.stiffness);
    p.smoothness                    = GetOr<float>(j, "smoothness", p.smoothness);
    p.nricpStiffnessSteps           = GetOr<int>  (j, "nricp_stiffness_steps", p.nricpStiffnessSteps);
    p.nricpAlphaInitial             = GetOr<float>(j, "nricp_alpha_initial", p.nricpAlphaInitial);
    p.nricpAlphaFinal               = GetOr<float>(j, "nricp_alpha_final", p.nricpAlphaFinal);
    p.nricpIcpIterations            = GetOr<int>  (j, "nricp_icp_iterations", p.nricpIcpIterations);
    p.nricpNormalThreshold          = GetOr<float>(j, "nricp_normal_threshold", p.nricpNormalThreshold);
    p.nricpLandmarkWeight           = GetOr<float>(j, "nricp_landmark_weight", p.nricpLandmarkWeight);
    p.nricpEpsilon                  = GetOr<float>(j, "nricp_epsilon", p.nricpEpsilon);
    p.nricpEnableBoundaryExclusion  = GetOr<bool> (j, "nricp_enable_boundary_exclusion", p.nricpEnableBoundaryExclusion);
    p.nricpBoundaryExclusionHops    = GetOr<int>  (j, "nricp_boundary_exclusion_hops", p.nricpBoundaryExclusionHops);
    p.nricpOptimizationIterations   = GetOr<int>  (j, "nricp_optimization_iterations", p.nricpOptimizationIterations);
    p.nricpDpInitial                = GetOr<float>(j, "nricp_dp_initial", p.nricpDpInitial);
    p.nricpDpFinal                  = GetOr<float>(j, "nricp_dp_final", p.nricpDpFinal);
    p.nricpGammaInitial             = GetOr<float>(j, "nricp_gamma_initial", p.nricpGammaInitial);
    p.nricpGammaFinal               = GetOr<float>(j, "nricp_gamma_final", p.nricpGammaFinal);
    p.nricpSamplingInitial          = GetOr<float>(j, "nricp_sampling_initial", p.nricpSamplingInitial);
    p.nricpSamplingFinal            = GetOr<float>(j, "nricp_sampling_final", p.nricpSamplingFinal);
    p.nricpNormalizeSampling        = GetOr<bool> (j, "nricp_normalize_sampling", p.nricpNormalizeSampling);
    return p;
}

} // namespace

std::string PresetStore::UserPresetsDir() {
    namespace fs = std::filesystem;
    const std::string root = Paths::AppDataDir();
    if (root.empty()) return {};
    const fs::path dir = fs::path(root) / "presets";
    std::error_code ec;
    fs::create_directories(dir, ec); // idempotent
    return dir.generic_string();
}

std::vector<WrapPreset> PresetStore::LoadAll() {
    namespace fs = std::filesystem;
    std::vector<WrapPreset> out;

    const std::string dir = UserPresetsDir();
    if (dir.empty()) {
        MV_LOG_WARNING("PresetStore: APPDATA root unavailable; user presets disabled");
        return out;
    }

    std::error_code ec;
    auto it = fs::directory_iterator(dir, ec);
    if (ec) return out; // empty dir or transient error — non-fatal

    for (const auto& entry : it) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        // Match *.mvpreset.json (case-sensitive — created by us).
        if (name.size() <= std::string_view(kFileExtension).size()) continue;
        if (name.compare(name.size() - std::string_view(kFileExtension).size(),
                         std::string_view(kFileExtension).size(),
                         kFileExtension) != 0) continue;

        std::ifstream in(entry.path());
        if (!in.is_open()) {
            MV_LOG_WARNING(std::format("PresetStore: cannot open {}", name));
            continue;
        }
        std::stringstream buf;
        buf << in.rdbuf();
        OrderedJson root;
        try {
            root = OrderedJson::parse(buf.str());
        } catch (const std::exception& e) {
            MV_LOG_WARNING(std::format("PresetStore: malformed JSON in {}: {}", name, e.what()));
            continue;
        }

        const int version = GetOr<int>(root, "schema_version", 0);
        if (version != kSchemaVersion) {
            MV_LOG_WARNING(std::format(
                "PresetStore: skipping {} (schema_version={}; expected {})",
                name, version, kSchemaVersion));
            continue;
        }

        WrapPreset preset;
        preset.id          = GetStr(root, "id");
        preset.name        = GetStr(root, "name");
        preset.description = GetStr(root, "description");
        preset.iconGlyph   = GetStr(root, "icon_glyph");
        preset.isBuiltIn   = false; // built-ins are never on disk
        if (auto p = root.find("params"); p != root.end() && p->is_object()) {
            preset.params = DeserializeParams(*p);
        }
        if (preset.id.empty() || preset.name.empty()) {
            MV_LOG_WARNING(std::format("PresetStore: skipping {} (missing id or name)", name));
            continue;
        }
        // Path-traversal guard. Any preset whose id wasn't produced by
        // GenerateUserPresetId is treated as malformed — this keeps a
        // hand-edited "id": "../bad" out of the in-memory list and out of
        // the path Save / Delete would later construct.
        if (!IsValidUserPresetId(preset.id)) {
            MV_LOG_WARNING(std::format(
                "PresetStore: skipping {} (id '{}' fails user-XXXXXXXX format check)",
                name, preset.id));
            continue;
        }
        out.push_back(std::move(preset));
    }

    return out;
}

bool PresetStore::Save(const WrapPreset& preset) {
    namespace fs = std::filesystem;
    if (preset.id.empty()) {
        MV_LOG_ERROR("PresetStore::Save: preset has empty id");
        return false;
    }
    if (!IsValidUserPresetId(preset.id)) {
        MV_LOG_ERROR(std::format(
            "PresetStore::Save: refusing id '{}' — fails user-XXXXXXXX format check",
            preset.id));
        return false;
    }
    const std::string dir = UserPresetsDir();
    if (dir.empty()) {
        MV_LOG_ERROR("PresetStore::Save: APPDATA root unavailable");
        return false;
    }

    OrderedJson root;
    const fs::path path = fs::path(dir) / (preset.id + kFileExtension);

    // Preserve created_at across Rename / Replace flows. If a file already
    // exists at this id, peek at its created_at field and reuse it; otherwise
    // stamp IsoNow() (true creation timestamp). Without this, every Save
    // overwrote created_at with the rename/replace time — silent today
    // because no UI surfaces the field, but breaks any future "sort by date"
    // listing. (PR #22 review feedback.)
    std::string createdAt;
    {
        std::error_code existsEc;
        if (std::filesystem::exists(path, existsEc) && !existsEc) {
            std::ifstream existing(path);
            if (existing.is_open()) {
                std::stringstream buf;
                buf << existing.rdbuf();
                try {
                    const auto existingRoot = OrderedJson::parse(buf.str());
                    createdAt = GetStr(existingRoot, "created_at");
                } catch (...) {
                    // Malformed existing file — fall through to IsoNow().
                }
            }
        }
    }
    if (createdAt.empty()) createdAt = IsoNow();

    root["schema_version"] = kSchemaVersion;
    root["id"]             = preset.id;
    root["name"]           = preset.name;
    root["description"]    = preset.description;
    root["icon_glyph"]     = preset.iconGlyph;
    root["created_at"]     = createdAt;
    root["params"]         = SerializeParams(preset.params);

    std::ofstream out(path);
    if (!out.is_open()) {
        MV_LOG_ERROR(std::format("PresetStore::Save: cannot open {} for writing",
                                 path.generic_string()));
        return false;
    }
    out << root.dump(2);
    out.flush();
    if (out.fail()) {
        MV_LOG_ERROR(std::format(
            "PresetStore::Save: write failed for {} (disk full or I/O error)",
            path.generic_string()));
        return false;
    }
    return true;
}

bool PresetStore::Delete(const std::string& presetId) {
    namespace fs = std::filesystem;
    if (presetId.empty()) return false;
    if (!IsValidUserPresetId(presetId)) {
        MV_LOG_ERROR(std::format(
            "PresetStore::Delete: refusing id '{}' — fails user-XXXXXXXX format check",
            presetId));
        return false;
    }
    const std::string dir = UserPresetsDir();
    if (dir.empty()) return false;
    const fs::path path = fs::path(dir) / (presetId + kFileExtension);
    std::error_code ec;
    fs::remove(path, ec); // remove-or-no-op; idempotent
    if (ec) {
        MV_LOG_ERROR(std::format("PresetStore::Delete: filesystem error: {}", ec.message()));
        return false;
    }
    return true;
}

std::string PresetStore::GenerateUserPresetId() {
    // High-res clock bits XOR'd with random_device entropy → 32 bits → 8 hex.
    using namespace std::chrono;
    const auto t = high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    const uint32_t mix = static_cast<uint32_t>(t) ^ rd();
    return std::format("user-{:08x}", mix);
}

} // namespace MetaVisage
