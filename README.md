# MetaVisage - MetaHuman Mesh Morphing Tool

An open-source desktop application for automated MetaHuman mesh morphing. This tool enables users to wrap MetaHuman head meshes to custom head models for use in Unreal Engine, providing a free alternative to expensive commercial solutions.

## Status

**Current Version:** 0.1.0 (Development)
**License:** MIT
**Platform:** Windows 10/11 (64-bit)

This project is currently in active development (Sprint 1 of 12).

## Features (Planned)

- ✅ Four-stage workflow (Alignment, Point Reference, Morph, Touch Up)
- ✅ Support for FBX, OBJ, and GLTF mesh formats
- ✅ Blender-style camera controls
- ✅ Point-based correspondence system with symmetry support
- ✅ RBF/TPS mesh deformation
- ✅ Sculpting tools for refinement
- ✅ Project save/load functionality
- ✅ Unreal Engine-ready export

## Requirements

### Build Requirements
- **Qt 6.x** - UI framework
- **CMake 3.16+** - Build system
- **C++17 compiler** - MSVC 2019+ (Windows)
- **OpenGL 4.3+** - Graphics API

### Runtime Requirements
- Windows 10/11 (64-bit)
- Intel i5/AMD Ryzen 5 or better
- 8GB RAM minimum (16GB recommended)
- DirectX 11 compatible GPU with 2GB VRAM
- 1920x1080 minimum display resolution

## Building from Source

### Prerequisites

1. Install Qt 6.x from https://www.qt.io/download
2. Install CMake from https://cmake.org/download/
3. Install Visual Studio 2019 or later with C++ development tools

### Build Steps

```bash
# Clone the repository
git clone https://github.com/YourUsername/MetaVisage.git
cd MetaVisage

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# Build
cmake --build . --config Release

# Run
./bin/MetaVisage.exe
```

## Project Structure

```
MetaVisage/
├── assets/           # Icons, shaders, sample meshes
├── build/            # Build output (generated)
├── docs/             # Documentation
│   ├── PRD.md        # Product Requirements Document
│   └── SPRINT_PLAN.md # Development roadmap
├── include/          # Header files
│   ├── core/         # Core data models
│   ├── rendering/    # Rendering system
│   └── ui/           # User interface
├── src/              # Source files
│   ├── core/         # Core implementations
│   ├── rendering/    # Rendering implementations
│   └── ui/           # UI implementations
├── tests/            # Unit tests
├── CMakeLists.txt    # Build configuration
└── README.md         # This file
```

## Development Roadmap

This project follows a 12-sprint development plan (24 weeks):

- **Sprint 1-2:** Foundation & Basic Rendering ← *Current*
- **Sprint 3:** Alignment Stage
- **Sprint 4-5:** Point Reference Stage
- **Sprint 6-7:** Morph Stage
- **Sprint 8-9:** Touch Up Stage
- **Sprint 10:** Export & Project Management
- **Sprint 11:** Polish & Optimization
- **Sprint 12:** Testing & Release

See [docs/SPRINT_PLAN.md](docs/SPRINT_PLAN.md) for detailed sprint breakdown.

## Contributing

This is an open-source project under the MIT license. Contributions are welcome!

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## Documentation

- [Product Requirements Document](docs/PRD.md) - Complete feature specifications
- [Sprint Plan](docs/SPRINT_PLAN.md) - Development roadmap
- [Claude Guide](CLAUDE.md) - AI assistant project guide

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

Casey Mershon

## Acknowledgments

- Epic Games for MetaHuman Creator
- Qt Project for the excellent UI framework
- The open-source community for inspiration and tools

---

**Note:** This project is in early development. Features are being implemented according to the sprint plan. Not all functionality is currently available.
