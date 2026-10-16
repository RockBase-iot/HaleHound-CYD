#!/usr/bin/env python3
"""Update test_gatt_names for the B4 union of the two BLE service tables.

Three edits, each asserted to match exactly once, because a batch of same-file
edits in this repository has previously landed only the last one.

Usage:
    python build-reports/patch_gatt_names_test.py [--dry-run]
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "test" / "native" / "test_gatt_names" / "test_main.cpp"

COUNT_OLD = """    // Pinned so that adding or dropping an entry is a conscious change. Update
    // the number here and say why, rather than letting it drift.
    TEST_ASSERT_EQUAL(44, GattNameTable::serviceCount());"""

COUNT_NEW = """    // Pinned so that adding or dropping an entry is a conscious change. Update
    // the number here and say why, rather than letting it drift. Services went
    // 44 -> 75 in B4, when this table absorbed the legacy service table's rows
    // (see test_the_legacy_service_rows_are_now_covered_here below).
    TEST_ASSERT_EQUAL(75, GattNameTable::serviceCount());"""

NEW_TEST = """// B4: `include/ble_database.h` carried 73 service rows, 31 of which this table
// did not have. They are now here, which is what lets the legacy copy go. The
// 26 adopted ones carry the SIG name verbatim; the 5 member UUIDs carry a
// provenance tag rather than a name that looks assigned.
void test_the_legacy_service_rows_are_now_covered_here() {
    // Spots from the 0x183x-0x185x block only the legacy table used to have.
    TEST_ASSERT_EQUAL_STRING("Binary Sensor", GattNameTable::serviceName(0x183B));
    TEST_ASSERT_EQUAL_STRING("Emergency Configuration", GattNameTable::serviceName(0x183C));
    TEST_ASSERT_EQUAL_STRING("Audio Input Control", GattNameTable::serviceName(0x1843));
    TEST_ASSERT_EQUAL_STRING("Electronic Shelf Label", GattNameTable::serviceName(0x1857));
    TEST_ASSERT_EQUAL_STRING("Mesh Proxy Solicitation", GattNameTable::serviceName(0x1859));

    // The row the legacy table got wrong: 0x1822 is Pulse Oximeter; HTTP Proxy
    // is 0x1823. Pinned so the wrong mapping cannot come back through the
    // legacy path.
    TEST_ASSERT_EQUAL_STRING("Pulse Oximeter", GattNameTable::serviceName(0x1822));
    TEST_ASSERT_EQUAL_STRING("HTTP Proxy", GattNameTable::serviceName(0x1823));

    // Member UUIDs must say so, so a community mapping is never read as an
    // assigned number.
    TEST_ASSERT_EQUAL_STRING("Blecon service (member)", GattNameTable::serviceName(0xFD0D));
    TEST_ASSERT_EQUAL_STRING("Fast Pair service (member)", GattNameTable::serviceName(0xFE2C));
    TEST_ASSERT_EQUAL_STRING("Eddystone beacon (member)", GattNameTable::serviceName(0xFEAA));
}

int main(int, char**) {"""

RUN_OLD = """    RUN_TEST(test_tables_are_sorted_and_the_counts_are_pinned);"""

RUN_NEW = """    RUN_TEST(test_tables_are_sorted_and_the_counts_are_pinned);
    RUN_TEST(test_the_legacy_service_rows_are_now_covered_here);"""

EDITS = [
    ("service count 44 -> 75", COUNT_OLD, COUNT_NEW),
    ("new union test", "int main(int, char**) {", NEW_TEST),
    ("register the new test", RUN_OLD, RUN_NEW),
]


def main() -> int:
    dry = "--dry-run" in sys.argv
    text = TARGET.read_text(encoding="utf-8")

    for label, old, new in EDITS:
        count = text.count(old)
        if count != 1:
            sys.exit(f"patch_gatt_names_test: {label!r} matched {count} times, expected 1")
        text = text.replace(old, new)
        print(f"  ok  {label}")

    if dry:
        print("--dry-run: nothing written")
        return 0

    TARGET.write_text(text, encoding="utf-8")
    print(f"wrote {TARGET.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
