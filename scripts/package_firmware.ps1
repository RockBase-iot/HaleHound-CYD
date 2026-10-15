[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("esp32-cyd", "nm-cyd-c5")]
    [string]$Environment,

    [string]$BuildDirectory,
    [string]$OutputRoot = "dist",
    [string]$BootApp0Path,
    [string]$PlatformioCoreDirectory
)

$ErrorActionPreference = "Stop"
$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDirectory = Split-Path -Parent $scriptDirectory

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $projectDirectory ".pio/build/$Environment"
}
if ([string]::IsNullOrWhiteSpace($PlatformioCoreDirectory)) {
    $coreCandidates = @()
    if (-not [string]::IsNullOrWhiteSpace($env:PLATFORMIO_CORE_DIR)) {
        $coreCandidates += $env:PLATFORMIO_CORE_DIR
    }
    $pioCommand = Get-Command pio -ErrorAction SilentlyContinue
    if ($pioCommand) {
        $coreCandidates += Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $pioCommand.Source))
    }
    if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE)) {
        $coreCandidates += Join-Path $env:USERPROFILE ".platformio"
    }
    $coreCandidates += Join-Path ([Environment]::GetFolderPath("UserProfile")) ".platformio"
    $PlatformioCoreDirectory = $coreCandidates | Where-Object {
        Test-Path -LiteralPath (Join-Path $_ "penv/Scripts/python.exe") -PathType Leaf
    } | Select-Object -First 1
}
if ([string]::IsNullOrWhiteSpace($PlatformioCoreDirectory)) {
    throw "PlatformIO core directory not found; pass -PlatformioCoreDirectory explicitly"
}

$targets = @{
    "esp32-cyd" = @{
        Chip = "esp32"
        BootloaderOffset = "0x1000"
    }
    "nm-cyd-c5" = @{
        Chip = "esp32c5"
        BootloaderOffset = "0x2000"
    }
}
$target = $targets[$Environment]

$buildDirectoryPath = (Resolve-Path -LiteralPath $BuildDirectory).Path
$sourceFiles = [ordered]@{
    "bootloader.bin" = Join-Path $buildDirectoryPath "bootloader.bin"
    "partitions.bin" = Join-Path $buildDirectoryPath "partitions.bin"
    "firmware.bin"   = Join-Path $buildDirectoryPath "firmware.bin"
}
foreach ($sourceFile in $sourceFiles.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $sourceFile.Value -PathType Leaf)) {
        throw "missing build artifact: $($sourceFile.Value)"
    }
}

& (Join-Path $scriptDirectory "verify_build_freshness.ps1") `
    -FirmwarePath $sourceFiles["firmware.bin"] `
    -InputPaths @(
        (Join-Path $projectDirectory "src"),
        (Join-Path $projectDirectory "include"),
        (Join-Path $projectDirectory "boards"),
        (Join-Path $projectDirectory "platformio.ini")
    )

if ([string]::IsNullOrWhiteSpace($BootApp0Path)) {
    $bootAppCandidates = @(
        (Join-Path $PlatformioCoreDirectory "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"),
        (Join-Path $projectDirectory ".pio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin")
    )
    $BootApp0Path = $bootAppCandidates | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    } | Select-Object -First 1
}
if ([string]::IsNullOrWhiteSpace($BootApp0Path) -or
    -not (Test-Path -LiteralPath $BootApp0Path -PathType Leaf)) {
    throw "boot_app0.bin not found; pass -BootApp0Path explicitly"
}
$BootApp0Path = (Resolve-Path -LiteralPath $BootApp0Path).Path

$segmentDirectory = Join-Path $OutputRoot $Environment
New-Item -ItemType Directory -Path $segmentDirectory -Force | Out-Null
foreach ($sourceFile in $sourceFiles.GetEnumerator()) {
    Copy-Item -LiteralPath $sourceFile.Value `
        -Destination (Join-Path $segmentDirectory $sourceFile.Key) -Force
}
Copy-Item -LiteralPath $BootApp0Path `
    -Destination (Join-Path $segmentDirectory "boot_app0.bin") -Force

$pythonCandidates = @(
    (Join-Path $PlatformioCoreDirectory "penv/Scripts/python.exe"),
    (Join-Path $env:USERPROFILE ".platformio/penv/Scripts/python.exe")
)
$esptoolCandidates = @(
    (Join-Path $PlatformioCoreDirectory "packages/tool-esptoolpy/esptool.py"),
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-esptoolpy/esptool.py")
)
$python = $pythonCandidates | Where-Object {
    Test-Path -LiteralPath $_ -PathType Leaf
} | Select-Object -First 1
$esptool = $esptoolCandidates | Where-Object {
    Test-Path -LiteralPath $_ -PathType Leaf
} | Select-Object -First 1
if (-not $python -or -not $esptool) {
    throw "PlatformIO Python or esptool.py not found under $PlatformioCoreDirectory"
}

$fullImage = Join-Path $OutputRoot "HaleHound-CYD-$Environment-FULL.bin"
if (Test-Path -LiteralPath $fullImage) {
    Remove-Item -LiteralPath $fullImage -Force
}

& $python $esptool --chip $target.Chip merge_bin `
    -o $fullImage `
    $target.BootloaderOffset (Join-Path $segmentDirectory "bootloader.bin") `
    0x8000 (Join-Path $segmentDirectory "partitions.bin") `
    0xe000 (Join-Path $segmentDirectory "boot_app0.bin") `
    0x10000 (Join-Path $segmentDirectory "firmware.bin")
if ($LASTEXITCODE -ne 0) {
    throw "esptool merge_bin failed with exit code $LASTEXITCODE"
}

& (Join-Path $scriptDirectory "verify_flash_layout.ps1") `
    -Environment $Environment `
    -ImagePath $fullImage `
    -SegmentsDirectory $segmentDirectory

$hashLines = Get-ChildItem -LiteralPath $segmentDirectory -Filter "*.bin" |
    Sort-Object Name |
    ForEach-Object {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "$hash  $($_.Name)"
    }
$fullHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fullImage).Hash.ToLowerInvariant()
$hashLines += "$fullHash  $([System.IO.Path]::GetFileName($fullImage))"
$hashLines | Set-Content -Encoding ascii -LiteralPath (Join-Path $segmentDirectory "SHA256SUMS.txt")

Write-Host "PACKAGE_FIRMWARE=PASS environment=$Environment full=$fullImage"
