# Sprint 10 Retrospective

**Date**: 2026-10-30
**Sprint**: [Sprint 10: App Shell & Morph Stage Polish](../sprints/sprint-10.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 12 points
- **Velocity (Actual)**: 12 points
- **Sprint Goal Achievement**: 100%
- **Stories Completed**: 7 of 7
- **Carryover Items**: None. Running a full MinGW build is still open (see Known issues).
- **Tangential Work**:
  - Config is never saved unless it was loaded, which protects the user's config from the smoke harnesses.
  - Home / Reset Camera returns to perspective.
  - View > Frame Meshes added.
  - Menu popups no longer inherit the host's 0 px item spacing.
  - The viewport toolbar has a projection toggle, and Auto Perspective applies after orbiting out of a preset view.
  - The shortcuts dialog lists F and Home.
  - The MinGW resource is compiled in a separate `windres` step.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | `tests/smoke/ui_smoke.cpp`: `shell_menus_and_debug_hud`, `morph_reset_to_source` | The menu, grid, camera preset, projection toggle, Auto Perspective, reset/undo/confirm and section order tests are all automated |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK. `ctest --test-dir build -C Release --output-on-failure`: 4/4 passed (45.4 s). `smoke_ui`: 13/13 | `git diff --check` clean. `.\build-mingw.ps1` not run (see Known issues) |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Pass. The EXE icon and the running window's class icon were both extracted and show the logo. Casey confirmed the taskbar icon | The packaged ZIP was not rebuilt; the icon is embedded in the EXE the ZIP copies |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | Yes | Casey supplied File / Edit / View / Help menu screenshots and a Morph inspector screenshot | Shared in session, not committed |
| Loading / empty / error states | Yes | Reset to source mesh is disabled with no result; View items are disabled without a viewport; an accepted result asks for confirmation and Cancel keeps it | Covered by smoke tests |
| Known issues | Yes | The full MinGW build is unvalidated, because `build-mingw.ps1` shares `build\` with MSVC; the resource step was validated in a throwaway MinGW project. Ctrl+Y redo is unbound (backlog). The renderer cache is not released on project switch (backlog) | The MinGW item was already a known gap in BUILD.md |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 4 | Every story has smoke coverage. The icon is checked by extraction, not by a test |
| Visual consistency | 5 | Menu spacing uses new `MvStyle::Menu` tokens; Casey approved the screenshots |
| Loading behavior | N/A | No new async work |
| Navigation completeness | 5 | Every visible menu item works; full Stage 1 -> 4 -> Export run by Casey |
| Error handling | 4 | Reset is guarded while processing and with no result, and asks for confirmation on accepted results |

---

## Retrospective

### START (Start doing)
- Pairing every keyboard-only control with something visible on screen. Casey found the Perspective menu item too hidden; the toolbar toggle and Auto Perspective fixed that.
- Validating build-system changes in a throwaway project when the real build directory can't be reused. The spaced-path `windres` failure only showed up that way.

### STOP (Stop doing)
- Writing Python string replacements containing backslashes inside heredocs. `\b` and `\a` turned into control characters twice. Use script files, or edit those lines with the Edit tool.

### CONTINUE (Keep doing)
- Running the owner's visual feedback inside the sprint. Both follow-ups (projection toggle, mask placement) were implemented, tested, and recorded the same day.
- Stacking a sprint branch on an unmerged predecessor when the two share files, and saying so in the sprint doc.

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Give `build-mingw.ps1` its own build directory (for example `build-mingw\`) so the full MinGW build can run alongside MSVC | Next sprint implementer | Next sprint | [ ] Backlog |
| 2 | Merge cmershon2/MetaVisage#29, then open the Sprint 10 PR into `development` | Casey / implementer | Before the next sprint | [ ] |

---

## Previous Action Items Review

| # | Action (from Sprint 9) | Status |
|---|---------------------------|--------|
| 1 | Hand-check the orthographic views through the View menu in Sprint 10.3 | ✅ Done (Casey verified the presets and orbiting away) |
| 2 | Open a PR for Sprint 9 and run `$review-pr` before Sprint 10 | 🔄 PR #29 is open and reviewed (Approve), but not merged yet |

---

## Key Metrics
- **Velocity Trend**: Stable (10 -> 12 points, both fully delivered)
- **Team Morale**: High
- **Process Health**: All steps done in order: approval, branch, implementation, tests, owner verification, follow-ups, review, retro.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. The only open gap is the MinGW full build, which predates this sprint and has an action item.
