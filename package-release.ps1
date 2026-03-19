param(
    [string]$QtPath = "C:/Qt/6.10.1/msvc2022_64",
    [string]$BuildType = "Release",
    [string]$Version = "1.0.0"
)

Write-Host "MetaVisage Release Packaging Script" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor White
Write-Host ""

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$ExeDir = Join-Path $BuildDir "bin\$BuildType"
$PackageDir = Join-Path $ProjectRoot "release\MetaVisage-$Version"
$ZipPath = Join-Path $ProjectRoot "release\MetaVisage-$Version-win64.zip"

# Verify build exists
$ExePath = Join-Path $ExeDir "MetaVisage.exe"
if (-not (Test-Path $ExePath)) {
    Write-Host "ERROR: MetaVisage.exe not found at $ExePath" -ForegroundColor Red
    Write-Host "Run build-msvc.ps1 first to build the project." -ForegroundColor Yellow
    exit 1
}

# Clean previous package
if (Test-Path (Join-Path $ProjectRoot "release")) {
    Write-Host "Cleaning previous release directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force (Join-Path $ProjectRoot "release")
}

# Create package directory
Write-Host "Creating package directory..." -ForegroundColor Green
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

# Copy executable
Write-Host "Copying executable..." -ForegroundColor Green
Copy-Item $ExePath $PackageDir

# Deploy Qt dependencies
Write-Host "Deploying Qt dependencies..." -ForegroundColor Green
$WinDeployQt = Join-Path $QtPath "bin\windeployqt.exe"
if (Test-Path $WinDeployQt) {
    Push-Location $PackageDir
    & $WinDeployQt MetaVisage.exe --no-translations --no-system-d3d-compiler --no-compiler-runtime 2>&1 | Out-Null
    Pop-Location

    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: windeployqt returned non-zero exit code" -ForegroundColor Yellow
    }
} else {
    Write-Host "WARNING: windeployqt not found at $WinDeployQt" -ForegroundColor Yellow
    Write-Host "Qt DLLs will need to be copied manually." -ForegroundColor Yellow
}

# Copy Assimp DLL if present
$AssimpDll = Join-Path $ExeDir "assimp-vc*.dll"
$AssimpDlls = Get-ChildItem -Path $ExeDir -Filter "assimp*.dll" -ErrorAction SilentlyContinue
foreach ($dll in $AssimpDlls) {
    Write-Host "Copying $($dll.Name)..." -ForegroundColor Green
    Copy-Item $dll.FullName $PackageDir
}

# Copy assets
Write-Host "Copying assets..." -ForegroundColor Green
$AssetsSource = Join-Path $ExeDir "assets"
if (-not (Test-Path $AssetsSource)) {
    $AssetsSource = Join-Path $ProjectRoot "assets"
}
if (Test-Path $AssetsSource) {
    Copy-Item -Recurse $AssetsSource (Join-Path $PackageDir "assets")
} else {
    Write-Host "WARNING: Assets directory not found" -ForegroundColor Yellow
}

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Green
Copy-Item (Join-Path $ProjectRoot "README.md") $PackageDir
Copy-Item (Join-Path $ProjectRoot "LICENSE") $PackageDir

# Create zip archive
Write-Host "Creating zip archive..." -ForegroundColor Green
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath
}
Compress-Archive -Path $PackageDir -DestinationPath $ZipPath

# Summary
Write-Host ""
Write-Host "Package created successfully!" -ForegroundColor Green
Write-Host "  Directory: $PackageDir" -ForegroundColor White
Write-Host "  Archive:   $ZipPath" -ForegroundColor White

# List package contents
Write-Host ""
Write-Host "Package contents:" -ForegroundColor Cyan
$files = Get-ChildItem -Recurse $PackageDir | Where-Object { -not $_.PSIsContainer }
$totalSize = ($files | Measure-Object -Property Length -Sum).Sum
Write-Host "  Files: $($files.Count)" -ForegroundColor White
Write-Host "  Total size: $([math]::Round($totalSize / 1MB, 2)) MB" -ForegroundColor White
