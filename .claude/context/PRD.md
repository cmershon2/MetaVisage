# Product Requirements Document (PRD)
## MetaVisage - MetaHuman Mesh Morphing Tool

**Version:** 1.0  
**Date:** December 26, 2025  
**Status:** MVP Specification  
**License:** MIT  
**Author:** Casey Mershon

---

## Executive Summary

The MetaHuman Mesh Morphing Tool is an open-source desktop application designed to automate the process of wrapping MetaHuman head meshes to custom head models for use in Unreal Engine. This tool addresses the significant cost barrier ($500/month) presented by existing commercial solutions like Faceform's Wrapped, making custom MetaHuman creation accessible to indie developers and small studios.

### Problem Statement
Creating custom MetaHuman heads requires wrapping a MetaHuman mesh to a custom head model—a complex process currently only available through expensive proprietary software. This creates a barrier for indie developers who need custom characters but cannot afford enterprise-level tools.

### Solution
A free, open-source desktop application that provides a complete workflow for:
- Aligning custom head meshes with MetaHuman base meshes
- Defining correspondence points between meshes
- Automatically morphing the MetaHuman mesh to match the custom head
- Refining the result with sculpting tools
- Exporting Unreal Engine-ready assets

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Goals & Objectives](#goals--objectives)
3. [Target Audience](#target-audience)
4. [Technical Requirements](#technical-requirements)
5. [Functional Requirements](#functional-requirements)
6. [User Interface Specifications](#user-interface-specifications)
7. [Data Model](#data-model)
8. [System Architecture](#system-architecture)
9. [Development Phases](#development-phases)
10. [Success Criteria](#success-criteria)
11. [Future Enhancements](#future-enhancements)
12. [Appendix](#appendix)

---

## Project Overview

### Scope
The MVP will focus on core mesh morphing functionality for Windows platforms, providing a complete pipeline from mesh import to Unreal Engine-ready export.

### In Scope (MVP)
- Windows desktop application
- Import/export support for FBX, OBJ, and GLTF formats
- Four-stage workflow (Alignment, Point Reference, Morph, Touch Up)
- Interactive 3D viewport with Blender-style controls
- Point-based correspondence system with symmetry support
- Mesh deformation using advanced interpolation algorithms
- Basic sculpting tools for refinement
- Project save/load functionality
- Undo/redo system
- Export with geometry, UVs, normals, and materials

### Out of Scope (MVP)
- Texture/material editing
- macOS and Linux support
- Direct Unreal Engine integration
- Automated point placement (AI/ML)
- Auto-save functionality
- Hair or body mesh morphing
- Real-time preview during morphing
- Multi-mesh batch processing

---

## Goals & Objectives

### Primary Goals
1. **Accessibility**: Provide a free alternative to $500/month commercial solutions
2. **Quality**: Deliver results comparable to professional tools
3. **Usability**: Create an intuitive workflow for experienced 3D artists
4. **Open Source**: Build a community-driven tool under MIT license

### Success Metrics
- Successfully morph MetaHuman meshes to custom heads
- Maintain mesh topology and UV integrity
- Export meshes that load correctly in Unreal Engine
- Complete morphing workflow in under 30 minutes for experienced users
- Process meshes with varying polygon counts without crashes

### Non-Goals
- Compete on advanced features with enterprise solutions
- Support complete beginners with no 3D experience
- Provide real-time rendering/game engine features

---

## Target Audience

### Primary Users
- **Indie Game Developers**: Solo developers and small teams creating custom characters
- **3D Artists**: Mid to expert-level artists working with Unreal Engine MetaHumans
- **Virtual Production Studios**: Small studios needing custom digital humans

### User Characteristics
- **Experience Level**: Intermediate to expert 3D artists
- **Technical Skills**: Comfortable with 3D software (Blender, Maya, etc.)
- **Domain Knowledge**: Familiar with MetaHuman workflow and Unreal Engine
- **Use Case**: Creating 1-10 custom characters per project

### User Needs
- Cost-effective solution for custom MetaHuman creation
- Control over the morphing process
- Ability to refine results manually
- Reliable export to Unreal Engine
- Save and iterate on projects

---

## Technical Requirements

### Platform Requirements

#### Operating System
- **Target Platform**: Windows 10/11 (64-bit)
- **Minimum Version**: Windows 10 version 1809 or later

#### Hardware Requirements
- **Processor**: Intel i5/AMD Ryzen 5 or better
- **RAM**: 8GB minimum, 16GB recommended
- **Graphics**: DirectX 11 compatible GPU with 2GB VRAM
- **Storage**: 500MB for application, additional space for projects
- **Display**: 1920x1080 minimum resolution

### Software Stack Recommendations

#### Development Framework
- **Language**: C++17 or later
- **UI Framework**: Qt 6.x (cross-platform ready, excellent 3D support)
- **Alternative**: Dear ImGui (lightweight, game-dev friendly)

#### 3D Rendering
- **Graphics API**: OpenGL 4.3+ (widest compatibility)
- **Alternative**: Vulkan (better performance, more complex)
- **Rendering Library**: Consider GLM for math, GLEW/GLAD for OpenGL loading

#### 3D Processing Libraries
- **Mesh I/O**: 
  - Assimp (Open Asset Import Library) for FBX, OBJ, GLTF import/export
  - FBX SDK (Autodesk) for enhanced FBX support
- **Mesh Processing**: 
  - libigl (geometry processing library)
  - OpenMesh or CGAL for mesh data structures
- **Deformation**: 
  - Custom implementation of TPS/RBF
  - Eigen library for linear algebra operations

#### Additional Libraries
- **Math**: GLM (OpenGL Mathematics) or Eigen
- **Serialization**: JSON (nlohmann/json) or Protocol Buffers for project files
- **Compression**: zlib for project file compression

### File Format Support

#### Import Formats
| Format | Priority | Use Case | Library |
|--------|----------|----------|---------|
| FBX | High | Primary MetaHuman format | FBX SDK / Assimp |
| OBJ | High | Universal mesh format | Assimp |
| GLTF | Medium | Modern 3D format | Assimp |

#### Export Formats
| Format | Priority | Requirements |
|--------|----------|--------------|
| FBX | High | Geometry, UVs, normals, materials, UE naming conventions |

#### Project Format
- Custom binary or JSON format (.mmproj)
- Store: mesh references, transforms, point correspondences, viewport state
- Compression supported for large projects

---

## Functional Requirements

### FR-1: Application Lifecycle

#### FR-1.1: Application Launch
- **Description**: User launches the application
- **Requirements**:
  - Application window opens centered on screen
  - Default resolution: 1920x1080
  - Show splash screen with project name and version
  - Load application preferences from config file
  - Initialize 3D rendering context

#### FR-1.2: New Project
- **Description**: User creates a new morphing project
- **Requirements**:
  - Prompt user to select/upload morph mesh (MetaHuman head)
  - Prompt user to upload target mesh (custom head)
  - Validate mesh formats and integrity
  - Load meshes into Alignment stage
  - Initialize default camera position

#### FR-1.3: Load Project
- **Description**: User loads an existing project
- **Requirements**:
  - File dialog to select .mmproj file
  - Load all mesh data, transformations, and point correspondences
  - Restore stage and viewport state
  - Handle missing mesh file references gracefully

#### FR-1.4: Save Project
- **Description**: User saves current project
- **Requirements**:
  - Save all project data to .mmproj file
  - Store relative paths to mesh files when possible
  - Compress project data to reduce file size
  - Provide "Save" and "Save As" options

### FR-2: Stage 1 - Alignment

#### FR-2.1: Viewport Layout
- **Description**: Display meshes in a 3D viewport with controls
- **Requirements**:
  - Main viewport occupies 75% of window width (left side)
  - Sidebar occupies 25% of window width (right side)
  - Viewport displays both morph mesh and target mesh
  - Visual distinction between meshes (different colors/wireframe)

#### FR-2.2: Mesh Display
- **Description**: Render meshes in the viewport
- **Requirements**:
  - Display morph mesh (locked, reference position)
  - Display target mesh (transformable)
  - Support multiple shading modes:
    - Solid shaded
    - Wireframe
    - Solid + Wireframe
    - Textured (if textures present)
  - Toggle visibility of each mesh independently

#### FR-2.3: Transform Tools
- **Description**: User manipulates target mesh position
- **Requirements**:
  - **Move Tool**: Translate mesh on X, Y, Z axes
  - **Rotate Tool**: Rotate mesh around X, Y, Z axes
  - **Scale Tool**: Uniform and non-uniform scaling
  - Visual gizmos for each tool (similar to Blender)
  - Numeric input for precise transformations
  - Tool hotkeys: G (move), R (rotate), S (scale)
  - Axis constraints: X, Y, Z keys to lock to axis

#### FR-2.4: Camera Controls
- **Description**: User navigates the 3D viewport
- **Requirements**:
  - **Blender-style controls**:
    - Middle mouse button drag: Orbit camera
    - Shift + Middle mouse drag: Pan camera
    - Scroll wheel: Zoom in/out
  - **Orthographic views**:
    - Numpad 1: Front view
    - Numpad 3: Right view
    - Numpad 7: Top view
    - Numpad 0: Camera view
    - Numpad 5: Toggle perspective/orthographic
  - Reset camera to default view (Home key)
  - Focus on selection (F key)

#### FR-2.5: Alignment Sidebar
- **Description**: Controls for alignment stage
- **Requirements**:
  - Transform mode selector (Move/Rotate/Scale)
  - Numeric transform inputs (X, Y, Z values)
  - Reset transform button
  - Viewport shading mode selector
  - Mesh visibility toggles
  - Grid display toggle
  - "Next Stage" button to proceed to Point Reference

### FR-3: Stage 2 - Point Reference

#### FR-3.1: Dual Viewport Layout
- **Description**: Display two synchronized viewports
- **Requirements**:
  - Split main area into two equal viewports (side by side)
  - Left viewport: Target mesh
  - Right viewport: Morph mesh
  - Sidebar on right (shared between viewports)
  - Viewports take 75% of window width, sidebar 25%

#### FR-3.2: Camera Synchronization
- **Description**: Link camera movement between viewports
- **Requirements**:
  - Camera rotation synchronized across both viewports
  - Camera zoom synchronized across both viewports
  - Camera pan synchronized across both viewports
  - Both viewports show same view direction and distance
  - User can interact with either viewport to control camera

#### FR-3.3: Point Placement
- **Description**: User adds correspondence points to meshes
- **Requirements**:
  - Click on mesh surface to place point
  - Points automatically numbered sequentially (1, 2, 3...)
  - Points displayed as visible markers (spheres/pins)
  - Point labels show number
  - Point color coding:
    - Newly placed: Yellow
    - Selected: Orange/Highlighted
    - Unselected: Green
  - Points "stick" to mesh surface during viewport rotation

#### FR-3.4: Point Management
- **Description**: User edits and removes points
- **Requirements**:
  - Click on point to select it
  - Delete key to remove selected point
  - Renumber remaining points automatically
  - Highlight corresponding point in other viewport
  - Show list of all points in sidebar
  - Click point in list to select/focus in viewport

#### FR-3.5: Symmetry Mode
- **Description**: Mirror point placement across mesh symmetry plane
- **Requirements**:
  - Toggle symmetry mode on/off
  - Default symmetry plane: YZ (X-axis symmetry)
  - When placing point on one side, automatically place mirrored point
  - Mirrored points numbered consecutively
  - Visual indicator showing symmetry plane
  - Manual symmetry plane adjustment (X, Y, Z axis selection)

#### FR-3.6: Point Reference Sidebar
- **Description**: Controls for point reference stage
- **Requirements**:
  - Active viewport indicator
  - Point count display (target vs morph mesh)
  - Point list with scroll
  - Delete point button
  - Clear all points button (with confirmation)
  - Symmetry toggle and axis selector
  - Viewport shading mode selectors (per viewport)
  - Point size slider
  - Validation: equal point count before proceeding
  - "Next Stage" button (enabled only when point counts match)

### FR-4: Stage 3 - Morph

#### FR-4.1: Deformation Algorithm
- **Description**: Transform morph mesh to match target mesh
- **Requirements**:
  - **Primary Algorithm**: Radial Basis Function (RBF) interpolation
    - Supports thin-plate spline (TPS) kernel
    - Alternative kernels: Gaussian, multiquadric
  - **Alternative Algorithm**: As-Rigid-As-Possible (ARAP) deformation
  - Use point correspondences to compute transformation
  - Preserve mesh topology (connectivity unchanged)
  - Maintain UV coordinates
  - Preserve vertex normals (recalculate after deformation)

#### FR-4.2: Processing Interface
- **Description**: User initiates and monitors morphing
- **Requirements**:
  - Single viewport showing morph mesh
  - "Process" button to start deformation
  - Progress bar during computation
  - Estimated time remaining
  - Cancel operation option
  - Error handling for invalid configurations

#### FR-4.3: Deformation Parameters
- **Description**: User adjusts morphing behavior
- **Requirements**:
  - **Stiffness**: Control mesh rigidity (0.0 - 1.0)
  - **Smoothness**: Blend between point influence regions (0.0 - 1.0)
  - **Kernel Type**: Select RBF kernel function
  - Parameter tooltips explaining effect
  - Default values that work for most cases
  - "Reset to Defaults" button

#### FR-4.4: Preview Modes
- **Description**: Visualize deformation result
- **Requirements**:
  - View morphed mesh alone
  - Overlay target mesh as reference (wireframe)
  - Toggle between original and morphed mesh
  - Heat map showing deformation magnitude
  - Wireframe overlay to check topology

#### FR-4.5: Morph Sidebar
- **Description**: Controls for morph stage
- **Requirements**:
  - Deformation parameter controls
  - Process button
  - Progress indicator
  - Preview mode toggles
  - Accept/Re-process options
  - "Next Stage" button to proceed to Touch Up

### FR-5: Stage 4 - Touch Up

#### FR-5.1: Viewport Layout
- **Description**: Single viewport with sculpting tools
- **Requirements**:
  - Main viewport occupies 75% of window width
  - Sidebar occupies 25% of window width
  - Display morphed mesh for editing
  - Optional reference mesh overlay

#### FR-5.2: Sculpting Tools
- **Description**: Refine mesh with brush-based tools
- **Requirements**:
  - **Smooth Brush**: Average vertex positions to smooth surface
  - **Grab Brush**: Move vertices with falloff
  - **Push/Pull Brush**: Move vertices along surface normal
  - **Inflate/Deflate Brush**: Expand/contract surface
  - Circular brush cursor showing radius and falloff
  - Brush affects vertices within radius
  - Falloff curve: linear, smooth, or sharp

#### FR-5.3: Brush Parameters
- **Description**: Control brush behavior
- **Requirements**:
  - **Radius**: Brush size (adjustable via slider or [ ] keys)
  - **Strength**: Effect intensity (0.0 - 1.0)
  - **Falloff**: How influence decreases from center
  - Visual preview of brush parameters in viewport
  - Pressure sensitivity support (if graphics tablet detected)

#### FR-5.4: Symmetry Mode (Sculpting)
- **Description**: Mirror sculpting operations
- **Requirements**:
  - Toggle symmetry on/off
  - Symmetry plane selection (X, Y, Z)
  - Visual indicator of symmetry plane
  - Strokes on one side automatically mirrored

#### FR-5.5: Mesh Display Options
- **Description**: Visualization aids for sculpting
- **Requirements**:
  - Multiple shading modes (flat, smooth, wireframe)
  - Lighting adjustment (key light direction)
  - MatCap materials for surface detail visibility
  - Wireframe overlay toggle
  - Reference mesh overlay (semi-transparent target mesh)

#### FR-5.6: Touch Up Sidebar
- **Description**: Controls for touch-up stage
- **Requirements**:
  - Brush tool selector (icon buttons)
  - Radius slider
  - Strength slider
  - Falloff curve selector
  - Symmetry toggle and axis
  - Shading mode selector
  - Lighting controls
  - "Finalize and Export" button

### FR-6: Export

#### FR-6.1: Export Dialog
- **Description**: User exports final mesh
- **Requirements**:
  - File dialog to choose export location
  - Filename input (default: "morphed_metahuman_head.fbx")
  - Format selector (FBX primary, OBJ/GLTF optional)
  - Export options panel

#### FR-6.2: Export Requirements
- **Description**: Data included in export
- **Requirements**:
  - Geometry (vertex positions, faces)
  - UV coordinates (original UVs from morph mesh)
  - Vertex normals (recalculated)
  - Materials (if present in original morph mesh)
  - Mesh name and hierarchy preserved
  - Unreal Engine naming conventions:
    - Head mesh: "SK_Head"
    - Material slots preserved

#### FR-6.3: Export Options
- **Description**: User customizes export
- **Requirements**:
  - FBX version selector (FBX 2020, 2019, etc.)
  - Coordinate system (Y-up for UE)
  - Scale factor (1.0 default for UE)
  - Triangulate option (convert quads to tris)
  - Include materials toggle
  - Embed textures toggle (if textures present)

#### FR-6.4: Export Validation
- **Description**: Verify export success
- **Requirements**:
  - Progress bar during export
  - Success/failure notification
  - Export log showing any warnings
  - Option to open export folder
  - Option to export project file alongside mesh

### FR-7: Undo/Redo System

#### FR-7.1: Undo/Redo Functionality
- **Description**: Revert and restore user actions
- **Requirements**:
  - Undo: Ctrl+Z (or Command+Z)
  - Redo: Ctrl+Shift+Z (or Command+Shift+Z)
  - Unlimited undo history (memory permitting)
  - Actions that support undo/redo:
    - Transform operations (Alignment stage)
    - Point placement/deletion (Point Reference stage)
    - Mesh morphing (Morph stage)
    - Sculpting strokes (Touch Up stage)
  - Visual feedback when undo/redo executed
  - Disable undo/redo when history is empty

#### FR-7.2: History Management
- **Description**: Track and manage action history
- **Requirements**:
  - Maintain separate undo stacks per stage
  - Clear history when moving to new stage (with warning)
  - Option to view action history
  - Memory management to prevent overflow

### FR-8: Cross-Cutting Features

#### FR-8.1: Stage Navigation
- **Description**: Move between workflow stages
- **Requirements**:
  - Stage indicator showing current step (1/4, 2/4, etc.)
  - "Next Stage" button (bottom-right)
  - "Previous Stage" button (back navigation)
  - Confirmation dialog if unsaved changes
  - Disable "Next" if stage requirements not met

#### FR-8.2: Validation
- **Description**: Ensure data integrity before proceeding
- **Requirements**:
  - **Alignment Stage**: Both meshes loaded
  - **Point Reference Stage**: Equal point counts on both meshes
  - **Morph Stage**: Successful deformation completion
  - **Touch Up Stage**: No invalid geometry
  - Display error messages for validation failures
  - Highlight issues in UI

#### FR-8.3: Error Handling
- **Description**: Graceful handling of errors
- **Requirements**:
  - User-friendly error messages (no stack traces)
  - Logging system for debugging
  - Crash recovery (auto-save state before risky operations)
  - "Report Issue" button (opens GitHub issues)

#### FR-8.4: Performance
- **Description**: Responsive user experience
- **Requirements**:
  - Viewport frame rate: 30 FPS minimum for meshes <100k vertices
  - Morph computation: <30 seconds for 150 points, 50k vertex mesh
  - Sculpting responsiveness: <16ms per stroke
  - Lazy loading for large meshes
  - Background threading for heavy computations

#### FR-8.5: Tooltips and Help
- **Description**: In-app user guidance
- **Requirements**:
  - Tooltips on all buttons and controls
  - Tooltips explain function and hotkey (if applicable)
  - Status bar showing current tool and hints
  - Contextual help based on current stage
  - "Help" menu linking to external documentation

---

## User Interface Specifications

### UI-1: Visual Design

#### Color Scheme
- **Background**: Dark theme (to reduce eye strain during extended use)
- **Primary**: #2C3E50 (dark blue-gray)
- **Secondary**: #34495E (lighter blue-gray)
- **Accent**: #3498DB (blue for interactive elements)
- **Success**: #2ECC71 (green)
- **Warning**: #F39C12 (orange)
- **Error**: #E74C3C (red)

#### Typography
- **Font Family**: Segoe UI (Windows standard) or Roboto
- **Headers**: 14pt, bold
- **Body Text**: 11pt, regular
- **Labels**: 10pt, regular
- **Code/Numbers**: Consolas or monospace, 10pt

#### Spacing and Layout
- **Padding**: 8px standard, 16px for major sections
- **Margins**: 4px between related elements, 12px between sections
- **Button Height**: 32px
- **Input Height**: 28px
- **Sidebar Width**: 320px (25% of 1280px minimum width)

### UI-2: Stage-Specific Layouts

#### Stage 1: Alignment
```
┌─────────────────────────────────┬───────────┐
│                                 │ Alignment │
│                                 │           │
│                                 │ Transform │
│        3D Viewport              │  • Move   │
│      (Both Meshes)              │  • Rotate │
│                                 │  • Scale  │
│                                 │           │
│                                 │ Shading   │
│                                 │           │
│                                 │ Meshes    │
│                                 │           │
│                                 │ [Next]    │
└─────────────────────────────────┴───────────┘
```

#### Stage 2: Point Reference
```
┌────────────────┬────────────────┬───────────┐
│                │                │ Points    │
│  Target Mesh   │  Morph Mesh    │           │
│    Viewport    │    Viewport    │ Count: 0  │
│                │                │           │
│                │                │ List:     │
│                │                │ [      ]  │
│                │                │           │
│                │                │ Symmetry  │
│                │                │           │
│                │                │ [Next]    │
└────────────────┴────────────────┴───────────┘
```

#### Stage 3: Morph
```
┌─────────────────────────────────┬───────────┐
│                                 │ Morph     │
│                                 │           │
│                                 │ Params    │
│        3D Viewport              │ Stiffness │
│      (Morphed Mesh)             │ Smoothness│
│                                 │           │
│                                 │ [Process] │
│                                 │           │
│                                 │ Preview   │
│                                 │           │
│                                 │ [Next]    │
└─────────────────────────────────┴───────────┘
```

#### Stage 4: Touch Up
```
┌─────────────────────────────────┬───────────┐
│                                 │ Sculpt    │
│                                 │           │
│                                 │ Tools     │
│        3D Viewport              │ ⚫Smooth  │
│      (Morphed Mesh)             │ ⚫Grab    │
│                                 │ ⚫Push/Pul│
│                                 │           │
│                                 │ Brush     │
│                                 │ Radius    │
│                                 │ Strength  │
│                                 │           │
│                                 │ [Export]  │
└─────────────────────────────────┴───────────┘
```

### UI-3: Common Elements

#### Menu Bar
```
File | Edit | View | Tools | Help
```

**File Menu**:
- New Project (Ctrl+N)
- Open Project (Ctrl+O)
- Save Project (Ctrl+S)
- Save Project As (Ctrl+Shift+S)
- Export Mesh (Ctrl+E)
- Exit (Alt+F4)

**Edit Menu**:
- Undo (Ctrl+Z)
- Redo (Ctrl+Shift+Z)
- Preferences

**View Menu**:
- Shading Mode submenu
- Show/Hide Grid
- Show/Hide Points
- Reset Camera (Home)
- Front View (Numpad 1)
- Right View (Numpad 3)
- Top View (Numpad 7)

**Tools Menu**:
- Move (G)
- Rotate (R)
- Scale (S)
- Place Point (P)
- Sculpt Tools submenu

**Help Menu**:
- Documentation (F1)
- Keyboard Shortcuts
- Report Issue
- About

#### Status Bar
- Left: Current tool/mode
- Center: Tips based on context
- Right: Vertex count, face count, FPS

#### Button Styles
- **Primary Button**: Blue background, white text, 32px height
- **Secondary Button**: Gray background, white text, 32px height
- **Danger Button**: Red background, white text (for destructive actions)
- **Icon Button**: Square, 32x32px, icon centered

### UI-4: Responsive Behavior

#### Minimum Window Size
- Width: 1280px
- Height: 720px
- Allow resizing above minimum
- Maintain aspect ratios of viewports

#### Viewport Resizing
- Viewports scale proportionally
- Sidebar maintains fixed width or minimum width
- Splitter between dual viewports adjustable

---

## Data Model

### DM-1: Project Structure

```cpp
class Project {
    string projectName;
    string projectPath;
    DateTime created;
    DateTime lastModified;
    
    MeshReference morphMesh;
    MeshReference targetMesh;
    
    AlignmentData alignment;
    PointReferenceData pointReference;
    MorphData morph;
    
    ViewportState viewportState;
    UndoStack undoHistory;
};
```

### DM-2: Mesh Data

```cpp
class Mesh {
    string name;
    string filepath;
    
    vector<Vector3> vertices;
    vector<Vector3> normals;
    vector<Vector2> uvs;
    vector<Face> faces;
    vector<Material> materials;
    
    BoundingBox bounds;
    
    // Methods
    void Load(string path);
    void Save(string path);
    void CalculateNormals();
    void Validate();
};

class MeshReference {
    string filepath;
    Transform transform;
    bool isLoaded;
};
```

### DM-3: Transform Data

```cpp
class Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    
    Matrix4x4 GetMatrix();
    void Reset();
};
```

### DM-4: Point Correspondence

```cpp
class PointCorrespondence {
    int pointID;
    Vector3 morphMeshPosition;
    int morphMeshVertexIndex;
    Vector3 targetMeshPosition;
    int targetMeshVertexIndex;
    bool isSymmetric;
    int symmetricPairID;
};

class PointReferenceData {
    vector<PointCorrespondence> correspondences;
    bool symmetryEnabled;
    Axis symmetryAxis;
    float symmetryPlaneOffset;
};
```

### DM-5: Morph Settings

```cpp
class MorphData {
    enum DeformationAlgorithm {
        RBF_TPS,
        RBF_GAUSSIAN,
        RBF_MULTIQUADRIC,
        ARAP
    };
    
    DeformationAlgorithm algorithm;
    float stiffness;
    float smoothness;
    
    Mesh originalMorphMesh;
    Mesh deformedMorphMesh;
    bool isProcessed;
};
```

### DM-6: Viewport State

```cpp
class ViewportState {
    Camera camera;
    ShadingMode shadingMode;
    bool showGrid;
    bool showWireframe;
    bool showPoints;
    float pointSize;
};

class Camera {
    Vector3 position;
    Vector3 target;
    Vector3 up;
    float fov;
    ProjectionMode projectionMode;
    
    enum ProjectionMode {
        PERSPECTIVE,
        ORTHOGRAPHIC_FRONT,
        ORTHOGRAPHIC_RIGHT,
        ORTHOGRAPHIC_TOP
    };
};
```

### DM-7: Undo/Redo System

```cpp
class UndoableAction {
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    string description;
};

class UndoStack {
    vector<UndoableAction*> undoStack;
    vector<UndoableAction*> redoStack;
    
    void Push(UndoableAction* action);
    void Undo();
    void Redo();
    void Clear();
};
```

---

## System Architecture

### SA-1: Architecture Overview

The application follows a **layered architecture** with clear separation of concerns:

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

### SA-2: Component Diagram

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   UI Layer   │────▶│  Workflow    │────▶│  Mesh Engine │
│  (Views)     │     │  Controller  │     │  (Processing)│
└──────────────┘     └──────────────┘     └──────────────┘
                             │                     │
                             ▼                     ▼
                     ┌──────────────┐     ┌──────────────┐
                     │   Command    │     │  Deformation │
                     │   Manager    │     │  Algorithm   │
                     └──────────────┘     └──────────────┘
                             │                     │
                             ▼                     ▼
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   Renderer   │     │   Project    │     │  File I/O    │
│  (OpenGL)    │     │   Manager    │     │  (Assimp)    │
└──────────────┘     └──────────────┘     └──────────────┘
```

### SA-3: Key Components

#### 1. UI Layer (Presentation)
- **Responsibility**: Display information, capture user input
- **Components**:
  - MainWindow: Application window container
  - ViewportWidget: 3D rendering surface
  - SidebarWidget: Tool panels and controls
  - MenuBar, ToolBar, StatusBar
- **Technology**: Qt Widgets or Dear ImGui

#### 2. Workflow Controller (Application)
- **Responsibility**: Orchestrate workflow stages
- **Components**:
  - StageManager: Controls stage transitions
  - AlignmentController
  - PointReferenceController
  - MorphController
  - TouchUpController
- **Patterns**: State pattern for stage management

#### 3. Command Manager (Application)
- **Responsibility**: Implement undo/redo
- **Components**:
  - CommandInvoker: Execute and store commands
  - Concrete commands for each action type
- **Patterns**: Command pattern

#### 4. Mesh Engine (Domain)
- **Responsibility**: Core mesh operations
- **Components**:
  - MeshLoader: Load mesh files
  - MeshProcessor: Validate, optimize meshes
  - MeshExporter: Save mesh files
  - TopologyAnalyzer: Check mesh validity
- **Libraries**: Assimp, libigl, OpenMesh

#### 5. Deformation Algorithm (Domain)
- **Responsibility**: Morph mesh based on correspondences
- **Components**:
  - RBFInterpolator: Radial basis function
  - TPSDeformer: Thin-plate spline
  - ARAPDeformer: As-rigid-as-possible
  - DeformationSolver: Solve linear systems
- **Libraries**: Eigen for linear algebra

#### 6. Sculpting Engine (Domain)
- **Responsibility**: Interactive mesh editing
- **Components**:
  - SculptBrush: Base brush class
  - SmoothBrush, GrabBrush, PushPullBrush, InflateBrush
  - BrushStroke: Capture stroke data
  - SymmetryEngine: Mirror operations

#### 7. Renderer (Infrastructure)
- **Responsibility**: 3D visualization
- **Components**:
  - OpenGLContext: Manage GL state
  - MeshRenderer: Draw meshes
  - PointRenderer: Draw correspondence points
  - GridRenderer: Draw reference grid
  - ShaderManager: Compile and manage shaders
- **Technology**: OpenGL 4.3+, GLSL

#### 8. Project Manager (Infrastructure)
- **Responsibility**: Serialize/deserialize projects
- **Components**:
  - ProjectSerializer: Save projects to disk
  - ProjectDeserializer: Load projects from disk
  - FileWatcher: Monitor external file changes
- **Format**: JSON or custom binary

### SA-4: Threading Model

- **Main Thread**: UI rendering and event handling
- **Worker Threads**: 
  - Mesh loading/saving
  - Deformation computation
  - Heavy mesh processing
- **Thread Safety**: 
  - Use mutexes for shared data
  - Progress callbacks to main thread
  - Cancel tokens for long operations

### SA-5: Memory Management

- **Strategies**:
  - Smart pointers (unique_ptr, shared_ptr) for ownership
  - Object pooling for frequently created objects
  - Lazy loading for large meshes
  - Streaming for very large files
  - Release unused resources after stage completion

---

## Development Phases

### Phase 1: Foundation (4-6 weeks)

**Objective**: Establish core infrastructure

**Tasks**:
1. Set up development environment
   - Install Qt or ImGui framework
   - Configure OpenGL context
   - Set up build system (CMake)
2. Implement basic UI skeleton
   - Main window with menu bar
   - Basic viewport rendering
   - Sidebar layout
3. Integrate Assimp library
   - Mesh loading (FBX, OBJ, GLTF)
   - Basic mesh display in viewport
4. Implement camera controls
   - Blender-style orbit/pan/zoom
   - Orthographic view switching
5. Basic project structure
   - Create data model classes
   - Implement project save/load (basic)

**Deliverable**: Application that can load and display a mesh with camera navigation

### Phase 2: Alignment Stage (3-4 weeks)

**Objective**: Complete Stage 1 functionality

**Tasks**:
1. Implement transform gizmos
   - Move, rotate, scale tools
   - Visual gizmo rendering
2. Transform controls in sidebar
   - Numeric inputs
   - Tool selector
3. Mesh display options
   - Shading modes (solid, wireframe, etc.)
   - Mesh visibility toggles
4. Two-mesh display
   - Load morph and target meshes
   - Visual differentiation
5. Stage validation
   - Check both meshes loaded
   - Enable/disable "Next Stage" button

**Deliverable**: Functional alignment stage where users can position meshes

### Phase 3: Point Reference Stage (4-5 weeks)

**Objective**: Complete Stage 2 functionality

**Tasks**:
1. Dual viewport implementation
   - Split viewport rendering
   - Synchronized camera controls
2. Point placement system
   - Ray casting for surface clicking
   - Point marker rendering
   - Automatic numbering
3. Point management
   - Point selection
   - Point deletion
   - List view in sidebar
4. Symmetry mode
   - Mirror point placement
   - Symmetry plane visualization
5. Point correspondence storage
   - Link points between meshes
   - Validate equal counts

**Deliverable**: Working point reference system with symmetry support

### Phase 4: Morph Stage (5-6 weeks)

**Objective**: Implement deformation algorithm

**Tasks**:
1. Integrate Eigen library
   - Set up linear algebra operations
2. Implement RBF interpolation
   - Thin-plate spline kernel
   - Gaussian kernel
   - Build and solve linear system
3. Mesh deformation
   - Apply deformation to vertices
   - Preserve topology and UVs
   - Recalculate normals
4. Parameter controls
   - Stiffness and smoothness sliders
   - Kernel type selector
5. Progress indication
   - Threading for computation
   - Progress bar and cancel option
6. Result preview
   - Multiple visualization modes
   - Accept/re-process workflow

**Deliverable**: Functional mesh morphing with adjustable parameters

### Phase 5: Touch Up Stage (4-5 weeks)

**Objective**: Implement sculpting tools

**Tasks**:
1. Sculpting infrastructure
   - Brush base class
   - Brush stroke capture
   - Vertex influence calculation
2. Implement brushes
   - Smooth brush
   - Grab brush
   - Push/Pull brush
   - Inflate/Deflate brush
3. Brush controls
   - Radius, strength, falloff
   - Visual brush cursor
   - Keyboard shortcuts
4. Symmetry sculpting
   - Mirror stroke operations
5. Display options
   - MatCap shading
   - Lighting controls

**Deliverable**: Working sculpting tools for mesh refinement

### Phase 6: Export & Polish (3-4 weeks)

**Objective**: Finalize export and polish UX

**Tasks**:
1. Export functionality
   - FBX export with full data
   - Unreal Engine naming conventions
   - Export options dialog
2. Undo/Redo system
   - Implement command pattern
   - Hook up to all actions
3. Project management
   - Complete save/load implementation
   - Project file format finalization
4. Error handling
   - User-friendly error messages
   - Crash recovery
5. Performance optimization
   - Profile rendering performance
   - Optimize heavy computations
6. Tooltips and help
   - Add tooltips to all controls
   - Status bar hints

**Deliverable**: Polished, complete MVP application

### Phase 7: Testing & Documentation (2-3 weeks)

**Objective**: Ensure quality and usability

**Tasks**:
1. Manual testing
   - Test full workflow multiple times
   - Test with various mesh types
   - Validate UE integration
2. Bug fixing
   - Address discovered issues
   - Regression testing
3. Performance testing
   - Test with large meshes
   - Memory leak detection
4. Documentation
   - README with installation
   - User guide with screenshots
   - Contribution guidelines
5. Prepare for release
   - Create GitHub repository
   - Set up CI/CD if applicable
   - Create installer/package

**Deliverable**: Release-ready application with documentation

---

## Success Criteria

### Primary Success Criteria

1. **Functional Completeness**
   - ✅ All four workflow stages are functional
   - ✅ Users can complete the full pipeline from alignment to export
   - ✅ Exported meshes load correctly in Unreal Engine

2. **Quality Outputs**
   - ✅ Morphed meshes maintain topology integrity
   - ✅ UV coordinates are preserved correctly
   - ✅ Deformation quality is comparable to commercial tools
   - ✅ No visual artifacts in standard use cases

3. **Usability**
   - ✅ Experienced 3D artists can complete workflow in <30 minutes
   - ✅ UI is intuitive with minimal learning curve
   - ✅ Tooltips and status hints provide adequate guidance

4. **Performance**
   - ✅ Viewport maintains 30+ FPS for typical meshes
   - ✅ Morphing completes in <30 seconds for standard configurations
   - ✅ No application crashes during normal use

5. **Compatibility**
   - ✅ Supports FBX, OBJ, and GLTF import
   - ✅ Exports FBX compatible with Unreal Engine 5.x
   - ✅ Runs on Windows 10 and Windows 11

### Secondary Success Criteria

1. **Open Source Adoption**
   - GitHub stars/forks indicating community interest
   - Bug reports and feature requests from users
   - Contributions from other developers

2. **Documentation Quality**
   - Clear installation instructions
   - Comprehensive user guide
   - Example workflows with sample assets

3. **Code Quality**
   - Clean, maintainable code architecture
   - Reasonable test coverage for critical components
   - No major technical debt

---

## Future Enhancements

### Post-MVP Features (Prioritized)

#### High Priority
1. **Auto-save functionality**
   - Periodic automatic project saves
   - Crash recovery system
   
2. **Texture/Material editing**
   - Import and edit texture maps
   - Material property adjustment
   - Texture projection tools

3. **Automated point placement**
   - ML-based facial landmark detection
   - Semi-automatic correspondence generation
   - Point suggestion system

4. **Advanced deformation options**
   - Multiple algorithm choices
   - Deformation preview
   - Region-based deformation weights

5. **Batch processing**
   - Process multiple meshes with same settings
   - Batch export functionality

#### Medium Priority
1. **macOS and Linux support**
   - Cross-platform builds
   - Platform-specific installers

2. **Advanced sculpting tools**
   - Crease/pinch brushes
   - Layer-based sculpting
   - Masking system

3. **Hair and body support**
   - Extend beyond head meshes
   - Support for hair cards
   - Full body MetaHuman morphing

4. **Real-time deformation preview**
   - Interactive parameter adjustment
   - Live morphing visualization

5. **Plugin architecture**
   - Custom deformation algorithms
   - User-created tools
   - Export format plugins

#### Low Priority
1. **Direct Unreal Engine integration**
   - UE plugin for direct import
   - Live link support
   - Asset browser integration

2. **Cloud rendering**
   - Offload heavy computations
   - Collaboration features

3. **VR sculpting support**
   - VR headset integration
   - 3D spatial sculpting

4. **Animation support**
   - Blend shape creation
   - Facial animation morphs

---

## Appendix

### A. Glossary

- **MetaHuman**: Epic Games' system for creating realistic digital humans in Unreal Engine
- **Morph Mesh**: The base MetaHuman head mesh that will be deformed
- **Target Mesh**: The custom head model that the morph mesh will conform to
- **Point Correspondence**: Matching points on two meshes indicating the same anatomical location
- **TPS (Thin-Plate Spline)**: An interpolation method for smooth deformations
- **RBF (Radial Basis Function)**: A mathematical function used for scattered data interpolation
- **ARAP (As-Rigid-As-Possible)**: A deformation technique that preserves local rigidity
- **UV Coordinates**: 2D texture coordinates mapped to 3D mesh surface
- **Topology**: The connectivity structure of a mesh (how vertices connect to form faces)

### B. References

**Mesh Processing**:
- Botsch et al., "Polygon Mesh Processing" (book)
- libigl library documentation: https://libigl.github.io/
- OpenMesh documentation: https://www.graphics.rwth-aachen.de/software/openmesh/

**Deformation Algorithms**:
- Bookstein, "Principal Warps: Thin-Plate Splines and the Decomposition of Deformations" (1989)
- Sorkine and Alexa, "As-Rigid-As-Possible Surface Modeling" (2007)
- Botsch and Sorkine, "On Linear Variational Surface Deformation Methods" (2008)

**Libraries**:
- Qt Framework: https://www.qt.io/
- Assimp: https://www.assimp.org/
- Eigen: https://eigen.tuxfamily.org/
- OpenGL: https://www.opengl.org/

**Unreal Engine**:
- MetaHuman Creator: https://www.unrealengine.com/en-US/metahuman
- UE Documentation: https://docs.unrealengine.com/

### C. Competitive Analysis

**Faceform Wrapped**:
- Pricing: ~$500/month subscription
- Strengths: Polished UI, automatic features, proven results
- Weaknesses: Very expensive, closed-source, subscription model
- Target: Professional studios

**Wrap by R3DS**:
- Pricing: High-end enterprise pricing
- Strengths: Industry-standard tool, advanced features
- Weaknesses: Extremely expensive, complex for simple tasks
- Target: VFX studios, AAA game studios

**Open-Source Alternatives**:
- Currently none that specifically target MetaHuman workflow
- Blender has some mesh morphing tools but requires manual work
- Opportunity for our tool to fill this gap

### D. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|------------|---------|------------|
| Deformation algorithm doesn't produce good results | Medium | High | Research and test multiple algorithms; iterate on parameters |
| Performance issues with large meshes | Medium | Medium | Optimize early; use spatial data structures; implement LOD |
| Complex UI discourages users | Low | Medium | User testing; iterate on UX; provide examples |
| Compatibility issues with UE | Low | High | Regular testing with UE; follow naming conventions strictly |
| Limited adoption due to niche use case | Medium | Low | Focus on quality; engage with UE community |
| Technical complexity exceeds solo dev capacity | Medium | High | Start with MVP; prioritize core features; seek community help |

### E. Development Resources

**Estimated Effort**: 
- Total Development Time: 25-35 weeks for MVP
- Post-MVP Polish: 8-12 weeks
- Ongoing Maintenance: Variable

**Required Skills**:
- C++ programming (intermediate to advanced)
- 3D graphics programming (OpenGL/Vulkan)
- UI/UX design (Qt or ImGui)
- Linear algebra and geometry processing
- Mesh processing and deformation theory

**Recommended Learning Path** (if needed):
1. OpenGL basics and rendering pipeline
2. Qt framework fundamentals
3. Linear algebra for graphics
4. Mesh data structures and operations
5. Deformation and interpolation algorithms

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-12-26 | Casey Mershon | Initial MVP specification |


---

*This PRD is a living document and will be updated as the project evolves and new requirements emerge.*
