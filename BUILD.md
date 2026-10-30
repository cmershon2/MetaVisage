# Building MetaVisage

MetaVisage is a standalone Windows C++20 application. No account, API keys,
environment profiles, or online license service is required to build or run it.
Dependency downloads require network access during the initial vcpkg setup.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++ and a Windows SDK
- CMake 3.16 or newer, Git, and vcpkg
- An OpenGL 4.3 capable GPU for the application and UI smoke test

The vcpkg manifest installs GLFW, Dear ImGui (including the test engine), GLEW,
Eigen, Assimp, nlohmann/json, and Native File Dialog Extended. Qt is not used.

## First-time setup

Install the prerequisites above, then run in PowerShell. This example places
vcpkg at `C:/vcpkg`; use another location if preferred:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
& C:/vcpkg/bootstrap-vcpkg.bat
git clone --branch development https://github.com/cmershon2/MetaVisage.git
cd MetaVisage
```

If you already have either repository, use that checkout. CMake installs the
manifest dependencies when configured with the vcpkg toolchain. No individual
`vcpkg install assimp` step is needed. The manifest lists dependencies but does
not currently pin a vcpkg baseline; dependency versions depend on your checkout.

## Build

Run from the repository root in PowerShell:

```powershell
.\build-msvc.ps1 -Deploy -VcpkgToolchain "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

The executable is `build/bin/Release/MetaVisage.exe`. Close any running instance
before relinking; otherwise MSVC may fail with LNK1104. The helper preserves
vcpkg's runtime DLL deployment. Use `-BuildType Debug` for a debug build.
Pass `-VcpkgToolchain` explicitly on the first helper build; the helpers do not
translate `VCPKG_ROOT` into a toolchain argument automatically.

Launch from the output directory so staged assets can be found:

```powershell
Push-Location build/bin/Release
.\MetaVisage.exe
Pop-Location
```

Keep the executable, runtime DLLs, and staged `assets/` directory together when
copying a build. Clean-machine release package validation remains outstanding.

For manual CMake configuration:

```powershell
$env:VCPKG_ROOT = "C:/vcpkg"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

## MinGW

MinGW remains available through `build-mingw.ps1`, but needs a compatible
MinGW compiler and vcpkg triplet. The default MSVC-built x64-windows libraries
cannot be linked with MinGW. MSVC is the validated toolchain.
The helper accepts `-MinGWPath`, `-VcpkgToolchain`, `-BuildType`, `-Clean`, and
`-Deploy`; it does not accept `-QtPath`. Its `-Deploy` switch reports vcpkg
runtime staging. It does not run `windeployqt`. Standalone MinGW validation is
still pending; do not reuse an MSVC-configured build directory for MinGW.

## App icon

`MetaVisage.exe` embeds `assets/icons/metavisage.ico` through
`resources/metavisage.rc.in`. The resource is named `GLFW_ICON`, so GLFW also
uses it for the window and taskbar. After changing the
`assets/icons/Windows/Square44x44Logo.targetsize-*.png` files, regenerate the
icon with `.\resources\build-ico.ps1`. MinGW builds compile the resource in a
separate `windres` step because windres cannot handle include paths that
contain spaces.

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -L ui --output-on-failure
```

The four smoke targets cover mesh round-trip/export, v1 project compatibility,
wrap parity, and UI startup/navigation/shading/brush selection. The UI lane opens
a real GLFW window. Wrap parity can take about 70 seconds (120-second timeout).

## Troubleshooting

| Symptom | Check |
|---------|-------|
| Missing GLFW, ImGui, GLEW, Eigen, or NFD package | Configure with the vcpkg toolchain and allow manifest installation to finish. |
| Invalid toolchain path | Pass the existing `scripts/buildsystems/vcpkg.cmake` file from your vcpkg checkout. |
| Generator or compiler mismatch | Use a fresh build directory when switching toolchains; the helpers' `-Clean` option deletes the existing `build/` directory. |
| `LNK1104` for `MetaVisage.exe` | Close the running application and build again. |
| Missing DLL or shader at launch | Run from the output directory with its staged DLLs and `assets/` directory intact. |
| UI smoke cannot create a window | Check OpenGL 4.3 driver support and run in an interactive Windows desktop session. |

## Local data

Settings, recent projects, and inspector preferences live under
`%APPDATA%/MetaVisage`. Legacy account fields are ignored and omitted when
settings are saved. The standalone application does not register browser URL
handlers, contact a web service, or report exports.

## Package a release

After the Release build and tests pass, run from the repository root:

```powershell
.\package-release.ps1
```

The default version comes from `vcpkg.json` (currently `2.0.0`). Use
`-Version 2.0.0-rc.1` to label a release candidate. Packaging uses the existing
`build/bin/Release` output; it does not rebuild, rerun tests, create a Git tag,
or upload a release.

Outputs are under the ignored `release/` directory:

- `MetaVisage-2.0.0-win-x64/`: executable, staged runtime DLLs and assets,
  run instructions, project license, and dependency notices.
- `MetaVisage-2.0.0-win-x64.zip`: upload this as the Windows binary asset.
- `MetaVisage-2.0.0-win-x64.zip.sha256`: upload alongside the ZIP for verification.

The script requires staged assets and DLLs and copies notices from the vcpkg
installation identified by `build/CMakeCache.txt`, plus embedded stb licenses.
It excludes test executables, fixtures, logs, and developer configuration. It
never deletes prior packages or overwrites an existing output; move an existing
version's outputs aside or choose another version before rerunning.

Before publishing, extract the ZIP on a clean Windows machine and verify launch,
mesh loading, wrapping, project save/reopen, and FBX export. The package requires
the Microsoft Visual C++ x64 Redistributable; its installer is not bundled.
Bundled head/font/icon provenance and notices still need verification as recorded
in the [documentation review](.agents/context/reviews/open-source-documentation.md).
Collecting dependency notices does not resolve those asset-specific items.
