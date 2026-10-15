# MetaVisage Build Script for MinGW
# Sprint 3.7 -- Qt removed; this script is retained for cross-toolchain use.
# MSVC remains the authoritative build (build-msvc.ps1).

param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$BuildType = "Release",
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

# Validate before cleaning so an invalid toolchain leaves the existing build intact.
if ($VcpkgToolchain -ne "") {
    if (-not (Test-Path -LiteralPath $VcpkgToolchain -PathType Leaf)) {
        Write-Host "ERROR: vcpkg toolchain file not found: $VcpkgToolchain" -ForegroundColor Red
        Write-Host "Pass the path to an existing scripts/buildsystems/vcpkg.cmake file." -ForegroundColor Red
        exit 1
    }
    $VcpkgToolchain = (Resolve-Path -LiteralPath $VcpkgToolchain).Path.Replace('\', '/')
}

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
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_C_COMPILER=$MinGWPath/bin/gcc.exe",
    "-DCMAKE_CXX_COMPILER=$MinGWPath/bin/g++.exe",
    "-DCMAKE_MAKE_PROGRAM=$MinGWPath/bin/mingw32-make.exe"
)

# Add vcpkg toolchain if specified.
#
# Note: with the default x64-windows triplet, vcpkg ships MSVC-built .libs
# that do not link against MinGW's GCC. Sprint 3.5.2 onward requires ImGui
# symbols in the executable, so MSVC is the authoritative toolchain. See
# build-msvc.ps1. This script is retained for pre-3.5.2 targets and for any
# future cross-toolchain experimentation via custom triplets.
if ($VcpkgToolchain -ne "") {
    Write-Host "Using vcpkg toolchain: $VcpkgToolchain" -ForegroundColor Cyan
    # PowerShell passes each array element as one argument, including paths with spaces.
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgToolchain"
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

# Deploy runtime dependencies if requested. Qt was removed in Sprint 3.7; the
# executable's runtime DLLs are now staged by vcpkg's applocal post-build step.
if ($Deploy) {
    $ExePath = Join-Path $BinDir "MetaVisage.exe"
    if (Test-Path $ExePath) {
        Write-Host ""
        Write-Host "Runtime dependencies staged by vcpkg applocal." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Skipping deploy: no executable was produced." -ForegroundColor Yellow
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
