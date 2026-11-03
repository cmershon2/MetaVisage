# Auto Point Reference: Design

**Status**: Sprint 11 complete (2026-10-30). Sprint 12 complete (2026-11-03); Sprints 13–14 outlined.
**Owner**: Casey
**Replaces backlog item**: "Automated point placement (ML)"

## Goal

After Stage 1, the user chooses **Auto** or **Manual** point placement. Auto
places the reference points on the target mesh from a preconfigured set of
source landmarks. The user can then move points, add more, and continue to
Stage 3. This matches the Mesh to MetaHuman flow in Unreal: add a custom mesh,
let it place markers, adjust, then process.

Manual placement stays exactly as it works today.

## Why the structure has to change

Auto-mapping needs to know, before Stage 2 starts, which source vertices are
the landmarks and which regions the wrap should ignore. Today nothing about
the source mesh is preconfigured:

- The source (morph) mesh is the bundled `assets/models/MetaHumanHeadLod0.obj`
  or any imported file (`AppWindow::UseDefaultMorphMesh`,
  `AppWindow::ImportMorphMeshFromDisk`). Its -90° X fix-up rotation is
  hardcoded.
- Every point pair is placed by hand (`PointCorrespondence` in
  `include/core/Project.h`).
- The vertex mask (`MorphData::vertexMask`) starts empty in every project and
  is painted in Stage 3.

The fix is a reusable **Source Profile**: a source mesh plus its landmark set,
default mask and default transform. Stage 1 picks a profile instead of a bare
mesh file.

## Terms

| Term | Meaning |
|------|---------|
| Source mesh | The MetaHuman head being wrapped. `morphMesh_` in code; "Source mesh" in the UI. |
| Target mesh | The user's custom head. |
| Source Profile | A source mesh plus its landmarks, default mask, default transform and symmetry settings. Built-in or user-created. |
| Landmark | A named source vertex that auto-map finds on the target (for example `nose_tip`, `mouth_corner_l`). |
| Topology fingerprint | Vertex count, face count and a 64-bit hash of face vertex indices. Proves a mesh has the topology a profile's vertex indices refer to. |
| Auto-map | Running the landmark solver to produce Stage 2 correspondences. |

Use these terms consistently in code, docs and UI copy.

## Source Profile

### Contents

| Field | Notes |
|-------|-------|
| `id` | Stable key. Built-ins use readable ids (`metahuman-head-lod0`); user profiles use `user-XXXXXXXX` like `PresetStore::GenerateUserPresetId`. |
| `name`, `description` | Display only. |
| `isBuiltIn` | Built-ins are read-only. |
| `mesh` | Built-in: path relative to `assets/`. User: a copy of the mesh inside the profile folder (see Decision 2). |
| `fingerprint` | `{ vertexCount, faceCount, faceHash }`. |
| `defaultTransform` | Replaces the hardcoded MetaHuman rotation. |
| `symmetry` | `{ axis, planeOffset }` for mirroring landmarks and masks. |
| `landmarks[]` | `{ id, name, vertexIndex, mirrorId?, region }`. `region` is one of `brow`, `eye`, `nose`, `mouth`, `jaw`, `cheek`, `ear`, `cranium`, `neck`. |
| `mask` | Excluded vertex indices, stored as sorted ranges to keep files small. |
| `defaultWrapPresetId` | Optional. Applied when the profile is selected for a new project. |

### File format

```json
{
  "schema_version": 1,
  "id": "metahuman-head-lod0",
  "name": "MetaHuman Head (LOD0)",
  "mesh": "models/MetaHumanHeadLod0.obj",
  "fingerprint": { "vertex_count": 24049, "face_count": 24002, "face_hash": "a1670396aaad548d" },
  "default_transform": {
    "position": [0, 0, 0],
    "rotation": [-0.70710677, 0, 0, 0.70710677],
    "scale": [1, 1, 1]
  },
  "symmetry": { "axis": "X", "plane_offset": 0.0 },
  "landmarks": [
    { "id": "nose_tip", "name": "Nose tip", "vertex": 1234, "region": "nose" },
    { "id": "mouth_corner_l", "name": "Mouth corner (L)", "vertex": 2345,
      "mirror": "mouth_corner_r", "region": "mouth" }
  ],
  "mask_ranges": [[100, 180], [512, 512]],
  "default_wrap_preset": "quick"
}
```

`rotation` is a quaternion (x, y, z, w). `is_built_in` is not stored: the
directory a profile loads from decides it. The fingerprint above is the real
one for the bundled head, measured in Sprint 11 on the welded vertex list
`Mesh::Load` produces. The landmark vertices are illustrative.

### Storage

- Built-in: `assets/profiles/<id>.mvprofile.json`. CMake already copies
  `assets/` into the build, so no deploy change is needed.
  `package-release.ps1` should require the built-in profile like the default head.
- User: `%APPDATA%\MetaVisage\profiles\<id>\profile.mvprofile.json`, plus the
  copied mesh in the same folder.
- `ProfileStore` mirrors `PresetStore`: `BuiltInDir()`, `UserProfilesDir()`,
  `LoadAll()`, `Save()`, `Delete()`, `GenerateUserProfileId()`. Unknown
  `schema_version` values are skipped with a warning.

### Fingerprint rules

- Landmarks and masks only apply when the loaded mesh's fingerprint matches the
  profile's. A mismatch means the vertex indices would point at the wrong
  vertices, so the profile is refused with a clear message.
- When the user imports a source mesh directly, the app checks it against every
  profile. If one matches (for example, a MetaHuman head exported to FBX that
  still has MetaHuman topology), that profile is attached automatically.
- Risk: an OBJ and an FBX of the same mesh can load with different vertex
  counts if Assimp splits vertices at UV seams. Sprint 11 measured this: the
  bundled head and an FBX of it written by MetaVisage produce the same
  fingerprint, because `Mesh::Load` welds seam vertices before the hash. FBX
  files exported by Maya or Unreal are not measured yet.

### Project integration

- `Project` gains `sourceProfileId` (empty means "no profile"). The serializer
  bumps `FORMAT_VERSION` from 2 to 3. Version 1 and 2 files load with an empty id.
- Points and masks already live in the project, so a project still opens and
  processes normally if its profile is later deleted. Only auto-map needs the
  profile.
- Selecting a profile sets the source mesh, its transform and the default mask.
  If the project already has a painted mask or points, it asks before replacing them.

## Auto-map solver

### Interface

```cpp
struct LandmarkSolveInput {
    std::shared_ptr<const Mesh> source;  Transform sourceTransform;
    std::shared_ptr<const Mesh> target;  Transform targetTransform;
    const SourceProfile* profile;        // landmarks, symmetry
    std::vector<bool> mask;              // profile mask merged with user mask
};

struct SolvedLandmark {
    std::string landmarkId;
    int sourceVertex;
    Vector3 targetPosition;
    int targetVertex;      // snapped like manual placement
    float confidence;      // 0..1
};

class LandmarkSolver {
public:
    virtual ~LandmarkSolver() = default;
    virtual LandmarkSolveResult Solve(const LandmarkSolveInput&,
                                      const ProgressCallback&,
                                      const std::atomic<bool>& cancel) = 0;
};
```

It runs on a worker thread, like the Stage 3 wrap. It is a domain-layer class
with no ImGui dependency, so it can be tested headless.

### Geometric template-fit solver (first implementation)

1. **Similarity refinement.** Rigid ICP plus uniform scale of the masked,
   subsampled source onto the target, starting from the Stage 1 alignment.
2. **Coarse fit.** Run the existing wrap engine with a stiff, few-step parameter
   set, no landmarks, and the profile mask plus boundary exclusion. It is fast
   because it only needs to land landmarks in the right neighbourhood.
3. **Projection.** For each landmark, take its fitted position, find the
   closest point on the target surface (existing `BVH` / `RayCaster`), and
   snap to the nearest target vertex, as manual placement does.
4. **Symmetry.** If the target is close to mirror-symmetric on the profile axis,
   average each mirrored pair so left and right agree.
5. **Confidence.** Combine the fit residual in a neighbourhood around the
   landmark (2.5% of the diagonal), normal agreement, mirror-pair consistency,
   how flat the surface is there (flat patches cap a landmark at "review"),
   and the overall alignment residual. Bands: high ≥ 0.75, review 0.4–0.75,
   low < 0.4.

**As built in Sprint 12** (`TemplateFitSolver`): step 1 is followed by a
bounded affine refinement (up to 1.4× per axis) so proportions can differ, and
by a facing check that refuses a target turned 180° about the vertical. The
symmetry step only runs when the target's median mirror distance is under 0.2%
of its diagonal. After Casey's first review two steps were added: each eye and
ear gets its own bounded affine fit (up to 2.2× per axis, matched both ways)
before the coarse fit, and the mouth corners and lip contact snap to the
deepest point of the crease nearby. A solve takes 3–6 s on the bundled head
against targets of up to ~900k vertices.

**Measured accuracy (2026-11-01).** On synthetic variants of the bundled head:
0.07–0.54% median error, at most 1.8% (fractions of the ~57-unit diagonal).
On Casey's six hand-placed heads: 0.53–1.79% median. Distinct features
(eyelids, nose, lips, inner brows) are within 0.4–0.7%. The largest errors come
from landmarks whose profile vertex differs from where Casey clicks them on
targets (cheekbone, ear top, jaw middle; same offset on every head), from
smooth-surface landmarks, and from exaggerated features (the goblin's ears).
`metavisage_automap_bench` reproduces these numbers; see
`tests/datasets/README.md`.

**After the feature refit and crease snap (2026-11-03).** Median across all
points 1.08% → 0.98%; mouth corners 1.43% → 1.15%; ear points 2.30% → 2.05%
(goblin ear lobes 2.3% → 0.4%). Lower lids are unchanged (0.9%; cartoon female
still 2.9%). Tried and dropped: refitting the mouth (pulled closed lips apart
on the facescan), refitting the nose (moved cartoon nostrils off), and
snapping the nose tip, subnasale and ear extremes (helped some heads, hurt as
many). The goblin's ear tops stay ~13% off; they are now rated review or low.
Confidence was not retuned: most remaining high-confidence misses are the
cheekbone and jaw points, which are offset the same way on every head, and
the fit can't detect that from the geometry.

Why geometric first: it needs no new dependency, works on untextured meshes,
and does not assume a realistic human face. MetaVisage ships a Fantasy Wrap
preset, and stylized heads are where 2D face-landmark models do worst.

### ML detector (possible later solver)

Unreal's Mesh to MetaHuman renders the mesh and runs a trained 2D face-landmark
model, then projects the results onto the 3D surface. A second
`LandmarkSolver` could do the same (for example with ONNX Runtime and an
Apache-2.0 face-landmark model). It is only worth building if the geometric
solver falls short on realistic heads in Sprint 12 testing. It must stay local
and offline.

## Stage 2 workflow (Sprint 13)

- **Prompt.** On Continue from Stage 1, if the profile has landmarks and Stage 2
  has no points, show a modal: **Auto-map points** (primary) or **Place
  manually**. Without a profile, or with existing points, go straight to Stage 2.
- **Progress.** A modal with progress and Cancel while the solver runs. The UI
  thread stays responsive.
- **Result.** Points appear paired on both meshes. Each point is coloured by
  confidence band; the sidebar lists review and low points first with their
  landmark names.
- **Editing.** Click and drag a placed point; it slides along the mesh surface
  under the cursor (new; today points can only be deleted). Requested by Casey
  on 2026-11-03 after reviewing Sprint 12's auto-placed points:
  - **Midline lock.** Midline landmarks (no mirror partner) stay on the
    symmetry plane while dragged, moving only along the curve where the plane
    meets the mesh. A per-point unlock (and a sidebar toggle for all) frees
    them for asymmetrical features.
  - **Mirrored drag.** Dragging one point of a mirrored pair moves its partner
    to the reflected spot, snapped to the surface. On by default when the
    target is near-symmetric; a toggle (and a modifier key held while
    dragging) turns it off for one-sided adjustments.
  - One drag is one undo step, covering both points of a mirrored pair.
  - Dragging works on either viewport (source or target side).
  - Users can add manual points as before.
- **Re-run.** A sidebar "Auto-map" action re-runs the solver. It asks whether to
  replace all auto points or only fill missing ones, and never moves points the
  user has edited.
- **Undo.** One auto-map is one undo step (a `PointReferenceData` snapshot
  action, like `PointPlacementUndoAction`).
- **Data.** `PointCorrespondence` gains `landmarkId`, `confidence` and
  `isAuto`. Editing a point clears `isAuto`. These are serialized in format v3.

## Custom profiles (Sprint 14)

- **Profile editor.** A single-viewport mode opened from the Stage 1 source
  picker ("New profile from this mesh…"). It shows the canonical landmark list
  as a checklist; the user clicks to place each landmark, and mirrored
  partners fill in from symmetry. It also has the mask brush. Extra custom
  landmarks are allowed.
- **Library management.** Rename, duplicate, delete and set as default. Built-ins
  are read-only, but they can be duplicated.
- **Save from project.** "Save source as profile" captures the current
  project's source-side points and mask. This is a quick path for users who
  already did a manual Stage 2.
- **Sharing.** Import and export a profile as a single `.mvprofile.zip` (JSON
  plus mesh).

## Canonical landmark set (draft)

Authored on the built-in MetaHuman profile in Sprint 11, and used as the
editor checklist in Sprint 14. That is 14 midline landmarks plus 19 pairs,
52 points in total.

- **Midline (14)**: cranium top, forehead centre, glabella, nasion, nose bridge,
  nose tip, subnasale, upper lip centre, lip contact centre, lower lip centre,
  chin point, chin bottom, occiput, nape.
- **Pairs, L/R (19 × 2)**: brow inner, brow peak, brow outer, eye inner corner,
  eye outer corner, upper lid centre, lower lid centre, nostril wing, nasolabial
  crease top, upper lip peak, mouth corner, cheekbone, jaw angle, jaw middle,
  temple, ear top, tragus, ear lobe bottom, ear back.

The density matches the mouth, eyes and nose, where the wrap most often drifts.

## Decisions

Confirm each decision before the sprint that depends on it.

| # | Decision | Status | Needed by |
|---|----------|--------|-----------|
| 1 | Solver approach | **Confirmed by Casey, 2026-10-31.** Geometric template-fit now, so the feature works end to end; an ML face detector is the likely long-term improvement and stays a later solver behind the same `LandmarkSolver` interface | Sprint 12 |
| 2 | User profile mesh storage | Recommended: copy the mesh into the profile folder. Not yet confirmed | Sprint 14 |
| 3 | Should Auto refine the Stage 1 alignment? | **Confirmed by Casey, 2026-10-31.** No. The similarity refinement is internal to the solver and never changes the user's Stage 1 transforms. A Stage 1 mesh auto-aligner is a separate future feature (backlog) | Sprint 12 |

## Roadmap

| Sprint | Goal | Backlog item |
|--------|------|--------------|
| 11 | Source Profiles exist and Stage 1 uses them | **Source Profiles and built-in MetaHuman profile** |
| 12 | A tested, headless auto-map solver | **Auto-map landmark solver** |
| 13 | Full Auto / Manual Stage 2 workflow | **Stage 2 auto-map workflow** |
| 14 | Users can create and reuse their own source profiles | **Custom source profiles** |
| Later | Evaluate an ML landmark solver | **ML landmark solver (evaluate)** |

## Constraints carried from AGENTS.md

- Topology, UVs, normals and vertex order are untouched; profiles only reference
  vertex indices.
- Auto-map and profile loading run off the UI thread.
- Every mutating action is undoable.
- UI uses `MvStyle`, `MvIcons` and `MvInspectorPrimitives`.
- User-facing text, logs and user docs never name the internal algorithm. Say
  "wrap engine" or "auto-map".
- Everything is local: no downloads, accounts or web calls, including for any
  future ML model.
