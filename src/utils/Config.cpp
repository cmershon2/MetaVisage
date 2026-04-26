#include "utils/Config.h"
#include "utils/Paths.h"
#include "utils/Logger.h"

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
        accessToken_ = ReadOptString(*it, "access_token");
        refreshToken_ = ReadOptString(*it, "refresh_token");
    }

    licenseJwt_ = ReadOptString(j, "license_jwt");
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

    if (accessToken_ || refreshToken_) {
        json auth = json::object();
        if (accessToken_) auth["access_token"] = *accessToken_;
        if (refreshToken_) auth["refresh_token"] = *refreshToken_;
        j["auth"] = std::move(auth);
    }
    if (licenseJwt_) j["license_jwt"] = *licenseJwt_;

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

} // namespace MetaVisage
