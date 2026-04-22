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

} // namespace MetaVisage::Paths

#endif
