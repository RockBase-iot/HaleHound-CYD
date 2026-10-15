[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("esp32-cyd", "nm-cyd-c5")]
    [string]$Environment,

    [Parameter(Mandatory = $true)]
    [string]$ImagePath,

    [Parameter(Mandatory = $true)]
    [string]$SegmentsDirectory
)

$ErrorActionPreference = "Stop"

$layouts = @{
    "esp32-cyd" = [ordered]@{
        "bootloader.bin" = 0x1000
        "partitions.bin" = 0x8000
        "boot_app0.bin"  = 0xe000
        "firmware.bin"   = 0x10000
    }
    "nm-cyd-c5" = [ordered]@{
        "bootloader.bin" = 0x2000
        "partitions.bin" = 0x8000
        "boot_app0.bin"  = 0xe000
        "firmware.bin"   = 0x10000
    }
}

function Assert-SegmentMatches {
    param(
        [System.IO.FileStream]$Image,
        [string]$SegmentPath,
        [int64]$Offset
    )

    $segment = [System.IO.File]::OpenRead($SegmentPath)
    try {
        if (($Offset + $segment.Length) -gt $Image.Length) {
            throw "segment exceeds image: $([System.IO.Path]::GetFileName($SegmentPath)) at 0x$($Offset.ToString('x'))"
        }

        $Image.Position = $Offset
        $imageBuffer = New-Object byte[] 4096
        $segmentBuffer = New-Object byte[] 4096
        $position = 0L
        while ($position -lt $segment.Length) {
            $remaining = $segment.Length - $position
            $count = [int][Math]::Min($imageBuffer.Length, $remaining)
            $imageRead = $Image.Read($imageBuffer, 0, $count)
            $segmentRead = $segment.Read($segmentBuffer, 0, $count)
            if ($imageRead -ne $count -or $segmentRead -ne $count) {
                throw "short read while checking $([System.IO.Path]::GetFileName($SegmentPath))"
            }

            for ($index = 0; $index -lt $count; $index++) {
                if ($imageBuffer[$index] -ne $segmentBuffer[$index]) {
                    $absoluteOffset = $Offset + $position + $index
                    throw "segment mismatch: $([System.IO.Path]::GetFileName($SegmentPath)) at 0x$($absoluteOffset.ToString('x'))"
                }
            }
            $position += $count
        }
    } finally {
        $segment.Dispose()
    }
}

$resolvedImage = (Resolve-Path -LiteralPath $ImagePath).Path
$resolvedSegments = (Resolve-Path -LiteralPath $SegmentsDirectory).Path
$image = [System.IO.File]::OpenRead($resolvedImage)
try {
    foreach ($entry in $layouts[$Environment].GetEnumerator()) {
        $segmentPath = Join-Path $resolvedSegments $entry.Key
        if (-not (Test-Path -LiteralPath $segmentPath -PathType Leaf)) {
            throw "missing segment: $segmentPath"
        }
        Assert-SegmentMatches -Image $image -SegmentPath $segmentPath -Offset $entry.Value
    }
} finally {
    $image.Dispose()
}

Write-Host "FLASH_LAYOUT=PASS environment=$Environment image=$resolvedImage"
