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

    // Story 5.1 — UI state. Missing block / unknown keys silently fall
    // through to defaults; bad value types are skipped, not crashed.
    if (auto ui = j.find("ui_state"); ui != j.end() && ui->is_object()) {
        showGrid_ = ui->value("show_grid", showGrid_);
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
    if (!loaded_) return;
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

    // Story 5.1 — only emit ui_state entries that differ from defaults;
    // keeps fresh configs minimal.
    if (!collapsedSections_.empty()) {
        json sections = json::object();
        for (const auto& [key, expanded] : collapsedSections_) {
            sections[key] = expanded;
        }
        j["ui_state"]["collapsed_sections"] = std::move(sections);
    }
    if (!showGrid_) j["ui_state"]["show_grid"] = false;

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

void Config::SetShowGrid(bool show) {
    if (showGrid_ == show) return;
    showGrid_ = show;
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
