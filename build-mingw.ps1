# MetaVisage Build Script for MinGW
# This script automates the build process for Qt MinGW builds

param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$BuildType = "Release",
    [string]$QtPath = "C:/Qt/6.10.1/mingw_64",
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

# Deploy Qt dependencies if requested
if ($Deploy) {
    Write-Host ""
    Write-Host "Deploying Qt dependencies..." -ForegroundColor Green
    Set-Location $BinDir
    & windeployqt MetaVisage.exe

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Qt dependencies deployed successfully!" -ForegroundColor Green
    } else {
        Write-Host "WARNING: windeployqt failed" -ForegroundColor Yellow
    }
}

# Return to project root
Set-Location $ProjectRoot

Write-Host ""
Write-Host "Executable location: $BinDir\MetaVisage.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run the application:" -ForegroundColor Yellow
Write-Host "  cd build\bin" -ForegroundColor Yellow
Write-Host "  .\MetaVisage.exe" -ForegroundColor Yellow
Write-Host ""
