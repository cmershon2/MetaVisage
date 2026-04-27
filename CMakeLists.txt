cmake_minimum_required(VERSION 3.16)
project(MetaVisage VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sprint 3.7 — Qt6 fully removed. The application is now self-hosted on
# GLFW + Dear ImGui + standard C++ libraries (std::format, std::filesystem,
# std::chrono, nlohmann::json). No Qt link or build dependency remains.

# Configure-time Qt-purity guard. If a future change accidentally re-introduces
# any Qt6::Gui / Qt6::Widgets / Qt6::OpenGL / Qt6::OpenGLWidgets target —
# either via a transitive vcpkg dep or a stray find_package call — fail the
# configure with a clear message instead of letting the build pull Qt back in.
# Qt6::Core is included even though it's already gone; the guard exists to keep
# it gone.
foreach(_forbidden_qt_target IN ITEMS
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
    if(TARGET ${_forbidden_qt_target})
        message(FATAL_ERROR
            "Sprint 3.7 removed Qt entirely from MetaVisage, but the target "
            "${_forbidden_qt_target} is defined in this configure. Track down the "
            "find_package(Qt6 …) or transitive dependency that re-introduced it "
            "before continuing the build.")
    endif()
endforeach()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# OpenGL (system)
find_package(OpenGL REQUIRED)

# GLEW — OpenGL function loader (replaces QOpenGLFunctions_4_3_Core)
find_package(GLEW REQUIRED)

# GLFW — window + context + input (replaces QWindow/QOpenGLWidget/Qt events)
find_package(glfw3 CONFIG REQUIRED)

# Dear ImGui — UI rendering (replaces Qt Widgets; Sprint 3.5.2+)
find_package(imgui CONFIG REQUIRED)

# nlohmann_json — project serialization + license JWT payloads (Sprint 3.6+)
find_package(nlohmann_json CONFIG REQUIRED)

# Native File Dialog Extended — replaces QFileDialog (Sprint 3.6)
find_package(nfd CONFIG REQUIRED)

# libcurl — HTTP client for licensing API (Sprint 3.6)
find_package(CURL REQUIRED)

# Eigen3 — deformation math (unchanged)
find_package(Eigen3 CONFIG REQUIRED)

# Assimp — mesh I/O (optional, unchanged)
find_package(assimp CONFIG)
if(assimp_FOUND)
    message(STATUS "Assimp found - mesh loading enabled")
    add_compile_definitions(HAVE_ASSIMP)
else()
    message(WARNING "Assimp not found - mesh loading will be disabled. Install with: vcpkg install assimp")
endif()

# OpenMP — parallel deformation (optional, unchanged)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - parallel NRICP enabled")
else()
    message(WARNING "OpenMP not found - NRICP will run single-threaded")
endif()

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Sources — Sprint 3.5.2 scope
# ---------------------------------------------------------------------------
# The remaining Qt Widgets UI files (MainWindow, SidebarWidget, dialogs,
# ViewportWidget, ViewportContainer, ErrorHelper) are still excluded while
# their ImGui replacements are built. They return in Story 3.5.3 (Viewport)
# and Sprint 3.6 (sidebars + menus + dialogs).

set(CORE_SOURCES
    src/core/Mesh.cpp
    src/core/Transform.cpp
    src/core/Project.cpp
    src/core/Camera.cpp
    src/core/UndoStack.cpp
    src/core/TextureData.cpp
    src/rendering/Renderer.cpp
    src/rendering/MeshRenderer.cpp
    src/rendering/ShaderManager.cpp
    src/rendering/PointRenderer.cpp
    src/rendering/GizmoRenderer.cpp
    src/ui/Viewport.cpp
    src/ui/ViewportHost.cpp
    src/ui/FileDialogs.cpp
    src/ui/Modals.cpp
    src/ui/StageSidebar.cpp
    src/utils/RayCaster.cpp
    src/utils/Logger.cpp
    src/utils/BVH.cpp
    src/utils/SpatialHash.cpp
    src/utils/Paths.cpp
    src/utils/Config.cpp
    src/utils/OpenUrl.cpp
    src/utils/SingleInstance.cpp
    src/utils/HttpClient.cpp
    src/utils/SecureStorage.cpp
    src/deformation/RBFInterpolator.cpp
    src/deformation/MeshDeformer.cpp
    src/deformation/NRICPDeformer.cpp
    src/sculpting/SculptBrush.cpp
    src/sculpting/SmoothBrush.cpp
    src/sculpting/GrabBrush.cpp
    src/sculpting/PushPullBrush.cpp
    src/sculpting/InflateBrush.cpp
    src/sculpting/MaskBrush.cpp
    src/io/MeshExporter.cpp
    src/io/ProjectSerializer.cpp
    src/baking/TextureBaker.cpp
    src/baking/TextureWriter.cpp
)

set(CORE_HEADERS
    include/ui/AppWindow.h
    include/ui/Viewport.h
    include/ui/ViewportHost.h
    include/ui/FileDialogs.h
    include/ui/Modals.h
    include/ui/StageSidebar.h
    include/core/Mesh.h
    include/core/Transform.h
    include/core/Project.h
    include/core/Camera.h
    include/core/Types.h
    include/core/UndoableAction.h
    include/core/UndoStack.h
    include/core/UndoActions.h
    include/core/TextureData.h
    include/rendering/Renderer.h
    include/rendering/MeshRenderer.h
    include/rendering/ShaderManager.h
    include/rendering/PointRenderer.h
    include/rendering/GizmoRenderer.h
    include/utils/RayCaster.h
    include/utils/Logger.h
    include/utils/BVH.h
    include/utils/SpatialHash.h
    include/utils/Paths.h
    include/utils/Config.h
    include/utils/OpenUrl.h
    include/utils/SingleInstance.h
    include/utils/HttpClient.h
    include/utils/SecureStorage.h
    include/deformation/RBFInterpolator.h
    include/deformation/MeshDeformer.h
    include/deformation/NRICPDeformer.h
    include/sculpting/SculptBrush.h
    include/sculpting/SmoothBrush.h
    include/sculpting/GrabBrush.h
    include/sculpting/PushPullBrush.h
    include/sculpting/InflateBrush.h
    include/sculpting/MaskBrush.h
    include/sculpting/BrushStroke.h
    include/io/MeshExporter.h
    include/io/ProjectSerializer.h
    include/baking/TextureBaker.h
    include/baking/TextureWriter.h
)

# NOTE: Sprint 3.6 retired the Qt-era UI files (MainWindow / ViewportWidget /
# ViewportContainer / SidebarWidget / ExportDialog / ShortcutsDialog /
# TexturePreviewDialog / WelcomeDialog / ErrorHelper); Sprint 3.7 deleted them
# outright once the ImGui replacements were verified.

# Static library of engine + rendering translation units, consumed by the
# executable target below.
add_library(${PROJECT_NAME}Core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Sprint 3.5.2: application shell — GLFW window, GL context, ImGui frame.
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/ui/AppWindow.cpp
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}Core
        imgui::imgui
        glfw
        GLEW::GLEW
        OpenGL::GL
)

# ---------------------------------------------------------------------------
# Link
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}Core
    PUBLIC
        OpenGL::GL
        GLEW::GLEW
        glfw
        imgui::imgui
        nlohmann_json::nlohmann_json
        nfd::nfd
        CURL::libcurl
        Eigen3::Eigen
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME}Core PUBLIC OpenMP::OpenMP_CXX)
endif()

if(assimp_FOUND)
    target_link_libraries(${PROJECT_NAME}Core PUBLIC assimp::assimp)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME}Core PRIVATE /W4)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}Core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# ---------------------------------------------------------------------------
# Asset staging
# ---------------------------------------------------------------------------
if(MSVC)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# Installation rules remain deferred until Sprint 3.6 reintroduces the full
# UI surface (menus, dialogs, sidebars) needed for distribution packaging.

# ---------------------------------------------------------------------------
# Smoke test (Story 3.7.6)
# ---------------------------------------------------------------------------
# Headless test that exercises the load → serialize → reload → export pipeline
# against the bundled MetaHuman head. Catches regressions in the engine layers
# (Mesh, Project, ProjectSerializer, MeshExporter) without needing a GL
# context. Build it as part of the default target so a green build implies the
# core engine round-trip still works end-to-end.
enable_testing()

add_executable(metavisage_smoke
    tests/smoke/launch_to_export.cpp
)
target_link_libraries(metavisage_smoke
    PRIVATE
        ${PROJECT_NAME}Core
)
if(MSVC)
    target_compile_options(metavisage_smoke PRIVATE /W4)
else()
    target_compile_options(metavisage_smoke PRIVATE -Wall -Wextra)
endif()

add_test(NAME smoke_load_serialize_export
         COMMAND metavisage_smoke
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
