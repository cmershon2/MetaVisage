# Sprint Plan: MetaVisage v2.0

**Version:** 2.0  
**Date:** April 2, 2026  
**Total Duration:** 16 weeks (8 sprints @ 2 weeks each)  
**License:** Proprietary (Closed Source)

---

## Overview

This sprint plan covers the v2.0 release of MetaVisage. It addresses all v1.0.0 user feedback, introduces the licensing and subscription system required by the transition to a paid product, adds distribution infrastructure (installer and auto-updates), and completes the remaining UX polish work across all four workflow stages.

**Inputs:**
- v1.0.0 user feedback (Version_1_0_0_Feedback.md)
- Desktop App Integration Guide (DESKTOP_APP_INTEGRATION.md) — licensing API contract from the web team
- PRD-WebPortal.md — tier feature matrix and feature flag definitions

**Sprint Structure:**
- Sprint 1: Critical Bug Fixes & Navigation Polish
- Sprint 2: Viewport & Global UI Infrastructure
- Sprint 3: Stage 1 — Transform Gizmos & Texture System
- Sprint 4: Stage 3 — Wrap Engine Redesign (Algorithm & Preset System)
- Sprint 5: Stage 3 — Advanced Parameters, Masking & Stage 4 Polish
- Sprint 6: Licensing Infrastructure — Core Integration
- Sprint 7: Licensing Infrastructure — Feature Gating & Tier Enforcement
- Sprint 8: Distribution, Auto-Update & Release

---

## Sprint 1: Critical Bug Fixes & Navigation Polish

**Duration:** 2 weeks  
**Goals:** Resolve all confirmed bugs from v1.0.0 testing, clean up the top navigation bar, and address high-impact UX issues that affect all users immediately.

### Stories

#### Story 1.1: Stage 2 Save/Load Bug
**Background:** When a project saved during Stage 2 (Point Reference) is reopened, points render correctly in the viewport but the sidebar shows zero target/morph point counts, an empty Correspondence Points list, and symmetry mode shows as disabled even when it was previously enabled.

**Tasks:**
- [x] Audit `ProjectSerializer` deserialization path for Stage 2 — identify why `PointReferenceData` is not being applied to `SidebarWidget` on load
- [x] Ensure `SidebarWidget::OnStageChanged()` re-populates point list and count displays from loaded `Project` state when entering Stage 2 from a loaded project
- [x] Restore symmetry enabled/axis state from deserialized `PointReferenceData` to the sidebar checkbox and axis selector
- [ ] Write a manual regression test: save a project mid-Stage 2 with 10 points and symmetry enabled, reload, verify counts and symmetry state match

#### Story 1.2: Top Navigation Bar Cleanup
**Background:** Several menu items are either broken or shown in stages where they are not applicable, creating confusion.

**Tasks:**
- [x] Fix `Help > Documentation` — open `https://docs.metavisage.io` via `QDesktopServices::openUrl()`
- [x] Remove the `Tools` menu from the menu bar entirely — transform tools are accessible via hotkeys and Stage 1 sidebar; the global menu adds noise in stages where it does not apply
- [x] Remove `Edit > Preferences` — currently a no-op; remove until a real preferences dialog is implemented
- [x] Make `File > Export Mesh` context-sensitive — hide this item in all stages except Stage 4 (Touch Up), where it should remain available
- [x] Audit `View` menu items — remove or disable any entries that have no functional implementation; retain only working options (shading mode, grid toggle, camera views)

#### Story 1.3: System Theme Override
**Background:** On systems configured to use Windows Light Mode, Qt inherits the light palette and renders the application background in white/light gray, inconsistent with the intended dark theme.

**Tasks:**
- [x] Force the application-wide Qt palette to the existing dark theme at launch, regardless of the OS-level theme setting — use `QApplication::setStyle("Fusion")` and apply the custom dark palette unconditionally
- [ ] Verify that all widgets (including the settings panel, which was reportedly already dark) render consistently
- [ ] Test on a Windows 11 system configured to Light Mode

#### Story 1.4: Remove FPS Counter
**Background:** The FPS counter in the status bar was reported as frequently inaccurate and is not considered a useful user-facing metric.

**Tasks:**
- [x] Remove the FPS counter display from the status bar
- [x] Remove the 500ms FPS update timer from `ViewportWidget`
- [x] Retain any underlying frame timing code that is used internally by the rendering pipeline; only remove the display

#### Story 1.5: Landing Page — Recent Projects Scrollability
**Background:** The recent projects list on the landing/splash screen squishes items together and is not scrollable when more than a few projects are present.

**Tasks:**
- [x] Wrap the recent projects list widget in a `QScrollArea`
- [x] Set a maximum visible height for the list (e.g. 5 items before scrolling activates) so the dialog does not grow unbounded
- [x] Verify that project items display at consistent height and do not compress when the list is long

### Acceptance Criteria
- [x] Reopening a Stage 2 project shows correct point counts, correspondence list, and symmetry state in the sidebar
- [x] `Help > Documentation` opens `https://docs.metavisage.io` in the default browser
- [x] `Tools` menu is no longer visible in the menu bar
- [x] `Edit > Preferences` is no longer visible in the menu bar
- [x] `File > Export Mesh` is hidden in Stages 1–3 and visible in Stage 4
- [x] Application renders dark background on both Dark Mode and Light Mode Windows systems
- [x] FPS counter is no longer shown in the status bar
- [x] Recent projects list is scrollable and items display at uniform height

---

## Sprint 2: Viewport & Global UI Infrastructure

**Duration:** 2 weeks  
**Goals:** Resolve all viewport-level feedback items: the shading mode control placement, orientation gizmo, infinite grid, and camera-relative lighting. These changes affect the experience across all four stages.

### Stories

#### Story 2.1: Shading Mode — Viewport Overlay Control
**Background:** The shading mode selector currently lives in the Stage 1 sidebar and does not function in Stage 1 at all. It should be moved to a persistent overlay in the top-left corner of the viewport, visible and functional in all stages.

**Tasks:**
- [x] Remove the shading mode selector from `SidebarWidget`'s Stage 1 controls
- [x] Add a compact shading mode dropdown/button group as a floating overlay widget anchored to the top-left of `ViewportWidget` — rendered above the 3D viewport using a `QWidget` child positioned absolutely
- [x] Ensure the overlay is visible and functional in all four stages, including the dual-viewport layout of Stage 2 (one control per viewport, or a shared control driving both)
- [x] Fix the underlying bug that caused the shading mode selector to have no effect in Stage 1 — trace the signal connection between the old sidebar control and the renderer and replicate it for the new overlay control

#### Story 2.2: Orientation Gizmo
**Background:** Users have no visual reference for axis orientation while navigating the viewport, which makes alignment and point placement disorienting.

**Tasks:**
- [x] Implement an orientation gizmo rendered in the top-right corner of the viewport — a small, fixed-size 3-axis indicator (X = red, Y = green, Z = blue) that rotates to match the current camera orientation
- [x] Render the gizmo in its own viewport-space region (e.g. 80×80px inset) using a separate MVP matrix that applies only the camera's rotation component, not translation or the main projection
- [x] Display axis labels (X, Y, Z) alongside each axis arm
- [x] Ensure the gizmo renders in both single-viewport and dual-viewport layouts without overlapping other overlays
- [x] Gizmo is display-only; clicking it is not required for v2 (deferred)

#### Story 2.3: Infinite Grid
**Background:** The current 3D grid has a fixed size and visually terminates at the edges, which looks unpolished and disorienting at certain camera distances.

**Tasks:**
- [x] Replace the fixed-size grid with an infinite/fading grid rendered via a shader technique — use a full-screen quad in world space with distance-based alpha fade, or extend the existing grid plane shader to use a repeating pattern with distance fade
- [x] Grid lines should fade out gracefully at the horizon/edges so the boundary is not visible
- [x] Grid line density should adapt to zoom level (coarser lines visible at greater zoom distances, finer lines visible when zoomed in)
- [x] Grid rendering performance should not degrade compared to v1 for typical camera distances

#### Story 2.4: Camera-Relative Lighting
**Background:** The directional light in the viewport is currently fixed in world space, meaning it does not move as the camera orbits. This causes the mesh to appear statically lit from one direction regardless of viewing angle, reducing the usefulness of the viewport for evaluating surface quality.

**Tasks:**
- [x] Change the directional light direction to be specified in view space (or update the light direction uniform to track the camera view matrix) so that the key light always illuminates the mesh from a consistent direction relative to the viewer
- [x] Ensure this change applies to all shading modes (solid, heatmap, matcap) and all stages
- [x] The change should be transparent to the user — no new UI control is needed; the light simply moves naturally with the camera

### Acceptance Criteria
- [x] Shading mode control appears as a viewport overlay in the top-left corner in all four stages
- [x] Shading mode changes take effect immediately in Stage 1 (bug fixed)
- [x] Orientation gizmo appears in the top-right corner of the viewport and rotates correctly as the camera orbits
- [x] Grid extends to the visual horizon with no visible boundary edge
- [x] Grid line density adjusts appropriately as the user zooms in and out
- [x] The directional light moves with the camera in all stages and all shading modes

---

## Sprint 3: Stage 1 — Transform Gizmos & Texture System

**Duration:** 2 weeks  
**Goals:** Implement visual transform gizmos in Stage 1 (the most commonly cited UX gap from v1.0.0 feedback), fix the rotation alignment bug, and build the unlimited texture system with preview thumbnails.

### Stories

#### Story 3.1: Transform Gizmos
**Background:** Transform tools in Stage 1 currently rely entirely on keyboard shortcuts and mouse drag with no visual indication. Users expect Blender-style axis gizmos for Move, Rotate, and Scale.

**Tasks:**
- [x] Implement a `TransformGizmo` base class responsible for rendering and hit detection
- [x] Implement `MoveGizmo` — three axis arrows (X/Y/Z) plus a center handle for free movement; arrows rendered as cones with shaft lines; axis colors match orientation gizmo (X=red, Y=green, Z=blue)
- [x] Implement `RotateGizmo` — three rotation arcs around each axis, rendered as partial circles in their respective axis colors
- [x] Implement `ScaleGizmo` — three axis handles with box endpoints plus a center handle for uniform scale
- [x] Implement ray casting for gizmo hit detection (`GizmoRayCast()`) to determine which axis handle the mouse is hovering over or clicking
- [x] Highlight the hovered axis handle on mouse-over
- [x] Apply the drag delta to the target mesh transform for the selected axis when the user clicks and drags a gizmo handle
- [x] Gizmos activate based on the current transform mode (G=Move, R=Rotate, S=Scale) — the active gizmo is shown at the target mesh's world position
- [x] Keyboard axis constraint (X/Y/Z keys) still functions when a gizmo is active — pressing an axis key locks the gizmo to that axis

#### Story 3.2: Rotation Alignment Fix
**Background:** Users reported that the rotation values displayed in the sidebar do not match the apparent rotation of the mesh in the viewport.

**Tasks:**
- [x] Investigate the discrepancy between the Euler angles displayed in the sidebar and the visual rotation applied via the `Transform` quaternion — likely a gimbal lock or axis convention mismatch
- [x] Standardize the rotation display to use a consistent Euler convention (XYZ, extrinsic) matching what the viewport quaternion represents
- [x] Ensure that setting rotation values via the sidebar (when numeric spinboxes are implemented) and via the rotate gizmo both produce the same visual result
- [x] Regression test: apply a 45° Y rotation via both the viewport drag and sidebar input, verify the mesh visually matches in both cases

#### Story 3.3: Unlimited Texture Uploads
**Background:** The texture system is currently hard-coded to accept only two textures (Albedo and Normal Map). Users need to upload arbitrary numbers of texture maps.

**Tasks:**
- [x] Refactor the texture data model in `MeshReference` to store a dynamic list of named textures (`vector<TextureSlot>` where `TextureSlot` has a name, filepath, and GL texture ID) rather than two fixed slots
- [x] Update the Stage 1 sidebar texture section to show a scrollable list of uploaded textures, one row per texture
- [x] Add an "Add Texture" button that opens a file dialog allowing the user to select an image file; the user can provide a name for the slot (e.g. "Roughness", "AO", "Emissive")
- [x] Add a remove button per texture slot
- [x] Persist all texture slots in the project file via `ProjectSerializer`

#### Story 3.4: Texture Thumbnails & Preview Modal
**Background:** Users have no way to visually identify a texture from its filename alone.

**Tasks:**
- [x] For each texture in the sidebar list, display a small thumbnail (48×48px) generated from the texture image alongside the filename
- [ ] Load thumbnail images asynchronously on a background thread to avoid blocking the UI when loading projects with many textures
- [x] Clicking a thumbnail opens a modal dialog (`TexturePreviewDialog`) that displays the full-resolution texture image, the texture name, filepath, and dimensions
- [x] The modal should be resizable and display the image scaled to fit while preserving aspect ratio

### Acceptance Criteria
- [x] Move, Rotate, and Scale gizmos render at the target mesh position in Stage 1
- [x] Gizmo axis handles highlight on hover
- [x] Dragging a gizmo handle applies the corresponding transform to the target mesh
- [x] Rotation values in the sidebar match the visual rotation in the viewport
- [x] Users can add any number of named texture slots via the sidebar
- [x] Each texture slot displays a 48×48px thumbnail
- [x] Clicking a thumbnail opens a preview modal with the full-resolution image
- [x] All texture slots are saved and restored by the project serializer

---

## Sprint 4: Stage 3 — Wrap Engine Redesign (Algorithm & Preset System)

**Duration:** 2 weeks  
**Goals:** Remove the RBF algorithms and all public references to the underlying algorithm technology, introduce the renamed "Wrap Engine", and build the preset card system with Quick Wrap and Fantasy Wrap built-in presets plus user-saved presets.

### Stories

#### Story 4.1: Remove RBF Algorithms & Rebrand Algorithm
**Background:** RBF algorithms are outperformed by the current NRICP implementation and are no longer relevant. Additionally, exposing the name "NRICP" in the UI reveals implementation details the team wants to keep confidential.

**Tasks:**
- [ ] Remove `RBF_TPS`, `RBF_GAUSSIAN`, and `RBF_MULTIQUADRIC` from the `DeformationAlgorithm` enum in `Project.h`
- [ ] Remove `RBFInterpolator` class and associated files from the codebase and `CMakeLists.txt`
- [ ] Remove the RBF parameter panel (Stiffness/Smoothness/Kernel dropdown) from `SidebarWidget`
- [ ] Remove the Algorithm dropdown from the sidebar entirely — the underlying algorithm is now a single, unnamed engine
- [ ] Replace any user-visible references to "NRICP", "Non-Rigid ICP", or "Iterative Closest Point" with "Wrap Engine" throughout the UI (status messages, progress dialogs, tooltips, sidebar labels)
- [ ] Ensure project files saved with v1.0.0 (which may contain `algorithm: "NRICP"`) continue to load correctly — treat any previously saved algorithm value as the current engine

#### Story 4.2: Built-In Wrapping Presets
**Background:** Users need sensible starting points for different character types. Two built-in presets have been defined: Quick Wrap (human characters, rigidity parameters set to 1) and Fantasy Wrap (non-human/dramatic features, current default parameters).

**Tasks:**
- [ ] Define a `WrapPreset` data structure: name, description, thumbnail asset path, and a full set of wrapping parameters (all fields in `NRICPParams` and boundary exclusion params)
- [ ] Implement `Quick Wrap` preset: all parameters identical to current defaults except rigidity init and rigidity final both set to 1.0
- [ ] Implement `Fantasy Wrap` preset: all parameters set to current application defaults (no change from v1.0.0 behavior)
- [ ] Built-in presets are read-only and cannot be deleted or renamed by the user
- [ ] On new projects, `Quick Wrap` is selected by default

#### Story 4.3: Preset Card UI
**Background:** Presets should be displayed as visual cards at the top of the Stage 3 sidebar, each with a graphic thumbnail, rather than a text dropdown.

**Tasks:**
- [ ] Replace the algorithm dropdown at the top of the Stage 3 sidebar with a horizontal scrollable card row
- [ ] Each card displays: a thumbnail graphic (128×80px, from asset file or placeholder), a preset name label, and a selection highlight when active
- [ ] Clicking a card applies that preset's parameters to all parameter controls beneath it
- [ ] Built-in preset thumbnails are provided as static assets (`assets/presets/quick_wrap.png`, `assets/presets/fantasy_wrap.png`)
- [ ] A `+ Save Preset` card at the end of the row opens the save preset dialog (Story 4.4)

#### Story 4.4: User-Saved Presets
**Background:** Users want to save their own tuned parameter sets for reuse across projects.

**Tasks:**
- [ ] Implement a "Save Current as Preset" dialog: prompts for a preset name; saves the current parameter values as a named `WrapPreset`
- [ ] User presets are stored in a local presets directory (`%APPDATA%\MetaVisage\presets\`) as individual JSON files
- [ ] User presets appear in the card row after the built-in presets, with a user-defined thumbnail (placeholder image used if none provided — allow the user to optionally choose a thumbnail image via file dialog)
- [ ] User presets have a context menu (right-click) with options to rename and delete
- [ ] Deleting a user preset shows a confirmation dialog
- [ ] User presets persist across projects and application restarts

### Acceptance Criteria
- [ ] No references to "RBF", "TPS", "Gaussian", "Multiquadric", "NRICP", or "Iterative Closest Point" appear anywhere in the UI
- [ ] `RBFInterpolator` source files are removed from the project and build
- [ ] v1.0.0 project files load correctly without errors
- [ ] Quick Wrap and Fantasy Wrap preset cards appear at the top of the Stage 3 sidebar
- [ ] Selecting a preset card populates all parameter controls with the preset's values
- [ ] Users can save the current parameters as a named preset
- [ ] User presets appear in the card row and persist across restarts
- [ ] User presets can be renamed and deleted via right-click context menu

---

## Sprint 5: Stage 3 — Advanced Parameters & Stage 4 Polish

**Duration:** 2 weeks  
**Goals:** Implement the full collapsible parameter hierarchy in Stage 3, investigate and address the "cottage cheese face" deformation artifact, and add visual brush selector images to Stage 4.

### Stories

#### Story 5.1: Collapsible Parameter Sections
**Background:** The Stage 3 sidebar needs a new parameter hierarchy to reduce visual clutter while exposing advanced controls to power users. The structure was specified in the feedback document.

**Tasks:**
- [ ] Implement collapsible section widgets in `SidebarWidget` — each section has a header button that toggles a child widget's visibility with a caret/arrow indicator
- [ ] Restructure the Stage 3 parameter area with the following layout:
  - **Vertex Masking** — not collapsible; always visible; existing masking controls
  - **General** (collapsible, expanded by default):
    - Stiffness Init slider/spinbox
    - Stiffness Final slider/spinbox
    - Stiffness Steps spinbox
    - ICP Iterations spinbox
    - Normal Threshold slider
    - Landmark Weight slider
    - **Interior Geometry** (sub-section, collapsible):
      - Exclude Boundary Regions checkbox
      - Boundary Depth slider (1–10 hops)
  - **Advanced** (collapsible, collapsed by default):
    - **Optimization** (sub-section):
      - Opt Iterations spinbox
      - Dp Initial slider
      - Dp Final slider
    - **Rigidity** (sub-section):
      - Rigidity Init slider
      - Rigidity Final slider
    - **Control Node Sampling** (sub-section):
      - Sampling Init spinbox
      - Sampling Final spinbox
      - Normalize Sampling checkbox
- [ ] Collapsed/expanded state for each section persists between sessions (stored in `QSettings`)
- [ ] "Reset to Defaults" button resets all parameters across all sections to their default values and re-populates the controls
- [ ] Serialize all new Advanced parameters (`OptIterations`, `DpInitial`, `DpFinal`, `RigidityInit`, `RigidityFinal`, `SamplingInit`, `SamplingFinal`, `NormalizeSampling`) in `NRICPParams`, `MorphData`, and `ProjectSerializer` with backward-compatible defaults for v1.0.0 project files

#### Story 5.2: "Cottage Cheese Face" Artifact Investigation
**Background:** Users have reported severe local geometry stretching/distortion around correspondence point locations after wrapping, colloquially called "cottage cheese face".

**Tasks:**
- [ ] Reproduce the artifact reliably using the reported conditions (points placed densely on facial features, typical MetaHuman LOD0 mesh)
- [ ] Profile the RBF-to-NRICP transition area — investigate whether the issue is caused by landmark weight values being too high relative to stiffness, causing over-pull at landmark vertices
- [ ] Investigate whether a minimum stiffness floor or landmark weight cap should be enforced to prevent singular deformation around control points
- [ ] Investigate whether the boundary exclusion BFS hop count needs tuning for typical MetaHuman meshes
- [ ] If a root cause is identified: implement the fix and document it in the progress notes
- [ ] If the root cause is not fully isolatable within this sprint: document findings, add a user-visible warning in the UI when landmark weight is above a threshold known to cause artifacts, and schedule follow-up work

#### Story 5.3: Stage 4 — Visual Brush Selector
**Background:** The brush selector in Stage 4 currently uses text-labeled buttons. Users requested visual sample images showing what each brush does.

**Tasks:**
- [ ] Replace the current 2×2 QPushButton grid brush selector with a visual card-style selector
- [ ] Each brush card displays: a sample illustration or render showing the brush effect (e.g. a "before/after" surface diagram), the brush name, and a selection highlight when active
- [ ] Brush sample images are provided as static assets (`assets/brushes/smooth_preview.png`, `assets/brushes/grab_preview.png`, `assets/brushes/pushpull_preview.png`, `assets/brushes/inflate_preview.png`)
- [ ] Cards are arranged in a 2×2 grid; clicking a card selects that brush tool
- [ ] The active brush card remains visually highlighted

### Acceptance Criteria
- [ ] Stage 3 sidebar shows Vertex Masking always visible, General and Advanced as collapsible sections
- [ ] All parameters from the specified hierarchy are present and functional
- [ ] Advanced parameters are wired through to `NRICPParams` and affect the deformation
- [ ] Collapsed/expanded state persists across sessions
- [ ] v1.0.0 project files with missing advanced parameter fields load with sensible defaults
- [ ] "Cottage cheese face" artifact is either fixed or a user-visible warning is shown when parameter values are known to risk the issue
- [ ] Stage 4 brush selector displays visual sample image cards for all four brushes
- [ ] Active brush card is highlighted

---

## Sprint 6: Licensing Infrastructure — Core Integration

**Duration:** 2 weeks  
**Goals:** Integrate the desktop application with the MetaVisage licensing API. This sprint covers the full license validation flow, JWT caching, offline grace period, and the in-app license entry and status UI. This is the foundational sprint enabling the transition to the paid model.

### Stories

#### Story 6.1: License Key Entry & Storage
**Background:** Users receive a 32-character hex license key from the web portal dashboard. The desktop app must provide a way to enter this key and store it securely.

**Tasks:**
- [ ] Add a `LicenseManager` singleton class responsible for all license-related operations: key storage, JWT caching, validation calls, and feature flag access
- [ ] On first launch (no cached license), show a license entry dialog: a text input for the license key, a "Validate" button, and a link to `https://metavisage.io` to obtain a key
- [ ] The free tier must still be accessible — add a "Continue with Free Tier" button that calls the validation API with no key and accepts a free-tier JWT response; this allows users who signed up for free to use the app without manually copying a key if one is provided
- [ ] Store the license key securely in `QSettings` under an obfuscated key name
- [ ] If a valid cached JWT already exists on launch, skip the entry dialog and proceed directly to the application

#### Story 6.2: License Validation API Integration
**Background:** The app must call `POST /api/license/validate` on launch (and periodically every 24 hours) to obtain a fresh signed JWT. See `DESKTOP_APP_INTEGRATION.md` for the full API contract.

**Tasks:**
- [ ] Implement `LicenseManager::ValidateOnline()` using `QNetworkAccessManager` to call `POST https://metavisage.io/api/license/validate` with `Authorization: Bearer <license_key>`
- [ ] Parse the JSON response: extract `token`, `tier`, `conversions_used`, `conversions_limit`, `conversions_reset_at`
- [ ] On success (HTTP 200): cache the JWT (Story 6.3), update in-memory tier and usage state
- [ ] Handle all documented error codes with appropriate user-facing messages:
  - `401 unauthorized` — "Invalid license key. Please check your key at metavisage.io."
  - `403 inactive` — "Your subscription has been cancelled. Please resubscribe at metavisage.io."
  - `403 expired` — "Your payment is past due. Please update your payment method at metavisage.io."
  - `404 not_found` — "No active license found for this key."
  - `429 rate_limited` — Respect `Retry-After` header; retry silently after the specified delay
  - `500 internal_error` — Retry with exponential backoff (1s, 2s, 4s, max 30s); fall back to cached JWT
- [ ] Set up a 24-hour `QTimer` to trigger background re-validation while the app is running; re-validation failures fall back to the cached JWT silently if within the grace window
- [ ] Configure the API base URL via a compile-time constant (`METAVISAGE_API_BASE_URL`), with staging and local dev values available via build flags

#### Story 6.3: JWT Cache (Offline Grace Period)
**Background:** The cached JWT allows the app to function for up to 7 days without network access. The cache must be encrypted to prevent copying between machines.

**Tasks:**
- [ ] On successful validation, serialize the raw JWT string to a local cache file (`%APPDATA%\MetaVisage\license.cache`)
- [ ] Encrypt the cache file using AES-256 with a key derived from: the Windows machine GUID (`HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid`) concatenated with the user's license key, passed through SHA-256
- [ ] On app launch, if online validation fails or the device is offline: attempt to load and decrypt the cached JWT; verify the JWT signature using `LICENSE_JWT_SECRET` (embedded as a compile-time constant)
- [ ] Check the JWT `exp` claim: if not expired (within the 7-day window), grant full access using the cached tier and feature flags
- [ ] If the cached JWT is expired and the device is offline: show a dialog informing the user they must connect to the internet to continue, with a "Retry" button
- [ ] On `403 inactive` or `403 expired` responses: clear the cache file and downgrade/block access

#### Story 6.4: License Status UI
**Background:** Users need to see their current license status, tier, and remaining conversions within the application.

**Tasks:**
- [ ] Add a "License" item to the `Help` menu that opens a `LicenseStatusDialog`
- [ ] `LicenseStatusDialog` displays: current tier name, conversions used / conversions limit for the current period, reset date, license key (masked except last 4 characters), validation status (Online / Offline - Grace Period / Expired), and a "Manage Subscription" link that opens `https://metavisage.io/dashboard`
- [ ] Add a small persistent tier indicator to the status bar (e.g. "Free", "Indie", "Pro") so users always know which tier they are on

### Acceptance Criteria
- [ ] First-launch license entry dialog appears when no cached JWT exists
- [ ] Entering a valid license key validates successfully and stores the JWT
- [ ] "Continue with Free Tier" button functions correctly for free accounts
- [ ] `POST /api/license/validate` is called on launch and every 24 hours
- [ ] All documented error codes produce appropriate user-facing messages
- [ ] The cached JWT is encrypted and stored in `%APPDATA%\MetaVisage\license.cache`
- [ ] App functions offline for up to 7 days using the cached JWT
- [ ] App blocks access and prompts reconnection when the cached JWT is expired and offline
- [ ] `403` errors clear the cache and block/downgrade access
- [ ] `Help > License` opens the `LicenseStatusDialog` with correct information
- [ ] Tier badge appears in the status bar

---

## Sprint 7: Licensing Infrastructure — Feature Gating & Conversion Tracking

**Duration:** 2 weeks  
**Goals:** Enforce all tier-based feature flags throughout the application, implement conversion reporting, and provide upgrade prompts when users hit tier limits.

### Stories

#### Story 7.1: Feature Flag Enforcement
**Background:** The license JWT payload contains a `feature_flags` object that defines what each tier can access. These flags must be enforced throughout the application.

**Feature Flag Reference (from `DESKTOP_APP_INTEGRATION.md`):**

| Flag | Free | Indie | Pro |
|---|---|---|---|
| `conversions_limit` | 3 | 20 | 60 |
| `custom_head_upload` | false | true | true |
| `texture_transfer` | none | albedo | albedo_normal |
| `morphing_parameters` | presets | full | full |
| `masking` | false | true | true |
| `touchup_tools` | basic_smooth | basic | full |
| `batch_processing` | false | false | false |

**Tasks:**
- [ ] Expose all feature flag values from `LicenseManager` via typed getters (e.g. `LicenseManager::CanUploadCustomHead()`, `LicenseManager::GetTextureTransferMode()`, `LicenseManager::GetTouchUpToolset()`)
- [ ] `custom_head_upload = false` (Free): Disable the "Load Morph Mesh" and "Load Target Mesh" file buttons in Stage 1; show an upgrade prompt explaining this feature requires Indie or higher
- [ ] `texture_transfer = none` (Free): Hide the texture upload section in Stage 1 entirely
- [ ] `texture_transfer = albedo` (Indie): Allow Albedo texture upload; hide and disable additional texture slot uploads beyond one (Albedo only); show an upgrade prompt for additional slots
- [ ] `texture_transfer = albedo_normal` (Pro): Allow unlimited texture uploads (existing behavior)
- [ ] `morphing_parameters = presets` (Free): Disable the General and Advanced collapsible parameter sections; only preset cards are selectable and parameters cannot be individually adjusted; show an upgrade prompt on the section headers
- [ ] `morphing_parameters = full` (Indie/Pro): Full parameter access (existing behavior)
- [ ] `masking = false` (Free): Hide/disable the Vertex Masking section in Stage 3; show an upgrade prompt
- [ ] `touchup_tools = basic_smooth` (Free): Only Smooth brush is available in Stage 4; Grab, Push/Pull, and Inflate brushes are visible but disabled with an upgrade prompt on hover
- [ ] `touchup_tools = basic` (Indie): Smooth and Grab brushes available; Push/Pull and Inflate disabled with upgrade prompt
- [ ] `touchup_tools = full` (Pro): All brushes available (existing behavior)

#### Story 7.2: Conversion Tracking
**Background:** Each completed wrap (successful morph in Stage 3 that the user accepts) counts as one conversion against the user's monthly limit. The app must report each conversion to the API.

**Tasks:**
- [ ] Implement `LicenseManager::ReportConversion()` which calls `POST /api/license/conversion` with `Authorization: Bearer <license_key>`
- [ ] Call `ReportConversion()` when the user clicks "Accept Result" in Stage 3 — only after the morph has completed successfully
- [ ] On success (HTTP 200): update the in-memory `conversions_used` counter and refresh the `LicenseStatusDialog` if open
- [ ] On `403 limit_reached`: do not allow the user to accept the result; show a dialog explaining they have reached their monthly limit, when it resets (`conversions_reset_at`), and a link to upgrade
- [ ] Handle `429 rate_limited` with `Retry-After` backoff; handle `500` with exponential backoff and surface a non-blocking notification if the retry ultimately fails
- [ ] If the device is offline when accepting: queue the conversion report and send it on next successful network connection; allow the user to proceed (do not block acceptance due to network error)

#### Story 7.3: Upgrade Prompts
**Background:** Free and Indie users should see clear, non-intrusive upgrade prompts when they encounter a feature gated to a higher tier.

**Tasks:**
- [ ] Implement a reusable `UpgradePromptWidget` — a small inline panel or tooltip-style callout that displays: a brief description of the locked feature, the tier required to unlock it, and a "Upgrade at metavisage.io" button that opens the pricing page
- [ ] Use `UpgradePromptWidget` consistently at all feature gate points identified in Story 7.1
- [ ] Upgrade prompts should be informative and non-alarming — no modal dialogs for passive feature gates; use inline disabled states with a visible indicator

#### Story 7.4: Conversion Limit Warning
**Background:** Users approaching their monthly conversion limit should be warned before they hit it.

**Tasks:**
- [ ] After each successful conversion report, check if `conversions_used >= conversions_limit - 2` (within 2 conversions of the limit)
- [ ] If so, show a non-blocking status bar notification: "You have X conversions remaining this month. Resets on [date]."
- [ ] When `conversions_used == conversions_limit`, show the same notification with "0 conversions remaining" and an upgrade link
- [ ] Display current conversion usage (`conversions_used / conversions_limit`) in the `LicenseStatusDialog`

### Acceptance Criteria
- [ ] Free tier users cannot load custom meshes; an upgrade prompt is shown
- [ ] Free tier users cannot access individual morphing parameters; preset cards are the only option
- [ ] Free tier users cannot use Vertex Masking
- [ ] Free tier users can only use the Smooth brush in Stage 4
- [ ] Indie tier users can use Smooth and Grab brushes; Push/Pull and Inflate show upgrade prompts
- [ ] Pro tier users have access to all features
- [ ] Each accepted morph reports to `POST /api/license/conversion`
- [ ] Accepting a morph when the limit is reached is blocked with a clear dialog
- [ ] Offline acceptance queues the conversion report for later delivery
- [ ] Upgrade prompts appear inline at all gated feature locations
- [ ] A warning appears in the status bar when within 2 conversions of the monthly limit

---

## Sprint 8: Distribution, Auto-Update & Release

**Duration:** 2 weeks  
**Goals:** Package MetaVisage as a properly installable Windows application with Inno Setup, integrate WinSparkle for automatic update delivery, perform final end-to-end testing across all tiers, and cut the v2.0.0 release.

### Stories

#### Story 8.1: Inno Setup Installer
**Background:** v1.0.0 had no installer — users received raw application files. An installer is required for professional distribution and for WinSparkle to function correctly.

**Tasks:**
- [ ] Install and configure Inno Setup 6.x
- [ ] Write the Inno Setup script (`installer/metavisage_setup.iss`):
  - Application name, version, publisher metadata
  - Install directory: `%PROGRAMFILES%\MetaVisage`
  - Bundle all Qt DLLs, Assimp DLL, and application assets (shaders, models, preset thumbnails, brush previews)
  - Create Start Menu shortcuts
  - Create Desktop shortcut (optional, user-selectable during install)
  - Register application in Windows `Add or Remove Programs`
  - Uninstaller that cleanly removes all installed files but does not delete `%APPDATA%\MetaVisage` (preserving projects, license cache, and user presets)
- [ ] Automate installer build via a PowerShell script (`package-release.ps1`) that runs the Inno Setup compiler after the MSVC Release build
- [ ] Test installer on a clean Windows 10 and Windows 11 VM with no Qt or MSVC redistributables pre-installed

#### Story 8.2: WinSparkle Auto-Update
**Background:** There is currently no mechanism for users to receive application updates. WinSparkle provides a standard Sparkle-compatible update check and download flow for Windows applications.

**Tasks:**
- [ ] Add WinSparkle to the project (via vcpkg or manual DLL distribution)
- [ ] Configure CMakeLists.txt to link and deploy `WinSparkle.dll`
- [ ] Initialize WinSparkle at application startup with the appcast URL: `https://metavisage.io/appcast.xml`
- [ ] Set the WinSparkle check interval to once per day
- [ ] Hook WinSparkle's update check into the `Help > Check for Updates` menu item (add this item)
- [ ] Allow WinSparkle's automatic background check to also trigger (silent check on launch, user-visible prompt only when an update is found)
- [ ] Create the initial `appcast.xml` template that will be updated as part of the release process for each future version — document the release process for publishing updates
- [ ] Verify the update download and install flow works end-to-end on a test machine

#### Story 8.3: End-to-End Testing Across Tiers
**Tasks:**
- [ ] Test the complete workflow (Stage 1 → Stage 4 → Export) using a Free tier account — verify all feature gates block correctly and upgrade prompts appear
- [ ] Test the complete workflow using an Indie tier account — verify Indie features work and Pro-only features are blocked
- [ ] Test the complete workflow using a Pro tier account — verify all features are accessible
- [ ] Test offline operation: disconnect from the internet after a valid validation, verify the app functions for the grace period, verify blocking occurs after the JWT expires
- [ ] Test conversion limit enforcement: exhaust the monthly conversion limit on a Free account, verify the block dialog appears on the next acceptance attempt
- [ ] Test the Stage 2 save/load regression (from Sprint 1)
- [ ] Test the installer on a clean Windows 10 VM
- [ ] Test the installer on a clean Windows 11 VM
- [ ] Verify that a v1.0.0 project file opens correctly in v2.0.0 without errors or data loss
- [ ] Test the WinSparkle update check against the staging appcast

#### Story 8.4: Release Preparation
**Tasks:**
- [ ] Update `README.md` with v2.0.0 feature list, new installation instructions (installer-based), and licensing information
- [ ] Write `RELEASE_NOTES_v2.0.0.md` documenting all changes from v1.0.0
- [ ] Update `CONTRIBUTING.md` to note the closed-source status and the process for external contributors (if any)
- [ ] Build final MSVC Release configuration with zero warnings
- [ ] Run `package-release.ps1` to produce the signed installer `MetaVisage_Setup_2.0.0.exe`
- [ ] Create the GitHub release tag `v2.0.0` and upload the installer as the release asset
- [ ] Publish `appcast.xml` to `https://metavisage.io/appcast.xml` pointing to the v2.0.0 release

### Acceptance Criteria
- [ ] `MetaVisage_Setup_2.0.0.exe` installs the application cleanly on Windows 10 and Windows 11
- [ ] Installed application launches, validates license, and completes the full workflow
- [ ] Uninstaller removes the application and does not delete user data in `%APPDATA%\MetaVisage`
- [ ] `Help > Check for Updates` triggers a WinSparkle update check
- [ ] WinSparkle performs a background check on launch
- [ ] Free, Indie, and Pro tier workflows each behave correctly per their feature flags
- [ ] Offline grace period allows 7 days of use with a cached JWT
- [ ] v1.0.0 project files open correctly in v2.0.0
- [ ] `RELEASE_NOTES_v2.0.0.md` is complete and accurate
- [ ] GitHub release `v2.0.0` is created with the installer attached
- [ ] `appcast.xml` is live and WinSparkle successfully detects the v2.0.0 release

---

## Notes

**Deferred to Post-v2.0:**
- Orientation gizmo click-to-snap-view interaction
- Stage 1 numeric transform spinboxes (editable)
- Batch processing (reserved for Studio tier, per PRD-WebPortal.md)
- macOS and Linux support
- Automated/ML-assisted point placement
- Annual billing and team accounts (handled by web portal in a future phase)

**Dependency Notes:**
- Sprint 6 and 7 depend on the web portal's license API (`/api/license/validate` and `/api/license/conversion`) being deployed to production before or concurrent with this work. Staging environment (`https://staging.metavisage.io`) should be used for integration testing throughout Sprints 6–7.
- Sprint 8 WinSparkle integration requires the `appcast.xml` endpoint to be live on `metavisage.io` before release testing can be completed.
- The `LICENSE_JWT_SECRET` shared secret must be agreed upon between the desktop and web teams before Sprint 6 begins and should be provided as a build-time constant via a secure mechanism (not committed to the repository).

**Risk Mitigation:**
- The "cottage cheese face" artifact (Sprint 5.2) has an open-ended investigation scope. If the root cause is not resolved within Sprint 5, apply a conservative parameter warning as a stopgap and schedule a dedicated algorithm spike in a future sprint.
- Feature flag enforcement (Sprint 7) must be tested against all three tiers before release; enlist web team support to provision test accounts at each tier for QA.