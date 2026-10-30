[CmdletBinding()]
param(
    [string]$Version = "",
    [ValidateSet("Release")]
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Package an already-built MSVC Release. Never remove previous release output.
$ProjectRoot = $PSScriptRoot
$ExeDir = Join-Path $ProjectRoot "build/bin/$BuildType"
$ReleaseDir = Join-Path $ProjectRoot "release"

function Require-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file is missing: $Path. Build with build-msvc.ps1 -Deploy first."
    }
}

try {
    if ([string]::IsNullOrWhiteSpace($Version)) {
        $manifestPath = Join-Path $ProjectRoot "vcpkg.json"
        Require-File $manifestPath
        $Version = (Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json).'version-string'
    }
    # A version becomes a directory/file name; disallow separators and traversal.
    if ($Version -notmatch '^\d+\.\d+\.\d+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?(?:\+[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?$') {
        throw "Version must be a filename-safe semantic version, for example 2.0.0 or 2.0.0-rc.1."
    }
    $PackageName = "MetaVisage-$Version-win-x64"
    $PackageDir = Join-Path $ReleaseDir $PackageName
    $ZipPath = Join-Path $ReleaseDir "$PackageName.zip"
    $ChecksumPath = "$ZipPath.sha256"
    foreach ($outputPath in @($PackageDir, $ZipPath, $ChecksumPath)) {
        if (Test-Path -LiteralPath $outputPath) {
            throw "Output already exists: $outputPath. Move the existing output or choose another -Version; previous packages are never deleted."
        }
    }
    if ((Test-Path -LiteralPath $ReleaseDir) -and
        ((Get-Item -LiteralPath $ReleaseDir).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "The release directory must be a regular directory inside the repository."
    }

    # Validate the build and staged assets before creating any output.
    $ExePath = Join-Path $ExeDir "MetaVisage.exe"
    Require-File $ExePath
    Require-File (Join-Path $ProjectRoot "LICENSE")
    $Dlls = @(Get-ChildItem -LiteralPath $ExeDir -Filter "*.dll" -File | Sort-Object Name)
    foreach ($pattern in @("glfw3.dll", "glew32.dll", "nfd.dll", "assimp*.dll")) {
        if (@($Dlls | Where-Object { $_.Name -like $pattern }).Count -eq 0) {
            throw "Missing staged runtime DLL: $pattern. Rebuild with the vcpkg toolchain and -Deploy."
        }
    }
    $AssetsSource = Join-Path $ExeDir "assets"
    foreach ($asset in @("models/MetaHumanHeadLod0.obj", "fonts/lucide.ttf",
                          "shaders/basic.vert", "shaders/basic.frag", "brushes/smooth_preview.png")) {
        Require-File (Join-Path $AssetsSource $asset)
    }
    # Catch incomplete staging, including fonts or shaders beyond the core set.
    $RepoAssets = Join-Path $ProjectRoot "assets"
    foreach ($asset in Get-ChildItem -LiteralPath $RepoAssets -Recurse -File) {
        $relativePath = $asset.FullName.Substring($RepoAssets.Length).TrimStart([char[]]"\/")
        Require-File (Join-Path $AssetsSource $relativePath)
    }

    # Use the configured dependency tree rather than assuming a vcpkg location.
    $CachePath = Join-Path $ProjectRoot "build/CMakeCache.txt"
    Require-File $CachePath
    $Cache = Get-Content -LiteralPath $CachePath -Raw
    $InstalledMatch = [regex]::Match($Cache, '(?m)^VCPKG_INSTALLED_DIR:PATH=(.+)$')
    $TripletMatch = [regex]::Match($Cache, '(?m)^VCPKG_TARGET_TRIPLET:STRING=(.+)$')
    if (-not $InstalledMatch.Success -or -not $TripletMatch.Success) {
        throw "The CMake cache does not identify the vcpkg installed directory and triplet. Reconfigure with build-msvc.ps1 and -VcpkgToolchain."
    }
    $ShareDir = Join-Path $InstalledMatch.Groups[1].Value.Trim() ($TripletMatch.Groups[1].Value.Trim() + "/share")
    $DependencyNotices = @(Get-ChildItem -LiteralPath $ShareDir -Directory | ForEach-Object {
        $notice = Join-Path $_.FullName "copyright"
        if (Test-Path -LiteralPath $notice -PathType Leaf) {
            [pscustomobject]@{ Name = $_.Name; Path = $notice }
        }
    })
    if ($DependencyNotices.Count -eq 0) {
        throw "No vcpkg dependency notices found under $ShareDir."
    }
    $StbNotices = @{}
    foreach ($header in @("stb_image.h", "stb_image_write.h")) {
        $headerPath = Join-Path $ProjectRoot "include/thirdparty/$header"
        Require-File $headerPath
        $text = Get-Content -LiteralPath $headerPath -Raw
        $start = $text.LastIndexOf("This software is available under 2 licenses")
        if ($start -lt 0) { throw "Could not locate the embedded license in $headerPath." }
        $StbNotices[$header] = ($text.Substring($start) -replace '\*/\s*$', '').Trim()
    }

    Write-Host "Packaging MetaVisage $Version (Windows x64)..." -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $PackageDir | Out-Null
    Copy-Item -LiteralPath $ExePath -Destination $PackageDir
    foreach ($dll in $Dlls) {
        Copy-Item -LiteralPath $dll.FullName -Destination $PackageDir
    }
    Copy-Item -LiteralPath $AssetsSource -Destination (Join-Path $PackageDir "assets") -Recurse
    Copy-Item -LiteralPath (Join-Path $ProjectRoot "LICENSE") -Destination $PackageDir

    $LicenseDir = Join-Path $PackageDir "licenses"
    New-Item -ItemType Directory -Path $LicenseDir | Out-Null
    foreach ($notice in $DependencyNotices) {
        Copy-Item -LiteralPath $notice.Path -Destination (Join-Path $LicenseDir ($notice.Name + ".txt"))
    }
    $Utf8 = [Text.UTF8Encoding]::new($false)
    foreach ($header in $StbNotices.Keys) {
        [IO.File]::WriteAllText((Join-Path $LicenseDir "$header.txt"), $StbNotices[$header], $Utf8)
    }
    $Readme = @"
MetaVisage $Version - Windows x64

Extract this entire folder, then run MetaVisage.exe. Keep the DLLs and assets
folder beside the executable. No account or service configuration is required.

Requirements:
- Windows 10/11 (64-bit).
- GPU and driver supporting OpenGL 4.3 or newer.
- Current Microsoft Visual C++ x64 Redistributable:
  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
  The redistributable installer is not included in this ZIP.

Save frequently with Ctrl+S; auto-save and crash recovery are not implemented.
Settings and logs are stored under %APPDATA%/MetaVisage.

Project and current known issues: https://github.com/cmershon2/MetaVisage
MetaVisage's license is in LICENSE. Dependency notices are in licenses/;
third-party code and assets retain their applicable terms.
"@
    [IO.File]::WriteAllText((Join-Path $PackageDir "README.txt"), $Readme, $Utf8)

    Compress-Archive -LiteralPath $PackageDir -DestinationPath $ZipPath
    $Hash = (Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText($ChecksumPath, "$Hash  $PackageName.zip`n", $Utf8)
    Write-Host "Package:  $PackageDir" -ForegroundColor Green
    Write-Host "Archive:  $ZipPath" -ForegroundColor Green
    Write-Host "Checksum: $ChecksumPath" -ForegroundColor Green
    Write-Host "Includes $($Dlls.Count) runtime DLLs and $($DependencyNotices.Count + $StbNotices.Count) dependency notices."
    Write-Host "Test the extracted ZIP on a clean Windows machine before publishing."
} catch {
    Write-Error "Packaging failed: $($_.Exception.Message)" -ErrorAction Continue
    exit 1
}
