#include "utils/SecureStorage.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>

#include <cstdint>
#include <vector>

#pragma comment(lib, "Crypt32.lib")

namespace MetaVisage::SecureStorage {

namespace {

constexpr const wchar_t* kEntropyDescription = L"MetaVisage cached credentials";

// RFC 4648 base64 alphabet — used to keep DPAPI output JSON-safe.
constexpr const char* kB64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 3 <= data.size(); i += 3) {
        const std::uint32_t v =
            (static_cast<std::uint32_t>(data[i])     << 16) |
            (static_cast<std::uint32_t>(data[i + 1]) <<  8) |
             static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kB64Alphabet[(v >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 12) & 0x3F]);
        out.push_back(kB64Alphabet[(v >>  6) & 0x3F]);
        out.push_back(kB64Alphabet[ v        & 0x3F]);
    }
    if (i < data.size()) {
        std::uint32_t v = static_cast<std::uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) v |= static_cast<std::uint32_t>(data[i + 1]) << 8;
        out.push_back(kB64Alphabet[(v >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 12) & 0x3F]);
        out.push_back(i + 1 < data.size() ? kB64Alphabet[(v >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

int B64Lookup(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool Base64Decode(const std::string& s, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve((s.size() / 4) * 3);
    int buf = 0;
    int bits = 0;
    for (char ch : s) {
        if (ch == '=' || ch == '\r' || ch == '\n') continue;
        const int v = B64Lookup(static_cast<unsigned char>(ch));
        if (v < 0) return false;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buf >> bits) & 0xFF));
        }
    }
    return true;
}

} // namespace

std::optional<std::string> EncryptString(const std::string& plaintext) {
    if (plaintext.empty()) return std::nullopt;

    DATA_BLOB in;
    in.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(plaintext.data()));
    in.cbData = static_cast<DWORD>(plaintext.size());

    DATA_BLOB out{};
    if (!CryptProtectData(&in,
                          kEntropyDescription,
                          /*pOptionalEntropy=*/nullptr,
                          /*pvReserved=*/nullptr,
                          /*pPromptStruct=*/nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN,
                          &out)) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return Base64Encode(bytes);
}

std::optional<std::string> DecryptString(const std::string& ciphertextB64) {
    if (ciphertextB64.empty()) return std::nullopt;

    std::vector<std::uint8_t> raw;
    if (!Base64Decode(ciphertextB64, raw) || raw.empty()) return std::nullopt;

    DATA_BLOB in;
    in.pbData = raw.data();
    in.cbData = static_cast<DWORD>(raw.size());

    DATA_BLOB out{};
    if (!CryptUnprotectData(&in,
                            /*ppszDataDescr=*/nullptr,
                            /*pOptionalEntropy=*/nullptr,
                            /*pvReserved=*/nullptr,
                            /*pPromptStruct=*/nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &out)) {
        return std::nullopt;
    }

    std::string plaintext(reinterpret_cast<const char*>(out.pbData), out.cbData);
    LocalFree(out.pbData);
    return plaintext;
}

} // namespace MetaVisage::SecureStorage
