#include "utils/Paths.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

namespace MetaVisage::Paths {

namespace {

// Convert a Windows wide string to a UTF-8 std::string.
std::string WideToUtf8(const wchar_t* wide, int wideLen = -1) {
    if (!wide) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide, wideLen, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    // If wideLen < 0 the count includes the trailing null; trim it.
    std::string out(static_cast<size_t>(wideLen < 0 ? bytes - 1 : bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, wideLen, out.data(), bytes, nullptr, nullptr);
    return out;
}

std::string RoamingAppDataRoot() {
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
    if (FAILED(hr) || !path) {
        if (path) CoTaskMemFree(path);
        return {};
    }
    std::string result = WideToUtf8(path);
    CoTaskMemFree(path);
    return result;
}

std::string EnsureDir(const std::string& absolute) {
    if (absolute.empty()) return {};
    std::error_code ec;
    std::filesystem::create_directories(absolute, ec);
    if (ec && !std::filesystem::exists(absolute)) return {};
    return absolute;
}

} // namespace

std::string AppDataDir() {
    const std::string root = RoamingAppDataRoot();
    if (root.empty()) return {};
    return EnsureDir(root + "/MetaVisage");
}

std::string LogDir() {
    const std::string base = AppDataDir();
    if (base.empty()) return {};
    return EnsureDir(base + "/logs");
}

std::string ConfigFile() {
    const std::string base = AppDataDir();
    if (base.empty()) return {};
    return base + "/config.json";
}

std::string ExecutableDir() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) return {};
    std::string full = WideToUtf8(buf, static_cast<int>(len));
    // Strip the executable filename, leaving the directory.
    size_t slash = full.find_last_of("\\/");
    if (slash == std::string::npos) return {};
    std::string dir = full.substr(0, slash);
    std::replace(dir.begin(), dir.end(), '\\', '/');
    return dir;
}

} // namespace MetaVisage::Paths
