#ifndef SUBGHZ_ATTACKS_H
#define SUBGHZ_ATTACKS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD SubGHz Attack Modules
// CC1101 Signal Capture & Replay with RMT Hardware Timing
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════
//
// ATTACK MODULE:
// ┌──────────────────────────────────────────────────────────────────────────┐
// │ ReplayAttack - SubGHz Signal Capture & Replay                           │
// │   - RCSwitch-based signal capture (protocols 1-12)                      │
// │   - RMT hardware-timed transmission (~100ns jitter vs 10-150μs)         │
// │   - 17 preset frequencies (300MHz - 925MHz)                             │
// │   - Auto-scan mode with RSSI-based pause                                │
// │   - Profile save/load with custom naming                                │
// │   - On-screen touch keyboard for profile names                          │
// └──────────────────────────────────────────────────────────────────────────┘
//
// FREQUENCY LIST:
// 300.000 | 303.875 | 304.250 | 310.000 | 315.000 | 318.000
// 390.000 | 418.000 | 433.075 | 433.420 | 433.920 | 434.420
// 434.775 | 438.900 | 868.350 | 915.000 | 925.000 MHz
//
// CC1101 WIRING (uses SPI Manager):
//   VCC  → 3.3V
//   GND  → GND
//   SCK  → GPIO 18 (shared VSPI)
//   MOSI → GPIO 23 (shared VSPI)
//   MISO → GPIO 19 (shared VSPI)
//   CSN  → GPIO 27
//   GDO0 → GPIO 16 (TX data TO CC1101)
//   GDO2 → GPIO 26 (RX data FROM CC1101)
//
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <RCSwitch.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "driver/rmt.h"
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// REPLAY ATTACK
// ═══════════════════════════════════════════════════════════════════════════

namespace ReplayAttack {

// Profile structure for saved signals
struct SignalProfile {
    uint32_t frequency;      // Frequency in Hz
    unsigned long value;     // Captured code value
    int bitLength;           // Number of bits
    int protocol;            // RCSwitch protocol (1-12)
    char name[16];           // Custom name
};

// Maximum profiles that can be saved
#define MAX_PROFILES 4

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

// Initialize replay attack module - call before first use
void setup();

// Main loop function - call repeatedly
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup and release CC1101/SPI resources
void cleanup();

// ═══════════════════════════════════════════════════════════════════════════
// FREQUENCY CONTROL
// ═══════════════════════════════════════════════════════════════════════════

// Get number of preset frequencies
int getFrequencyCount();

// Get frequency at index (in Hz)
uint32_t getFrequency(int index);

// Get frequency at index (in MHz)
float getFrequencyMHz(int index);

// Set current frequency index
void setFrequencyIndex(int index);

// Get current frequency index
int getFrequencyIndex();

// Get current frequency in Hz
uint32_t getCurrentFrequency();

// Get current frequency in MHz
float getCurrentFrequencyMHz();

// Next frequency
void nextFrequency();

// Previous frequency
void prevFrequency();

// ═══════════════════════════════════════════════════════════════════════════
// AUTO-SCAN MODE
// ═══════════════════════════════════════════════════════════════════════════

// Enable/disable auto-scan
void setAutoScan(bool enabled);

// Toggle auto-scan
void toggleAutoScan();

// Check if auto-scan is enabled
bool isAutoScanEnabled();

// Check if auto-scan is paused (signal detected)
bool isAutoScanPaused();

// ═══════════════════════════════════════════════════════════════════════════
// SIGNAL CAPTURE
// ═══════════════════════════════════════════════════════════════════════════

// Check if a signal has been captured
bool hasSignal();

// Get captured signal value
unsigned long getCapturedValue();

// Get captured signal bit length
int getCapturedBitLength();

// Get captured signal protocol
int getCapturedProtocol();

// Get current RSSI
int getRSSI();

// Clear captured signal
void clearSignal();

// ═══════════════════════════════════════════════════════════════════════════
// SIGNAL TRANSMISSION
// ═══════════════════════════════════════════════════════════════════════════

// Send the currently captured signal
void sendSignal();

// Send a specific signal
void sendSignal(unsigned long value, int bitLength, int protocol);

// Send using RMT hardware timing (more precise)
bool sendSignalRMT(unsigned long value, int bitLength, int protocol, int repetitions = 10);

// ═══════════════════════════════════════════════════════════════════════════
// PROFILE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

// Save current signal as profile
bool saveProfile(const char* name);

// Load profile by index
bool loadProfile(int index);

// Delete profile by index
bool deleteProfile(int index);

// Get profile count
int getProfileCount();

// Get profile at index
SignalProfile* getProfile(int index);

// ═══════════════════════════════════════════════════════════════════════════
// RMT DRIVER (for hardware-timed OOK transmission)
// ═══════════════════════════════════════════════════════════════════════════

// Initialize RMT driver
bool initRMT();

// Check if RMT is initialized
bool isRMTInitialized();

// Transmit RMT symbols
void rmtTransmit(rmt_item32_t* items, size_t numItems);

}  // namespace ReplayAttack

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ JAMMER
// ═══════════════════════════════════════════════════════════════════════════

namespace SubJammer {

// Initialize jammer module
void setup();

// Main loop function
void loop();

// Start jamming
void start();

// Stop jamming
void stop();

// Toggle jamming state
void toggle();

// Check if currently jamming
bool isJamming();

// Set frequency (MHz)
void setFrequency(float mhz);

// Get current frequency (MHz)
float getFrequency();

// Next frequency
void nextFrequency();

// Previous frequency
void prevFrequency();

// Toggle auto-sweep mode
void toggleAutoSweep();

// Check if auto-sweep enabled
bool isAutoSweep();

// Toggle continuous carrier mode
void toggleContinuousMode();

// Check if continuous mode
bool isContinuousMode();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace SubJammer

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ BRUTE FORCE
// ═══════════════════════════════════════════════════════════════════════════

namespace SubBrute {

// Protocol definitions - 21 protocols covering all major SubGHz frequencies
enum Protocol {
    // CAME 12-bit
    PROTO_CAME_433 = 0,
    PROTO_CAME_315,
    // Nice FLO 12-bit
    PROTO_NICE_433,
    PROTO_NICE_315,
    // Linear 10-bit
    PROTO_LINEAR_300,
    PROTO_LINEAR_310,
    // Chamberlain 9-bit
    PROTO_CHAMBERLAIN,
    // PT2262/EV1527 12-bit - covers ALL common frequencies
    PROTO_PT2262_300,
    PROTO_PT2262_303,
    PROTO_PT2262_304,
    PROTO_PT2262_310,
    PROTO_PT2262_315,
    PROTO_PT2262_318,
    PROTO_PT2262_390,
    PROTO_PT2262_418,
    PROTO_PT2262_433,
    PROTO_PT2262_434,
    PROTO_PT2262_438,
    PROTO_PT2262_868,
    PROTO_PT2262_915,
    PROTO_COUNT  // = 21
};

// Initialize brute force module
void setup();

// Main loop function
void loop();

// Start brute force attack
void startAttack();

// Stop attack
void stopAttack();

// Pause/resume attack
void togglePause();

// Check if attack is running
bool isRunning();

// Check if attack is paused
bool isPaused();

// Set protocol
void setProtocol(int proto);

// Get current protocol
int getProtocol();

// Next protocol
void nextProtocol();

// Previous protocol
void prevProtocol();

// Get protocol name
const char* getProtocolName(int proto);

// Toggle De Bruijn mode
void toggleDeBruijn();

// Check if De Bruijn mode
bool isDeBruijn();

// Get progress (0-100)
float getProgress();

// Get current code
uint32_t getCurrentCode();

// Get max code for current protocol
uint32_t getMaxCode();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace SubBrute

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ SPECTRUM ANALYZER
// ═══════════════════════════════════════════════════════════════════════════

namespace SubAnalyzer {

// Initialize spectrum analyzer
void setup();

// Main loop function
void loop();

// Start continuous scanning
void startScan();

// Stop scanning
void stopScan();

// Check if scanning
bool isScanning();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace SubAnalyzer

// ═══════════════════════════════════════════════════════════════════════════
// SUBGHZ UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Initialize CC1101 radio
void cc1101Init();

// Cleanup CC1101 and release resources
void cc1101Cleanup();

#endif // SUBGHZ_ATTACKS_H
