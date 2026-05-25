# Sprint 7: Licensing Infrastructure — Feature Gating & Conversion Tracking

**Sprint Goal**: Enforce all tier-based feature flags throughout the application, ship conversion reporting (with an offline queue so transient network failure never blocks a user-completed export), surface inline upgrade prompts at every gated surface, and warn users approaching their monthly conversion limit. Then close the cottage-cheese-face root-cause spike that has now carried Sprint 5 → Sprint 6 → Sprint 7 — landing the fix or making the explicit "Outcome B stopgap is the answer" decision called out in Sprint 6 retro Action Item #2. End the sprint with the two carryover `smoke_ui` scenarios from Sprint 5 retro AI #1 finally fully closed.
**Duration**: 2026-05-05 — 2026-05-18 (2 weeks, 10 working days)
**Capacity**: 28 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor — same baseline as Sprints 4/5/6; the cottage-cheese spike's open-ended profile is the reason for keeping the buffer at 3 pts rather than committing the full 28)
**Committed**: 25 points

> Scope is set by [SPRINT_PLAN.md § Sprint 7](./SPRINT_PLAN.md#sprint-7-licensing-infrastructure--feature-gating--conversion-tracking) (Stories 7.1–7.4) plus carryover Action Item #2 from [retro-sprint-06.md](../retrospectives/retro-sprint-06.md) (cottage-cheese root-cause spike — Sprint 5 retro AI #2 → Sprint 6 deferred → Sprint 7 hard close) and the partial portion of Sprint 5 retro AI #1 still open (Stage Stepper `smoke_ui` scenario; the Login Screen portion shipped in Sprint 6 Story 6.7).
>
> **Branch**: `sprint/Licensing-Infrastructure-Feature-Gating-&-Conversion-Tracking` (already cut from `development` post Sprint 6 close).
>
> **Standing on Sprint 6's plumbing**: every data hook the Sprint 7 stories need is already wired. `LicenseManager` exposes `GetTier()` / `GetConversionsUsed()` / `GetConversionsLimit()` / `GetConversionsResetAt()` / `GetLicenseKey()` / `GetLastSyncedAt()` / `RequestRefresh()`. The exchange-worker pattern (`ExchangeResult` struct + `exchangeThread_` + `std::atomic<bool>` finished flag + `DrainPendingExchange()` on the main thread) is the template Story 7.2's `ReportConversion()` mirrors. The amber `Offline` pill in `RenderStatusBar` is the visual template Stories 7.3 and 7.4 reuse for inline upgrade prompts and limit warnings. **Sprint 7 adds new behaviour on top of an already-shaped surface — no foundational refactors expected.**
>
> **Sprint 6 retro AI #3 (License Status modal redesign + title-bar account chip)** is deliberately deferred to Sprint 8 — see "Deferred & Out of Scope" below for the rationale. Story 7.4 still touches the existing modal to add the conversion-usage row, but the full ~325 LoC redesign pairs better with Sprint 8's installer/release surface than with Sprint 7's functional gating.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 7.1: Feature Flag Enforcement** | 8 | None — foundational getters the rest of the sprint reads from | [ ] |
| 2 | **Story 7.2: Conversion Tracking** | 5 | Story 7.1 (`GetConversionsLimit()` for the limit-reached path) | [ ] |
| 3 | **Story 7.3: Upgrade Prompts** | 3 | Story 7.1 (gate sites surface upgrade prompts) | [ ] |
| 4 | **Story 7.4: Conversion Limit Warning** | 2 | Story 7.2 (counter increment after a successful report) | [ ] |
| 5 | **Story 7.5: Cottage-Cheese-Face Root-Cause Spike** *(Sprint 5 retro AI #2 → Sprint 6 → Sprint 7 hard close)* | 5 | None — orthogonal to licensing surface | [ ] |
| 6 | **Story 7.6: Stage Stepper smoke_ui Scenario** *(Sprint 5 retro AI #1, partial carryover — Stage Stepper portion only)* | 2 | Story 7.1 (`SetTestFeatureFlags` test-mode hook makes Free/Indie/Pro fixtures hermetic) | [ ] |

> **Critical-path**: 7.1 → (7.2, 7.3) is the load-bearing chain. 7.1 lands the typed getters and the JWT `feature_flags` parser; the gating UI in 7.1 itself uses 7.3's `UpgradePromptWidget`, so 7.3 is best done as a parallel track that 7.1 calls into rather than sequenced after. Aim for 7.1 + 7.3 by Day 5 so 7.2 + 7.4 (the conversion path) lands in the second week. 7.5 is independent and can run in parallel; 7.6 lands once 7.1's `SetTestFeatureFlags` exists.

---

## Story Detail

### Story 7.1: Feature Flag Enforcement (8 pts)

The biggest story by surface area. Two phases: **(a) parse `feature_flags` out of the license JWT** (Sprint 6 left this on the cutting-room floor — `LoadClaimsFromJwt` at [src/licensing/LicenseManager.cpp:557](../../../src/licensing/LicenseManager.cpp#L557) extracts `tier` / `license_key` / `conversions_*` but does not touch the `feature_flags` object), and **(b) wire seven distinct UI gates** across Stages 1, 3, and 4 against those flags.

**Phase (a) — `LicenseManager` feature-flag surface:**

- New struct `MetaVisage::licensing::FeatureFlags` in [include/licensing/LicenseManager.h](../../../include/licensing/LicenseManager.h):
  ```cpp
  struct FeatureFlags {
      int conversionsLimit = 0;             // mirrors GetConversionsLimit() — single source of truth
      bool customHeadUpload = false;
      enum class TextureTransfer { None, Albedo, AlbedoNormal };
      TextureTransfer textureTransfer = TextureTransfer::None;
      enum class MorphingParameters { Presets, Full };
      MorphingParameters morphingParameters = MorphingParameters::Presets;
      bool masking = false;
      enum class TouchUpToolset { BasicSmooth, Basic, Full };
      TouchUpToolset touchUpToolset = TouchUpToolset::BasicSmooth;
      bool batchProcessing = false;         // Studio tier reservation; never true today
  };
  ```
- Typed getters on `LicenseManager` (alongside the existing tier/conversion getters at [LicenseManager.h:100-105](../../../include/licensing/LicenseManager.h#L100)):
  ```cpp
  const FeatureFlags& GetFeatureFlags() const noexcept;
  bool CanUploadCustomHead() const noexcept;
  FeatureFlags::TextureTransfer GetTextureTransferMode() const noexcept;
  FeatureFlags::MorphingParameters GetMorphingParametersMode() const noexcept;
  bool CanUseMasking() const noexcept;
  FeatureFlags::TouchUpToolset GetTouchUpToolset() const noexcept;
  ```
- Extend `LoadClaimsFromJwt` at [LicenseManager.cpp:557-589](../../../src/licensing/LicenseManager.cpp#L557) to parse the `feature_flags` JSON object out of the JWT payload using `jwt-cpp`'s `nlohmann_json` traits (already wired). Map string enum values (`"none"` / `"albedo"` / `"albedo_normal"`, `"presets"` / `"full"`, `"basic_smooth"` / `"basic"` / `"full"`) to the enum values above. **Defaulting policy**: if any field is missing from the JWT (server-side bug, tier downgrade racing the client, etc.), fall back to the most restrictive value for that field (Free-tier shape) — *never* infer permissions from absence.
- Symmetry walk (per Sprint 6 retro lessons): every `Initialize` / `LoadClaimsFromJwt` / `SignOut` / `ClearAuthState` path that touches `tier_` / `licenseKey_` must also reset `featureFlags_` to the restrictive default. Audit with grep before merging.
- Test-mode hook `LicenseManager::SetTestFeatureFlags(const FeatureFlags&)` — gated on the existing `testMode_` flag from Sprint 6 Story 6.7 (`SetTestState`). Lets `smoke_ui` and Story 7.6 inject a Free / Indie / Pro fixture without standing up a real JWT signer.

**Phase (b) — UI gating sites** (gate ordering follows the master-plan story tasks at [SPRINT_PLAN.md:788-799](./SPRINT_PLAN.md#L788)):

| Tier flag | Behaviour | Code site (per architect audit) |
|---|---|---|
| `custom_head_upload = false` (Free) | **Source-mesh slot only** — the "Load Morph Mesh" file slot in Stage 1 wrapped in `ImGui::BeginDisabled` with an `UpgradePromptWidget` callout. Live-test correction: the gate covers ONLY the custom MetaHuman head upload (source slot), NOT the target mesh — Free users still need to upload a target head to morph the Default MetaHuman against. | `StageSidebar::RenderAlignment` at [StageSidebar.cpp:159](../../../src/ui/StageSidebar.cpp#L159); `out.importMorphMeshRequested` action site only. The `Use Default MetaHuman Head` button stays enabled for every tier so Free users have a starting point. |
| `texture_transfer = none` (Free) | Hide the entire "TARGET TEXTURES" section in Stage 1 + show a single `UpgradePromptWidget` "Texture transfer requires Indie or higher" in its place | `StageSidebar.cpp:222-253` (the `kSlotNames[]` combo + "+ Texture" button block) |
| `texture_transfer = albedo` (Indie) | Allow Albedo upload; hide non-Albedo options in the slot combo and disable the "+ Texture" button after the first slot is filled, with an inline upgrade prompt for additional slots | same block (`kSlotNames[]` combo filtering — restrict the iterated `kSlotNames` to `{ "Albedo" }`) |
| `texture_transfer = albedo_normal` (Pro) | Existing behaviour (all slots) | unchanged — default path |
| `morphing_parameters = presets` (Free) | The General + Advanced collapsible parameter sections in Stage 3 wrapped in `ImGui::BeginDisabled`; only preset-card click is functional. Inline `UpgradePromptWidget` placed above the General section header. | General collapsible at [StageSidebar.cpp:1140](../../../src/ui/StageSidebar.cpp#L1140), Advanced at line 1164. Preset cards at lines 995-1005 (built-ins) and ~1013 (user) stay enabled. The `paramEdited` → `activePresetId_.clear()` seam at line 1223 is unchanged — disabled inputs never edit. |
| `morphing_parameters = full` (Indie/Pro) | Existing behaviour | unchanged |
| `masking = false` (Free) | Stage 3 vertex masking section wrapped in `ImGui::BeginDisabled` with an `UpgradePromptWidget` over the "Paint mask" / "Show mask" / radius / Clear / Invert controls | `StageSidebar.cpp:1058-1110` (the `DrawSectionLabel("VERTEX MASK")` block) |
| `touchup_tools = basic_smooth` (Free) | Only Smooth brush enabled; Grab / Push-Pull / Inflate disabled in **both** the inspector cards (2x2 `BeginTable` at [StageSidebar.cpp:1338-1354](../../../src/ui/StageSidebar.cpp#L1338)) **and** the ToolRail (`kStage4Tools[]` at [ToolRail.cpp:42](../../../src/ui/ToolRail.cpp#L42), render loop ~lines 224-248). Hover-tooltip upgrade prompt on disabled brushes. | both surfaces — must be gated together; `viewport->SetBrushType(kind)` at ToolRail line 248 must early-out for disabled brushes |
| `touchup_tools = basic` (Indie) | Smooth + Grab enabled; Push-Pull + Inflate disabled with upgrade prompts | both surfaces, same approach |
| `touchup_tools = full` (Pro) | Existing behaviour | unchanged |

Each gate uses the same pattern: `ImGui::BeginDisabled(!licenseManager.GetX())` around the input, with an `UpgradePromptWidget` callout (Story 7.3) keyed to the locked feature. Hide-vs-disable is a per-gate decision documented in the table above.

**Out of scope this story**: the actual `UpgradePromptWidget` rendering primitive (Story 7.3); status-bar tier-aware messaging (Story 7.4); conversion-counter writes (Story 7.2).

### Story 7.2: Conversion Tracking (5 pts)

`POST /api/license/conversion` after every successful export. Conversion counting deliberately happens at export time, not Accept Result, so users can reprocess and compare morph settings freely before committing a result.

**Endpoint contract** (per [DESKTOP_APP_INTEGRATION.md § Report Conversion](../DESKTOP_APP_INTEGRATION.md#2-report-conversion--post-apilicenseconversion)):
- `POST {METAVISAGE_API_BASE_URL}/api/license/conversion`, `Authorization: Bearer <access_token>`, no body.
- 200: response body `{ conversions_used, conversions_limit, conversions_reset_at, limit_reached }` — update `LicenseManager::conversionsUsed_` from this, not from a local increment.
- 403 `limit_reached`: response carries the same usage/reset fields. Show the limit dialog, clear queued reports, and rely on the export pre-check to block future export attempts once effective usage is at the cap.
- 429: respect `Retry-After` (integer seconds delta per QA Round 2 — `parseInt` is sufficient).
- 5xx: exponential backoff (1s, 2s, 4s, max 30s), 3 retries.

**Worker pattern** (mirror the exchange worker at [LicenseManager.h:207-220](../../../include/licensing/LicenseManager.h#L207)):
- New struct `ConversionResult { int httpStatus = 0; int conversionsUsed = 0; int conversionsLimit = 0; std::string conversionsResetAt; bool limitReached = false; std::string errorMessage; }`.
- `LicenseManager::ReportConversion()` spawns a fire-and-forget `std::thread` that runs the POST + retry loop; result lands in a mutex-guarded `pendingConversionResult_`. `DrainPendingConversion()` runs each frame on the main thread inside `AppWindow::TickOnce`, dispatches outcomes (200 → silent counter update, 403 → open block modal, 429/5xx exhausted → enqueue offline).
- Retry budget constants alongside the existing exchange budget at [LicenseManager.h:170-171](../../../include/licensing/LicenseManager.h#L170): `kConversionMaxAttempts = 4`, `kConversionBudgetSeconds = 30` — match exchange path.

**Trigger site** ([AppWindow.cpp](../../../src/ui/AppWindow.cpp)):
- Export is the conversion boundary. The export action path performs a local/effective usage pre-check before `DoExport()`, and `DoExport()` calls `LicenseManager::ReportConversion()` only after `MeshExporter::Export(...)` succeeds. `AcceptMorph()` remains a pure review-state flip so users can try multiple morph settings without burning conversions.

**Offline queue** (the core of this story):
- New `pendingConversionQueue_` — `std::deque<std::chrono::system_clock::time_point>` of timestamps for un-reported conversions. Persisted to `%APPDATA%\MetaVisage\conversion_queue.json` via `nlohmann::ordered_json` (atomic write through `<file>.tmp` + `MoveFileExW`, same pattern as Sprint 6's session caches but no encryption — these are timestamps, not credentials).
- On `ReportConversion()` failure (transport-level after retry exhaustion *or* the offline branch is already known via `IsOfflineMode()`), append the timestamp to the queue and persist. **The user is unblocked immediately** — completing an export never depends on the network.
- A new `LicenseManager::FlushConversionQueue()` runs after every successful `RefreshLicenseJWT` (the token refresh thread is the natural retry trigger — already wakes every 24h or via `NotifyRefreshNow`). Drain the queue oldest-first; one POST per queued entry. On success, pop. On 403 `limit_reached` mid-queue: drop the remaining queue (the user is over-limit; no point forcing failed POSTs). On any other error: stop, leave the queue intact, retry on next refresh.
- **Cap the queue at 100 entries** to bound disk usage during prolonged offline sessions. Drop oldest beyond the cap with a single `MV_LOG_WARN`.

**`limit_reached` blocking dialog** (Free-tier 4th export of the month):
- Reuse the existing `Modals::OpenConfirm` infrastructure with custom title "Monthly conversion limit reached", body listing the limit (`conversions_limit`), the reset date (`conversions_reset_at` formatted as a readable date string), and an "Upgrade at metavisage.io" link (left-aligned, `OpenUrl(METAVISAGE_API_BASE_URL "/pricing")`) plus a "Close" button. **Important**: the local pre-check blocks export when effective usage is already at the cap; users can still tweak parameters, re-run, and compare results, they just can't write another export until reset or upgrade.

**Out of scope this story**: status-bar warning at `limit - 2` (Story 7.4); License Status modal usage row update (Story 7.4); the `Help > Account & License` modal redesign (Sprint 8).

### Story 7.3: Upgrade Prompts (3 pts)

A reusable `UpgradePromptWidget` for every gate site identified in Story 7.1.

**Visual model** (closest existing template per audit: amber `Offline` pill at [AppWindow.cpp:1650-1653](../../../src/ui/AppWindow.cpp#L1650) + the Story 5.2 amber inline warning around [StageSidebar.cpp:1210](../../../src/ui/StageSidebar.cpp#L1210)):
- Pill-shaped inline panel, amber palette tokens (`MvStyle::Colors::MvAmber*` — never raw `IM_COL32` per AGENTS.md design-system rules), `MvIcons::Lock` glyph at the leading edge.
- Single line of body copy: `"<Feature> requires <Tier>"` — e.g., `"Vertex masking requires Indie"`.
- Trailing right-aligned text link: `"Upgrade at metavisage.io →"` (uses `MvStyle::Colors::MvBlue500` link styling from existing `Modals::RenderLicenseModal` patterns).
- Click on the link calls `MetaVisage::OpenUrl::Open(METAVISAGE_API_BASE_URL "/pricing")` — *not* `/manage-subscription` (that's the post-purchase Stripe portal); the upgrade target is the public pricing page.

**API surface** (new file `include/ui/UpgradePromptWidget.h`):
```cpp
namespace MetaVisage::ui {
struct UpgradePromptCopy {
    std::string_view feature;     // "Vertex masking", "Custom mesh upload", etc.
    std::string_view minimumTier; // "Indie", "Pro"
};
void DrawUpgradePrompt(const UpgradePromptCopy& copy);
void DrawUpgradePromptTooltip(const UpgradePromptCopy& copy);  // for disabled-button hover
}
```

`DrawUpgradePrompt` is the inline pill (Stages 1/3 sidebar gates). `DrawUpgradePromptTooltip` is the hover variant for disabled controls (Stage 4 brush buttons in the inspector cards + ToolRail) — uses `ImGui::SetItemTooltip` (ImGui 1.89+, already in use). Both share the same copy struct so re-keying a gate's tier requirement is a one-line change.

**Hover-on-disabled gotcha**: `ImGui::BeginDisabled` blocks `IsItemHovered()` by default. Use `ImGui::SetItemTooltip(...)` *outside* the `BeginDisabled` scope or pass `ImGuiHoveredFlags_AllowWhenDisabled` — verify both work via the smoke harness.

**Where it gets called**: every Story 7.1 gate site lists its prompt copy explicitly. Centralize the strings in a `kUpgradePromptCopy` table inside `UpgradePromptWidget.cpp` so a future tier-rename (Indie → Studio, etc.) is a single-file edit.

**Out of scope this story**: status-bar conversion-limit warning (Story 7.4 — different visual surface); the License Status modal (untouched here).

### Story 7.4: Conversion Limit Warning (2 pts)

Two surfaces touched: the status bar (new "X conversions remaining" indicator) and the License Status modal (existing usage display gets a refresh hook).

**Status-bar warning** ([AppWindow::RenderStatusBar](../../../src/ui/AppWindow.cpp#L1599) inserts after the Tier block at ~line 1703 per the architect audit):
- Trigger condition: `LicenseManager::GetConversionsUsed() >= LicenseManager::GetConversionsLimit() - 2` AND `GetConversionsLimit() > 0` (defensive — never trigger on a torn read where limit is still 0).
- Render an amber-pill section using the same `drawSection()` / `drawSep()` lambdas already defined in `RenderStatusBar`. Body copy: `"<remaining> conversions left this month"` when 1 ≤ remaining ≤ 2. When remaining == 0: `"0 conversions left — resets <date>"` plus a subtle right-aligned link `"Upgrade →"` (same `MvBlue500` link style as Story 7.3, calls `OpenUrl(METAVISAGE_API_BASE_URL "/pricing")`).
- **Pro tier note**: 60-conversion limit means "within 2" triggers far more often than for Free or Indie. Don't suppress on Pro — the warning is correct behaviour for any tier nearing its cap; users can ignore.

**License Status modal usage row** ([Modals.cpp:1037-1038](../../../src/ui/Modals.cpp#L1037)):
- Sprint 6 already renders `<used> / <limit>`. Add a one-line "Resets <date>" beneath using the same `ImGui::TextColored` muted-grey treatment used elsewhere in the modal. Use `MvStyle::Colors::MvSlate400` for the muted line.
- No structural changes — the redesign per Sprint 6 retro AI #3 is Sprint 8 territory.

**Out of scope this story**: full License Status modal redesign + title-bar account chip (Sprint 6 retro AI #3 → Sprint 8).

### Story 7.5: Cottage-Cheese-Face Root-Cause Spike (5 pts) *(Sprint 5 retro AI #2 — three-sprint hard close)*

Carryover from Sprint 5 retro AI #2 → Sprint 6 deferred deliberately → Sprint 7. The Sprint 5 Story 5.2 stopgap (amber UI warning when `LandmarkWeight / StiffnessFinal > 0.5` plus the env-gated diagnostic dump at `%TEMP%\metavisage_residuals_<epochMs>_step<N>.csv` via `METAVISAGE_DUMP_RESIDUALS=1`) is in place. **This story closes the spike — either by shipping a fix or by making the explicit "stopgap is the answer, retire the warning threshold" decision** per the Sprint 6 retro AI #2 framing. Three-sprint deferral is the anti-pattern Sprint 4's retro originally flagged.

**Investigation plan** (each step has an output that gates the next):
1. **Reproduce reliably.** Use the bundled MetaHuman head + a target mesh with dense landmark clusters (eyes/lips area). Confirm the artifact appears with default Quick Wrap + landmark weight bumped above 0.5 — capture a frozen FBX and a screenshot for the regression record. Output: a `tests/fixtures/cottage_cheese_repro.mmproj` if the repro is stable enough to pin.
2. **Inspect residual dumps.** Run with `METAVISAGE_DUMP_RESIDUALS=1` set. Open the per-step CSVs in a spreadsheet; sort by per-vertex residual descending. **Hypothesis correlation**: are the high-residual vertices clustered around landmarks, around UV seams, or scattered? (Each cluster shape points to a different one of the three audit-flagged hypothesis sites below.)
3. **Walk the three audit-flagged hypothesis sites in order**:
   - **Landmark-weight floor relative to alpha** at [src/deformation/NRICPDeformer.cpp:764-790](../../../src/deformation/NRICPDeformer.cpp#L764). Hypothesis: when `alpha` (stiffness) decays toward `nricpAlphaFinal`, the relative weight of landmark constraints climbs unboundedly. Test: log the effective `landmarkWeight / alpha` ratio per step; correlate against residual peaks in the dumps.
   - **UV-seam BFS over-exclusion** at [NRICPDeformer.cpp:198-286](../../../src/deformation/NRICPDeformer.cpp#L198). Hypothesis: the seam-detection BFS over-excludes interior vertices, which then drift independently. Test: dump the seam-vertex set, visualize against the cottage-cheese region.
   - **Excluded-vertex landmark contribution mismatch** at [NRICPDeformer.cpp:350, 387](../../../src/deformation/NRICPDeformer.cpp#L350). Hypothesis: a landmark whose closest morph-mesh vertex is in the excluded seam set still contributes to the system, but its row indexing is off-by-one. Test: trace the indexing logic for a landmark known to be near a seam.
4. **Ship the fix** — *or* close the spike with the explicit "Outcome B is the answer" decision. If the latter: file a backlog item to retire the inspector warning threshold once we have user data on the parameter range that actually misfires (the warning can become noise above value X), and explicitly close Sprint 5 retro AI #2 as "investigated, decided stopgap is correct" with the residual-dump evidence in the close-out commit.
5. **Regression test.** If a fix lands: add a `smoke_wrap_parity` case using the repro fixture from step 1. The expected output is the post-fix mesh; the smoke compares vertex-position deltas against a tolerance. If "stopgap is the answer": no regression test needed.

**Time budget**: 5 pts ≈ 3 working days. **Decision gate at Day 2 of Story 7.5**: if step 2's residual analysis hasn't pointed at one of the three hypotheses by then, this becomes a stopgap-is-the-answer close — don't sink the rest of the sprint chasing a fourth hypothesis. The retro framing was explicit about this.

**Out of scope this story**: parameter-tuning UX (the inspector warning threshold tweak is a separate ~1 LoC future change keyed to the decision in step 4); broader algorithmic redesigns (different deformation algorithm, GPU acceleration — post-v2.0).

### Story 7.6: Stage Stepper smoke_ui Scenario (2 pts) *(Sprint 5 retro AI #1, partial carryover)*

The Sprint 5 retro AI #1 split cleanly: Login Screen portion shipped in Sprint 6 Story 6.7; Stage Stepper portion deferred here because it needs project-state setup that Sprint 7's feature-gating work primes (the `SetTestFeatureFlags` hook from Story 7.1 unlocks hermetic Free / Indie / Pro test fixtures).

- Add `stage_stepper_progression` scenario to [tests/smoke/ui_smoke.cpp](../../../tests/smoke/ui_smoke.cpp). Pattern: same `IM_REGISTER_TEST` shape as the existing smoke scenarios; null-guard `g_app` per the local convention; register in `main()` alongside the other scenarios.
- Walk: pre-condition `LicenseManager::SetTestState(Authenticated)` + `SetTestFeatureFlags(<Pro fixture>)`; load a fixture project (reuse `tests/fixtures/v1.0.0_*` if one already covers a full Stage 1→4 state, otherwise build a minimal stub). Step through the four `StageStepper` pills (Alignment → Point Reference → Morph → Touch Up); assert each transition is reflected in `Project::GetCurrentStage()` and that the corresponding sidebar renders.
- **Hermetic constraints**: no real network (test mode flag from Sprint 6 already suppresses browser launch + HKCU writes). No real morph compute — Stage 3 progression is asserted by a UI-state check, not by waiting for the worker thread.
- Add to existing `smoke_ui` CTest target with `LABELS ui`. Runtime budget: under 1 second. Total `smoke_ui` runtime budget after this addition and the Sprint 7 brush-gate clamp hardening scenario: under 5 seconds.
- **Out of scope**: Stage 3 process+Accept and Stage 4 brush+sculpt+undo scenarios from the original Sprint 5.4 spec — those need real worker-thread + sculpt-mesh state which lands cleanly post-v2.0. The Stage Stepper progression scenario is the carryover the retros tracked.

---

## Sprint Context (Key Affected Files)

**Story 7.1 (Feature Flag Enforcement)**
- [include/licensing/LicenseManager.h](../../../include/licensing/LicenseManager.h) — new `FeatureFlags` struct + 6 typed getters + `featureFlags_` member + `SetTestFeatureFlags(...)` test-mode hook
- [src/licensing/LicenseManager.cpp:557-589](../../../src/licensing/LicenseManager.cpp#L557) — extend `LoadClaimsFromJwt` with `feature_flags` parsing; symmetry-walk all `tier_` reset paths
- [src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp) — gate sites at lines 159 (file-load buttons), 222-253 (textures), 1058-1110 (vertex mask), 1140 (General morph params), 1164 (Advanced morph params), 1338-1354 (Stage 4 brush cards)
- [src/ui/ToolRail.cpp:42, 224-248](../../../src/ui/ToolRail.cpp#L42) — `kStage4Tools[]` + Stage 4 ToolRail render loop brush gating

**Story 7.2 (Conversion Tracking)**
- [include/licensing/LicenseManager.h](../../../include/licensing/LicenseManager.h) — `ConversionResult` struct + `ReportConversion()` + `FlushConversionQueue()` + retry budget constants
- [src/licensing/LicenseManager.cpp](../../../src/licensing/LicenseManager.cpp) — worker thread + `DrainPendingConversion()` + offline-queue persistence
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — export pre-check uses effective usage; `DoExport()` calls `ReportConversion()` after successful mesh export
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) (`TickOnce`) — drain hook for `pendingConversionResult_` before any UI render
- [src/ui/Modals.cpp](../../../src/ui/Modals.cpp) — new "Monthly conversion limit reached" confirm-modal payload
- New: `%APPDATA%\MetaVisage\conversion_queue.json` (persisted offline queue; created on first offline export)

**Story 7.3 (Upgrade Prompts)**
- New: `include/ui/UpgradePromptWidget.h` / `src/ui/UpgradePromptWidget.cpp` (~80 LoC — `DrawUpgradePrompt` + `DrawUpgradePromptTooltip` + the centralized `kUpgradePromptCopy` table)
- All Story 7.1 gate sites — add `DrawUpgradePrompt(...)` / `DrawUpgradePromptTooltip(...)` calls inline

**Story 7.4 (Conversion Limit Warning)**
- [src/ui/AppWindow.cpp:1599-1724](../../../src/ui/AppWindow.cpp#L1599) (`RenderStatusBar`) — new amber pill after the Tier block at ~line 1703
- [src/ui/Modals.cpp:1037-1038](../../../src/ui/Modals.cpp#L1037) — add "Resets <date>" line beneath the existing `<used> / <limit>` text

**Story 7.5 (Cottage-Cheese-Face Root-Cause Spike)**
- [src/deformation/NRICPDeformer.cpp:198-286, 350, 387, 764-790](../../../src/deformation/NRICPDeformer.cpp#L198) — three audit-flagged hypothesis sites (one or more touched depending on the fix path)
- New (conditional): `tests/fixtures/cottage_cheese_repro.mmproj` if the repro pins reliably
- New (conditional): `tests/smoke/wrap_parity_cottage_cheese.cpp` regression case (only if a fix lands; uses existing `smoke_wrap_parity` infrastructure)

**Story 7.6 (Stage Stepper smoke_ui)**
- [tests/smoke/ui_smoke.cpp](../../../tests/smoke/ui_smoke.cpp) — new `stage_stepper_progression` scenario + registration in `main()`
- [include/licensing/LicenseManager.h](../../../include/licensing/LicenseManager.h) — already-added (Story 7.1) `SetTestFeatureFlags(...)` is the hermetic-fixture entry point

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Cottage-cheese-face spike (Story 7.5) is open-ended** — three sprints of carryover suggests the hypotheses already on the table may not converge to a fix in 5 pts. | **High** | **Med** | The Day-2 decision gate inside Story 7.5 is the explicit relief valve: if residual analysis hasn't pointed at one of the three audit-flagged sites by Day 2, close the spike with "stopgap is the answer" and the dump-evidence commit. The retro framing (three-sprint anti-pattern) is the forcing function. **Worst case: 5 pts spent on investigation that retires the warning rather than a code fix — still closes the carryover and frees future sprints.** |
| **`POST /api/license/conversion` may not be live on staging** (per Sprint 6 retro Open Item #1, staging endpoints are still unverified). | **Med** | **Med** | Story 7.2 already runs against the **development profile + localhost portal** like Sprint 6 did. The endpoint contract is fully specified in [DESKTOP_APP_INTEGRATION.md § Report Conversion](../DESKTOP_APP_INTEGRATION.md#2-report-conversion--post-apilicenseconversion). If localhost portal doesn't expose this route, surface the gap to the web team Day 1; in the meantime, the offline-queue path is the natural fallback for any 5xx — code can land and ship correctly even before the endpoint goes hot. |
| **`feature_flags` claim shape surprises** — the JWT spec carries 7 fields but the actual Sprint 6 token might omit some. | **Low** | **Med** | The defaulting policy ("most-restrictive on absence") in Story 7.1 is the safety net — a Free user with a bug-shaped JWT gets Free behaviour, never an accidental upgrade. Add a one-time `MV_LOG_WARN("feature_flags missing field: <name>")` so the gap is debuggable. Day 1 of Story 7.1: decode a fresh production-shape JWT and verify field-by-field. |
| **`ImGui::BeginDisabled` + tooltip interaction quirks** — disabled controls block hover tooltips by default. Story 7.3's tooltip variant could silently no-op on the disabled brush buttons. | **Med** | **Low** | `ImGuiHoveredFlags_AllowWhenDisabled` is the documented escape; verify in the smoke harness explicitly. The Story 5.2 amber inline warning already lives next to a disabled control and works — that's the proven template. |
| **Status bar real estate** — Sprint 6 added the Tier chip and Offline pill; Sprint 7 adds the conversion-limit warning. Three pills + GPU on the right risks horizontal overflow on small windows. | **Med** | **Low** | The status bar already supports adaptive layout via the `drawSection()` lambda — measure cumulative width at end of paint and elide non-critical sections (GPU first, then Source/Target paths). Plumb a min-window-width assertion into the smoke harness. |
| **Offline queue persistence** — `conversion_queue.json` could get corrupted (power loss mid-write, disk full). | **Low** | **Low** | Atomic write through `<file>.tmp` + `MoveFileExW` (same pattern as Sprint 6 session caches) prevents half-written files. On parse failure: log a warning, drop the queue, continue — losing offline conversions is preferable to crashing on launch. **Bound the queue at 100 entries** to keep recovery cheap. |
| **Cumulative scope: 25 points in 28-pt capacity**, with one open-ended spike (7.5) and the conversion endpoint dependency on staging. | **Low** | **Med** | 3-pt buffer is the same shape as Sprint 6's 26/28. **Discretionary cuts**: Story 7.6 (Stage Stepper smoke_ui) is the first cut if the critical path slips — same role Story 6.7 played in Sprint 6. Story 7.4 (limit warning) is cut #2 — it's a 2-pt polish layer on top of 7.2's actual conversion path; the License Status modal already shows usage, so users have a route to the data even without the status-bar pill. **Don't cut 7.5** — that's the whole reason the retro AI exists; cutting it forces a fourth-sprint deferral that the retro explicitly closes off. |

---

## Definition of Done

- [ ] All committed stories implemented and merged to the sprint branch (`sprint/Licensing-Infrastructure-Feature-Gating-&-Conversion-Tracking`)
- [ ] Build clean: `cmake --build . --config Release` succeeds with no new warnings (the three pre-existing C5054 warnings in the dockspace flag-combine code carry over from Sprint 3.6 unchanged — out of scope here, same as Sprints 3.8 / 4 / 5 / 6)
- [ ] CTest green: `ctest -C Release` passes the existing smoke lanes (`smoke_load_serialize_export`, `smoke_v1_backward_compat`, `smoke_wrap_parity`, and `smoke_ui`) with the six current UI scenarios, including `stage_stepper_progression` and `touchup_brush_gate_clamp` — and, conditional on Story 7.5 landing a fix, the new `wrap_parity_cottage_cheese` regression case
- [ ] `feature_flags` parsing extracts all 7 fields from a fresh-from-portal JWT; missing fields default to Free-tier-restrictive shape with a one-time `MV_LOG_WARN` per missing field
- [ ] **Free tier** (verified via `SetTestFeatureFlags` fixture): cannot load a custom source MetaHuman head but can still load a target head; texture upload section hidden with upgrade prompt; General + Advanced morphing parameter sections disabled with upgrade prompt above the General header; Vertex Mask section disabled with upgrade prompt; only Smooth brush enabled in Stage 4 (inspector cards + ToolRail, hotkeys included)
- [ ] **Indie tier** (verified via fixture): mesh upload enabled; texture upload restricted to Albedo only with upgrade prompt for additional slots; full morphing parameters; masking enabled; Smooth + Grab brushes enabled, Push-Pull + Inflate disabled with upgrade prompts
- [ ] **Pro tier** (verified via fixture): all features enabled (no regressions from Sprint 6 surface)
- [ ] `LicenseManager::ReportConversion()` posts to `/api/license/conversion` with the access token; counter updates from the response body, not from a local increment
- [ ] Conversion-limit pre-check blocks the export path when effective usage is at the cap; 403 `limit_reached` from the server clears queued reports and shows the limit + reset date + an "Upgrade at metavisage.io" link
- [ ] 429 retries with `Retry-After`; 5xx retries with exponential backoff (1s/2s/4s/30s, 3 retries)
- [ ] Offline export queues the conversion to `%APPDATA%\MetaVisage\conversion_queue.json` and never blocks the user; queue is drained on the next successful refresh or after an in-flight online report completes cleanly
- [ ] Queue cap at 100 entries — overflow drops oldest with a `MV_LOG_WARN`
- [ ] `UpgradePromptWidget` (inline pill + hover tooltip) renders consistently at all 7 gate sites identified in Story 7.1; copy lives in a single `kUpgradePromptCopy` table
- [ ] Status bar shows the "X conversions remaining" amber pill when `used >= limit - 2`; shows "0 conversions left — resets <date>" with an upgrade link at `used == limit`
- [ ] License Status modal shows a "Resets <date>" line beneath the existing `<used> / <limit>` text
- [ ] Cottage-cheese-face spike closed: either a fix landed with a `smoke_wrap_parity` regression case, **or** the explicit "Outcome B stopgap is the answer" decision is documented in the close-out commit body with residual-dump evidence; either outcome closes Sprint 5 retro AI #2 for good
- [ ] `stage_stepper_progression` and `touchup_brush_gate_clamp` scenarios pass under `ctest -L ui`; total `smoke_ui` runtime budget under 5 seconds
- [ ] PR opened against `development` and `$review-pr` agent run; all blocking findings resolved
- [ ] Phase D manual UI smoke — all three tier fixtures walked end-to-end (Stage 1 → 2 → 3 → 4 → Export); offline-pill smoke (Wi-Fi off + successful export + verify queue persistence + Wi-Fi on + verify queue drain); Undo/Redo regression check on point placement / gizmo drag / mask paint / sculpt brush. Done against the **development** profile (staging endpoint deployment is tracked separately per Sprint 6 retro Open Item #1)
- [ ] Sprint outcome notes updated with actual velocity, carryover, retrospective link, and explicit close-outs on Sprint 5 retro AI #1 (Stage Stepper portion) + AI #2 (cottage-cheese spike)

> **Mechanical guard (Sprint 4 Action Item #3 + Sprint 5 Story 5.5):** the
> `$sprint-retrospective` skill refuses to run in two cases — (Clause A) when
> this sprint's `Sprint Outcome` Status is `Complete`/`Implementation Complete`
> and any DoD item containing `"PR opened"`, `"$review-pr"`, `"manual UI smoke"`,
> or `"Phase D"` is still unchecked, *and* (Clause B) when every committed
> backlog story is shipped (`[x]`) but Status is still `Planned`/`In Progress`
> with the same validation DoD items unchecked. Don't flip Status to `Complete`
> until those boxes are genuinely ticked, and don't try to slip past Clause A
> by leaving Status at `Planned`/`In Progress` — Clause B closes that bypass.

---

## Deferred & Out of Scope

- **Sprint 6 retro Action Item #3 — License Status modal redesign + title-bar account chip** (~325 LoC of pure rendering work). Deliberately deferred to Sprint 8. Rationale: Sprint 7 already adds 25 pts of functional gating; the modal redesign is orthogonal rendering polish that pairs better with Sprint 8's installer/release surface than with Sprint 7's gating logic. Story 7.4 still touches `RenderLicenseModal` to add the "Resets <date>" line, but the full redesign waits. Every data hook the redesign needs (`license_key`, `lastSyncedAt_`, `RequestRefresh()`) was plumbed in Sprint 6 and stays plumbed.
- **`smoke_ui` full deep-link round trip with mock HTTP server** (Sprint 6 retro "longed for"). Defer indefinitely until the second Login Screen scenario is wanted; for Sprint 7 the `SetTestFeatureFlags` hook covers the realistic-fixture concern that motivated it.
- **Stage 3 process+Accept and Stage 4 brush+sculpt+undo `smoke_ui` scenarios** (original Sprint 5.4 spec). Defer post-v2.0 — they need real worker-thread + sculpt-mesh state which is genuinely heavy to fixture; the Stage Stepper progression scenario in Story 7.6 is the carryover the retros tracked.
- **`Win32Window` + `utils/Random` consolidation helpers** (Sprint 6 retro "longed for"). Defer post-v2.0 — Sprint 6 added scattered raw Win32 surface across `LicenseManager.cpp`, `SingleInstance.cpp`, `AppWindow.cpp`; centralizing it is a refactor opportunity, not a v2.0 ship blocker.
- **Studio-tier batch processing (`batch_processing` flag = false on all current tiers)**. The flag is parsed and stored in `FeatureFlags` (always false today), but no UI surface is wired since no current tier sets it true. Reserved for the post-v2.0 Studio tier per [DESKTOP_APP_INTEGRATION.md](../DESKTOP_APP_INTEGRATION.md#feature-flags).
- **`/api/license/validate` (the alternative endpoint to `/api/desktop/token`)**. Sprint 6 standardized on `/api/desktop/token` for the 24h refresh; `/api/license/validate` is the equivalent endpoint described in the spec but is not used in the desktop today. No reason to switch in Sprint 7 — both endpoints return the same payload shape per the QA doc.

---

## Sprint Outcome

**Status**: Planned
**Velocity (Actual)**: TBD
**Carryover**: TBD (track Sprint 5 retro AI #1 Stage Stepper portion + AI #2 cottage-cheese spike close-out explicitly in the retro)
**Notes**: TBD (populate at retrospective; reference [retro-sprint-07.md](../retrospectives/retro-sprint-07.md) once written)
