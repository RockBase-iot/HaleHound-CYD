#include "drivers/Pn532I2cDriver.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Adafruit_PN532.h>
#include <Wire.h>
#endif

ProbeResult Pn532I2cDriver::probe(const ExpansionRoute& route,
                                  uint32_t timeoutMs) {
    (void)timeoutMs;
    if (route.transport != Transport::I2c) {
        return {PeripheralStatus::Unsupported, "PN532 I2C route unavailable"};
    }
#ifdef ARDUINO
    if (route.controlA >= 0 && route.controlB >= 0) {
        Wire.begin(route.controlA, route.controlB);
    } else {
        Wire.begin();
    }
    Adafruit_PN532 pn532(0xFFU, 0xFFU, &Wire);
    pn532.begin();
    const uint32_t version = pn532.getFirmwareVersion();
    if (version != 0U && version != 0xFFFFFFFFUL) {
        return {PeripheralStatus::Available, "PN532 firmware version responded"};
    }
    return {PeripheralStatus::NotFound, "PN532 firmware version not found"};
#else
    return {PeripheralStatus::Unsupported, "PN532 probe requires Arduino I2C"};
#endif
}
