#include "utils/Config.h"
#include "utils/Paths.h"
#include "utils/Logger.h"
#include "utils/SecureStorage.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>

namespace MetaVisage {

using nlohmann::json;

namespace {

constexpr size_t kMaxRecentProjects = 10;

std::optional<std::string> ReadOptString(const json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return std::nullopt;
    return it->get<std::string>();
}

// Read an encrypted-string value. Falls back to plaintext for one-time
// migration of pre-encryption configs (existing dev installs); returns
// std::nullopt if the value is missing, fails to decrypt, AND is not
// recognizable plaintext. Forces re-auth on failure rather than throwing —
// the auth-flow recovery path is well-trodden and safer than guessing.
std::optional<std::string> ReadEncryptedString(const json& j, const char* key) {
    auto raw = ReadOptString(j, key);
    if (!raw) return std::nullopt;
    if (auto decrypted = SecureStorage::DecryptString(*raw)) {
        return decrypted;
    }
    // Legacy plaintext migration: tokens look like JWTs (start with "eyJ") or
    // Supabase access tokens (start with "ey"); any other shape we treat as
    // bogus and drop. The next Save() re-encrypts whatever we surface here.
    if (raw->size() > 4 && (*raw)[0] == 'e' && (*raw)[1] == 'y') return raw;
    return std::nullopt;
}

} // namespace

Config& Config::Instance() {
    static Config instance;
    return instance;
}

Config::Config() = default;

void Config::Load() {
    if (loaded_) return;
    loaded_ = true;

    const std::string path = Paths::ConfigFile();
    if (path.empty()) return;

    std::ifstream in(path);
    if (!in.is_open()) {
        return; // First launch — defaults apply.
    }
    std::stringstream raw;
    raw << in.rdbuf();
    in.close();

    json j;
    try {
        j = json::parse(raw.str());
    } catch (const std::exception& e) {
        MV_LOG_WARNING(std::format("Config: failed to parse {}: {}", path, e.what()));
        return;
    }

    if (auto it = j.find("window"); it != j.end() && it->is_object()) {
        windowWidth_ = it->value("width", windowWidth_);
        windowHeight_ = it->value("height", windowHeight_);
        windowMaximized_ = it->value("maximized", windowMaximized_);
    }

    if (auto it = j.find("recent_projects"); it != j.end() && it->is_array()) {
        recentProjects_.clear();
        for (const auto& entry : *it) {
            if (entry.is_string()) recentProjects_.push_back(entry.get<std::string>());
        }
    }

    if (auto it = j.find("auth"); it != j.end() && it->is_object()) {
        accessToken_ = ReadEncryptedString(*it, "access_token");
        refreshToken_ = ReadEncryptedString(*it, "refresh_token");
    }

    licenseJwt_ = ReadEncryptedString(j, "license_jwt");
    userEmail_  = ReadEncryptedString(j, "user_email");

    // Story 5.1 — UI state. Missing block / unknown keys silently fall
    // through to defaults; bad value types are skipped, not crashed.
    if (auto ui = j.find("ui_state"); ui != j.end() && ui->is_object()) {
        if (auto cs = ui->find("collapsed_sections");
            cs != ui->end() && cs->is_object()) {
            for (auto entry = cs->begin(); entry != cs->end(); ++entry) {
                if (entry.value().is_boolean()) {
                    collapsedSections_[entry.key()] = entry.value().get<bool>();
                }
            }
        }
    }
}

void Config::Save() const {
    const std::string path = Paths::ConfigFile();
    if (path.empty()) return;

    json j;
    j["window"] = {
        {"width", windowWidth_},
        {"height", windowHeight_},
        {"maximized", windowMaximized_},
    };
    json recent = json::array();
    for (const auto& p : recentProjects_) recent.push_back(p);
    j["recent_projects"] = std::move(recent);

    // Encrypt tokens with DPAPI before they touch disk. On encryption failure
    // we drop the value rather than persisting plaintext — the user re-auths
    // on next launch via the browser flow.
    if (accessToken_ || refreshToken_) {
        json auth = json::object();
        if (accessToken_) {
            if (auto enc = SecureStorage::EncryptString(*accessToken_)) auth["access_token"] = *enc;
        }
        if (refreshToken_) {
            if (auto enc = SecureStorage::EncryptString(*refreshToken_)) auth["refresh_token"] = *enc;
        }
        if (!auth.empty()) j["auth"] = std::move(auth);
    }
    if (licenseJwt_) {
        if (auto enc = SecureStorage::EncryptString(*licenseJwt_)) j["license_jwt"] = *enc;
    }
    if (userEmail_) {
        if (auto enc = SecureStorage::EncryptString(*userEmail_)) j["user_email"] = *enc;
    }

    // Story 5.1 — only emit ui_state if at least one section has been
    // explicitly recorded; keeps fresh configs minimal.
    if (!collapsedSections_.empty()) {
        json sections = json::object();
        for (const auto& [key, expanded] : collapsedSections_) {
            sections[key] = expanded;
        }
        j["ui_state"] = json::object();
        j["ui_state"]["collapsed_sections"] = std::move(sections);
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        MV_LOG_WARNING(std::format("Config: failed to open for write: {}", path));
        return;
    }
    out << j.dump(2);
}

int Config::WindowWidth() const { return windowWidth_; }
int Config::WindowHeight() const { return windowHeight_; }
bool Config::WindowMaximized() const { return windowMaximized_; }

void Config::SetWindowGeometry(int width, int height, bool maximized) {
    windowWidth_ = width;
    windowHeight_ = height;
    windowMaximized_ = maximized;
    Save();
}

void Config::AddRecentProject(const std::string& absolutePath) {
    auto it = std::find(recentProjects_.begin(), recentProjects_.end(), absolutePath);
    if (it != recentProjects_.end()) recentProjects_.erase(it);
    recentProjects_.insert(recentProjects_.begin(), absolutePath);
    if (recentProjects_.size() > kMaxRecentProjects) recentProjects_.resize(kMaxRecentProjects);
    Save();
}

void Config::ClearRecentProjects() {
    recentProjects_.clear();
    Save();
}

std::optional<std::string> Config::AccessToken() const { return accessToken_; }
std::optional<std::string> Config::RefreshToken() const { return refreshToken_; }

void Config::SetTokens(const std::string& access, const std::string& refresh) {
    accessToken_ = access;
    refreshToken_ = refresh;
    Save();
}

void Config::ClearTokens() {
    accessToken_.reset();
    refreshToken_.reset();
    Save();
}

std::optional<std::string> Config::LicenseJwt() const { return licenseJwt_; }

void Config::SetLicenseJwt(const std::string& jwt) {
    licenseJwt_ = jwt;
    Save();
}

void Config::ClearLicenseJwt() {
    licenseJwt_.reset();
    Save();
}

std::optional<std::string> Config::UserEmail() const { return userEmail_; }

void Config::SetUserEmail(const std::string& email) {
    userEmail_ = email;
    Save();
}

void Config::ClearUserEmail() {
    userEmail_.reset();
    Save();
}

bool Config::GetCollapsedSection(const std::string& key, bool defaultExpanded) const {
    auto it = collapsedSections_.find(key);
    if (it == collapsedSections_.end()) return defaultExpanded;
    return it->second;
}

void Config::SetCollapsedSection(const std::string& key, bool expanded) {
    auto it = collapsedSections_.find(key);
    if (it != collapsedSections_.end() && it->second == expanded) return;
    collapsedSections_[key] = expanded;
    Save();
}

} // namespace MetaVisage
