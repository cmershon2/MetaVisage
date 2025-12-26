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

### Optional Tools

- **Qt Creator** - Recommended IDE for Qt development
- **Visual Studio Code** - Alternative IDE with C++ extensions
- **Ninja** - Faster build system (alternative to MSBuild)

## Building the Project

### Method 1: Command Line Build

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
   ```cmd
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" -G "Visual Studio 17 2022" -A x64
   ```

   **Note:** Adjust the Qt path to match your installation.
   Common Qt installation paths:
   - `C:/Qt/6.5.0/msvc2019_64`
   - `C:/Qt/6.6.0/msvc2019_64`
   - `C:/Program Files/Qt/6.x.x/msvc2019_64`

5. **Build the project**
   ```cmd
   cmake --build . --config Release
   ```

   For debug builds:
   ```cmd
   cmake --build . --config Debug
   ```

6. **Run the application**
   ```cmd
   bin\Release\MetaVisage.exe
   ```

### Method 2: Qt Creator

1. **Open Qt Creator**

2. **Open Project**
   - File → Open File or Project
   - Navigate to `MetaVisage` folder
   - Select `CMakeLists.txt`

3. **Configure Kit**
   - Select "Desktop Qt 6.x MSVC2019 64bit" kit
   - Click "Configure Project"

4. **Build**
   - Click the hammer icon or press Ctrl+B
   - Build mode selector: Debug or Release

5. **Run**
   - Click the green play button or press Ctrl+R

### Method 3: Visual Studio 2022

1. **Open Visual Studio 2022**

2. **Open a Local Folder**
   - File → Open → Folder
   - Select the `MetaVisage` folder

3. **Configure CMake Settings**
   - Right-click `CMakeLists.txt` → CMake Settings
   - Set `CMAKE_PREFIX_PATH` to your Qt installation path
   - Example: `C:/Qt/6.5.0/msvc2019_64`

4. **Build**
   - Build → Build All
   - Or press Ctrl+Shift+B

5. **Set Startup Item**
   - Select `MetaVisage.exe` from the startup item dropdown

6. **Run**
   - Debug → Start Without Debugging (Ctrl+F5)

## Troubleshooting

### Qt Not Found

**Error:** `Could not find a package configuration file provided by "Qt6"`

**Solution:**
- Verify Qt is installed
- Check CMAKE_PREFIX_PATH is set correctly
- Ensure you're using the correct Qt version (6.x, not 5.x)

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
