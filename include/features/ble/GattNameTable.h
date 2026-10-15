#pragma once

// GATT UUID -> human name.
//
// A GATT tree is unreadable as sixteen-byte hex, which is exactly why upstream
// "deepened the dumps" with real characteristic names in v3.9.1. This is that
// table, split out of SkeletonKeyModel so the dump path and the on-screen tree
// resolver share one source of truth.
//
// Three rules, in priority order:
//
//   1. Normalise first. Every Bluetooth SIG UUID is `xxxxxxxx-0000-1000-8000-
//      00805f9b34fb`, so it collapses to its 16-bit field. That is what every
//      GATT tool prints, and it means the vendor LED-controller UUIDs
//      (`0000fff0-...`) fall out of the same path as the SIG ones.
//   2. Unknown means empty. `resolve()` returns "" and the caller shows the raw
//      UUID. It never invents a plausible-looking name -- a wrong name in a dump
//      is worse than no name, because the dump is evidence.
//   3. The table is curated, not exhaustive -- but every entry is checkable.
//      Names are taken VERBATIM from the Bluetooth SIG Assigned Numbers list,
//      which means the SIG's own abbreviations stand: 0x1800 is "GAP", not
//      "Generic Access", and 0x180F is "Battery", not "Battery Service".
//      Following the SIG rather than common usage is what lets
//      build-reports/verify_gatt_names.py diff the whole table with no
//      allowlist. Coverage is what this build's screens render: GAP/GATT,
//      Device Information, Battery, HID, the health/fitness sensors, the
//      environmental-sensing block, and the RGB-controller and serial-bridge
//      vendor ranges. An entry the SIG does not assign carries a "(vendor)"
//      or "(member)" provenance tag.
//
// Note on the legacy table: include/ble_database.h is an older,
// Arduino-dependent service list whose names are truncated to ~14 characters
// for a small display, and it is used only by src/legacy/bluetooth_attacks.cpp.
// This table holds canonical names and is host-testable. The two are expected
// to differ in wording; ble_database.h should eventually delegate here rather
// than keep a second copy of the same assigned numbers.

#include <cstdint>
#include <string>

enum class GattUuidKind : uint8_t {
    Service,
    Characteristic,
    Descriptor,
};

class GattNameTable {
public:
    // The base UUID every SIG-assigned UUID is built from. Anything else is a
    // genuine 128-bit vendor UUID and is left alone.
    static const char* baseUuidSuffix();

    // Collapses "180F", "0x180F", "0000180F" or the full 128-bit form to 0x180F.
    // Returns false for a UUID that is not built on the SIG base -- including a
    // malformed one, because a malformed UUID must not silently become a short
    // one.
    static bool toShortUuid(const std::string& uuid, uint16_t& out);

    // Empty string when the UUID is not in the table. Never a guess.
    static const char* serviceName(uint16_t uuid);
    static const char* characteristicName(uint16_t uuid);
    static const char* descriptorName(uint16_t uuid);

    // True for the 0xFF00..0xFFFF range, where unregistered vendors squat;
    // 0xFFF0..0xFFFF specifically is the block the spec reserves for
    // duplicating an assigned number. Kept separate so the dump can label a
    // name as vendor-supplied.
    static bool isVendorShortUuid(uint16_t uuid);

    // Resolves either UUID form. Returns "" for anything unknown, including a
    // genuine 128-bit vendor UUID that is not on the base.
    static std::string resolve(const std::string& uuid, GattUuidKind kind);

    static const char* kindLabel(GattUuidKind kind);

    // Table sizes, so a test can pin that the tables were not silently emptied.
    static uint16_t serviceCount();
    static uint16_t characteristicCount();
    static uint16_t descriptorCount();

    // True when all three tables are strictly ascending by UUID. `lookup()` is a
    // linear scan, so an out-of-order or duplicated entry would not crash -- it
    // would just make the second copy unreachable. Exposed so a test can prove
    // that has not happened.
    static bool tablesAreSorted();
};
