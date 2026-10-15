# Building MetaVisage

MetaVisage is a standalone Windows C++20 application. No account, API keys,
environment profiles, or online license service is required to build or run it.
Dependency downloads require network access during the initial vcpkg setup.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++ and a Windows SDK
- CMake 3.16 or newer and vcpkg
- An OpenGL 4.3 capable GPU for the application and UI smoke test

The vcpkg manifest installs GLFW, Dear ImGui (including the test engine), GLEW,
Eigen, Assimp, nlohmann/json, and Native File Dialog Extended. Qt is not used.

## Build

Run from the repository root in PowerShell:

```powershell
.\build-msvc.ps1 -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

The executable is `build/bin/Release/MetaVisage.exe`. Close any running instance
before relinking; otherwise MSVC may fail with LNK1104. The helper preserves
vcpkg's runtime DLL deployment. Use `-BuildType Debug` for a debug build.

For manual CMake configuration:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

MinGW remains available through `build-mingw.ps1`, but needs a compatible
MinGW compiler and vcpkg triplet. The default MSVC-built x64-windows libraries
cannot be linked with MinGW. MSVC is the validated toolchain.

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -L ui --output-on-failure
```

The four smoke targets cover mesh round-trip/export, v1 project compatibility,
wrap parity, and UI startup/navigation/shading/brush selection. The UI lane opens
a real GLFW window. Wrap parity can take about 70 seconds (120-second timeout).

## Local data

Settings, recent projects, and inspector preferences live under
`%APPDATA%/MetaVisage`. Legacy account fields are ignored and omitted when
settings are saved. The standalone application does not register browser URL
handlers, contact a web service, or report exports.
