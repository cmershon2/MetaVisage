# Standalone Open-Source PR Review

**Date**: 2026-10-15
**Target**: `feat/open-source-standalone` into `development`
**Method**: Local diff and source review using the review-pr skill. No delegated review.

## Findings

- [P2, fixed] `CMakeLists.txt`: `smoke_ui` used the common `build/bin` working
  directory, while MSVC stages shaders under the selected configuration directory.
  UI assertions could pass with shader-load warnings. Use the test executable's
  target directory; the final standard CTest run loads shaders without warnings.
- [P3, fixed] `src/ui/AppWindow.cpp`: removing the remote/account Help entries
  left an unnecessary leading separator. Removed it to keep the menu coherent.

## Review Scope

Checked startup/shutdown and single-instance behavior, unconditional feature
availability across inspector/ToolRail/viewport hotkeys, export limit/reporting
removal, local settings persistence, removed service helpers/build dependencies,
key/profile configuration, user guidance, and focused branch history. The existing
toolchain argument fix remains intact. No runtime service endpoint or license/gate
reference remains in source/build manifests.

## Validation

- MSVC Release build passed through `build-msvc.ps1 -Deploy -VcpkgToolchain`.
- Full CTest: 4/4 targets passed in 37.39 seconds.
- Live UI lane: 5/5 scenarios, including unrestricted brush hotkeys/viewport
  selection, without auth fixtures or warning/error logs.
- `git diff --check` passed; PowerShell parser/dependency checks are recorded in
  the transition evidence. The executable imports neither libcurl nor OpenSSL.

## Residual Risk

MinGW was not built. Full loaded-mesh Process/Accept and sculpt/undo UI automation,
and clean-machine release package testing, remain tracked follow-ups. Existing
MSVC deprecation warnings are unrelated to this removal. UI consistency was
reviewed in code and through the live harness; no saved visual capture is claimed.

## Verdict

Approve. No outstanding material findings after the fixes above. This is a local
review verdict, not a GitHub approval submitted on the owner's own PR.

**Pull request**: [#27](https://github.com/cmershon2/MetaVisage/pull/27), targeting `development`.
