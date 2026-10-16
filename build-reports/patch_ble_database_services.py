#!/usr/bin/env python3
"""B4: stop include/ble_database.h keeping a second copy of the service numbers.

What was measured before this change
------------------------------------
`ble_database.h` is included by exactly one file, src/legacy/bluetooth_attacks.cpp,
and that file uses **only** `lookupCompanyName`. Repo-wide greps find no caller
for `lookupServiceName`, `lookupAppearanceCategory`, `lookupAppearanceDetail` or
`lookupCompanyFromMfgData` (the file defines its own static `extractCompanyId`,
shadowing the header's). So the 73-row service table was dead code -- and it
carried a real drift bug: 0x1822 labelled "HTTP Proxy", where the SIG assigns
0x1822 to Pulse Oximeter and 0x1823 to HTTP Proxy.

What this patch does
--------------------
  * Service data deleted. `lookupServiceName()` stays, delegating to
    BleLegacyDisplay, which resolves through GattNameTable and clamps to the
    panel's 14 characters. The legacy property survives; the duplicate numbers
    do not.
  * Company IDs kept -- the only section with a caller.
  * Appearances kept and marked as having no caller, because nothing supersedes
    them yet and deleting data with no replacement is a separate call.

Usage:
    python build-reports/patch_ble_database_services.py [--dry-run]
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "include" / "ble_database.h"

INCLUDE_OLD = """#include <Arduino.h>
#include <pgmspace.h>
"""

INCLUDE_NEW = """#include <Arduino.h>
#include <pgmspace.h>

// Service names resolve through GattNameTable via BleLegacyDisplay. Keeping the
// clamp in a pure TU is what makes it testable; this header cannot be.
#include "features/ble/BleLegacyDisplay.h"
"""

SECTION2_NEW = """// ═══════════════════════════════════════════════════════════════════════════════
// SECTION 2: GATT SERVICE UUIDs
// ═══════════════════════════════════════════════════════════════════════════════
// The numbers and their names live in exactly one place: GattNameTable
// (include/features/ble/GattNameTable.h). This section used to carry its own
// 73-row copy, which meant two lists of the same assigned numbers and, in
// practice, two different answers: 0x1822 here said "HTTP Proxy" while the SIG
// assigns it to Pulse Oximeter (HTTP Proxy is 0x1823). B4 deleted the copy.
//
// What is kept is the legacy presentation rule: this panel budgets 14
// characters per row, so a name is clamped rather than allowed to overrun it.
//
// Returns nullptr for an unknown UUID, preserving the old contract -- callers
// print the raw UUID rather than a name that could not be verified.
static const char* lookupServiceName(uint16_t uuid) {
    static char row[BleLegacyDisplay::kMaxNameChars + 1];
    if (!BleLegacyDisplay::writeServiceName(uuid, row, sizeof(row))) {
        return nullptr;
    }
    return row;
}

"""

# From the SECTION 2 banner up to (not including) the SECTION 3 banner.
SECTION2_RE = re.compile(
    r"// ═+\n// SECTION 2: GATT SERVICE UUIDs.*?\n(?=// ═+\n// SECTION 3: GAP APPEARANCE)",
    re.S,
)

SECTION3_OLD = """// We store category-level names (52 entries). For subcategory detail,
// use lookupAppearanceDetail() which returns specific types like
// "Keyboard", "Smartwatch", "Earbud" etc."""

SECTION3_NEW = """// We store category-level names (52 entries). For subcategory detail,
// use lookupAppearanceDetail() which returns specific types like
// "Keyboard", "Smartwatch", "Earbud" etc.
//
// NOTE (B4): nothing in this tree calls lookupAppearanceCategory/Detail, and
// GattNameTable does not cover appearance values, so this block is retained
// rather than superseded. It is dead weight today (~1 KB of PROGMEM); delete it
// only alongside a decision about where appearance names should live."""


def main() -> int:
    dry = "--dry-run" in sys.argv
    text = TARGET.read_text(encoding="utf-8")

    before = len(text)

    for label, old, new in (
        ("include BleLegacyDisplay", INCLUDE_OLD, INCLUDE_NEW),
        ("note the appearance block", SECTION3_OLD, SECTION3_NEW),
    ):
        count = text.count(old)
        if count != 1:
            sys.exit(f"patch_ble_database_services: {label!r} matched {count} times, expected 1")
        text = text.replace(old, new)
        print(f"  ok  {label}")

    matches = SECTION2_RE.findall(text)
    if len(matches) != 1:
        sys.exit(f"patch_ble_database_services: SECTION 2 block matched {len(matches)} times")
    text = SECTION2_RE.sub(SECTION2_NEW, text, count=1)
    print("  ok  replace SECTION 2 with the delegating shim")

    for forbidden in ("_sn_", "bleServiceDB", "BleServiceEntry", "BLE_SERVICE_DB_SIZE"):
        left = text.count(forbidden)
        if left:
            sys.exit(f"patch_ble_database_services: {forbidden!r} still present ({left})")
    print("  ok  no service data or _sn_ strings remain")

    if "lookupServiceName" not in text:
        sys.exit("patch_ble_database_services: the delegating lookupServiceName is missing")

    print(f"size {before} -> {len(text)} chars  (-{before - len(text)})")

    if dry:
        print("--dry-run: nothing written")
        return 0

    TARGET.write_text(text, encoding="utf-8")
    print(f"wrote {TARGET.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
