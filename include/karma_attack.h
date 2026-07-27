#ifndef KARMA_ATTACK_H
#define KARMA_ATTACK_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Karma Attack Module
// Probe request listener + rogue AP responder
// Created: 2026-02-16
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>

namespace KarmaAttack {

// Initialize — starts probe collection
void setup();

// Main loop — handles collection + attack
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup — stop WiFi, release resources
void cleanup();

}  // namespace KarmaAttack

#endif // KARMA_ATTACK_H
