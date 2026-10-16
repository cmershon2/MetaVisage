# Open-source documentation review

**Date**: 2026-10-16
**Branch**: `development`
**Scope**: Public README, build guides, contribution guide, release notes,
known issues, and the boundary between current and historical maintainer context.

## Findings resolved

- **High — obsolete build requirements:** `README_BUILD.md` and
  `CONTRIBUTING.md` required Qt and C++17. Replaced them with the validated
  Visual Studio 2022/C++20/vcpkg workflow and current ImGui architecture.
- **High — misleading onboarding:** The README cloned the default branch and
  mixed Bash and Windows command syntax. It now explicitly selects the
  standalone `development` branch and uses PowerShell and the build helper.
- **Medium — outdated public feature descriptions:** Removed account/login
  smoke-test descriptions, deleted configuration paths, old algorithm choices,
  and the internal wrapping algorithm name from public documentation.
- **Medium — incorrect graphics and dependency claims:** Replaced DirectX
  requirements with OpenGL 4.3+, corrected compiler requirements, and removed
  claims that the dependency manifest pins versions. Helpers require an explicit
  toolchain argument on a fresh build; they do not consume `VCPKG_ROOT` directly.
- **Medium — unsupported known-issue claims:** Removed retired Qt layout and
  RBF solver notes and blanket file-I/O exception claims. Documented current
  morph parameter trade-offs, missing auto-save/recovery, and validation limits.
- **Medium — historical guidance presented as current:** Added retirement
  notices to all four desktop web contracts and a scope note to the v1 PRD.
  Kept historical records intact. Release notes distinguish the standalone
  development work from v1.0.0 without inventing a release date.
- **Low — contribution friction:** Updated the PR base to `development`,
  architecture rules, Conventional Commits, meaningful validation guidance,
  log location, and expectations for shareable fixtures and asset provenance.

## Remaining release-readiness findings

1. **High — bundled asset provenance and notices are incomplete.** The tracked
   reference head, fonts (Inter, JetBrains Mono, Saira, Lucide), and other assets
   lack adjacent source/redistribution records. The tracked top-level license
   inventory contains only `LICENSE`; stb headers retain embedded license text.
   Record the exact sources, versions, applicable terms, and attribution for
   bundled assets, and gather notices for dependencies included in binary
   packages. Repository evidence does not establish redistribution permission
   for the reference head. The README no longer presents it as unconditionally
   ready to redistribute. This review does not change `LICENSE` or assert asset
   permissions that have not been verified.
2. **Medium — clean-machine package validation is pending.** The Sprint 8
   retrospective and backlog already track this. Verify DLLs, shaders, fonts,
   brush previews, the reference mesh, and local export before publishing the
   next package. A development-machine build is not evidence of package readiness.
3. **Medium — dependency versions are not reproducibly pinned.** `vcpkg.json`
   has no `builtin-baseline` or version constraints. Documented accurately;
   pinning and validating a dependency baseline is a separate build change.
4. **Low — broader UI and toolchain coverage remains pending.** Existing
   follow-ups cover loaded-mesh Process/Accept and sculpt/undo UI automation.
   MinGW has not been validated for the standalone release. Public guides now
   state these limits without promising cross-platform support or a release date.

## Validation

Checked commands and claims against `CMakeLists.txt`, `vcpkg.json`, both build
helpers, mesh export options/dialogs, settings/log/preset paths, the live UI
smoke registrations, and Sprint 8 validation records. Documentation validation
checks local Markdown links/anchors, PowerShell syntax in documented command
blocks, obsolete public terminology, and `git diff --check`.

- Local link/anchor audit: 25 references across 13 reviewed documents passed.
- PowerShell parser: all 8 documented command blocks parsed without errors;
  setup/build commands were not executed by this review.
- Public obsolete-terminology check and `git diff --check`: passed.
- Targeted credential-pattern scan of tracked project Markdown (excluding the
  vendored boilerplate): no matches. This is not a Git-history secret audit.

No application code or assets changed. No fresh build, application smoke run,
external URL availability check, or third-party licensing determination is
claimed by this documentation review. Historical sprint documents and vendored
agent boilerplate were not rewritten or treated as current public instructions.
