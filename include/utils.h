#ifndef UTILS_H
#define UTILS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Utility Functions
// Button handling, display helpers, common utilities
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "cyd_config.h"
#include "touch_buttons.h"
#include "nosifer_font.h"

// ═══════════════════════════════════════════════════════════════════════════
// EXTERNAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

extern TFT_eSPI tft;

// ═══════════════════════════════════════════════════════════════════════════
// BUTTON INPUT FUNCTIONS (Replaces PCF8574)
// ═══════════════════════════════════════════════════════════════════════════

// Initialize input system (touchscreen + BOOT button)
void initButtons();

// Update button states - call every loop iteration
void updateButtons();

// Check if specific direction pressed (for menu navigation)
bool isUpButtonPressed();
bool isDownButtonPressed();
bool isLeftButtonPressed();
bool isRightButtonPressed();

// Check action buttons
bool isSelectButtonPressed();
bool isBackButtonPressed();
bool isBootButtonPressed();

// Wait for any button press (blocking)
void waitForButtonPress();

// Wait for button release
void waitForButtonRelease();

// Check if any button is currently pressed
bool anyButtonActive();

// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Clear screen with background color
void clearScreen();

// Draw status bar at top of screen (GPS, battery, etc.)
void drawStatusBar();

// Draw title bar
void drawTitleBar(const char* title);

// Draw menu item (highlighted or normal)
void drawMenuItem(int y, const char* text, bool selected);

// Draw progress bar
void drawProgressBar(int x, int y, int width, int height, int percent, uint16_t color);

// Draw centered text
void drawCenteredText(int y, const char* text, uint16_t color, int size);

// Glitch text - chromatic aberration with Nosifer horror font
void drawGlitchText(int y, const char* text, const GFXfont* font);
void drawGlitchTitle(int y, const char* text);                        // 18pt big title
void drawGlitchStatus(int y, const char* text, uint16_t color);       // 12pt status

// ═══════════════════════════════════════════════════════════════════════════
// GPS STATUS HELPERS
// ═══════════════════════════════════════════════════════════════════════════

// Draw GPS indicator in status bar
void drawGPSIndicator(int x, int y);

// Get GPS status string for display
String getGPSStatusText();

// ═══════════════════════════════════════════════════════════════════════════
// STRING UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Truncate string to fit width
String truncateString(const String& str, int maxChars);

// Format frequency for display (e.g., "433.92 MHz")
String formatFrequency(float mhz);

// Format RSSI for display
String formatRSSI(int rssi);

// ═══════════════════════════════════════════════════════════════════════════
// TIMING UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Non-blocking delay with button check (returns true if button pressed)
bool delayWithButtonCheck(uint32_t ms);

// Get elapsed time string (e.g., "1m 23s")
String getElapsedTimeString(uint32_t startMillis);

// ═══════════════════════════════════════════════════════════════════════════
// EEPROM / STORAGE HELPERS
// ═══════════════════════════════════════════════════════════════════════════

// Save settings to EEPROM
void saveSettings();

// Load settings from EEPROM
void loadSettings();

// Apply BGR/RGB color order to display
void applyColorOrder();

// Apply color mode palette (0=Default, 1=Colorblind, 2=High Contrast)
void applyColorMode(uint8_t mode);

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG HELPERS
// ═══════════════════════════════════════════════════════════════════════════

// Print free heap memory
void printHeapStatus();

// Print system info
void printSystemInfo();

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 SAFE CHECK
// ═══════════════════════════════════════════════════════════════════════════

// Pre-check if CC1101 is connected before calling ELECHOUSE library.
// ELECHOUSE has blocking while(digitalRead(MISO)) loops that freeze
// forever if no CC1101 is on the bus. This does a raw SPI probe with
// 50ms timeout — returns true if CC1101 pulled MISO LOW (ready).
bool cc1101SafeCheck();

#endif // UTILS_H
