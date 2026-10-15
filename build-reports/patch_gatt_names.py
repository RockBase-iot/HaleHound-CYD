#!/usr/bin/env python3
"""Apply the SIG-verification results to GattNameTable and its tests.

`build-reports/verify_gatt_names.py` diffed the hand-written table against the
Bluetooth SIG assigned-numbers YAML and found:

  * 13 names that disagreed with the SIG, including 0x2A31, which was pointing
    at "Scan Parameters" (that is the *service* 0x1813) while the SIG assigns
    the characteristic 0x2A31 the name "Scan Refresh".
  * 7 entries the SIG does not assign at all -- a mix of withdrawn
    characteristics and ones I mis-remembered. Unverifiable entries are deleted
    rather than excused, matching docs/protocol-evidence/mifare-classic.md.
  * 1 member-assigned UUID whose name did not declare its provenance.

Every replacement asserts its match count first, so a silent no-op is
impossible. Idempotence is not a goal: run it once.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def patch(relative: str, edits: list[tuple[str, str, int]]) -> None:
    """Apply (old, new, expected_count) edits to one file, asserting each."""
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    for old, new, expected in edits:
        found = text.count(old)
        if found != expected:
            sys.exit(
                f"{relative}: expected {expected} occurrence(s) of\n  {old!r}\n"
                f"but found {found}. No changes were written."
            )
        text = text.replace(old, new)
    path.write_text(text, encoding="utf-8")
    print(f"patched {relative}: {len(edits)} edit(s)")


# ---------------------------------------------------------------------------
# 1. The tables themselves
# ---------------------------------------------------------------------------

TABLE_EDITS: list[tuple[str, str, int]] = [
    # 13 names corrected to the SIG's own wording (abbreviations included).
    ('{0x1800, "Generic Access"}', '{0x1800, "GAP"}', 1),
    ('{0x1801, "Generic Attribute"}', '{0x1801, "GATT"}', 1),
    ('{0x1805, "Current Time Service"}', '{0x1805, "Current Time"}', 1),
    ('{0x1806, "Reference Time Update Service"}', '{0x1806, "Reference Time Update"}', 1),
    ('{0x1807, "Next DST Change Service"}', '{0x1807, "Next DST Change"}', 1),
    ('{0x180E, "Phone Alert Status Service"}', '{0x180E, "Phone Alert Status"}', 1),
    ('{0x180F, "Battery Service"}', '{0x180F, "Battery"}', 1),
    ('{0x1811, "Alert Notification Service"}', '{0x1811, "Alert Notification"}', 1),
    ('{0x1820, "Internet Protocol Support Service"}', '{0x1820, "Internet Protocol Support"}', 1),
    ('{0x1825, "Object Transfer Service"}', '{0x1825, "Object Transfer"}', 1),
    ('{0x1827, "Mesh Provisioning Service"}', '{0x1827, "Mesh Provisioning"}', 1),
    ('{0x1828, "Mesh Proxy Service"}', '{0x1828, "Mesh Proxy"}', 1),
    ('{0x2A31, "Scan Parameters"}', '{0x2A31, "Scan Refresh"}', 1),

    # 7 entries the SIG does not assign: deleted, not excused.
    ('    {0x2A1F, "Temperature Celsius"},\n', '', 1),
    ('    {0x2A20, "Temperature Fahrenheit"},\n', '', 1),
    ('    {0x2A2D, "Scan Refresh"},\n', '', 1),
    ('    {0x2A2E, "Boot Keyboard Output Report"},\n', '', 1),
    ('    {0x2A2F, "Boot Mouse Input Report"},\n', '', 1),
    ('    {0x2A56, "Digital"},\n', '', 1),
    ('    {0x2A58, "Analog"},\n', '', 1),

    # 0xFD6F is an Apple/Google member assignment, not a SIG-adopted service.
    ('{0xFD6F, "Exposure Notification Service"}',
     '{0xFD6F, "Exposure Notification Service (member)"}', 1),

    # The vendor range is 0xFFxx in practice; 0xFFF0..0xFFFF is the block the
    # spec reserves for duplicating an assigned number.
    ("    // 0xFFF0..0xFFFF is the range the spec reserves for members to duplicate\n"
     "    // without a separate license -- i.e. where the unregistered vendors live.\n"
     "    return uuid >= 0xFFF0;",
     "    // 0xFF00..0xFFFF is where unregistered vendors squat in practice -- the LED\n"
     "    // controllers and UART bridges this build targets all sit there. The\n"
     "    // narrower 0xFFF0..0xFFFF block is what the spec reserves for duplicating\n"
     "    // an assigned number without a separate license.\n"
     "    return uuid >= 0xFF00;",
     1),
]

# ---------------------------------------------------------------------------
# 2. The header's contract description
# ---------------------------------------------------------------------------

HEADER_EDITS: list[tuple[str, str, int]] = [
    ("//   3. The table is curated, not exhaustive. It covers what this build's screens\n"
     "//      actually render: the Generic Access/Attribute services (every device has\n"
     "//      them), Device Information, Battery, HID, the health/fitness sensors, the\n"
     "//      environmental-sensing block, and the RGB-controller and serial-bridge\n"
     "//      vendor ranges. Names come from the Bluetooth SIG Assigned Numbers\n"
     "//      document; the vendor entries are marked as such in `vendorNote`.",
     "//   3. The table is curated, not exhaustive -- but every entry is checkable.\n"
     "//      Names are taken VERBATIM from the Bluetooth SIG Assigned Numbers list,\n"
     "//      which means the SIG's own abbreviations stand: 0x1800 is \"GAP\", not\n"
     "//      \"Generic Access\", and 0x180F is \"Battery\", not \"Battery Service\".\n"
     "//      Following the SIG rather than common usage is what lets\n"
     "//      build-reports/verify_gatt_names.py diff the whole table with no\n"
     "//      allowlist. Coverage is what this build's screens render: GAP/GATT,\n"
     "//      Device Information, Battery, HID, the health/fitness sensors, the\n"
     "//      environmental-sensing block, and the RGB-controller and serial-bridge\n"
     "//      vendor ranges. An entry the SIG does not assign carries a \"(vendor)\"\n"
     "//      or \"(member)\" provenance tag.\n"
     "//\n"
     "// Note on the legacy table: include/ble_database.h is an older,\n"
     "// Arduino-dependent service list whose names are truncated to ~14 characters\n"
     "// for a small display, and it is used only by src/legacy/bluetooth_attacks.cpp.\n"
     "// This table holds canonical names and is host-testable. The two are expected\n"
     "// to differ in wording; ble_database.h should eventually delegate here rather\n"
     "// than keep a second copy of the same assigned numbers.",
     1),
    ("    // True for the 0xFFF0..0xFFFF range, where non-SIG vendors squat. Kept\n"
     "    // separate so the dump can label a name as vendor-supplied.\n"
     "    static bool isVendorShortUuid(uint16_t uuid);",
     "    // True for the 0xFF00..0xFFFF range, where unregistered vendors squat;\n"
     "    // 0xFFF0..0xFFFF specifically is the block the spec reserves for\n"
     "    // duplicating an assigned number. Kept separate so the dump can label a\n"
     "    // name as vendor-supplied.\n"
     "    static bool isVendorShortUuid(uint16_t uuid);",
     1),
]

# ---------------------------------------------------------------------------
# 3. Tests: the pinned counts and the names the SIG wording changed
# ---------------------------------------------------------------------------

GATT_TEST_EDITS: list[tuple[str, str, int]] = [
    # 113 -> 106 after the 7 deletions.
    ("TEST_ASSERT_EQUAL(113, GattNameTable::characteristicCount());",
     "TEST_ASSERT_EQUAL(106, GattNameTable::characteristicCount());", 1),
    ('TEST_ASSERT_EQUAL_STRING("Generic Access", GattNameTable::serviceName(0x1800));',
     'TEST_ASSERT_EQUAL_STRING("GAP", GattNameTable::serviceName(0x1800));', 1),
    ('TEST_ASSERT_EQUAL_STRING("Battery Service", GattNameTable::serviceName(0x180F));',
     'TEST_ASSERT_EQUAL_STRING("Battery", GattNameTable::serviceName(0x180F));', 1),
    ('    TEST_ASSERT_EQUAL_STRING("Battery Service",\n'
     '                             GattNameTable::resolve("0000180f-0000-1000-8000-00805f9b34fb",\n'
     '                                                    GattUuidKind::Service)\n'
     '                                 .c_str());',
     '    TEST_ASSERT_EQUAL_STRING("Battery",\n'
     '                             GattNameTable::resolve("0000180f-0000-1000-8000-00805f9b34fb",\n'
     '                                                    GattUuidKind::Service)\n'
     '                                 .c_str());',
     1),
    ("    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFF0));",
     "    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFE0));\n"
     "    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFF0));", 1),
]

SKELETON_TEST_EDITS: list[tuple[str, str, int]] = [
    ('TEST_ASSERT_EQUAL_STRING("Battery Service", tree.services[0].name.c_str());',
     'TEST_ASSERT_EQUAL_STRING("Battery", tree.services[0].name.c_str());', 1),
    ('TEST_ASSERT_EQUAL_STRING("Battery Service", session.tree().services[0].name.c_str());',
     'TEST_ASSERT_EQUAL_STRING("Battery", session.tree().services[0].name.c_str());', 1),
]


def main() -> int:
    patch("src/features/ble/GattNameTable.cpp", TABLE_EDITS)
    patch("include/features/ble/GattNameTable.h", HEADER_EDITS)
    patch("test/native/test_gatt_names/test_main.cpp", GATT_TEST_EDITS)
    patch("test/native/test_skeleton_key/test_main.cpp", SKELETON_TEST_EDITS)
    print("\nNow re-run: build-reports/verify_gatt_names.py and the hostcheck suites.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
