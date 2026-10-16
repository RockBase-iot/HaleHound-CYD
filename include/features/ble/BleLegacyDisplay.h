#pragma once

// Legacy BLE display names for the small CYD panel.
//
// B4 ("BLE name-library unification") put every assigned number in exactly one
// place: GattNameTable. This module is what the legacy BLE screens read instead
// of keeping a second copy of those numbers. It resolves through GattNameTable
// and applies the one thing that is genuinely a legacy concern -- the old
// panel's 14-character row budget.
//
// Why this is its own translation unit instead of inline in
// include/ble_database.h: that header includes Arduino.h, so nothing inside it
// can run under the host test runner. Having the clamp here is what turns "no
// legacy row overflows" into an assertion rather than a promise.
//
// Unknown stays unknown. A UUID with no name returns false / nullptr so the
// caller prints the raw UUID -- the same rule GattNameTable follows, and the
// reason a GATT dump can be used as evidence.

#include <cstddef>
#include <cstdint>

class BleLegacyDisplay {
public:
    // The old panel's per-row budget. It is a rendering constraint, not a data
    // one, which is why the full name is stored once and clamped here.
    static constexpr std::size_t kMaxNameChars = 14;

    // Full name for a 16-bit service UUID, or nullptr when the UUID is unknown.
    static const char* serviceName(uint16_t uuid);

    // Clamps `name` into `out`, always NUL-terminated and never longer than
    // kMaxNameChars. Returns false for a null or empty name, leaving `out`
    // empty rather than half-written.
    static bool writeClampedName(const char* name, char* out, std::size_t capacity);

    // Resolve-then-clamp. False when the UUID is unknown.
    static bool writeServiceName(uint16_t uuid, char* out, std::size_t capacity);

    // The longest service name in the table. Exposed so a test can show the
    // clamp is actually exercised instead of merely present.
    static std::size_t longestServiceNameLength();
};
