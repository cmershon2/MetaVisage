param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$QtPath = "C:/Qt/6.10.1/msvc2022_64",
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

# Deploy Qt dependencies if requested
if ($Deploy) {
    Write-Host ""
    Write-Host "Deploying Qt dependencies..." -ForegroundColor Green

    $ExePath = Join-Path $BuildDir "bin\$BuildType\MetaVisage.exe"
    $BinDir = Split-Path -Parent $ExePath

    if (Test-Path $ExePath) {
        $WinDeployQt = Join-Path $QtPath "bin\windeployqt.exe"

        if (Test-Path $WinDeployQt) {
            Set-Location $BinDir
            & $WinDeployQt MetaVisage.exe

            if ($LASTEXITCODE -eq 0) {
                Write-Host "Qt dependencies deployed successfully!" -ForegroundColor Green
            } else {
                Write-Host "WARNING: windeployqt failed" -ForegroundColor Yellow
            }
        } else {
            Write-Host "WARNING: windeployqt not found at $WinDeployQt" -ForegroundColor Yellow
        }
    } else {
        Write-Host "WARNING: Executable not found at $ExePath" -ForegroundColor Yellow
    }
}

# Return to project root
Set-Location $ProjectRoot

Write-Host ""
Write-Host "Done! Executable location:" -ForegroundColor Cyan
Write-Host "  build\bin\$BuildType\MetaVisage.exe" -ForegroundColor White
Write-Host ""
Write-Host "To run the application:" -ForegroundColor Cyan
Write-Host "  cd build\bin\$BuildType" -ForegroundColor White
Write-Host "  .\MetaVisage.exe" -ForegroundColor White
