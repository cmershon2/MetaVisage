#ifndef METAVISAGE_LICENSING_LICENSEMANAGER_H
#define METAVISAGE_LICENSING_LICENSEMANAGER_H

#include <optional>
#include <string>

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

    // Story 6.2 wires the browser launch + state token; 6.1 just transitions
    // AuthState so the LoginScreen flips to its Waiting view.
    void BeginAuthFlow();

    // Aborts an in-flight auth flow; returns to Unauthenticated.
    void CancelAuthFlow();

    // Drops cached tokens and returns to Unauthenticated. Story 6.6 wires
    // the License Status dialog Sign Out button to this.
    void SignOut();

    // Test-only: pin the state machine to a specific AuthState and mark
    // initialized so subsequent Initialize() calls are no-ops. Used by
    // the smoke_ui harness to bypass the LoginScreen gate when exercising
    // the rest of the app shell. Must be called before AppWindow::Initialize
    // so the cached-session bootstrap doesn't overwrite the forced value.
    // Production code never calls this.
    void SetTestState(AuthState forced);

private:
    LicenseManager();
    ~LicenseManager() = default;
    LicenseManager(const LicenseManager&) = delete;
    LicenseManager& operator=(const LicenseManager&) = delete;

    void TransitionTo(AuthState next);

    AuthState state_ = AuthState::Unauthenticated;
    bool initialized_ = false;

    // Populated by Initialize() from the encrypted Config cache. Stories
    // 6.2-6.4 read/write these fields directly so the public API stays small.
    std::optional<std::string> accessToken_;
    std::optional<std::string> refreshToken_;
    std::optional<std::string> licenseJwt_;
};

}  // namespace MetaVisage

#endif  // METAVISAGE_LICENSING_LICENSEMANAGER_H
