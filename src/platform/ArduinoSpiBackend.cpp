#include "platform/ArduinoSpiBackend.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <SPI.h>

#include "cyd_config.h"
#include "spi_manager.h"

namespace {
SPIDevice toLegacyDevice(SpiDeviceId device) {
    switch (device) {
        case SpiDeviceId::Sd:
            return SPI_DEVICE_SD;
        case SpiDeviceId::Cc1101:
            return SPI_DEVICE_CC1101;
        case SpiDeviceId::Nrf24:
            return SPI_DEVICE_NRF24;
        case SpiDeviceId::Pn532:
            return SPI_DEVICE_PN532;
        case SpiDeviceId::Display:
        case SpiDeviceId::Touch:
            return SPI_DEVICE_NONE;
    }
    return SPI_DEVICE_NONE;
}

void configureChipSelect(int pin) {
    if (pin >= 0) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
}

void deselectChip(int pin) {
    if (pin >= 0) {
        digitalWrite(pin, HIGH);
    }
}

void selectChip(int pin) {
    if (pin >= 0) {
        digitalWrite(pin, LOW);
    }
}

int chipSelectFor(SpiDeviceId device) {
    switch (device) {
        case SpiDeviceId::Sd:
            return SD_CS;
        case SpiDeviceId::Cc1101:
            return CC1101_CS;
        case SpiDeviceId::Nrf24:
            return NRF24_CSN;
        case SpiDeviceId::Pn532:
#if defined(PN532_CS)
            return PN532_CS;
#else
            return -1;
#endif
        case SpiDeviceId::Display:
#if defined(TFT_CS)
            return TFT_CS;
#else
            return -1;
#endif
        case SpiDeviceId::Touch:
#if defined(TOUCH_CS)
            return TOUCH_CS;
#else
            return -1;
#endif
    }
    return -1;
}
}

bool ArduinoSpiBackend::initialize() {
    configureChipSelect(SD_CS);
    configureChipSelect(CC1101_CS);
    configureChipSelect(NRF24_CSN);
#if defined(PN532_CS)
    configureChipSelect(PN532_CS);
#endif
#if defined(TFT_CS)
    configureChipSelect(TFT_CS);
#endif
#if defined(TOUCH_CS)
    configureChipSelect(TOUCH_CS);
#endif
    SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI);
    return true;
}

bool ArduinoSpiBackend::lock(uint32_t) {
    return true;
}

void ArduinoSpiBackend::unlock() {}

void ArduinoSpiBackend::deselectAll() {
    deselectChip(SD_CS);
    deselectChip(CC1101_CS);
    deselectChip(NRF24_CSN);
#if defined(PN532_CS)
    deselectChip(PN532_CS);
#endif
#if defined(TFT_CS)
    deselectChip(TFT_CS);
#endif
#if defined(TOUCH_CS)
    deselectChip(TOUCH_CS);
#endif
}

bool ArduinoSpiBackend::beginTransaction(SpiDeviceId device) {
    SPI.beginTransaction(spiGetSettings(toLegacyDevice(device)));
    return true;
}

bool ArduinoSpiBackend::select(SpiDeviceId device) {
    switch (device) {
        case SpiDeviceId::Display:
#if defined(TFT_CS)
            selectChip(TFT_CS);
#endif
            return true;
        case SpiDeviceId::Touch:
#if defined(TOUCH_CS)
            selectChip(TOUCH_CS);
#endif
            return true;
        default:
            if (device == SpiDeviceId::Sd || device == SpiDeviceId::Cc1101 ||
                device == SpiDeviceId::Nrf24 || device == SpiDeviceId::Pn532) {
                const int pin = chipSelectFor(device);
                if (pin < 0) {
                    return false;
                }
                selectChip(pin);
                return true;
            }
            return false;
    }
}

uint8_t ArduinoSpiBackend::transfer(uint8_t value) {
    return SPI.transfer(value);
}

void ArduinoSpiBackend::deselect(SpiDeviceId device) {
    switch (device) {
        case SpiDeviceId::Display:
#if defined(TFT_CS)
            deselectChip(TFT_CS);
#endif
            break;
        case SpiDeviceId::Touch:
#if defined(TOUCH_CS)
            deselectChip(TOUCH_CS);
#endif
            break;
        default:
            deselectChip(chipSelectFor(device));
            break;
    }
}

void ArduinoSpiBackend::endTransaction() {
    SPI.endTransaction();
}

#else

bool ArduinoSpiBackend::initialize() { return false; }
bool ArduinoSpiBackend::lock(uint32_t) { return false; }
void ArduinoSpiBackend::unlock() {}
void ArduinoSpiBackend::deselectAll() {}
bool ArduinoSpiBackend::beginTransaction(SpiDeviceId) { return false; }
bool ArduinoSpiBackend::select(SpiDeviceId) { return false; }
uint8_t ArduinoSpiBackend::transfer(uint8_t value) { return value; }
void ArduinoSpiBackend::deselect(SpiDeviceId) {}
void ArduinoSpiBackend::endTransaction() {}

#endif
