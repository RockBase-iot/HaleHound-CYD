#!/usr/bin/env python3
"""Add index accessors to GattNameTable and refresh its ble_database.h note.

Three edits across two files, each asserted to match exactly once.

Why the accessors: B4 needs to *prove* that every service name clamps to the
legacy panel's 14-character budget. Without enumeration a test can only spot
check, and spot checks are how the original 0x1822 drift survived.

Usage:
    python build-reports/patch_gatt_names_accessors.py [--dry-run]
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "include" / "features" / "ble" / "GattNameTable.h"
SRC = ROOT / "src" / "features" / "ble" / "GattNameTable.cpp"

HDR_COUNTS_OLD = """    // Table sizes, so a test can pin that the tables were not silently emptied.
    static uint16_t serviceCount();
    static uint16_t characteristicCount();
    static uint16_t descriptorCount();"""

HDR_COUNTS_NEW = """    // Table sizes, so a test can pin that the tables were not silently emptied.
    static uint16_t serviceCount();
    static uint16_t characteristicCount();
    static uint16_t descriptorCount();

    // Index access, so a caller can audit the whole table instead of keeping a
    // private copy of the numbers. `index` at or past serviceCount() returns 0
    // and "" respectively.
    static uint16_t serviceUuidAt(uint16_t index);
    static const char* serviceNameAt(uint16_t index);"""

HDR_NOTE_OLD = """// Note on the legacy table: include/ble_database.h is an older,
// Arduino-dependent service list whose names are truncated to ~14 characters
// for a small display, and it is used only by src/legacy/bluetooth_attacks.cpp.
// This table holds canonical names and is host-testable. The two are expected
// to differ in wording; ble_database.h should eventually delegate here rather
// than keep a second copy of the same assigned numbers."""

HDR_NOTE_NEW = """// Note on the legacy table: include/ble_database.h is an older,
// Arduino-dependent list used only by src/legacy/bluetooth_attacks.cpp. It no
// longer carries service names of its own -- `lookupServiceName()` there
// delegates to BleLegacyDisplay, which resolves through this table and clamps
// to the old panel's 14-character budget. The assigned numbers therefore exist
// in exactly one place, and the clamp is host-testable; see
// docs/protocol-evidence/gatt-names.md section 5."""

SRC_OLD = """uint16_t GattNameTable::descriptorCount() {
    return static_cast<uint16_t>(sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}"""

SRC_NEW = """uint16_t GattNameTable::descriptorCount() {
    return static_cast<uint16_t>(sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}

uint16_t GattNameTable::serviceUuidAt(uint16_t index) {
    if (index >= serviceCount()) {
        return 0;
    }
    return kServices[index].uuid;
}

const char* GattNameTable::serviceNameAt(uint16_t index) {
    if (index >= serviceCount()) {
        return "";
    }
    return kServices[index].name;
}"""

EDITS = [
    (HDR, "header: index accessors", HDR_COUNTS_OLD, HDR_COUNTS_NEW),
    (HDR, "header: legacy delegation note", HDR_NOTE_OLD, HDR_NOTE_NEW),
    (SRC, "source: accessor definitions", SRC_OLD, SRC_NEW),
]


def main() -> int:
    dry = "--dry-run" in sys.argv
    texts = {path: path.read_text(encoding="utf-8") for path in {p for p, *_ in EDITS}}

    for path, label, old, new in EDITS:
        text = texts[path]
        count = text.count(old)
        if count != 1:
            sys.exit(f"patch_gatt_names_accessors: {label!r} matched {count} times, expected 1")
        texts[path] = text.replace(old, new)
        print(f"  ok  {path.name}: {label}")

    if dry:
        print("--dry-run: nothing written")
        return 0

    for path, text in texts.items():
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
