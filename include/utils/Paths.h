#ifndef METAVISAGE_UTILS_PATHS_H
#define METAVISAGE_UTILS_PATHS_H

#include <string>

namespace MetaVisage::Paths {

// %APPDATA%\MetaVisage (created on first access). Empty string on failure.
std::string AppDataDir();

// %APPDATA%\MetaVisage\logs
std::string LogDir();

// %APPDATA%\MetaVisage\config.json
std::string ConfigFile();

// Directory of the running MetaVisage executable. Used to locate bundled
// assets that ship next to the binary (e.g. assets/models/). Empty on error.
std::string ExecutableDir();

// Full path to the running MetaVisage executable (forward-slash form).
// Used by LicenseManager::EnsureSchemeRegistered to write the
// `shell\open\command` value for the metavisage:// URL handler. Empty on
// error. Callers that write to the Windows registry should convert to
// backslashes at the call site since the OS prefers native separators.
std::string ExecutablePath();

} // namespace MetaVisage::Paths

#endif
