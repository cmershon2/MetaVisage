# Builds assets/icons/metavisage.ico from the Windows PNG logo set.
#
# The .ico stores each size as an embedded PNG (supported by Windows Vista
# and later, rc.exe and windres), so no image library is needed. Re-run after
# changing any Square44x44Logo.targetsize-*.png:
#
#   .\resources\build-ico.ps1

$ErrorActionPreference = 'Stop'

$sizes  = 16, 20, 24, 32, 48, 64, 256
$iconDir = Join-Path $PSScriptRoot '..\assets\icons'
$srcDir  = Join-Path $iconDir 'Windows'
$outIco  = Join-Path $iconDir 'metavisage.ico'

$images = foreach ($size in $sizes) {
    $path  = Join-Path $srcDir "Square44x44Logo.targetsize-$size.png"
    $bytes = [IO.File]::ReadAllBytes($path)
    # IHDR width/height are big-endian at byte offsets 16 and 20.
    $be32 = { param($i) ([int]$bytes[$i] -shl 24) -bor ([int]$bytes[$i + 1] -shl 16) -bor
                        ([int]$bytes[$i + 2] -shl 8) -bor [int]$bytes[$i + 3] }
    $w = & $be32 16
    $h = & $be32 20
    if ($w -ne $size -or $h -ne $size) {
        throw "$path is ${w}x${h}, expected ${size}x${size}"
    }
    [pscustomobject]@{ Size = $size; Bytes = $bytes }
}

$stream = New-Object IO.MemoryStream
$writer = New-Object IO.BinaryWriter($stream)

# ICONDIR
$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$images.Count)

# ICONDIRENTRY per image; data follows the directory.
$offset = 6 + 16 * $images.Count
foreach ($img in $images) {
    $dim = if ($img.Size -ge 256) { 0 } else { $img.Size }  # 0 means 256
    $writer.Write([byte]$dim)
    $writer.Write([byte]$dim)
    $writer.Write([byte]0)       # palette colours
    $writer.Write([byte]0)       # reserved
    $writer.Write([UInt16]1)     # colour planes
    $writer.Write([UInt16]32)    # bits per pixel
    $writer.Write([UInt32]$img.Bytes.Length)
    $writer.Write([UInt32]$offset)
    $offset += $img.Bytes.Length
}
foreach ($img in $images) { $writer.Write($img.Bytes) }

$writer.Flush()
[IO.File]::WriteAllBytes($outIco, $stream.ToArray())
Write-Host "Wrote $outIco ($($images.Count) sizes: $($sizes -join ', '))"
