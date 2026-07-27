#ifndef BLUETOOTH_ATTACKS_H
#define BLUETOOTH_ATTACKS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Bluetooth Attack Modules
// COMPLETE HALEHOUND BLUETOOTH ATTACK SUITE - Adapted for CYD Touch Interface
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════
//
// ATTACK MODULES:
// ┌──────────────────────────────────────────────────────────────────────────┐
// │ BleSpoofer - Apple/Beats Device BLE Advertisement Spam                  │
// │   - 17 device payloads (AirPods, Beats variants)                        │
// │   - Triggers popup notifications on nearby iPhones/iPads               │
// │   - Randomized MAC addresses for stealth                                │
// │   - Multiple advertisement types                                        │
// ├──────────────────────────────────────────────────────────────────────────┤
// │ BleBeacon - iBeacon / Eddystone Beacon Transmitter                      │
// │   - Standards-compliant iBeacon broadcasts (Apple format)               │
// │   - Eddystone-URL and Eddystone-UID beacon modes                       │
// │   - Retail preset cloning (Apple Store, Starbucks)                      │
// │   - Geo-fence flood testing mode                                        │
// ├──────────────────────────────────────────────────────────────────────────┤
// │ BleScan - Bluetooth Low Energy Device Scanner                           │
// │   - Active BLE scanning                                                 │
// │   - Device list with details view                                       │
// │   - Shows name, MAC, RSSI, TX power                                     │
// │   - Service UUID and manufacturer data                                  │
// ├──────────────────────────────────────────────────────────────────────────┤
// │ BleJammer - 2.4GHz RF Jammer (REQUIRES NRF24 HARDWARE)                  │
// │   - NOTE: CYD does not have NRF24 radios                                │
// │   - This feature requires external NRF24L01 modules                     │
// │   - Will display hardware not available message                         │
// └──────────────────────────────────────────────────────────────────────────┘
//
// SUPPORTED DEVICES (BleSpoofer):
// 1.  AirPods              10. Beats Studio Buds
// 2.  AirPods Pro          11. Beats Flex
// 3.  AirPods Max          12. BeatsX
// 4.  AirPods Gen 2        13. Beats Solo 3
// 5.  AirPods Gen 3        14. Beats Studio 3
// 6.  AirPods Pro Gen 2    15. Beats Studio Pro
// 7.  PowerBeats           16. Beats Fit Pro
// 8.  PowerBeats Pro       17. Beats Studio Buds Plus
// 9.  Beats Solo Pro
//
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEScan.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// BLE SPOOFER - Multi-Platform BLE Spam Engine
// Targets: Apple iOS, Google Android, Samsung Galaxy, Microsoft Windows
// Modes: Apple Popup, Sour Apple, Fast Pair, Samsung Buds, Samsung Watch,
//        Swift Pair, CHAOS (rotates all platforms)
// ═══════════════════════════════════════════════════════════════════════════

namespace BleSpoofer {

// Initialize BLE spoofer and draw UI
void setup();

// Main loop - touch handling, broadcast engine, display
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup BLE resources
void cleanup();

// Set initial spam mode before setup() (used by SourApple redirect)
void setInitialMode(int mode);

}  // namespace BleSpoofer

// ═══════════════════════════════════════════════════════════════════════════
// BLE BEACON - iBeacon / Eddystone Beacon Transmitter
// Modes: Random iBeacon, Apple Store, Starbucks, Eddystone URL,
//        Eddystone UID, Geo-Fence (rapid preset cycling)
// ═══════════════════════════════════════════════════════════════════════════

namespace BleBeacon {

// Initialize BLE Beacon and draw UI
void setup();

// Main loop - touch handling, broadcast engine, display
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup BLE resources
void cleanup();

}  // namespace BleBeacon

// ═══════════════════════════════════════════════════════════════════════════
// BLE SCAN - Bluetooth Device Scanner
// ═══════════════════════════════════════════════════════════════════════════

namespace BleScan {

// Initialize BLE scanner
void setup();

// Main loop function
void loop();

// Start a new scan
void startScan();

// Get number of devices found
int getDeviceCount();

// Check if currently scanning
bool isScanning();

// Check if in detail view
bool isDetailView();

// Check if user requested exit
bool isExitRequested();

// Cleanup and release BLE resources
void cleanup();

}  // namespace BleScan

// ═══════════════════════════════════════════════════════════════════════════
// BLE SNIFFER - Continuous BLE Advertisement Monitor
// Passive scanning with live device tracking, vendor ID, device type
// classification, RSSI history, manufacturer data parsing, and filter modes
// ═══════════════════════════════════════════════════════════════════════════

namespace BleSniffer {

// Initialize sniffer
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup
void cleanup();

}  // namespace BleSniffer

// ═══════════════════════════════════════════════════════════════════════════
// WHISPERPAIR - CVE-2025-36911 Fast Pair Exploit Chain
// Phase 1: Discovery + GATT Service Probe
// Phase 2: Multi-Strategy KBP Exploit + BR/EDR Extraction + Account Key Injection
// ═══════════════════════════════════════════════════════════════════════════

namespace WhisperPair {

// Initialize WhisperPair scanner
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup and release BLE resources
void cleanup();

}  // namespace WhisperPair

// ═══════════════════════════════════════════════════════════════════════════
// AIRTAG DETECT - Apple FindMy Tracker Detection
// Scans for AirTags, FindMy accessories, and compatible trackers
// Detects Apple manufacturer data (0x4C) with FindMy type (0x12)
// ═══════════════════════════════════════════════════════════════════════════

namespace AirTagDetect {

// Initialize detector and start scanning
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup and release BLE resources
void cleanup();

}  // namespace AirTagDetect

// ═══════════════════════════════════════════════════════════════════════════
// LUNATIC FRINGE - Multi-Platform Tracker Detection
// Scans for Google FMDN (0xFEAA), Samsung SmartTag (0xFD5A), Tile (0xFEED),
// Chipolo (0xFE33), and Apple AirTag (0x004C) trackers via BLE advertisements
// ═══════════════════════════════════════════════════════════════════════════

namespace LunaticFringe {

// Initialize detector and start scanning
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup and release BLE resources
void cleanup();

}  // namespace LunaticFringe

// ═══════════════════════════════════════════════════════════════════════════
// PHANTOM FLOOD - Apple FindMy Offline Finding BLE Flood
// Broadcasts fake FindMy OF advertisements with random 28-byte public keys
// Each key appears as a unique tracker — floods locationd on nearby iPhones
// ═══════════════════════════════════════════════════════════════════════════

namespace PhantomFlood {

// Initialize BLE and draw UI
void setup();

// Main loop - touch handling, display update
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup BLE resources
void cleanup();

}  // namespace PhantomFlood

// ═══════════════════════════════════════════════════════════════════════════
// AIRTAG REPLAY - FindMy Advertisement Sniff & Replay
// Captures real AirTag/FindMy BLE advertisements (MAC + full 31-byte payload)
// then replays them — ESP32 impersonates the real AirTag identity
// ═══════════════════════════════════════════════════════════════════════════

namespace AirTagReplay {

// Initialize scanner and draw UI
void setup();

// Main loop - scan/replay phases, touch handling, display
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup BLE resources and stop replay task
void cleanup();

}  // namespace AirTagReplay

// ═══════════════════════════════════════════════════════════════════════════
// FIND YOU - Stealth AirTag Clone (P-224 EC Key Rotation)
// Broadcasts real FindMy OF advertisements using pre-generated P-224 keypairs
// FindYou namespace removed — feature was not useful

// ═══════════════════════════════════════════════════════════════════════════
// BLE DUCKY - BLE HID Keyboard Injection
// ESP32 acts as BLE HID keyboard, injects keystrokes into paired devices
// Uses T-vK/ESP32-BLE-Keyboard library
// ═══════════════════════════════════════════════════════════════════════════

namespace BleDucky {

// Initialize BLE HID keyboard and draw UI
void setup();

// Main loop - pairing status, payload selection, injection
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup BLE resources
void cleanup();

}  // namespace BleDucky

// ═══════════════════════════════════════════════════════════════════════════
// BLE JAMMER - 2.4GHz BLE Jammer using NRF24L01+PA+LNA
// Continuous carrier wave jamming on Bluetooth 2.402-2.480 GHz
// Modes: ALL CHANNELS | ADV ONLY (Ch37/38/39) | DATA ONLY
// ═══════════════════════════════════════════════════════════════════════════

namespace BleJammer {

// Initialize jammer and NRF24 hardware
void setup();

// Main loop function
void loop();

// Check if user requested exit
bool isExitRequested();

// Cleanup and power down NRF24
void cleanup();

}  // namespace BleJammer

// ═══════════════════════════════════════════════════════════════════════════
// BLE UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Initialize BLE stack
void bleInit();

// Cleanup BLE and release resources
void bleCleanup();

#endif // BLUETOOTH_ATTACKS_H
