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

} // namespace MetaVisage::Paths

#endif
