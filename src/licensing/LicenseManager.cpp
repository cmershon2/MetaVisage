#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "licensing/LicenseManager.h"

#include "licensing/LicenseConfig.h"
#include "utils/Config.h"
#include "utils/HttpClient.h"
#include "utils/Logger.h"
#include "utils/OpenUrl.h"
#include "utils/Paths.h"

#include <nlohmann/json.hpp>
// jwt-cpp pulls picojson by default; route it through nlohmann_json since
// we already link nlohmann + parse the exchange response with it. The
// `defaults.h` variant exposes jwt::decode() / jwt::verify() with the
// traits already wired so we don't have to template every call site.
#include <jwt-cpp/traits/nlohmann-json/defaults.h>

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
#include <chrono>
#include <cstdint>
#include <deque>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

namespace MetaVisage {

namespace {

constexpr auto kAuthFlowTimeout = std::chrono::minutes(5);

std::string_view StateName(LicenseManager::AuthState s) {
    using S = LicenseManager::AuthState;
    switch (s) {
        case S::Unauthenticated: return "Unauthenticated";
        case S::Authenticating:  return "Authenticating";
        case S::Verifying:       return "Verifying";
        case S::Authenticated:   return "Authenticated";
        case S::Expired:         return "Expired";
        case S::Inactive:        return "Inactive";
    }
    return "Unknown";
}

// metavisage://auth?code=<hex>&state=<hex> -- single fixed shape, no need
// for a URL library. Tolerates the trailing-slash form
// "metavisage://auth/?code=..." that Windows / Chromium-family browsers
// produce when normalizing a host-only URI. Returns nullopt if the
// scheme/host don't match or either query parameter is missing/empty.
struct DeepLinkParts { std::string code; std::string state; };

std::optional<DeepLinkParts> ParseDeepLink(std::string_view uri) {
    constexpr std::string_view kSchemePrefix = "metavisage://";
    if (uri.size() <= kSchemePrefix.size() ||
        uri.substr(0, kSchemePrefix.size()) != kSchemePrefix) {
        return std::nullopt;
    }
    std::string_view rest = uri.substr(kSchemePrefix.size());

    // Trim trailing whitespace / line-endings from the named-pipe payload
    // so a stray \r\n from the OS launch path doesn't fail the host check.
    while (!rest.empty() && (rest.back() == '\n' || rest.back() == '\r' ||
                              rest.back() == ' '  || rest.back() == '\t')) {
        rest.remove_suffix(1);
    }

    const size_t qmark = rest.find('?');
    if (qmark == std::string_view::npos) return std::nullopt;

    // Host should be "auth" -- accept a trailing slash since Windows /
    // Chromium-family browsers normalize host-only URIs as "auth/?..."
    // even though the portal redirect emits "auth?..." verbatim.
    std::string_view hostPath = rest.substr(0, qmark);
    if (hostPath != "auth" && hostPath != "auth/") return std::nullopt;

    std::string_view query = rest.substr(qmark + 1);
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

LicenseManager::~LicenseManager() {
    Shutdown();
}

void LicenseManager::Shutdown() {
    shuttingDown_.store(true);
    StopRefreshThread();
    if (exchangeThread_.joinable()) {
        exchangeThread_.join();
    }
    // Story 7.2 -- conversion-report worker is fire-and-forget and
    // bounded by the retry budget; it should always finish on its own.
    // Persist the queue so any timestamps still in memory survive the
    // exit (the worker writes the file after each operation, but a
    // mid-execution shutdown could miss the most recent enqueue).
    if (conversionThread_.joinable()) {
        conversionThread_.join();
    }
    // Normal shutdown still gets a chance to apply a completed conversion
    // worker result. This prevents a successful POST from being reported
    // again on the next launch just because the app closed before the next
    // frame drained the worker result.
    DrainPendingConversion();
    PersistConversionQueue();
}

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
    if (auto cachedEmail = config.UserEmail()) {
        userEmail_ = *cachedEmail;
    }

    EnsureSchemeRegistered();

    // Story 7.2 -- conversion queue persists across launches so an
    // export-while-offline-then-quit flow doesn't lose the report.
    LoadConversionQueue();

    // Story 6.3 -- verify the cached JWT before granting Authenticated.
    // Bad signature / expired / wrong issuer / missing audience all force
    // re-auth. ClearLicenseJwt persists the empty value through Config so
    // a corrupted cache doesn't bounce the user every relaunch.
    if (licenseJwt_ && !licenseJwt_->empty()) {
        if (VerifyJwt(*licenseJwt_)) {
            // Story 6.4 -- pull tier + conversions from the JWT payload
            // so the License Status badge has data on cold launch before
            // the first /api/desktop/token refresh lands.
            LoadClaimsFromJwt(*licenseJwt_);
            // Cold-launch follow-up: seed lastSyncedAt_ from the JWT's
            // iat claim so the License Status dialog shows a meaningful
            // "Last sync: ..." value before the immediate background
            // refresh completes. iat is when the server issued the
            // token, which is the most recent server-side touch.
            try {
                auto decoded = jwt::decode(*licenseJwt_);
                if (decoded.has_issued_at()) {
                    lastSyncedAt_ = decoded.get_issued_at();
                }
            } catch (const std::exception&) {
                // Already decoded successfully in VerifyJwt; ignore.
            }
            TransitionTo(AuthState::Authenticated);
            // Story 6.4 -- start the 24h refresh thread + fire one
            // immediate refresh so a stale cached JWT gets replaced
            // within seconds of launch.
            StartRefreshThread();
            NotifyRefreshNow();
        } else {
            MV_LOG_WARNING(
                "LicenseManager: cached JWT failed verification; clearing");
            Config::Instance().ClearLicenseJwt();
            licenseJwt_.reset();
            TransitionTo(AuthState::Unauthenticated);
        }
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

    if (!testMode_) {
        const std::string url = std::format(
            "{}/desktop-auth/init?state={}",
            LicenseConfig::kApiBaseUrl, token);
        if (!OpenUrl(url)) {
            MV_LOG_ERROR(std::format(
                "LicenseManager: failed to launch browser to {}", url));
            return;
        }
    } else {
        MV_LOG_INFO(
            "LicenseManager: test mode -- skipping browser launch");
    }

    authFlowState_ = token;
    authFlowStartedAt_ = std::chrono::steady_clock::now();
    TransitionTo(AuthState::Authenticating);
}

void LicenseManager::CancelAuthFlow() {
    // Accept both Authenticating (browser open) and Verifying (exchange
    // in flight) as cancellable. If the user cancels mid-exchange the
    // worker thread keeps running until libcurl returns; its result is
    // then discarded by DrainPendingExchange's state check.
    if (state_ != AuthState::Authenticating &&
        state_ != AuthState::Verifying) return;
    authFlowState_.clear();
    authFlowStartedAt_.reset();
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SignOut() {
    StopRefreshThread();
    auto& config = Config::Instance();
    config.ClearTokens();
    config.ClearLicenseJwt();
    config.ClearUserEmail();
    accessToken_.reset();
    refreshToken_.reset();
    licenseJwt_.reset();
    authFlowState_.clear();
    authFlowStartedAt_.reset();
    userEmail_.clear();
    tier_ = "free";
    licenseKey_.clear();
    conversionsUsed_ = 0;
    conversionsLimit_ = 0;
    conversionsResetAt_.clear();
    featureFlags_ = FeatureFlags{};  // restrictive defaults
    lastSyncedAt_.reset();
    offlineMode_ = false;
    // Story 7.2 -- queue is per-account; sign-out drops it. The queue
    // file is rewritten as an empty array so a sign-in by a different
    // user doesn't inherit the previous user's pending reports.
    {
        std::lock_guard<std::mutex> lock(conversionQueueMutex_);
        conversionQueue_.clear();
    }
    PersistConversionQueue();
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SetTestState(AuthState forced) {
    state_ = forced;
    initialized_ = true;
    testMode_ = true;
    MV_LOG_INFO(std::format("LicenseManager: test override -> {}",
                            StateName(forced)));
}

void LicenseManager::SetTestFeatureFlags(const FeatureFlags& flags) {
    if (!testMode_) {
        MV_LOG_WARNING(
            "LicenseManager::SetTestFeatureFlags ignored (test mode off)");
        return;
    }
    featureFlags_ = flags;
    if (flags.conversionsLimit > 0) {
        conversionsLimit_ = flags.conversionsLimit;
    }
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
    // Watch both Authenticating (browser open, no deep link yet) and
    // Verifying (deep link received, exchange in flight). The exchange
    // worker has its own 30s budget so Verifying typically self-resolves,
    // but the 5-min total guards against a hung libcurl edge case.
    if ((state_ != AuthState::Authenticating &&
         state_ != AuthState::Verifying) || !authFlowStartedAt_) return;
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
        // Log only the prefix (up to the query string) so the auth code
        // never lands in log files even on a malformed URI.
        const size_t qmark = uri.find('?');
        const std::string_view safePrefix =
            (qmark == std::string_view::npos) ? std::string_view(uri)
                                              : std::string_view(uri).substr(0, qmark);
        MV_LOG_WARNING(std::format(
            "LicenseManager: deep-link rejected (malformed URI): \"{}\"",
            safePrefix));
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

    MV_LOG_INFO("LicenseManager: state validated; spawning exchange worker");
    // PR #25 UI feedback: transition out of Authenticating so the
    // LoginScreen can render its dedicated Verifying view while the
    // exchange POST is in flight. authFlowStartedAt_ stays set so
    // TickTimeout's 5-min budget still covers the combined wait.
    TransitionTo(AuthState::Verifying);
    StartExchange(parsed->code);
}

void LicenseManager::StartExchange(const std::string& code) {
    if (exchangeRunning_.load()) {
        MV_LOG_WARNING(
            "LicenseManager: exchange already in flight; ignoring duplicate");
        return;
    }
    if (exchangeThread_.joinable()) exchangeThread_.join();

    exchangeRunning_.store(true);
    exchangeFinished_.store(false);

    // Capture by value -- the worker outlives this stack frame.
    std::string codeCopy = code;
    exchangeThread_ = std::thread([this, codeCopy = std::move(codeCopy)]() {
        ExchangeResult result;
        const std::string url = std::format(
            "{}/api/desktop/exchange", LicenseConfig::kApiBaseUrl);
        const std::string body = std::format(R"({{"code":"{}"}})", codeCopy);

        const auto deadline =
            std::chrono::steady_clock::now() +
            std::chrono::seconds(kExchangeBudgetSeconds);

        HttpClient http;
        HttpResponse resp;
        for (int attempt = 0; attempt < kExchangeMaxAttempts; ++attempt) {
            resp = http.PostJson(url, body, {});
            // Done if 2xx, or any 4xx that isn't 429 -- those are
            // terminal user-visible errors, not transient.
            const bool transient =
                (resp.status == 0) || (resp.status == 429) ||
                (resp.status >= 500);
            if (!transient) break;

            int waitSec = (resp.status == 429 && resp.retryAfterSeconds > 0)
                            ? static_cast<int>(resp.retryAfterSeconds)
                            : (1 << attempt);  // 1s, 2s, 4s, 8s
            const auto wakeAt =
                std::chrono::steady_clock::now() +
                std::chrono::seconds(waitSec);
            if (wakeAt > deadline) break;
            std::this_thread::sleep_until(wakeAt);
        }

        result.httpStatus = static_cast<int>(resp.status);

        if (resp.status == 200 && resp.errorMessage.empty()) {
            try {
                auto j = nlohmann::json::parse(resp.body);
                result.accessToken  = j.value("access_token", "");
                result.refreshToken = j.value("refresh_token", "");
                result.licenseJwt   = j.value("license_jwt", "");
                result.email        = j.value("email", "");
                if (result.accessToken.empty() ||
                    result.refreshToken.empty() ||
                    result.licenseJwt.empty()) {
                    result.errorMessage =
                        "Server response was missing required fields. "
                        "Please try again.";
                } else {
                    result.ok = true;
                }
            } catch (const std::exception& e) {
                MV_LOG_ERROR(std::format(
                    "LicenseManager: exchange JSON parse error: {}", e.what()));
                result.errorMessage =
                    "Could not understand the server response. "
                    "Please try again.";
            }
        } else if (resp.status == 400) {
            result.errorMessage =
                "The sign-in link expired. Please sign in again.";
        } else if (resp.status == 429) {
            result.errorMessage =
                "Server is busy. Please try again in a moment.";
        } else if (resp.status >= 500) {
            result.errorMessage =
                "Server error. Please try again in a few minutes.";
        } else if (resp.status == 0) {
            result.errorMessage =
                "Could not reach the server. Check your internet connection.";
        } else {
            result.errorMessage = std::format(
                "Sign-in failed (status {}). Please try again.", resp.status);
        }

        {
            std::lock_guard<std::mutex> lock(exchangeResultMutex_);
            exchangeResult_ = std::move(result);
        }
        exchangeFinished_.store(true);
    });
}

void LicenseManager::DrainPendingExchange() {
    if (!exchangeFinished_.load()) return;

    if (exchangeThread_.joinable()) exchangeThread_.join();
    exchangeRunning_.store(false);
    exchangeFinished_.store(false);

    ExchangeResult result;
    {
        std::lock_guard<std::mutex> lock(exchangeResultMutex_);
        result = std::move(exchangeResult_);
        exchangeResult_ = ExchangeResult{};
    }

    // If the user canceled while the worker was in flight, the auth
    // flow is no longer valid; discard whatever the server returned.
    // Verifying is the expected state by the time the result lands;
    // Authenticating only here for defense-in-depth in case some race
    // skips the HandleDeepLink transition.
    if (state_ != AuthState::Verifying &&
        state_ != AuthState::Authenticating) {
        MV_LOG_INFO(std::format(
            "LicenseManager: exchange completed but state is {} -- discarding",
            StateName(state_)));
        return;
    }

    if (!result.ok) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: exchange failed (status={}, msg={})",
            result.httpStatus, result.errorMessage));
        SetLastError(result.errorMessage);
        authFlowState_.clear();
        authFlowStartedAt_.reset();
        TransitionTo(AuthState::Unauthenticated);
        return;
    }

    // RS256 verification of the freshly-issued JWT -- catches a portal
    // misconfiguration before we persist a worthless token.
    if (!VerifyJwt(result.licenseJwt)) {
        MV_LOG_ERROR(
            "LicenseManager: server-issued JWT failed verification");
        SetLastError(
            "Server response could not be verified. Please try again.");
        authFlowState_.clear();
        authFlowStartedAt_.reset();
        TransitionTo(AuthState::Unauthenticated);
        return;
    }

    // Persist on the main thread -- Config::SetTokens / SetLicenseJwt /
    // SetUserEmail call Save() synchronously and are not thread-safe.
    auto& config = Config::Instance();
    config.SetTokens(result.accessToken, result.refreshToken);
    config.SetLicenseJwt(result.licenseJwt);
    if (!result.email.empty()) {
        config.SetUserEmail(result.email);
    }

    accessToken_  = result.accessToken;
    refreshToken_ = result.refreshToken;
    licenseJwt_   = result.licenseJwt;
    if (!result.email.empty()) {
        userEmail_ = result.email;
    }
    LoadClaimsFromJwt(result.licenseJwt);
    lastSyncedAt_ = std::chrono::system_clock::now();

    authFlowState_.clear();
    authFlowStartedAt_.reset();
    TransitionTo(AuthState::Authenticated);
    MV_LOG_INFO("LicenseManager: auth flow completed successfully");

    // Story 6.4 -- start the 24h /api/desktop/token refresh loop now
    // that we have valid tokens. No immediate re-fire (the JWT we just
    // verified is fresh).
    StartRefreshThread();
}

void LicenseManager::LoadClaimsFromJwt(const std::string& jwt) {
    // Decode payload only -- VerifyJwt has already run signature checks
    // by this point. jwt-cpp throws on malformed input; legacy claim values
    // stay visible for diagnostics, but feature flags reset first so a
    // bug-shaped refreshed JWT can never preserve stale paid entitlements.
    featureFlags_ = FeatureFlags{};
    try {
        auto decoded = jwt::decode(jwt);
        if (decoded.has_payload_claim("tier")) {
            tier_ = decoded.get_payload_claim("tier").as_string();
        }
        if (decoded.has_payload_claim("license_key")) {
            licenseKey_ = decoded.get_payload_claim("license_key").as_string();
        }
        if (decoded.has_payload_claim("conversions_used")) {
            const auto& c = decoded.get_payload_claim("conversions_used");
            conversionsUsed_ = static_cast<int>(c.as_integer());
        }
        if (decoded.has_payload_claim("conversions_limit")) {
            const auto& c = decoded.get_payload_claim("conversions_limit");
            conversionsLimit_ = static_cast<int>(c.as_integer());
        }
        if (decoded.has_payload_claim("conversions_reset_at")) {
            conversionsResetAt_ =
                decoded.get_payload_claim("conversions_reset_at").as_string();
        }

        // Story 7.1 -- feature_flags object. Restrictive defaults already
        // live on featureFlags_ from a fresh struct value; we overwrite
        // each field only when the JWT carries it. Missing fields (server
        // bug, tier downgrade racing the client) keep the Free-tier shape
        // so we never grant accidental upgrades.
        FeatureFlags ff{};
        bool sawFlags = false;
        if (decoded.has_payload_claim("feature_flags")) {
            sawFlags = true;
            const auto& claim = decoded.get_payload_claim("feature_flags");
            const auto& obj = claim.to_json();  // nlohmann::json under the traits header
            auto getInt = [&](const char* key, int fallback) {
                auto it = obj.find(key);
                return (it != obj.end() && it->is_number_integer())
                           ? it->get<int>()
                           : fallback;
            };
            auto getBool = [&](const char* key, bool fallback) {
                auto it = obj.find(key);
                return (it != obj.end() && it->is_boolean())
                           ? it->get<bool>()
                           : fallback;
            };
            auto getString = [&](const char* key) -> std::string {
                auto it = obj.find(key);
                return (it != obj.end() && it->is_string())
                           ? it->get<std::string>()
                           : std::string{};
            };

            ff.conversionsLimit  = getInt("conversions_limit", 0);
            ff.customHeadUpload  = getBool("custom_head_upload", false);
            ff.masking           = getBool("masking", false);
            ff.batchProcessing   = getBool("batch_processing", false);

            const std::string tt = getString("texture_transfer");
            if      (tt == "albedo_normal") ff.textureTransfer = FeatureFlags::TextureTransfer::AlbedoNormal;
            else if (tt == "albedo")        ff.textureTransfer = FeatureFlags::TextureTransfer::Albedo;
            else                            ff.textureTransfer = FeatureFlags::TextureTransfer::None;

            const std::string mp = getString("morphing_parameters");
            ff.morphingParameters = (mp == "full")
                                        ? FeatureFlags::MorphingParameters::Full
                                        : FeatureFlags::MorphingParameters::Presets;

            const std::string tu = getString("touchup_tools");
            if      (tu == "full")  ff.touchUpToolset = FeatureFlags::TouchUpToolset::Full;
            else if (tu == "basic") ff.touchUpToolset = FeatureFlags::TouchUpToolset::Basic;
            else                    ff.touchUpToolset = FeatureFlags::TouchUpToolset::BasicSmooth;

            // One-time warnings for missing fields so a bug-shaped JWT is
            // debuggable. Restrictive defaults still apply -- the warning
            // is informational.
            if (!obj.contains("conversions_limit"))     MV_LOG_WARNING("LicenseManager: feature_flags missing conversions_limit");
            if (!obj.contains("custom_head_upload"))    MV_LOG_WARNING("LicenseManager: feature_flags missing custom_head_upload");
            if (!obj.contains("texture_transfer"))      MV_LOG_WARNING("LicenseManager: feature_flags missing texture_transfer");
            if (!obj.contains("morphing_parameters"))   MV_LOG_WARNING("LicenseManager: feature_flags missing morphing_parameters");
            if (!obj.contains("masking"))               MV_LOG_WARNING("LicenseManager: feature_flags missing masking");
            if (!obj.contains("touchup_tools"))         MV_LOG_WARNING("LicenseManager: feature_flags missing touchup_tools");
        } else {
            MV_LOG_WARNING(
                "LicenseManager: JWT carries no feature_flags object -- "
                "defaulting to Free-tier-restrictive shape");
        }
        featureFlags_ = ff;

        MV_LOG_INFO(std::format(
            "LicenseManager: claims tier={}, conversions={}/{}, flags={}",
            tier_, conversionsUsed_, conversionsLimit_,
            sawFlags ? "parsed" : "defaulted"));
    } catch (const std::exception& e) {
        featureFlags_ = FeatureFlags{};
        MV_LOG_WARNING(std::format(
            "LicenseManager: failed to decode JWT claims: {}", e.what()));
    }
}

std::string LicenseManager::GetTier() const            { return tier_; }
std::string LicenseManager::GetLicenseKey() const      { return licenseKey_; }
int         LicenseManager::GetConversionsUsed() const { return conversionsUsed_; }
int         LicenseManager::GetConversionsLimit() const{ return conversionsLimit_; }
std::string LicenseManager::GetConversionsResetAt() const { return conversionsResetAt_; }
std::string LicenseManager::GetUserEmail() const       { return userEmail_; }

std::optional<std::chrono::system_clock::time_point>
LicenseManager::GetLastSyncedAt() const {
    return lastSyncedAt_;
}

void LicenseManager::RequestRefresh() {
    if (!refreshThread_.joinable()) {
        MV_LOG_INFO(
            "LicenseManager: RequestRefresh ignored (no refresh thread)");
        return;
    }
    MV_LOG_INFO("LicenseManager: manual refresh requested");
    NotifyRefreshNow();
}

void LicenseManager::StartRefreshThread() {
    // PR review fix: distinguish "thread is genuinely running" from
    // "thread soft-stopped via stopRefresh_ but its OS handle was never
    // reclaimed". The latter happens after RefreshOutcome::AuthInvalid:
    // DrainPendingRefresh sets stopRefresh_ + notifies (because it can't
    // call StopRefreshThread from inside the worker), the worker's loop
    // exits, but the std::thread handle is still joinable. Without this
    // join-and-reset path, a re-auth after an AuthInvalid flow would
    // silently fail to restart the refresh loop and the user's session
    // would lose 24h refresh until next launch -- meaning the cached
    // JWT could exhaust the 7-day offline grace with no prior refresh.
    if (refreshThread_.joinable()) {
        if (!stopRefresh_.load()) return;  // genuinely running
        refreshThread_.join();             // soft-stopped; reclaim handle
    }
    stopRefresh_.store(false);
    refreshFinished_.store(false);
    refreshThread_ = std::thread([this]() { RefreshThreadFn(); });
}

void LicenseManager::StopRefreshThread() {
    if (!refreshThread_.joinable()) return;
    {
        std::lock_guard<std::mutex> lock(refreshCvMutex_);
        stopRefresh_.store(true);
    }
    refreshCv_.notify_all();
    refreshThread_.join();
}

void LicenseManager::NotifyRefreshNow() {
    {
        std::lock_guard<std::mutex> lock(refreshCvMutex_);
        refreshFireImmediate_ = true;
    }
    refreshCv_.notify_all();
}

void LicenseManager::RefreshThreadFn() {
    while (true) {
        // Wait up to 24h for the timer; wake early on shutdown or
        // explicit refreshFireImmediate_ ping.
        {
            std::unique_lock<std::mutex> lock(refreshCvMutex_);
            refreshCv_.wait_for(lock, kRefreshInterval, [this] {
                return stopRefresh_.load() || refreshFireImmediate_;
            });
            if (stopRefresh_.load()) return;
            refreshFireImmediate_ = false;
        }

        // Snapshot tokens off the main-thread-managed fields. We only
        // read; the main thread writes them so a brief data race here is
        // harmless (worst case: the refresh fails with the old token and
        // retries on the next cycle). For tighter ordering we could add
        // a token mutex; not warranted for a 24h cadence.
        const std::string apiBase = LicenseConfig::kApiBaseUrl;
        const std::string supabaseUrl = LicenseConfig::kSupabaseProjectUrl;
        const std::string anonKey = LicenseConfig::kSupabaseAnonKey;
        const std::string accessTokenSnap =
            accessToken_.value_or(std::string{});
        const std::string refreshTokenSnap =
            refreshToken_.value_or(std::string{});

        if (accessTokenSnap.empty()) {
            MV_LOG_WARNING(
                "LicenseManager: refresh skipped (no access token)");
            continue;
        }

        HttpClient http;
        RefreshResult result;

        // Primary call: POST /api/desktop/token with the access token.
        const std::string tokenUrl = std::format("{}/api/desktop/token", apiBase);

        auto callTokenEndpoint = [&](const std::string& bearer) {
            return http.PostJsonBearer(tokenUrl, "{}", bearer);
        };

        HttpResponse resp = callTokenEndpoint(accessTokenSnap);

        // 401 -> refresh the Supabase access token via Option A (direct
        // to Supabase) per QA Round 2 Decision 2, then retry once.
        if (resp.status == 401 && !refreshTokenSnap.empty() &&
            !supabaseUrl.empty() && !anonKey.empty()) {
            MV_LOG_INFO(
                "LicenseManager: access token expired; refreshing via Supabase");
            const std::string supRefreshUrl = std::format(
                "{}/auth/v1/token?grant_type=refresh_token", supabaseUrl);
            const std::string supBody = std::format(
                R"({{"refresh_token":"{}"}})", refreshTokenSnap);
            HttpResponse supResp = http.PostJson(
                supRefreshUrl, supBody, {{"apikey", anonKey}});
            if (supResp.status == 200 && supResp.errorMessage.empty()) {
                try {
                    auto j = nlohmann::json::parse(supResp.body);
                    std::string newAccess  = j.value("access_token",  "");
                    std::string newRefresh = j.value("refresh_token", "");
                    if (!newAccess.empty() && !newRefresh.empty()) {
                        result.accessToken  = newAccess;
                        result.refreshToken = newRefresh;
                        // Retry the desktop endpoint with the fresh access token.
                        resp = callTokenEndpoint(newAccess);
                    }
                } catch (const std::exception& e) {
                    MV_LOG_WARNING(std::format(
                        "LicenseManager: Supabase refresh JSON parse: {}",
                        e.what()));
                }
            }
            if (resp.status == 401 || result.accessToken.empty()) {
                // Refresh token is also invalid -> force re-auth.
                result.outcome = RefreshOutcome::AuthInvalid;
                result.httpStatus = static_cast<int>(resp.status);
                result.errorMessage =
                    "Your session expired. Please sign in again.";
                {
                    std::lock_guard<std::mutex> lock(refreshResultMutex_);
                    pendingRefreshResult_ = std::move(result);
                }
                refreshFinished_.store(true);
                continue;
            }
        }

        result.httpStatus = static_cast<int>(resp.status);

        if (resp.status == 200 && resp.errorMessage.empty()) {
            try {
                auto j = nlohmann::json::parse(resp.body);
                std::string jwt = j.value("token", "");
                if (jwt.empty()) {
                    result.outcome = RefreshOutcome::TransportFail;
                    result.errorMessage = "Refresh response missing token.";
                } else {
                    result.outcome = RefreshOutcome::Success;
                    result.licenseJwt = std::move(jwt);
                    // accessToken/refreshToken populated above only if a
                    // Supabase refresh ran; otherwise leave empty so the
                    // drain knows not to overwrite the existing tokens.
                }
            } catch (const std::exception& e) {
                result.outcome = RefreshOutcome::TransportFail;
                result.errorMessage = std::format(
                    "Refresh response parse error: {}", e.what());
            }
        } else if (resp.status == 403 || resp.status == 404) {
            result.outcome = RefreshOutcome::Inactive;
            result.errorMessage =
                "Your subscription is inactive. "
                "Visit metavisage.io to manage it.";
        } else if (resp.status == 401) {
            // 401 with no available refresh path (no Supabase URL or no
            // refresh token) -- treat as auth invalid.
            result.outcome = RefreshOutcome::AuthInvalid;
            result.errorMessage =
                "Your session expired. Please sign in again.";
        } else {
            // 0 / 429 / 5xx / unknown -- transient. Keep state, log.
            result.outcome = RefreshOutcome::TransportFail;
            result.errorMessage = std::format(
                "Refresh failed (status {}).", resp.status);
        }

        {
            std::lock_guard<std::mutex> lock(refreshResultMutex_);
            pendingRefreshResult_ = std::move(result);
        }
        refreshFinished_.store(true);
    }
}

void LicenseManager::DrainPendingRefresh() {
    if (!refreshFinished_.load()) return;
    refreshFinished_.store(false);

    std::optional<RefreshResult> resultOpt;
    {
        std::lock_guard<std::mutex> lock(refreshResultMutex_);
        resultOpt.swap(pendingRefreshResult_);
    }
    if (!resultOpt) return;
    const auto& result = *resultOpt;

    switch (result.outcome) {
        case RefreshOutcome::Success: {
            // Verify + persist the freshly-issued JWT.
            if (!VerifyJwt(result.licenseJwt)) {
                MV_LOG_WARNING(
                    "LicenseManager: refreshed JWT failed verification");
                return;
            }
            auto& config = Config::Instance();
            if (!result.accessToken.empty() && !result.refreshToken.empty()) {
                // Supabase refresh ran -- persist new tokens too.
                config.SetTokens(result.accessToken, result.refreshToken);
                accessToken_  = result.accessToken;
                refreshToken_ = result.refreshToken;
            }
            config.SetLicenseJwt(result.licenseJwt);
            licenseJwt_ = result.licenseJwt;
            LoadClaimsFromJwt(result.licenseJwt);
            lastSyncedAt_ = std::chrono::system_clock::now();
            // Story 6.5 -- back online; clear the pill silently.
            if (offlineMode_) {
                MV_LOG_INFO("LicenseManager: back online; clearing offline pill");
                offlineMode_ = false;
            }
            // Stay in Authenticated; this was a background refresh.
            MV_LOG_INFO("LicenseManager: refresh succeeded");
            // Story 7.2 -- now that we're back online with a fresh token,
            // drain anything queued during prior offline windows.
            FlushConversionQueue();
            break;
        }
        case RefreshOutcome::Inactive: {
            MV_LOG_WARNING("LicenseManager: subscription inactive");
            SetLastError(result.errorMessage);
            offlineMode_ = false;
            TransitionTo(AuthState::Inactive);
            break;
        }
        case RefreshOutcome::AuthInvalid: {
            MV_LOG_WARNING(
                "LicenseManager: refresh auth invalid; forcing re-auth");
            SetLastError(result.errorMessage);
            // Clear caches so the next launch hits the LoginScreen
            // instead of a stale cached JWT.
            auto& config = Config::Instance();
            config.ClearTokens();
            config.ClearLicenseJwt();
            accessToken_.reset();
            refreshToken_.reset();
            licenseJwt_.reset();
            TransitionTo(AuthState::Unauthenticated);
            // Stop our own thread since SignOut was effectively done;
            // a successful re-auth via BeginAuthFlow restarts it.
            // Can't call StopRefreshThread from inside the thread; do
            // a soft-stop by setting the flag and letting the thread
            // return naturally on the next loop top.
            stopRefresh_.store(true);
            refreshCv_.notify_all();
            offlineMode_ = false;
            break;
        }
        case RefreshOutcome::TransportFail: {
            // Story 6.5 -- network unreachable / persistent 5xx. Cached
            // JWT is still valid (it was verified on every Authenticated
            // entry path), so we stay in Authenticated and surface the
            // offline pill. The next successful refresh clears it
            // silently.
            if (state_ == AuthState::Authenticated && !offlineMode_) {
                MV_LOG_WARNING(std::format(
                    "LicenseManager: refresh transport failure (status={}); "
                    "entering offline mode", result.httpStatus));
                offlineMode_ = true;
            }
            break;
        }
    }
}

bool LicenseManager::VerifyJwt(const std::string& jwt) const {
    // Development fallback per QA Round 2: with no public key embedded
    // (build profile = development and no keys/development-public.pem
    // committed yet), we skip signature verification and warn so the
    // localhost dogfooding path works pre-key-delivery. The CMake
    // configure step refuses to build production with an empty key, so
    // this fallback never escapes a production binary.
    const std::string_view publicKey = LicenseConfig::kLicenseJwtPublicKey;
    if (publicKey.empty()) {
        MV_LOG_WARNING(
            "LicenseManager: kLicenseJwtPublicKey is empty -- skipping JWT "
            "signature verification (development-only fallback)");
        return !jwt.empty();
    }

    try {
        auto decoded = jwt::decode(jwt);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::rs256(
                std::string(publicKey), "", "", ""))
            .with_issuer("metavisage-web")
            .with_audience("metavisage-desktop")
            .leeway(60);  // 60s clock-skew tolerance
        verifier.verify(decoded);
        return true;
    } catch (const std::exception& e) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: JWT verification failed: {}", e.what()));
        return false;
    }
}

void LicenseManager::SetLastError(std::string message) {
    std::lock_guard<std::mutex> lock(lastErrorMutex_);
    lastError_ = std::move(message);
}

std::optional<std::string> LicenseManager::ConsumeLastError() {
    std::lock_guard<std::mutex> lock(lastErrorMutex_);
    std::optional<std::string> out;
    out.swap(lastError_);
    return out;
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
    if (testMode_) {
        MV_LOG_INFO(
            "LicenseManager: test mode -- skipping HKCU scheme registration");
        schemeRegistered_ = true;
        return;
    }

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

// =============================================================================
// Story 7.2 -- conversion tracking + offline queue
// =============================================================================

namespace {

std::string ConversionQueuePath() {
    const std::string dir = MetaVisage::Paths::AppDataDir();
    if (dir.empty()) return {};
    return dir + "/conversion_queue.json";
}

#ifdef _WIN32
std::wstring Utf8ToWide(std::string_view value) {
    if (value.empty()) return {};
    const int wideChars = MultiByteToWideChar(
        CP_UTF8, 0,
        value.data(), static_cast<int>(value.size()),
        nullptr, 0);
    if (wideChars <= 0) return {};
    std::wstring out(static_cast<size_t>(wideChars), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0,
        value.data(), static_cast<int>(value.size()),
        out.data(), wideChars);
    return out;
}
#endif

}  // namespace

std::optional<LicenseManager::ConversionLimitNotice>
LicenseManager::ConsumeConversionLimitNotice() {
    std::optional<ConversionLimitNotice> out;
    std::lock_guard<std::mutex> lock(conversionLimitNoticeMutex_);
    out.swap(conversionLimitNotice_);
    return out;
}

int LicenseManager::GetPendingConversionCount() const {
    std::lock_guard<std::mutex> lock(
        const_cast<std::mutex&>(conversionQueueMutex_));
    return static_cast<int>(conversionQueue_.size());
}

void LicenseManager::EnqueueConversion(
        std::chrono::system_clock::time_point timestamp) {
    std::lock_guard<std::mutex> lock(conversionQueueMutex_);
    conversionQueue_.push_back(timestamp);
    while (conversionQueue_.size() > kConversionQueueCap) {
        // Drop oldest beyond the cap; one warning total per overflow.
        conversionQueue_.pop_front();
        MV_LOG_WARNING(
            "LicenseManager: conversion queue at cap; dropped oldest entry");
    }
}

void LicenseManager::RemoveCompletedConversions(
        const std::vector<std::chrono::system_clock::time_point>& completed) {
    if (completed.empty()) return;

    std::lock_guard<std::mutex> lock(conversionQueueMutex_);
    for (const auto& tp : completed) {
        const auto it = std::find(conversionQueue_.begin(),
                                  conversionQueue_.end(),
                                  tp);
        if (it != conversionQueue_.end()) {
            conversionQueue_.erase(it);
        }
    }
}

void LicenseManager::LoadConversionQueue() {
    const std::string path = ConversionQueuePath();
    if (path.empty()) return;
    std::ifstream f(path);
    if (!f) return;  // first run / file does not exist -- not an error
    try {
        nlohmann::json j;
        f >> j;
        if (!j.is_array()) return;
        std::lock_guard<std::mutex> lock(conversionQueueMutex_);
        conversionQueue_.clear();
        for (const auto& v : j) {
            if (!v.is_number_integer()) continue;
            const auto epochSec = v.get<int64_t>();
            conversionQueue_.push_back(
                std::chrono::system_clock::time_point(
                    std::chrono::seconds(epochSec)));
        }
        if (!conversionQueue_.empty()) {
            MV_LOG_INFO(std::format(
                "LicenseManager: loaded {} queued conversions from disk",
                conversionQueue_.size()));
        }
    } catch (const std::exception& e) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: failed to parse conversion_queue.json: {}; "
            "dropping queue", e.what()));
        std::lock_guard<std::mutex> lock(conversionQueueMutex_);
        conversionQueue_.clear();
    }
}

void LicenseManager::PersistConversionQueue() const {
    const std::string path = ConversionQueuePath();
    if (path.empty()) return;
    nlohmann::json j = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(
            const_cast<std::mutex&>(conversionQueueMutex_));
        for (const auto& tp : conversionQueue_) {
            const auto epochSec = std::chrono::duration_cast<std::chrono::seconds>(
                                      tp.time_since_epoch()).count();
            j.push_back(epochSec);
        }
    }
    // Atomic write through .tmp + rename so a power failure can't leave
    // a half-written file (mirrors the Sprint 6 session.cache pattern).
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            MV_LOG_WARNING("LicenseManager: failed to open conversion_queue.tmp");
            return;
        }
        out << j.dump();
    }
#ifdef _WIN32
    const std::wstring tmpWide = Utf8ToWide(tmp);
    const std::wstring pathWide = Utf8ToWide(path);
    if (tmpWide.empty() || pathWide.empty() ||
        !MoveFileExW(tmpWide.c_str(), pathWide.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: failed to persist conversion queue (Win32 {})",
            GetLastError()));
        std::error_code ignored;
        std::filesystem::remove(tmp, ignored);
    }
#else
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        MV_LOG_WARNING(std::format(
            "LicenseManager: failed to persist conversion queue: {}",
            ec.message()));
        std::filesystem::remove(tmp, ec);
    }
#endif
}

void LicenseManager::ReportConversion() {
    const auto now = std::chrono::system_clock::now();
    EnqueueConversion(now);
    PersistConversionQueue();

    // Offline path -- queue and bail. The next successful refresh
    // (DrainPendingRefresh -> FlushConversionQueue) drains.
    if (offlineMode_) {
        MV_LOG_INFO("LicenseManager: conversion queued (offline mode)");
        return;
    }

    // Already-running worker -- queue and let DrainPendingConversion()
    // start a follow-up flush after the active report completes cleanly.
    if (conversionRunning_.load()) {
        return;
    }

    // No active worker and we're online -- drain the durable queue.
    FlushConversionQueue();
}

void LicenseManager::FlushConversionQueue() {
    if (conversionRunning_.load()) return;  // drain starts a follow-up flush

    std::vector<std::chrono::system_clock::time_point> batch;
    {
        std::lock_guard<std::mutex> lock(conversionQueueMutex_);
        if (conversionQueue_.empty()) return;
        // Copy the queue into the batch, but keep it durable until the
        // server confirms each timestamp. This keeps effective usage honest
        // while the worker is in flight and avoids losing reports during
        // shutdown or a transient worker failure.
        batch.assign(conversionQueue_.begin(), conversionQueue_.end());
    }
    MV_LOG_INFO(std::format(
        "LicenseManager: flushing {} queued conversions", batch.size()));
    StartConversionWorker(std::move(batch));
}

void LicenseManager::StartConversionWorker(
        std::vector<std::chrono::system_clock::time_point> batch) {
    if (batch.empty()) return;
    if (conversionThread_.joinable()) conversionThread_.join();
    conversionRunning_.store(true);
    conversionFinished_.store(false);

    // Snapshot tokens for the worker -- it must not touch member fields
    // mid-flight. accessToken_ is std::optional; an empty token short-
    // circuits the POST and we re-queue the batch.
    std::string accessTokenCopy = accessToken_.value_or("");

    conversionThread_ = std::thread(
        [this, batch = std::move(batch),
         accessTokenCopy = std::move(accessTokenCopy)]() mutable {
            ConversionResult result;
            const std::string url = std::format(
                "{}/api/license/conversion", LicenseConfig::kApiBaseUrl);

            if (accessTokenCopy.empty()) {
                // Should never happen on the export path (we are
                // Authenticated); guard for completeness.
                result.errorMessage = "No access token; conversion deferred.";
                result.remaining = std::move(batch);
                {
                    std::lock_guard<std::mutex> lock(conversionResultMutex_);
                    conversionResult_ = std::move(result);
                }
                conversionFinished_.store(true);
                return;
            }

            HttpClient http;
            // POST one entry at a time so a 403 limit_reached mid-batch
            // can stop the rest cleanly.
            while (!batch.empty()) {
                const auto deadline =
                    std::chrono::steady_clock::now() +
                    std::chrono::seconds(kConversionBudgetSeconds);

                HttpResponse resp;
                bool retried = false;
                for (int attempt = 0; attempt < kConversionMaxAttempts; ++attempt) {
                    resp = http.PostJsonBearer(url, "{}", accessTokenCopy);
                    const bool transient =
                        (resp.status == 0) || (resp.status == 429) ||
                        (resp.status >= 500);
                    if (!transient) break;
                    int waitSec =
                        (resp.status == 429 && resp.retryAfterSeconds > 0)
                            ? static_cast<int>(resp.retryAfterSeconds)
                            : (1 << attempt);  // 1s, 2s, 4s, 8s
                    const auto wakeAt =
                        std::chrono::steady_clock::now() +
                        std::chrono::seconds(waitSec);
                    if (wakeAt > deadline) break;
                    retried = true;
                    std::this_thread::sleep_until(wakeAt);
                }
                (void)retried;

                result.httpStatus = static_cast<int>(resp.status);
                if (resp.status == 200 && resp.errorMessage.empty()) {
                    const auto completed = batch.front();
                    try {
                        auto j = nlohmann::json::parse(resp.body);
                        result.conversionsUsed   = j.value("conversions_used", 0);
                        result.conversionsLimit  = j.value("conversions_limit", 0);
                        result.conversionsResetAt = j.value("conversions_reset_at",
                                                            std::string{});
                        result.limitReached      = j.value("limit_reached", false);
                        result.ok = true;
                    } catch (const std::exception& e) {
                        MV_LOG_WARNING(std::format(
                            "LicenseManager: conversion JSON parse: {}", e.what()));
                    }
                    result.completed.push_back(completed);
                    batch.erase(batch.begin());  // entry succeeded; drop
                    continue;
                }
                if (resp.status == 403) {
                    // limit_reached -- response carries usage fields.
                    try {
                        auto j = nlohmann::json::parse(resp.body);
                        result.conversionsUsed    = j.value("conversions_used", 0);
                        result.conversionsLimit   = j.value("conversions_limit", 0);
                        result.conversionsResetAt = j.value("conversions_reset_at",
                                                            std::string{});
                    } catch (const std::exception&) { /* fields stay zero */ }
                    result.limitReached = true;
                    result.errorMessage = "Monthly conversion limit reached.";
                    // Drop the rest of the batch -- no point pushing more
                    // POSTs we know will all fail with the same 403.
                    batch.clear();
                    break;
                }
                // Any other terminal error: stop the batch and re-queue
                // remaining timestamps so we try again on the next refresh.
                result.errorMessage = resp.errorMessage.empty()
                    ? std::format("Conversion report failed (status {}).",
                                  resp.status)
                    : resp.errorMessage;
                break;
            }

            // Anything left in batch failed transiently and gets re-queued.
            if (!batch.empty()) {
                result.remaining = std::move(batch);
            }

            {
                std::lock_guard<std::mutex> lock(conversionResultMutex_);
                conversionResult_ = std::move(result);
            }
            conversionFinished_.store(true);
        });
}

void LicenseManager::DrainPendingConversion() {
    if (!conversionFinished_.load()) return;
    if (conversionThread_.joinable()) conversionThread_.join();
    conversionRunning_.store(false);
    conversionFinished_.store(false);

    ConversionResult result;
    {
        std::lock_guard<std::mutex> lock(conversionResultMutex_);
        result = std::move(conversionResult_);
        conversionResult_ = ConversionResult{};
    }

    // Apply usage fields from the server (200 or 403 both return them).
    // Local state mirrors server truth -- we never increment locally.
    if (result.httpStatus == 200 || result.httpStatus == 403) {
        if (result.conversionsLimit > 0) {
            conversionsUsed_   = result.conversionsUsed;
            conversionsLimit_  = result.conversionsLimit;
        }
        if (!result.conversionsResetAt.empty()) {
            conversionsResetAt_ = result.conversionsResetAt;
        }
    }

    if (!result.completed.empty()) {
        RemoveCompletedConversions(result.completed);
        PersistConversionQueue();
        MV_LOG_INFO(std::format(
            "LicenseManager: confirmed {} conversion report(s)",
            result.completed.size()));
    }

    // Unconfirmed timestamps remain in the durable queue while the worker is
    // in flight, so a transient failure only needs to log and persist.
    if (!result.remaining.empty()) {
        PersistConversionQueue();
        MV_LOG_INFO(std::format(
            "LicenseManager: kept {} conversion(s) queued after transient failure",
            result.remaining.size()));
    } else {
        // The batch fully cleared -- persist the queue after removing the
        // confirmed entries, plus anything queued during the worker run.
        PersistConversionQueue();
    }

    // 403 -> raise a one-shot block-modal notice. If another export
    // queued while this worker was in flight, drop it too: the server
    // says this account is at its cap, so additional reports cannot land
    // until the next reset or upgrade.
    if (result.limitReached) {
        {
            std::lock_guard<std::mutex> lock(conversionQueueMutex_);
            conversionQueue_.clear();
        }
        PersistConversionQueue();
        std::lock_guard<std::mutex> lock(conversionLimitNoticeMutex_);
        conversionLimitNotice_ = ConversionLimitNotice{
            result.conversionsLimit, result.conversionsResetAt};
        return;
    }

    // If another export happened while this worker was reporting, drain
    // that online backlog immediately after a clean completion instead of
    // waiting for the next 24h/manual refresh. Do not immediately retry a
    // transient failure; that path waits for refresh so it cannot spin.
    if (result.httpStatus == 200 &&
        result.remaining.empty() &&
        !offlineMode_ &&
        !shuttingDown_.load()) {
        FlushConversionQueue();
    }
}

}  // namespace MetaVisage
