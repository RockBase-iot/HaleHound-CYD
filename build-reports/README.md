# build-reports

Scratch area for build/verification tooling and its output. Nothing here ships in
the firmware.

## `syntax_env_check.py` — real-toolchain syntax verification

Compiles changed translation units with the *real* environment flags by reading the
authoritative files PlatformIO itself writes:

- `nm-cyd-c5` → `.pio/c5-core/packages/framework-arduinoespressif32-libs/esp32c5/flags/*`
- `esp32-cyd` → `.pio/packages/framework-arduinoespressif32/tools/sdk/...`

```bash
python build-reports/syntax_env_check.py --env nm-cyd-c5 src/features/rfid/CardDumpFormat.cpp
python build-reports/syntax_env_check.py --env esp32-cyd --baseline   # tool-noise proof
```

`--baseline` compiles untouched files with the same flags. If they fail identically,
the failure is the tool configuration, not your change.

No env fails its baseline any more, and that is the point: both causes were fixed
rather than documented around.

* `esp32-cyd` used to report `'assert' was not declared` / `'errno' was not declared`
  for *every* TU, touched or not. Cause: the pre-split framework's
  `tools/sdk/<chip>/include/newlib/platform_include/` shadows the toolchain's own
  `assert.h` / `errno.h`, and `soc/spinlock.h` calls `assert()` without including
  `<assert.h>` — the real build only survives because it force-includes them.
  `prepare()` now prepends the `xtensa-esp32-elf/include` directory and adds
  `-include <sysinc>/assert.h` / `errno.h` *before* the `@rsp`, which is where they
  have to go.
* `esp32-cyd` also passed no `ARDUINO`, `ARDUINO_ARCH_ESP32` or `ESP32`. That was
  worse than a bad error message. With `ARDUINO` undefined, every `#ifdef ARDUINO`
  body in that environment preprocessed to nothing, so `src/drivers/Esp32BleCentral.cpp`
  reported PASS while **not one line of it was compiled** — a green check on a
  discarded translation unit, which is worse than no check at all. Adding the define
  surfaced a real defect immediately (a missing `#include <BLEDevice.h>`), so it had
  been hiding something. Values and sources:

  | Define | Value | Where it comes from |
  |---|---|---|
  | `ARDUINO` | `20017` | `2*10000 + 0*100 + 17`; framework `package.json` is `3.20017.241212` and `esp_arduino_version.h` says MAJOR 2. Same encoding the `nm-cyd-c5` entry uses for 3.3.6 (`30306`). |
  | `ARDUINO_ARCH_ESP32` | — | `{build.arch}` in the framework's `platform.txt` recipes |
  | `ESP32` | `ESP32` | `-DESP32=ESP32`, one occurrence in the same `platform.txt`. The libdeps RF24 library picks its architecture by testing `defined(ESP32)`, not `defined(ARDUINO_ARCH_ESP32)`; without it, it takes the AVR branch and includes `<avr/pgmspace.h>`. |

  **Before trusting a PASS, check that the TU was compiled at all** — a guard that is
  false in this environment is the one failure mode `-fsyntax-only` cannot tell you
  about.

Not covered: linking, `extra_scripts`, `build_src_filter`, Flash/RAM footprint.

## `hostenv/` — a host C++ compiler, without a system install

`pio run` is unusable in this sandbox (the safe-delete guard's 50-file-per-round budget
is blown by the package manager before the first `Compiling` line), and the bundled
`toolchain-gccmingw32` cannot compile anything: it ships no 32-bit `libgmp-10.dll` /
`libmpfr-4.dll`, so `g++ --version` works and every real compile exits 1 with an *empty*
log.

`ziglang` is a pip wheel that bundles clang plus a complete libc++/libc sysroot, so it
gives a working host compiler with nothing installed system-wide:

```bash
python -m venv build-reports/hostenv
build-reports/hostenv/Scripts/python.exe -m pip install --upgrade pip ziglang
build-reports/hostenv/Scripts/python.exe -m ziglang version    # 0.16.0 (clang 21.1.0)
```

`hostenv/` is git-ignored. Re-create it with the three commands above on a fresh clone.

## `run_host_tests.py` — compile *and execute* the native suites

```bash
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py --list
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py test_heap_baseline
```

Compiler resolution order: `--cxx` → `$HOSTCXX` → PlatformIO mingw32 → PlatformIO
mingw64 → PATH `g++`/`clang++` → repo-local `ziglang` venv. **Every candidate is
probe-compiled first** (`int main(){return 0;}`, built, linked *and run*), because a
present-but-unloadable toolchain otherwise gets picked and then fails with an empty log.

Two things it does that are easy to get wrong:

1. Shared sources are archived into `build-reports/hostcheck/libhostcheck.a` and linked
   as an archive, mirroring the `native` PlatformIO env. Force-linking the raw `.o` files
   instead pulls in `src/core/AppKernel.o`, which needs `legacySetup` / `legacyLoop` from
   the `build_src_filter`-excluded `src/app/LegacyApp.inc`, and the link dies with
   `undefined symbol: legacySetup()`. Do **not** "fix" that with empty stubs — the stubs
   would mask genuine undefined symbols in the firmware build.
2. `-w` is passed deliberately: zig's bundled libc++ headers otherwise emit a wall of
   nullability warnings on any `<string>` / `<functional>` use.

Result on 2026-10-16: **49/49 suites, 446 test cases green** (`test/native/` holds 50
directories; `support/` is a header-only test double and has no suite).

## `patch_*.py` — applied-and-verified source edits

Each script performs exact-string replacements with a match-count assertion, so a
silent no-op cannot happen. They are kept for provenance; re-running one whose edit is
already applied will abort harmlessly (`expected 1 match, found 0`). The table lists
every tracked script, not only the recent ones.

A second trap these scripts have to avoid: `Path.write_text()` without `newline=""`
rewrites every line ending in the target file (`\n` → CRLF on Windows). With
`core.autocrlf=true` the diff still shows only the intended lines, because git
normalises before comparing — so the rewrite stays invisible until someone commits with
that setting off. Read and write with `newline=""` and the file is left alone.

| Script | What it changed |
|---|---|
| `patch_legacy_policy.py` | 21 duplicated gates in `LegacyApp.inc` → `requestOffensiveAccess()`; policy panel, wipe-SD, banner tap |
| `patch2.py`, `patch3.py`, `patch_catalog.py` | feature catalog / lifecycle edits |
| `patch_hostcheck_zig.py` | `run_host_tests.py`: compiler as a command prefix + zig detection |
| `patch_hostcheck_probe.py` | `run_host_tests.py`: probe-compile every compiler candidate |
| `patch_hostcheck_archive.py` | `run_host_tests.py`: archive shared objects before linking |
| `patch_hostcheck_fixes.py` | the nine latent defects from the first host run (see DEVELOPMENT_PLAN.md §5.5) |
| `patch_plan_hostcheck.py` | DEVELOPMENT_PLAN.md §5.4 / §5.5 / §7 / §9 |
| `patch_gatt_names.py` | B4: `GattNameTable` made SIG-verbatim -- 13 wrong names corrected (0x2A31 was pointing at the Scan Parameters *service*), 7 unassigned entries deleted |
| `patch_gatt_names_union.py` | B4: `GattNameTable::kServices` grown to the union of the two BLE service tables, so the legacy 73-row copy could be deleted without losing coverage |
| `patch_gatt_names_accessors.py` | B4: index accessors on `GattNameTable`, so a test can prove every service name clamps to the legacy panel's 14-character budget |
| `patch_gatt_names_test.py` | `test_gatt_names` updated for the B4 union |
| `patch_ble_database_services.py` | B4: deleted `include/ble_database.h`'s dead 73-row service table (its only caller used `lookupCompanyName`; 0x1822 was labelled HTTP Proxy where the SIG assigns that number to Pulse Oximeter) |
| `patch_pn532_transport.py` | `Pn532Transport.cpp`: I2C timing after a write, and UID bookkeeping |
| `patch_pn532_transport_test.py` | three defects in `test_pn532_transport` before its first run: a reply helper that takes an initializer_list, a test that re-called `transceive` against an unscripted fake, a wrong comment about where the status byte goes |
| `patch_mifare_driver_test.py` | three wrong expectations in `test_mifare_classic_driver`: the ATQA the NTAG row needs, and `unknownBlockCount()` counting every unrecovered block |
| `patch_hid_write_ref.py` | the HID reader picks its write target from the discovered GATT properties, instead of writing commands to the notify characteristic |
| `patch_chameleon_rearm.py` | the Chameleon receiver now looks for a frame start inside a rejected window -- a real frame whose SOF landed in the doomed header window used to be consumed and lost |
| `patch_chameleon_slot_enabled.py` | `ChameleonSlot` grows `enabled`, so GET_ENABLED_SLOTS' per-slot flag pair has somewhere to land instead of the answer being discarded |
| `patch_chameleon_frame_test.py` | two `test_chameleon_frame` assumptions: this shim has no `TEST_ASSERT_EQUAL_MEMORY`, and `TEST_ASSERT_*` may not expand inside a non-void function |
| `patch_hostcheck_support.py` | `run_host_tests.py`: `test/native/support` added to the include path so suites can share scripted test doubles |
| `patch_plan_remaining.py` | `DEVELOPMENT_PLAN.md` after the 2026-10-15 RFID/BLE model-layer session |
| `patch_remaining_work_rfid.py` | `REMAINING_WORK.md` after the PN532 driver layer landed: four new suites, R3/R4/R5/R7 done at the driver layer, R1 demoted to P1 by instruction |
| `patch_remaining_work_deps.py` | three `REMAINING_WORK.md` dependency cells the driver layer made stale: R2 -> W1, R8 unblocked, R10 unblocked |
| `patch_remaining_work.py` | `REMAINING_WORK.md` after closing B1-B3 and the host half of B5 |
| `patch_release_brief.py` | `docs/upstream-analysis/RELEASE_BRIEF_zh.md`: the stale gap matrix and its statistics, the module/suite inventories, and two verification rows that still claimed native tests cannot run |
| `patch_skill_syntax_vacuity.py` | **targets a file outside this repo**: the `esp32-platformio-syntax-check` skill gained the "-fsyntax-only PASS that compiled nothing" subsection |
| `patch_patch_inventory.py` | this table: the 19 tracked-but-unlisted scripts above, and the note below |
| `patch_u2_wiring.py` | U2: `LegacyApp.inc`'s ten-case Bluetooth switch -> catalog dispatch, the policy store moved to `src/core/PolicyService.cpp`, the catalog-derived submenu arrays |
| `patch_u2_docs.py` | U2 docs: `REMAINING_WORK.md` §0.7, the two stale claims in this file, the evidence doc §6 |

> One script in this table edits a file outside the repository: `patch_skill_syntax_vacuity.py` writes
> `~/.workbuddy/skills/esp32-platformio-syntax-check/SKILL.md`. It is kept here so that change is still
> traceable from the repo. Every other script touches only tracked source, tests and docs.
