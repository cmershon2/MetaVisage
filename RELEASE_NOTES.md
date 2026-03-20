# MetaVisage v1.0.0 Release Notes

**Release Date:** March 2026
**Platform:** Windows 10/11 (64-bit)
**License:** MIT

## Overview

MetaVisage v1.0.0 is the initial release of the open-source MetaHuman mesh morphing tool. It provides a complete workflow for wrapping MetaHuman head meshes to custom head models for use in Unreal Engine.

## Features

### Four-Stage Workflow
- **Alignment**: Load and position meshes with move/rotate/scale tools and Blender-style gizmos
- **Point Reference**: Place correspondence landmarks in synchronized dual viewports with symmetry support
- **Morph**: Deform the MetaHuman mesh using NRICP (Non-Rigid ICP), RBF-TPS, RBF-Gaussian, or RBF-Multiquadric algorithms
- **Touch Up**: Refine results with Smooth, Grab, Push/Pull, and Inflate sculpting brushes

### Deformation Algorithms
- **NRICP (Recommended)**: Non-Rigid Iterative Closest Point for high-quality surface registration with configurable stiffness schedule, boundary exclusion, and landmark weighting
- **RBF-TPS**: Radial Basis Function with Thin-Plate Spline kernel
- **RBF-Gaussian**: Radial Basis Function with Gaussian kernel
- **RBF-Multiquadric**: Radial Basis Function with Multiquadric kernel

### Rendering & Visualization
- OpenGL 4.3 Core Profile rendering
- MatCap shading for detail visualization
- Heatmap overlay showing deformation displacement
- Multiple preview modes: original, deformed, overlay
- Real-time FPS counter and mesh statistics

### Import/Export
- Import: FBX, OBJ, GLTF formats via Assimp
- Export: FBX and OBJ with preserved topology, UVs, normals, and material slots
- Unreal Engine naming conventions and coordinate system support
- UV seam handling with proper vertex splitting

### Project Management
- Save/load project state (.mmproj format)
- Full undo/redo system for all operations
- Logging system with file rotation

### Included Assets
- MetaHuman head mesh (LOD0) included in `assets/models/`

## System Requirements

- Windows 10/11 (64-bit)
- Intel i5 / AMD Ryzen 5 or better
- 8 GB RAM (16 GB recommended)
- DirectX 11 GPU with 2 GB VRAM
- 1920x1080 display resolution

## Known Limitations

- Windows only (macOS/Linux planned for future releases)
- No auto-save or crash recovery
- No automated landmark detection
- See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for full list

## Building from Source

See [BUILD.md](BUILD.md) for detailed build instructions. Requires Qt 6.x, CMake 3.16+, and a C++17 compiler.
