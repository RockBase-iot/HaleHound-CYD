# Menu Orientation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make all legacy application menus fully visible and touchable in portrait and landscape on ESP32-2432S028 and NM-CYD-C5.

**Architecture:** Add a pure `MenuLayout` value object that calculates runtime item rectangles from the active display dimensions. Make menu rendering and touch handlers consume the same rectangles, while preserving existing feature dispatch and security policy behavior.

**Tech Stack:** C++17, Arduino/TFT_eSPI, PlatformIO native Unity tests, ESP32 Arduino builds.

---

### Task 1: Pure Runtime Menu Layout

**Files:**
- Create: `include/services/MenuLayout.h`
- Create: `src/services/pure/MenuLayout.cpp`
- Create: `test/native/test_menu_layout/test_main.cpp`

- [x] **Step 1: Write the failing layout tests**

Define tests that construct main, submenu, and rotation layouts at 240x320 and
320x240. Assert expected column/row counts, every rectangle is within its
content bounds, rectangles do not overlap, and each rectangle center maps back
to its index through `hitTest()`.

- [x] **Step 2: Run the focused test to verify RED**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f native/test_menu_layout
```

Expected: compilation fails because `services/MenuLayout.h` does not exist. If
the host native compiler is unavailable, record that infrastructure blocker and
continue with target compilation as the executable verification path.

- [x] **Step 3: Implement the pure layout object**

Expose `MenuRect`, `MenuLayoutKind`, and `MenuLayout`. Required constructors are
factory functions for main menu, submenu, and rotation options. Use
column-major index mapping and half-open rectangle containment.

- [x] **Step 4: Run the focused test to verify GREEN**

Run the focused PlatformIO native command again. Expected: all layout tests
pass, or the previously recorded missing-host-compiler blocker repeats before
test compilation.

### Task 2: Main Menu Runtime Integration

**Files:**
- Modify: `src/app/LegacyApp.inc`

- [x] **Step 1: Add shared menu drawing helpers**

Include `services/MenuLayout.h`. Add helpers that create layouts from
`tft.width()`/`tft.height()`, draw one main-menu item, draw one submenu item,
and return a touched item by applying `isTouchInArea()` to layout rectangles.

- [x] **Step 2: Replace main-menu coordinate formulas**

Use `MenuLayout::mainMenu()` in initial rendering, highlight restoration, and
touch handling. Calculate the watermark, mode banner, and PIN icon from runtime
dimensions. Keep safe-mode and policy branches unchanged.

- [x] **Step 3: Compile the classic ESP32 target**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd -j 4
```

Expected: successful firmware link with no menu-layout compile errors.

### Task 3: Submenus and Rotation Screen

**Files:**
- Modify: `src/app/LegacyApp.inc`

- [x] **Step 1: Replace submenu drawing formulas**

Use `MenuLayout::submenu()` for initialization and selection redraw. The same
helper must cover WiFi, Bluetooth, nRF24, SubGHz, RFID, Jam Detect, SIGINT,
Tools, Settings, About, and the nested AirTag menu.

- [x] **Step 2: Replace every submenu touch formula**

In each handler, use the shared touched-submenu helper. Preserve all existing
selected-index switch statements, policy guards, lifecycle calls, and return
paths.

- [x] **Step 3: Adapt the rotation chooser**

Use `MenuLayout::rotationOptions()` for drawing and hit testing. Recreate the
layout after `setRotation()` so landscape immediately uses a two-by-two grid.

- [x] **Step 4: Compile both board targets**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd -j 4
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e nm-cyd-c5 -j 1
```

Expected: both environments compile and link successfully.

### Task 4: Final Verification and Records

**Files:**
- Modify: `task_plan.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [x] **Step 1: Run layout and repository checks**

Run the focused native test, `scripts/verify_targets.ps1`,
`scripts/audit_boundaries.ps1`, and `git diff --check`.

- [x] **Step 2: Inspect the final diff**

Confirm changes are limited to the new pure layout module, its tests, the
legacy application menu integration, and local planning records. Confirm no
VALHALLA policy or feature dispatch branch changed.

- [x] **Step 3: Record verification evidence**

Append the exact pass/fail results and any host-only test limitation to
`progress.md`. Mark the orientation phase complete in `task_plan.md` only after
both firmware builds pass.

No git commit is created because the repository contains the user's existing
uncommitted Task 1-17 work and the user previously requested documentation and
implementation to remain local.

### Task 5: Board-Specific Full-Image Packaging

**Files:**
- Create: `scripts/verify_build_freshness.ps1`
- Create: `scripts/verify_flash_layout.ps1`
- Create: `scripts/package_firmware.ps1`

- [x] **Step 1: Reproduce the C5 image-layout failure**

The old C5 FULL image fails comparison at `bootloader.bin at 0x2000`, matching
the ROM's `invalid header: 0x65646f6d` report.

- [x] **Step 2: Select the bootloader offset by target**

Use `0x1000` for `esp32-cyd` and `0x2000` for `nm-cyd-c5`. Keep partitions at
`0x8000`, `boot_app0.bin` at `0xe000`, and the application at `0x10000`.

- [x] **Step 3: Reject stale builds and verify every merged segment**

Packaging first checks that the firmware is newer than source/config inputs,
then compares every byte of every input segment against the merged FULL image.

- [x] **Step 4: Rebuild and package both targets**

Both target builds and both FULL-image layout checks pass. The C5 image has
erased bytes at `0x1000`, an ESP image header at `0x2000`, a partition-table
header at `0x8000`, and an application image header at `0x10000`.
