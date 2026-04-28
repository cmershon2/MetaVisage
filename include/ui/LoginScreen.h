#ifndef METAVISAGE_UI_LOGINSCREEN_H
#define METAVISAGE_UI_LOGINSCREEN_H

namespace MetaVisage::LoginScreen {

// Sprint 3.8.12 — full-screen first-launch authentication surface. Net-new
// construction (no Sprint 6.1 auth backend has shipped); the screen is a
// purely visual scaffold so the design's intent can be reviewed alongside
// the rest of the chrome work. AppWindow gates it behind a dev-only Help
// menu toggle today; once the metavisage:// deep-link auth flow lands in
// Sprint 6.1, the same LoginScreen surface will drive the user-visible
// state machine.
//
// State machine — caller-owned. Render() never mutates state; it reports
// user clicks via the Result struct so AppWindow runs the transitions:
//
//   Idle → Sign in clicked   → Waiting   (browser would open here)
//   Waiting → Cancel clicked → Idle
//   Waiting → Retry clicked  → Waiting   (resets the spinner timer)
//   * → Esc / dev close      → close screen (dev-only escape hatch)
//
// Note: by product policy users must complete sign-in at least once per
// session — the design's "Use offline" option is intentionally omitted.
// The dev close X / Esc are scaffolding for visual review only and will
// be removed once the Sprint 6.1 auth backend ships.
//
// `tickSeconds` is `ImGui::GetTime()` for the waiting-state spinner ring.

enum class State {
    Idle,        // user has not yet clicked Sign In
    Waiting,     // browser opened, waiting on the OS deep-link callback
};

struct Result {
    bool signInRequested = false;  // Idle → Waiting
    bool cancelRequested = false;  // Waiting → Idle
    bool retryRequested  = false;  // Waiting reset (re-open browser)
    bool closeRequested  = false;  // dev escape hatch (Esc / top-right X)
};

// Renders the login screen as a full-viewport window covering all other UI.
// Caller is responsible for visibility (don't call when the user is signed
// in). All state transitions are pushed back through the Result struct.
Result Render(State state, double tickSeconds);

} // namespace MetaVisage::LoginScreen

#endif // METAVISAGE_UI_LOGINSCREEN_H
