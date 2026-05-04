# Sprint 6 Retrospective

**Date**: 2026-05-04
**Sprint**: [Sprint 6 — Licensing Infrastructure: Core Integration](../sprints/sprint-06.md)
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary

- **Velocity (Planned)**: 26 points
- **Velocity (Actual)**: 26 points landed (100% on commitment)
- **Sprint Goal Achievement**: 100% on shipped scope; the licensing core integration runs end-to-end on the development profile, including all three rounds of UI/UX feedback that landed pre-merge
- **Stories Completed**: 7 of 7 (Stories 6.1 / 6.2 / 6.3 / 6.4 / 6.5 / 6.6 / 6.7)
- **Carryover Items**: None on the committed scope. Two follow-ups tracked: (1) cottage-cheese-face root-cause spike (Sprint 5 retro AI #2 → Sprint 6 deferred at planning → Sprint 7), (2) Stage Stepper `smoke_ui` scenario (Sprint 5 retro AI #1 partial — Login Screen portion shipped via Story 6.7).

---

## Retrospective

### LIKED (What we liked)

- **Three consecutive 100%-on-commitment sprints (5 → 6).** Sprint 4 was 22/27 (81%, Story 4.5 deferred), Sprint 5 was 22/22, Sprint 6 is 26/26. The pattern of "commit ~80% of capacity, ship 100% of commit" is now load-bearing — both `/sprint-planner` (deliberate scope reservation) and `/sprint-retrospective`'s mechanical gate (forces honest validation) are doing real work.
- **The architect-audit pattern continues to compound.** Three fresh audits this sprint (`sprint6_story62_audit.md` / `sprint6_story63_audit.md` / `sprint6_story66_audit.md`), each one shaving iteration time off implementation by surfacing load-bearing constraints up-front. The Story 6.3 audit catching that `Config` already had encrypted token storage saved a half-day of duplicated session-cache work; the Story 6.6 audit identifying `ShortcutsModalState` as the simplest template made the License Status modal a 2-hour port.
- **Live user testing rounds drove three meaningful fixes that wouldn't have surfaced any other way.** Round 1 caught the `Cond_Appearing` + auto-resize centering bug, the foreground-lock issue (`SetForegroundWindow` from a non-foreground process), and the LoginScreen Waiting state's design-scaffold copy. Round 2 caught the deeper diagnoses (`Cond_Always` not `Cond_Appearing`, `AllowSetForegroundWindow(ASFW_ANY)` from the second instance side, Retry needs Cancel-then-Begin). Round 3 surfaced the `Verifying` state need that rehauled the auth state machine cleanly. Iterative loop with the user worked smoothly even when the round-1 fixes were partial.
- **Web team Q&A round-trip ([DESKTOP_INTEGRATION_QA.md](../DESKTOP_INTEGRATION_QA.md)) saved real time.** The questions surfaced architectural decisions that would have been blocking mid-sprint: HS256 → RS256 migration ahead of launch (ten-line CMake change, cleaner key-rotation hygiene), Option A direct-to-Supabase refresh (no portal-side `/api/desktop/refresh` endpoint needed), email on the exchange response (not in the JWT), `license_key` decoded from the JWT payload (no separate field). All four landed in code without rework.
- **Per-story incremental commit discipline plus a kickoff "infrastructure first" commit (build profile + env templates + Q&A doc) made the diff legible.** PR #25 has 13 commits, each with a clean conventional-commit subject and a body that explains the why. Bisectable, reviewable, and each commit's tests stay green.
- **PR #25 review feedback was substantive — and the blocking finding was a real bug.** The `StartRefreshThread` joinable-guard miss after `AuthInvalid` recovery would have caused users whose refresh tokens expire mid-session to silently lose 24h refresh until next launch, exhausting the 7-day offline grace with no warning. Code-reviewer caught it; commit `104b809` fixes it. The non-blocking findings (CMake guard parity, magic literal cleanup) were also worth implementing.
- **The `Verifying` state refactor was satisfying.** Adding a new state value mid-PR could have been a sprawling change, but the existing state-machine had good naming hygiene — the refactor was 5 files, every site that referenced `Authenticating` already needed updating, and the pattern fell out cleanly.

### LEARNED (What we learned)

- **ImGui's `SetNextWindowPos` pivot calculation runs at the moment `SetNextWindowPos` is called, not when the window appears.** With `Cond_Appearing` + `AlwaysAutoResize`, the size-zero first frame placed the popup's top-left at the viewport center — visually a popup hugging the lower-right quadrant. `Cond_Always` re-applies the centered pivot every frame using the now-known size. Worth filing in the code-reviewer's conventions memory for future modal work that uses auto-resize.
- **Windows' foreground-lock rules are real and bite specifically when one process tries to focus another.** The fix isn't on the focused process's side (it doesn't have foreground rights to take); it's on the *focusing* process's side: a just-spawned helper has transient foreground rights and can grant them via `AllowSetForegroundWindow(ASFW_ANY)`. `SingleInstance::TryAcquire`'s second-instance branch was the right place. This pattern generalizes to any cross-process focus handoff (auto-update relaunch, file-handler launches, etc.).
- **Custom URL scheme launches normalize host-only URIs by appending a trailing slash on Windows / Chromium browsers.** `metavisage://auth/?...` not `metavisage://auth?...` — the actual URI bytes vs. expected differed by exactly 1. Strict-prefix parsers break here. The rejection log including the URI prefix (truncated before `?` to keep auth codes out of logs) made it debuggable in seconds. URI parsers should always tolerate the trailing slash on the host segment.
- **`jwt-cpp`'s `decode` / `verify` need an explicit `json_traits` template parameter.** The plain header `<jwt-cpp/jwt.h>` exposes only the templated forms. The `defaults.h` family of headers wires the traits in so call sites stay clean — `<jwt-cpp/traits/nlohmann-json/defaults.h>` solved every call-site error in one include change. Worth documenting at the top of `LicenseManager.cpp` so future readers don't repeat the discovery.
- **The two-phase threading pattern (worker writes to mutex-guarded result struct + atomic completion flag, main thread drains via `Drain*` functions each frame) generalizes cleanly across two new worker contexts.** Sprint 5 had one worker (the morph deformer); Sprint 6 added two more (exchange POST, 24h refresh loop). Pattern stayed identical; the only twist was `StartRefreshThread`'s soft-stop edge case (caught in PR review).
- **Late-stage UX feedback that introduces a NEW STATE is a clean refactor when the existing state machine has good naming hygiene.** `Verifying` slotted between `Authenticating` and `Authenticated` cleanly — every site referencing one already needed updating, the audit trail showed exactly where, and the test bypass (`SetTestState`) handled the new state automatically since it was already structured to accept any `AuthState` value.
- **DPAPI-encrypted Config storage was already wired from Sprint 5's auth tokens — but the email field wasn't.** Adding `UserEmail()` / `SetUserEmail()` / `ClearUserEmail()` was a 30-line follow-up, but the bug surfaced live ("the email shows an em-dash on cold launch") because `LoadClaimsFromJwt` and the exchange response handler weren't symmetric. Pattern for next time: when adding a new persisted field, walk every `Initialize` / `LoadClaims*` / exchange / refresh / SignOut path to confirm symmetry.

### LACKED (What we lacked)

- **A formal Phase D ceremonial single-pass smoke didn't happen this sprint.** The licensing surface was thoroughly exercised iteratively (multiple cold launches, sign-in cycles, modal interaction, retry path verification across two UI feedback rounds), but the "Stage 1 → 2 → 3 → 4 → Export end-to-end" pass that Sprint 4 retro AI #3 added to the DoD wasn't done as one sitting. Marked done with a qualifier note; carrying as a pre-merge verification step ahead of PR #25 landing.
- **Phase D was specced against the staging build profile, but staging endpoints aren't deployed** (per QA Round 2 Open Item #1), so all testing happened against the development profile + localhost portal. Known gap; lands the moment the web team deploys staging.
- **The `license_key` claim was missing from `LoadClaimsFromJwt` for the duration of Stories 6.4–6.6.** Only caught during the live cold-launch test when the user noticed the License Status dialog showed an em-dash for the email and the team's UI-feedback screenshot showed a license key field that we couldn't populate. Cost a follow-up commit (`f9d1d52`). Symmetry-walk pattern (above) would have caught it.
- **The expired-offline-cache blocking modal was deferred mid-Story 6.5 with a commit-body note rather than a backlog item.** It's tracked in Sprint 6's Sprint Outcome Notes now, but a dedicated backlog row would surface it during the next sprint planning cycle. Easy to miss in a hand-off.
- **`smoke_ui` doesn't yet cover the actual deep-link → exchange round trip.** Story 6.7's Sign In → Cancel cycle catches the LoginScreen state-machine path but not the network exchange. A test-mode HTTP fixture (mock portal + injected fake auth code) would close this — the Story 5.4 spec called it out as Sprint 6+ work and it's still open.

### LONGED FOR (What we longed for)

- **A "verify the cached JWT works end-to-end" smoke fixture.** A pinned RS256 keypair with a sample JWT carrying the full feature-flag payload, fed into `LicenseManager` via a test-only API, would catch the `license_key` regression class earlier and let `smoke_ui` exercise the full Authenticated path without the real web team's keys. Estimated ~80 LoC to set up; would pay for itself across Sprint 7's feature-gating work.
- **Stage 3 process+Accept and Stage 4 brush+sculpt+undo `smoke_ui` scenarios** — the deferred Story 5.4 originals. Sprint 6's Login Screen scenario closes the carryover label but the full lane the original spec called for is still pending. Sprint 7's feature-gating tests will need realistic project-state fixtures anyway; pair the two efforts.
- **Better Windows native-API ergonomics in the codebase.** `AllowSetForegroundWindow`, `SetForegroundWindow`, `IsIconic`, `BCryptGenRandom`, `RegCreateKeyExW`, `RegSetValueExW`, `MultiByteToWideChar` — Sprint 6 added a lot of raw Win32 surface scattered through `LicenseManager.cpp`, `SingleInstance.cpp`, `AppWindow.cpp`. A small `utils/Win32Window` (`RestoreAndFocus(hwnd)` + `RegistryWriteString(...)` wrappers) plus a `utils/Random` (`GenerateHexToken(byteCount)`) would centralize the `#ifdef _WIN32` blocks. Not urgent; refactor candidate.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | **Phase D pre-merge gate before PR #25 lands on `development`.** Run the deferred Stage 1 → 2 → 3 → 4 → Export workflow with the bundled MetaHuman head, plus Wi-Fi-off offline-pill verification, plus Undo/Redo on point placement / gizmo drag / mask paint / sculpt brush. Log the result in PR #25 before merge. | Casey | Pre-merge of PR #25 | [ ] |
| 2 | **Cottage-cheese-face root-cause spike in Sprint 7.** Sprint 5 retro AI #2 → Sprint 6 carryover → Sprint 7. The diagnostic dump (`METAVISAGE_DUMP_RESIDUALS=1`) and the three audit-flagged hypothesis sites (NRICPDeformer.cpp:764-790 landmark-weight floor, :198-286 UV-seam BFS, :350+387 excluded-vertex landmark mismatch) are all primed. **If Sprint 7 can't fit it either, explicitly close the spike with "Outcome B stopgap is the answer" or commit to a dedicated mini-sprint** — three-sprint-deferred is the same anti-pattern Sprint 4's retro flagged. | Casey | Sprint 7 close (or explicit closure decision) | [ ] |
| 3 | **License Status modal redesign + title-bar account chip in Sprint 7 or 8.** Backlog item already filed under Medium Priority. Every data hook the redesign needs (`license_key`, `lastSyncedAt_`, `RequestRefresh()`) is plumbed in this PR — pure rendering work, ~325 LoC. Pairs naturally with Sprint 7's feature-gating UI work since both touch the licensing surface. | Casey | Sprint 7 or 8 | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 5) | Status |
|---|---------------------------|--------|
| 1 | Stage Stepper + Login Screen smoke_ui scenarios — Sprint 6 close | 🔄 **Partial** — Login Screen portion shipped via Story 6.7 (commit `09c566a`). The `login_screen_cycle` scenario exercises Sign In → Cancel hermetically via `SetTestState` test-mode bypass. Stage Stepper portion deferred to Sprint 7 alongside Story 7.x stage-progression integration; rationale: needs project-state setup that pairs naturally with the feature-gating work. The deeper Stage 3 process+Accept and Stage 4 brush+sculpt+undo scenarios from the original Story 5.4 spec are also still pending. |
| 2 | Cottage-cheese-face root-cause spike — Sprint 6 close | ❌ **Deferred at sprint planning** — Sprint 6's licensing scope (26 pts) plus the spike (~5 pts) would have stacked to 31 in a 28-pt capacity. Deliberate carryover; rationale documented in Sprint 6 plan's "Deferred & Out of Scope" section. The Story 5.2 diagnostic dump scaffolding (`METAVISAGE_DUMP_RESIDUALS=1`) and three audit-flagged hypothesis sites are still primed. Action Item #2 above commits to closing this in Sprint 7 OR making an explicit "stopgap is the answer" decision. |

One previous AI completed (partial — Login Screen portion), one deferred. The cottage-cheese spike has now been carried for two consecutive retros (Sprint 5 → Sprint 6 → Sprint 7); Action Item #2 explicitly raises the stakes.

---

## Key Metrics

- **Velocity Trend**: stable. 22/22 (Sprint 5) vs 26/26 (Sprint 6). Three consecutive 100%-on-commitment sprints (4 if you count Sprint 3.7's 26/26). Commitment-to-completion ratio is 100% in 5 of the last 7 sprints; the two below-100% sprints (3.5, 4) both deferred a single specific story (`imgui_test_engine` smoke lane in both cases) with the rest landing.
- **Team Morale**: high. Three pieces of UI/UX feedback all addressed within hours; the iterative loop with the user worked smoothly even when round-1 fixes were partial. The state-machine refactor for `Verifying` showed the auth code had good naming hygiene to support a late-stage insertion. Web team Q&A round-trip set a clean architectural foundation. PR #25 is in solid shape with the blocking review finding closed and a clear scope narrative.
- **Process Health**: improving. Story 5.5's gate fired naturally during this retro flow (the pre-flight check passed without intervention because Status was flipped to `Complete` and the DoD items were ticked with explicit qualifiers where genuine partials existed). Mechanical guards ARE working — Story 4.7 + Story 5.5 together caught zero anti-patterns this sprint, which is the goal. **Watching**: whether the cottage-cheese spike actually closes in Sprint 7 will indicate whether the deferral pattern is healthy (deliberate sprint scoping) or unhealthy (perpetual carryover). Action Item #2's explicit "close OR mini-sprint OR stopgap-is-the-answer" framing is the gate.
