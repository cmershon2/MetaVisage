#include "licensing/LicenseManager.h"

#include "licensing/LicenseConfig.h"
#include "utils/Config.h"
#include "utils/Logger.h"

#include <format>
#include <string_view>

namespace MetaVisage {

namespace {

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
    // Story 6.2 fills in: random state generation + browser launch +
    // 5-minute timeout watchdog. 6.1 only flips state so LoginScreen
    // shows the Waiting spinner.
    TransitionTo(AuthState::Authenticating);
}

void LicenseManager::CancelAuthFlow() {
    if (state_ != AuthState::Authenticating) return;
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SignOut() {
    auto& config = Config::Instance();
    config.ClearTokens();
    config.ClearLicenseJwt();
    accessToken_.reset();
    refreshToken_.reset();
    licenseJwt_.reset();
    TransitionTo(AuthState::Unauthenticated);
}

void LicenseManager::SetTestState(AuthState forced) {
    state_ = forced;
    initialized_ = true;
    MV_LOG_INFO(std::format("LicenseManager: test override -> {}",
                            StateName(forced)));
}

void LicenseManager::TransitionTo(AuthState next) {
    if (state_ == next) return;
    MV_LOG_INFO(std::format("LicenseManager: {} -> {}",
                            StateName(state_),
                            StateName(next)));
    state_ = next;
}

}  // namespace MetaVisage
