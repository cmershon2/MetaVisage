#include "utils/Paths.h"

#include <QDir>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

namespace MetaVisage::Paths {

namespace {

QString RoamingAppDataRoot() {
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
    if (FAILED(hr) || !path) {
        if (path) CoTaskMemFree(path);
        return {};
    }
    QString result = QString::fromWCharArray(path);
    CoTaskMemFree(path);
    return result;
}

QString EnsureDir(const QString& absolute) {
    if (absolute.isEmpty()) return {};
    QDir dir(absolute);
    if (!dir.exists() && !dir.mkpath(".")) {
        return {};
    }
    return absolute;
}

} // namespace

QString AppDataDir() {
    const QString root = RoamingAppDataRoot();
    if (root.isEmpty()) return {};
    return EnsureDir(root + "/MetaVisage");
}

QString LogDir() {
    const QString base = AppDataDir();
    if (base.isEmpty()) return {};
    return EnsureDir(base + "/logs");
}

QString ConfigFile() {
    const QString base = AppDataDir();
    if (base.isEmpty()) return {};
    return base + "/config.json";
}

} // namespace MetaVisage::Paths
