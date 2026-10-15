[CmdletBinding()]
param(
    [string]$PlatformioFile = "platformio.ini",
    [string]$UserSetupFile = "include/User_Setup.h",
    [string]$TouchFile = "src/legacy/touch_buttons.cpp",
    [string]$UtilsFile = "src/legacy/utils.cpp"
)

$ErrorActionPreference = "Stop"

$platformio = Get-Content -Raw -LiteralPath $PlatformioFile
$userSetup = Get-Content -Raw -LiteralPath $UserSetupFile
$touch = Get-Content -Raw -LiteralPath $TouchFile
$utils = Get-Content -Raw -LiteralPath $UtilsFile
$failures = [System.Collections.Generic.List[string]]::new()

if ($platformio -match '(?m)^\s*-DST7789_DRIVER(?:=1)?\s*$') {
    $failures.Add("ST7789_DRIVER is duplicated in platformio.ini and User_Setup.h")
}

$requiredTouchDefines = @(
    '#define CYD28_TouchR_IRQ\s+-1',
    '#define CYD28_TouchR_MISO\s+TFT_MISO',
    '#define CYD28_TouchR_MOSI\s+TFT_MOSI',
    '#define CYD28_TouchR_CLK\s+TFT_SCLK',
    '#define CYD28_TouchR_CS\s+(?:1|TOUCH_CS)'
)
foreach ($pattern in $requiredTouchDefines) {
    if ($userSetup -notmatch $pattern) {
        $failures.Add("missing C5 touch definition: $pattern")
    }
}

if ($touch -notmatch '(?s)#ifdef CYD_C5.*?touch\.begin\(&SPI\);.*?#else.*?touch\.begin\(\);.*?#endif') {
    $failures.Add("C5 touch must use touch.begin(&SPI), while ESP32-CYD keeps software SPI")
}

if ($utils -notmatch '(?s)void applyColorOrder\(\)\s*\{\s*#if defined\(CYD_C5\).*?return;.*?#endif') {
    $failures.Add("C5 must not overwrite TFT_eSPI ST7789 MADCTL rotation data")
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Output "C5_DISPLAY_TOUCH_VIOLATION=$_" }
    throw "C5 display/touch contract failed with $($failures.Count) violation(s)"
}

Write-Output "C5_DISPLAY_TOUCH_CONTRACT=PASS"
