---
name: Sprint 5 Pre-Implementation Audit
description: Findings from the Sprint 5 pre-sprint code audit covering Stories 5.1–5.4 — what exists vs. what the master plan assumes
type: project
---

## Story 5.1 — Stage 3 Advanced Parameters (Collapsible Hierarchy)

**Current state of RenderMorph** (`src/ui/StageSidebar.cpp:900–1190`):
- GENERAL section: flat `ImGui::Slider*` calls at lines 1026–1031 (stiffness steps/init/final, iterations, normal threshold, landmark weight), no `CollapsingHeader`
- ADVANCED section: single `ImGui::CollapsingHeader("Advanced")` at line 1036, flat list with all remaining params (sampling, rigidity, damping, inner iters, epsilon, boundary exclusion, boundary hops)
- "Reset to defaults" button at line 1058 is INSIDE the Advanced collapsing header — needs relocation to always-visible per master plan

**MvInspectorPrimitives gap** (`include/ui/MvInspectorPrimitives.h`):
- Only 7 helpers exist: DrawStageHeader, DrawSectionLabel, DrawFileSlot, DrawAxisInput, DrawTransformGroup, DrawValidationRow, DrawInspectorFooter
- No sub-section / nested collapsible primitive
- Closest: DrawSectionLabel (flat, lines 39–40) — a new DrawSubSectionLabel would be needed

**Config persistence gap** (`include/utils/Config.h`):
- Fields: window geometry, recent projects, access token, refresh token, license JWT only
- No UI-state JSON section, no collapsed-state persistence
- To add: new `std::map<std::string, bool> collapsedSections_` member + getter/setter pair under a `"ui_state"` JSON key

**Why:** Master plan demands persistent collapsed state across sessions — currently there is nowhere to store it.

## Story 5.2 — Cottage-Cheese Artifact Investigation

Three primary culprit locations in `src/deformation/NRICPDeformer.cpp`:

1. **Landmark weight injection** — lines 764–790 (`BuildSystemMatrix`): `wl` is a flat scalar; no floor/cap relative to `alpha`. When alpha reaches its minimum at the final stiffness step, landmark rows retain full weight, creating local over-constraint at correspondence-point locations.

2. **BFS boundary exclusion** — lines 198–286 (`DetectAndExcludeBoundaryVertices`): BFS expands uniformly from all open boundary edges. For UV-seamed MetaHuman LOD0, weld-map seam edges are treated as open boundaries, so the exclusion zone may be larger than intended near UV seams.

3. **Excluded-vertex correspondence skip + landmark interaction** — lines 350 and 387 (ICP loop): excluded vertices skip the data term but landmark term still fires (line 766). If a landmark vertex falls inside the exclusion zone, unbalanced pull bends local geometry.

**No diagnostic mode exists** — only `MV_LOG_INFO` counts at line 1185. Intermediate residual dump would need to be built from scratch.

## Story 5.3 — Stage 4 Brush Selector Polish

**Brush selector is NOT on ToolRail.** `RenderTouchUp` (`src/ui/StageSidebar.cpp:1230–1243`) uses a 2-column `ImGui::BeginTable("##brushGrid", 2)` with four `DrawPresetCard` calls — no ToolRail involvement. Grep for "ToolRail" in StageSidebar.cpp returns zero hits. Story is NOT obsolete — the 2×2 grid exists in ImGui form and needs promotion to ToolRail.

**No brush preview assets** — `assets/brushes/` directory does not exist.

**Legitimate remaining polish gaps:** ToolRail promotion, hotkey badge display (`[`/`]`), active-highlight consistency.

## Story 5.4 — imgui_test_engine Carryover

**vcpkg.json** (`vcpkg.json:7–11`): imgui features are `["glfw-binding", "opengl3-binding", "docking-experimental"]` — `test-engine` feature is ABSENT. Adding it is day-1 work.

**AppWindow::TickOnce()** (`include/ui/AppWindow.h`): Public API is only `Initialize`, `Run`, `Shutdown`. `Run()` is a blocking loop. No headless-tick entry point. The private `RenderFrame(int fbWidth, int fbHeight)` exists at line 63 — a public `TickOnce` wrapper would need to be added before any test harness can drive the window.

**How to apply:** Sprint 5 story scoping should account for these gaps as prerequisite work before the test lane can be written.
