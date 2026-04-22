#include "utils/OpenUrl.h"
#include "utils/Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <vector>

namespace MetaVisage {

namespace {

bool IsAllowedScheme(const QString& url) {
    return url.startsWith("https://", Qt::CaseInsensitive)
        || url.startsWith("http://", Qt::CaseInsensitive)
        || url.startsWith("mailto:", Qt::CaseInsensitive);
}

} // namespace

bool OpenUrl(const QString& url) {
    if (url.isEmpty()) return false;

    if (!IsAllowedScheme(url)) {
        MV_LOG_WARNING(QString("OpenUrl: rejected scheme in %1").arg(url));
        return false;
    }

    std::vector<wchar_t> buf(static_cast<size_t>(url.size()) + 1);
    const int written = url.toWCharArray(buf.data());
    buf[written] = L'\0';

    const HINSTANCE rc = ShellExecuteW(nullptr, L"open", buf.data(), nullptr, nullptr, SW_SHOWNORMAL);
    const auto code = reinterpret_cast<INT_PTR>(rc);
    if (code <= 32) {
        MV_LOG_WARNING(QString("OpenUrl: ShellExecuteW failed (code=%1) for %2").arg(code).arg(url));
        return false;
    }
    return true;
}

} // namespace MetaVisage
