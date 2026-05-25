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

# jwt-cpp + OpenSSL — RS256 license JWT verification (Sprint 6 Story 6.3).
# vcpkg's libcurl on Windows ships Schannel, not OpenSSL, so jwt-cpp's
# OpenSSL backend pulls in libssl/libcrypto explicitly.
find_package(jwt-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

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
# Build profile + licensing configuration (Sprint 6 Story 6.1)
# ---------------------------------------------------------------------------
# Compile-time licensing config driven by the active .env.<profile> file. The
# PowerShell helpers (build-msvc.ps1 / build-mingw.ps1 -Profile <name>) read
# .env.<profile> from the project root and forward each value as a -D cache
# variable. Running cmake by hand without the helper falls back to the
# development profile (localhost) so quick iteration still works; staging
# and production refuse to configure with empty values.
set(METAVISAGE_BUILD_PROFILE "development" CACHE STRING
    "Build profile: development | staging | production")
set_property(CACHE METAVISAGE_BUILD_PROFILE PROPERTY STRINGS
    development staging production)

set(METAVISAGE_API_BASE_URL "" CACHE STRING
    "Base URL for the MetaVisage web portal API")
set(METAVISAGE_SUPABASE_PROJECT_URL "" CACHE STRING
    "Supabase project URL — used for the access-token refresh endpoint")
set(METAVISAGE_SUPABASE_ANON_KEY "" CACHE STRING
    "Supabase NEXT_PUBLIC_ anon key — safe to embed in the desktop binary")
set(METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE "" CACHE STRING
    "Path to the RS256 license JWT public key (PEM, SPKI). Relative paths resolve from the project root.")

# Apply development defaults if no helper script ran. Staging / production
# require explicit values; refusing to configure protects against shipping a
# binary that points at localhost or has no public key.
if(METAVISAGE_API_BASE_URL STREQUAL "")
    if(METAVISAGE_BUILD_PROFILE STREQUAL "development")
        set(METAVISAGE_API_BASE_URL "http://localhost:3000")
    else()
        message(FATAL_ERROR
            "METAVISAGE_BUILD_PROFILE=${METAVISAGE_BUILD_PROFILE} but "
            "METAVISAGE_API_BASE_URL is empty. Run the build through "
            "build-msvc.ps1 -Profile ${METAVISAGE_BUILD_PROFILE} or pass -D "
            "values explicitly.")
    endif()
endif()

# PR #25 review fix: parity guard for the Supabase refresh credentials on
# non-development profiles. An empty project URL or anon key would silently
# disable the access-token refresh path (LicenseManager::RefreshThreadFn
# falls through to AuthInvalid on every 401 instead of refreshing), which
# would force re-auth every ~hour in production. Cheap to catch at configure
# time. Development profile leaves them empty-tolerable.
if(NOT METAVISAGE_BUILD_PROFILE STREQUAL "development")
    if(METAVISAGE_SUPABASE_PROJECT_URL STREQUAL "")
        message(FATAL_ERROR
            "METAVISAGE_BUILD_PROFILE=${METAVISAGE_BUILD_PROFILE} but "
            "METAVISAGE_SUPABASE_PROJECT_URL is empty. The Supabase "
            "access-token refresh path needs the project URL.")
    endif()
    if(METAVISAGE_SUPABASE_ANON_KEY STREQUAL "")
        message(FATAL_ERROR
            "METAVISAGE_BUILD_PROFILE=${METAVISAGE_BUILD_PROFILE} but "
            "METAVISAGE_SUPABASE_ANON_KEY is empty. The Supabase "
            "access-token refresh path needs the NEXT_PUBLIC anon key.")
    endif()
endif()

# Read the public-key PEM from disk so callers don't wrestle with multi-line
# CMake string escaping. Empty path is allowed in development; LicenseManager
# falls back to skipping signature verification when the key is empty.
set(METAVISAGE_LICENSE_JWT_PUBLIC_KEY "")
if(NOT METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE STREQUAL "")
    if(IS_ABSOLUTE "${METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE}")
        set(_resolved_key_path "${METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE}")
    else()
        set(_resolved_key_path
            "${CMAKE_CURRENT_SOURCE_DIR}/${METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE}")
    endif()
    if(EXISTS "${_resolved_key_path}")
        file(READ "${_resolved_key_path}" METAVISAGE_LICENSE_JWT_PUBLIC_KEY)
    else()
        message(WARNING
            "METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE points to a non-existent "
            "file: ${_resolved_key_path}. The build will continue, but JWT "
            "signature verification will fail at runtime.")
    endif()
endif()

if(METAVISAGE_BUILD_PROFILE STREQUAL "production"
   AND METAVISAGE_LICENSE_JWT_PUBLIC_KEY STREQUAL "")
    message(FATAL_ERROR
        "METAVISAGE_BUILD_PROFILE=production but no public key was loaded "
        "from METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE. Refusing to ship a "
        "production binary that cannot verify license JWTs.")
endif()

message(STATUS "MetaVisage build profile: ${METAVISAGE_BUILD_PROFILE}")
message(STATUS "  API base URL:        ${METAVISAGE_API_BASE_URL}")
message(STATUS "  Supabase project:    ${METAVISAGE_SUPABASE_PROJECT_URL}")
message(STATUS "  Public key file:     ${METAVISAGE_LICENSE_JWT_PUBLIC_KEY_FILE}")

# Generate the consumer header from cmake/LicenseConfig.h.in. LicenseManager
# (Sprint 6 Story 6.1) includes "licensing/LicenseConfig.h" without caring
# whether the file lives in source or build tree.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LicenseConfig.h.in
    ${CMAKE_BINARY_DIR}/generated/licensing/LicenseConfig.h
    @ONLY
)

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
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
    src/core/WrapPreset.cpp
    src/rendering/Renderer.cpp
    src/rendering/MeshRenderer.cpp
    src/rendering/ShaderManager.cpp
    src/rendering/PointRenderer.cpp
    src/rendering/GizmoRenderer.cpp
    src/ui/Viewport.cpp
    src/ui/ViewportHost.cpp
    src/ui/FileDialogs.cpp
    src/ui/LoginScreen.cpp
    src/ui/Modals.cpp
    src/ui/MvInspectorPrimitives.cpp
    src/ui/MvStyle.cpp
    src/ui/StageSidebar.cpp
    src/ui/StageStepper.cpp
    src/ui/ToolRail.cpp
    src/ui/UpgradePromptWidget.cpp
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
    src/io/PresetStore.cpp
    src/baking/TextureBaker.cpp
    src/baking/TextureWriter.cpp
    src/licensing/LicenseManager.cpp
)

set(CORE_HEADERS
    include/ui/AppWindow.h
    include/ui/Viewport.h
    include/ui/ViewportHost.h
    include/ui/FileDialogs.h
    include/ui/LoginScreen.h
    include/ui/Modals.h
    include/ui/MvInspectorPrimitives.h
    include/ui/MvStyle.h
    include/ui/StageSidebar.h
    include/ui/StageStepper.h
    include/ui/ToolRail.h
    include/ui/UpgradePromptWidget.h
    include/core/Mesh.h
    include/core/Transform.h
    include/core/Project.h
    include/core/Camera.h
    include/core/Types.h
    include/core/UndoableAction.h
    include/core/UndoStack.h
    include/core/UndoActions.h
    include/core/TextureData.h
    include/core/WrapPreset.h
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
    include/io/PresetStore.h
    include/baking/TextureBaker.h
    include/baking/TextureWriter.h
    include/licensing/LicenseManager.h
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
        jwt-cpp::jwt-cpp
        OpenSSL::SSL
        OpenSSL::Crypto
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

# ---------------------------------------------------------------------------
# v1.0.0 backward-compat regression (Story 4.1)
# ---------------------------------------------------------------------------
# Loads a set of minimal .mmproj fixtures carrying every discontinued
# DeformationAlgorithm ordinal (RBF_TPS / RBF_GAUSSIAN / RBF_MULTIQUADRIC /
# ARAP) and asserts the serializer coerces each silently to NRICP. Catches
# regressions in the Sprint 4 algorithm-collapse path without needing GL.
add_executable(metavisage_v1_compat
    tests/smoke/v1_backward_compat.cpp
)
target_link_libraries(metavisage_v1_compat
    PRIVATE
        ${PROJECT_NAME}Core
)
if(MSVC)
    target_compile_options(metavisage_v1_compat PRIVATE /W4)
else()
    target_compile_options(metavisage_v1_compat PRIVATE -Wall -Wextra)
endif()

# Stage the fixtures next to the test exe so the LocateFixture path walk
# resolves on Windows builds (build/bin/<config>/fixtures/) and Linux/Mac
# (build/<config>/fixtures/).
add_custom_command(TARGET metavisage_v1_compat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/tests/fixtures"
            "$<TARGET_FILE_DIR:metavisage_v1_compat>/fixtures"
    COMMENT "Staging v1.0.0 .mmproj fixtures next to metavisage_v1_compat")

add_test(NAME smoke_v1_backward_compat
         COMMAND metavisage_v1_compat
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ---------------------------------------------------------------------------
# Wrap Engine behavioural-parity smoke (Story 4.6)
# ---------------------------------------------------------------------------
# Loads the bundled MetaHuman head as both source and target with a small
# Y-offset, places four correspondence anchors, runs MeshDeformer for both
# Quick Wrap and Fantasy Wrap presets, and asserts each converges with
# preserved topology + bounded displacement. Catches the bug class Sprint
# 3.8.9 first shipped — preset numbers that look reasonable in the
# inspector but produce a visibly broken wrap.
add_executable(metavisage_smoke_wrap_parity
    tests/smoke/wrap_parity.cpp
)
target_link_libraries(metavisage_smoke_wrap_parity
    PRIVATE
        ${PROJECT_NAME}Core
)
if(MSVC)
    target_compile_options(metavisage_smoke_wrap_parity PRIVATE /W4)
else()
    target_compile_options(metavisage_smoke_wrap_parity PRIVATE -Wall -Wextra)
endif()

# Wrap-parity needs the bundled MetaHuman head — same asset the existing
# smoke_load_serialize_export target resolves via Paths::ExecutableDir().
# No fixture-staging step here because the test reads from the existing
# build/bin/<config>/assets/ tree the main app already populates.

add_test(NAME smoke_wrap_parity
         COMMAND metavisage_smoke_wrap_parity
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set_tests_properties(smoke_wrap_parity PROPERTIES TIMEOUT 120)

# ---------------------------------------------------------------------------
# imgui_test_engine smoke-UI lane (Story 5.4)
# ---------------------------------------------------------------------------
# Drives the live AppWindow through three click-through scenarios via
# imgui_test_engine — stage stepper, login screen toggle cycle, viewport
# toolbar shading-mode toggles. Catches the click-through and modal-toggle
# bug classes the Sprint 3.6 / 3.7 / 3.8 retros all flagged. Real GLFW
# window pops up briefly during ctest (no headless mode); deps inherit
# through MetaVisageCore's PUBLIC link set (imgui::imgui pulls in the
# folded-in test-engine library — see vcpkg.json's imgui[test-engine]
# feature). AppWindow.cpp is compiled into the smoke directly because
# the main exe owns it (CMakeLists.txt line 208) — pulling it into the
# core library would force the smoke to also drag main.cpp's startup
# code, which it doesn't need.
add_executable(metavisage_smoke_ui
    tests/smoke/ui_smoke.cpp
    src/ui/AppWindow.cpp
)
target_link_libraries(metavisage_smoke_ui
    PRIVATE
        ${PROJECT_NAME}Core
)
if(MSVC)
    target_compile_options(metavisage_smoke_ui PRIVATE /W4)
else()
    target_compile_options(metavisage_smoke_ui PRIVATE -Wall -Wextra)
endif()

add_test(NAME smoke_ui
         COMMAND metavisage_smoke_ui
         WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# 90 s CTest budget = 60 s harness deadline + 30 s init/teardown buffer.
# Filterable via `ctest -L ui` for local iteration.
set_tests_properties(smoke_ui PROPERTIES
    LABELS ui
    TIMEOUT 90)
