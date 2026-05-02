param(
    [switch]$Clean,
    [switch]$Deploy,
    [string]$VcpkgToolchain = "",
    [string]$BuildType = "Release",
    [ValidateSet("development", "staging", "production")]
    [string]$Profile = "development"
)

Write-Host "MetaVisage MSVC Build Script" -ForegroundColor Cyan
Write-Host "============================" -ForegroundColor Cyan
Write-Host ""

# Set paths
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"

# ---------------------------------------------------------------------------
# Build profile + .env.<profile> resolution (Sprint 6 Story 6.1)
# ---------------------------------------------------------------------------
# Read .env.<profile> for licensing config (API URL, Supabase project URL,
# anon key, public-key file). Each KEY=VALUE pair becomes a -D flag on the
# CMake configure step. Lines starting with `#` and blank lines are skipped;
# surrounding single or double quotes are stripped.
function Read-EnvFile {
    param([Parameter(Mandatory)] [string]$Path)
    $values = @{}
    if (-not (Test-Path $Path)) { return $values }
    foreach ($rawLine in Get-Content -LiteralPath $Path) {
        $line = $rawLine.Trim()
        if ($line -eq "" -or $line.StartsWith("#")) { continue }
        $idx = $line.IndexOf("=")
        if ($idx -lt 1) { continue }
        $key = $line.Substring(0, $idx).Trim()
        $value = $line.Substring($idx + 1).Trim()
        if ($value.Length -ge 2) {
            $first = $value[0]
            $last  = $value[$value.Length - 1]
            if (($first -eq '"' -and $last -eq '"') -or
                ($first -eq "'" -and $last -eq "'")) {
                $value = $value.Substring(1, $value.Length - 2)
            }
        }
        $values[$key] = $value
    }
    return $values
}

$EnvFile = Join-Path $ProjectRoot ".env.$Profile"
Write-Host "Build profile: $Profile" -ForegroundColor Cyan
if (Test-Path $EnvFile) {
    Write-Host "Reading config: $EnvFile" -ForegroundColor Gray
    $EnvValues = Read-EnvFile -Path $EnvFile
} else {
    if ($Profile -eq "development") {
        Write-Host "No .env.development found -- falling back to CMake defaults (localhost:3000)." -ForegroundColor Yellow
        Write-Host "Copy .env.example to .env.development to customize." -ForegroundColor Yellow
        $EnvValues = @{}
    } else {
        Write-Host "ERROR: .env.$Profile not found at $EnvFile" -ForegroundColor Red
        Write-Host "Copy .env.example to .env.$Profile and populate it from the web team's 1Password drop." -ForegroundColor Red
        exit 1
    }
}

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
    "-G", "Visual Studio 17 2022"
    "-A", "x64"
    "-DMETAVISAGE_BUILD_PROFILE=$Profile"
)

# Forward each .env.<profile> key as a -D flag. Cache var names match the
# CMake side exactly (METAVISAGE_API_BASE_URL, METAVISAGE_SUPABASE_*, etc.)
foreach ($key in $EnvValues.Keys) {
    $CMakeArgs += "-D$key=$($EnvValues[$key])"
}

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

# Deploy runtime dependencies if requested. Qt was removed in Sprint 3.7,
# so the only DLLs the .exe needs are the GLFW / GLEW / ImGui / libcurl /
# Eigen / Assimp set staged by vcpkg's applocal post-build step (which runs
# automatically when configuring with -DCMAKE_TOOLCHAIN_FILE=<vcpkg.cmake>).
if ($Deploy) {
    $ExePath = Join-Path $BuildDir "bin\$BuildType\MetaVisage.exe"

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
