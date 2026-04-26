#include "utils/OpenUrl.h"
#include "utils/Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <cctype>
#include <format>
#include <string>
#include <vector>

namespace MetaVisage {

namespace {

bool StartsWithICase(const std::string& s, const char* prefix) {
    const size_t n = std::char_traits<char>::length(prefix);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

bool IsAllowedScheme(const std::string& url) {
    return StartsWithICase(url, "https://")
        || StartsWithICase(url, "http://")
        || StartsWithICase(url, "mailto:");
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int chars = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), out.data(), chars);
    return out;
}

} // namespace

bool OpenUrl(const std::string& url) {
    if (url.empty()) return false;

    if (!IsAllowedScheme(url)) {
        MV_LOG_WARNING(std::format("OpenUrl: rejected scheme in {}", url));
        return false;
    }

    const std::wstring wide = Utf8ToWide(url);
    const HINSTANCE rc = ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    const auto code = reinterpret_cast<INT_PTR>(rc);
    if (code <= 32) {
        MV_LOG_WARNING(std::format("OpenUrl: ShellExecuteW failed (code={}) for {}",
            static_cast<long long>(code), url));
        return false;
    }
    return true;
}

} // namespace MetaVisage
