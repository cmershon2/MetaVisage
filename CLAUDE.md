# CLAUDE.md - MetaVisage Project Guide

## Project Overview

**MetaVisage** is an open-source desktop application for automated MetaHuman mesh morphing. It enables users to wrap MetaHuman head meshes to custom head models for use in Unreal Engine, providing a free alternative to expensive commercial solutions like Faceform's Wrapped ($500/month).

**License:** MIT
**Platform:** Windows 10/11 (64-bit)
**Status:** MVP Development Phase
**Author:** Casey Mershon

## Core Purpose

This tool addresses the significant cost barrier in creating custom MetaHuman heads by providing a complete workflow for:
1. Aligning custom head meshes with MetaHuman base meshes
2. Defining correspondence points between meshes
3. Automatically morphing the MetaHuman mesh to match the custom head
4. Refining the result with sculpting tools
5. Exporting Unreal Engine-ready assets

## Technical Stack

### Development Framework
- **Language:** C++17 or later
- **UI Framework:** Qt 6.x (cross-platform ready) or Dear ImGui
- **Graphics API:** OpenGL 4.3+

### Key Libraries
- **Mesh I/O:** Assimp (FBX, OBJ, GLTF support)
- **Mesh Processing:** libigl, OpenMesh, or CGAL
- **Deformation:** Custom RBF/TPS implementation using Eigen
- **Math:** GLM or Eigen for linear algebra
- **Serialization:** JSON (nlohmann/json) for project files

### Hardware Requirements
- **Processor:** Intel i5/AMD Ryzen 5 or better
- **RAM:** 8GB minimum, 16GB recommended
- **Graphics:** DirectX 11 compatible GPU with 2GB VRAM
- **Display:** 1920x1080 minimum resolution

## Architecture Overview

The application follows a **layered architecture**:

```
┌─────────────────────────────────────┐
│        Presentation Layer           │
│  (Qt Widgets / ImGui Interface)     │
├─────────────────────────────────────┤
│         Application Layer           │
│  (Workflow Controllers, Commands)   │
├─────────────────────────────────────┤
│          Domain Layer               │
│  (Mesh Processing, Deformation)     │
├─────────────────────────────────────┤
│       Infrastructure Layer          │
│  (File I/O, Rendering, Persistence) │
└─────────────────────────────────────┘
```

### Key Components
1. **UI Layer:** MainWindow, ViewportWidget, SidebarWidget
2. **Workflow Controller:** StageManager, stage-specific controllers
3. **Command Manager:** Undo/redo implementation (Command pattern)
4. **Mesh Engine:** Loading, validation, optimization, export
5. **Deformation Algorithm:** RBF interpolation with TPS kernel
6. **Sculpting Engine:** Brush-based mesh editing tools
7. **Renderer:** OpenGL-based 3D visualization
8. **Project Manager:** Save/load functionality

## Four-Stage Workflow

### Stage 1: Alignment
- Load morph mesh (MetaHuman head) and target mesh (custom head)
- Transform target mesh to align with morph mesh using:
  - Move tool (G key)
  - Rotate tool (R key)
  - Scale tool (S key)
- Visual gizmos with Blender-style controls
- Transform locks to X/Y/Z axes

### Stage 2: Point Reference
- Dual synchronized viewports (target mesh left, morph mesh right)
- Place correspondence points by clicking mesh surfaces
- Points auto-numbered sequentially
- Symmetry mode for mirrored point placement
- Point counts must match before proceeding
- Camera movements synchronized across both viewports

### Stage 3: Morph
- RBF deformation algorithm with adjustable parameters:
  - Stiffness (0.0-1.0): Controls mesh rigidity
  - Smoothness (0.0-1.0): Blends point influence regions
  - Kernel type: TPS, Gaussian, or Multiquadric
- Threaded computation with progress indication
- Preview modes: original, deformed, overlay, heat map
- Preserves topology, UVs, and normals

### Stage 4: Touch Up
- Sculpting brushes for mesh refinement:
  - **Smooth Brush:** Average vertex positions
  - **Grab Brush:** Move vertices with falloff
  - **Push/Pull Brush:** Move along surface normal
  - **Inflate/Deflate Brush:** Expand/contract surface
- Brush parameters: radius, strength, falloff
- Symmetry mode for mirrored sculpting
- MatCap shading for detail visualization

## File Format Support

### Import
- **FBX** (primary MetaHuman format)
- **OBJ** (universal mesh format)
- **GLTF** (modern 3D format)

### Export
- **FBX** with Unreal Engine naming conventions
- Includes: geometry, UVs, normals, materials
- Y-up coordinate system
- Optional triangulation

### Project Files
- `.mmproj` format (JSON or binary)
- Stores: mesh references, transforms, point correspondences, viewport state
- Compression support using zlib

## Data Model

### Core Classes
```cpp
class Project {
    string projectName, projectPath;
    DateTime created, lastModified;
    MeshReference morphMesh, targetMesh;
    AlignmentData alignment;
    PointReferenceData pointReference;
    MorphData morph;
    ViewportState viewportState;
    UndoStack undoHistory;
};

class Mesh {
    vector<Vector3> vertices, normals;
    vector<Vector2> uvs;
    vector<Face> faces;
    vector<Material> materials;
    BoundingBox bounds;
};

class PointCorrespondence {
    int pointID;
    Vector3 morphMeshPosition, targetMeshPosition;
    int morphMeshVertexIndex, targetMeshVertexIndex;
    bool isSymmetric;
    int symmetricPairID;
};
```

## Development Phases (24 weeks / 12 sprints)

1. **Sprint 1-2:** Foundation & basic rendering
2. **Sprint 3:** Alignment stage
3. **Sprint 4-5:** Point Reference stage
4. **Sprint 6-7:** Morph stage with RBF deformation
5. **Sprint 8-9:** Touch Up stage with sculpting
6. **Sprint 10:** Export & project management
7. **Sprint 11:** Polish, undo/redo, optimization
8. **Sprint 12:** Testing & release preparation

## UI Specifications

### Color Scheme (Dark Theme)
- **Primary:** #2C3E50 (dark blue-gray)
- **Secondary:** #34495E (lighter blue-gray)
- **Accent:** #3498DB (blue for interactive elements)
- **Success:** #2ECC71 (green)
- **Warning:** #F39C12 (orange)
- **Error:** #E74C3C (red)

### Camera Controls (Blender-style)
- **Orbit:** Middle mouse button drag
- **Pan:** Shift + Middle mouse drag
- **Zoom:** Scroll wheel
- **Orthographic views:** Numpad 1 (front), 3 (right), 7 (top)
- **Toggle perspective/ortho:** Numpad 5
- **Reset camera:** Home key
- **Focus on selection:** F key

### Keyboard Shortcuts
- **New Project:** Ctrl+N
- **Open Project:** Ctrl+O
- **Save Project:** Ctrl+S
- **Export Mesh:** Ctrl+E
- **Undo:** Ctrl+Z
- **Redo:** Ctrl+Shift+Z
- **Move Tool:** G
- **Rotate Tool:** R
- **Scale Tool:** S
- **Axis Constraints:** X, Y, Z keys
- **Brush Radius:** [ and ] keys

## Success Criteria

### Primary
- All four workflow stages are functional
- Complete pipeline from alignment to export works
- Exported meshes load correctly in Unreal Engine 5.x
- Topology and UV integrity maintained
- Viewport maintains 30+ FPS for typical meshes (<100k vertices)
- Morphing completes in <30 seconds for standard configurations
- Experienced 3D artists can complete workflow in <30 minutes

### Quality Outputs
- Morphed meshes maintain topology integrity
- UV coordinates preserved correctly
- Deformation quality comparable to commercial tools
- No visual artifacts in standard use cases

## What's In Scope (MVP)

- Windows desktop application
- Import/export for FBX, OBJ, GLTF
- Four-stage workflow
- Interactive 3D viewport with Blender-style controls
- Point-based correspondence with symmetry support
- RBF/TPS mesh deformation
- Basic sculpting tools
- Project save/load
- Undo/redo system
- Export with geometry, UVs, normals, materials

## What's Out of Scope (MVP)

- Texture/material editing
- macOS and Linux support
- Direct Unreal Engine integration
- Automated point placement (AI/ML)
- Auto-save functionality
- Hair or body mesh morphing
- Real-time preview during morphing
- Multi-mesh batch processing

## Future Enhancements (Post-MVP)

### High Priority
1. Auto-save and crash recovery
2. Texture/material editing
3. Automated point placement with ML
4. Advanced deformation options
5. Batch processing

### Medium Priority
1. macOS and Linux support
2. Advanced sculpting tools (crease, pinch, masking)
3. Hair and body mesh support
4. Real-time deformation preview
5. Plugin architecture

### Low Priority
1. Direct Unreal Engine integration
2. Cloud rendering
3. VR sculpting support
4. Animation/blend shape support

## Development Guidelines

### When Assisting with This Project

1. **Prioritize Core Functionality:** Focus on the four-stage workflow and essential features before adding extras
2. **Maintain Architecture:** Follow the layered architecture pattern
3. **Performance Matters:** Target 30+ FPS for typical meshes, optimize early
4. **UE Compatibility:** Always test exports work with Unreal Engine
5. **User Experience:** Implement Blender-style controls as specified for familiarity
6. **Preserve Mesh Data:** Topology, UVs, and normals must remain intact through the entire pipeline
7. **Thread Heavy Operations:** Mesh loading, deformation, and export should run on worker threads
8. **Implement Undo/Redo:** Use Command pattern for all user actions

### Code Quality Standards
- Smart pointers (unique_ptr, shared_ptr) for memory management
- Clear separation of concerns across layers
- Comprehensive error handling with user-friendly messages
- Logging system for debugging
- No technical debt in core algorithms

### Testing Requirements
- Test with various mesh types and polygon counts (1k-100k+ vertices)
- Validate UV preservation throughout workflow
- Test all transform and sculpting tools
- Verify export to Unreal Engine 5.x
- Memory leak detection
- Cross-platform build testing (when applicable)

## Key Files

- **docs/PRD.md:** Complete Product Requirements Document with detailed specifications
- **docs/SPRINT_PLAN.md:** 12-sprint development roadmap with tasks and acceptance criteria
- **CLAUDE.md:** This file - project guide for AI assistants

## Target Audience

- **Indie Game Developers:** Solo developers and small teams creating custom characters
- **3D Artists:** Mid to expert-level artists working with Unreal Engine MetaHumans
- **Virtual Production Studios:** Small studios needing custom digital humans

### User Characteristics
- Intermediate to expert 3D artists
- Comfortable with 3D software (Blender, Maya, etc.)
- Familiar with MetaHuman workflow and Unreal Engine
- Creating 1-10 custom characters per project

## Glossary

- **MetaHuman:** Epic Games' system for creating realistic digital humans in Unreal Engine
- **Morph Mesh:** The base MetaHuman head mesh that will be deformed
- **Target Mesh:** The custom head model that the morph mesh will conform to
- **Point Correspondence:** Matching points on two meshes indicating the same anatomical location
- **TPS (Thin-Plate Spline):** An interpolation method for smooth deformations
- **RBF (Radial Basis Function):** A mathematical function used for scattered data interpolation
- **ARAP (As-Rigid-As-Possible):** A deformation technique that preserves local rigidity
- **UV Coordinates:** 2D texture coordinates mapped to 3D mesh surface
- **Topology:** The connectivity structure of a mesh (how vertices connect to form faces)

## Contributing

This is an open-source project under the MIT license. Contributions are welcome following standard practices:
- Fork the repository
- Create feature branches
- Write clean, documented code
- Test thoroughly
- Submit pull requests with clear descriptions

## Questions or Issues?

Refer to the detailed specifications in:
- **docs/PRD.md** for complete requirements
- **docs/SPRINT_PLAN.md** for implementation roadmap
