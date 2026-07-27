#ifndef NRF24_CONFIG_H
#define NRF24_CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD NRF24L01+PA+LNA Configuration
// 2.4GHz Radio for Mouse Jacker, BLE Spam, and Wireless Attacks
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include <SPI.h>
#include <RF24.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 OBJECT
// ═══════════════════════════════════════════════════════════════════════════

extern RF24 nrf24Radio;

// ═══════════════════════════════════════════════════════════════════════════
// PIN CONFIGURATION (from cyd_config.h)
// ═══════════════════════════════════════════════════════════════════════════
//
// NRF24L01+PA+LNA Wiring:
//   VCC  → 3.3V (add 10uF capacitor for stability!)
//   GND  → GND
//   SCK  → GPIO 18 (shared VSPI bus)
//   MOSI → GPIO 23 (shared VSPI bus)
//   MISO → GPIO 19 (shared VSPI bus)
//   CSN  → GPIO 4  (was RGB Red LED)
//   CE   → GPIO 16 (was RGB Green LED)
//   IRQ  → GPIO 17 (was RGB Blue LED) - optional
//
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
// RADIO MODES
// ═══════════════════════════════════════════════════════════════════════════

enum NRF24Mode {
    NRF24_MODE_OFF,
    NRF24_MODE_SCANNER,       // Channel scanner
    NRF24_MODE_SNIFFER,       // Packet sniffer
    NRF24_MODE_MOUSEJACKER,   // Mouse/keyboard injection
    NRF24_MODE_BLE_SPAM,      // BLE advertisement spam
    NRF24_MODE_JAMMER         // 2.4GHz jammer
};

extern NRF24Mode currentNRF24Mode;

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

// Initialize NRF24 radio - returns true if successful
bool nrf24Setup();

// Shutdown NRF24 (free up SPI bus for CC1101)
void nrf24Shutdown();

// Check if NRF24 is currently active
bool nrf24IsActive();

// ═══════════════════════════════════════════════════════════════════════════
// CHANNEL SCANNER
// ═══════════════════════════════════════════════════════════════════════════

// Scan all 126 channels for activity
void nrf24ScanChannels(uint8_t* results);

// Get channel with most activity
uint8_t nrf24GetBusiestChannel();

// ═══════════════════════════════════════════════════════════════════════════
// MOUSE JACKER
// ═══════════════════════════════════════════════════════════════════════════

// Mouse jacker target address (5 bytes)
extern uint8_t mouseJackerTarget[5];

// Mouse jacker target channel (from Sniffer or built-in scan)
extern uint8_t mouseJackerChannel;

// Mouse jacker device type (0xC1=keyboard, 0xC2=mouse, 0x0F=HID — from Sniffer handoff)
extern uint8_t mouseJackerDeviceType;

// Send mouse movement injection
bool nrf24InjectMouseMove(int8_t x, int8_t y);

// Send keystroke injection
bool nrf24InjectKeystroke(uint8_t key, uint8_t modifiers);

// Send string as keystrokes
bool nrf24InjectString(const char* str);

// ═══════════════════════════════════════════════════════════════════════════
// BLE SPAM (Using NRF24 for BLE advertisement)
// ═══════════════════════════════════════════════════════════════════════════

// BLE spam types
enum BLESpamType {
    BLE_SPAM_AIRTAG,          // Fake AirTag advertisements
    BLE_SPAM_ANDROID_FAST,    // Android Fast Pair
    BLE_SPAM_WINDOWS_SWIFT,   // Windows Swift Pair
    BLE_SPAM_SAMSUNG,         // Samsung device popups
    BLE_SPAM_RANDOM           // Random advertisements
};

// Start BLE spam
void nrf24StartBLESpam(BLESpamType type);

// Stop BLE spam
void nrf24StopBLESpam();

// Send single BLE advertisement
void nrf24SendBLEAdvert(uint8_t* data, uint8_t len);

// ═══════════════════════════════════════════════════════════════════════════
// 2.4GHz JAMMER
// ═══════════════════════════════════════════════════════════════════════════

// Start jamming on channel (or all channels if channel = 0xFF)
void nrf24StartJammer(uint8_t channel);

// Stop jamming
void nrf24StopJammer();

// ═══════════════════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Set radio channel (0-125)
void nrf24SetChannel(uint8_t channel);

// Set power level (RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX)
void nrf24SetPower(rf24_pa_dbm_e level);

// Set data rate (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS)
void nrf24SetDataRate(rf24_datarate_e rate);

// Get current channel
uint8_t nrf24GetChannel();

// Check if +PA+LNA version
bool nrf24IsPALNA();

// Print radio status to Serial
void nrf24PrintStatus();

// ═══════════════════════════════════════════════════════════════════════════
// PROMISCUOUS MODE (Travis Goodspeed technique)
// ═══════════════════════════════════════════════════════════════════════════

// Configure NRF24 for promiscuous RX (raw SPI, bypasses RF24 library)
// Uses SETUP_AW=0x00 trick for 2-byte illegal address width
// Returns true if radio was configured successfully
bool nrf24SetPromiscuous();

// Restore normal RF24 library control after promiscuous mode
void nrf24ExitPromiscuous();

// Read raw packet from RX FIFO in promiscuous mode
// Returns number of bytes read (0 if FIFO empty), up to maxLen
int nrf24ReadRawPacket(uint8_t* buf, uint8_t maxLen);

// ═══════════════════════════════════════════════════════════════════════════
// SPI BUS MANAGEMENT (Shared with CC1101)
// ═══════════════════════════════════════════════════════════════════════════

// Claim SPI bus for NRF24 (disables CC1101)
void nrf24ClaimSPI();

// Release SPI bus (allows CC1101 to use it)
void nrf24ReleaseSPI();

#endif // NRF24_CONFIG_H
