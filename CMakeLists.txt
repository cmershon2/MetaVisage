cmake_minimum_required(VERSION 3.16)
project(MetaVisage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 - check common installation paths
if(NOT DEFINED CMAKE_PREFIX_PATH)
    if(EXISTS "C:/Qt/6.10.1/msvc2022_64")
        list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")
    endif()
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find Assimp (optional for Sprint 2 testing)
find_package(assimp CONFIG)

if(assimp_FOUND)
    message(STATUS "Assimp found - mesh loading enabled")
    add_compile_definitions(HAVE_ASSIMP)
else()
    message(WARNING "Assimp not found - mesh loading will be disabled. Install with: vcpkg install assimp:x64-mingw-dynamic")
endif()

# Find OpenMP for parallel computation
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - parallel NRICP enabled")
else()
    message(WARNING "OpenMP not found - NRICP will run single-threaded")
endif()

# Find Eigen3 (required for Sprint 6 deformation)
find_package(Eigen3 CONFIG REQUIRED)

if(Eigen3_FOUND)
    message(STATUS "Eigen3 found - deformation algorithms enabled")
else()
    message(FATAL_ERROR "Eigen3 not found. Install with: vcpkg install eigen3:x64-windows")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ViewportWidget.cpp
    src/ui/ViewportContainer.cpp
    src/ui/SidebarWidget.cpp
    src/core/Mesh.cpp
    src/core/Transform.cpp
    src/core/Project.cpp
    src/core/Camera.cpp
    src/rendering/Renderer.cpp
    src/rendering/MeshRenderer.cpp
    src/rendering/ShaderManager.cpp
    src/rendering/PointRenderer.cpp
    src/utils/RayCaster.cpp
    src/deformation/RBFInterpolator.cpp
    src/deformation/MeshDeformer.cpp
    src/deformation/NRICPDeformer.cpp
    src/sculpting/SculptBrush.cpp
    src/sculpting/SmoothBrush.cpp
    src/sculpting/GrabBrush.cpp
    src/sculpting/PushPullBrush.cpp
    src/sculpting/InflateBrush.cpp
    src/io/MeshExporter.cpp
    src/io/ProjectSerializer.cpp
    src/ui/ExportDialog.cpp
    src/utils/Logger.cpp
    src/utils/ErrorHelper.cpp
    src/core/UndoStack.cpp
    src/utils/BVH.cpp
    src/utils/SpatialHash.cpp
    src/ui/ShortcutsDialog.cpp
    src/ui/WelcomeDialog.cpp
)

# Header files
set(HEADERS
    include/ui/MainWindow.h
    include/ui/ViewportWidget.h
    include/ui/ViewportContainer.h
    include/ui/SidebarWidget.h
    include/core/Mesh.h
    include/core/Transform.h
    include/core/Project.h
    include/core/Camera.h
    include/core/Types.h
    include/rendering/Renderer.h
    include/rendering/MeshRenderer.h
    include/rendering/ShaderManager.h
    include/rendering/PointRenderer.h
    include/utils/RayCaster.h
    include/deformation/RBFInterpolator.h
    include/deformation/MeshDeformer.h
    include/deformation/NRICPDeformer.h
    include/sculpting/SculptBrush.h
    include/sculpting/SmoothBrush.h
    include/sculpting/GrabBrush.h
    include/sculpting/PushPullBrush.h
    include/sculpting/InflateBrush.h
    include/sculpting/BrushStroke.h
    include/io/MeshExporter.h
    include/io/ProjectSerializer.h
    include/ui/ExportDialog.h
    include/utils/Logger.h
    include/utils/ErrorHelper.h
    include/core/UndoableAction.h
    include/core/UndoStack.h
    include/core/UndoActions.h
    include/utils/BVH.h
    include/utils/SpatialHash.h
    include/ui/ShortcutsDialog.h
    include/ui/WelcomeDialog.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    Eigen3::Eigen
)

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()

# Link Assimp if found
if(assimp_FOUND)
    target_link_libraries(${PROJECT_NAME} assimp::assimp)
endif()

# Set compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy assets to build directory
# For multi-config generators (MSVC), copy to each config directory
if(MSVC)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
         DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
