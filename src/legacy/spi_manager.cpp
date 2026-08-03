#include <Arduino.h>
#include <SPI.h>

#include "cyd_config.h"
#include "platform/ArduinoSpiBackend.h"
#include "platform/ArduinoResourceMutex.h"
#include "services/ResourceBroker.h"
#include "services/SpiBusService.h"
#include "spi_manager.h"

namespace {
ArduinoResourceMutex resourceMutex;
ResourceBroker resourceBroker(&resourceMutex);
ArduinoSpiBackend spiBackend;
SpiBusService spiService(resourceBroker, spiBackend);
SPIDevice currentDevice = SPI_DEVICE_NONE;
bool busLocked = false;
bool initialized = false;

SpiDeviceId toServiceDevice(SPIDevice device) {
    switch (device) {
        case SPI_DEVICE_SD:
            return SpiDeviceId::Sd;
        case SPI_DEVICE_CC1101:
            return SpiDeviceId::Cc1101;
        case SPI_DEVICE_NRF24:
            return SpiDeviceId::Nrf24;
        case SPI_DEVICE_PN532:
            return SpiDeviceId::Pn532;
        case SPI_DEVICE_NONE:
        default:
            return SpiDeviceId::Display;
    }
}

const char* deviceName(SPIDevice device) {
    switch (device) {
        case SPI_DEVICE_SD:
            return "SD";
        case SPI_DEVICE_CC1101:
            return "CC1101";
        case SPI_DEVICE_NRF24:
            return "NRF24";
        case SPI_DEVICE_PN532:
            return "PN532";
        case SPI_DEVICE_NONE:
        default:
            return "NONE";
    }
}
}

void spiManagerSetup() {
    if (initialized) {
        return;
    }

    initialized = spiBackend.initialize();
    currentDevice = SPI_DEVICE_NONE;
    busLocked = false;

#if CYD_DEBUG
    Serial.printf("[SPI] shared service %s (SCK=%d MISO=%d MOSI=%d)\n",
                  initialized ? "ready" : "unavailable",
                  VSPI_SCK,
                  VSPI_MISO,
                  VSPI_MOSI);
#endif
}

bool spiSelect(SPIDevice device) {
    if (!initialized) {
        spiManagerSetup();
    }
    if (device == SPI_DEVICE_NONE) {
        spiDeselect();
        return true;
    }
    if (currentDevice == device) {
        return true;
    }
    if (busLocked && currentDevice != SPI_DEVICE_NONE) {
#if CYD_DEBUG
        Serial.printf("[SPI] reject switch %s -> %s while locked\n",
                      deviceName(currentDevice), deviceName(device));
#endif
        return false;
    }
    if (currentDevice != SPI_DEVICE_NONE) {
        spiDeselect();
    }

    const SpiResult result =
        spiService.beginLegacySession(toServiceDevice(device), 0, false);
    if (result.code != SpiResultCode::Completed) {
#if CYD_DEBUG
        Serial.printf("[SPI] select %s failed: %s\n", deviceName(device), result.detail);
#endif
        return false;
    }
    currentDevice = device;
    return true;
}

void spiDeselect() {
    if (spiService.hasLegacySession()) {
        const SpiResult result = spiService.endLegacySession();
#if CYD_DEBUG
        if (result.code != SpiResultCode::Completed) {
            Serial.printf("[SPI] session cleanup failed: %s\n", result.detail);
        }
#endif
    }
    currentDevice = SPI_DEVICE_NONE;
}

SPIDevice spiGetSelected() {
    return currentDevice;
}

bool spiIsSelected(SPIDevice device) {
    return currentDevice == device;
}

bool spiSelectSD() {
    return spiSelect(SPI_DEVICE_SD);
}

bool spiSelectCC1101() {
    return spiSelect(SPI_DEVICE_CC1101);
}

bool spiSelectNRF24() {
    return spiSelect(SPI_DEVICE_NRF24);
}

bool spiSelectPN532() {
    return spiSelect(SPI_DEVICE_PN532);
}

void spiLock() {
    busLocked = true;
}

void spiUnlock() {
    busLocked = false;
}

bool spiIsLocked() {
    return busLocked;
}

SPISettings spiGetSettings(SPIDevice device) {
    switch (device) {
        case SPI_DEVICE_SD:
            return SPISettings(SPI_SPEED_SD, MSBFIRST, SPI_MODE0);
        case SPI_DEVICE_CC1101:
            return SPISettings(SPI_SPEED_CC1101, MSBFIRST, SPI_MODE0);
        case SPI_DEVICE_NRF24:
            return SPISettings(SPI_SPEED_NRF24, MSBFIRST, SPI_MODE0);
        case SPI_DEVICE_PN532:
            return SPISettings(SPI_SPEED_PN532, LSBFIRST, SPI_MODE0);
        case SPI_DEVICE_NONE:
        default:
            return SPISettings(1000000, MSBFIRST, SPI_MODE0);
    }
}

void spiPrintStatus() {
#if CYD_DEBUG
    Serial.printf("[SPI] initialized=%s locked=%s selected=%s session=%s\n",
                  initialized ? "yes" : "no",
                  busLocked ? "yes" : "no",
                  deviceName(currentDevice),
                  spiService.hasLegacySession() ? "yes" : "no");
#endif
}
