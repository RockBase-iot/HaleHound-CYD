[CmdletBinding()]
param(
    [string]$OutputDirectory = "build-reports"
)

$ErrorActionPreference = "Stop"
$environments = @(
    "esp32-cyd",
    "esp32-cyd-hat",
    "esp32-e32r28t",
    "esp32-e32r35t",
    "nm-cyd-c5"
)

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$results = foreach ($environment in $environments) {
    Write-Host "Building $environment..."
    $coreGroup = if ($environment -eq "nm-cyd-c5") { "c5" } else { "classic" }
    $previousCoreDirectory = $env:PLATFORMIO_CORE_DIR
    $env:PLATFORMIO_CORE_DIR = Join-Path $PWD ".pio/matrix/$coreGroup"
    $stdoutPath = Join-Path $OutputDirectory "$environment.stdout.log"
    $stderrPath = Join-Path $OutputDirectory "$environment.stderr.log"
    try {
        $process = Start-Process -FilePath (Get-Command pio).Source `
            -ArgumentList @("run", "-e", $environment) `
            -NoNewWindow -Wait -PassThru `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath
    } finally {
        $env:PLATFORMIO_CORE_DIR = $previousCoreDirectory
    }
    $output = (Get-Content -Raw $stdoutPath) + (Get-Content -Raw $stderrPath)
    if ($process.ExitCode -ne 0) {
        Write-Error $output
    }

    $firmware = Join-Path ".pio/build/$environment" "firmware.bin"
    $ramMatch = [regex]::Match($output, "RAM:\s+\[[^\]]+\]\s+([\d.]+)% \(used (\d+) bytes from (\d+) bytes\)")
    $flashMatch = [regex]::Match($output, "Flash:\s+\[[^\]]+\]\s+([\d.]+)% \(used (\d+) bytes from (\d+) bytes\)")
    $hash = (Get-FileHash -Algorithm SHA256 -Path $firmware).Hash.ToLowerInvariant()

    [pscustomobject]@{
        environment = $environment
        ramUsed = [int64]$ramMatch.Groups[2].Value
        ramTotal = [int64]$ramMatch.Groups[3].Value
        flashUsed = [int64]$flashMatch.Groups[2].Value
        flashTotal = [int64]$flashMatch.Groups[3].Value
        firmwareSha256 = $hash
    }
}

$reportPath = Join-Path $OutputDirectory "build-matrix.json"
$results | ConvertTo-Json | Set-Content -Encoding ascii -Path $reportPath
$results | Format-Table environment, ramUsed, ramTotal, flashUsed, flashTotal, firmwareSha256
Write-Host "Build report: $reportPath"