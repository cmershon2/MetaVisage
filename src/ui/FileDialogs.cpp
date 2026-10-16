#include "ui/FileDialogs.h"
#include "utils/Logger.h"

#include <nfd.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <format>
#include <string>
#include <vector>

namespace MetaVisage::FileDialogs {

namespace {

struct NfdLifetime {
    bool ok = false;
    NfdLifetime() { ok = (NFD::Init() == NFD_OKAY); }
    ~NfdLifetime() { if (ok) NFD::Quit(); }
};

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int chars = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                                          nullptr, 0);
    std::wstring out(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), out.data(), chars);
    return out;
}

std::string WideToUtf8(const wchar_t* w) {
    if (!w) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes - 1), '\0'); // strip trailing null
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), bytes, nullptr, nullptr);
    return out;
}

// Build wide-string backing storage that `nfdnfilteritem_t[]` can point into.
// The storage vector must outlive the filter-item array.
struct FilterStorage {
    std::vector<std::wstring> names;
    std::vector<std::wstring> specs;
    std::vector<nfdnfilteritem_t> items;
};

FilterStorage BuildFilters(const std::vector<Filter>& filters) {
    FilterStorage out;
    out.names.reserve(filters.size());
    out.specs.reserve(filters.size());
    out.items.reserve(filters.size());
    for (const auto& f : filters) {
        out.names.emplace_back(Utf8ToWide(f.name));
        out.specs.emplace_back(Utf8ToWide(f.extensions));
        out.items.push_back({out.names.back().c_str(), out.specs.back().c_str()});
    }
    return out;
}

} // namespace

std::optional<std::string> OpenFile(const std::vector<Filter>& filters) {
    NfdLifetime lifetime;
    if (!lifetime.ok) {
        MV_LOG_WARNING("FileDialogs: NFD init failed");
        return std::nullopt;
    }

    FilterStorage storage = BuildFilters(filters);
    nfdnchar_t* outPath = nullptr;
    const nfdresult_t rc = NFD::OpenDialog(
        outPath,
        storage.items.empty() ? nullptr : storage.items.data(),
        static_cast<nfdfiltersize_t>(storage.items.size()));
    if (rc != NFD_OKAY) {
        if (rc == NFD_ERROR) MV_LOG_WARNING(std::format("FileDialogs: open error: {}", NFD::GetError()));
        return std::nullopt;
    }
    std::string result = WideToUtf8(outPath);
    NFD::FreePath(outPath);
    return result;
}

std::optional<std::string> SaveFile(const std::vector<Filter>& filters,
                                    const std::string& defaultName) {
    NfdLifetime lifetime;
    if (!lifetime.ok) {
        MV_LOG_WARNING("FileDialogs: NFD init failed");
        return std::nullopt;
    }

    FilterStorage storage = BuildFilters(filters);
    const std::wstring defaultNameW = Utf8ToWide(defaultName);

    nfdnchar_t* outPath = nullptr;
    const nfdresult_t rc = NFD::SaveDialog(
        outPath,
        storage.items.empty() ? nullptr : storage.items.data(),
        static_cast<nfdfiltersize_t>(storage.items.size()),
        /*defaultPath=*/nullptr,
        defaultNameW.empty() ? nullptr : defaultNameW.c_str());
    if (rc != NFD_OKAY) {
        if (rc == NFD_ERROR) MV_LOG_WARNING(std::format("FileDialogs: save error: {}", NFD::GetError()));
        return std::nullopt;
    }
    std::string result = WideToUtf8(outPath);
    NFD::FreePath(outPath);
    return result;
}

} // namespace MetaVisage::FileDialogs
