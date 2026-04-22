#ifndef METAVISAGE_UTILS_CONFIG_H
#define METAVISAGE_UTILS_CONFIG_H

#include <QString>
#include <optional>
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
    const std::vector<QString>& RecentProjects() const { return recentProjects_; }
    void AddRecentProject(const QString& absolutePath);
    void ClearRecentProjects();

    // Auth tokens cached between launches. std::nullopt means "no token stored".
    std::optional<QString> AccessToken() const;
    std::optional<QString> RefreshToken() const;
    void SetTokens(const QString& access, const QString& refresh);
    void ClearTokens();

    // Cached license JWT (7-day offline grace period — see DESKTOP_APP_INTEGRATION.md).
    std::optional<QString> LicenseJwt() const;
    void SetLicenseJwt(const QString& jwt);
    void ClearLicenseJwt();

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void Save() const;

    int windowWidth_ = 1600;
    int windowHeight_ = 900;
    bool windowMaximized_ = false;
    std::vector<QString> recentProjects_;
    std::optional<QString> accessToken_;
    std::optional<QString> refreshToken_;
    std::optional<QString> licenseJwt_;
    bool loaded_ = false;
};

} // namespace MetaVisage

#endif
