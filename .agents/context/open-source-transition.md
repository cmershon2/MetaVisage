# Standalone open-source transition

**Date**: 2026-10-15
**Branch**: `feat/open-source-standalone`, based on `development`
**Authorization**: Owner requested removal of login, account, payment gates,
and all web integrations; the web-integrated version is archived.

This started as a direct implementation task outside the completed Sprint 7 plan.
At the owner's closeout request it is recorded as [Sprint 8](sprints/sprint-08.md).
Historical sprint/backlog/integration documents remain as records; commercial
roadmap items no longer govern implementation. The MIT license is unchanged.

Removed runtime authentication/licensing and account UI, tier-based feature
restrictions, conversion limits/reporting/queue, browser auth/deep links,
remote documentation/update actions, HTTP/JWT/DPAPI helpers and dependencies,
and web environment/public-key configuration. Existing local settings ignore
legacy auth fields and omit them on subsequent saves. No existing user data
or registry entries are deleted by this change.

The original working tree included toolchain argument fixes in both build
helpers. Those fixes are retained while removing their credential profiles.

## Validation

- MSVC Release build passed through `build-msvc.ps1 -Deploy -VcpkgToolchain
  "C:/Users/Casey Mershon/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake"`.
- `ctest --test-dir build -C Release --output-on-failure`: 4/4 targets passed
  in 37.39 seconds on the final review build (mesh round-trip/export, v1 compatibility, wrap parity, UI).
- `metavisage_smoke_ui.exe` from `build/bin/Release`: 5/5 UI scenarios passed
  with staged shaders loaded and no warning/error logs. This additional run
  established the CTest working-directory issue, which is now fixed using the
  selected configuration's executable directory. The final standard CTest UI
  run also loads staged shaders without warnings.
- Standalone UI coverage uses no auth fixture; verifies the workspace renders,
  all four stages mount, and every brush hotkey/viewport selection remains
  available without clamping.
- Both PowerShell build scripts passed parser checks; `git diff --check` passed.
- `dumpbin /dependents` confirms the application no longer imports libcurl or
  OpenSSL. Source/build manifests no longer reference runtime web endpoints,
  the license manager, feature flags, HTTP client, or secure token storage.
- MinGW was not built. Existing MSVC enum-combination deprecation warnings remain.

The owner authorized commits, pushes, a reviewed PR into `development`, and merge.
See [review](reviews/open-source-standalone.md) and
[retrospective](retrospectives/retro-sprint-08.md) for final evidence.


**Pull request**: [#27](https://github.com/cmershon2/MetaVisage/pull/27), targeting `development`.
