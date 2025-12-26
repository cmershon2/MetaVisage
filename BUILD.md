# Build Instructions - MetaVisage

This document provides detailed instructions for building MetaVisage from source on Windows.

## Prerequisites

### Required Software

1. **Qt 6.x** (6.5 or later recommended)
   - Download from: https://www.qt.io/download-qt-installer
   - During installation, select:
     - Qt 6.x for MSVC 2019 64-bit
     - Qt Creator (optional but recommended)
     - CMake integration

2. **CMake** (3.16 or later)
   - Download from: https://cmake.org/download/
   - Add CMake to system PATH during installation

3. **Visual Studio 2019 or 2022**
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Install with "Desktop development with C++" workload
   - Ensure C++17 support is enabled

4. **Git**
   - Download from: https://git-scm.com/download/win
   - Use Git Bash or your preferred Git client

5. **Assimp** (Asset Importer Library) - **REQUIRED for Sprint 2+**
   - **Recommended:** Install via vcpkg (see section below)
   - Alternative: Download pre-built binaries from https://github.com/assimp/assimp/releases

### Optional Tools

- **Qt Creator** - Recommended IDE for Qt development
- **Visual Studio Code** - Alternative IDE with C++ extensions
- **Ninja** - Faster build system (alternative to MSBuild)
- **vcpkg** - C++ package manager (recommended for Assimp installation)

## Installing Assimp (Required for Sprint 2+)

### Method 1: Using vcpkg (Recommended)

1. **Install vcpkg** (if not already installed)
   ```cmd
   cd C:\
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   bootstrap-vcpkg.bat
   ```

2. **Install Assimp**
   ```cmd
   vcpkg install assimp:x64-windows
   ```

3. **Integrate with CMake**
   ```cmd
   vcpkg integrate install
   ```

   This command will output a CMake toolchain file path. Copy this path for later use.

### Method 2: Manual Installation

1. Download Assimp pre-built binaries from:
   https://github.com/assimp/assimp/releases

2. Extract to a location (e.g., `C:\Libraries\assimp`)

3. Add to your CMake command: `-DCMAKE_PREFIX_PATH="C:\Libraries\assimp;C:\Qt\..."`

## Building the Project

### Method 1: Command Line Build with MinGW (Recommended for Qt mingw_64)

**Use this method if you have Qt with MinGW (e.g., Qt 6.10.1/mingw_64)**

1. **Open PowerShell**
   - Start Menu → PowerShell

2. **Navigate to the project directory**
   ```powershell
   cd "C:\Users\Casey Mershon\Desktop\Web Dev\MetaVisage"
   ```

3. **Clean and create build directory**
   ```powershell
   Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
   mkdir build
   cd build
   ```

4. **Add MinGW to PATH**
   ```powershell
   $env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path
   ```

   **Note:** Adjust MinGW version to match your Qt Tools installation (check `C:\Qt\Tools\`)

5. **Configure with CMake**

   **Without vcpkg:**
   ```powershell
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/mingw_64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe"
   ```

   **With vcpkg (for Assimp - Sprint 2+):**
   ```powershell
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/mingw_64" -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe"
   ```

   **Important:** Adjust these paths:
   - Qt path: `C:/Qt/6.10.1/mingw_64` (match your version)
   - MinGW tools: `C:/Qt/Tools/mingw1310_64` (match your version from `C:\Qt\Tools\`)
   - vcpkg toolchain: `C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

6. **Build the project**
   ```powershell
   mingw32-make
   ```

7. **Deploy Qt dependencies**
   ```powershell
   cd bin
   windeployqt MetaVisage.exe
   ```

8. **Run the application**
   ```powershell
   .\MetaVisage.exe
   ```

### Method 2: MSVC Build (if you have Qt with MSVC)

**Use this method if you have Qt with MSVC (e.g., Qt 6.x.x/msvc2019_64 or msvc2022_64)**

1. **Open Developer Command Prompt for Visual Studio**
   - Start Menu → Visual Studio 2022 → Developer Command Prompt

2. **Navigate to the project directory**
   ```cmd
   cd "C:\Users\Casey Mershon\Desktop\Web Dev\MetaVisage"
   ```

3. **Create build directory**
   ```cmd
   mkdir build
   cd build
   ```

4. **Configure with CMake**

   **Without vcpkg:**
   ```cmd
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -G "Visual Studio 17 2022" -A x64
   ```

   **With vcpkg (for Assimp - Sprint 2+):**
   ```cmd
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64
   ```

5. **Build the project**
   ```cmd
   cmake --build . --config Release
   ```

6. **Run the application**
   ```cmd
   bin\Release\MetaVisage.exe
   ```

### Method 3: Qt Creator

1. **Open Qt Creator**

2. **Open Project**
   - File → Open File or Project
   - Navigate to `MetaVisage` folder
   - Select `CMakeLists.txt`

3. **Configure Kit**
   - Select "Desktop Qt 6.x MinGW 64bit" or "Desktop Qt 6.x MSVC2019/2022 64bit" kit
   - If using vcpkg, add to CMake arguments: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`
   - Click "Configure Project"

4. **Build**
   - Click the hammer icon or press Ctrl+B
   - Build mode selector: Debug or Release

5. **Run**
   - Click the green play button or press Ctrl+R

### Method 4: Visual Studio 2022 (MSVC only)

1. **Open Visual Studio 2022**

2. **Open a Local Folder**
   - File → Open → Folder
   - Select the `MetaVisage` folder

3. **Configure CMake Settings**
   - Right-click `CMakeLists.txt` → CMake Settings
   - Set `CMAKE_PREFIX_PATH` to your Qt MSVC installation path
   - Example: `C:/Qt/6.x.x/msvc2022_64`
   - If using vcpkg, add to CMake command arguments: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

4. **Build**
   - Build → Build All
   - Or press Ctrl+Shift+B

5. **Set Startup Item**
   - Select `MetaVisage.exe` from the startup item dropdown

6. **Run**
   - Debug → Start Without Debugging (Ctrl+F5)

## Troubleshooting

### MinGW-Specific Issues

#### CMake uses wrong compiler (MSVC instead of MinGW)

**Error:** CMake detects MSVC compiler when you want MinGW, or you get:
```
fatal error C1189: #error: "Qt requires a C++17 compiler, and a suitable value for __cplusplus"
```

**Solution:**
- Clean the build directory completely: `Remove-Item -Recurse -Force build`
- Explicitly specify compilers in CMake command:
  ```powershell
  -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"
  ```

#### mingw32-make not found

**Error:** `The term 'mingw32-make' is not recognized`

**Solution:**
- Add MinGW to PATH before running CMake:
  ```powershell
  $env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path
  ```
- Verify MinGW version matches your Qt Tools installation (check `C:\Qt\Tools\`)

#### Qt DLLs not found when running

**Error:** `Qt6Core.dll was not found`, `Qt6Gui.dll was not found`, etc.

**Solution:**
- Run `windeployqt` after building:
  ```powershell
  cd bin
  windeployqt MetaVisage.exe
  ```
- This copies all required Qt DLLs to the executable directory

### Assimp Not Found

**Error:** `Could not find a package configuration file provided by "assimp"`

**Solution:**
- If using vcpkg:
  - For MinGW: `vcpkg install assimp:x64-mingw-dynamic`
  - For MSVC: `vcpkg install assimp:x64-windows`
  - Ensure you ran `vcpkg integrate install`
  - Add `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake` to CMake command
- If using manual installation:
  - Ensure Assimp is in CMAKE_PREFIX_PATH
  - Verify you downloaded the version matching your compiler (MinGW vs MSVC)

### Qt Not Found

**Error:** `Could not find a package configuration file provided by "Qt6"`

**Solution:**
- Verify Qt is installed
- Check CMAKE_PREFIX_PATH is set correctly
- Ensure you're using the correct Qt version (6.x, not 5.x)
- For MinGW builds, use the mingw_64 Qt installation
- For MSVC builds, use the msvc2019_64 or msvc2022_64 Qt installation

### OpenGL Headers Not Found

**Error:** `Cannot find OpenGL headers`

**Solution:**
- Install Windows SDK (included with Visual Studio)
- Ensure Visual Studio has "Desktop development with C++" workload

### MSVC Compiler Not Found

**Error:** `No CMAKE_CXX_COMPILER could be found`

**Solution:**
- Open Visual Studio Installer
- Modify your installation
- Ensure "MSVC v142 - VS 2019 C++ x64/x86 build tools" is installed

### Link Errors with Qt Libraries

**Error:** `unresolved external symbol` errors

**Solution:**
- Ensure CMAKE_PREFIX_PATH matches your Qt installation
- Verify you're building for the same architecture (x64) as Qt installation
- Clean build directory and reconfigure

## Build Configuration Options

### Debug vs Release

**Debug Build:**
- Includes debug symbols
- No optimizations
- Larger executable size
- Use for development and debugging

```cmd
cmake --build . --config Debug
```

**Release Build:**
- Optimized for performance
- Smaller executable size
- Use for distribution

```cmd
cmake --build . --config Release
```

### Clean Build

To start fresh:

```cmd
cd build
cmake --build . --target clean
```

Or delete the build directory:

```cmd
cd ..
rmdir /s /q build
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"
```

## Next Steps

After successfully building:

1. **Verify the build**
   - Application should launch and display the main window
   - Check the About dialog (Help → About)

2. **Continue development**
   - See [docs/SPRINT_PLAN.md](docs/SPRINT_PLAN.md) for the development roadmap
   - Current sprint tasks are tracked in the sprint plan

3. **Report issues**
   - If you encounter build problems, please report them
   - Include: OS version, Qt version, CMake output, error messages

## Environment Variables

For convenience, add these to your system environment:

```cmd
set Qt6_DIR=C:/Qt/6.5.0/msvc2019_64
set CMAKE_PREFIX_PATH=%Qt6_DIR%
```

Add to system PATH:
- CMake bin directory (e.g., `C:\Program Files\CMake\bin`)
- Qt bin directory (e.g., `C:\Qt\6.5.0\msvc2019_64\bin`)

## IDE-Specific Notes

### Visual Studio Code

1. Install extensions:
   - C/C++
   - CMake Tools
   - Qt tools

2. Configure `settings.json`:
   ```json
   {
     "cmake.configureSettings": {
       "CMAKE_PREFIX_PATH": "C:/Qt/6.5.0/msvc2019_64"
     }
   }
   ```

### CLion

1. File → Settings → Build, Execution, Deployment → CMake
2. Add CMake options: `-DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/msvc2019_64`
3. Build → Build Project

---

**Last Updated:** December 26, 2025
