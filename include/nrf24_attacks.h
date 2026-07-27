#ifndef NRF24_ATTACKS_H
#define NRF24_ATTACKS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD NRF24 Attack Modules
// STUB IMPLEMENTATIONS - CYD does not have NRF24 hardware
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════
//
// NOTE: The CYD (Cheap Yellow Display) board does not include NRF24L01 radios.
// These are STUB implementations that display "Hardware Not Available" messages.
//
// To use NRF24 features, you need:
//   - NRF24L01+PA+LNA module
//   - Wiring: VCC=3.3V, GND, SCK=GPIO18, MOSI=GPIO23, MISO=GPIO19
//   - CSN=GPIO4, CE=GPIO16, IRQ=GPIO17 (optional)
//   - 10uF capacitor on VCC for stability
//
// ATTACK MODULES (STUBS) - MATCHES ORIGINAL ESP32-DIV:
// ┌──────────────────────────────────────────────────────────────────────────┐
// │ Scanner - 2.4GHz Channel Scanner (REQUIRES NRF24 HARDWARE)               │
// │ Analyzer - Spectrum Analyzer (REQUIRES NRF24 HARDWARE)                   │
// │ WLANJammer - WLAN Jammer (REQUIRES NRF24 HARDWARE)                       │
// │ ProtoKill - Protocol Kill Attack (REQUIRES NRF24 HARDWARE)               │
// └──────────────────────────────────────────────────────────────────────────┘
//
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 SCANNER - 2.4GHz Channel Scanner
// Original function names: scannerSetup(), scannerLoop()
// ═══════════════════════════════════════════════════════════════════════════

namespace Scanner {

// Initialize scanner (shows hardware not available)
void scannerSetup();

// Main loop function
void scannerLoop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace Scanner

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 SPECTRUM ANALYZER
// Original function names: analyzerSetup(), analyzerLoop()
// ═══════════════════════════════════════════════════════════════════════════

namespace Analyzer {

// Initialize (shows hardware not available)
void analyzerSetup();

// Main loop function
void analyzerLoop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace Analyzer

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 WLAN JAMMER
// Original function names: wlanjammerSetup(), wlanjammerLoop()
// ═══════════════════════════════════════════════════════════════════════════

namespace WLANJammer {

// Initialize (shows hardware not available)
void wlanjammerSetup();

// Main loop function
void wlanjammerLoop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace WLANJammer

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 PROTO KILL
// Original function names: prokillSetup(), prokillLoop()
// ═══════════════════════════════════════════════════════════════════════════

namespace ProtoKill {

// Initialize (shows hardware not available)
void prokillSetup();

// Main loop function
void prokillLoop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace ProtoKill

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 PROMISCUOUS SNIFFER
// Travis Goodspeed technique — captures raw 2.4GHz packets from ANY device
// Extracts device addresses for MouseJack injection
// ═══════════════════════════════════════════════════════════════════════════

namespace NrfSniffer {

// Initialize sniffer and draw UI
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace NrfSniffer

// ═══════════════════════════════════════════════════════════════════════════
// MOUSEJACK - Wireless Keyboard Keystroke Injection
// Injects keystrokes into vulnerable Logitech Unifying, Dell, Microsoft
// 2.4GHz wireless keyboards via NRF24L01+
// Ref: Bastille Research (mousejack.com), Marc Newlin nrf-research-firmware
// ═══════════════════════════════════════════════════════════════════════════

namespace MouseJack {

// Initialize MouseJack and draw UI
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace MouseJack

#endif // NRF24_ATTACKS_H
