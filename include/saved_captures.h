#ifndef SAVED_CAPTURES_H
#define SAVED_CAPTURES_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Saved Captures Browser
// SD card file browser for /eapol/ directory (.hc22000, .pcap)
// Created: 2026-02-16
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

namespace SavedCaptures {

// Initialize — mount SD, scan directory, draw list
void setup();

// Main loop — handles touch, scrolling, file actions
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — release SD
void cleanup();

}  // namespace SavedCaptures

#endif // SAVED_CAPTURES_H
