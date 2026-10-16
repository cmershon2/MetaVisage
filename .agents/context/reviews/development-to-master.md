# Development-to-master promotion review

**Date**: 2026-10-16
**PR**: [#28](https://github.com/cmershon2/MetaVisage/pull/28)
**Base / head**: `master` / `development`
**Scope**: Accumulated desktop rewrite, standalone restoration, and documentation
updates. The repository default branch is `master`; there is no `main` branch.
**Method**: Local source/diff review using `review-pr`; no delegated review or
self-approval submitted through GitHub.

## Findings

- **P1, fixed — stale morph result applied to another project:** New/Open could
  replace the workspace while a morph worker ran. Completion then wrote its
  mesh into whichever project happened to be current. Track project generations
  and discard results belonging to an earlier workspace. A tiny mesh UI fixture
  checks both normal completion and project replacement before completion.
- **P2, fixed — internal implementation name exposed:** Solver progress/log
  strings, warning source metadata, and CMake OpenMP messages still exposed the
  internal wrapping name. Use the public Wrap Engine name and a public diagnostic
  source label. Regression assertions cover progress messages and log metadata.
- **P3, fixed — Windows path conversion result unchecked:** Allocate the full
  UTF-8 output count, check conversion success, then trim the trailing terminator
  from null-terminated input. This makes the Windows API buffer contract explicit.

## Review coverage

Reviewed branch history/change inventory, earlier sprint review evidence, mesh
and transform changes, morph input/result lifetimes, project persistence,
undo-stack ownership, preset path validation, startup/shutdown, asset staging,
build helpers, removed runtime services/gates, tests, and public documentation.
The promotion includes the historical work already reviewed into development;
it is not represented as a documentation-only diff or an exhaustive visual audit.

## Validation

- MSVC Release build passed through `build-msvc.ps1 -Deploy -VcpkgToolchain`.
  Existing MSVC deprecation warnings remain; no build errors.
- `ctest --test-dir build -C Release --output-on-failure`: 4/4 passed in
  50.56 seconds. The UI target passed all six scenarios in 3.10 seconds,
  including the tiny-mesh worker/project replacement regression.
- Mesh/export smoke checks public diagnostic source metadata; both wrap presets
  check that progress messages use public naming.
- Local Markdown link/anchor audit: 26 references across 14 documents passed.
- Targeted credential-pattern scan of added/modified text: no matches.
- `git diff --check`: passed. No external URL availability or Git-history
  secret audit is claimed.

## Existing limitations and release follow-ups

- Asset provenance/notices, dependency baseline pinning, clean-machine packages,
  MinGW validation, and broader loaded-asset UI automation remain documented in
  [the documentation review](open-source-documentation.md) and backlog.
- Mesh import, project save/load, baking, and export are synchronous, as in the
  former main-branch application. The public known-issues page now records this
  existing responsiveness limitation. It is not resolved by the background-morph
  ownership fix; remaining heavy I/O still needs migration to workers.
- Source merge does not publish a binary release, validate redistribution rights,
  or change repository visibility. Preserve the `development` branch on merge.

## Verdict

Approve for source promotion after the fixes above. Existing release-preparation
and legacy responsiveness limitations are explicitly retained above; this is not
binary-release or redistribution signoff.
