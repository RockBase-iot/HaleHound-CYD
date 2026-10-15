[CmdletBinding()]
param(
    [string]$SourceRoot = "src"
)

$ErrorActionPreference = "Stop"
$violations = [System.Collections.Generic.List[string]]::new()

$pureRoots = @("core", "features", "services") | ForEach-Object {
    Join-Path $SourceRoot $_
}
$forbidden = @(
    "SPI\.begin\s*\(",
    "SPI\.end\s*\(",
    "SD\.begin\s*\(",
    "EEPROM\.begin\s*\(",
    "TFT_eSPI",
    "xTaskCreatePinnedToCore"
)

foreach ($root in $pureRoots) {
    if (-not (Test-Path $root)) {
        continue
    }
    foreach ($pattern in $forbidden) {
        $matches = Select-String -Path (Join-Path $root "*.cpp"), (Join-Path $root "*.h") -Pattern $pattern -ErrorAction SilentlyContinue
        foreach ($match in $matches) {
            $violations.Add("$($match.Path):$($match.LineNumber): forbidden boundary '$pattern'")
        }
    }
}

$pureSourceFiles = foreach ($root in $pureRoots) {
    if (Test-Path $root) {
        Get-ChildItem -LiteralPath $root -Recurse -File -Include '*.cpp', '*.h'
    }
}
$pinnedMatches = $pureSourceFiles | Select-String -Pattern "xTaskCreatePinnedToCore" -ErrorAction SilentlyContinue
foreach ($match in $pinnedMatches) {
    if ($match.Path -notlike "*src\platform\ExecutionPolicyEsp32.cpp") {
        $violations.Add("$($match.Path):$($match.LineNumber): pinned task outside ExecutionPolicyEsp32.cpp")
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output "BOUNDARY_AUDIT=PASS"
