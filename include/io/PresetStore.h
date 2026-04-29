#ifndef METAVISAGE_IO_PRESETSTORE_H
#define METAVISAGE_IO_PRESETSTORE_H

#include "core/WrapPreset.h"

#include <string>
#include <vector>

namespace MetaVisage {

// Persistence for user-saved Wrap Engine presets.
//
// File layout: %APPDATA%\MetaVisage\presets\<id>.mvpreset.json — one JSON
// document per preset, schema_version: 1. Filenames are derived from the
// preset's stable id (NOT the display name) so a rename never has to
// shuffle files.
//
// Built-in presets (WrapPreset::QuickWrap / FantasyWrap) live in code, not
// on disk. PresetStore only deals with user-created presets.
class PresetStore {
public:
    // %APPDATA%\MetaVisage\presets — created on first access. Empty string
    // if the APPDATA root can't be resolved (rare; usually a service-account
    // launch with no roaming profile).
    static std::string UserPresetsDir();

    // Scans UserPresetsDir() for *.mvpreset.json files and returns the
    // parsed presets. Files with an unknown schema_version are skipped
    // with a logged warning (not a hard error — forward-compat).
    static std::vector<WrapPreset> LoadAll();

    // Writes preset to <id>.mvpreset.json under UserPresetsDir(). Overwrites
    // an existing file with the same id (used by Rename and Replace flows).
    // Returns true on success.
    static bool Save(const WrapPreset& preset);

    // Deletes <id>.mvpreset.json. Returns true if the file was removed (or
    // didn't exist — idempotent). Returns false only if the filesystem
    // operation itself errored.
    static bool Delete(const std::string& presetId);

    // Generates a fresh user preset id of the form "user-XXXXXXXX" where the
    // suffix is 8 hex characters mixing high-resolution clock bits with
    // std::random_device entropy. Sufficient uniqueness for the per-user
    // preset directory; not an RFC 4122 UUID.
    static std::string GenerateUserPresetId();
};

} // namespace MetaVisage

#endif
