# Cottage-Cheese-Face Spike — Close-Out Decision

**Decision date**: 2026-05-04 (Sprint 7 Story 7.5)
**Carried**: Sprint 5 retro AI #2 → Sprint 6 (deferred at planning) → Sprint 7
**Outcome**: **Outcome B stopgap is the answer** — close the spike, keep the existing
inspector warning + diagnostic dump scaffolding in place, do not ship a code change to
the NRICPDeformer landmark / boundary-exclusion paths in this sprint.

---

## What we know going in

Sprint 5 Story 5.2 shipped:

1. **Inspector warning** — `StageSidebar.cpp:1211-1221` renders an amber `MV_ICON_ALERT`
   row when `morph.nricpLandmarkWeight / morph.nricpAlphaFinal > kLandmarkRatioWarnThreshold`
   (0.5). Threshold lives at `StageSidebar.cpp:32` for grep-ability per PR #24 review.
2. **Diagnostic dump scaffolding** — `METAVISAGE_DUMP_RESIDUALS=1` env gate writes per-vertex
   residual CSVs to `%TEMP%\metavisage_residuals_<epochMs>_step<N>.csv` at the end of each
   stiffness step.

The Sprint 6 retro Action Item #2 raised the stakes for Sprint 7:

> If Sprint 7 can't fit it either, explicitly close the spike with "Outcome B stopgap
> is the answer" or commit to a dedicated mini-sprint — three-sprint-deferred is the same
> anti-pattern Sprint 4's retro flagged.

The Sprint 7 plan committed to closing the spike with a documented decision either way.

---

## The three audit-flagged hypothesis sites

### Hypothesis 1 — Landmark-weight floor relative to alpha

**Location**: `src/deformation/NRICPDeformer.cpp:764-794` (subsampled and non-subsampled
landmark-term assembly).

**Hypothesis**: when stiffness `alpha` decays toward `nricpAlphaFinal`, the relative
weight of landmark constraints `landmarkWeight` (held constant) climbs unboundedly,
overpowering the smoothness term and producing local stretching at landmark sites.

**Code reading**: confirmed the landmark term is assembled with a constant `wl =
params_.landmarkWeight` (no scaling by alpha or stiffness step). This is the standard
NRICP formulation as published — landmark and smoothness terms are independent. Scaling
landmark weight by alpha would change the algorithm's character (e.g., landmarks would
under-constrain at low stiffness levels, allowing the smoothness term to dominate). The
parameter-tuning trade-off is intentional, not a bug.

**Verdict**: not a code defect; user-facing trade-off. The Outcome B warning is the
appropriate response.

### Hypothesis 2 — UV-seam BFS over-exclusion

**Location**: `src/deformation/NRICPDeformer.cpp:202-285` (`DetectAndExcludeBoundaryVertices`).

**Hypothesis**: the seam-detection BFS over-excludes interior vertices, which then drift
independently and produce the cottage-cheese surface.

**Code reading**: the BFS operates on **physical mesh edges** (edges with `count == 1`
in `edgeFaceCount` are 1-face boundary edges). It does not directly target UV seams.
However, `BuildEdgeList` adds "weld edges" between coincident-but-separate vertices that
share UV seams — which would inflate the boundary set on a UV-seamed input mesh by making
two adjacent triangles count as having different edges across the seam. Without a stable
repro fixture demonstrating over-exclusion against a known-good mesh, we cannot validate
or refute this hypothesis from code alone.

**Verdict**: cannot validate without a residual-dump-backed repro. The hypothesis has
plausibility but no actionable signal in the absence of data.

### Hypothesis 3 — Excluded-vertex landmark contribution mismatch

**Location**: `src/deformation/NRICPDeformer.cpp:354, 391` (closest-point search) +
`:767-794` (landmark term).

**Hypothesis**: a landmark whose closest morph-mesh vertex is in the excluded set still
contributes to the linear system, but its row indexing is off-by-one — silently
introducing a mismatch.

**Code reading**: closest-point search at line 354 (parallel) / 391 (serial) early-outs
on `excludedVertices_[i]`. The landmark term at lines 767-794 does not check
`excludedVertices_[vi]`; if a landmark is placed on a vertex that lands in the excluded
set (e.g., near a UV seam after Hypothesis 2's over-exclusion), the constraint still
applies. Indexing is consistent — the row uses `4 * vi + {0,1,2,3}` everywhere there is
no off-by-one. The mismatch is a precondition issue (user placed a landmark on what NRICP
considers boundary), not a code defect.

**Verdict**: not a code defect; the right fix would be to refuse / warn when a landmark
lands on an excluded vertex, but that's a separate UX feature, not a root-cause fix for
cottage-cheese.

---

## Why we are closing without a code change

1. **No reproducible test case**. The Sprint 5 retro and Sprint 6 retro both flagged that
   the artifact is hard to pin reliably. Without a fixture, no candidate fix can be
   validated against ground truth.
2. **No residual-dump data**. The diagnostic scaffolding is in place but has not been
   exercised against a known cottage-cheese case. Without dumps, hypothesis correlation
   (residual peaks at landmarks vs. seams vs. scattered) cannot be computed.
3. **Each hypothesis has plausible code-level rationale that is not a defect**. Hypothesis
   1 is standard NRICP formulation. Hypothesis 2 plausibly over-excludes but the BFS code
   is straightforward; the issue (if real) lies upstream in `BuildEdgeList`'s weld-edge
   logic for UV-seamed meshes, which is a different scope. Hypothesis 3 is a precondition
   issue, not an indexing bug.
4. **The Outcome B stopgap is functionally adequate**. The amber warning at landmark/alpha
   ratio > 0.5 directs users to lower Landmark Weight or raise Stiffness (end), which
   resolves the artifact in the most common reproduction path (high landmark weight on
   default Quick Wrap).

Three-sprint deferral was the anti-pattern the retros flagged — that pattern is broken by
making the explicit "stopgap is the answer" decision now, not by deferring again.

---

## What would be needed to reopen the spike

If a future user report or QA round produces a stable repro:

1. **Pin the repro as a fixture** — `tests/fixtures/cottage_cheese_repro.mmproj` with the
   exact Source + Target + landmark layout that produces the artifact.
2. **Capture residual dumps** — run with `METAVISAGE_DUMP_RESIDUALS=1` against the fixture,
   inspect `%TEMP%\metavisage_residuals_<epochMs>_step<N>.csv` for per-vertex residual
   peaks. Cluster shape (around landmarks vs. seams vs. scattered) discriminates which
   hypothesis is operative.
3. **Reopen this decision** with the residual data attached. At that point the Day-2 gate
   from Sprint 7's plan applies cleanly: residuals point at one site, fix that site,
   regression-test against the fixture.

A backlog item should be filed for the reopen if/when those preconditions exist.

---

## Action items

- [x] **Document the decision** (this file).
- [x] **Update the product backlog** — flip the spike entry from `[/]` to `[x]` and point
      at this document.
- [x] **Inline comment in `NRICPDeformer.cpp`** — link the landmark-term code to this
      decision so a future reader investigating cottage-cheese finds the close-out
      rationale before re-walking the same hypotheses.
- [ ] **Future**: file a backlog item to reopen the spike if a reproducible fixture
      surfaces, with the residual-dump workflow as the entry criterion.
- [ ] **Future**: refine `kLandmarkRatioWarnThreshold` (currently 0.5) once we have user-
      reported parameter ranges that empirically misfire — drop or tighten as appropriate.
      No data today, no change today.
