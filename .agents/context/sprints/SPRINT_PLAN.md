# Sprint Plan: MetaVisage v2.0

**Current direction (2026-10-15)**: [Sprint 8 - Standalone Open-Source Transition](sprint-08.md)
supersedes the commercial distribution/auto-update roadmap below. Sprints 1-7
remain historical records. The current application is MIT licensed and offline.

**Latest**: [Sprint 9 - Alignment Stage Usability Fixes](sprint-09.md), complete 2026-10-30.
**Next (planned 2026-10-29, pending approval)**:
[Sprint 10 - App Shell & Morph Stage Polish](sprint-10.md), from the same user feedback triage.

**Version:** 2.0  
**Date:** April 19, 2026  
**Total Duration:** 20 weeks (10 sprints @ 2 weeks each)  
**Historical roadmap license:** Proprietary (superseded; current license is MIT)

---

## Overview

This sprint plan covers the v2.0 release of MetaVisage. It addresses all v1.0.0 user feedback, introduces the licensing and subscription system required by the transition to a paid product, adds distribution infrastructure (installer and auto-updates), completes the remaining UX polish work across all four workflow stages, and migrates the application off Qt onto a self-contained stack of Dear ImGui, GLFW, and standard C++ libraries.

**Framework Migration Context:** MetaVisage v1.0.0 was built on Qt under an open-source model. With the shift to a commercial product, Qt requires a commercial license. Rather than incur that cost, the team has decided to migrate to Dear ImGui (UI rendering) and GLFW (window and OpenGL context management), paired with lightweight standard libraries to cover the remaining Qt responsibilities. Development is paused after Sprint 3 while this migration is completed via Sprints 3.5 and 3.6. Sprints 4–8 continue as originally planned, built on the new stack.

**Inputs:**
- v1.0.0 user feedback (Version_1_0_0_Feedback.md)
- Desktop App Integration Guide (DESKTOP_APP_INTEGRATION.md) — licensing API contract from the web team
- Desktop Auth Flow (DESKTOP_AUTH_FLOW.md) — browser-based OAuth auth flow specification
- PRD-WebPortal.md — tier feature matrix and feature flag definitions

**Sprint Structure:**
- Sprint 1: Critical Bug Fixes & Navigation Polish *(Complete)*
- Sprint 2: Viewport & Global UI Infrastructure *(Complete)*
- Sprint 3: Stage 1 — Transform Gizmos & Texture System *(Complete)*
- Sprint 3.5: Framework Migration — Platform Layer (Qt → GLFW + ImGui)
- Sprint 3.6: Framework Migration — UI Layer (Sidebars, Dialogs & System Services)
- Sprint 4: Stage 3 — Wrap Engine Redesign (Algorithm & Preset System)
- Sprint 5: Stage 3 — Advanced Parameters, Masking & Stage 4 Polish
- Sprint 6: Licensing Infrastructure — Core Integration
- Sprint 7: Licensing Infrastructure — Feature Gating & Tier Enforcement
- Sprint 8: [Standalone Open-Source Transition](sprint-08.md) (replaces Distribution, Auto-Update & Release)

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
- [x] Verify that all widgets (including the settings panel, which was reportedly already dark) render consistently
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

## Sprint 3.5: Framework Migration — Platform Layer

**Duration:** 2 weeks  
**Goals:** Remove Qt from the project entirely and replace the platform layer — window creation, OpenGL context management, the main event loop, and input handling — with GLFW and Dear ImGui. By the end of this sprint the application must compile, launch, render the 3D viewport, and respond to camera input. All existing rendering code (shaders, VBOs, MeshRenderer, PointRenderer, GridRenderer, etc.) is carried over unchanged; only the hosting layer changes.

**New Dependencies (all available via vcpkg):**
- `glfw3` — window, OpenGL context, and raw input
- `imgui[glfw-binding,opengl3-binding]` — UI rendering with GLFW and OpenGL3 backends
- `glad` or `glew` — OpenGL function loading (replacing Qt's built-in loader)

### Stories

#### Story 3.5.1: Dependency & Build System Overhaul
**Background:** CMakeLists.txt currently finds and links Qt6 components across the entire project. These must be replaced cleanly before any other migration work can proceed.

**Tasks:**
- [ ] Remove all `find_package(Qt6 ...)`, `qt_add_executable`, `target_link_libraries(...Qt6::...)` and Qt MOC/UIC/RCC invocations from `CMakeLists.txt`
- [ ] Add `find_package` calls for `glfw3`, `imgui`, and `glad` (or `glew`) via vcpkg toolchain
- [ ] Add `find_package` calls for `nlohmann_json`, `nativefiledialog-extended` (NFD), and `CURL` — these will be needed in Sprint 3.6 but should be confirmed as buildable now
- [ ] Confirm Eigen3, Assimp, and all deformation/sculpting source files still compile cleanly with no Qt headers in scope — these are the portions that transfer without change
- [ ] Update `build-msvc.ps1` to remove Qt deployment steps (`windeployqt`) and add any new DLL deployment steps required by GLFW/ImGui
- [ ] Verify a clean MSVC Release build with zero errors after dependency swap (linking will fail until the application entry point is rewritten in Story 3.5.2, but compilation of non-UI translation units should succeed)

#### Story 3.5.2: Application Entry Point & Main Loop
**Background:** Qt's `QApplication::exec()` owns the event loop. This is replaced with a standard GLFW `while (!glfwWindowShouldClose)` render loop with ImGui frame management.

**Tasks:**
- [ ] Replace `main.cpp` — remove `QApplication`, create a GLFW window (`glfwCreateWindow`) at 1920×1080 with an OpenGL 4.3 Core Profile context, and implement the main render loop:
  - `glfwPollEvents()`
  - `ImGui_ImplGlfw_NewFrame()` / `ImGui_ImplOpenGL3_NewFrame()` / `ImGui::NewFrame()`
  - Application update tick (stage logic, input processing)
  - 3D viewport render pass (existing OpenGL pipeline)
  - ImGui render pass (`ImGui::Render()` / `ImGui_ImplOpenGL3_RenderDrawData()`)
  - `glfwSwapBuffers()`
- [ ] Initialize Dear ImGui with `ImGui::CreateContext()`, `ImGui_ImplGlfw_InitForOpenGL()`, `ImGui_ImplOpenGL3_Init("#version 430")`, and apply the existing dark theme colours to `ImGui::GetStyle()`
- [ ] Implement `AppWindow` class to own the GLFW window handle, ImGui context, and top-level render orchestration — this is the direct replacement for Qt's `MainWindow`
- [ ] Enforce the always-dark background: set `glfwWindowHint(GLFW_DECORATED, GLFW_TRUE)` and clear the framebuffer to the existing dark background colour regardless of OS theme — ImGui renders entirely within the OpenGL context so OS light/dark mode has no effect
- [ ] Implement clean shutdown: `ImGui_ImplOpenGL3_Shutdown()`, `ImGui_ImplGlfw_Shutdown()`, `ImGui::DestroyContext()`, `glfwDestroyWindow()`, `glfwTerminate()`

#### Story 3.5.3: OpenGL Context & Viewport Rendering
**Background:** The existing `ViewportWidget` (a `QOpenGLWidget` subclass) managed its own OpenGL context. The rendering code inside it — `Renderer`, `MeshRenderer`, `PointRenderer`, `ShaderManager` — is entirely portable and does not need to change. Only the hosting mechanism changes.

**Tasks:**
- [ ] Remove `ViewportWidget` as a Qt widget — convert it to a plain `Viewport` class that receives the framebuffer region to render into (defined by an `ImVec2` position and size passed from the ImGui layout each frame)
- [ ] Use `ImGui::GetWindowDrawList()` and `ImGui::InvisibleButton()` with a matching size to create a mouse-interactive 3D viewport region within the ImGui window — route all mouse events from this region to the existing camera and sculpting input handlers
- [ ] Restore OpenGL state after the 3D render pass before handing back to ImGui (depth test, blend mode, VAO binding) — ImGui requires a clean GL state
- [ ] Implement dual-viewport layout for Stage 2 using two side-by-side `ImGui::InvisibleButton` regions within the same ImGui window, each with their own scissor rect and render pass — this replaces `ViewportContainer` and `QSplitter`
- [ ] Verify the grid, mesh rendering, point markers, brush cursor, orientation gizmo, and shading mode overlay all render correctly in the new hosting context

#### Story 3.5.4: Input Handling
**Background:** Qt's mouse and keyboard event system (`mousePressEvent`, `keyPressEvent`, etc.) is replaced with GLFW callbacks and ImGui's input polling.

**Tasks:**
- [ ] Register GLFW callbacks (`glfwSetMouseButtonCallback`, `glfwSetCursorPosCallback`, `glfwSetScrollCallback`, `glfwSetKeyCallback`) and forward events to the appropriate handler (camera controls, transform tools, sculpting, point placement) only when ImGui is not consuming the input (`!ImGui::GetIO().WantCaptureMouse`, `!ImGui::GetIO().WantCaptureKeyboard`)
- [ ] Re-implement all keyboard shortcuts (G/R/S transform modes, X/Y/Z axis constraints, Numpad camera views, `[`/`]` brush radius, `Delete` for points, `Escape` to cancel) using GLFW key codes in the main input dispatch
- [ ] Implement the `[ ]` brush radius shortcut and `Delete` key for point removal via the GLFW key callback
- [ ] Verify camera orbit (middle mouse), pan (Shift + middle mouse), and zoom (scroll wheel) all function correctly in the new input system

### Acceptance Criteria
- [ ] Project compiles with zero Qt dependencies — no Qt headers included anywhere, no Qt libraries linked
- [ ] Application launches and displays a GLFW window with the dark background
- [ ] Dear ImGui initializes and renders (confirm with a placeholder debug window)
- [ ] The 3D viewport renders the grid, and loaded meshes render correctly with all shading modes
- [ ] Camera orbit, pan, and zoom work via mouse input
- [ ] All keyboard shortcuts function correctly
- [ ] Dual-viewport layout renders correctly for Stage 2 (two side-by-side viewports, camera sync intact)
- [ ] `build-msvc.ps1` builds cleanly with the new dependency set
- [ ] No regressions in rendering quality compared to the Qt-hosted version

---

## Sprint 3.6: Framework Migration — UI Layer

**Duration:** 2 weeks  
**Goals:** Re-implement all user-facing UI in Dear ImGui — the menu bar, all four stage sidebar panels, all dialogs and modals — and replace Qt's system service APIs (networking, file dialogs, settings, single-instance detection, URL opening) with their lightweight standard equivalents. By the end of this sprint the application must be fully functional across all four stages with feature parity to the Qt version as it stood at the end of Sprint 3.

**New Dependencies (all available via vcpkg):**
- `nativefiledialog-extended` (NFD) — native Windows file open/save dialogs
- `curl` — HTTP networking (replaces `QNetworkAccessManager`, needed for Sprint 6)
- `stb` — `stb_image.h` for texture thumbnail decoding (header-only, likely already present)

### Stories

#### Story 3.6.1: ImGui Layout Architecture & Menu Bar
**Background:** The application layout — menu bar across the top, 3D viewport on the left, sidebar on the right — must be implemented as a set of ImGui windows docked to the main GLFW window. Dear ImGui's docking API (`ImGui::DockSpace`) is used to create a stable, resizable layout without floating panels.

**Tasks:**
- [ ] Enable ImGui docking (`ImGuiConfigFlags_DockingEnable`) and create a full-window dockspace as the root layout container
- [ ] Define three docked regions: `MenuBar` (top, fixed height), `Viewport` (centre-left, fills remaining space), `Sidebar` (right, fixed 320px width, scrollable)
- [ ] Implement the `ImGui::BeginMainMenuBar()` menu bar with all menus restored:
  - **File**: New Project, Open Project, Save Project (`Ctrl+S`), Save As (`Ctrl+Shift+S`), Export Mesh (Stage 4 only — hidden in other stages via `ImGui::BeginDisabled`), Exit
  - **Edit**: Undo (`Ctrl+Z`), Redo (`Ctrl+Shift+Z`)
  - **View**: Shading Mode submenu, Show/Hide Grid, camera view presets (Front/Right/Top/Perspective)
  - **Help**: Documentation, Account & License, Check for Updates, Keyboard Shortcuts, About
- [ ] Implement stage-sensitive menu item visibility — items not relevant to the current stage are hidden or disabled, matching the behaviour specified in Sprint 1.2
- [ ] Implement the status bar as a fixed-height ImGui window anchored to the bottom of the screen, displaying current tool, contextual hint text, and the tier badge from the license manager

#### Story 3.6.2: Stage Sidebar Panels
**Background:** All four stage sidebar panels must be re-implemented in ImGui, reproducing the full control set delivered in Sprints 1–3 and designed to accommodate the additions coming in Sprints 4–5.

**Tasks:**
- [ ] **Stage 1 — Alignment sidebar**: mesh load status (morph/target file paths and load buttons), transform mode indicator (Move/Rotate/Scale active tool), position and scale readouts (X/Y/Z), Reset Transform button, stage validation message, Next Stage button — all signals/slots replaced with direct state reads and writes on the `Project` singleton each frame
- [ ] **Stage 2 — Point Reference sidebar**: active viewport indicator, target/morph point count displays with colour-coded match status, scrollable point correspondence list (selectable rows), point size slider, symmetry enable checkbox and axis selector (X/Y/Z), Clear All Points button with inline confirmation, Next Stage button
- [ ] **Stage 3 — Morph/Wrap sidebar**: preset card row (horizontal scroll, 128×80px image cards with selection highlight, `+ Save Preset` card), Vertex Masking section (always visible), collapsible General and Advanced parameter sections (collapse state persisted in config), Process button, Cancel button, progress bar, preview mode dropdown, Accept Result / Re-process buttons, Reset to Defaults button, Next Stage button — full parameter hierarchy from Sprint 5 spec scaffolded here, enabled in Sprint 5
- [ ] **Stage 4 — Touch Up sidebar**: 2×2 visual brush card selector (brush preview images, active highlight), radius slider, strength slider, falloff combo, symmetry toggle and axis selector, target overlay checkbox, Finalize and Export button
- [ ] Implement the stage indicator (`1 / 4`, `2 / 4`, etc.) as a persistent element at the top of the sidebar across all stages
- [ ] Implement `ImGui::BeginDisabled()` / `ImGui::EndDisabled()` guards on all Next Stage buttons when stage validation conditions are not met, with an inline validation message below the button explaining what is missing

#### Story 3.6.3: Dialogs & Modals
**Background:** Qt's `QDialog`, `QMessageBox`, and `QFileDialog` classes must be replaced. File dialogs use NFD for native OS pickers; all other dialogs are implemented as ImGui modal popups.

**Tasks:**
- [ ] **File dialogs**: replace all `QFileDialog::getOpenFileName` / `getSaveFileName` calls with `NFD_OpenDialog` / `NFD_SaveDialog` from `nativefiledialog-extended` — applies to mesh loading (FBX/OBJ/GLTF filter), texture loading (PNG/JPG/TGA filter), project open/save (`.mmproj` filter), and export (FBX/OBJ filter)
- [ ] **Confirmation dialogs**: replace `QMessageBox::question` with `ImGui::OpenPopup` / `ImGui::BeginPopupModal` — applies to Clear All Points, Delete Point, Delete User Preset, Sign Out, and stage-change-with-unsaved-changes confirmations
- [ ] **Error dialogs**: replace `ErrorHelper` / `QMessageBox::critical` with a reusable `ShowErrorModal(title, message)` helper that opens a centred ImGui modal with an OK button — applies to mesh load failures, export failures, and validation errors
- [ ] **About dialog**: re-implement as an ImGui modal with application name, version, and MIT/proprietary notice
- [ ] **Keyboard Shortcuts dialog**: re-implement as a scrollable ImGui modal listing all shortcuts in a two-column table
- [ ] **Texture Preview modal**: display the full-resolution texture in an `ImGui::Image()` call within a resizable modal, with the texture name, filepath, and dimensions shown below — texture is uploaded to a GL texture and passed as an `ImTextureID`
- [ ] **Export dialog**: re-implement as an ImGui modal containing the format selector, triangulate checkbox, scale factor input, include materials toggle, and the Export button — triggers NFD save dialog to choose the output path

#### Story 3.6.4: System Service Replacements
**Background:** Qt provided several system-level services that have no ImGui equivalent and must be replaced with OS-level or standard library solutions.

**Tasks:**
- [ ] **Settings/Config persistence**: replace `QSettings` with a hand-written JSON config file at `%APPDATA%\MetaVisage\config.json` using `nlohmann::json` — store: last opened project path, sidebar collapse states, window size, recent projects list (max 10 entries). Read on launch, write on clean exit and on any setting change
- [ ] **Open URL**: replace `QDesktopServices::openUrl()` with `ShellExecuteEx(NULL, "open", url, ...)` — used for the Documentation link, Manage Subscription link, and the auth browser launch in Sprint 6
- [ ] **Recent projects list**: re-implement using the JSON config; populate the landing screen on launch by reading the `recentProjects` array; on project open/save, prepend to the list and trim to 10 entries; display as a scrollable ImGui list within the new project / landing modal
- [ ] **libcurl initialisation**: call `curl_global_init(CURL_GLOBAL_DEFAULT)` at application startup and `curl_global_cleanup()` at shutdown — this prepares libcurl for use in Sprint 6 without requiring further setup work at that point. Implement a reusable `HttpClient::Post(url, headers, body)` helper that wraps a synchronous `CURL*` easy handle and returns `{statusCode, responseBody}` — Sprint 6 will call this from a `std::thread` to avoid blocking the render loop
- [ ] **Single-instance detection & Named Pipe**: implement the single-instance pattern using a Win32 Named Pipe (`\\.\pipe\MetaVisageSingleInstance`) — on launch, attempt to create the pipe server; if creation fails (pipe already exists), open it as a client, write the command-line arguments (the `metavisage://` URI if present) to the pipe, and exit; the first instance reads from the pipe in a background `std::thread` and passes any received URI to the auth handler. This replaces `QLocalServer`/`QLocalSocket` and is required for the Sprint 6 deep link auth flow
- [ ] **Threading**: audit all remaining uses of `QThread`, `QMutex`, `QWaitCondition`, and `QAtomicInt` — replace with `std::thread`, `std::mutex`, `std::condition_variable`, and `std::atomic`. The `MorphWorker` thread in `MainWindow.cpp` and the texture thumbnail loader are the primary targets; `MeshDeformer` already uses `std::atomic<bool>` for cancellation and requires no change

#### Story 3.6.5: Shading Mode Viewport Overlay & Orientation Gizmo (ImGui Port)
**Background:** Sprints 2.1 and 2.2 implemented the shading mode overlay and orientation gizmo in Qt. These need to be re-implemented in the ImGui context — the underlying OpenGL rendering is unchanged, but the overlay widget hosting changes.

**Tasks:**
- [ ] Re-implement the shading mode overlay as a small `ImGui::SetNextWindowPos`-anchored window in the top-left of the viewport region, rendered after the 3D pass — contains a compact `ImGui::Combo` for Solid / Wireframe / Solid+Wireframe
- [ ] The orientation gizmo continues to be rendered via OpenGL (as implemented in Sprint 2.2) directly into the viewport framebuffer — no ImGui changes needed; verify it still renders correctly in the new viewport hosting

### Acceptance Criteria
- [ ] Application launches into the full ImGui layout with menu bar, 3D viewport, and sidebar
- [ ] All four stage sidebar panels are present and functionally complete
- [ ] Stage transitions work correctly — Next Stage buttons validate and advance the stage, sidebar panel switches
- [ ] All file dialogs (mesh load, texture load, project open/save, export) open native Windows pickers via NFD
- [ ] All confirmation and error dialogs function correctly as ImGui modals
- [ ] About, Keyboard Shortcuts, Texture Preview, and Export dialogs open and display correctly
- [ ] `config.json` is written on exit and read on launch; recent projects list, window size, and sidebar collapse states are restored
- [ ] `ShellExecuteEx` opens URLs correctly in the default browser
- [ ] Named Pipe single-instance detection works — a second launch forwards its arguments to the first instance and exits
- [ ] `HttpClient::Post()` helper compiles and returns correct results against a test endpoint
- [ ] libcurl initialises and shuts down cleanly with no memory leaks
- [ ] `MorphWorker` and texture loader threads use `std::thread` / `std::mutex` exclusively — no remaining `QThread` usage
- [ ] No Qt headers or libraries remain anywhere in the codebase or build system
- [ ] Full workflow (Stage 1 → Stage 4 → Export) completes without crashes or regressions compared to the Qt version at end of Sprint 3

---

## Sprint 3.8: UI/UX Redesign Implementation
 
**Duration:** 2 weeks  
**Goals:** Implement the full visual redesign delivered by the UI/UX team (`MetaVisage_Desktop_Application_Redesign.zip`) across every surface of the application — design tokens, typography, window chrome, the stage stepper, the left tool rail, all four stage inspector panels, all modals, the login screen, and the welcome screen. The goal is maximum fidelity to the design files within the constraints of Dear ImGui's rendering model. Where CSS effects have no direct ImGui equivalent (box-shadow, backdrop blur, gradient backgrounds), the closest practical approximation is used and noted below.
 
**Design System Reference:**
- **Colour palette**: `colors_and_type.css` defines all tokens. The full set must be translated to named `ImVec4` constants in a new `MvStyle.h` header and applied to `ImGui::GetStyle()` and `ImGui::GetStyleColorVec4()` at startup
- **Fonts**: Three families required — Saira Bold Italic (`Saira_800_italic.ttf`) for display headings, Inter Regular/Medium/SemiBold for body text, JetBrains Mono Regular/Medium for technical readouts and labels. All variants loaded via `ImGui::GetIO().Fonts->AddFontFromFileTTF()` and bundled in `assets/fonts/`
- **ImGui constraints**: No CSS `box-shadow`, `backdrop-filter: blur`, or gradient fills on arbitrary shapes. Approximate as: coloured borders for glow/shadow effects, solid dark overlay backgrounds for blur surfaces, `ImDrawList` gradient rectangles where gradients are structurally important (e.g. the login card top accent bar)
**One open question for the team:** The v1.0.0 feedback (Sprint 1.4) called for removing the FPS counter. The redesign files show "FPS 120" in the Stage 3 and Stage 4 viewport toolbars. Please confirm the desired behaviour before Sprint 3.8 begins — this sprint plan assumes the FPS counter is **not** re-introduced and the pill shows only the READY/processing status, matching the Sprint 1.4 decision. If the team wants to reverse that decision, this story should be updated accordingly.
 
### Stories
 
#### Story 3.8.1: Design Token System & Font Loading
**Background:** All colour values and spacing constants currently hardcoded in the post-migration ImGui style pass must be replaced with a unified token system that mirrors `colors_and_type.css` exactly, making future theme changes a single-file edit.
 
**Tasks:**
- [ ] Create `include/ui/MvStyle.h` defining all colour tokens as named `ImVec4` constants mirroring the CSS variable names: `MvBg0` through `MvBg4`, `MvBlue300` through `MvBlue900`, `MvFg1` through `MvFg4`, `MvLine1` through `MvLine4`, `MvSuccess`, `MvWarning`, `MvDanger`, and all semantic variants
- [ ] Create `MvStyle::Apply()` function that sets `ImGui::GetStyle()` colours, sizes, and rounding values: `WindowRounding = 6`, `FrameRounding = 4`, `PopupRounding = 8`, `ScrollbarRounding = 6`, `GrabRounding = 4`, `WindowBorderSize = 1`, `FrameBorderSize = 1` — all border and background colours mapped from the token set
- [ ] Call `MvStyle::Apply()` in `AppWindow` after `ImGui::CreateContext()` — this replaces the temporary dark theme applied in Sprint 3.5
- [ ] Bundle font files in `assets/fonts/`: `Saira-ExtraBoldItalic.ttf`, `Inter-Regular.ttf`, `Inter-Medium.ttf`, `Inter-SemiBold.ttf`, `JetBrainsMono-Regular.ttf`, `JetBrainsMono-Medium.ttf`. Source from Google Fonts (open licence, safe to bundle)
- [ ] Load all font variants into the ImGui font atlas at application startup with appropriate sizes: display (28px), heading (20px), body (14px/13px), mono-label (11px/10px). Store font pointers as globals in `MvStyle.h` (`MvFont::Display`, `MvFont::Body`, `MvFont::BodySm`, `MvFont::Mono`, `MvFont::MonoSm`)
- [ ] Add spacing scale constants to `MvStyle.h`: `MvSpacing::S1` (4px) through `MvSpacing::S6` (32px) — use these throughout the codebase instead of hardcoded pixel values
#### Story 3.8.2: Window Chrome — Title Bar, Menu Bar & Status Bar
**Background:** The redesign introduces a custom frameless-style title bar (`TitleBar` in `app.jsx`) showing the logo mark, filename, and stage breadcrumb. The menu bar gains an autosave indicator and a "UE 5.4 · LIVE LINK" status pill on the right. The status bar gains more structured mono readouts.
 
**Tasks:**
- [ ] **Title bar**: Replace the current plain `ImGui::BeginMainMenuBar` title with a custom-height (36px) top bar rendered using `ImGui::GetWindowDrawList()`. Draw: the MetaVisage logo mark image (18×18px, from `assets/metavisage-logo-mark.png` loaded as a GL texture), "MetaVisage" in `MvFont::Body` SemiBold at `MvFg1`, a `MvLine3` separator dot, the current filename in `MvFg2`, a `›` separator, and "Stage N / 4 · STAGE NAME" in `MvBlue400` mono uppercase. Right-align the three standard Windows caption buttons (minimize, maximize, close) — close button background turns `#E81123` on hover. Use `glfwSetWindowTitle` to keep the OS taskbar title in sync
- [ ] **Menu bar**: Implement as a 32px bar below the title bar. Left side: `File | Edit | View | Mesh | Help` menu items in `MvFg2` at 13px, highlight to `MvFg1` with `MvBg3` background on hover. Right side: autosave indicator ("Autosaved 12s ago" in `MvFg3` mono) and the LIVE LINK pill (green dot + "UE 5.4 · LIVE LINK" in `MvSuccess` mono, `rgba(63,214,158,0.06)` background, green border) — the Live Link indicator is display-only for now (always shows connected state); real Live Link integration is out of scope for v2.0
- [ ] **Status bar**: Redesign as a 28px bottom bar in `MvBg0` with `MvLine2` top border. Sections separated by `MvLine3` dot dividers: `● Ready` (status dot + state), `Stage 01 ALIGN` (stage in `MvBlue400` mono), `Source filename`, `Target MetaHuman`, then right-aligned: `FPS nnn` (removed per Sprint 1.4 decision — replace with `GPU model · n%` readout), and `Live link UE 5.4 ●` in `MvSuccess`
- [ ] Replace the `"Mesh"` menu label for the existing `"Tools"` entry (the design file uses `File | Edit | View | Mesh | Help` — rename to match)
#### Story 3.8.3: Stage Stepper
**Background:** The most visually prominent new element in the redesign. The stage stepper replaces the old compact stage indicator with a full-width 64px horizontal bar showing all four stages simultaneously, each with a numbered disc, status label, and stage name.
 
**Tasks:**
- [ ] Implement `StageStepper` as a 64px fixed-height `ImGui` window spanning the full application width, positioned below the menu bar
- [ ] For each of the four stages, render using `ImGui::GetWindowDrawList()`:
  - **Numbered disc** (36×36px, `border-radius` approximated with `ImDrawList::AddRectFilled` + `ImDrawList::AddRect` using `rounding=6`): filled `MvBlue500` with `MvBlue300` border when active; filled `rgba(63,214,158,0.12)` with green border when complete; filled `MvBg2` with `MvLine2` border when upcoming. Shows a checkmark icon when complete, mono "0N" number text otherwise
  - **Status label** (10px mono uppercase, 0.16em tracking): "IN PROGRESS" in `MvBlue500` when active, "COMPLETE" in `MvSuccess` when done, "UPCOMING" in `MvFg4` otherwise
  - **Stage name** (14px, SemiBold): `MvFg1` when active, `MvFg2` when done, `MvFg3` when upcoming
  - **Active underline**: 2px `MvBlue500` line along the bottom of the active step button
  - **Chevron separators** between steps in `MvFg4`
- [ ] The entire step button is clickable and calls `StageManager::SetStage(n)` — apply the same validation guards that currently gate the Next Stage button (cannot jump forward past an incomplete stage)
- [ ] Hover state: non-active steps get `MvBg2` background fill on hover
#### Story 3.8.4: Left Tool Rail
**Background:** The redesign adds a 56px-wide vertical tool rail on the left of the work area (visible in Stage 1, hidden in Stages 2–4 where it is not applicable). It contains Move/Rotate/Scale tool buttons with hotkey badges, symmetry and pivot snap toggles, and a help button.
 
**Tasks:**
- [ ] Implement `ToolRail` as a 56px-wide `ImGui` child window docked to the left of the work area in Stage 1 only — not rendered in Stages 2, 3, or 4
- [ ] Render a mono uppercase "XFORM" label at the top (9px, `MvFg4`, 0.18em tracking)
- [ ] Three tool buttons (40×40px each, `rounding=6`): Move (W), Rotate (E), Scale (R). Active button: `MvBlue500` fill, dark icon color, glow border. Inactive: transparent fill, `MvFg2` icon. Each button has a hotkey badge in the bottom-right corner (8px mono, `MvBg1` background with `MvLine2` border when inactive; hidden when active)
- [ ] Thin `MvLine2` horizontal divider below the transform tools
- [ ] Two secondary buttons: symmetry toggle (X-axis mirror icon) and pivot snap toggle — these are visual scaffolding for now; wire to existing symmetry state from Stage 1 where available
- [ ] Help button anchored to the bottom of the rail (`IconHelp`)
- [ ] Tool rail button clicks update the active transform mode — this replaces the G/R/S keyboard shortcut as the primary visual affordance (keyboard shortcuts remain functional)
- [ ] The work area `gridTemplateColumns` updates from `"1fr 360px"` to `"56px 1fr 360px"` for Stage 1 only
#### Story 3.8.5: Viewport Toolbar & HUD Overlays
**Background:** The redesign moves shading controls from the viewport overlay (Sprint 2.1) into a centred floating toolbar, adds visibility toggles and a hotkey hint strip, replaces the plain overlay style with a frosted dark background, and adds a bottom-left instruction HUD.
 
**Tasks:**
- [ ] **Top-centre toolbar**: Render as a `ImGui::SetNextWindowPos`-anchored window at the top-centre of the viewport. Dark background (`rgba(11,22,34,0.85)` → `MvBg1` at 85% alpha), 1px `MvLine2` border, `rounding=6`. Contents left-to-right: perspective mode label ("PERSP" in `MvBlue400` mono), `MvLine2` separator, Solid/Wireframe/Textured shading toggle buttons (28×26px each, active state `MvBlue500` fill), separator, Source/Target visibility eye-icon toggles, separator, hotkey hint strip ("F · ⇧ axis" in `MvFg3` mono 11px). For Stage 3 specifically, add Source/Target/Deformed view mode buttons and a READY/PROCESSING status indicator in place of the visibility toggles
- [ ] **Top-right meta pills**: Two small pills anchored top-right of the viewport: a LIVE status indicator pill and, for Stage 4 only, a strength percentage pill. Rendered as `ImDrawList` rounded rectangles with the badge styling from the design system
- [ ] **Bottom-left instruction HUD**: Render a 320px-wide overlay box anchored to the bottom-left of the viewport (`MvBg1` at 88% alpha, `MvLine2` border, `rounding=6`). Contains: a mono stage label ("/ Stage N · STAGE NAME"), a contextual instruction line, and a hotkey reference strip of `Kbd` badges. Content is stage-specific and updates as the active stage changes. This is the existing status bar hint concept elevated to a proper in-viewport HUD
- [ ] The existing shading mode overlay (Sprint 2.1 top-left dropdown) is removed — shading mode is now controlled exclusively via the top-centre toolbar
#### Story 3.8.6: Inspector Panel — Shared Primitives
**Background:** Every stage's right sidebar uses the same set of primitive components defined in `app.jsx`. These must be implemented as reusable ImGui helper functions before the per-stage panels are built.
 
**Tasks:**
- [ ] **Stage header block**: `MvDrawStageHeader(eyebrow, title, accentWord, description)` — renders the mono eyebrow ("STAGE 01 / 04" in `MvBlue400`), the bold italic display heading with one word or syllable in `MvBlue500` (e.g. "Align**ment**"), and a 13px description line in `MvFg2`
- [ ] **Section label**: `MvDrawSectionLabel(title)` — 10px mono uppercase, `MvFg3`, 0.16em tracking, with a 6×6px `MvBlue500` square dot prefix. Renders a hairline `MvLine2` rule above it to visually group sections
- [ ] **File slot**: `MvDrawFileSlot(filename, placeholder, isLoaded, isPrimary)` — renders the dark `MvBg1` container with a folder icon, filename in `MvFontMono` (or placeholder in `MvFg3`), file type + size line below when loaded, and an Import button beneath (primary blue style if `isPrimary`, ghost otherwise)
- [ ] **Coloured axis numeric input**: `MvDrawAxisInput(axis, value, axisColor, &outValue)` — renders the drag-to-edit numeric field with the axis letter (X=`#FF6B6B`, Y=`#3FD69E`, Z=`#3DA5F5`) in the left corner, value right-aligned in `MvFontMono`, drag behaviour via `ImGui::DragFloat` with zero label width
- [ ] **Transform group**: `MvDrawTransformGroup(label, unit, values[3], uniform, onChange)` — renders a label+unit row followed by three `MvDrawAxisInput` fields in a 3-column grid
- [ ] **Validation row**: `MvDrawValidationRow(label, value, status)` — renders a 14px circle badge (✓ green / ! amber / × red), label in `MvFg2`, and value in `MvFontMono` right-aligned
- [ ] **Inspector footer**: `MvDrawInspectorFooter(stepLabel, onPrevious, onContinue, continueLabel)` — renders the 3-column Previous / step counter / Continue(or Accept/Finish) footer with `MvBg1` background and `MvLine2` top border. Continue button uses primary blue style in Stages 1–3, green `MvSuccess` fill for Stage 4's Finish button
#### Story 3.8.7: Stage 1 Inspector Panel
**Tasks:**
- [ ] Apply `MvDrawStageHeader("STAGE 01 / 04", "Alignment", "ment", "Position the target mesh to roughly align with the source mesh.")` at the top
- [ ] **Source mesh section**: `MvDrawSectionLabel("Source mesh")`, hint text, `MvDrawFileSlot` for the morph mesh (primary button style)
- [ ] **Target mesh section**: `MvDrawSectionLabel("Target mesh")`, `MvDrawFileSlot` for the target mesh (ghost button), "Use Default Head" ghost button below
- [ ] **Target textures section**: Scrollable list of texture slots using the existing dynamic texture model from Sprint 3.3. Each row: 48×48px thumbnail (`ImGui::Image`), filename in mono, remove button. "Add Texture" button at the bottom
- [ ] **Target transform section**: Three `MvDrawTransformGroup` calls for Position (cm), Rotation (°), Scale (× uniform). "Reset" and "Auto-align" ghost buttons in a 2-column grid below — Auto-align is currently a stub that resets to a neutral near-zero transform
- [ ] **Validation section**: Four `MvDrawValidationRow` entries: Source loaded (vertex count), Target loaded (MetaHuman version), Bounds intersect (% overlap), Pivot offset (quality hint). These read from the `Project` data model each frame
- [ ] `MvDrawInspectorFooter("STEP 01 OF 04", ...)` — Previous is disabled on Step 1; Continue enabled only when both meshes are loaded
#### Story 3.8.8: Stage 2 Inspector Panel & Viewport Bar
**Background:** Stage 2 has a unique source/target info bar above the dual viewports (not part of the sidebar) in addition to its inspector panel.
 
**Tasks:**
- [ ] **Viewport info bar**: Render a 36px bar spanning both viewports (between the stepper and the viewports). Left side: SOURCE pill badge + filename + vertex count + point count in mono. Right side: TARGET pill badge + filename + vertex count + point count. Separated by a centred point-count indicator showing "N pts". Background `MvBg1`, `MvLine2` bottom border
- [ ] **Inspector panel**: `MvDrawStageHeader("STAGE 02 / 04", "Point reference", "reference", "Place matching landmarks on each mesh...")`
- [ ] **Stats row**: Three equal-width stat cards side by side: PAIRED (count in `MvBlue500`), SOURCE ONLY (count in `MvWarning`), TARGET ONLY (count in `MvWarning` or `MvSuccess` when 0). Each card is a `MvBg1` box with a `MvLine2` border and mono uppercase label
- [ ] **Placement tools section**: `MvDrawSectionLabel("Placement tools")`. Two ghost buttons side-by-side: "Auto-detect" (stub, disabled with tooltip "Coming in a future update") and "Load preset" (stub, same). These are scaffolded here per the design file; functional implementation is not in scope for v2.0
- [ ] The existing scrollable point list, symmetry controls, point size slider, and Clear All button from Sprint 3.6 are retained; restyle to match the design token system
- [ ] `MvDrawInspectorFooter("STEP 02 OF 04", ...)` — Continue enabled only when paired point count > 0 and SOURCE ONLY + TARGET ONLY are both 0
#### Story 3.8.9: Stage 3 Inspector Panel & Viewport Stats
**Background:** Stage 3 has a results stats panel in the bottom-left of the viewport and a preset card row at the top of the inspector.**
 
**Tasks:**
- [ ] **Viewport result stats panel**: After processing completes, render a bottom-left overlay panel in the viewport (same frosted style as the HUD). Contains a `/ RESULT` header with a `● CONVERGED` / `● PROCESSING` / `● FAILED` status indicator, and four mono data rows: Max disp. (mm), Avg disp. (mm), Iterations, Elapsed (s). Values sourced from `DeformationResult`. Hidden until a process attempt has been made
- [ ] **Viewport toolbar additions for Stage 3**: Extend the top-centre toolbar with Source / Target / Deformed view mode toggle buttons (replacing the visibility toggles used in other stages). Active mode uses the primary button style
- [ ] **Preset card row**: Horizontal scrollable area at the top of the inspector (below the stage header). Each card is ~140×72px: a `MvBg1` background with `MvLine2` border (active: `MvBlue500` border + `rgba(61,165,245,0.08)` fill), a `PresetIcon` SVG (32×32px, rendered via `ImDrawList` line primitives matching the design file icons — tight-wrap radial lines for Quick Wrap, loose flowing lines for Fantasy Wrap, star for Custom), preset name in 14px SemiBold, and a sub-label in 11px `MvFg3`. The `+ Save preset` card uses a dashed `MvLine3` border and a `+` icon. Clicking a preset card applies its parameters to the parameter controls — this is the same preset system from Sprint 4/5, now with the correct visual treatment
- [ ] **Inspector parameter sections**: The collapsible General and Advanced sections from Sprint 5.1 are retained and restyled using `MvDrawSectionLabel` and the design token colours. Collapsible section headers use `ImGui::CollapsingHeader` with a custom `MvLine2` background fill
- [ ] `MvDrawInspectorFooter("STEP 03 OF 04", ...)` with "Accept" as the continue label — enabled only after a successful process result is available
#### Story 3.8.10: Stage 4 Inspector Panel & Viewport Stats
**Tasks:**
- [ ] **Viewport top bar for Stage 4**: A 36px bar above the viewport (below the stepper): left side has SOLID shading mode label + "Debug HUD" ghost button; right side shows a `● COMPLETE` / `● PROCESSING` status pill and a "N% strength" indicator
- [ ] **Viewport mesh stats panel**: Bottom-left overlay panel with `/ MESH` header and a `● READY TO EXPORT` / `● PROCESSING` status indicator. Three data rows: Vertices (count), Triangles (count), UV maps (count). Sourced from the current deformed mesh
- [ ] **Brush selector**: 2×2 grid of brush cards (full width of inspector, two per row). Each card: `MvBg1` background, `MvLine2` border (active: `MvBlue500` border + blue fill tint), a `BrushIcon` SVG drawn via `ImDrawList` primitives (smooth wave line, grab hand outline, push/pull arrows, inflate radial lines), brush name in 14px SemiBold, sub-label in 11px `MvFg3`. The four brushes are Smooth, Grab, Push/Pull, Inflate — active brush has the selected card highlighted
- [ ] **Radius and Strength sliders**: Rendered using `ImGui::SliderFloat` with a custom draw override using `ImDrawList` to match the design's gradient track (`MvBlue600` → `MvBlue400`) and circular thumb with `MvBlue500` border
- [ ] **Export mesh button**: Full-width primary green button (`MvSuccess` fill) labelled "Export Mesh…" at the bottom of the inspector above the footer. Opens the export modal on click
- [ ] `MvDrawInspectorFooter("STEP 04 OF 04", ...)` with a green "Finish" button style — triggers the finalize and export flow

#### Story 3.8.11: Welcome Modal Redesign
**Background:** The welcome modal from Sprint 3.6 must be updated to match the redesign in `welcome.jsx` and the preview screenshot.
 
**Tasks:**
- [ ] Render as a centred `ImGui::BeginPopupModal` of 560px width with a dark scrim overlay behind it (`MvBg0` at 55% alpha drawn via `ImGui::GetBackgroundDrawList()`)
- [ ] **Header section**: `MvBg2` background with a `MvLine2` bottom border. Left: 56×56px app mark icon (gradient blue square with the head SVG icon, approximated with a solid `MvBlue500` fill rounded rect + the logo mark image). Right: version eyebrow ("V 2.0.0 · EARLY ACCESS" in `MvBlue400` mono), "Welcome to **MetaVisage**" heading (Saira Bold Italic, "MetaVisage" in `MvBlue400`), and tagline in `MvFg2`. Close `×` button top-right
- [ ] **Actions row**: 2-column grid of `BigAction` buttons. New Project (primary blue border + fill tint, Import icon) and Open Project... (ghost, Folder icon). Each button: icon in a 36×36 rounded container, label in 14px SemiBold, sub-label in 11px `MvFg3`, right-arrow chevron
- [ ] **Recent projects list**: Mono uppercase "RECENT PROJECTS" section label + file count. Scrollable list in a `MvBg1` `MvLine2`-bordered container. Each row: 28×28 file icon in `MvBg2`, filename in `MvFontMono` 12px, "N hours ago · N MB" in 11px `MvFg3`, right chevron. Hover state: `rgba(61,165,245,0.06)` background tint on the row
- [ ] **Footer**: "Show this on startup" checkbox + Close ghost button

#### Story 3.8.12: Login Screen Redesign
**Background:** The login screen from Sprint 6.1 must match the redesign in `login.jsx`. It has two states — idle (Sign In button) and waiting (browser open, spinner active).
 
**Tasks:**
- [ ] Render as a full-screen `ImGui` window covering the entire application surface (`MvBg0` background with the fine blueprint grid drawn via `ImGui::GetWindowDrawList()` repeated pattern)
- [ ] **Corner bracket decorations**: Four decorative L-shaped corner marks drawn via `ImDrawList::AddLine` in `MvLine3` at the four corners of the content area
- [ ] **Auth card** (480px wide, centred): `MvBg2` background, `MvLine3` border, `rounding=8`. Top accent bar: 2px gradient line (`ImDrawList::AddRectFilledMultiColor` from transparent → `MvBlue500` → `MvBlue300` → `MvBlue500` → transparent)
- [ ] **Card hero**: 64×64px app mark (same as welcome modal but larger), version + state eyebrow in `MvBlue400` mono, display heading ("Sign in to **MetaVisage**" or "Awaiting **browser**"), description text in `MvFg2`. When waiting, a spinning animation ring is drawn around the app mark via `ImDrawList::AddArc` with a rotating offset updated each frame via the render tick
- [ ] **Step divider**: Mono label "STEP 01 / 02 — AUTHENTICATION" or "STEP 02 / 02 — BROWSER HANDOFF" with `MvLine2` rules either side
- [ ] **Idle state body**: Full-width primary blue "Sign in with browser" button (48px height). Three numbered step explanation rows (01/02/03) with square `MvBg1` number badges and explanation text in `MvFg2` 13px
- [ ] **Waiting state body**: A status block showing the Named Pipe listener address, a countdown timer ("Will time out in M:SS"), and a green "LIVE" badge. Below it: a "Callback URI" display with the `metavisage://` URI in mono and a Copy button. Below that: an amber warning box ("Browser didn't open? Open link manually or retry")
- [ ] **Card footer**: Mono security label ("SSO · OAuth 2.1 · PKCE") on the left with a lock icon; Cancel (waiting) or "Use offline" (idle) ghost button on the right
- [ ] **Bottom metadata strip**: Four mono items centred at the bottom of the screen: BUILD version, NODE identifier, HELP link, PRIVACY link

### Acceptance Criteria
- [ ] All CSS colour tokens from `colors_and_type.css` are present in `MvStyle.h` and applied to the ImGui style
- [ ] Saira Bold Italic, Inter, and JetBrains Mono fonts load and render correctly at all required sizes
- [ ] Custom title bar renders the logo, filename, and stage breadcrumb; caption buttons function; close button turns red on hover
- [ ] Menu bar shows `File | Edit | View | Mesh | Help` with autosave indicator and Live Link pill on the right
- [ ] Status bar renders in the new structured mono layout with all data fields
- [ ] Stage stepper renders all four stages with correct active/complete/upcoming states, disc icons, and underline indicator; clicking a completed stage navigates back
- [ ] Left tool rail appears in Stage 1 only with Move/Rotate/Scale buttons, hotkey badges, and correct active state
- [ ] Viewport top-centre toolbar shows correct controls per stage (shading in Stages 1/2/4; view mode in Stage 3)
- [ ] Bottom-left instruction HUD shows stage-appropriate instruction text and hotkey badges
- [ ] All six inspector panel primitives (`MvDrawStageHeader`, `MvDrawSectionLabel`, `MvDrawFileSlot`, `MvDrawAxisInput`, `MvDrawTransformGroup`, `MvDrawValidationRow`, `MvDrawInspectorFooter`) render correctly and are used consistently across all stage panels
- [ ] Stage 2 viewport info bar shows source/target filename, vertex counts, and point counts
- [ ] Stage 2 PAIRED / SOURCE ONLY / TARGET ONLY stat cards update in real time as points are placed
- [ ] Stage 3 result stats panel appears after processing with correct deformation statistics
- [ ] Stage 3 preset cards render with icons and the correct active border highlight
- [ ] Stage 4 brush selector renders as a 2×2 grid with icons, labels, and active highlight
- [ ] Stage 4 mesh stats panel shows vertex/triangle/UV counts
- [ ] Welcome modal matches the redesign with scrollable recent projects, action cards, and footer checkbox
- [ ] Login screen renders correctly in both idle and waiting states; spinner animates; countdown timer ticks
- [ ] No Qt style or default Dear ImGui grey theme is visible anywhere in the application
- [ ] Full workflow (Stage 1 → Stage 4 → Export) completes without crashes

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
**Goals:** Implement the browser-based OAuth-style authentication flow using the `metavisage://` custom URL scheme, as documented in `DESKTOP_AUTH_FLOW.md`. Users open the app, are directed to the web portal to sign in, and the portal redirects back to the desktop app automatically — no manual license key copying required. This sprint also covers session storage, the offline JWT cache, and the in-app license status UI.

### Stories

#### Story 6.1: LicenseManager Foundation & Auth State
**Background:** All license and session operations need a central owner before any auth flow can be wired up.

**Tasks:**
- [ ] Add a `LicenseManager` singleton class responsible for: session token storage, JWT caching, auth flow initiation, token refresh, feature flag access, and conversion tracking
- [ ] Define the session state machine: `Unauthenticated` → `Authenticating` (browser open, waiting for deep link) → `Authenticated` → `Expired` / `Inactive`
- [ ] On app launch, check for a valid cached session (Story 6.3): if found and within grace period, transition to `Authenticated` and proceed; if not found, transition to `Unauthenticated` and show the sign-in prompt
- [ ] Add a sign-in prompt screen (shown in place of the main application when unauthenticated): MetaVisage branding, a "Sign In / Create Account" primary button, and a brief explanation of why sign-in is required — no manual key input field
- [ ] Configure the API base URL via a compile-time constant (`METAVISAGE_API_BASE_URL`), with staging (`https://staging.metavisage.io`) and local dev (`http://localhost:3000`) values available via CMake build flags

#### Story 6.2: Browser Launch & Custom URL Scheme Interception
**Background:** The auth flow begins by opening the system browser to the web portal and ends when the portal redirects back to the desktop app via the `metavisage://` custom URL scheme. The desktop side must generate the state parameter, open the browser, and then intercept the deep link URI when it arrives.

**Tasks:**
- [ ] Implement `LicenseManager::BeginAuthFlow()`:
  - Generate a cryptographically random 32-character state string using `QRandomGenerator::global()->generate()` (or OS entropy via `BCryptGenRandom` for stronger entropy)
  - Store the state in memory for later validation
  - Open `https://metavisage.io/desktop-auth/init?state={state}` via `QDesktopServices::openUrl()`
  - Transition auth state to `Authenticating`; show a waiting UI ("Waiting for sign-in in your browser... Cancel" with a cancel button that aborts the flow)
- [ ] Handle the `metavisage://` deep link URI on Windows — the OS launches the application with the URI as a command-line argument (`argv[1]`) when the scheme is triggered. Parse `argv` at startup and detect if a `metavisage://auth?...` URI is present
- [ ] Handle the **already-running** case: if the app is already open when the deep link fires, Windows will launch a second instance. The second instance must forward the URI to the first via `QLocalServer`/`QLocalSocket` (single-instance pattern) and then exit. The first instance receives the URI via the local socket and continues the auth flow
- [ ] Implement `LicenseManager::HandleDeepLink(const QString& uri)`: parse `code` and `state` query parameters from the `metavisage://auth?code=...&state=...` URI
- [ ] Validate that the `state` parameter in the deep link exactly matches the one generated in `BeginAuthFlow()` — if they do not match, abort the flow, show an error ("Authentication failed: security check failed. Please try again."), and return to the sign-in prompt
- [ ] On successful state validation, proceed to the code exchange (Story 6.3)
- [ ] Handle timeout: if no deep link is received within 5 minutes of opening the browser, cancel the flow automatically and return to the sign-in prompt with a message ("Sign-in timed out. Please try again.")

#### Story 6.3: Code Exchange & Session Storage
**Background:** After intercepting the deep link and validating the state, the desktop app exchanges the short-lived code for the actual session tokens and license JWT via `POST /api/desktop/exchange`.

**Tasks:**
- [ ] Implement `LicenseManager::ExchangeCode(const QString& code)` using `QNetworkAccessManager`:
  - `POST https://metavisage.io/api/desktop/exchange` with JSON body `{"code": "{code}"}`
  - No `Authorization` header required — the code is the credential
  - Parse the response: extract `access_token`, `refresh_token`, `license_jwt`
  - On `400 Bad Request` (code expired or already used): show a clear error ("The sign-in link expired. Please sign in again.") and restart the auth flow — exchange codes are single-use and expire after 2 minutes
  - On `429 Too Many Requests`: respect the `Retry-After` header and retry silently
- [ ] On successful exchange:
  - Store `access_token` and `refresh_token` encrypted in `%APPDATA%\MetaVisage\session.cache` using AES-256 with a key derived from the Windows machine GUID (`HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid`) passed through SHA-256
  - Store the `license_jwt` encrypted in `%APPDATA%\MetaVisage\license.cache` using the same derivation scheme
  - Verify the JWT signature using `LICENSE_JWT_SECRET` (embedded as a compile-time constant, never committed to the repository)
  - Transition auth state to `Authenticated` and dismiss the sign-in prompt, revealing the main application
- [ ] On app launch with a valid cached session, skip the browser flow entirely and load the cached `license_jwt` directly — no network call needed until the 24-hour refresh timer fires

#### Story 6.4: Token Refresh & Ongoing Validation
**Background:** Supabase access tokens are short-lived. The app uses `POST /api/desktop/token` (authenticated with the Supabase access token) to get a fresh license JWT, replacing the license-key-based `POST /api/license/validate` flow from the original plan.

**Tasks:**
- [ ] Implement `LicenseManager::RefreshLicenseJWT()` using `QNetworkAccessManager`:
  - `POST https://metavisage.io/api/desktop/token` with `Authorization: Bearer {access_token}`
  - Parse the response: extract `token`, `tier`, `conversions_used`, `conversions_limit`, `conversions_reset_at`
  - Update the cached `license_jwt`, in-memory tier, and usage counters
  - On `401 Unauthorized` (access token expired): use the stored refresh token to obtain a new access token via the Supabase client library, then retry `RefreshLicenseJWT()` once with the fresh access token; if the refresh token is also invalid, transition to `Unauthenticated` and prompt re-authentication
  - On `403 Forbidden` (subscription inactive/cancelled/past due): clear the license cache, transition to `Inactive`, and show a non-blocking banner: "Your subscription is inactive. [Manage at metavisage.io]"
  - On `404 Not Found` (no active license): same treatment as `403 Forbidden`
  - On `429 Too Many Requests`: respect `Retry-After`; retry silently (rate limit is 20 req/min)
  - On `500`: retry with exponential backoff (1s, 2s, 4s, max 30s); fall back to cached JWT
- [ ] Set up a 24-hour `QTimer` to trigger `RefreshLicenseJWT()` in the background while the app is running; failures within the 7-day grace window fall back to the cached JWT silently
- [ ] For launch-time validation, `POST /api/license/validate` also accepts a Supabase access token as the Bearer credential (same response shape as `/api/desktop/token`) — use this endpoint on launch to get an immediate fresh JWT before the timer fires

#### Story 6.5: Offline Grace Period
**Background:** The cached license JWT allows the app to function for up to 7 days without network access.

**Tasks:**
- [ ] On app launch, if the network is unavailable or all token refresh attempts fail: load and decrypt the cached `license_jwt` from `%APPDATA%\MetaVisage\license.cache`
- [ ] Verify the JWT signature against `LICENSE_JWT_SECRET` and check the `exp` claim — if within the 7-day window, grant full access using the cached tier and feature flags; show a non-blocking status bar notice: "Offline mode — using cached license"
- [ ] If the cached JWT is expired and all refresh attempts fail: show a blocking dialog ("No internet connection and your offline license has expired. Please connect to the internet to continue.") with a "Retry" button that re-attempts the refresh flow
- [ ] On a successful online refresh after an offline period: clear the offline notice and update the cache silently

#### Story 6.6: License Status UI
**Background:** Users need to see their current session and license status from within the application.

**Tasks:**
- [ ] Add a "Account & License" item to the `Help` menu that opens a `LicenseStatusDialog`
- [ ] `LicenseStatusDialog` displays: signed-in account email, current tier name, conversions used / conversions limit, reset date, validation status (Online / Offline – Grace Period / Inactive), and a "Manage Subscription" button that opens `https://metavisage.io/dashboard`
- [ ] Add a "Sign Out" button in the dialog that clears both the session cache and license cache, then transitions to `Unauthenticated` — requires confirmation ("Signing out will require you to sign in again to use MetaVisage. Continue?")
- [ ] Add a small persistent tier indicator to the status bar (e.g. "Free", "Indie", "Pro") so users always know which tier they are on

### Acceptance Criteria
- [ ] First launch with no cached session shows the sign-in prompt screen (no license key field)
- [ ] Clicking "Sign In / Create Account" opens the system browser to `https://metavisage.io/desktop-auth/init?state=...`
- [ ] The app shows a "Waiting for sign-in..." state while the browser is open
- [ ] The `metavisage://` deep link is intercepted correctly whether the app is newly launched or already running
- [ ] A mismatched `state` parameter aborts the flow and shows an error
- [ ] An expired or already-used exchange code shows a clear error and restarts the flow
- [ ] Successful code exchange stores encrypted session and license cache files
- [ ] Subsequent launches with a valid cache skip the browser flow entirely
- [ ] `POST /api/desktop/token` is called every 24 hours to refresh the license JWT
- [ ] An expired Supabase access token triggers a silent refresh token exchange before retrying
- [ ] A `403` or `404` from the token endpoint shows the inactive subscription banner
- [ ] App functions offline for up to 7 days using the cached JWT, with a status bar notice
- [ ] Expired offline cache shows a blocking reconnection dialog
- [ ] `Help > Account & License` opens `LicenseStatusDialog` with correct account and usage data
- [ ] "Sign Out" clears all cached credentials and returns to the sign-in prompt
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
- [x] Expose all feature flag values from `LicenseManager` via typed getters (e.g. `LicenseManager::CanUploadCustomHead()`, `LicenseManager::GetTextureTransferMode()`, `LicenseManager::GetTouchUpToolset()`)
- [x] `custom_head_upload = false` (Free): Disable only the custom source MetaHuman head upload in Stage 1; keep the target mesh upload and default MetaHuman head path available; show an upgrade prompt explaining the source-head feature requires Indie or higher
- [x] `texture_transfer = none` (Free): Hide the texture upload section in Stage 1 entirely
- [x] `texture_transfer = albedo` (Indie): Allow Albedo texture upload; hide and disable additional texture slot uploads beyond one (Albedo only); show an upgrade prompt for additional slots
- [x] `texture_transfer = albedo_normal` (Pro): Allow unlimited texture uploads (existing behavior)
- [x] `morphing_parameters = presets` (Free): Disable the General and Advanced collapsible parameter sections; only preset cards are selectable and parameters cannot be individually adjusted; show an upgrade prompt on the section headers
- [x] `morphing_parameters = full` (Indie/Pro): Full parameter access (existing behavior)
- [x] `masking = false` (Free): Hide/disable the Vertex Masking section in Stage 3; show an upgrade prompt
- [x] `touchup_tools = basic_smooth` (Free): Only Smooth brush is available in Stage 4; Grab, Push/Pull, and Inflate brushes are visible but disabled with an upgrade prompt on hover
- [x] `touchup_tools = basic` (Indie): Smooth and Grab brushes available; Push/Pull and Inflate disabled with upgrade prompt
- [x] `touchup_tools = full` (Pro): All brushes available (existing behavior)

#### Story 7.2: Conversion Tracking
**Background:** Each successful export counts as one conversion against the user's monthly limit. Accepting a morph in Stage 3 does not count; users can reprocess and compare settings freely before committing an export.

**Tasks:**
- [x] Implement `LicenseManager::ReportConversion()` which calls `POST /api/license/conversion` with `Authorization: Bearer <access_token>`
- [x] Call `ReportConversion()` only after a mesh export succeeds
- [x] On success (HTTP 200): update the in-memory `conversions_used` counter and refresh the `LicenseStatusDialog` if open
- [x] Before export, block when effective usage is already at the monthly limit; on server-side `403 limit_reached`, show a dialog explaining the limit, when it resets (`conversions_reset_at`), and a link to upgrade
- [x] Handle `429 rate_limited` with `Retry-After` backoff; handle `500` with exponential backoff and surface a non-blocking notification if the retry ultimately fails
- [x] If the device is offline after a successful export: queue the conversion report and send it on next successful network connection; do not block the completed export due to network error

#### Story 7.3: Upgrade Prompts
**Background:** Free and Indie users should see clear, non-intrusive upgrade prompts when they encounter a feature gated to a higher tier.

**Tasks:**
- [x] Implement a reusable `UpgradePromptWidget` — a small inline panel or tooltip-style callout that displays: a brief description of the locked feature, the tier required to unlock it, and a "Upgrade at metavisage.io" button that opens the pricing page
- [x] Use `UpgradePromptWidget` consistently at all feature gate points identified in Story 7.1
- [x] Upgrade prompts should be informative and non-alarming — no modal dialogs for passive feature gates; use inline disabled states with a visible indicator

#### Story 7.4: Conversion Limit Warning
**Background:** Users approaching their monthly conversion limit should be warned before they hit it.

**Tasks:**
- [x] After each successful conversion report, check if `conversions_used >= conversions_limit - 2` (within 2 conversions of the limit)
- [x] If so, show a non-blocking status bar notification: "You have X conversions remaining this month. Resets on [date]."
- [x] When `conversions_used == conversions_limit`, show the same notification with "0 conversions remaining" and an upgrade link
- [x] Display current conversion usage (`conversions_used / conversions_limit`) in the `LicenseStatusDialog`

### Acceptance Criteria
- [x] Free tier users cannot load a custom source MetaHuman head; target mesh upload remains available and an upgrade prompt is shown on the gated source slot
- [x] Free tier users cannot access individual morphing parameters; preset cards are the only option
- [x] Free tier users cannot use Vertex Masking
- [x] Free tier users can only use the Smooth brush in Stage 4
- [x] Indie tier users can use Smooth and Grab brushes; Push/Pull and Inflate show upgrade prompts
- [x] Pro tier users have access to all features
- [x] Each successful export reports to `POST /api/license/conversion`
- [x] Exporting when the limit is reached is blocked with a clear dialog
- [x] Offline export queues the conversion report for later delivery
- [x] Upgrade prompts appear inline at all gated feature locations
- [x] A warning appears in the status bar when within 2 conversions of the monthly limit

---

## Sprint 8: Distribution, Auto-Update & Release

**Duration:** 2 weeks  
**Goals:** Package MetaVisage as a properly installable Windows application with Inno Setup, integrate WinSparkle for automatic update delivery, perform final end-to-end testing across all tiers, and cut the v2.0.0 release.

### Stories

#### Story 8.1: Inno Setup Installer
**Background:** v1.0.0 had no installer — users received raw application files. An installer is required for professional distribution, for WinSparkle to function correctly, and critically, to register the `metavisage://` custom URL scheme that the Sprint 6 auth flow depends on.

**Tasks:**
- [ ] Install and configure Inno Setup 6.x
- [ ] Write the Inno Setup script (`installer/metavisage_setup.iss`):
  - Application name, version, publisher metadata
  - Install directory: `%PROGRAMFILES%\MetaVisage`
  - Bundle runtime DLLs (Assimp, GLFW/GLEW as needed, WinSparkle) and application assets (shaders, models, preset thumbnails, brush previews)
  - Create Start Menu shortcuts
  - Create Desktop shortcut (optional, user-selectable during install)
  - Register application in Windows `Add or Remove Programs`
  - Uninstaller that cleanly removes all installed files but does not delete `%APPDATA%\MetaVisage` (preserving projects, session cache, license cache, and user presets)
- [ ] Register the `metavisage://` custom URL scheme in the Inno Setup `[Registry]` section so that deep links from the browser auth flow fire correctly after installation. The uninstaller must also remove these registry entries (`Flags: uninsdeletekey`)
- [ ] Automate installer build via a PowerShell script (`package-release.ps1`) that runs the Inno Setup compiler after the MSVC Release build
- [ ] Test installer on a clean Windows 10 and Windows 11 VM with no Qt or MSVC redistributables pre-installed
- [ ] On the test VMs, verify the full auth flow end-to-end: install → launch → sign-in browser opens → portal redirects → `metavisage://` deep link fires → app receives the URI and completes authentication

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
- [ ] Test conversion limit enforcement: exhaust the monthly conversion limit on a Free account, verify the block dialog appears on the next export attempt
- [ ] Verify staging `/api/desktop/token` and `/api/license/conversion` are live before treating auth/conversion testing as release evidence
- [ ] Capture paid-feature visual QA frames for Free / Indie / Pro gate states, including standard locked rows, hover cards, ToolRail gates, and brush preview preservation
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
- [ ] `metavisage://` URL scheme is registered in the Windows registry after installation and removed cleanly by the uninstaller
- [ ] Full auth flow works end-to-end on a clean install: launch → browser sign-in → portal redirect → deep link fires → app authenticates
- [ ] Installed application launches, validates license, and completes the full workflow
- [ ] Uninstaller removes the application and registry entries but does not delete user data in `%APPDATA%\MetaVisage`
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

**Framework Migration (Sprints 3.5 & 3.6):**
- The portions of the codebase that transfer to the new stack without modification are: all GLSL shaders, all VBO/IBO/VAO rendering code, `Renderer`, `MeshRenderer`, `PointRenderer`, `ShaderManager`, `GridRenderer`, the entire `core/` data model (`Mesh`, `Transform`, `Project`, `Camera`), `ProjectSerializer`, all deformation code (`RBFInterpolator` removal notwithstanding, `NRICPDeformer`, `MeshDeformer`), all sculpting brushes, `RayCaster`, and `BVH`/`SpatialHash`. These represent the majority of the codebase by line count — the migration is a UI and hosting layer rewrite, not a ground-up rebuild.
- Dear ImGui's docking API (enabled via `ImGuiConfigFlags_DockingEnable`) is used to create the stable sidebar + viewport layout. The docking branch has been merged into Dear ImGui's main branch as of v1.89 and is stable for production use.
- The `stb_image.h` header may already be present in the project (it is commonly bundled with ImGui examples). Confirm before adding it as a separate vcpkg dependency.

**Deferred to Post-v2.0:**
- Orientation gizmo click-to-snap-view interaction
- Stage 1 numeric transform spinboxes (editable)
- Batch processing (reserved for Studio tier, per PRD-WebPortal.md)
- macOS and Linux support (GLFW and ImGui are cross-platform; a future macOS/Linux port is more feasible on this stack than it was on Qt without a commercial license)
- Automated/ML-assisted point placement
- Annual billing and team accounts (handled by web portal in a future phase)

**Dependency Notes:**
- Sprint 6 depends on the web portal's `/desktop-auth/init`, `/desktop-auth/success`, `POST /api/desktop/exchange`, `POST /api/desktop/token`, and `POST /api/license/validate` endpoints being deployed to the staging environment (`https://staging.metavisage.io`) before or concurrent with desktop development. Coordinate with the web team to confirm staging availability at the start of Sprint 6.
- The Named Pipe single-instance implementation (Story 3.6.4) and the `metavisage://` deep link interception (Story 6.2) are closely related. The pipe server thread started in 3.6.4 is the same thread that receives forwarded deep link URIs in 6.2 — treat them as a single mechanism, not two separate ones. The pipe server must be running before the auth flow can handle already-running instance redirects.
- The `metavisage://` URL scheme is registered by the Inno Setup installer (Sprint 8), but developers can manually add the registry entry on their dev machines during Sprint 6 to test the deep link interception without waiting for the installer.
- Sprint 8 WinSparkle integration requires the `appcast.xml` endpoint to be live on `metavisage.io` before release testing can be completed.
- The `LICENSE_JWT_SECRET` shared secret must be agreed upon between the desktop and web teams before Sprint 6 begins and provided as a build-time compile constant via a secure mechanism — never committed to the repository.

**Risk Mitigation:**
- The "cottage cheese face" artifact (Sprint 5.2) has an open-ended investigation scope. If the root cause is not resolved within Sprint 5, apply a conservative parameter warning as a stopgap and schedule a dedicated algorithm spike in a future sprint.
- Feature flag enforcement (Sprint 7) must be tested against all three tiers before release; enlist web team support to provision test accounts at each tier for QA.
- If the ImGui docking layout proves unstable or difficult to constrain to the desired sidebar-fixed / viewport-fills-remaining layout, a fallback approach is to use a non-docked layout with manually computed `ImGui::SetNextWindowPos` and `ImGui::SetNextWindowSize` calls each frame — simpler but requires manual resize tracking.
