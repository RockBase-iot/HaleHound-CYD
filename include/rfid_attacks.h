#ifndef RFID_ATTACKS_H
#define RFID_ATTACKS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD RFID/NFC Attack Module
// PN532 13.56 MHz — Scanner, Reader, Clone, Brute Force, Emulation
// Created: 2026-03-02
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// PN532 INITIALIZATION (shared across all RFID features)
// ═══════════════════════════════════════════════════════════════════════════

// Initialize PN532 on shared VSPI bus
// Deselects all other SPI devices, calls nfc.begin(), getFirmwareVersion(), SAMConfig()
// Returns true if PN532 responded and is ready
bool pn532Init();

// Release PN532 — deselect CS, free bus for other devices
void pn532Cleanup();

// Check if PN532 was detected during last init
bool pn532IsPresent();

// ═══════════════════════════════════════════════════════════════════════════
// CARD SCANNER — Detect and identify cards in range
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDScanner {
    void setup();
    void loop();
    void cleanup();
    bool isExitRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
// CARD READER — Read all sectors and dump to screen/SD
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDReader {
    void setup();
    void loop();
    void cleanup();
    bool isExitRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
// CARD CLONE — Read source card, write to blank
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDClone {
    void setup();
    void loop();
    void cleanup();
    bool isExitRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
// KEY BRUTE FORCE — Try known default keys against all sectors (dual-core)
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDBrute {
    void setup();
    void loop();
    void cleanup();
    bool isExitRequested();
}

// ═══════════════════════════════════════════════════════════════════════════
// CARD EMULATION — Emulate a card UID using PN532 target mode
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDEmulate {
    void setup();
    void loop();
    void cleanup();
    bool isExitRequested();
}

#endif // RFID_ATTACKS_H
