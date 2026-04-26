---
name: Codebase Conventions
description: Qt transitional coupling, C++ patterns, and review observations from Sprint 3.5 PR #18
type: project
---

## Qt Transitional Coupling (Sprint 3.5 / PR #18)

Qt6::Core is intentionally kept as a transitional dependency after the Gui/Widgets/OpenGL removal. `QString` still appears in `include/ui/Viewport.h` (viewportLabel_ field), `src/ui/ViewportHost.cpp` (QStringLiteral calls), `src/ui/AppWindow.cpp` (log formatting), and `src/main.cpp`. This is tracked in product_backlog.md as "Qt6Core removal" and is NOT a defect to flag.

**Why:** Removing Qt6::Core is a ~15–20 point sprint-level effort; Sprint 3.5 scope was the Gui/Widgets/OpenGL cut only.

## C++ Standard Bump

CMakeLists.txt sets `CMAKE_CXX_STANDARD 20` in this PR (was 17). No C++20 features are visibly used yet — the bump is pre-wiring.

**Why:** Unexplained in PR description. Could be an oversight or forward-looking. Worth tracking but not blocking.

## GL State Save/Restore Pattern

`Viewport::RenderGL` saves scissor, blend, depth, cull, VAO, program, and viewport state before the 3D pass and restores them after, so ImGui's draw pass sees expected state. This is the established pattern for this codebase — reviewers should not flag it as "too much boilerplate."

## Drag Capture Pattern (ViewportHost)

`capturedViewport_` + `pressedButtons_` bitmask is used for mouse drag capture — once a button goes down over a viewport, all subsequent moves/releases route to that viewport. This intentionally mirrors v1.0 Qt behavior.

## GLFW Callback Install Pattern

`install_callbacks=false` on `ImGui_ImplGlfw_InitForOpenGL`, then manual forwarding inside each GLFW callback to `ImGui_ImplGlfw_*` before the WantCapture gate. This is intentional to avoid fighting the ImGui backend chain-callback logic. Do not flag this as "bypassing ImGui callbacks."

## Scheduled Tasks Lock File

`.claude/scheduled_tasks.lock` is a runtime file (session ID + PID + timestamp). It should be gitignored rather than committed.

## ImGui Popup Style Var Mismatch Risk

`ViewportHost::BuildLayout` pushes 3 style vars before `ImGui::Begin` and pops them after `ImGui::End`. This ordering is correct per ImGui docs (pop after End is valid for window-level vars), but reviewers should verify the pop count matches the push count whenever this pattern appears.

## Debug HUD Left In (Sprint 3.6)

`AppWindow::RenderFrame` contains a "Debug HUD" ImGui window with FPS/GL/GLSL info. The comment says "Removed before Sprint 3.6 closes." This is intentional for the sprint and should be flagged as non-blocking dead code rather than a blocking issue.

## Auth/JWT Tokens Stored Unencrypted in Config.json

`Config.cpp` persists `access_token`, `refresh_token`, and `license_jwt` in plaintext JSON at `%APPDATA%\MetaVisage\config.json`. The spec (DESKTOP_APP_INTEGRATION.md) requires the JWT to be encrypted with a key derived from machine ID + license key. This gap is tracked from Sprint 3.6 — it is a genuine security defect but the encryption subsystem isn't built yet.

## HttpClient HeaderCallback Off-by-One

In `src/utils/HttpClient.cpp`, `HeaderCallback` compares `line.size() > 13` but "Retry-After:" is 12 characters. The check should be `> 12`. Additionally the value is parsed starting at offset 12 which is the colon itself — the leading space after the colon is handled by `strtol` skipping whitespace, so the parse itself is correct, but the guard should be `>= 13` to require at least one digit.

## Morph Worker Lifetime Pattern

`AppWindow::StartMorph` launches `morphThread_` as `[this]()` lambda. The join in `Shutdown()` correctly cancels the deformer then joins before `project_` is reset. `PollMorphCompletion` joins on the main thread. This pattern is safe — do not flag the `[this]` capture as dangling.

## Edge IBO Not Restored After Wireframe Draw in VAO Context

After `Wireframe`/`SolidWireframe` paths in `MeshRenderer::Render`, the code rebinds `ibo_` after drawing edge indices. This is intentional: the edge IBO is drawn outside the VAO (to avoid polluting the VAO's element binding), so the explicit restore is correct.

## GizmoRenderer Uses Relative Asset Paths (Sprint 3.6)

`GizmoRenderer::Initialize()` loads OBJ models via bare relative paths (`"assets/models/gizmos/..."`) that require the CWD to be the executable directory. Compare with `AppWindow::UseDefaultMorphMesh()` which correctly uses `Paths::ExecutableDir()`. This is a known inconsistency — the gizmos work in dev (VS sets CWD = project root) but will silently fail when the exe is launched from a different directory or via the installer. Flagged as non-blocking until installer packaging is addressed in Sprint 4.

## CancelTransform Is a Stub (Sprint 3.6)

`Viewport::CancelTransform()` clears `isTransforming_`/`activeGizmoAxis_`/`axisConstraint_` but does NOT restore `transformBeforeState_` to the target mesh. The comment says "3.5.4 restores transformBeforeState_" but the restore was never implemented. Pressing Escape mid-drag commits the partial transform instead of reverting it.

## sDockBuilt Static-Local (Sprint 3.6)

`AppWindow::RenderAppShell()` guards the DockBuilder setup with `static bool sDockBuilt`. This correctly fires once per process. The comment says it rebuilds "every launch" to resist `imgui.ini` edits — this is accurate because `io.IniFilename = nullptr` is set, so there is no ini to read, and the static is process-scoped (i.e., per-launch). Do not flag this as a "static-local in a method body" smell.

## GizmoRenderer qDebug/qWarning (Sprint 3.6)

`GizmoRenderer.cpp` uses Qt `qDebug()`/`qWarning()` directly instead of routing through `MV_LOG_*`. This is an inconsistency with the rest of the codebase (all new Sprint 3.6 code in AppWindow, Viewport, Config, etc. uses `MV_LOG_*`). Flag as a non-blocking cleanup.

## Scale Z-Axis Inversion in Gizmo Drag (Viewport.cpp)

In `Viewport::OnMouseMove`, the Scale transform path uses `sfInv` (inverted scale) for the Z-axis constraint. This is intentional — dragging right means "bigger in X/Y but the Z-axis gizmo pointer points away from camera" so the inversion gives a natural feel. Do not flag as a bug without first verifying with the user.

## HeaderCallback Off-by-One (Tracked From Sprint 3.5)

Already documented above. The `> 13` guard should be `>= 13`. Still present in Sprint 3.6 — not fixed.
