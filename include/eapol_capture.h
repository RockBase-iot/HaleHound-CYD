#ifndef EAPOL_CAPTURE_H
#define EAPOL_CAPTURE_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD EAPOL/PMKID Capture Module
// 4-way handshake capture with on-device PMKID extraction
// Outputs: hashcat .hc22000 + standard PCAP to SD card
// Created: 2026-02-16
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

namespace EapolCapture {

// Initialize — shows AP scan/selection screen
void setup();

// Main loop — handles capture + UI
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — stop promiscuous, release WiFi
void cleanup();

}  // namespace EapolCapture

#endif // EAPOL_CAPTURE_H
