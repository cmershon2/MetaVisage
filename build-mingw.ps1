# MetaVisage Build Script for MinGW
# This script automates the build process for Qt MinGW builds

param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$BuildType = "Release",
    [string]$QtPath = "C:/Qt/6.10.3/mingw_64",
    [string]$MinGWPath = "C:/Qt/Tools/mingw1310_64",
    [string]$VcpkgToolchain = ""
)

Write-Host "MetaVisage Build Script" -ForegroundColor Cyan
Write-Host "========================" -ForegroundColor Cyan
Write-Host ""

# Set up paths
$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$BinDir = Join-Path $BuildDir "bin"

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Green
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Set up environment
Write-Host "Setting up MinGW environment..." -ForegroundColor Green
$env:Path = "$MinGWPath\bin;" + $env:Path

# Check if MinGW is available
try {
    $null = & mingw32-make --version
    Write-Host "MinGW found: $MinGWPath" -ForegroundColor Green
} catch {
    Write-Host "ERROR: MinGW not found at $MinGWPath" -ForegroundColor Red
    Write-Host "Please check your MinGW installation path" -ForegroundColor Red
    exit 1
}

# Build CMake command
Write-Host "Configuring CMake..." -ForegroundColor Green
Set-Location $BuildDir

$CMakeArgs = @(
    "..",
    "-DCMAKE_PREFIX_PATH=`"$QtPath`"",
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_C_COMPILER=`"$MinGWPath/bin/gcc.exe`"",
    "-DCMAKE_CXX_COMPILER=`"$MinGWPath/bin/g++.exe`"",
    "-DCMAKE_MAKE_PROGRAM=`"$MinGWPath/bin/mingw32-make.exe`""
)

# Add vcpkg toolchain if specified
if ($VcpkgToolchain -ne "") {
    Write-Host "Using vcpkg toolchain: $VcpkgToolchain" -ForegroundColor Cyan
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=`"$VcpkgToolchain`""
}

# Run CMake
Write-Host "Running: cmake $($CMakeArgs -join ' ')" -ForegroundColor Gray
& cmake $CMakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

# Build
Write-Host ""
Write-Host "Building project..." -ForegroundColor Green
& mingw32-make

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green

# Deploy runtime dependencies if requested
# Sprint 3.5.1 builds MetaVisageCore as a static library only; the executable
# target returns in Story 3.5.2. Skip deployment when the .exe is absent.
if ($Deploy) {
    $ExePath = Join-Path $BinDir "MetaVisage.exe"
    if (Test-Path $ExePath) {
        Write-Host ""
        Write-Host "Deploying runtime dependencies..." -ForegroundColor Green
        Set-Location $BinDir

        # windeployqt still handles Qt6Core.dll (transitional Qt6::Core dependency)
        & windeployqt MetaVisage.exe
        if ($LASTEXITCODE -ne 0) {
            Write-Host "WARNING: windeployqt failed" -ForegroundColor Yellow
        }

        # vcpkg applocal deployment stages GLFW / GLEW / ImGui / libcurl / etc. DLLs
        # This runs automatically as a post-build step under the vcpkg toolchain.
        Write-Host "Runtime dependencies deployed." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Skipping deploy: no executable yet (Sprint 3.5.1 builds a static library only)." -ForegroundColor Yellow
    }
}

# Return to project root
Set-Location $ProjectRoot

Write-Host ""
$ExePath = Join-Path $BinDir "MetaVisage.exe"
if (Test-Path $ExePath) {
    Write-Host "Executable location: $ExePath" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To run the application:" -ForegroundColor Yellow
    Write-Host "  cd build\bin" -ForegroundColor Yellow
    Write-Host "  .\MetaVisage.exe" -ForegroundColor Yellow
} else {
    Write-Host "Built MetaVisageCore static library (Sprint 3.5.1 transitional target)." -ForegroundColor Cyan
    Write-Host "Executable target returns in Story 3.5.2." -ForegroundColor Cyan
}
Write-Host ""
