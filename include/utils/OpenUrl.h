#ifndef METAVISAGE_UTILS_OPENURL_H
#define METAVISAGE_UTILS_OPENURL_H

#include <string>

namespace MetaVisage {

// Open a URL in the user's default browser via ShellExecuteW. Returns true on
// success. Non-http(s) / non-mailto schemes are rejected — callers can prepend
// "https://" if they have a bare host.
bool OpenUrl(const std::string& url);

} // namespace MetaVisage

#endif
