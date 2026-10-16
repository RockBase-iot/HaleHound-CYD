#!/usr/bin/env python3
"""Add the "-fsyntax-only PASS that compiled nothing" subsection to the skill.

The skill file is CRLF; read/write with newline="" and translate the pattern's
\\n to \\r\\n so the whole file is not silently rewritten.

Usage: python build-reports/patch_skill_syntax_vacuity.py
"""

from __future__ import annotations

import sys
from pathlib import Path

SKILL = Path.home() / ".workbuddy" / "skills" / "esp32-platformio-syntax-check" / "SKILL.md"

ANCHOR = "## Comparing baseline vs. edited file"

INSERT = """### A `-fsyntax-only` PASS that compiled nothing: a missing guard macro

Separate from the SCons vacuity above, and it happens on the fallback path.
`-fsyntax-only` cannot distinguish "compiled cleanly" from "preprocessed away". If the
flag set lacks a macro that guards your file, every line of it is discarded and the
check reports PASS.

Measured case (HaleHound-CYD, `esp32-cyd`, 2026-10-16): the hand-built flag list passed
no `ARDUINO`. `src/drivers/Esp32BleCentral.cpp` is wrapped in
`#if defined(ARDUINO) && (defined(CONFIG_NIMBLE_ENABLED) || defined(CONFIG_BLUEDROID_ENABLED))`,
so the whole TU preprocessed to nothing and reported PASS across several sessions.
Adding `ARDUINO` immediately produced four real errors -- `'BLEDevice' has not been
declared` -- because the adapter header had never included `<BLEDevice.h>`: NimBLE
brings it in transitively through the other BLE headers, Bluedroid 2.0.17 keeps
`class BLEDevice` inside its own `#if defined(CONFIG_BLUEDROID_ENABLED)` and nothing
else pulls it in. The green check had been hiding a genuine defect.

**Three ways to check before trusting a PASS:**
1. Preprocess and look for your own symbols:
   `<gxx> <flags> -E <file> | grep -c '<one of your symbols>'` -- `0` means discarded.
2. Grep the flag set for every macro your top-level `#if` tests.
3. Introduce a one-line syntax error, confirm the check FAILS, then revert. A check that
   cannot fail proves nothing.

**What the legacy env was missing, and where each value comes from -- never guess:**

| Define | Value | Source |
|---|---|---|
| `ARDUINO` | `major*10000 + minor*100 + patch` (framework `package.json` `3.20017.241212` -> Arduino 2.0.17 -> `20017`) | same encoding the modern env already used for 3.3.6 (`30306`) |
| `ARDUINO_ARCH_ESP32` | -- | `{build.arch}` in the framework `platform.txt` recipe |
| `ESP32=ESP32` | `ESP32` | `-DESP32=ESP32` in that same `platform.txt` |

The last one is not cosmetic: libdeps libraries that test `defined(ESP32)` rather than
`defined(ARDUINO_ARCH_ESP32)` (RF24 does exactly this) otherwise fall through to their
AVR branch and `#include <avr/pgmspace.h>`.

"""


def main() -> int:
    text = SKILL.read_text(encoding="utf-8", newline="")
    crlf = "\r\n" in text
    anchor = ANCHOR.replace("\n", "\r\n") if crlf else ANCHOR
    insert = INSERT.replace("\n", "\r\n") if crlf else INSERT

    if "-fsyntax-only` PASS that compiled nothing" in text:
        sys.exit("skill: subsection already present. Nothing was written.")
    if text.count(anchor) != 1:
        sys.exit(f"skill: anchor found {text.count(anchor)} times, expected 1. Nothing written.")

    SKILL.write_text(text.replace(anchor, insert + anchor), encoding="utf-8", newline="")
    before = len(text.splitlines())
    after = len(SKILL.read_text(encoding="utf-8", newline="").splitlines())
    print(f"skill patched: {before} -> {after} lines (CRLF={crlf})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
