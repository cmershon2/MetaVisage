# Sprint 11: Source Profiles

**Sprint Goal**: Stage 1 selects a Source Profile instead of a bare mesh file,
and the built-in MetaHuman profile ships with its landmarks and default mask,
ready for the Sprint 12 auto-map solver.
**Duration**: 2026-11-16 — 2026-11-20
**Capacity**: ~13 points (Sprint 10 velocity: 12)
**Committed**: 13 points
**Execution Mode**: Classic
**Campaign**: None
**Approval**: Approved by Casey on 2026-10-30 ("Approved, create a new development branch to begin implementing this")

Source: Casey's 2026-10-30 request for auto point reference. Design:
[auto-point-reference.md](../design/auto-point-reference.md). This is the
first of four sprints (11–14); see the design's roadmap.

Branch: `feat/sprint-11-source-profiles`, created from `feat/sprint-10-shell-morph`
because the Sprint 9/10 PRs are not merged yet. This sprint edits `AppWindow.cpp`
and `StageSidebar.cpp`.

---

## Workflow Loop

- [x] Sprint plan created in `.agents/context/sprints/`
- [x] Sprint plan approved before execution
- [x] Approved scope executed
- [x] Tests run and validation recorded
- [x] Sprint completion confirmed (Casey, 2026-10-30)
- [x] Retrospective saved in `.agents/context/retrospectives/`
- [x] Context updated with `$update-context`
- [x] Focused sprint commit created with `$commit`
- [ ] Campaign handoff written, if this sprint is part of Campaign Autopilot (N/A: no campaign)

---

## Sprint Backlog

Backlog item: **Source Profiles and built-in MetaHuman profile**.

| # | Story | Points | Dependencies | Status |
|---|-------|--------|--------------|--------|
| 11.1 | **Source Profile model and topology fingerprint** | 2 | None | [x] Implemented, tested |
| 11.2 | **ProfileStore** | 2 | 11.1 | [x] Implemented, tested |
| 11.3 | **Author the built-in MetaHuman profile** | 5 | 11.1, 11.2 | [x] Authored and reviewed |
| 11.4 | **Stage 1 source picker** | 3 | 11.2, 11.3 | [x] Implemented, tested, checked by Casey |
| 11.5 | **Project format v3 with `sourceProfileId`** | 1 | 11.1 | [x] Implemented, tested |

### 11.1 Source Profile model and topology fingerprint

- [x] Add `include/core/SourceProfile.h` / `src/core/SourceProfile.cpp` with the
      fields from the design: id, name, description, isBuiltIn, mesh path,
      fingerprint, default transform, symmetry, landmarks, mask ranges and
      optional default wrap preset id. Also `Validate()` and the 52-entry
      `CanonicalLandmarks()` list.
- [x] Add `TopologyFingerprint::FromMesh(const Mesh&)`: vertex count, face count,
      and a 64-bit FNV-1a hash over each face's size and vertex indices in order.
      Computed on the merged vertex list `Mesh::Load` produces (the bundled
      head: 24,408 Assimp vertices welded to 24,049).
- [x] Add mask helpers to convert between `std::vector<bool>` and sorted ranges.
- [x] Measure whether the bundled OBJ and an FBX export of it produce the same
      fingerprint. Result: they **match** (24,049 vertices, 24,002 faces, hash
      `a1670396aaad548d`) for an FBX written by MetaVisage's own exporter. The
      smoke test re-measures this every run. FBX files exported by Maya or
      Unreal are not yet measured.

### 11.2 ProfileStore

- [x] Add `include/io/ProfileStore.h` / `src/io/ProfileStore.cpp`, mirroring
      `PresetStore`: `BuiltInDir()` (`<exe>/assets/profiles`),
      `UserProfilesDir()` (`%APPDATA%\MetaVisage\profiles`), `LoadAll()`,
      `Save()`, `Delete()` and `GenerateUserProfileId()`. There are also
      directory overloads for tests, plus `ReadFile` / `WriteFile`, `FindById` and
      `FindByFingerprint`.
- [x] Use `schema_version: 1`. Skip unknown versions and malformed files with a
      logged warning. Built-ins are never written or deleted. A user profile
      whose folder name differs from its id is skipped, and `Delete` only
      accepts `user-XXXXXXXX` ids, which guards against path traversal.
- [x] Add no user-facing create or delete UI yet; that is Sprint 14. `Save` and
      `Delete` are covered by the smoke test only.

### 11.3 Author the built-in MetaHuman profile

- [x] Add a dev-only Help > "Export Source Profile Draft (Dev)" item, next to
      the existing Dev items. It writes the current project's source-side point
      vertex indices, symmetry pairs and vertex mask to a draft
      `.mvprofile.json`. If the points are exactly 14 unpaired plus 19 symmetric
      pairs, they are named from the canonical list in placement order, so no
      hand-naming is needed. See the authoring steps below.
- [x] Place the 52-point canonical landmark set from the design on
      `MetaHumanHeadLod0.obj`, then export. **Casey**. Placed on 2026-10-30 in
      two exports (14 midline, then 19 pairs); merged by hand since neither
      export alone matched the 14 + 19 shape the auto-naming needs. Pairs
      mirror within 0.33 units. As placed, "jaw angle" sat
      between the chin and "jaw middle"; the two were swapped at Casey's request. The mesh
      has no vertex exactly on x = 0 (closest is 0.0003; the mesh is only
      symmetric to about 0.07 units), so midline points snap to the nearest
      vertex. The placed ones are within 0.19 of the plane on a head 36.7 wide.
- [x] Paint the default mask, then export. **Casey**. 649 vertices in one
      connected region covering the inner lips and mouth interior. It is
      mirror-symmetric (every masked vertex's mirror is also masked).
- [x] Commit the result as `assets/profiles/metahuman-head-lod0.mvprofile.json`,
      with the -90° X default transform that `UseDefaultMorphMesh` hardcoded.
      The file has the real fingerprint, the transform, X symmetry, Quick Wrap
      as the default preset, all 52 landmarks under their canonical ids, and
      the mask (132 ranges). `smoke_source_profile` now fails
      (code 56) if a built-in landmark's id, name, region or mirror id doesn't
      match the canonical set.
- [x] Add the profile to the required assets list in `package-release.ps1`.
- [x] Casey reviews landmark placement and mask coverage. Done on 2026-10-30
      in the app with the landmark viewer below; no screenshots committed.
- [x] Add a dev-only Help > "Show Profile Landmarks (Dev)" item for that review.
      In Stage 2 it replaces the points with the profile's landmarks as
      source-only points with mirrors linked (one undo step). The Stage 2 list
      shows each landmark's name, and the viewport names the selected point.
      Names live on `PointCorrespondence::landmarkName`, which is not
      serialized; Sprint 13 adds landmark ids to the format. Covered by the
      `source_profile_picker` UI scenario.

**Authoring steps** (the export names points by order, so order matters):

1. Start a new project. Pick "MetaHuman Head (LOD0)" as the source, and also
   import `assets/models/MetaHumanHeadLod0.obj` as the target so Stage 2 opens.
2. In Stage 2, with **symmetry off**, click the 14 midline landmarks on the
   source (right) pane in this order: cranium top, forehead center, glabella,
   nasion, nose bridge, nose tip, subnasale, upper lip center, lip contact
   center, lower lip center, chin point, chin bottom, occiput, nape.
3. Turn **symmetry on (X)** and click one side of each of the 19 pairs, in this
   order: brow inner, brow peak, brow outer, eye inner corner, eye outer
   corner, upper lid center, lower lid center, nostril wing, nasolabial crease
   top, upper lip peak, mouth corner, cheekbone, jaw angle, jaw middle,
   temple, ear top, tragus, ear lobe bottom, ear back. Either side works; the
   export labels the +X side "(L)".
4. In Stage 3, paint the default mask.
5. Help > Export Source Profile Draft (Dev). The info dialog reports the
   landmark count and any validation problems. Expect 52 landmarks and no
   problems.
6. Copy the `landmarks` and `mask_ranges` arrays into
   `assets/profiles/metahuman-head-lod0.mvprofile.json`. Keep the shipped id,
   name, mesh path and `default_wrap_preset`. Then run `smoke_source_profile`.

Assumption verified during review: +X in the head's mesh space is the
character's left.

### 11.4 Stage 1 source picker

- [x] Replace the "Use Default MetaHuman Head" button with a profile picker
      (`##sourceProfile` combo) above the source file slot. Built-in profiles
      come first, then user profiles after a separator. The file slot's button
      is now "Import mesh" (ghost style). A summary line shows the landmark
      count and whether there is a default mask, or says that no profile
      matches.
- [x] Selecting a profile loads its mesh and fingerprint-checks it, then applies
      the default transform, the default mask and the default wrap preset.
      Note: mesh loading is still synchronous on the UI thread, like
      `ImportMorphMeshFromDisk` before it. **Load meshes off the UI thread** is
      a separate backlog item.
- [x] If the project already has points, a processed morph, or a mask that
      differs from the profile default, ask before replacing them. Changing the
      source is one undo step (`SourceSwapUndoAction`).
- [x] "Import mesh…" keeps today's behaviour, plus the same confirmation. After
      loading, if the mesh matches a profile's fingerprint, it attaches that
      profile (landmarks and mask, not its transform) and says so in the status
      bar.
- [x] A fingerprint mismatch shows an error modal naming the profile and the
      mesh. The source stays unchanged.
- [x] Remove `UseDefaultMorphMesh` and its hardcoded rotation. A missing
      built-in mesh reports "The mesh for source profile '…' was not found at …".

### 11.5 Project format v3 with `sourceProfileId`

- [x] Add `Project::sourceProfileId_` with accessors.
- [x] Bump `ProjectSerializer::FORMAT_VERSION` from 2 to 3, write
      `sourceProfileId`, and read it as empty when absent.
- [x] Commit a format v2 fixture (`tests/fixtures/v2_project.mmproj`). It was
      written by hand to match the v2 `Save` output, with a vertex mask and
      non-default wrap parameters. Assert that v1 and v2 files load with an
      empty profile id, and that v3 round-trips the id and the mask.
- [x] If a saved profile id no longer exists, the project still opens normally
      and logs one warning.

---

## Sprint Context (Key Affected Files)

- New: `include/core/SourceProfile.h`, `src/core/SourceProfile.cpp`
- New: `include/io/ProfileStore.h`, `src/io/ProfileStore.cpp`
- New: `assets/profiles/metahuman-head-lod0.mvprofile.json`
- `include/core/Project.h`: `sourceProfileId`
- `include/core/UndoActions.h`: `SourceSwapUndoAction`
- `include/io/ProjectSerializer.h`, `src/io/ProjectSerializer.cpp`: format v3, load fix
- `src/ui/StageSidebar.cpp`, `include/ui/StageSidebar.h`: Stage 1 source picker, `selectSourceProfileId` action
- `src/ui/AppWindow.cpp`, `include/ui/AppWindow.h`: profile selection, source swap, import matching, dev export menu item
- `CMakeLists.txt`: new sources, new `smoke_source_profile` test target
- `package-release.ps1`: required asset list
- New: `tests/smoke/source_profile.cpp`; updated `tests/smoke/v1_backward_compat.cpp` and `tests/smoke/ui_smoke.cpp`
- New fixture: `tests/fixtures/v2_project.mmproj`

---

## Tangential Work Log

| Work | Why It Helped | Status |
|------|---------------|--------|
| `ProjectSerializer::Load` now restores the vertex mask and the optimization / rigidity / sampling parameters. They were saved but never copied back, so every reopened project lost its painted mask and those settings. | Sprint 11's default mask has to survive save and reopen. Covered by the v2 fixture. | [x] |
| Changing the source mesh is now undoable, and it clears the morph result and `originalMorphMesh`. Before, importing a new source kept the old `originalMorphMesh`, so Process would still wrap the previous source. | Required for the profile picker; also fixes the existing import path | [x] |
| Help > Show Profile Landmarks (Dev): shows a profile's landmarks as named, source-only Stage 2 points. | Casey had no way to see the authored landmarks after merging | [x] |
| Undo and redo refresh the mask overlay (`AppWindow::RefreshMaskOverlay`). Mask colors only reached the GPU while painting, so undoing a mask stroke or a source change kept showing the replaced mask. | Found in code review; source-swap undo depends on it | [x] |

---

## Quality Gates & Evidence Plan

| Gate | Required? | Evidence To Capture In Retro | Notes |
|------|-----------|------------------------------|-------|
| Tests added | Yes | New `smoke_source_profile`: fingerprint stability and sensitivity, mask range round-trip, validation, canonical set, ProfileStore save/load/delete in a temp dir, malformed / unknown-schema / mis-filed profiles skipped, path-traversal ids refused, generic and canonical draft export, built-in profile matches the bundled head, FBX round-trip fingerprint (informational). `smoke_v1_backward_compat`: v2 fixture and v3 round-trip. `ui_smoke` `source_profile_picker`: built-in selection, confirm/cancel/replace, one-step undo, fingerprint mismatch refused. | |
| Tests run | Yes | `.\build-msvc.ps1 -Deploy`; `ctest --test-dir build -C Release --output-on-failure`; `ctest ... -L ui` | Suite grows from four targets to five; update the count in AGENTS.md during `$update-context` |
| Live smoke tests | Not required | | Offline desktop app |
| Rendered image checks | Yes | Landmarks and default mask on the built-in head: front, side, three-quarter | Casey reviews placement (11.3) |
| Link audit | Not required | | No links touched |
| Screenshots / visual audit | Yes | Stage 1 inspector before and after the picker; mismatch error; replace confirmation | Pending Casey's manual pass |
| Impeccable design pass | Optional | Critique of the Stage 1 source picker | Only if the skill is enabled |
| Loading / empty / error states | Yes | Picker with no user profiles; missing built-in asset; malformed profile file; fingerprint mismatch; profile deleted after save | Malformed files and the mismatch are automated; the rest are manual |

---

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Assimp loads OBJ and FBX of the same head with different vertex counts, so fingerprints differ | Low (measured: MetaVisage FBX matches) | Med | Re-measured every smoke run; FBX from other tools still unmeasured |
| Landmark authoring takes longer than planned (52 points plus mask) | Med | Med | The dev export names points automatically; if needed, cut to a core set of about 30 points and carry the rest |
| Format v3 breaks older project loading | Low | High | v1 and v2 fixtures in `smoke_v1_backward_compat` |
| Default mask hides regions users expect to wrap | Low | Med | Casey reviews coverage; users can still edit the mask in Stage 3 |
| Picker change confuses existing users who used "Use Default MetaHuman Head" | Low | Low | The built-in profile is the first entry, and the placeholder reads "Select a source profile". It is not auto-loaded: loading on New Project would block the UI thread until meshes load off it |

---

## Definition of Done

- [x] Code reviewed (`$review-pr`): one finding (mask overlay not refreshed after undo), fixed
- [x] Tests passing
- [x] Windows manual UI smoke: new project → pick built-in profile → import target → Stage 2 → 3 (default mask visible) → 4 (Casey, 2026-10-30). Save and reopen keeping the profile id is covered by the automated v3 round-trip
- [x] Required quality evidence captured in the retrospective
- [x] PO approval
- [x] Sprint outcome confirmed
- [x] Retrospective saved
- [x] Context updated

---

## Sprint Outcome

**Status**: Complete (confirmed by Casey on 2026-10-30; ran ahead of the planned 2026-11-16 start)
**Velocity (Actual)**: 13 of 13 points
**Carryover**: —
**Validation** (2026-10-30):
- `.\build-msvc.ps1 -Deploy`: builds and deploys; no new warnings (the three C5054 warnings in `AppWindow.cpp` are in existing dock-flag code).
- `ctest --test-dir build -C Release --output-on-failure`: 5/5 passed.
- `metavisage_smoke_ui.exe`: 14/14, including the new `source_profile_picker` and the landmark viewer.
- `smoke_source_profile`: the built-in profile loads with 52 canonical landmarks and 132 mask ranges.
- `smoke_source_profile`: the bundled head fingerprint is 24,049 vertices, 24,002 faces, hash `a1670396aaad548d`; the FBX round-trip matches.
**Quality Evidence**: [retro-sprint-11.md](../retrospectives/retro-sprint-11.md)
**Notes**: The landmarks were authored in two exports (14 midline, then 19 pairs) and merged by hand. The jaw angle and jaw middle were swapped after review.
**Campaign Handoff**: N/A
