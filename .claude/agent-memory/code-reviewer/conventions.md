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
