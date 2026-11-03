# Sprint 14: Custom Source Profiles

**Sprint Goal**: A user can turn any source mesh into a reusable profile, by
placing landmarks in a guided editor or saving a manual Stage 2 setup, so
auto-map works beyond the bundled MetaHuman head.
**Duration**: 2026-11-04 — 2026-11-10
**Capacity**: ~14 points (Sprint 12 and 13 velocity: 13 and 15)
**Committed**: 14 points, plus a 3-point stretch story
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Approved by Casey on 2026-11-03 ("Approve as planned")

Source: [auto-point-reference.md](../design/auto-point-reference.md), "Custom
profiles (Sprint 14)", the last of the four Auto Point Reference sprints.
Sprint 11 already built the profile format, `ProfileStore` (including user
profile Save / Delete / ids) and the canonical 52-landmark list; Sprint 13
built point dragging. This sprint adds the user-facing tools on top.

Branch: `feat/sprint-14-custom-profiles`, from `development` after the Sprint
13 merge (cmershon2/MetaVisage#33).

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [x] Sprint plan approved before execution
- [x] Approved scope executed
- [x] Tests run and validation recorded
- [x] Sprint completion confirmed (Casey, 2026-11-03: "Close out now")
- [x] Retrospective saved in `.agents/context/retrospectives/`
- [x] Context updated with `$update-context`
- [x] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot (N/A: no campaign)

---

## Sprint Backlog

Backlog item: **Custom source profiles**.

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 14.1 | **User profiles own a copy of their mesh** | 2 | Decision 2 | [x] |
| 14.2 | **Save source as profile** | 3 | 14.1 | [x] |
| 14.3 | **Profile editor: guided landmark placement** | 5 | 14.1 | [x] |
| 14.4 | **Profile editor: default mask** | 2 | 14.3 | [x] |
| 14.5 | **Manage profiles** | 2 | 14.1 | [x] |
| 14.6 | *Stretch:* **Import and export a profile** | 3 | 14.1 | [x] |

### 14.1 User profiles own a copy of their mesh

- [x] Saving a user profile copies its source mesh (and nothing else) into
      `%APPDATA%\MetaVisage\profiles\<id>\`, and the profile file points at
      the copy, so a profile keeps working if the original file moves
      (design decision 2, see open question 1).
- [x] On load, a user profile whose mesh is missing or no longer matches its
      fingerprint is listed but disabled, with the reason in a tooltip.
- [x] The Stage 1 source picker lists user profiles below the built-ins, as
      it does today.

### 14.2 Save source as profile

- [x] A **Save source as profile…** action (Stage 1 source picker and Stage 2
      sidebar) asks for a name, then saves a user profile from the current
      project: source mesh, fingerprint, source transform, symmetry axis and
      offset, the source-side points as landmarks, and the vertex mask.
- [x] Landmarks keep their ids and names when the points carry them (auto-map
      or an edited profile). Otherwise the existing draft rules apply: exactly
      14 unpaired + 19 pairs are named from the canonical list, anything else
      becomes "Point N". Symmetric pairs keep their mirror link.
- [x] The project switches to the new profile without changing its points,
      mask or transforms. One undo step.
- [x] Replaces the developer-only "Export Source Profile Draft (Dev)" flow for
      users; the dev menu item stays for authoring built-ins.

### 14.3 Profile editor: guided landmark placement

- [x] **New profile from this mesh…** (source picker, when a source mesh is
      loaded) and **Edit landmarks…** (user profiles) open the profile editor:
      the sidebar shows the canonical checklist grouped by region (brow, eye,
      nose, mouth, cheek, jaw, ear, cranium, neck), and the viewport shows the
      source mesh only.
- [x] Click a checklist row to make it active, then click the mesh to place
      it; the next unplaced landmark becomes active. Placing one side of a
      pair fills its partner at the mirrored surface point (toggle, on by
      default). Points can be dragged as in Stage 2, with the midline lock.
- [x] Each landmark can be skipped; **Add custom landmark** adds a named
      extra point (midline or paired). The checklist shows placed / skipped /
      missing at a glance.
- [x] Editor edits have their own undo history (Ctrl+Z inside the editor);
      the project's history is untouched. **Save** writes the profile (new or
      updated); **Cancel** discards with a confirm when there are changes.
- [x] Auto-map works with any profile that has canonical landmarks; custom
      landmarks are placed by the same fit, without the per-feature extras.

### 14.4 Profile editor: default mask

- [x] The editor has the mask brush (paint, erase, radius, clear, invert) from
      Stage 3, painting the profile's default mask on the source mesh, with
      symmetry. The mask is saved as ranges, as built-ins are.

### 14.5 Manage profiles

- [x] A **Manage profiles** list (source picker menu) offers rename,
      duplicate and delete for user profiles, and duplicate for built-ins.
      Delete confirms first; projects that used the profile still open (their
      points and mask live in the project) and show "profile not installed".
- [x] **Set as default**: the default profile is preselected for new
      projects (stored in settings; built-in MetaHuman by default).

### 14.6 Stretch: Import and export a profile

- [x] **Export…** writes one `.mvprofile.zip` (profile JSON plus mesh);
      **Import…** reads one, gives it a fresh id if it collides, and checks the
      fingerprint against the bundled mesh before adding it. Needs a small zip
      library (open question 2).

---

## Sprint Context (Key Affected Files)

- `include/core/SourceProfile.h`, `src/core/SourceProfile.cpp`: build from
  project with landmark ids; editor helpers
- `include/io/ProfileStore.h`, `src/io/ProfileStore.cpp`: mesh copy, rename,
  duplicate, missing-mesh handling, default profile
- `src/ui/AppWindow.cpp` / `.h`: save-as-profile, editor mode (own undo stack),
  manage dialog, picker actions
- `src/ui/StageSidebar.cpp` / `.h`: source picker actions, editor panel
  (checklist, mask tools)
- `src/ui/Viewport.cpp` / `.h`: editor single-viewport mode, routing clicks and
  mask painting to the editor
- `src/ui/Modals.cpp` / `.h`: name prompt, manage profiles list
- `utils/Config`: default profile setting
- `tests/smoke/source_profile.cpp`, `tests/smoke/ui_smoke.cpp`
- `.agents/context/PRD.md`, design doc decision 2, README

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| **Fixed-width confirm and error modals**: with auto-resize alone, a wrapped message narrowed the window frame by frame into a tall column with the buttons off-screen (found when the editor's "Discard changes?" confirm could not be clicked). Both now use a 440 px width; the error modal is also centred | Long messages render as a readable box everywhere | [x] |
| **Casey's hands-on review**: the landmark checklist stuttered and wouldn't scroll (mouse wheel or scrollbar). It re-centred on the active landmark every frame; now it scrolls there only when the active landmark changes | The checklist scrolls freely | [x] |
| **Casey's second review**: Manage Source Profiles buttons were too short (ImGui small buttons); they are now 26 px row buttons. The stage bar is hidden while the profile editor is open, since the editor's stage is internal | Easier targets; no misleading stage bar in the editor | [x] |
| `DrawInspectorFooter` takes an optional left-button label | The editor footer reads Cancel / Save profile | [x] |
| The Stage 3 mask controls moved into a shared `RenderMaskControls` | The editor's mask tab reuses them unchanged | [x] |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes | `source_profile`: save copies the mesh, missing or changed mesh disables the profile, rename / duplicate / delete, build-from-project keeps ids, mirrors and mask. UI smoke: save source as profile, editor place / mirror fill / skip / custom / undo / save / cancel, manage list actions, auto-map with a custom profile | Uses a temporary profiles directory, never the real `%APPDATA%` |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy`; `ctest --test-dir build -C Release --output-on-failure` (six targets) | |
| Live smoke tests | Not required | | Offline desktop app |
| Rendered image checks | Yes | Casey builds a profile for a non-MetaHuman mesh in the editor and auto-maps a dataset head with it | The real check for this sprint |
| Link audit | Not required | | No links change |
| Screenshots / visual audit | Yes | Casey reviews the editor checklist, mask tools and manage list in the app | New UI surfaces |
| Impeccable design pass | Not required | | Not installed; `MvStyle` tokens and existing components |
| Loading / empty / error states | Yes | Profile with a missing mesh, fingerprint mismatch, duplicate name, deleting the active profile, cancelling an editor with changes, saving with no landmarks | Automated where possible |

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| The editor mode touches viewport routing, the sidebar and undo at once | Med | High | Reuse Stage 2 placement and drag code and the Stage 3 mask brush; a separate undo stack keeps the project history clean |
| Copying large source meshes (hundreds of MB) into %APPDATA% | Low | Med | Copy the file once on save; show its size in the confirmation |
| Auto-map quality on non-MetaHuman sources is unmeasured | Med | Med | The solver is source-agnostic; Casey's hands-on check uses a non-MetaHuman source. Benchmark dataset work stays a retro action item |
| A zip dependency for import/export | Low | Low | Stretch story; `miniz` (MIT) through vcpkg is small, or export a folder instead |

---

## Open Questions (resolved at approval)

Casey approved the plan as written, so each question takes its
recommendation: (1) design decision 2 is confirmed: user profiles copy their
source mesh into the profile folder; (2) import/export uses one
`.mvprofile.zip` through `miniz`; (3) once the editor works, Casey moves the
cheekbone, jaw middle and ear-top points on a duplicate of the built-in, and
the result is copied into the built-in profile file.


1. **Design decision 2: user profile mesh storage.** Recommendation: copy the
   source mesh into the profile's folder, so profiles survive moved or
   deleted files and can be exported as one package. The alternative is to
   store a link to the original file, which saves disk space but breaks when
   the file moves.
2. **Import/export package.** Recommendation: a single `.mvprofile.zip` using
   `miniz` (MIT, through vcpkg). The alternative, with no new dependency, is
   exporting a folder (profile JSON plus mesh) that users zip themselves.
3. **Sprint 12 action item: the cheekbone, jaw middle and ear-top points.**
   The new editor makes this easy to do by hand. Recommendation: once the
   editor works, you move those points on a duplicate of the built-in, and I
   copy the result into the built-in profile file (a few minutes of tangential
   work). Or leave the built-in as it is.

---

## Definition of Done

- [x] Code reviewed (`$review-pr`, 2026-11-03)
- [x] Tests passing (six CTest targets)
- [x] Casey has built a custom profile in the editor and auto-mapped with it
- [x] PRD updated for custom profiles; design decision 2 recorded
- [x] Required quality evidence captured in the retrospective
- [x] PO approval
- [x] Sprint outcome confirmed
- [x] Retrospective saved
- [x] Context updated

---

## Sprint Outcome

**Status**: Complete (confirmed by Casey 2026-11-03)
**Velocity (Actual)**: 17 points (14 committed plus the 14.6 stretch)
**Carryover**: Copying Casey's adjusted cheekbone, jaw middle and ear-top points into the built-in profile (closed out without it; retro action item 1)
**Deviations from the plan**:
- 14.6 uses `kubazip` (MIT) instead of `miniz`: it is already in the vcpkg tree as an Assimp dependency, so nothing new is downloaded. Added to `vcpkg.json` as a direct dependency.
- 14.1: a missing mesh copy is flagged when profiles load. A mesh that is present but changed is caught when the profile is picked or opened in the editor (it would need every user mesh loaded at startup otherwise).
- 14.4: the mask brush has no symmetry; the Stage 3 brush it reuses has none.
- 14.3: adding a custom landmark to the checklist is not an undo step (placing it is); Cancel discards it with the rest of the session.
- 14.5: no default profile is set until the user picks one, so File > New behaves as before; the plan said MetaHuman by default.
**Validation** (2026-11-03):
- `.\build-msvc.ps1 -Deploy`: builds. The full Core rebuild (new include path) surfaced two C4996 warnings in untouched code (`getenv` in `NRICPDeformer.cpp`, `sprintf` in the bundled `stb_image_write.h`); no warnings in new code.
- `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed (83 s). `smoke_ui` 16/16 (24 s).
- `smoke_source_profile` adds: SaveWithMesh copies the mesh and re-saving from the copy works; Duplicate of the built-in; a deleted mesh copy is flagged; drafts keep landmark ids, names, regions and mirrors; package export / import round-trip with a fresh id; a mismatched or missing package is refused.
- New `custom_profiles` UI scenario: save source as profile (one undo step, attached); the editor (own history, mirror fill, custom pair, mask tab, File > New blocked, save with name / landmarks / mask, attached to the project); auto-map with the 4-landmark custom profile places all 4 at their vertices; editing and cancelling with changes confirms; Manage duplicate / set default / File > New with default / delete; a missing mesh copy is flagged and refused by the editor. User profiles go to a temporary folder for the run; the real `%APPDATA%` folder was checked untouched.
- `git diff --check` clean.
- **Closeout (2026-11-03)**: `$review-pr` found no blocking issues; kubazip's license notice is picked up by `package-release.ps1` like every vcpkg dependency. Final run: `.\build-msvc.ps1 -Deploy` OK; `ctest` 6/6 passed (87 s; `smoke_ui` 16/16); `git diff --check` clean. Casey: "Everything functions great!"
**Quality Evidence**: [retro-sprint-14.md](../retrospectives/retro-sprint-14.md)
**Notes**: Auto Point Reference (Sprints 11–14) is complete.
**Campaign Handoff**: N/A
