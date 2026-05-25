![MetaVisage Banner](https://github.com/cmershon2/MetaVisage/blob/master/docs/MetaVisage_Banner.png?raw=true)

An open-source desktop application for automated MetaHuman mesh morphing. Wrap MetaHuman head meshes to custom head models for use in Unreal Engine.

## Features

- **Four-stage workflow**: Alignment, Point Reference, Morph, Touch Up
- **Non-rigid surface registration** for high-quality mesh wrapping
- **Wrap presets**: Quick Wrap (rigid, for human-like targets) and Fantasy Wrap (looser, for dramatic features) ship as built-ins; users can save their own tuned parameter sets as named presets with a 12-glyph picker, then rename / replace / delete via right-click on the preset card
- **Hierarchical Stage 3 inspector**: collapsible General + Advanced parameter sections with sub-sections (Interior Geometry, Optimization, Rigidity, Control Node Sampling); collapsed/expanded state persists across sessions
- **Sculpting tools**: Smooth, Grab, Push/Pull, and Inflate brushes for manual refinement, with visual brush-effect previews and `1`/`2`/`3`/`4` hotkeys
- **Symmetry support**: Mirrored point placement and sculpting
- **Blender-style controls**: Familiar orbit/pan/zoom camera and G/R/S transform shortcuts
- **Multiple formats**: Import/export FBX, OBJ, and GLTF meshes
- **Unreal Engine ready**: Exports preserve topology, UVs, normals, and material slots — quad topology is preserved end-to-end for UE5 MetaHuman Conform compatibility
- **Project management**: Save/load workflow state with full undo/redo support; v1.0.0 `.mmproj` files load without modification
- **Included assets**: Ships with a MetaHuman head mesh (LOD0) ready to use
- **Browser-based sign-in (v2.0)**: First launch hands off to the system browser via the `metavisage://` URL handler; the desktop intercepts the redirect, exchanges the code for a signed RS256 license JWT, and caches it (DPAPI-encrypted) for a 7-day offline grace window. License Status modal exposes tier / usage / Sign Out

## Requirements

### Runtime
- Windows 10/11 (64-bit)
- Intel i5 / AMD Ryzen 5 or better
- 8 GB RAM minimum (16 GB recommended)
- DirectX 11 compatible GPU with 2 GB VRAM
- 1920x1080 minimum display resolution

### Build
- CMake 3.16+
- **C++20** compiler — MSVC 2019+ (19.30+) or MinGW with C++20 support
- OpenGL 4.3+
- vcpkg for dependencies (manifest pinned in [vcpkg.json](vcpkg.json)):
  - GLFW3, Dear ImGui (with `glfw-binding` + `opengl3-binding` + `docking-experimental` + `test-engine`), GLEW
  - nlohmann/json, native-file-dialog-extended (NFD), libcurl
  - Assimp, Eigen3
  - jwt-cpp + OpenSSL (RS256 license JWT verification — Sprint 6)
- **No Qt dependency.** Earlier versions linked `Qt6::Core` transitionally; Sprint 3.7 removed it entirely. The CMake configure-time guard fails the build if any Qt target is reintroduced.

## Quick Start

### Building from Source

```bash
git clone https://github.com/cmershon2/MetaVisage.git
cd MetaVisage
mkdir build && cd build

# MSVC
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Run the full smoke suite (load → serialize → reload → export round-trip,
# v1.0.0 backward-compat regression, Wrap Engine behavioural-parity, and
# imgui_test_engine click-through smoke against the live AppWindow)
ctest -C Release

# Or filter to just the imgui_test_engine UI lane
ctest -C Release -L ui

# Launch
./bin/Release/MetaVisage.exe
```

See [BUILD.md](BUILD.md) for detailed instructions (MinGW, vcpkg setup, troubleshooting).

### Using the Build Scripts

```powershell
# MSVC build, development profile (default; reads .env.development)
.\build-msvc.ps1 -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# MSVC build, staging profile (reads .env.staging — gitignored)
.\build-msvc.ps1 -Profile staging -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# MinGW build
.\build-mingw.ps1 -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

**Build profile** (Sprint 6+): the `-Profile` flag selects which `.env.<profile>` is read into CMake at configure time. `.env.development` ships with localhost defaults and the non-secret Supabase dev creds; `.env.staging` / `.env.production` and `keys/*.pem` (the RS256 license-JWT public keys) are gitignored and delivered out-of-band. CMake refuses to configure non-development profiles without a real API URL, Supabase URL, anon key, and public key.

## Workflow

1. **Alignment** - Load your MetaHuman head mesh and custom target mesh, then align them using move (G), rotate (R), and scale (S) tools
2. **Point Reference** - Place corresponding landmark points on both meshes in synchronized dual viewports
3. **Morph** - Run the deformation algorithm to automatically wrap the MetaHuman mesh to your target shape
4. **Touch Up** - Refine the result with sculpting brushes (smooth, grab, push/pull, inflate)
5. **Export** - Save the morphed mesh as FBX or OBJ, ready for Unreal Engine (quad topology preserved for MetaHuman Conform)

## Included Assets

A MetaHuman head mesh (LOD0) is included in `assets/models/MetaHumanHeadLod0.obj` for immediate use as the morph source mesh.

## Project Structure

```
MetaVisage/
├── assets/           # Fonts (Inter / JetBrains Mono / Saira / Lucide), shaders,
│                     # gizmo models, default MetaHuman head, app icons, logo mark,
│                     # Stage 4 brush-effect preview PNGs (assets/brushes/)
├── cmake/            # configure_file templates (LicenseConfig.h.in)
├── docs/             # Banner, KNOWN_ISSUES.md
├── include/          # Public headers — baking/, core/, deformation/, io/,
│                     # licensing/, rendering/, sculpting/, thirdparty/, ui/, utils/
├── keys/             # RS256 license-JWT public keys per environment
│                     # (development/staging/production; PEMs gitignored)
├── src/              # Implementation (mirrors include/ structure) + main.cpp
├── tests/            # CTest suite — four smoke executables plus pinned v1.0.0
│                     # fixtures: load → serialize → export round-trip, v1.0.0
│                     # backward-compat regression, Wrap Engine behavioural-parity
│                     # over Quick + Fantasy presets, and an imgui_test_engine
│                     # click-through smoke against the live AppWindow (6 scenarios:
│                     # welcome modal / dockspace / shading-mode / stage stepper
│                     # / brush-gate clamp / login cycle)
├── .agents/          # Codex context, rules, skills, memory, boilerplate baseline
├── .codex/           # Codex custom agents, hooks, and local runtime config
├── .env.example      # Build-profile template (API URL, Supabase URL, anon key, key file)
├── .env.development  # Localhost dev profile (committed; non-secret defaults)
├── CMakeLists.txt    # Build configuration
├── vcpkg.json        # Pinned dependency manifest (jwt-cpp + openssl as of Sprint 6)
├── build-msvc.ps1    # MSVC build helper (-Profile + -Deploy flags)
├── build-mingw.ps1   # MinGW build helper (-Profile flag)
├── BUILD.md          # Detailed build instructions
├── CONTRIBUTING.md   # Contribution guidelines
└── LICENSE           # MIT License
```

The UI lives under `include/ui/` & `src/ui/`: shell hosts (`AppWindow`, `Viewport`, `ViewportHost`, `StageSidebar`, `StageStepper`, `ToolRail`, `Modals`, `FileDialogs`, `LoginScreen`) plus the design-system primitives (`MvStyle` for tokens + brand asset texture handles, `MvIcons` for the Lucide icon font, `MvInspectorPrimitives` for shared section / sub-section / row helpers). The licensing layer (`include/licensing/`, `src/licensing/`) owns the auth state machine, browser-based OAuth via the `metavisage://` URL handler, RS256 JWT verification, and the 24h refresh thread that keeps tokens current.

## Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Known Issues

See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for a list of known issues and platform limitations.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

Casey Mershon

## Acknowledgments

- Epic Games for MetaHuman Creator
- Dear ImGui, GLFW, and GLEW for the UI/windowing/GL stack
- Eigen for linear algebra
- Assimp for mesh I/O
- nlohmann/json, native-file-dialog-extended, libcurl, WinSparkle
