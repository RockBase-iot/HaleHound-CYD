#ifndef SUBCONFIG_H
#define SUBCONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD SubGHz Configuration
// CC1101 Radio + RCSwitch for SubGHz attacks
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include "cyd_config.h"
#include "utils.h"
#include "arduinoFFT.h"
#include <TFT_eSPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>

// ═══════════════════════════════════════════════════════════════════════════
// EXTERNAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

extern TFT_eSPI tft;

// NOTE: PCF8574 REMOVED - CYD uses touchscreen buttons instead
// extern PCF8574 pcf;  // NOT USED ON CYD

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 RADIO CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════
//
// Pin mapping from cyd_config.h:
//   CC1101_CS   = GPIO 27  (Chip Select)
//   CC1101_GDO0 = GPIO 22  (TX data TO radio)
//   CC1101_GDO2 = GPIO 35  (RX data FROM radio)
//   RADIO_SPI_SCK  = GPIO 18
//   RADIO_SPI_MOSI = GPIO 23
//   RADIO_SPI_MISO = GPIO 19
//
// ═══════════════════════════════════════════════════════════════════════════

// Radio Switching Support - Pin 22 for CC1101, Pin 16 for NRF24 CE
// Unlike ESP32-DIV, we don't have pin conflicts on CYD
namespace replayat {
    extern RCSwitch mySwitch;           // RCSwitch instance for SubGHz receive
    extern bool subghz_receive_active;  // Flag: true when CC1101 is in RX mode
}

// Cleanup function - call BEFORE switching FROM SubGHz TO 2.4GHz modes
void cleanupSubGHz();

// ═══════════════════════════════════════════════════════════════════════════
// TOUCH CONTROLLER PINS (XPT2046)
// These are board-specific and defined in cyd_config.h
// ═══════════════════════════════════════════════════════════════════════════

#define XPT2046_IRQ   CYD_TOUCH_IRQ     // Touch interrupt
#define XPT2046_MOSI  CYD_TOUCH_MOSI    // Touch data out
#define XPT2046_MISO  CYD_TOUCH_MISO    // Touch data in
#define XPT2046_CLK   CYD_TOUCH_CLK     // Touch clock
#define XPT2046_CS    CYD_TOUCH_CS      // Touch chip select

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ FUNCTION NAMESPACES
// ═══════════════════════════════════════════════════════════════════════════

namespace replayat {
    void ReplayAttackSetup();
    void ReplayAttackLoop();
}

namespace SavedProfile {
    void saveSetup();
    void saveLoop();
}

namespace subjammer {
    void subjammerSetup();
    void subjammerLoop();
}

namespace subbrute {
    void subBruteSetup();
    void subBruteLoop();
}

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 INITIALIZATION HELPER
// ═══════════════════════════════════════════════════════════════════════════

inline void initCC1101() {
    // Set SPI pins for CC1101
    ELECHOUSE_cc1101.setSpiPin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, CC1101_CS);

    // Set GDO pins (TX and RX data lines)
    ELECHOUSE_cc1101.setGDO(CC1101_GDO0, CC1101_GDO2);

    // Initialize the radio
    if (ELECHOUSE_cc1101.getCC1101()) {
        ELECHOUSE_cc1101.Init();
        #if CYD_DEBUG
        Serial.println("[CC1101] Initialized successfully");
        #endif
    } else {
        #if CYD_DEBUG
        Serial.println("[CC1101] ERROR: Not detected!");
        #endif
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// RCSWITCH PIN SETUP (HaleHound Fix Applied)
// ═══════════════════════════════════════════════════════════════════════════
//
// IMPORTANT: CiferTech had TX/RX pins SWAPPED in original ESP32-DIV!
// HaleHound Edition FIXED this. The naming is confusing but correct:
//
//   TX_PIN (GPIO 22 = GDO0) → Used for TRANSMIT (enableTransmit)
//   RX_PIN (GPIO 35 = GDO2) → Used for RECEIVE (enableReceive)
//
// The RCSwitch library function names are backwards from what you'd expect:
//   mySwitch.enableTransmit(TX_PIN);  // Data going OUT to CC1101
//   mySwitch.enableReceive(RX_PIN);   // Data coming IN from CC1101
//
// ═══════════════════════════════════════════════════════════════════════════

inline void setupRCSwitchTX() {
    replayat::mySwitch.enableTransmit(TX_PIN);
    #if CYD_DEBUG
    Serial.println("[RCSwitch] TX enabled on GPIO " + String(TX_PIN));
    #endif
}

inline void setupRCSwitchRX() {
    replayat::mySwitch.enableReceive(digitalPinToInterrupt(RX_PIN));
    replayat::subghz_receive_active = true;
    #if CYD_DEBUG
    Serial.println("[RCSwitch] RX enabled on GPIO " + String(RX_PIN));
    #endif
}

inline void disableRCSwitch() {
    replayat::mySwitch.disableTransmit();
    replayat::mySwitch.disableReceive();
    replayat::subghz_receive_active = false;
    #if CYD_DEBUG
    Serial.println("[RCSwitch] Disabled");
    #endif
}

#endif // SUBCONFIG_H
