# Quick build guide

The full setup and troubleshooting guide is [BUILD.md](BUILD.md).

Use Windows 10/11 x64, Visual Studio 2022 with Desktop development with C++,
a Windows SDK, CMake 3.16+, Git, and vcpkg. MetaVisage uses C++20 and Dear ImGui.

From the repository root in PowerShell, replace the toolchain path with your
vcpkg installation:

```powershell
.\build-msvc.ps1 -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
ctest --test-dir build -C Release --output-on-failure
Push-Location build/bin/Release
.\MetaVisage.exe
Pop-Location
```

The executable needs the DLLs and `assets/` directory staged beside it. Close
the application before rebuilding. Use `-BuildType Debug` for a debug build.

MSVC is the validated toolchain. The MinGW helper is retained for development
with compatible MinGW-built dependencies; see [BUILD.md](BUILD.md#mingw).
No Qt installation, account, or service credentials are required.
