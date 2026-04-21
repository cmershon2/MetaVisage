param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$QtPath = "C:/Qt/6.10.3/msvc2022_64",
    [string]$VcpkgToolchain = "",
    [string]$BuildType = "Release"
)

Write-Host "MetaVisage MSVC Build Script" -ForegroundColor Cyan
Write-Host "============================" -ForegroundColor Cyan
Write-Host ""

# Set paths
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"

# Clean build directory if requested
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Green
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

# Add CMake to PATH if not already there
$CmakePath = "C:\Program Files\CMake\bin"
if ($env:Path -notlike "*$CmakePath*") {
    $env:Path = "$CmakePath;$env:Path"
}

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Green

$CMakeArgs = @(
    ".."
    "-DCMAKE_PREFIX_PATH=`"$QtPath`""
    "-G", "Visual Studio 17 2022"
    "-A", "x64"
)

if ($VcpkgToolchain -ne "") {
    Write-Host "Using vcpkg toolchain: $VcpkgToolchain" -ForegroundColor Cyan
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=`"$VcpkgToolchain`""
}

Write-Host "Running: cmake $($CMakeArgs -join ' ')" -ForegroundColor Gray
& cmake $CMakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build
Write-Host ""
Write-Host "Building project..." -ForegroundColor Green
& cmake --build . --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green

# Deploy runtime dependencies if requested
# Sprint 3.5.1 builds MetaVisageCore as a static library only; the executable
# target returns in Story 3.5.2. Skip deployment when the .exe is absent.
if ($Deploy) {
    $ExePath = Join-Path $BuildDir "bin\$BuildType\MetaVisage.exe"
    $BinDir = Split-Path -Parent $ExePath

    if (Test-Path $ExePath) {
        Write-Host ""
        Write-Host "Deploying runtime dependencies..." -ForegroundColor Green

        # windeployqt handles Qt6Core.dll (transitional Qt6::Core dependency)
        $WinDeployQt = Join-Path $QtPath "bin\windeployqt.exe"
        if (Test-Path $WinDeployQt) {
            Set-Location $BinDir
            & $WinDeployQt MetaVisage.exe
            if ($LASTEXITCODE -ne 0) {
                Write-Host "WARNING: windeployqt failed" -ForegroundColor Yellow
            }
        } else {
            Write-Host "WARNING: windeployqt not found at $WinDeployQt" -ForegroundColor Yellow
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
$DoneExePath = Join-Path $BuildDir "bin\$BuildType\MetaVisage.exe"
if (Test-Path $DoneExePath) {
    Write-Host "Done! Executable location:" -ForegroundColor Cyan
    Write-Host "  build\bin\$BuildType\MetaVisage.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "To run the application:" -ForegroundColor Cyan
    Write-Host "  cd build\bin\$BuildType" -ForegroundColor White
    Write-Host "  .\MetaVisage.exe" -ForegroundColor White
} else {
    Write-Host "Built MetaVisageCore static library (Sprint 3.5.1 transitional target)." -ForegroundColor Cyan
    Write-Host "Executable target returns in Story 3.5.2." -ForegroundColor Cyan
}
