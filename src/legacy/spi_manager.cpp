// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD SPI Bus Manager Implementation
// Manages shared VSPI bus between SD Card, CC1101, NRF24L01, and PN532
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include "spi_manager.h"

// ═══════════════════════════════════════════════════════════════════════════
// INTERNAL STATE
// ═══════════════════════════════════════════════════════════════════════════

static SPIDevice currentDevice = SPI_DEVICE_NONE;
static bool busLocked = false;
static bool initialized = false;

// ═══════════════════════════════════════════════════════════════════════════
// CS PIN CONTROL
// ═══════════════════════════════════════════════════════════════════════════

// Set all CS pins HIGH (deselect all devices)
static void deselectAllCS() {
    #if CYD_HAS_SDCARD
    digitalWrite(SD_CS, HIGH);
    #endif

    #if CYD_HAS_CC1101
    digitalWrite(CC1101_CS, HIGH);
    #endif

    #if CYD_HAS_NRF24
    digitalWrite(NRF24_CSN, HIGH);
    #endif

    #if CYD_HAS_PN532 && defined(PN532_CS)
    digitalWrite(PN532_CS, HIGH);
    #endif
}

// Select a specific CS pin (pull LOW)
static void selectCS(SPIDevice device) {
    switch (device) {
        case SPI_DEVICE_SD:
            #if CYD_HAS_SDCARD
            digitalWrite(SD_CS, LOW);
            #endif
            break;

        case SPI_DEVICE_CC1101:
            #if CYD_HAS_CC1101
            digitalWrite(CC1101_CS, LOW);
            #endif
            break;

        case SPI_DEVICE_NRF24:
            #if CYD_HAS_NRF24
            digitalWrite(NRF24_CSN, LOW);
            #endif
            break;

        case SPI_DEVICE_PN532:
            #if CYD_HAS_PN532 && defined(PN532_CS)
            digitalWrite(PN532_CS, LOW);
            #endif
            break;

        case SPI_DEVICE_NONE:
        default:
            // All deselected
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void spiManagerSetup() {
    if (initialized) {
        return;  // Already initialized
    }

    #if CYD_DEBUG
    Serial.println("[SPI] Initializing SPI bus manager...");
    #endif

    // Configure all CS pins as outputs and set HIGH (deselected)
    #if CYD_HAS_SDCARD
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    #if CYD_DEBUG
    Serial.println("[SPI]   SD Card CS (GPIO 5) configured");
    #endif
    #endif

    #if CYD_HAS_CC1101
    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);
    #if CYD_DEBUG
    Serial.printf("[SPI]   CC1101 CS (GPIO %d) configured\n", CC1101_CS);
    #endif
    #endif

    #if CYD_HAS_NRF24
    pinMode(NRF24_CSN, OUTPUT);
    digitalWrite(NRF24_CSN, HIGH);
    #if CYD_DEBUG
    Serial.printf("[SPI]   NRF24 CSN (GPIO %d) configured\n", NRF24_CSN);
    #endif
    #endif

    #if CYD_HAS_PN532 && defined(PN532_CS)
    pinMode(PN532_CS, OUTPUT);
    digitalWrite(PN532_CS, HIGH);
    #if CYD_DEBUG
    Serial.printf("[SPI]   PN532 CS (GPIO %d) configured\n", PN532_CS);
    #endif
    #endif

    // Initialize VSPI bus
    SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI);

    currentDevice = SPI_DEVICE_NONE;
    busLocked = false;
    initialized = true;

    #if CYD_DEBUG
    Serial.println("[SPI] SPI bus manager ready");
    Serial.printf("[SPI]   VSPI: SCK=%d, MISO=%d, MOSI=%d\n", VSPI_SCK, VSPI_MISO, VSPI_MOSI);
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════
// DEVICE SELECTION
// ═══════════════════════════════════════════════════════════════════════════

bool spiSelect(SPIDevice device) {
    // Check if bus is locked by another device
    if (busLocked && currentDevice != device && device != SPI_DEVICE_NONE) {
        #if CYD_DEBUG
        Serial.println("[SPI] ERROR: Bus locked, cannot switch devices");
        #endif
        return false;
    }

    // Check if device is enabled
    switch (device) {
        case SPI_DEVICE_SD:
            #if !CYD_HAS_SDCARD
            #if CYD_DEBUG
            Serial.println("[SPI] ERROR: SD card not enabled in config");
            #endif
            return false;
            #endif
            break;

        case SPI_DEVICE_CC1101:
            #if !CYD_HAS_CC1101
            #if CYD_DEBUG
            Serial.println("[SPI] ERROR: CC1101 not enabled in config");
            #endif
            return false;
            #endif
            break;

        case SPI_DEVICE_NRF24:
            #if !CYD_HAS_NRF24
            #if CYD_DEBUG
            Serial.println("[SPI] ERROR: NRF24 not enabled in config");
            #endif
            return false;
            #endif
            break;

        case SPI_DEVICE_PN532:
            #if !CYD_HAS_PN532
            #if CYD_DEBUG
            Serial.println("[SPI] ERROR: PN532 not enabled in config");
            #endif
            return false;
            #endif
            break;

        case SPI_DEVICE_NONE:
            // Always allowed
            break;
    }

    // Deselect all first
    deselectAllCS();

    // Small delay for CS to settle
    delayMicroseconds(5);

    // Select the requested device
    selectCS(device);
    currentDevice = device;

    #if CYD_DEBUG
    if (device != SPI_DEVICE_NONE) {
        const char* names[] = {"NONE", "SD", "CC1101", "NRF24", "PN532"};
        Serial.print("[SPI] Selected: ");
        Serial.println(names[device]);
    }
    #endif

    return true;
}

void spiDeselect() {
    if (busLocked) {
        #if CYD_DEBUG
        Serial.println("[SPI] WARNING: Deselecting while bus locked");
        #endif
    }

    deselectAllCS();
    currentDevice = SPI_DEVICE_NONE;
}

SPIDevice spiGetSelected() {
    return currentDevice;
}

bool spiIsSelected(SPIDevice device) {
    return currentDevice == device;
}

// ═══════════════════════════════════════════════════════════════════════════
// DEVICE-SPECIFIC HELPERS
// ═══════════════════════════════════════════════════════════════════════════

bool spiSelectSD() {
    #if CYD_HAS_SDCARD
    return spiSelect(SPI_DEVICE_SD);
    #else
    return false;
    #endif
}

bool spiSelectCC1101() {
    #if CYD_HAS_CC1101
    return spiSelect(SPI_DEVICE_CC1101);
    #else
    return false;
    #endif
}

bool spiSelectNRF24() {
    #if CYD_HAS_NRF24
    return spiSelect(SPI_DEVICE_NRF24);
    #else
    return false;
    #endif
}

bool spiSelectPN532() {
    #if CYD_HAS_PN532
    return spiSelect(SPI_DEVICE_PN532);
    #else
    return false;
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════
// BUS LOCKING
// ═══════════════════════════════════════════════════════════════════════════

void spiLock() {
    busLocked = true;
    #if CYD_DEBUG
    Serial.println("[SPI] Bus LOCKED");
    #endif
}

void spiUnlock() {
    busLocked = false;
    #if CYD_DEBUG
    Serial.println("[SPI] Bus UNLOCKED");
    #endif
}

bool spiIsLocked() {
    return busLocked;
}

// ═══════════════════════════════════════════════════════════════════════════
// SPI SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

SPISettings spiGetSettings(SPIDevice device) {
    switch (device) {
        case SPI_DEVICE_SD:
            // SD card: 4 MHz, Mode 0, MSB first
            return SPISettings(SPI_SPEED_SD, MSBFIRST, SPI_MODE0);

        case SPI_DEVICE_CC1101:
            // CC1101: 4 MHz, Mode 0, MSB first
            return SPISettings(SPI_SPEED_CC1101, MSBFIRST, SPI_MODE0);

        case SPI_DEVICE_NRF24:
            // NRF24: 8 MHz, Mode 0, MSB first
            return SPISettings(SPI_SPEED_NRF24, MSBFIRST, SPI_MODE0);

        case SPI_DEVICE_PN532:
            // PN532: 2 MHz, Mode 0, LSB first (!) — only VSPI device using LSBFIRST
            return SPISettings(SPI_SPEED_PN532, LSBFIRST, SPI_MODE0);

        default:
            // Default safe settings
            return SPISettings(1000000, MSBFIRST, SPI_MODE0);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG
// ═══════════════════════════════════════════════════════════════════════════

void spiPrintStatus() {
    #if CYD_DEBUG
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.println("              SPI BUS MANAGER STATUS");
    Serial.println("═══════════════════════════════════════════════════════════");

    Serial.print("Initialized: ");
    Serial.println(initialized ? "YES" : "NO");

    Serial.print("Bus Locked:  ");
    Serial.println(busLocked ? "YES" : "NO");

    const char* deviceNames[] = {"NONE", "SD Card", "CC1101", "NRF24", "PN532"};
    Serial.print("Selected:    ");
    Serial.println(deviceNames[currentDevice]);

    Serial.println("───────────────────────────────────────────────────────────");
    Serial.println("Device Status:");

    #if CYD_HAS_SDCARD
    Serial.print("  SD Card (GPIO 5):  ");
    Serial.println(digitalRead(SD_CS) == LOW ? "SELECTED" : "idle");
    #else
    Serial.println("  SD Card:  DISABLED");
    #endif

    #if CYD_HAS_CC1101
    Serial.printf("  CC1101 (GPIO %d):  ", CC1101_CS);
    Serial.println(digitalRead(CC1101_CS) == LOW ? "SELECTED" : "idle");
    #else
    Serial.println("  CC1101:   DISABLED");
    #endif

    #if CYD_HAS_NRF24
    Serial.printf("  NRF24 (GPIO %d):    ", NRF24_CSN);
    Serial.println(digitalRead(NRF24_CSN) == LOW ? "SELECTED" : "idle");
    #else
    Serial.println("  NRF24:    DISABLED");
    #endif

    #if CYD_HAS_PN532 && defined(PN532_CS)
    Serial.printf("  PN532 (GPIO %d):   ", PN532_CS);
    Serial.println(digitalRead(PN532_CS) == LOW ? "SELECTED" : "idle");
    #else
    Serial.println("  PN532:    DISABLED");
    #endif

    Serial.println("═══════════════════════════════════════════════════════════");
    #endif
}
