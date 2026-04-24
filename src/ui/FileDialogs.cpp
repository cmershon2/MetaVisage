#include "ui/FileDialogs.h"
#include "utils/Logger.h"

#include <nfd.hpp>

#include <vector>

namespace MetaVisage::FileDialogs {

namespace {

struct NfdLifetime {
    bool ok = false;
    NfdLifetime() { ok = (NFD::Init() == NFD_OKAY); }
    ~NfdLifetime() { if (ok) NFD::Quit(); }
};

std::wstring Utf8ToWide(const std::string& s) {
    return QString::fromStdString(s).toStdWString();
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

std::optional<QString> OpenFile(const std::vector<Filter>& filters) {
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
        if (rc == NFD_ERROR) MV_LOG_WARNING(QString("FileDialogs: open error: %1").arg(NFD::GetError()));
        return std::nullopt;
    }
    QString result = QString::fromWCharArray(outPath);
    NFD::FreePath(outPath);
    return result;
}

std::optional<QString> SaveFile(const std::vector<Filter>& filters,
                                const QString& defaultName) {
    NfdLifetime lifetime;
    if (!lifetime.ok) {
        MV_LOG_WARNING("FileDialogs: NFD init failed");
        return std::nullopt;
    }

    FilterStorage storage = BuildFilters(filters);
    const std::wstring defaultNameW = defaultName.toStdWString();

    nfdnchar_t* outPath = nullptr;
    const nfdresult_t rc = NFD::SaveDialog(
        outPath,
        storage.items.empty() ? nullptr : storage.items.data(),
        static_cast<nfdfiltersize_t>(storage.items.size()),
        /*defaultPath=*/nullptr,
        defaultNameW.empty() ? nullptr : defaultNameW.c_str());
    if (rc != NFD_OKAY) {
        if (rc == NFD_ERROR) MV_LOG_WARNING(QString("FileDialogs: save error: %1").arg(NFD::GetError()));
        return std::nullopt;
    }
    QString result = QString::fromWCharArray(outPath);
    NFD::FreePath(outPath);
    return result;
}

} // namespace MetaVisage::FileDialogs
