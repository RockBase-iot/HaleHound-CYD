#ifndef JAM_DETECT_H
#define JAM_DETECT_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Jamming Detection Module
// Defensive RF spectrum monitoring — all 3 radios
// Created: 2026-03-02
// ═══════════════════════════════════════════════════════════════════════════
//
// DETECTION MODULES:
// ┌──────────────────────────────────────────────────────────────────────────┐
// │ WiFiGuardian   - Deauth/disassoc/beacon flood detection (promiscuous)  │
// │ SubSentinel    - CC1101 RSSI baseline comparison (300-928 MHz)         │
// │ GHzWatchdog    - NRF24 RPD channel occupancy (2400-2484 MHz)          │
// │ FullSpectrum   - All 3 radios time-shared, unified threat dashboard   │
// └──────────────────────────────────────────────────────────────────────────┘
//
// THREAT LEVELS:
//   CALIBRATING → CLEAR → SUSPICIOUS → JAMMING
//                   ↑         ↓            ↓
//                   └─── (3s clear) ───────┘
//
// DUAL-CORE PATTERN:
//   Core 0: Radio scanning (CC1101/NRF24)
//   Core 1: Display + touch (TFT/input)
//   WiFi Guardian: No dual-core needed (promiscuous callback is interrupt)
//
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// SHARED THREAT LEVEL ENUM
// ═══════════════════════════════════════════════════════════════════════════

enum ThreatLevel {
    THREAT_CALIBRATING = 0,
    THREAT_CLEAR,
    THREAT_SUSPICIOUS,
    THREAT_JAMMING
};

// ═══════════════════════════════════════════════════════════════════════════
// WIFI GUARDIAN — Deauth / Disassoc / Beacon Flood Detection
// Uses ESP32 built-in WiFi in promiscuous mode (no external radio)
// ═══════════════════════════════════════════════════════════════════════════

namespace WiFiGuardian {

// Initialize promiscuous mode, draw UI, start calibration
void setup();

// Main loop — called repeatedly from feature runner
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — disable promiscuous mode, release WiFi
void cleanup();

}  // namespace WiFiGuardian

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ SENTINEL — CC1101 RSSI Baseline Jamming Detection
// 33-frequency sweep, baseline comparison, dual-core
// ═══════════════════════════════════════════════════════════════════════════

namespace SubSentinel {

// Initialize CC1101, draw UI, start calibration
void setup();

// Main loop — called repeatedly from feature runner
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — stop scan task, release CC1101/SPI
void cleanup();

}  // namespace SubSentinel

// ═══════════════════════════════════════════════════════════════════════════
// 2.4GHZ WATCHDOG — NRF24 RPD Channel Occupancy Detection
// 85-channel RPD sweep, baseline comparison, dual-core
// ═══════════════════════════════════════════════════════════════════════════

namespace GHzWatchdog {

// Initialize NRF24, draw UI, start calibration
void setup();

// Main loop — called repeatedly from feature runner
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — stop scan task, release NRF24/SPI
void cleanup();

}  // namespace GHzWatchdog

// ═══════════════════════════════════════════════════════════════════════════
// FULL SPECTRUM — All 3 Radios, Unified Threat Dashboard
// WiFi promiscuous (always on) + CC1101/NRF24 time-shared on VSPI
// ═══════════════════════════════════════════════════════════════════════════

namespace FullSpectrum {

// Initialize all radios, draw UI, start calibration
void setup();

// Main loop — called repeatedly from feature runner
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — stop all tasks, release all radios
void cleanup();

}  // namespace FullSpectrum

#endif // JAM_DETECT_H
