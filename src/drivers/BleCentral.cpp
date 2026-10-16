#include "drivers/IBleCentral.h"

#include <cstdio>
#include <cstring>

const char* bleStatusLabel(BleStatus status) {
    switch (status) {
        case BleStatus::Ok:
            return "OK";
        case BleStatus::NotFound:
            return "not found";
        case BleStatus::NoService:
            return "no such service";
        case BleStatus::NotPermitted:
            return "not permitted";
        case BleStatus::TooLong:
            return "value too long";
        case BleStatus::ConnectFailed:
            return "connect failed";
        case BleStatus::Disconnected:
            return "disconnected";
        case BleStatus::Timeout:
            return "timeout";
        case BleStatus::Busy:
            return "busy";
        case BleStatus::IoError:
            return "io error";
        case BleStatus::Unsupported:
            return "unsupported";
    }
    return "unknown";
}

namespace {

// The SIG base UUID's 12 fixed trailing bytes, and its two zero fields. Written
// once here so `fromShort()`, `isShort()` and `shortValue()` cannot disagree
// about what "on the base" means.
constexpr uint8_t kBaseTrailing[12] = {
    0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

bool onSigBase(const std::array<uint8_t, 16>& bytes) {
    // Layout: 0000xxxx-0000-1000-8000-00805f9b34fb
    if (bytes[0] != 0x00 || bytes[1] != 0x00) {
        return false;
    }
    if (bytes[4] != 0x00 || bytes[5] != 0x00) {
        return false;
    }
    return std::memcmp(bytes.data() + 4, kBaseTrailing, sizeof(kBaseTrailing)) == 0;
}

}  // namespace

BleUuid BleUuid::fromShort(uint16_t value) {
    BleUuid uuid;
    uuid.bytes[2] = static_cast<uint8_t>(value >> 8);
    uuid.bytes[3] = static_cast<uint8_t>(value & 0xFF);
    std::memcpy(uuid.bytes.data() + 4, kBaseTrailing, sizeof(kBaseTrailing));
    return uuid;
}

BleUuid BleUuid::fromBytes(const uint8_t* sixteen) {
    BleUuid uuid;
    if (sixteen != nullptr) {
        std::memcpy(uuid.bytes.data(), sixteen, 16);
    }
    return uuid;
}

bool BleUuid::valid() const {
    for (uint8_t index = 0; index < 16; ++index) {
        if (bytes[index] != 0) {
            return true;
        }
    }
    return false;
}

bool BleUuid::isShort() const {
    return onSigBase(bytes);
}

uint16_t BleUuid::shortValue() const {
    if (!onSigBase(bytes)) {
        // 0 rather than a plausible-looking offset: a caller that forgets to ask
        // isShort() gets the "no name" answer instead of the wrong name.
        return 0;
    }
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[2]) << 8) | bytes[3]);
}

bool BleUuid::write(char* out, std::size_t capacity) const {
    if (out == nullptr || capacity == 0U) {
        return false;
    }
    out[0] = '\0';
    // 32 hex digits + 4 hyphens = 36, plus the terminator.
    if (capacity < 37U) {
        return false;
    }
    std::snprintf(out, capacity,
                  "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6],
                  bytes[7], bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13],
                  bytes[14], bytes[15]);
    return true;
}

bool BleUuid::operator==(const BleUuid& other) const {
    return std::memcmp(bytes.data(), other.bytes.data(), 16) == 0;
}

bool BleUuid::operator!=(const BleUuid& other) const {
    return !(*this == other);
}

bool BleAddress::valid() const {
    // All-zero is what an unwritten struct holds and is never a real peer, so it
    // doubles as the "unset" sentinel.
    for (uint8_t index = 0; index < 6; ++index) {
        if (bytes[index] != 0) {
            return true;
        }
    }
    return false;
}

bool BleAddress::write(char* out, std::size_t capacity) const {
    if (out == nullptr || capacity == 0U) {
        return false;
    }
    out[0] = '\0';
    // 6 bytes -> 5 separators is 17 characters, plus the terminator.
    if (capacity < 18U) {
        return false;
    }
    std::snprintf(out, capacity, "%02X:%02X:%02X:%02X:%02X:%02X", bytes[0], bytes[1],
                  bytes[2], bytes[3], bytes[4], bytes[5]);
    return true;
}

bool BleAddress::operator==(const BleAddress& other) const {
    // The type is part of the identity: the same 48 bits as a public and as a
    // random address are two different peers as far as the controller is
    // concerned, and connecting to the wrong one is a real failure mode.
    return type == other.type && std::memcmp(bytes.data(), other.bytes.data(), 6) == 0;
}

bool BleAddress::operator!=(const BleAddress& other) const {
    return !(*this == other);
}

bool BleAdvertisement::advertises(const BleUuid& uuid) const {
    for (uint8_t index = 0; index < serviceCount && index < kBleMaxAdvServices; ++index) {
        if (serviceUuids[index] == uuid) {
            return true;
        }
    }
    return false;
}

BleCharacteristicRef BleCharacteristicRef::fromShort(uint16_t service,
                                                     uint16_t characteristic) {
    BleCharacteristicRef ref;
    ref.service = BleUuid::fromShort(service);
    ref.characteristic = BleUuid::fromShort(characteristic);
    return ref;
}

bool BleCharacteristicRef::valid() const {
    // An all-zero UUID is the "nothing here" filler and is never a real handle
    // in a discovery result, so an unset ref is rejected rather than sent.
    return service.valid() && characteristic.valid();
}

bool BleCharacteristicRef::operator==(const BleCharacteristicRef& other) const {
    // Both halves are the identity: the same characteristic UUID under two
    // services is two different values, and a subscription keyed on the UUID
    // alone would deliver the wrong peer's data.
    return service == other.service && characteristic == other.characteristic;
}

bool BleCharacteristicRef::operator!=(const BleCharacteristicRef& other) const {
    return !(*this == other);
}
