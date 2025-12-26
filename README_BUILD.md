# Quick Build Guide

This is a condensed build guide. For detailed instructions, see [BUILD.md](BUILD.md).

## For MinGW Users (Recommended - Qt 6.10.1/mingw_64)

### Quick Build (PowerShell Script)

The easiest way to build:

```powershell
# First time build (with deployment)
.\build-mingw.ps1 -Clean -Deploy

# Subsequent builds
.\build-mingw.ps1

# With vcpkg (Sprint 2+)
.\build-mingw.ps1 -Clean -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Manual Build

```powershell
# Navigate to project
cd "C:\Users\Casey Mershon\Desktop\Web Dev\MetaVisage"

# Clean and create build directory
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
mkdir build
cd build

# Add MinGW to PATH
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path

# Configure
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/mingw_64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe"

# Build
mingw32-make

# Deploy Qt DLLs
cd bin
windeployqt MetaVisage.exe

# Run
.\MetaVisage.exe
```

## For MSVC Users (Qt with msvc2019_64 or msvc2022_64)

```cmd
cd "C:\Users\Casey Mershon\Desktop\Web Dev\MetaVisage"
mkdir build
cd build

cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -G "Visual Studio 17 2022" -A x64

cmake --build . --config Release

bin\Release\MetaVisage.exe
```

## With vcpkg (Sprint 2+ for Assimp)

### Install Assimp via vcpkg

```powershell
# For MinGW
vcpkg install assimp:x64-mingw-dynamic

# For MSVC
vcpkg install assimp:x64-windows
```

### Add vcpkg to build

**MinGW:**
```powershell
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/mingw_64" -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe"
```

**MSVC:**
```cmd
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64
```

## Common Issues

| Issue | Solution |
|-------|----------|
| `mingw32-make not found` | Add MinGW to PATH: `$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path` |
| `Qt6Core.dll not found` | Run `windeployqt MetaVisage.exe` in the bin directory |
| CMake uses MSVC instead of MinGW | Clean build dir and specify compilers explicitly |
| Assimp not found | Install via vcpkg and add toolchain file to CMake |

## Build Script Parameters

The PowerShell build script supports:

```powershell
.\build-mingw.ps1 [options]

Options:
  -Clean              Clean build directory before building
  -Deploy             Run windeployqt to copy Qt DLLs
  -BuildType <type>   Release or Debug (default: Release)
  -QtPath <path>      Path to Qt (default: C:/Qt/6.10.1/mingw_64)
  -MinGWPath <path>   Path to MinGW (default: C:/Qt/Tools/mingw1310_64)
  -VcpkgToolchain <path>  Path to vcpkg toolchain file

Examples:
  .\build-mingw.ps1 -Clean -Deploy
  .\build-mingw.ps1 -BuildType Debug
  .\build-mingw.ps1 -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

---

For detailed troubleshooting and alternative build methods, see [BUILD.md](BUILD.md).
