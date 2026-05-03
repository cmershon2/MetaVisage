#ifndef METAVISAGE_LICENSING_LICENSEMANAGER_H
#define METAVISAGE_LICENSING_LICENSEMANAGER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace MetaVisage {

// Sprint 6 Story 6.1+ -- central owner of the desktop auth + license state.
//
// Responsibilities (filled in incrementally across Sprint 6):
//   6.1  state machine, cached-session bootstrap, sign-in screen gating
//   6.2  BeginAuthFlow opens the browser; metavisage:// deep link parsed
//        and validated against the in-flight state token
//   6.3  ExchangeCode trades the deep-link code for tokens, persists them
//        encrypted via Config + SecureStorage, verifies the RS256 JWT
//   6.4  24h refresh timer; Supabase access-token refresh on 401
//   6.5  7-day offline grace using the cached JWT
//   6.6  SignOut clears caches and returns to Unauthenticated
//
// Per CLAUDE.md per-frame state model: AppWindow + LoginScreen poll
// GetState() each frame; transitions are pushed by the network/IPC
// layers, never by the UI.
//
// Singleton via Meyer's pattern -- matches Logger + Config.
class LicenseManager {
public:
    enum class AuthState {
        Unauthenticated,  // No valid cached session -- sign in required.
        Authenticating,   // Browser open, waiting for the deep-link callback.
        Authenticated,    // Valid session, app is unlocked.
        Expired,          // Cached JWT past exp + offline cache expired (6.5).
        Inactive,         // Server returned 403/404 -- subscription not active (6.4).
    };

    static LicenseManager& Instance();

    // Bootstraps state from the encrypted token cache in
    // %APPDATA%\MetaVisage\config.json. Safe to call once after
    // Config::Instance().Load(); idempotent on subsequent calls. Network
    // and signature verification are layered on by Stories 6.3-6.5; in 6.1
    // a non-empty cached JWT counts as Authenticated (forward-compatible
    // since 6.3's verifier rejects bad JWTs in the same path).
    void Initialize();

    AuthState GetState() const { return state_; }

    // Generates a CSPRNG state token, opens the system browser to
    // <api>/desktop-auth/init?state=<token>, and transitions to
    // Authenticating. Browser-launch failure stays in Unauthenticated and
    // logs a warning -- the user can retry from the LoginScreen.
    void BeginAuthFlow();

    // Aborts an in-flight auth flow; returns to Unauthenticated.
    void CancelAuthFlow();

    // Story 6.2 -- thread-safe deep-link queue. Called from main.cpp's
    // argv[1] parser (cold launch) and from SingleInstance's onRaise
    // callback (already-running launch fires from a worker thread). The
    // URI is queued and consumed on the main thread next frame.
    void QueueDeepLink(std::string uri);

    // Main-thread drain. Called once per frame from AppWindow::TickOnce
    // before the auth-state read so transitions land in the same frame.
    void DrainPendingDeepLink();

    // Main-thread timeout watchdog. If the auth flow has been
    // Authenticating for more than 5 minutes, transitions back to
    // Unauthenticated. Called once per frame from AppWindow::TickOnce.
    void TickTimeout();

    // Story 6.3 -- main-thread drain for the /api/desktop/exchange worker
    // thread. Joins the thread, persists tokens via Config, RS256-verifies
    // the JWT, and transitions to Authenticated on success or back to
    // Unauthenticated on failure (with a user-facing message available
    // via ConsumeLastError).
    void DrainPendingExchange();

    // Story 6.4 -- main-thread drain for the 24h /api/desktop/token
    // refresh worker. Persists the new tokens, parses tier/conversions
    // from the JWT payload, and reacts to 403/404 (-> Inactive) and
    // refresh-token-invalid 401 (-> Unauthenticated, force re-auth).
    void DrainPendingRefresh();

    // Story 6.6 read accessors -- populated by ApplyAuthSuccess /
    // DrainPendingRefresh / cached-JWT bootstrap. Empty / zero values
    // before the first successful exchange.
    std::string GetTier() const;
    int GetConversionsUsed() const;
    int GetConversionsLimit() const;
    std::string GetConversionsResetAt() const;
    std::string GetUserEmail() const;
    std::string GetLicenseKey() const;

    // Wall-clock time of the last successful exchange or refresh. nullopt
    // before the first sync this session. Drives the "Last sync ..."
    // display in the License Status modal.
    std::optional<std::chrono::system_clock::time_point> GetLastSyncedAt() const;

    // Wakes the 24h refresh thread immediately. Used by the License
    // Status modal's manual "Refresh license" button. No-op if the
    // refresh thread isn't running (Unauthenticated state).
    void RequestRefresh();

    // Story 6.5 -- true when we're operating off the cached JWT because
    // the most recent /api/desktop/token refresh hit a transport failure
    // (network unreachable, persistent 5xx, etc). State stays Authenticated;
    // AppWindow renders an amber pill in the status bar so the user knows.
    // Cleared on the next successful refresh or any state transition out
    // of Authenticated.
    bool IsOfflineMode() const { return offlineMode_; }

    // Returns + clears any user-facing error string queued by
    // DrainPendingExchange / DrainPendingDeepLink. AppWindow polls this
    // each frame and surfaces results via errorModal_. Empty optional
    // means "no error pending".
    std::optional<std::string> ConsumeLastError();

    // Joins the in-flight exchange worker thread (if any). AppWindow
    // calls this from Shutdown so the process doesn't exit while the
    // worker is mid-network-call.
    void Shutdown();

    // Drops cached tokens and returns to Unauthenticated. Story 6.6 wires
    // the License Status dialog Sign Out button to this.
    void SignOut();

    // Test-only: pin the state machine to a specific AuthState and mark
    // initialized so subsequent Initialize() calls are no-ops. Used by
    // the smoke_ui harness to bypass the LoginScreen gate when exercising
    // the rest of the app shell. Must be called before AppWindow::Initialize
    // so the cached-session bootstrap doesn't overwrite the forced value.
    // Side effect: sets a test-mode flag that suppresses browser launch
    // (BeginAuthFlow) and registry writes (EnsureSchemeRegistered) so the
    // harness can exercise the LoginScreen state transitions without
    // opening real browsers or touching HKCU.
    // Production code never calls this.
    void SetTestState(AuthState forced);

private:
    LicenseManager();
    ~LicenseManager();
    LicenseManager(const LicenseManager&) = delete;
    LicenseManager& operator=(const LicenseManager&) = delete;

    void TransitionTo(AuthState next);

    // Story 6.2 helpers (main thread only).
    void HandleDeepLink(const std::string& uri);
    std::string GenerateStateToken() const;
    void EnsureSchemeRegistered();

    // Story 6.3 helpers.
    void StartExchange(const std::string& code);  // main thread; spawns worker
    bool VerifyJwt(const std::string& jwt) const; // pure compute; safe anywhere
    void LoadClaimsFromJwt(const std::string& jwt); // tier + conversions
    void SetLastError(std::string message);       // any thread (mutex'd)
    static constexpr int kExchangeMaxAttempts = 4;
    static constexpr int kExchangeBudgetSeconds = 30;

    // Story 6.4 helpers -- 24h refresh thread.
    void StartRefreshThread();   // launched once when entering Authenticated
    void StopRefreshThread();    // called from Shutdown
    void RefreshThreadFn();      // worker body
    void NotifyRefreshNow();     // wakes the thread to fire immediately
    static constexpr auto kRefreshInterval = std::chrono::hours(24);

    AuthState state_ = AuthState::Unauthenticated;
    bool initialized_ = false;
    bool schemeRegistered_ = false;
    bool testMode_ = false;

    // Populated by Initialize() from the encrypted Config cache. Stories
    // 6.2-6.4 read/write these fields directly so the public API stays small.
    std::optional<std::string> accessToken_;
    std::optional<std::string> refreshToken_;
    std::optional<std::string> licenseJwt_;

    // Story 6.2 -- in-flight auth flow state. authFlowState_ is the
    // single-use CSPRNG token compared against the deep-link's `state`
    // parameter; cleared after validation. authFlowStartedAt_ drives the
    // 5-minute timeout watchdog.
    std::string authFlowState_;
    std::optional<std::chrono::steady_clock::time_point> authFlowStartedAt_;

    // Listener-thread -> main-thread deep-link queue. Held briefly during
    // QueueDeepLink (writer) and DrainPendingDeepLink (reader).
    std::mutex deepLinkMutex_;
    std::optional<std::string> pendingDeepLink_;

    // Story 6.3 -- exchange worker thread + result hand-off. The thread
    // runs the full POST + retry loop with no Config / UI access; the
    // main thread drains the result via DrainPendingExchange after
    // exchangeFinished_ flips true.
    struct ExchangeResult {
        bool ok = false;
        int httpStatus = 0;
        std::string accessToken;
        std::string refreshToken;
        std::string licenseJwt;
        std::string email;
        std::string errorMessage;  // user-facing on !ok
    };
    std::thread exchangeThread_;
    std::atomic<bool> exchangeRunning_{false};
    std::atomic<bool> exchangeFinished_{false};
    std::mutex exchangeResultMutex_;
    ExchangeResult exchangeResult_;

    // User-facing error queue (any thread can push via SetLastError;
    // main thread consumes via ConsumeLastError).
    std::mutex lastErrorMutex_;
    std::optional<std::string> lastError_;

    // Story 6.6 will surface this in the License Status dialog. Captured
    // at exchange time (per QA Round 2 Decision 3 -- email lives on the
    // exchange response body, not in the JWT payload).
    std::string userEmail_;

    // Story 6.4 -- claims parsed from the JWT payload. Refreshed on every
    // successful exchange / refresh; persists across launches because the
    // JWT itself is cached.
    std::string tier_ = "free";
    std::string licenseKey_;
    int conversionsUsed_ = 0;
    int conversionsLimit_ = 0;
    std::string conversionsResetAt_;

    // Last successful sync (exchange or refresh). Used by the License
    // Status dialog's "Last sync ..." line.
    std::optional<std::chrono::system_clock::time_point> lastSyncedAt_;

    // Story 6.5 -- offline-mode flag (main-thread only). Toggled in
    // DrainPendingRefresh based on the worker's outcome.
    bool offlineMode_ = false;

    // Story 6.4 -- refresh worker. Wakes every 24h (or sooner via
    // NotifyRefreshNow) until stopRefresh_ flips. Result is published to
    // pendingRefreshResult_ + refreshFinished_; main thread drains via
    // DrainPendingRefresh.
    enum class RefreshOutcome {
        Success,        // 200 -- new tokens persisted
        Inactive,       // 403/404 -- subscription terminated
        AuthInvalid,    // 401 + supabase refresh failed -- force re-auth
        TransportFail,  // 0/5xx persistent / 429 over budget -- keep state
    };
    struct RefreshResult {
        RefreshOutcome outcome = RefreshOutcome::TransportFail;
        int httpStatus = 0;
        std::string accessToken;
        std::string refreshToken;
        std::string licenseJwt;
        std::string errorMessage;
    };
    std::thread refreshThread_;
    std::atomic<bool> stopRefresh_{false};
    std::atomic<bool> refreshFinished_{false};
    std::mutex refreshCvMutex_;
    std::condition_variable refreshCv_;
    bool refreshFireImmediate_ = false;  // guarded by refreshCvMutex_
    std::mutex refreshResultMutex_;
    std::optional<RefreshResult> pendingRefreshResult_;
};

}  // namespace MetaVisage

#endif  // METAVISAGE_LICENSING_LICENSEMANAGER_H
