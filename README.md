![MetaVisage Banner](https://github.com/cmershon2/MetaVisage/blob/master/docs/MetaVisage_Banner.png?raw=true)

An open-source desktop application for automated MetaHuman mesh morphing. Wrap MetaHuman head meshes to custom head models for use in Unreal Engine.

## Features

- **Four-stage workflow**: Alignment, Point Reference, Morph, Touch Up
- **Non-rigid surface registration** for high-quality mesh wrapping
- **Sculpting tools**: Smooth, Grab, Push/Pull, and Inflate brushes for manual refinement
- **Symmetry support**: Mirrored point placement and sculpting
- **Blender-style controls**: Familiar orbit/pan/zoom camera and G/R/S transform shortcuts
- **Multiple formats**: Import/export FBX, OBJ, and GLTF meshes
- **Unreal Engine ready**: Exports preserve topology, UVs, normals, and material slots — quad topology is preserved end-to-end for UE5 MetaHuman Conform compatibility
- **Project management**: Save/load workflow state with full undo/redo support
- **Included assets**: Ships with a MetaHuman head mesh (LOD0) ready to use

## Requirements

### Runtime
- Windows 10/11 (64-bit)
- Intel i5 / AMD Ryzen 5 or better
- 8 GB RAM minimum (16 GB recommended)
- DirectX 11 compatible GPU with 2 GB VRAM
- 1920x1080 minimum display resolution

### Build
- CMake 3.16+
- C++17 compiler — MSVC 2019+ or MinGW
- OpenGL 4.3+
- vcpkg for dependencies (manifest pinned in [vcpkg.json](vcpkg.json)):
  - GLFW3, Dear ImGui (with `glfw-binding` + `opengl3-binding`), GLEW
  - nlohmann/json, native-file-dialog-extended (NFD), libcurl
  - Assimp, Eigen3
- Qt 6.10+ — `Qt6::Core` only, linked transitionally for QString/QFile/QJson (full removal tracked in the backlog)

## Quick Start

### Building from Source

```bash
git clone https://github.com/cmershon2/MetaVisage.git
cd MetaVisage
mkdir build && cd build

# MSVC
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.3/msvc2022_64" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Run
./bin/Release/MetaVisage.exe
```

See [BUILD.md](BUILD.md) for detailed instructions (MinGW, Qt Creator, vcpkg setup, troubleshooting).

### Using the Build Scripts

```powershell
# MSVC build with runtime DLL staging (Qt6::Core + GLFW/ImGui/GLEW/etc.)
.\build-msvc.ps1 -Deploy

# MinGW build
.\build-mingw.ps1
```

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
├── assets/           # Shaders, gizmo models, default MetaHuman head, app icons
├── docs/             # Banner, KNOWN_ISSUES.md
├── include/          # Public headers — baking/, core/, deformation/, io/,
│                     # rendering/, sculpting/, thirdparty/, ui/, utils/
├── src/              # Implementation (mirrors include/ structure) + main.cpp
├── .claude/          # Sprint plans, retros, agents, hooks, skills (dev tooling)
├── CMakeLists.txt    # Build configuration
├── vcpkg.json        # Pinned dependency manifest
├── build-msvc.ps1    # MSVC build helper (use -Deploy to stage runtime DLLs)
├── build-mingw.ps1   # MinGW build helper
├── BUILD.md          # Detailed build instructions
├── CONTRIBUTING.md   # Contribution guidelines
└── LICENSE           # MIT License
```

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
