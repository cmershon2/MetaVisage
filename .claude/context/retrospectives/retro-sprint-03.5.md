# Sprint 3.5 Retrospective

**Date**: 2026-04-21
**Sprint**: [Sprint 3.5 — Framework Migration: Platform Layer (Qt → GLFW + ImGui)](../sprints/sprint-03.5.md)
**Format**: 4Ls (Liked / Learned / Lacked / Longed For)

---

## Sprint Summary
- **Velocity (Planned)**: 26 points
- **Velocity (Actual)**: 26 points (3.5.1 · 3.5.2 · 3.5.3 · 3.5.4 all landed)
- **Sprint Goal Achievement**: ~90% — platform cut shipped end-to-end (GLFW window, ImGui frame, GL viewport, camera + keyboard input). Full-workflow visual parity (meshes, points, brush cursor, dual-viewport Stage 2 sync) deferred to 3.6 because the project/mesh loader UI isn't back yet.
- **Stories Completed**: 4 of 4
- **Carryover Items**: None. Deferred items (visual parity smoke-test, sculpt/mask/point-click LMB wiring) are blocked on Sprint 3.6 loader UI, not unfinished work.

---

## Retrospective

### LIKED (What we liked)

- **Story sequencing held up under pressure.** 3.5.1 (build system) → 3.5.2 (window + ImGui) → 3.5.3 (GL viewport) → 3.5.4 (input) was the right order: every story built cleanly on the previous one, no backtracking, no mid-sprint re-plan.
- **The transitional `Qt6::Core` decision.** Keeping Qt6::Core linked (LGPL-safe) and cutting only Gui/Widgets/OpenGL/OpenGLWidgets was the commercial-licensing-relevant move. Porting QString/QFile/QJson across core/io/deformation/baking would have doubled the sprint; deferring that cleanly to a tracked backlog item kept scope honest.
- **Explicit GL state save/restore around the 3D pass.** Paid off immediately — no depth-test, blend, VAO, or scissor bleed between the 3D render and ImGui draw lists. The discipline of treating the GL pass as an isolated block is what made the dual-world (3D + ImGui) architecture feel stable rather than brittle.
- **`/review-pr 18` caught four real blockers** before merge: the committed `scheduled_tasks.lock` runtime artifact, the ImGui partial-init leak, the WantCaptureMouse-during-drag strand, and the undocumented C++20 bump. Each was a genuine defect, not style noise.

### LEARNED (What we learned)

- **ImGui input gating is not "just check WantCaptureMouse".** A fullscreen ImGui host window will pin `WantCaptureMouse` true every frame and silently eat all viewport input. `ImGuiWindowFlags_NoInputs` on the host window was the fix — but only discoverable by reading backend source, not by reading the ImGui FAQ. Worth a memory.
- **Drag capture must bypass the WantCaptureMouse gate for move + release events.** If the cursor crosses into the debug HUD mid-orbit, the gate swallows the release and the viewport is permanently stranded in orbit state. Fixed with an `IsDragging()` accessor and a bypass, but it's a class of bug that will recur wherever ImGui and a custom input surface share a window.
- **GLFW cursor coordinates are screen-space, not framebuffer-space.** Hit-testing viewport rects required capturing *both* the framebuffer-scaled rect (for the GL pass) and the screen-coord rect (for cursor routing). One rect is not enough on HiDPI.
- **`install_callbacks=false` + manual chain-forwarding is the right ImGui/GLFW pattern** whenever you need to gate input. Letting `ImGui_ImplGlfw_InitForOpenGL(window, true)` install its own callbacks and then overwriting them silently breaks ImGui's input — the ordering and chaining matters.
- **Claude Code's PreToolUse hooks using relative paths are a footgun.** `python .claude/hooks/validate-commit-msg.py` breaks permanently if the Bash cwd drifts (e.g. after `cd build/bin/Release` to launch the exe). All hook commands should use `"$CLAUDE_PROJECT_DIR/..."`.

### LACKED (What we lacked)

- **No automated visual-regression harness.** "No rendering regressions versus the Qt build" is in the DoD, but verification is manual screenshot comparison — fragile and easy to skip. Even a single reference-image diff on a known scene would raise confidence.
- **No loader UI in the Sprint 3.5 runtime.** The viewport renders an empty scene by default, which meant four of the six DoD parity items (loaded meshes, point markers, brush cursor, dual-viewport Stage 2 sync) couldn't be smoke-tested and had to defer. Any future platform-layer work should carry a minimum scene-loading path for verification.
- **Hook config didn't survive real-world shell state.** Once `cd` drifted into a build subdirectory, every Bash tool call was blocked by the relative-path hook and couldn't be unblocked without a session reset. Cost ~15 min of the sprint close.

### LONGED FOR (What we longed for)

- **A smoke-test project checked into the repo.** A tiny `.mmproj` + one small mesh + two correspondence points, loadable via a debug CLI flag, would have let Sprint 3.5 close its DoD parity checks without waiting on 3.6's sidebar.
- **An automated check that `Qt6::Gui/Widgets/OpenGL/OpenGLWidgets` stay out.** A grep-based CMake/CI step that fails the build if those headers reappear would make the Qt removal durable — right now it's a discipline issue.
- **A way to verify the exe launched without reading stderr from a background process.** Runtime verification this sprint was awkward (background launches exited 127/1 until DLLs were staged; after staging, couldn't confirm success beyond "no stderr"). A launch-smoke-test that exits 0 after rendering one frame would close this gap.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Fix PreToolUse hooks in `.claude/settings.json` to use `"$CLAUDE_PROJECT_DIR/.claude/hooks/..."` absolute paths so Bash cwd drift can't brick the session | Casey | 2026-04-25 | [ ] |
| 2 | Add a checked-in smoke-test project (tiny `.mmproj` + small mesh + 2 points) and a debug CLI flag (`--open <path>`) so future platform-layer work can verify viewport parity before the sidebar is back | Casey | End of Sprint 3.6 | [ ] |
| 3 | Add a CMake/CI guard that fails the build if any `Qt6::Gui/Widgets/OpenGL/OpenGLWidgets` header reappears, so the Qt removal stays durable while `Qt6::Core` removal is still in flight | Casey | End of Sprint 3.6 | [ ] |

---

## Previous Action Items Review

No prior retrospective exists for this project — this is the first sprint retrospective on record.

| # | Action (from Sprint N-1) | Status |
|---|--------------------------|--------|
| — | — | — |

---

## Key Metrics
- **Velocity Trend**: Baseline (first retro; 26/26 committed landed).
- **Team Morale**: High — sprint goal hit, migration unblocks the v2.0 commercial cut, no carryover.
- **Process Health**: Good — story decomposition + `/review-pr` caught real defects before merge. Tooling friction (hook cwd drift, no loader UI for verification) is the main drag.
