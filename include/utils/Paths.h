#ifndef METAVISAGE_UTILS_PATHS_H
#define METAVISAGE_UTILS_PATHS_H

#include <QString>

namespace MetaVisage::Paths {

// %APPDATA%\MetaVisage (created on first access). Empty string on failure.
QString AppDataDir();

// %APPDATA%\MetaVisage\logs
QString LogDir();

// %APPDATA%\MetaVisage\config.json
QString ConfigFile();

// Directory of the running MetaVisage executable. Used to locate bundled
// assets that ship next to the binary (e.g. assets/models/). Empty on error.
QString ExecutableDir();

} // namespace MetaVisage::Paths

#endif
