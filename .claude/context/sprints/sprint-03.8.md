# Sprint 3.8: UI/UX Redesign Implementation

**Sprint Goal**: Land the UI/UX team's blueprint-aesthetic redesign across every surface of the application — design tokens, three-font typography stack, window chrome, stage stepper, tool rail, all four stage inspector panels, modals, welcome screen, and login screen — at maximum practical fidelity within Dear ImGui's rendering model.
**Duration**: 2026-04-27 — 2026-05-08 (2 weeks, 10 working days)
**Capacity**: 32 points (solo dev, 10 working days, ~6h focused/day @ 0.7 focus factor — stretched ~25% over the Sprint 3.7 baseline because most per-stage panel stories are assembly work on top of Story 3.8.6's primitives, so reuse compresses real effort)
**Committed**: 32 points

> Scope is fully scoped in [SPRINT_PLAN.md](./SPRINT_PLAN.md) Sprint 3.8 (Stories 3.8.1 – 3.8.12). Design source of truth: [.claude/context/design/version_2/](../design/version_2/) — `colors_and_type.css` for tokens, `app.jsx` for shell + Stage 1, `stage2.jsx` / `stage3.jsx` / `stage4.jsx` for per-stage layouts, `welcome.jsx` and `login.jsx` for full-screen surfaces, `previews/*.png` for the rendered target. The architect-agent audit of the current ImGui shell (recorded below in *Sprint Context*) confirms the redesign touches nearly every render path but concentrates style entry into a single function (`AppWindow::ApplyDarkTheme`) and overlay drawing into one (`Viewport::DrawOverlays`) — both replaceable wholesale.

---

## Sprint Backlog

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 1 | **Story 3.8.1: Design Token System & Font Loading** | 3 | None — load-bearing for every later story | [ ] |
| 2 | **Story 3.8.2: Window Chrome — Title Bar, Menu Bar & Status Bar** | 4 | Story 3.8.1 | [ ] |
| 3 | **Story 3.8.3: Stage Stepper** | 3 | Story 3.8.1 | [ ] |
| 4 | **Story 3.8.4: Left Tool Rail** | 1 | Story 3.8.1 | [ ] |
| 5 | **Story 3.8.5: Viewport Toolbar & HUD Overlays** | 3 | Story 3.8.1 | [ ] |
| 6 | **Story 3.8.6: Inspector Panel — Shared Primitives** | 4 | Story 3.8.1 | [ ] |
| 7 | **Story 3.8.7: Stage 1 Inspector Panel** | 2 | Story 3.8.6 | [ ] |
| 8 | **Story 3.8.8: Stage 2 Inspector Panel & Viewport Bar** | 2 | Story 3.8.6 | [ ] |
| 9 | **Story 3.8.9: Stage 3 Inspector Panel & Viewport Stats** | 3 | Story 3.8.6 | [ ] |
| 10 | **Story 3.8.10: Stage 4 Inspector Panel & Viewport Stats** | 2 | Story 3.8.6 | [ ] |
| 11 | **Story 3.8.11: Welcome Modal Redesign** | 2 | Story 3.8.1 | [ ] |
| 12 | **Story 3.8.12: Login Screen Redesign** | 3 | Story 3.8.1 | [ ] |

> **Critical-path**: 3.8.1 → 3.8.6 unblocks the four panel stories (3.8.7–3.8.10) which can run in parallel after primitives land. 3.8.2 / 3.8.3 / 3.8.4 / 3.8.5 / 3.8.11 / 3.8.12 only depend on 3.8.1 and can be slotted around the panel work.

---

## Story Detail

### Story 3.8.1: Design Token System & Font Loading (3 pts)

The single load-bearing change. Replace the ad-hoc `ApplyDarkTheme()` function (`src/ui/AppWindow.cpp:471–505` — 17 colour slots set with an `rgb()` lambda) with a token system that mirrors `.claude/context/design/version_2/colors_and_type.css` byte-for-byte.

- New `include/ui/MvStyle.h` — every `--mv-*` CSS variable as a named `ImVec4` constant: `MvBg0`–`MvBg4`, `MvBlue300`–`MvBlue900`, `MvFg1`–`MvFg4`, `MvLine1`–`MvLine4`, `MvSuccess`, `MvWarning`, `MvDanger`. Spacing scale (`MvSpacing::S1`–`S6`) in the same header.
- New `MvStyle::Apply()` — sets every `ImGuiCol_*`, `WindowRounding=6`, `FrameRounding=4`, `PopupRounding=8`, `ScrollbarRounding=6`, `GrabRounding=4`, `WindowBorderSize=1`, `FrameBorderSize=1`. Called from `AppWindow::Initialize()` in place of the current `ApplyDarkTheme()` invocation at line 150.
- Bundle Google-Fonts TTFs in `assets/fonts/`: `Saira-ExtraBoldItalic.ttf`, `Inter-Regular.ttf`, `Inter-Medium.ttf`, `Inter-SemiBold.ttf`, `JetBrainsMono-Regular.ttf`, `JetBrainsMono-Medium.ttf`. Load via `io.Fonts->AddFontFromFileTTF(...)` after `ImGui::CreateContext()` resolved against `Paths::ExecutableDir() / "assets/fonts" / ...` (matches the Sprint 3.7 `GizmoRenderer` fix pattern).
- Expose font handles globally: `MvFont::Display` (28px Saira), `MvFont::Body` (14px Inter Regular), `MvFont::BodySm` (13px Inter Regular), `MvFont::Heading` (20px Inter SemiBold), `MvFont::Mono` (11px JetBrains Mono Regular), `MvFont::MonoSm` (10px JetBrains Mono Medium).
- Bundle `assets/metavisage-logo-mark.png` (already exists at `.claude/context/design/version_2/assets/`) — load on startup as a GL texture for the title bar; reuse the `stb_image` infra already linked from `include/thirdparty/`.

### Story 3.8.2: Window Chrome — Title Bar, Menu Bar & Status Bar (4 pts)

The current shell renders the menu bar via `ImGui::BeginMenuBar()` inside `##AppHostWindow` at `AppWindow.cpp:773` and the status bar as a single `ImGui::Text(...)` line at `AppWindow.cpp:849`. Replace both, plus add a custom 36px title bar above the menu bar.

- **Title bar (36px)**: drawn with `ImGui::GetWindowDrawList()`. Contents: 18×18 logo image (`MvBg0` background), "MetaVisage" in `MvFont::Body` SemiBold, separator dot, filename in `MvFg2`, `›` separator, "Stage N / 4 · STAGE NAME" in `MvBlue400` mono uppercase. Right-aligned minimize / maximize / close caption buttons; close-button hover background `#E81123`. Keep `glfwSetWindowTitle` in sync. The window stays decorated (native OS frame); the custom title bar sits *inside* the GLFW client area below the OS chrome — this avoids the GLFW frameless / hit-test rabbit hole flagged in *Risks*.
- **Menu bar (32px)**: `MvBg2` fill, `MvLine2` bottom border. Items `File | Edit | View | Mesh | Help` (rename current `Tools` → `Mesh`). Right-side: autosave indicator in `MvFg3` mono, then the "UE 5.4 · LIVE LINK" pill (green dot + `MvSuccess` text on `rgba(63,214,158,0.06)` with green border). Live Link is **display-only scaffolding** for v2.0; real integration is later.
- **Status bar (28px)**: `MvBg0` fill, `MvLine2` top border. Mono sections separated by `MvLine3` middots: `● Ready`, `Stage 0N STAGENAME` (in `MvBlue400`), `Source filename`, `Target MetaHuman`, then right-aligned `GPU model · n%` and `Live link UE 5.4 ●` in `MvSuccess`. **No FPS counter** — preserves the Sprint 1.4 decision (open question: see *Risks*).

### Story 3.8.3: Stage Stepper (3 pts)

Most visually prominent new element. A 64px full-width window below the menu bar showing all four stages simultaneously.

- Each stage button drawn with `ImDrawList`: 36×36 numbered disc (`rounding=6`) — fill `MvBlue500` + border `MvBlue300` when active; `rgba(63,214,158,0.12)` + green border when complete; `MvBg2` + `MvLine2` when upcoming. Checkmark glyph when complete, mono "0N" otherwise.
- 10px mono uppercase status label ("IN PROGRESS" / "COMPLETE" / "UPCOMING"), 14px SemiBold stage name, 2px `MvBlue500` underline on the active step, `MvFg4` chevrons between steps.
- Click behaviour calls `StageManager::SetStage(n)`, applying the same forward-jump validation guards the current Next Stage button uses (cannot skip ahead past an incomplete stage).
- Hover: non-active steps fill `MvBg2`.

### Story 3.8.4: Left Tool Rail (1 pt)

56px-wide vertical rail visible **only in Stage 1**.

- 9px mono "XFORM" header in `MvFg4`.
- Three 40×40 tool buttons (Move/Rotate/Scale) with hotkey badges (W/E/R) in the bottom-right corner. Active: `MvBlue500` fill + glow border + dark icon; inactive: transparent + `MvFg2` icon.
- `MvLine2` divider, then symmetry and pivot-snap toggles (visual scaffolding — wire to existing Stage 1 symmetry state where it already exists).
- Help button anchored at the bottom.
- Stage 1 work-area columns change from `28% sidebar | viewport` (current `AppWindow.cpp:733`) to `56px rail | viewport | 360px sidebar`. Tool rail is **not** rendered in Stages 2/3/4.

### Story 3.8.5: Viewport Toolbar & HUD Overlays (3 pts)

Replaces the current Sprint 2.1 shading-mode dropdown overlay (`Viewport.cpp:828` — pinned at fixed offset). Three new floating windows over the viewport:

- **Top-centre toolbar**: `ImGui::SetNextWindowPos`-anchored, `MvBg1` at 85% alpha (no real backdrop blur — the spec lives with this), `MvLine2` border, `rounding=6`. Contents: "PERSP" mono label, separator, Solid/Wireframe/Textured toggles (28×26, active `MvBlue500`), separator, Source/Target eye-icon visibility toggles, separator, mono hotkey strip. **Stage-specific variant**: Stage 3 swaps the visibility toggles for Source/Target/Deformed view-mode toggles plus a `READY/PROCESSING` status indicator.
- **Top-right meta pills**: small `LIVE` pill always; Stage 4 adds a strength % pill.
- **Bottom-left instruction HUD**: 320px overlay (`MvBg1` at 88% alpha, `MvLine2` border). Mono stage label, contextual instruction line, and a strip of `Kbd`-styled hotkey badges. Content keys off the active stage from `StageManager`.
- The existing `DrawShadingModeOverlay()` floating window is **deleted** — shading mode is owned by the new top-centre toolbar.

### Story 3.8.6: Inspector Panel — Shared Primitives (4 pts)

Load-bearing for stories 3.8.7–3.8.10. Land this **before** any per-stage panel work begins. New header `include/ui/MvInspectorPrimitives.h`, implementation `src/ui/MvInspectorPrimitives.cpp`. Seven helpers:

- `MvDrawStageHeader(eyebrow, title, accentWord, description)` — mono eyebrow ("STAGE 0N / 04" `MvBlue400`), Saira italic display heading with the accent word in `MvBlue500` (e.g. "Align**ment**"), 13px description in `MvFg2`.
- `MvDrawSectionLabel(title)` — 10px mono uppercase + 0.16em tracking, `MvFg3`, with a 6×6 `MvBlue500` square dot prefix and a `MvLine2` hairline above.
- `MvDrawFileSlot(filename, placeholder, isLoaded, isPrimary)` — `MvBg1` container with folder icon, mono filename or `MvFg3` placeholder, file-type+size sub-line when loaded, Import button beneath (primary blue or ghost).
- `MvDrawAxisInput(axis, value, axisColor, &outValue)` — drag-to-edit numeric input with the X/Y/Z axis letter in coloured (`#FF6B6B` / `#3FD69E` / `#3DA5F5`) mono, value right-aligned. Wraps `ImGui::DragFloat` with no label.
- `MvDrawTransformGroup(label, unit, values[3], uniform, onChange)` — label + unit row, then a 3-column grid of `MvDrawAxisInput` calls (uniform mode greys axes 1 and 2).
- `MvDrawValidationRow(label, value, status)` — 14px circle badge (✓ green / ! amber / × red), label in `MvFg2`, value in mono right-aligned.
- `MvDrawInspectorFooter(stepLabel, onPrevious, onContinue, continueLabel)` — 3-column footer (Previous ghost / mono step counter / Continue primary). Stage 4 callers pass a green `MvSuccess` Continue label.

### Story 3.8.7: Stage 1 Inspector Panel (2 pts)

Replace `StageSidebar::RenderAlignment()` (`StageSidebar.cpp:112–206`, ~95 lines) with primitive composition.

- Stage header: `("STAGE 01 / 04", "Alignment", "ment", "Position the target mesh ...")`.
- Source mesh section + Target mesh section (with "Use Default Head" ghost button).
- Target textures section: scrollable list of texture slots from the existing dynamic-texture model — 48×48 thumbnails via `ImGui::Image`, mono filename, remove button. "Add Texture" button at the bottom.
- Three `MvDrawTransformGroup` calls (Position cm / Rotation ° / Scale × uniform). Reset and Auto-align ghost buttons in a 2-col grid; Auto-align stays a stub that sets a near-zero transform.
- Validation section: four rows (Source loaded + vertex count, Target loaded + MetaHuman version, Bounds intersect %, Pivot offset).
- Footer: Previous disabled, Continue enabled when both meshes load.

### Story 3.8.8: Stage 2 Inspector Panel & Viewport Bar (2 pts)

Stage 2 has a **viewport-spanning info bar** (not part of the sidebar) plus the inspector panel.

- New 36px viewport info bar between the stepper and the dual viewports: SOURCE pill + filename + vertex count + point count on the left; TARGET pill + same on the right; centred "N pts" indicator.
- Sidebar: replace `RenderPointReference()` (`StageSidebar.cpp:212–246`, ~35 lines).
- Stage header + 3 stat cards (PAIRED `MvBlue500`, SOURCE ONLY `MvWarning`, TARGET ONLY conditional).
- Placement Tools section: two scaffolded ghost buttons ("Auto-detect" / "Load preset"), both disabled with "Coming in a future update" tooltips.
- Retain (restyle only) the existing point list, symmetry controls, point-size slider, Clear All button from Sprint 3.6.
- Footer: Continue enabled iff paired > 0 **and** source-only + target-only both 0.

### Story 3.8.9: Stage 3 Inspector Panel & Viewport Stats (3 pts)

The largest per-stage panel — replaces `RenderMorph()` (`StageSidebar.cpp:252–370`, ~120 lines including the existing "Advanced" `CollapsingHeader`).

- **Viewport result stats panel**: bottom-left frosted overlay shown only after a process attempt. `/ RESULT` header + `● CONVERGED` / `PROCESSING` / `FAILED` indicator + four mono rows (Max disp. mm, Avg disp. mm, Iterations, Elapsed s). Sourced from `DeformationResult`.
- **Top-centre toolbar**: gains Source / Target / Deformed view-mode toggles (replacing the visibility toggles used in other stages). Active mode uses the primary blue button style.
- **Preset card row**: horizontal scrollable area below the stage header. Each card ~140×72: `MvBg1` + `MvLine2`, active card `MvBlue500` border + `rgba(61,165,245,0.08)` fill. 32×32 PresetIcon drawn via `ImDrawList` line primitives (tight radial = Quick Wrap, loose flowing = Fantasy Wrap, star = Custom). Plus a "+ Save preset" card with a dashed `MvLine3` border. Click applies the preset's params to the controls. **The full preset save/load mechanism lands in Sprint 4** — Story 3.8.9 only delivers the visual treatment for the two built-ins; "+ Save preset" stays disabled with a tooltip.
- **Inspector parameter sections**: keep the General/Advanced collapsible split from Sprint 5.1; restyle headers to use `MvDrawSectionLabel` semantics within an `ImGui::CollapsingHeader` whose background is overridden to a `MvLine2` fill.
- Footer: Continue label is "Accept", enabled only when a successful `DeformationResult` is available.

### Story 3.8.10: Stage 4 Inspector Panel & Viewport Stats (2 pts)

Replace `RenderTouchUp()` (`StageSidebar.cpp:376–428`, ~55 lines).

- **Stage 4 viewport top bar (36px)**: SOLID shading label + Debug HUD ghost button left; `● COMPLETE/PROCESSING` status pill + "N% strength" indicator right.
- **Viewport mesh stats panel**: bottom-left frosted overlay. `/ MESH` header + `● READY TO EXPORT/PROCESSING` + 3 mono rows (Vertices, Triangles, UV maps).
- **Brush selector**: 2×2 grid of cards (Smooth / Grab / Push-Pull / Inflate). Each card: `MvBg1` + `MvLine2`, active `MvBlue500` border + tinted blue fill + glow shadow. Each `BrushIcon` drawn via `ImDrawList` (smooth = wave line, grab = hand, push-pull = double arrow, inflate = radial burst). 14px SemiBold name, 11px sub-label.
- **Radius / Strength sliders**: `ImGui::SliderFloat` with a custom-drawn track via `ImDrawList::AddRectFilledMultiColor` (`MvBlue600` → `MvBlue400` gradient) and a circular thumb with `MvBlue500` border.
- **Export Mesh button**: full-width primary green (`MvSuccess` fill) above the footer; opens the export modal.
- Footer: green "Finish" continue button — triggers finalize + export.

### Story 3.8.11: Welcome Modal Redesign (2 pts)

Restyle the existing `Modals::RenderWelcomeModal()` (`Modals.cpp:113`) to match `welcome.jsx`. Modal structure stays: a `BeginPopupModal` of 560px width, `ImGui::GetBackgroundDrawList()` scrim at `MvBg0` 55% alpha behind it.

- Header: 56×56 app-mark icon (gradient blue rounded rect + the logo PNG composited on top), `V 2.0.0 · EARLY ACCESS` eyebrow in `MvBlue400` mono, "Welcome to **MetaVisage**" Saira display heading with "MetaVisage" in `MvBlue400`, tagline in `MvFg2`. Top-right close button.
- Action row: 2-col grid of `BigAction` buttons. New Project (primary blue tint, Import icon), Open Project... (ghost, Folder icon). Each with 36×36 icon container, 14px label, 11px sub.
- Recent projects: `MvDrawSectionLabel("RECENT PROJECTS")` + file-count, scrollable `MvBg1`/`MvLine2` container. Each row hover-tints to `rgba(61,165,245,0.06)`. Reads from the existing recent-projects list in `Config`.
- Footer: "Show this on startup" checkbox + Close ghost button.

### Story 3.8.12: Login Screen Redesign (3 pts)

**Net-new construction** — the architect audit confirms no `LoginScreen` currently exists in the ImGui port (Sprint 6.1 hasn't shipped). Build the visual-only screen now so the auth wiring in a later sprint can drop straight into the surface.

- Full-screen ImGui window (`MvBg0` blueprint-grid background drawn via `ImGui::GetWindowDrawList()` repeated rectangle pattern + a faint radial-gradient approximation via `AddCircleFilled` with `MvBlue500` low-alpha).
- Four corner-bracket decorations (`AddLine` in `MvLine3`).
- 480px centred auth card: `MvBg2` fill, `MvLine3` border, `rounding=8`. Top accent bar drawn with `AddRectFilledMultiColor` (transparent → `MvBlue500` → `MvBlue300` → `MvBlue500` → transparent).
- Card hero: 64×64 app mark, `V 2.0.0 · SIGN IN REQUIRED` eyebrow, Saira display heading ("Sign in to **MetaVisage**" idle / "Awaiting **browser**" waiting), description.
- **Idle state**: full-width 48px primary "Sign in with browser" button + three numbered explanation rows (01/02/03).
- **Waiting state**: status block with the named-pipe listener address + countdown timer + LIVE badge; "Callback URI" mono display with Copy button; amber "browser didn't open?" warning. Spinner ring drawn around the app mark via `AddCircle` with a rotating `strokeDashoffset` updated by frame-tick.
- Footer: "SSO · OAuth 2.1 · PKCE" mono left, Cancel/Use offline ghost button right.
- Bottom metadata strip: BUILD / NODE / HELP / PRIVACY mono items.
- Toggle exposed via a `MvStyle::ShowLoginScreen` runtime flag — defaults to `false` for Sprint 3.8 (no auth wiring yet); a "Show login screen" entry under Help gates the visual review.

---

## Sprint Context (Key Affected Files)

**Architect audit, 2026-04-27** — current ImGui shell concentrates style and overlay drawing into a small number of touchpoints, but adding the three-font typography and the per-stage chrome touches every render path.

**Story 3.8.1 (tokens + fonts)**
- New: [include/ui/MvStyle.h](../../../include/ui/MvStyle.h), [src/ui/MvStyle.cpp](../../../src/ui/MvStyle.cpp)
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — replace `ApplyDarkTheme()` (lines 471–505) call site at line 150; add font-atlas build after `ImGui::CreateContext()`
- New asset directory: `assets/fonts/` (6 TTF files); copy `assets/metavisage-logo-mark.png` from the design dir
- [CMakeLists.txt](../../../CMakeLists.txt) — install fonts + logo PNG alongside the executable

**Story 3.8.2 (window chrome)**
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — title-bar block above the menu bar; rewrite `BeginMenuBar()` body at line 773; rewrite `RenderStatusBar()` at line 849; reserve title-bar height via dockspace size adjustment near line 713–715
- New: `src/ui/TitleBar.cpp` (logo image binding, caption-button drawing) — uses existing `stb_image` infra under [include/thirdparty/](../../../include/thirdparty/)

**Story 3.8.3 (stage stepper)**
- New: `src/ui/StageStepper.cpp`
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — reserve 64px above the dockspace; integrate into the existing dock-build pass near line 733

**Story 3.8.4 (tool rail)**
- New: `src/ui/ToolRail.cpp`
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — Stage 1-only render gate; update the 28% sidebar split at line 733 to a 56px-rail / viewport / 360px-sidebar layout in Stage 1

**Story 3.8.5 (viewport toolbar + HUDs)**
- [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp) — delete `DrawShadingModeOverlay()` at line 828; add three new `SetNextWindowPos` overlay windows. Note: `glClearColor(0.2, 0.2, 0.2, 1.0)` at line 662 needs to flip to `MvBg0` to avoid a grey halo behind the toolbars
- [src/ui/Viewport.cpp:610](../../../src/ui/Viewport.cpp#L610) — `DrawOverlays()` PackColor literals (active border, point-label backgrounds, orientation-gizmo axis colours) bypass `ImGuiStyle` and must be re-keyed against `MvBlue500`/`MvSuccess`/etc. directly

**Story 3.8.6 (primitives)**
- New: [include/ui/MvInspectorPrimitives.h](../../../include/ui/MvInspectorPrimitives.h), [src/ui/MvInspectorPrimitives.cpp](../../../src/ui/MvInspectorPrimitives.cpp)

**Stories 3.8.7–3.8.10 (per-stage panels)**
- [src/ui/StageSidebar.cpp](../../../src/ui/StageSidebar.cpp) — wholesale rewrite of `RenderAlignment()` (112–206), `RenderPointReference()` (212–246), `RenderMorph()` (252–370), `RenderTouchUp()` (376–428). The `ImGui::Begin("Stage", ...)` window identity at line 67 must remain the same name to keep the dockspace binding at `AppWindow.cpp:749` valid — restructuring the header into a separate window would break docking-layout persistence.
- Stage 2 viewport info bar: new render in [src/ui/ViewportHost.cpp](../../../src/ui/ViewportHost.cpp) above the dual-pane area
- Stage 3/4 viewport stats overlays: extend [src/ui/Viewport.cpp](../../../src/ui/Viewport.cpp) `DrawOverlays()` with the new bottom-left panels

**Story 3.8.11 (welcome modal)**
- [src/ui/Modals.cpp](../../../src/ui/Modals.cpp) — rewrite `RenderWelcomeModal()` at line 113

**Story 3.8.12 (login screen)**
- New: `src/ui/LoginScreen.cpp` + header
- [src/ui/AppWindow.cpp](../../../src/ui/AppWindow.cpp) — top-of-frame render gate before any other UI

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Dear ImGui has no `box-shadow`, no `backdrop-filter: blur`, and no arbitrary-shape gradient fills — designs that lean on these (frosted overlays, glow shadows, gradient buttons) will visually regress against the JSX previews | **High** | **Med** | Define the approximation policy in Story 3.8.1: glow → solid coloured borders + `MvShadow1`-style inset; backdrop-blur surfaces → `MvBg1` at 85–88% alpha (no real blur); gradients → `AddRectFilledMultiColor` only where structurally important (e.g. login top accent, slider tracks). Capture a screenshot of each surface vs the design PNG in `previews/` and document deviations in the PR; ask the design team to sign off **before** Story 3.8.6 closes (so per-stage stories don't re-litigate the policy). |
| Custom title bar (Story 3.8.2) tempts a GLFW frameless / manual-hit-test rewrite — this rabbit hole burned days on similar Windows desktop ports and isn't covered by Sprint 3.7's smoke test | **Med** | **High** | Explicitly **keep the OS-native window frame**. The custom title bar lives inside the GLFW client area below the OS chrome. Defer frameless to a separate sprint behind a feature flag if the design team insists. The DoD checks the OS title bar still appears. |
| Story 3.8.6 (shared primitives) is a load-bearing dependency for Stories 3.8.7–3.8.10. If primitives land late or have API churn, all four panel stories cascade | **Med** | **High** | Land Story 3.8.6 by **end of day 4** (sprint mid-point). Lock the public API in a `MvInspectorPrimitives.h` PR before any per-stage panel work begins; iterate on internals only. |
| `Viewport::DrawOverlays()` (`src/ui/Viewport.cpp:610`) uses `PackColor()` literals that bypass `ImGuiStyle` entirely — easy to miss when restyling and the active-viewport border, point labels, orientation-gizmo axes will silently keep their old colours | **Med** | **Low** | Story 3.8.5 explicitly enumerates every `PackColor` site (active border, point ID label bg, axis colours, label badge bg) and re-keys them to `MvStyle` constants. Add a code-review checklist item: "no raw RGB literals in `Viewport.cpp` after this sprint". |
| Three TTF fonts × multiple sizes inflates the ImGui font atlas — first-frame texture upload can spike VRAM and lengthen startup; embedded ProggyClean (current) is essentially free | **Low** | **Low** | Subset the TTFs to Latin + currency glyphs only via `ImFontConfig::GlyphRanges`. Cap the atlas to 2048×2048 (default 1024² will overflow). Measure startup time before and after; flag if startup grows >250 ms. |
| Window identity `"Stage"` is load-bearing — `StageSidebar.cpp:67` (`ImGui::Begin("Stage", ...)`) and `AppWindow.cpp:749` (`DockBuilderDockWindow("Stage", rightId)`) must stay aligned. Restructuring the sidebar into a separate header window would break docking-layout persistence on user upgrade | **Low** | **Med** | Keep the single `"Stage"` window. Use `ImGui::BeginChild` regions inside it for the new header / body / footer if needed. The `imgui.ini` users have on disk continues to dock correctly. |
| 32-pt commitment is a stretch over the Sprint 3.7 baseline of 26 pts; per-stage panel stories assume primitives compress real work, but if the design team requests pixel-level fidelity revisions late in the sprint, Story 3.8.12 (login) is the natural carryover | **Med** | **Low** | Treat Story 3.8.12 as the soft-deferral candidate — login auth wiring isn't scheduled until Sprint 6.1, so the visual surface can slip a sprint without blocking anyone. Keep Stories 3.8.1–3.8.11 as the hard commitment. |
| Open question: the redesign shows "FPS 120" in Stage 3/4 viewport pills, but the v1.0.0 feedback (Sprint 1.4) explicitly removed the FPS counter | **Med** | **Low** | This sprint plan **honours the Sprint 1.4 decision** — no FPS counter; the LIVE pill replaces it. If the design team insists on re-introducing FPS, file a single-line scope-change PR before sprint day 5. Deferring the question past day 5 risks rework in Stories 3.8.5 / 3.8.9 / 3.8.10. |
| The texture-list section in Story 3.8.7 uses `ImGui::Image` with thumbnails — needs each texture's preview as a GL texture handle; current Sprint 3.6 texture model holds the FBX-bound albedo bytes but may not cache a preview surface | **Low** | **Med** | Verify the texture-cache shape early in Story 3.8.7. If no preview texture exists, generate one on import via `stb_image` resize (already linked) at 64×64 and cache on the texture record. |
| Logo PNG load fails if `assets/metavisage-logo-mark.png` isn't installed alongside the executable — same class of bug Sprint 3.7 fixed for `GizmoRenderer` | **Low** | **Med** | Always resolve via `Paths::ExecutableDir() / "assets/..."`. Verify the install rule in CMakeLists.txt covers PNGs and TTFs (the existing rule may only cover FBX/OBJ/GLSL). |

---

## Definition of Done

- [ ] All committed stories implemented and merged to the sprint branch
- [ ] Build clean: `cmake --build . --config Release` succeeds with no new warnings (treat new font / image-load paths as warning-fence)
- [ ] CTest green: `ctest -C Release` passes (Sprint 3.7's smoke test still loads the default MetaHuman → exports FBX with the new style applied)
- [ ] `MvStyle.h` contains every `--mv-*` token from `colors_and_type.css`; `ApplyDarkTheme()` is deleted and `MvStyle::Apply()` is called exactly once at startup
- [ ] Saira / Inter / JetBrains Mono fonts load from `assets/fonts/` and render at every required size; no `ProggyClean` glyphs visible anywhere
- [ ] OS-native window frame is still present (title bar custom chrome lives inside the client area)
- [ ] Menu bar reads `File | Edit | View | Mesh | Help`; Live Link pill renders even though it's display-only
- [ ] Stage stepper renders all four stages with correct active/complete/upcoming states; clicking a completed stage navigates back; forward-jump guard from `StageManager` is preserved
- [ ] Tool rail appears in Stage 1 only with three transform tools, hotkey badges, and active-state highlighting
- [ ] Top-centre viewport toolbar renders the correct controls per stage; old `DrawShadingModeOverlay` is gone; bottom-left HUD shows stage-appropriate text
- [ ] All seven inspector primitives (`MvDrawStageHeader`, `MvDrawSectionLabel`, `MvDrawFileSlot`, `MvDrawAxisInput`, `MvDrawTransformGroup`, `MvDrawValidationRow`, `MvDrawInspectorFooter`) are used consistently across all four stage panels — no raw `ImGui::Text("STAGE 0N / 04")` survives in `StageSidebar.cpp`
- [ ] Stage 2 viewport info bar shows source/target metadata; PAIRED/SOURCE-ONLY/TARGET-ONLY stat cards update in real time as points are placed/deleted
- [ ] Stage 3 result stats panel appears after a successful process; preset cards highlight correctly; Source/Target/Deformed view toggles drive the renderer
- [ ] Stage 4 brush selector renders as a 2×2 grid; mesh stats panel shows vertex/triangle/UV counts; Export button opens the existing export modal
- [ ] Welcome modal matches the design with scrollable recent projects, action cards, and "Show this on startup" footer checkbox
- [ ] Login screen renders correctly in idle and waiting states (toggle via Help menu); spinner animates; countdown timer ticks down each second
- [ ] No raw `ImVec4(...)` colour literals or RGB hex constants in any UI source file outside `MvStyle.cpp` (grep guard in CI is a stretch goal — at minimum a manual review check)
- [ ] No legacy grey ImGui theme is visible anywhere in the application
- [ ] PR opened against `development` and `/review-pr` agent run; **all blocking findings resolved**
- [ ] Phase D manual UI smoke completed on Windows — Stage 1 → 2 → 3 → 4 → Export end-to-end with the bundled MetaHuman head, plus Undo/Redo verification on point placement, gizmo drag, mask paint, and sculpt brush (Sprint 3.7 retro action item #1 — the new chrome must not regress what we just fixed)
- [ ] Side-by-side screenshot capture: every redesigned surface vs its `previews/*.png` reference, posted in the PR for design-team sign-off
- [ ] Sprint outcome notes updated with actual velocity, carryover (or "None"), and a retrospective link

> **Process note (Sprint 3.7 retro action item #2):** Do NOT mark the Sprint Outcome
> as "Complete" or "Implementation Complete" until **both** the `/review-pr`
> blockers are closed **and** the manual UI smoke + design-team screenshot review
> have run. The Sprint 3.6 and 3.7 retros both flagged the premature-completion
> anti-pattern; a UI sprint is *especially* prone to it because the build can be
> green while the visuals are wrong.

---

## Sprint Outcome

**Status**: Planned
**Velocity (Actual)**: TBD
**Carryover**: TBD
**Notes**: TBD
