#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "licensing/LicenseManager.h"

#include "licensing/LicenseConfig.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/OpenUrl.h"
#include "utils/Paths.h"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
// Story 6.2 -- BCryptGenRandom for the CSPRNG state token, RegCreateKeyExW
// for the metavisage:// URL scheme registration. Pragmas mirror the
// SecureStorage.cpp convention so the link set is documented inline.
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string_view>

namespace MetaVisage {

namespace {

constexpr auto kAuthFlowTimeout = std::chrono::minutes(5);

std::string_view StateName(LicenseManager::AuthState s) {
    using S = LicenseManager::AuthState;
    switch (s) {
        case S::Unauthenticated: return "Unauthenticated";
        case S::Authenticating:  return "Authenticating";
        case S::Authenticated:   return "Authenticated";
        case S::Expired:         return "Expired";
        case S::Inactive:        return "Inactive";
    }
    return "Unknown";
}

// metavisage://auth?code=<hex>&state=<hex> -- single fixed shape, no need
// for a URL library. Returns nullopt if the scheme/host don't match or
// either query parameter is missing/empty.
struct DeepLinkParts { std::string code; std::string state; };

std::optional<DeepLinkParts> ParseDeepLink(std::string_view uri) {
    constexpr std::string_view kPrefix = "metavisage://auth?";
    if (uri.size() <= kPrefix.size() ||
        uri.substr(0, kPrefix.size()) != kPrefix) {
        return std::nullopt;
    }
    std::string_view query = uri.substr(kPrefix.size());
    DeepLinkParts parts;
    while (!query.empty()) {
        const size_t amp = query.find('&');
        const std::string_view kv =
            (amp == std::string_view::npos) ? query : query.substr(0, amp);
        const size_t eq = kv.find('=');
        if (eq != std::string_view::npos) {
            const auto key = kv.substr(0, eq);
            const auto val = kv.substr(eq + 1);
            if      (key == "code")  parts.code  = std::string(val);
            else if (key == "state") parts.state = std::string(val);
        }
        if (amp == std::string_view::npos) break;
        query.remove_prefix(amp + 1);
    }
    if (parts.code.empty() || parts.state.empty()) return std::nullopt;
    return parts;
}

#ifdef _WIN32

// Set REG_SZ value at the given subkey under HKCU\Software\Classes\<...>.
// `valueName == nullptr` writes the (Default) value. Returns true on success;
// failures are logged but non-fatal -- the auth flow falls back to manual
// browser-paste if scheme registration is denied.
bool SetClassesString(const wchar_t* subkey,
                      const wchar_t* valueName,
                      const std::wstring& value) {
    HKEY key = nullptr;
    LSTATUS result = ::RegCreateKeyExW(
        HKEY_CURRENT_USER, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: RegCreateKeyExW failed ({}) for HKCU subkey",
            static_cast<long>(result)));
        return false;
    }
    const DWORD bytes =
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    result = ::RegSetValueExW(
        key, valueName, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()), bytes);
    ::RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: RegSetValueExW failed ({})",
            static_cast<long>(result)));
        return false;
    }
    return true;
}

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    const int needed = ::MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0) return {};
    std::wstring out(static_cast<size_t>(needed), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                          static_cast<int>(utf8.size()), out.data(), needed);
    return out;
}

#endif  // _WIN32

}  // namespace

LicenseManager& LicenseManager::Instance() {
    static LicenseManager instance;
    return instance;
}

LicenseManager::LicenseManager() = default;

void LicenseManager::Initialize() {
    if (initialized_) return;
    initialized_ = true;

    MV_LOG_INFO(std::format(
        "LicenseManager: initialize (profile={}, api={})",
        LicenseConfig::kBuildProfile,
        LicenseConfig::kApiBaseUrl));

    auto& config = Config::Instance();
    accessToken_  = config.AccessToken();
    refreshToken_ = config.RefreshToken();
    licenseJwt_   = config.LicenseJwt();

    EnsureSchemeRegistered();

    // Story 6.1: a non-empty cached JWT counts as Authenticated. Story 6.3
    // adds RS256 signature + exp claim verification; rejected JWTs flow
    // through ClearLicenseJwt() and the user is bounced back to the sign-in
    // screen on the next launch.
    if (licenseJwt_ && !licenseJwt_->empty()) {
        TransitionTo(AuthState::Authenticated);
    } else {
        TransitionTo(AuthState::Unauthenticated);
    }
}

void LicenseManager::BeginAuthFlow() {
    if (state_ == AuthState::Authenticating) return;

    const std::string token = GenerateStateToken();
    if (token.empty()) {
        MV_LOG_ERROR("LicenseManager: state token generation failed");
        return;
    }

    const std::string url = std::format(
        "{}/desktop-auth/init?state={}",
        LicenseConfig::kApiBaseUrl, token);

    if (!OpenUrl(url)) {
        MV_LOG_ERROR(std::format(
            "LicenseManager: failed to launch browser to {}", url));
        return;
    }

    authFlowState_ = token;
    authFlowStartedAt_ = std::chrono::steady_clock::now();
    TransitionTo(AuthState::Authenticating);
}

void LicenseManager::CancelAuthFlow() {
    if (state_ != AuthState::Authenticating) return;
    authFlowState_.clear();
    authFlowStartedAt_.reset();
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SignOut() {
    auto& config = Config::Instance();
    config.ClearTokens();
    config.ClearLicenseJwt();
    accessToken_.reset();
    refreshToken_.reset();
    licenseJwt_.reset();
    authFlowState_.clear();
    authFlowStartedAt_.reset();
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SetTestState(AuthState forced) {
    state_ = forced;
    initialized_ = true;
    MV_LOG_INFO(std::format("LicenseManager: test override -> {}",
                            StateName(forced)));
}

void LicenseManager::QueueDeepLink(std::string uri) {
    std::lock_guard<std::mutex> lock(deepLinkMutex_);
    pendingDeepLink_ = std::move(uri);
}

void LicenseManager::DrainPendingDeepLink() {
    std::optional<std::string> uri;
    {
        std::lock_guard<std::mutex> lock(deepLinkMutex_);
        uri.swap(pendingDeepLink_);
    }
    if (uri) HandleDeepLink(*uri);
}

void LicenseManager::TickTimeout() {
    if (state_ != AuthState::Authenticating || !authFlowStartedAt_) return;
    const auto elapsed = std::chrono::steady_clock::now() - *authFlowStartedAt_;
    if (elapsed > kAuthFlowTimeout) {
        MV_LOG_WARNING("LicenseManager: auth flow timed out after 5 minutes");
        CancelAuthFlow();
    }
}

void LicenseManager::HandleDeepLink(const std::string& uri) {
    MV_LOG_INFO(std::format("LicenseManager: deep-link received ({} bytes)",
                            uri.size()));

    const auto parsed = ParseDeepLink(uri);
    if (!parsed) {
        MV_LOG_WARNING("LicenseManager: deep-link rejected (malformed URI)");
        return;
    }

    if (state_ != AuthState::Authenticating) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: deep-link arrived while in state {} -- ignoring",
            StateName(state_)));
        return;
    }

    // CSRF check: the URL state must match the token we generated in
    // BeginAuthFlow. Single-use -- clear authFlowState_ after the
    // comparison regardless of outcome.
    const std::string expected = authFlowState_;
    authFlowState_.clear();
    if (expected.empty() || parsed->state != expected) {
        MV_LOG_ERROR(
            "LicenseManager: deep-link state mismatch -- aborting auth flow");
        authFlowStartedAt_.reset();
        TransitionTo(AuthState::Unauthenticated);
        return;
    }

    MV_LOG_INFO("LicenseManager: state validated; exchange code captured "
                "(awaiting Story 6.3 implementation of /api/desktop/exchange)");
    // Story 6.3 fills in: POST /api/desktop/exchange with parsed->code,
    // RS256 verify the returned JWT, persist via Config::SetTokens +
    // SetLicenseJwt, transition to Authenticated. For now the auth flow
    // sits in Authenticating until Cancel or 5-minute timeout.
}

std::string LicenseManager::GenerateStateToken() const {
#ifdef _WIN32
    std::array<std::uint8_t, 32> buf{};
    const NTSTATUS status = ::BCryptGenRandom(
        nullptr, buf.data(), static_cast<ULONG>(buf.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status != 0) return {};

    // Lowercase hex -- 32 bytes -> 64 chars.
    std::string out;
    out.reserve(buf.size() * 2);
    constexpr char kDigits[] = "0123456789abcdef";
    for (std::uint8_t b : buf) {
        out.push_back(kDigits[(b >> 4) & 0x0F]);
        out.push_back(kDigits[b & 0x0F]);
    }
    return out;
#else
    return {};
#endif
}

void LicenseManager::EnsureSchemeRegistered() {
#ifdef _WIN32
    if (schemeRegistered_) return;

    const std::string exePath = Paths::ExecutablePath();
    if (exePath.empty()) {
        MV_LOG_WARNING(
            "LicenseManager: ExecutablePath() empty -- skipping scheme "
            "registration; metavisage:// deep links will not resolve");
        return;
    }

    // Convert forward slashes back to native backslashes for the registry
    // value -- ShellExecute resolves either, but native is the convention.
    std::string nativeExe = exePath;
    std::replace(nativeExe.begin(), nativeExe.end(), '/', '\\');
    const std::wstring command =
        L"\"" + Utf8ToWide(nativeExe) + L"\" \"%1\"";

    bool ok = true;
    ok &= SetClassesString(L"Software\\Classes\\metavisage", nullptr,
                           L"URL:MetaVisage Protocol");
    ok &= SetClassesString(L"Software\\Classes\\metavisage", L"URL Protocol",
                           L"");
    ok &= SetClassesString(L"Software\\Classes\\metavisage\\shell\\open\\command",
                           nullptr, command);

    if (ok) {
        MV_LOG_INFO(std::format(
            "LicenseManager: registered metavisage:// scheme at HKCU -> {}",
            nativeExe));
        schemeRegistered_ = true;
    } else {
        MV_LOG_WARNING(
            "LicenseManager: scheme registration partially failed -- the "
            "OS may not route metavisage:// deep links to this exe");
    }
#endif  // _WIN32
}

void LicenseManager::TransitionTo(AuthState next) {
    if (state_ == next) return;
    MV_LOG_INFO(std::format("LicenseManager: {} -> {}",
                            StateName(state_),
                            StateName(next)));
    state_ = next;
}

}  // namespace MetaVisage
