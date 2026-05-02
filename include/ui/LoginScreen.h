#ifndef METAVISAGE_UI_LOGINSCREEN_H
#define METAVISAGE_UI_LOGINSCREEN_H

namespace MetaVisage::LoginScreen {

// Sprint 3.8.12 introduced the visual scaffold; Sprint 6.1 promoted it to
// the live first-launch authentication surface. The screen is rendered
// whenever LicenseManager::GetState() is Unauthenticated or Authenticating
// and fully covers the app shell -- no dev close X / Esc escape hatches
// remain (those were dev-only scaffolding pre-Sprint 6.1).
//
// State machine -- caller-owned. Render() never mutates state; it reports
// user clicks via the Result struct so AppWindow runs the transitions:
//
//   Idle    -> Sign in clicked   -> Waiting (caller calls BeginAuthFlow)
//   Waiting -> Cancel clicked    -> Idle    (caller calls CancelAuthFlow)
//   Waiting -> Retry clicked     -> Waiting (caller re-invokes BeginAuthFlow)
//   Waiting -> Esc pressed       -> Idle    (same as Cancel)
//
// `tickSeconds` is `ImGui::GetTime()` for the waiting-state spinner ring.

enum class State {
    Idle,        // user has not yet clicked Sign In
    Waiting,     // browser opened, waiting on the OS deep-link callback
};

struct Result {
    bool signInRequested = false;  // Idle    -> Waiting
    bool cancelRequested = false;  // Waiting -> Idle
    bool retryRequested  = false;  // Waiting reset (re-open browser)
};

// Renders the login screen as a full-viewport window covering all other UI.
// Caller is responsible for visibility (don't call when the user is signed
// in). All state transitions are pushed back through the Result struct.
Result Render(State state, double tickSeconds);

} // namespace MetaVisage::LoginScreen

#endif // METAVISAGE_UI_LOGINSCREEN_H
