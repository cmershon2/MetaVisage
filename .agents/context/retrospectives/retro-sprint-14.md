# Sprint 14 Retrospective

**Date**: 2026-11-03
**Sprint**: [Sprint 14: Custom Source Profiles](../sprints/sprint-14.md)
**Campaign**: None
**Format**: Start-Stop-Continue

---

## Sprint Summary
- **Velocity (Planned)**: 14 points, plus a 3-point stretch
- **Velocity (Actual)**: 17 points (stretch 14.6 included)
- **Sprint Goal Achievement**: 100%. This completes Auto Point Reference (Sprints 11–14)
- **Stories Completed**: 6 of 6
- **Carryover Items**: Copying Casey's adjusted cheekbone, jaw middle and ear-top points into the built-in profile (closed out without it; stays an action item)
- **Tangential Work**:
  - Confirm and error modals have a fixed width; auto-resize alone collapsed long messages into a tall column with the buttons off-screen.
  - `DrawInspectorFooter` takes an optional left-button label; the Stage 3 mask controls became a shared `RenderMaskControls`.
  - From Casey's review: the landmark checklist no longer re-centres every frame (it stuttered and wouldn't scroll); Manage Source Profiles uses full-height buttons; the stage bar is hidden in the profile editor.

---

## Sprint Quality Evidence

| Evidence | Required? | Result | Notes |
|----------|-----------|--------|-------|
| Tests added | Yes | `source_profile.cpp`: mesh copy and re-save, duplicate, missing mesh flagged, drafts keep landmark ids / names / regions / mirrors, package export / import round-trip, mismatched and missing packages refused. `ui_smoke.cpp`: new `custom_profiles` scenario (save source as profile, editor place / mirror fill / custom pair / undo / mask / blocked File > New / save / cancel-with-confirm, auto-map with a 4-landmark custom profile, Manage duplicate / default / New with default / delete, missing mesh refused) | User profiles go to a temporary folder; the real `%APPDATA%` folder was checked untouched |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy` OK. `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed (87 s). `smoke_ui` 16/16 (25 s) | `git diff --check` clean. A full Core rebuild surfaced two C4996 warnings in untouched code (`getenv` in NRICPDeformer, `sprintf` in stb). `.\build-mingw.ps1` not run (backlog) |
| Live smoke tests | No | N/A | Offline desktop app |
| Rendered image checks | Yes | Casey built and used custom profiles in the app: "Everything functions great!" | Including a "Metahuman (UE 5.8)" user profile set as default |
| Link audit | No | N/A | No links changed |
| Screenshots / visual audit | Yes | Casey reviewed the editor, checklist and Manage Source Profiles (screenshot of the modal shared in review) | Two rounds of visual fixes |
| Impeccable design pass | No | Not installed | `MvStyle` tokens, Lucide icons and existing components |
| Loading / empty / error states | Yes | Automated: missing mesh copy, mismatched package, missing package, cancel with changes, File > New while editing. By design: a profile with no landmarks saves (mask only); an empty name can't be saved | Fingerprint drift of a present mesh is caught on pick or edit, not at startup |
| Known issues | Yes | Mask brush has no symmetry; adding a custom checklist entry is not an undo step; auto-map quality with sparse custom profiles on real heads is unmeasured | See action items |

### Quality Rubric

| Dimension | Score (1-5) | Evidence |
|-----------|-------------|----------|
| Test coverage | 5 | Every story has store-level and UI-level checks, including the package path and a custom-profile auto-map |
| Visual consistency | 4 | Existing tokens and components; two small layout fixes came from review |
| Loading behavior | 4 | Profile meshes load synchronously when the editor opens (a 24k-vertex head is instant; very large meshes would pause) |
| Navigation completeness | 5 | Every profile action reachable from Profile tools or Manage; the editor blocks File actions and hides the stage bar |
| Error handling | 5 | Missing or changed meshes, bad packages, name conflicts and unsaved changes all handled with clear messages |

---

## Retrospective

### START (Start doing)
- Giving every auto-resizing modal a fixed width. The collapse bug had been latent since Sprint 3.8; it took a long message to expose it.
- Treating per-frame ImGui calls that set state (`SetScrollHereY`, focus) as suspects in any "stutter" report. Both this sprint's UI bugs were "applied every frame instead of on change".

### STOP (Stop doing)
- Stashing the working tree to compare builds. It risked untracked work for a question the git history could answer (the warnings were in unchanged files).

### CONTINUE (Keep doing)
- Reusing proven machinery through a narrow seam. Running the editor on a scratch project with its own undo stack let Stage 2 placement and dragging and the Stage 3 mask brush work unchanged, and kept the real project's history clean.
- Checking a new dependency against what's already installed before adding one (kubazip was already in the vcpkg tree).

---

## Action Items

| # | Action | Owner | Due Date | Status |
|---|--------|-------|----------|--------|
| 1 | Copy the adjusted cheekbone, jaw middle and ear-top points from Casey's duplicate into the built-in profile | Casey (name the profile) / implementer | Next sprint | [ ] |
| 2 | Measure auto-map with a sparse custom profile on real dataset heads | Casey / implementer | Before further solver work | [ ] |
| 3 | Update the MetaVisage docs site for auto-map, point dragging and custom profiles (carried from Sprint 13) | Casey / docs | Before the next release | [ ] |
| 4 | Grow the auto-map dataset with heads kept out of tuning (carried from Sprint 12) | Casey | Before further solver work | [ ] |
| 5 | Measure an Unreal or Maya FBX export's fingerprint (carried since Sprint 11) | Casey / implementer | Next sprint | [ ] |
| 6 | Give `build-mingw.ps1` its own build directory (carried since Sprint 10) | Next sprint implementer | Next sprint | [ ] Backlog |

---

## Previous Action Items Review

| # | Action (from Sprint 13) | Status |
|---|---------------------------|--------|
| 1 | Update the docs site for auto-map, confidence rings and dragging | ❌ Not started (carried as action 3) |
| 2 | Plan Sprint 14 | ✅ Done |
| 3 | Decide on the cheekbone, jaw middle and ear-top points | ✅ Decided (Casey adjusts on a duplicate); the copy is carried as action 1 |
| 4 | Grow the dataset with heads kept out of tuning | ❌ Not started (carried as action 4) |
| 5 | Measure an Unreal or Maya FBX export's fingerprint | ❌ Not started (carried as action 5) |
| 6 | Give `build-mingw.ps1` its own build directory | ❌ Not started (carried as action 6) |

---

## Key Metrics
- **Velocity Trend**: Up (15 -> 17 points, stretch delivered)
- **Team Morale**: High; Casey: "Everything functions great!"
- **Process Health**: All steps in order: plan, approval with decisions, implementation, two hands-on review rounds, code review, retro.
- **Campaign Loop Readiness**: Ready for next sprint
- **Readiness Rationale**: All required evidence is green. Auto Point Reference is complete; the next sprint should be planned from the backlog, with action items 1 and 5 as small candidates.
