#ifndef METAVISAGE_UTILS_CONFIG_H
#define METAVISAGE_UTILS_CONFIG_H

#include <map>
#include <string>
#include <vector>

namespace MetaVisage {

// JSON-backed user configuration at %APPDATA%\MetaVisage\config.json.
// Replaces QSettings from the Qt-era UI. Values are loaded once at startup
// (Load()) and persisted on each setter call (writes are synchronous but
// best-effort — I/O errors are logged, not raised).
class Config {
public:
    static Config& Instance();

    // Load from %APPDATA%\MetaVisage\config.json. Missing/invalid file is
    // silently treated as "all defaults".
    void Load();

    // Window geometry (main application window).
    int WindowWidth() const;
    int WindowHeight() const;
    bool WindowMaximized() const;
    void SetWindowGeometry(int width, int height, bool maximized);

    // Recent projects (most-recent-first, capped at 10).
    const std::vector<std::string>& RecentProjects() const { return recentProjects_; }
    void AddRecentProject(const std::string& absolutePath);
    void ClearRecentProjects();

    // Story 5.1 — UI state persistence for inspector CollapsingHeaders.
    // Keys are dotted scopes ("morph.general", "morph.advanced",
    // "morph.interior_geometry"). The default is consulted only when the
    // key has never been written (first launch or new section); later
    // writes are persisted in the JSON ui_state.collapsed_sections block.
    //
    // PR #24 review nit — Setter performance contract: SetCollapsedSection
    // calls Save() synchronously on every state change, which opens +
    // writes + closes %APPDATA%\MetaVisage\config.json. Callers MUST
    // gate the call (e.g. `if (newState != savedState)`) so the setter
    // only fires once per gesture, not once per frame. The
    // DrawCollapsibleSection lambda in StageSidebar.cpp::RenderMorph is
    // the canonical pattern; replicate it for new collapsible callers.
    // Do not call this from any per-frame hot path or scroll-resized
    // sub-window without the gate — synchronous I/O at >1 Hz will stutter.
    bool GetCollapsedSection(const std::string& key, bool defaultExpanded) const;
    void SetCollapsedSection(const std::string& key, bool expanded);

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void Save() const;

    int windowWidth_ = 1600;
    int windowHeight_ = 900;
    bool windowMaximized_ = false;
    std::vector<std::string> recentProjects_;
    // Story 5.1 — value is "section is expanded"; absence = use default.
    std::map<std::string, bool> collapsedSections_;
    bool loaded_ = false;
};

} // namespace MetaVisage

#endif
