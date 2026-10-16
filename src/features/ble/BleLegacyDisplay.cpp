#include "features/ble/BleLegacyDisplay.h"

#include <cstring>

#include "features/ble/GattNameTable.h"

const char* BleLegacyDisplay::serviceName(uint16_t uuid) {
    const char* name = GattNameTable::serviceName(uuid);
    // GattNameTable signals "unknown" with an empty string so its own callers can
    // print it directly. The legacy callers test for a missing name instead, so
    // the two conventions are reconciled here rather than at each call site.
    return (name != nullptr && name[0] != '\0') ? name : nullptr;
}

bool BleLegacyDisplay::writeClampedName(const char* name, char* out, std::size_t capacity) {
    if (out == nullptr || capacity == 0U) {
        return false;
    }
    out[0] = '\0';
    if (name == nullptr || name[0] == '\0') {
        return false;
    }

    std::size_t length = std::strlen(name);
    if (length > kMaxNameChars) {
        length = kMaxNameChars;
    }
    // A caller with a smaller buffer gets that buffer's limit, never an overrun.
    if (length > capacity - 1U) {
        length = capacity - 1U;
    }
    std::memcpy(out, name, length);
    out[length] = '\0';
    return true;
}

bool BleLegacyDisplay::writeServiceName(uint16_t uuid, char* out, std::size_t capacity) {
    return writeClampedName(serviceName(uuid), out, capacity);
}

std::size_t BleLegacyDisplay::longestServiceNameLength() {
    std::size_t longest = 0U;
    const uint16_t count = GattNameTable::serviceCount();
    for (uint16_t index = 0; index < count; ++index) {
        const std::size_t length = std::strlen(GattNameTable::serviceNameAt(index));
        if (length > longest) {
            longest = length;
        }
    }
    return longest;
}
