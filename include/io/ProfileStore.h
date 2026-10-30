#ifndef METAVISAGE_IO_PROFILESTORE_H
#define METAVISAGE_IO_PROFILESTORE_H

#include "core/SourceProfile.h"

#include <optional>
#include <string>
#include <vector>

namespace MetaVisage {

// Sprint 11 — persistence for Source Profiles.
//
// Built-in profiles: <exe>/assets/profiles/<id>.mvprofile.json, read-only.
// Their mesh path is relative to <exe>/assets/.
//
// User profiles: %APPDATA%\MetaVisage\profiles\<id>\profile.mvprofile.json,
// one folder per profile so the profile can carry its own mesh copy
// (Sprint 14). Their mesh path is relative to that folder. Folder names come
// from the stable id, never the display name.
//
// Every file carries schema_version: 1. Unknown versions and malformed files
// are skipped with a logged warning. The *Dir overloads exist so tests can
// point the store at a temporary directory instead of the real install.
class ProfileStore {
public:
    // <exe>/assets/profiles. Not created; empty if the exe dir is unknown.
    static std::string BuiltInDir();

    // %APPDATA%\MetaVisage\profiles, created on first access. Empty if the
    // APPDATA root can't be resolved.
    static std::string UserProfilesDir();

    // Built-ins first, then user profiles; each group sorted by name. A user
    // profile whose id collides with a built-in is skipped.
    static std::vector<SourceProfile> LoadAll();

    static std::vector<SourceProfile> LoadBuiltIns(const std::string& dir);
    static std::vector<SourceProfile> LoadUserProfiles(const std::string& dir);

    // Parses one profile file. Leaves isBuiltIn / resolvedMeshPath unset;
    // the directory loaders fill them in. On failure returns nullopt and
    // writes a reason to `error` when given.
    static std::optional<SourceProfile> ReadFile(const std::string& path,
                                                 std::string* error = nullptr);

    // Writes `profile` to an explicit path (dev export, tests).
    static bool WriteFile(const SourceProfile& profile, const std::string& path);

    // Writes a user profile to <dir>/<id>/profile.mvprofile.json. Refuses
    // built-ins and ids that GenerateUserProfileId couldn't have produced.
    static bool Save(const SourceProfile& profile);
    static bool Save(const SourceProfile& profile, const std::string& dir);

    // Removes <dir>/<id>/. Idempotent; false only on a filesystem error or
    // an invalid id.
    static bool Delete(const std::string& profileId);
    static bool Delete(const std::string& profileId, const std::string& dir);

    // "user-XXXXXXXX" (8 lower-case hex digits), like PresetStore ids.
    static std::string GenerateUserProfileId();
    static bool IsValidUserProfileId(const std::string& id);

    static const SourceProfile* FindById(const std::vector<SourceProfile>& profiles,
                                         const std::string& id);
    // First profile whose fingerprint equals `fp`, or nullptr.
    static const SourceProfile* FindByFingerprint(const std::vector<SourceProfile>& profiles,
                                                  const TopologyFingerprint& fp);
};

} // namespace MetaVisage

#endif
