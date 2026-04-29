---
name: Sprint 4 Story 4.1 Pre-Implementation Audit
description: Full audit of RBFInterpolator reach, MeshDeformer dispatch, enum comparison sites, ProjectSerializer contract, user-visible strings, and test infrastructure as of Sprint 4 start (2026-04-29)
type: project
---

## RBFInterpolator Status

Dead code at runtime, still compiles. Only include site in live code: `include/deformation/MeshDeformer.h:8`. Instantiated in `MeshDeformer::Deform()` legacy RBF block (lines 198-329 in MeshDeformer.cpp) which is unreachable because `kernelType_` is always `NRICP`.

**Why:** Sprint 3.8.9 removed the algorithm dropdown from the UI; the preset cards always apply `MorphData` defaults which initialize `algorithm = NRICP`. No UI path can set a non-NRICP algorithm value.

## DeformationAlgorithm Enum (include/core/Project.h:71-77)

Current values: `RBF_TPS`, `RBF_GAUSSIAN`, `RBF_MULTIQUADRIC`, `ARAP`, `NRICP`
`ARAP` has zero code references (one comment in NRICPDeformer.cpp:793 uses "ARAP" as a math term, not an enum).
After Story 4.1: only `NRICP` survives.

## MeshDeformer Dispatch (src/deformation/MeshDeformer.cpp)

- Line 103-110: `ValidateInputs()` — two branches for NRICP vs RBF min-points check. Collapses to minPoints=1.
- Line 194: `if (kernelType_ == NRICP) return DeformNRICP()` + legacy block lines 198-329. Becomes unconditional `return DeformNRICP()`.
- Only external caller: `AppWindow.cpp:2303` `morphDeformer_->SetKernelType(morphData.algorithm)`

No other switch/if over `DeformationAlgorithm` outside RBFInterpolator.cpp.

## ProjectSerializer Contract

- **Write:** `src/io/ProjectSerializer.cpp:284` — `obj["algorithm"] = static_cast<int>(data.algorithm)`
- **Read:** `src/io/ProjectSerializer.cpp:457-460` — raw `static_cast<DeformationAlgorithm>(GetOr<int>(..., NRICP_default))`
- **Risk:** After enum shrinks, old int values 0/1/2/3 produce out-of-range enum. Must add explicit coercion guard: read the int, always set `data.algorithm = DeformationAlgorithm::NRICP`.
- **Usage:** The loaded algorithm field is ONLY used at AppWindow.cpp:2303 for SetKernelType — no other dispatch.

## User-Visible Strings with Algorithm Names

Logger is NOT user-visible (file+console only, no in-app panel).
Progress callback strings are NOT user-visible (AppWindow lambda discards the string, stores only float).

**Only 3 user-visible algorithm-name strings exist:**
1. `src/deformation/MeshDeformer.cpp:106` — `"At least 1 point correspondence required for NRICP"` — surfaces via error modal (AppWindow.cpp:2382)
2. `src/deformation/MeshDeformer.cpp:108` — `"...for stable RBF deformation"` — dead code, deleted with RBF block
3. `src/deformation/MeshDeformer.cpp:412` — `"NRICP solver failed"` — surfaces via error modal

No hits in: AppWindow.cpp user strings, StageSidebar.cpp rendered strings (all occurrences are comments), Modals.cpp, FileDialogs.cpp.

## Test Infrastructure

- `tests/fixtures/` does NOT exist — must be created for Story 4.1 backward compat test.
- Only test: `tests/smoke/launch_to_export.cpp` — headless, links MetaVisageCore static lib, no GL/ImGui.
- CMakeLists pattern: `add_executable + target_link_libraries(PRIVATE MetaVisageCore) + add_test(NAME ... COMMAND ...)`
- Asset resolution via Paths::ExecutableDir() + candidate path walk.
- Fixture files can be minimal JSON: just need `{"morph": {"algorithm": 0}}` etc. to test coercion.

## Sprint 4.1 CMakeLists.txt Changes Required

Remove from CORE_SOURCES: `src/deformation/RBFInterpolator.cpp` (line 127)
Remove from CORE_HEADERS: `include/deformation/RBFInterpolator.h` (line 178)
