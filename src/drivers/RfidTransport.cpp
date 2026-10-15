#include "drivers/IRfidTransport.h"

const char* rfidStatusLabel(RfidStatus status) {
    switch (status) {
        case RfidStatus::Ok:
            return "ok";
        case RfidStatus::NoTarget:
            return "no card in field";
        case RfidStatus::Timeout:
            return "card stopped answering";
        case RfidStatus::FrameError:
            return "unexpected response";
        case RfidStatus::CrcError:
            return "crc mismatch";
        case RfidStatus::Collision:
            return "more than one card";
        case RfidStatus::AuthFailed:
            return "authentication rejected";
        case RfidStatus::IoError:
            return "reader I/O error";
        case RfidStatus::Unsupported:
            return "not supported by this reader";
        default:
            return "unknown reader status";
    }
}

uint16_t rfidCrcA(const uint8_t* data, uint16_t length) {
    // Reflected (LSB-first) form of x^16 + x^12 + x^5 + 1, initial value 0x6363
    // in the register's orientation -- the catalogue writes the same CRC as
    // init 0xC6C6, and the check value over "123456789" is 0xBF05 either way.
    // Feeding the init value through the loop is what the datasheet's table
    // form does, so the two agree byte for byte.
    uint16_t crc = 0x6363U;
    for (uint16_t i = 0; i < length; ++i) {
        uint8_t byte = static_cast<uint8_t>(data[i] ^ (crc & 0xFFU));
        byte = static_cast<uint8_t>(byte ^ (byte << 4));
        crc = static_cast<uint16_t>(
            (crc >> 8) ^
            (static_cast<uint16_t>(byte) << 8) ^
            (static_cast<uint16_t>(byte) << 3) ^
            (static_cast<uint16_t>(byte) >> 4));
    }
    return crc;
}

void rfidAppendCrcA(uint8_t* data, uint16_t length) {
    const uint16_t crc = rfidCrcA(data, length);
    data[length] = static_cast<uint8_t>(crc & 0xFFU);
    data[length + 1] = static_cast<uint8_t>(crc >> 8);
}

bool rfidCheckCrcA(const uint8_t* data, uint16_t length) {
    if (length < 2U) {
        return false;
    }
    const uint16_t expected = rfidCrcA(data, static_cast<uint16_t>(length - 2U));
    const uint16_t actual = static_cast<uint16_t>(
        data[length - 2U] | (static_cast<uint16_t>(data[length - 1U]) << 8));
    return expected == actual;
}
