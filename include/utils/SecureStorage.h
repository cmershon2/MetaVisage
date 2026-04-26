#ifndef METAVISAGE_UTILS_SECURESTORAGE_H
#define METAVISAGE_UTILS_SECURESTORAGE_H

#include <optional>
#include <string>

namespace MetaVisage::SecureStorage {

// Win32 DPAPI wrapper keyed to the current Windows user. Used by Config to
// encrypt cached license JWTs and Supabase auth tokens before they touch
// %APPDATA%\MetaVisage\config.json — copying the config file to another user
// account will fail to decrypt, so a simple file-copy attack does not exfiltrate
// active tokens. Both helpers operate on UTF-8 strings; ciphertext is encoded
// as base64 so it fits cleanly into JSON.

// Encrypt UTF-8 plaintext. Returns base64-encoded ciphertext, or std::nullopt
// on DPAPI failure (the caller treats failure as "drop the cached value" and
// falls back to the browser auth flow rather than throwing).
std::optional<std::string> EncryptString(const std::string& plaintext);

// Decrypt base64-encoded ciphertext produced by EncryptString. Returns the
// original UTF-8 plaintext, or std::nullopt on any decryption failure
// (wrong user, corruption, or pre-encryption legacy plaintext).
std::optional<std::string> DecryptString(const std::string& ciphertextB64);

} // namespace MetaVisage::SecureStorage

#endif
