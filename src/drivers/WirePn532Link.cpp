#include "drivers/Pn532Transport.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#endif

// The one Arduino-dependent file in the RFID stack. Everything above it -- the
// frame codec, the command sequencing, the MIFARE and NTAG command bytes -- is
// host-testable, which is why the seam is drawn at the byte level and not at
// "transceive".

namespace {
// The PN532 is specified for 100 kHz, and the NM-RF-HAT wires are long enough
// that the 400 kHz default can be marginal.
constexpr uint32_t kPn532I2cClockHz = 100000;
// Ready byte value (PN532-I2C: 0x01 = a reply is waiting).
constexpr uint8_t kPn532I2cReady = 0x01;
}  // namespace

bool WirePn532Link::begin(uint8_t address, int sda, int scl) {
#ifdef ARDUINO
    address_ = address;
    sda_ = sda;
    scl_ = scl;
    if (sda >= 0 && scl >= 0) {
        Wire.begin(static_cast<int>(sda), static_cast<int>(scl));
    } else {
        // Board-default pair: the classic CYD profile does not name one.
        Wire.begin();
    }
    Wire.setClock(kPn532I2cClockHz);
    started_ = true;
    return true;
#else
    // No Arduino: the host suites use a scripted link instead, so reaching here
    // means the wiring was done wrong rather than that the card misbehaved.
    (void)address;
    (void)sda;
    (void)scl;
    return false;
#endif
}

void WirePn532Link::end() {
#ifdef ARDUINO
    if (started_) {
        // Do not call Wire.end(): other devices share this bus on some
        // profiles, and the same rule the SD path follows applies here.
        started_ = false;
    }
#endif
    started_ = false;
}

bool WirePn532Link::write(const uint8_t* data, uint16_t length) {
#ifdef ARDUINO
    if (data == nullptr || length == 0U) {
        return false;
    }
    Wire.beginTransmission(address_);
    Wire.write(data, length);
    return Wire.endTransmission() == 0;
#else
    (void)data;
    (void)length;
    return false;
#endif
}

bool WirePn532Link::read(uint8_t* data, uint16_t length) {
#ifdef ARDUINO
    if (data == nullptr || length == 0U) {
        return false;
    }
    // One extra byte: the PN532 prefixes every I2C read with its ready status,
    // which the link consumes so callers only ever see frame bytes.
    const uint16_t wanted = static_cast<uint16_t>(length + 1U);
    const uint16_t received =
        static_cast<uint16_t>(Wire.requestFrom(static_cast<int>(address_),
                                               static_cast<int>(wanted)));
    if (received < wanted) {
        // A short read is a truncated frame, not a short reply: reporting it as
        // success would let a partial InDataExchange answer look like a MIFARE
        // NAK.
        while (Wire.available() > 0) {
            (void)Wire.read();
        }
        return false;
    }
    (void)Wire.read();  // ready byte
    for (uint16_t i = 0; i < length; ++i) {
        data[i] = static_cast<uint8_t>(Wire.read());
    }
    return true;
#else
    (void)data;
    (void)length;
    return false;
#endif
}

bool WirePn532Link::ready() {
#ifdef ARDUINO
    uint8_t status = 0;
    if (Wire.requestFrom(static_cast<int>(address_), 1) != 1) {
        return false;
    }
    status = static_cast<uint8_t>(Wire.read());
    return status == kPn532I2cReady;
#else
    return false;
#endif
}

uint32_t WirePn532Link::millis() const {
#ifdef ARDUINO
    return ::millis();
#else
    return 0;
#endif
}

void WirePn532Link::delayMs(uint32_t ms) {
#ifdef ARDUINO
    delay(ms);
#else
    (void)ms;
#endif
}
