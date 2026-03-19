# Contributing to MetaVisage

Thank you for your interest in contributing to MetaVisage! This document provides guidelines for contributing to the project.

## Getting Started

1. Fork the repository
2. Clone your fork locally
3. Set up the development environment (see [BUILD.md](BUILD.md))
4. Create a feature branch from `master`

## Development Setup

### Prerequisites

- Qt 6.x (6.5+)
- CMake 3.16+
- C++17 compiler (MSVC 2019+ or MinGW)
- Assimp library (via vcpkg recommended)
- OpenGL 4.3+ capable GPU

See [BUILD.md](BUILD.md) for detailed build instructions.

## Making Changes

### Branch Naming

- `feature/description` - New features
- `bugfix/description` - Bug fixes
- `refactor/description` - Code refactoring
- `docs/description` - Documentation changes

### Code Style

- Use C++17 features where appropriate
- Follow the existing naming conventions:
  - `PascalCase` for class names and public methods
  - `camelCase_` with trailing underscore for private member variables
  - `UPPER_CASE` for constants and macros
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for heap allocations
- Prefer `const` references for function parameters where possible
- Keep functions focused and reasonably sized

### Architecture

MetaVisage follows a layered architecture. When adding code, place it in the appropriate layer:

- **`include/core/` and `src/core/`** - Core data models (Mesh, Project, Transform)
- **`include/deformation/` and `src/deformation/`** - Deformation algorithms
- **`include/rendering/` and `src/rendering/`** - OpenGL rendering
- **`include/sculpting/` and `src/sculpting/`** - Sculpting brush tools
- **`include/ui/` and `src/ui/`** - Qt UI components
- **`include/io/` and `src/io/`** - File I/O (import/export)
- **`include/utils/` and `src/utils/`** - Utility classes

### Commit Messages

- Use clear, descriptive commit messages
- Start with a verb in present tense: "Add", "Fix", "Update", "Remove"
- Reference issue numbers where applicable: "Fix #42: Resolve UV seam artifacts"

## Submitting Changes

1. Ensure your code builds without warnings
2. Test your changes with various mesh types and polygon counts
3. Verify UV preservation if your changes touch mesh processing
4. Push your branch to your fork
5. Open a Pull Request against `master`

### Pull Request Guidelines

- Provide a clear description of what changed and why
- Include screenshots for UI changes
- List any new dependencies
- Note any breaking changes

## Reporting Issues

When reporting bugs, please include:

- Operating system and version
- Qt version
- GPU model and driver version
- Steps to reproduce the issue
- Expected vs actual behavior
- Mesh file details (format, vertex count) if relevant

## Areas for Contribution

### High Impact

- macOS and Linux platform support
- Automated landmark detection (ML-based)
- Additional deformation algorithms
- Performance optimizations for high-poly meshes

### Medium Impact

- Advanced sculpting tools (crease, pinch, masking)
- Hair and body mesh support
- Plugin architecture
- Batch processing

### Documentation

- Tutorials and workflow guides
- Video walkthroughs
- Translation/localization

## License

By contributing to MetaVisage, you agree that your contributions will be licensed under the [MIT License](LICENSE).
