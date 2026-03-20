![MetaVisage Banner](https://github.com/cmershon2/MetaVisage/blob/master/docs/MetaVisage_Banner.png?raw=true)

An open-source desktop application for automated MetaHuman mesh morphing. Wrap MetaHuman head meshes to custom head models for use in Unreal Engine.

## Features

- **Four-stage workflow**: Alignment, Point Reference, Morph, Touch Up
- **NRICP deformation**: Non-Rigid ICP algorithm for high-quality surface registration (plus RBF/TPS alternatives)
- **Sculpting tools**: Smooth, Grab, Push/Pull, and Inflate brushes for manual refinement
- **Symmetry support**: Mirrored point placement and sculpting
- **Blender-style controls**: Familiar orbit/pan/zoom camera and G/R/S transform shortcuts
- **Multiple formats**: Import/export FBX, OBJ, and GLTF meshes
- **Unreal Engine ready**: Exports preserve topology, UVs, normals, and material slots
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
- Qt 6.x (6.5+)
- CMake 3.16+
- C++17 compiler — MSVC 2019+ or MinGW
- Assimp (recommended via vcpkg)
- Eigen3
- OpenGL 4.3+

## Quick Start

### Building from Source

```bash
git clone https://github.com/cmershon2/MetaVisage.git
cd MetaVisage
mkdir build && cd build

# MSVC
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/msvc2022_64" -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Run
./bin/Release/MetaVisage.exe
```

See [BUILD.md](BUILD.md) for detailed instructions (MinGW, Qt Creator, vcpkg setup, troubleshooting).

### Using the Build Scripts

```powershell
# MSVC build with Qt dependency deployment
.\build-msvc.ps1 -Deploy

# MinGW build
.\build-mingw.ps1
```

## Workflow

1. **Alignment** — Load your MetaHuman head mesh and custom target mesh, then align them using move (G), rotate (R), and scale (S) tools
2. **Point Reference** — Place corresponding landmark points on both meshes in synchronized dual viewports
3. **Morph** — Run the NRICP deformation algorithm to automatically wrap the MetaHuman mesh to your target shape
4. **Touch Up** — Refine the result with sculpting brushes (smooth, grab, push/pull, inflate)
5. **Export** — Save the morphed mesh as FBX or OBJ, ready for Unreal Engine

## Included Assets

A MetaHuman head mesh (LOD0) is included in `assets/models/MetaHumanHeadLod0.obj` for immediate use as the morph source mesh.

## Project Structure

```
MetaVisage/
├── assets/           # Shaders, models, icons
├── docs/             # Documentation (PRD, sprint plan, known issues)
├── include/          # Header files (core, deformation, rendering, sculpting, ui, io, utils)
├── src/              # Source files (mirrors include/ structure)
├── tests/            # Unit tests
├── CMakeLists.txt    # Build configuration
├── BUILD.md          # Detailed build instructions
├── CONTRIBUTING.md   # Contribution guidelines
└── LICENSE           # MIT License
```

## Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Known Issues

See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for a list of known issues and platform limitations.

## License

MIT License — see [LICENSE](LICENSE) for details.

## Author

Casey Mershon

## Acknowledgments

- Epic Games for MetaHuman Creator
- Qt Project for the UI framework
- Eigen for linear algebra
- Assimp for mesh I/O
