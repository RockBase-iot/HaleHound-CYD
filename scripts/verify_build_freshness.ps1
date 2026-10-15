[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$FirmwarePath,

    [string[]]$InputPaths = @("src", "include", "boards", "platformio.ini")
)

$ErrorActionPreference = "Stop"

$firmware = Get-Item -LiteralPath $FirmwarePath
$inputs = foreach ($inputPath in $InputPaths) {
    $item = Get-Item -LiteralPath $inputPath
    if ($item.PSIsContainer) {
        Get-ChildItem -LiteralPath $item.FullName -Recurse -File
    } else {
        $item
    }
}

$newestInput = $inputs | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if ($null -eq $newestInput) {
    throw "no build inputs found"
}

if ($firmware.LastWriteTimeUtc -lt $newestInput.LastWriteTimeUtc) {
    throw "stale firmware: $($firmware.FullName) is older than $($newestInput.FullName)"
}

Write-Host "BUILD_FRESHNESS=PASS firmware=$($firmware.FullName)"
