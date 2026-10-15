[CmdletBinding()]
param(
    [string]$ProjectFile = "platformio.ini",
    [string]$BuildMatrix = "scripts/build_matrix.ps1"
)

$ErrorActionPreference = "Stop"
$expected = @("esp32-cyd", "nm-cyd-c5")
$iniTargets = Select-String -Path $ProjectFile -Pattern '^\[env:([^\]]+)\]$' |
    ForEach-Object { $_.Matches[0].Groups[1].Value } |
    Where-Object { $_ -ne "native" }
$matrixText = Get-Content -Raw -LiteralPath $BuildMatrix
$unexpected = @($iniTargets | Where-Object { $_ -notin $expected })
$missing = @($expected | Where-Object {
    $_ -notin $iniTargets -or -not $matrixText.Contains('"' + $_ + '"')
})

if ($unexpected.Count -gt 0 -or $missing.Count -gt 0) {
    throw "Target contract failed. Unexpected=$($unexpected -join ',') Missing=$($missing -join ',')"
}

Write-Output "TARGET_CONTRACT=PASS"
