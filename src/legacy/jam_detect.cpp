// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Jamming Detection Module
// Defensive RF spectrum monitoring — WiFi + CC1101 + NRF24
// Created: 2026-03-02
// ═══════════════════════════════════════════════════════════════════════════
//
// WiFiGuardian:  Deauth/disassoc/beacon flood via promiscuous mode
// SubSentinel:   CC1101 33-freq RSSI baseline comparison (dual-core)
// GHzWatchdog:   NRF24 85-ch RPD occupancy analysis (dual-core)
// FullSpectrum:  All 3 radios time-shared, unified threat dashboard
//
// ═══════════════════════════════════════════════════════════════════════════

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include "jam_detect.h"
#include "cyd_config.h"
#include "shared.h"
#include "utils.h"
#include "touch_buttons.h"
#include "spi_manager.h"
#include "icon.h"
#include "skull_bg.h"
#include "nosifer_font.h"

extern TFT_eSPI tft;

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 PA MODULE CONTROL (E07-433M20S)
// Same wrappers as subghz_attacks.cpp — static to avoid link conflicts
// ═══════════════════════════════════════════════════════════════════════════

static void cc1101PaSetRx() {
    #if defined(CC1101_TX_EN) && defined(CC1101_RX_EN)
    if (cc1101_pa_module) {
        digitalWrite(CC1101_TX_EN, LOW);
        delayMicroseconds(2);
        digitalWrite(CC1101_RX_EN, HIGH);
    }
    #endif
    ELECHOUSE_cc1101.SetRx();
}

static void cc1101PaSetIdle() {
    ELECHOUSE_cc1101.setSidle();
    #if defined(CC1101_TX_EN) && defined(CC1101_RX_EN)
    if (cc1101_pa_module) {
        digitalWrite(CC1101_TX_EN, LOW);
        digitalWrite(CC1101_RX_EN, LOW);
    }
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════
// SHARED CONSTANTS & HELPERS
// ═══════════════════════════════════════════════════════════════════════════

// NRF24 register definitions (same as nrf24_attacks.cpp)
#define JD_NRF24_CONFIG    0x00
#define JD_NRF24_EN_AA     0x01
#define JD_NRF24_RF_CH     0x05
#define JD_NRF24_RF_SETUP  0x06
#define JD_NRF24_STATUS    0x07
#define JD_NRF24_RPD       0x09

// Jesse's custom 16x16 skull icons — cycle through all 8 (same as every module)
static const unsigned char* jdSkulls[] = {
    bitmap_icon_skull_wifi,
    bitmap_icon_skull_bluetooth,
    bitmap_icon_skull_jammer,
    bitmap_icon_skull_subghz,
    bitmap_icon_skull_ir,
    bitmap_icon_skull_tools,
    bitmap_icon_skull_setting,
    bitmap_icon_skull_about
};
#define JD_NUM_SKULLS 8

// Teal-to-hotpink color interpolation (matches Scanner/Deauther/BLE Spoofer)
static uint16_t tealToHotPink(float ratio) {
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    uint8_t r = (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
    return tft.color565(r, g, b);
}

// 8-phase skull wave color (matches Deauther/BLE Spoofer skull animations)
static uint16_t skullWaveColor(int skullFrame, int idx) {
    int phase = (skullFrame + idx) % 8;
    float ratio;
    if (phase < 4) {
        ratio = (float)phase / 3.0f;
    } else {
        ratio = (float)(phase - 4) / 3.0f;
        ratio = 1.0f - ratio;
    }
    return tealToHotPink(ratio);
}

// Threat level text
static const char* threatText(ThreatLevel level) {
    switch (level) {
        case THREAT_CALIBRATING: return "CALIBRATING";
        case THREAT_CLEAR:       return "CLEAR";
        case THREAT_SUSPICIOUS:  return "SUSPICIOUS";
        case THREAT_JAMMING:     return "JAMMED!";
        default:                 return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ICON BAR — 3 icons: Back (left), status area (center), Power icon (right)
// Matches PacketMonitor/Deauther icon bar with embedded status
// ═══════════════════════════════════════════════════════════════════════════

#define JD_ICON_SIZE 16

static void drawJdIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_GUNMETAL);
    tft.drawBitmap(10, ICON_BAR_Y, bitmap_icon_go_back, JD_ICON_SIZE, JD_ICON_SIZE, HALEHOUND_MAGENTA);
    tft.drawBitmap(SCALE_X(210), ICON_BAR_Y, bitmap_icon_power, JD_ICON_SIZE, JD_ICON_SIZE, HALEHOUND_MAGENTA);
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

// Update status text inside icon bar (like PacketMonitor)
static void drawJdIconBarStatus(const char* text) {
    tft.fillRect(SCALE_X(30), ICON_BAR_Y, SCALE_W(170), 16, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setTextSize(1);
    tft.setCursor(SCALE_X(35), ICON_BAR_Y + 4);
    tft.print(text);
}

static bool isJdBackTapped() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= ICON_BAR_TOUCH_TOP && ty <= ICON_BAR_TOUCH_BOTTOM && tx >= 10 && tx < 26) {
            consumeTouch();
            delay(150);
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION PROGRESS BAR — Teal→Hotpink gradient fill
// Matches HaleHound gradient bar style with magenta border
// ═══════════════════════════════════════════════════════════════════════════

static void drawCalibrationBar(int y, int percent) {
    int barX = 20;
    int barW = SCREEN_WIDTH - 40;
    int barH = 10;

    tft.drawRect(barX - 1, y - 1, barW + 2, barH + 2, HALEHOUND_MAGENTA);

    int fillW = (barW * percent) / 100;
    for (int x = 0; x < fillW; x++) {
        float t = (float)x / (float)barW;
        tft.drawFastVLine(barX + x, y, barH, tealToHotPink(t));
    }

    if (fillW < barW) {
        tft.fillRect(barX + fillW, y, barW - fillW, barH, TFT_BLACK);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// THREAT STATUS BAR — Teal→Hotpink gradient with text overlay
// CLEAR = teal dominant, SUSPICIOUS = mid gradient, JAMMING = hotpink pulsing
// ═══════════════════════════════════════════════════════════════════════════

static void drawThreatBar(int y, ThreatLevel level, bool pulseOn, int barH = 14) {
    int barX = 5;
    int barW = SCREEN_WIDTH - 10;

    float threatRatio;
    switch (level) {
        case THREAT_CLEAR:       threatRatio = 0.0f;  break;
        case THREAT_SUSPICIOUS:  threatRatio = 0.5f;  break;
        case THREAT_JAMMING:     threatRatio = 1.0f;  break;
        default:                 threatRatio = 0.0f;  break;
    }

    for (int x = 0; x < barW; x++) {
        float t = (float)x / (float)barW;
        float brightness = 0.6f + 0.4f * sinf(t * 3.14159f);

        if (level == THREAT_JAMMING && pulseOn) {
            brightness *= 1.0f;
        } else if (level == THREAT_JAMMING) {
            brightness *= 0.4f;
        }

        uint16_t base = tealToHotPink(threatRatio);
        uint8_t r = ((base >> 11) & 0x1F) * brightness;
        uint8_t g = ((base >> 5) & 0x3F) * brightness;
        uint8_t b = (base & 0x1F) * brightness;
        tft.drawFastVLine(barX + x, y, barH, (r << 11) | (g << 5) | b);
    }

    // White text overlay
    const char* txt = threatText(level);
    int tw = strlen(txt) * 6;
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor((SCREEN_WIDTH - tw) / 2, y + 3);
    tft.print(txt);
}

// ═══════════════════════════════════════════════════════════════════════════
// SKULL ROW ANIMATION — 8-phase wave, matches Deauther/BLE Spoofer
// ═══════════════════════════════════════════════════════════════════════════

static int jdSkullFrame = 0;

// Draw skull signal meter — 8 of Jesse's custom 16x16 skulls with wave animation
static void drawSkullMeter(int y, ThreatLevel level) {
    int numSkulls = JD_NUM_SKULLS;
    int spacing = (SCREEN_WIDTH - 20) / numSkulls;
    int startX = 10;

    for (int i = 0; i < numSkulls; i++) {
        int x = startX + (i * spacing);
        tft.fillRect(x, y, 16, 16, HALEHOUND_BLACK);

        uint16_t color;
        if (level == THREAT_CLEAR) {
            color = HALEHOUND_GUNMETAL;
        } else if (level == THREAT_SUSPICIOUS) {
            // Light up first 4 with wave
            if (i < 4) {
                color = skullWaveColor(jdSkullFrame, i);
            } else {
                color = HALEHOUND_GUNMETAL;
            }
        } else if (level == THREAT_JAMMING) {
            // All skulls lit with wave
            color = skullWaveColor(jdSkullFrame, i);
        } else {
            color = HALEHOUND_GUNMETAL;
        }

        tft.drawBitmap(x, y, jdSkulls[i], 16, 16, color);
    }

    // Status indicator next to skulls
    if (level >= THREAT_SUSPICIOUS) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setCursor(startX + numSkulls * spacing + 2, y + 4);
        tft.setTextSize(1);
        tft.print("!");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// HEAT MAP PALETTE — Matches PacketMonitor FFT + SubAnalyzer exactly
// Black → Deep Purple → Electric Blue → Hot Pink → White
// ═══════════════════════════════════════════════════════════════════════════

static uint16_t jdHeatPalette[128];
static bool jdPaletteInit = false;

static void initJdHeatPalette() {
    if (jdPaletteInit) return;
    for (int i = 0; i < 32; i++) {
        byte r = (i * 15) / 31;
        byte g = 0;
        byte b = (i * 20) / 31;
        jdHeatPalette[i] = (r << 11) | (g << 5) | b;
    }
    for (int i = 32; i < 64; i++) {
        int t = i - 32;
        byte r = 15 - (t * 15) / 31;
        byte g = (t * 31) / 31;
        byte b = 20 + (t * 11) / 31;
        jdHeatPalette[i] = (r << 11) | (g << 5) | b;
    }
    for (int i = 64; i < 96; i++) {
        int t = i - 64;
        byte r = (t * 31) / 31;
        byte g = 31 - (t * 31) / 31;
        byte b = 31;
        jdHeatPalette[i] = (r << 11) | (g << 5) | b;
    }
    for (int i = 96; i < 128; i++) {
        int t = i - 96;
        byte r = 31;
        byte g = (t * 63) / 31;
        byte b = 31;
        jdHeatPalette[i] = (r << 11) | (g << 5) | b;
    }
    jdPaletteInit = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// NRF24 RAW SPI — Local to this module (avoid name collision)
// ═══════════════════════════════════════════════════════════════════════════

static byte jdNrfGetReg(byte r) {
    byte c;
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer(r & 0x1F);
    c = SPI.transfer(0);
    digitalWrite(NRF24_CSN, HIGH);
    return c;
}

static void jdNrfSetReg(byte r, byte v) {
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer((r & 0x1F) | 0x20);
    SPI.transfer(v);
    digitalWrite(NRF24_CSN, HIGH);
}

static void jdNrfSetChannel(uint8_t ch) {
    jdNrfSetReg(JD_NRF24_RF_CH, ch);
}

static void jdNrfPowerUp() {
    jdNrfSetReg(JD_NRF24_CONFIG, jdNrfGetReg(JD_NRF24_CONFIG) | 0x02);
    delayMicroseconds(130);
}

static void jdNrfSetRX() {
    jdNrfSetReg(JD_NRF24_CONFIG, jdNrfGetReg(JD_NRF24_CONFIG) | 0x01);
    digitalWrite(NRF24_CE, HIGH);
    delayMicroseconds(100);
}

static bool jdNrfCarrierDetected() {
    return jdNrfGetReg(JD_NRF24_RPD) & 0x01;
}

static bool jdNrfInit() {
    pinMode(NRF24_CE, OUTPUT);
    pinMode(NRF24_CSN, OUTPUT);
    digitalWrite(NRF24_CE, LOW);
    digitalWrite(NRF24_CSN, HIGH);

    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    SPI.end();
    delay(10);
    SPI.begin(18, 19, 23);
    SPI.setDataMode(SPI_MODE0);
    SPI.setFrequency(4000000);
    SPI.setBitOrder(MSBFIRST);
    delay(10);

    bool found = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) delay(attempt * 100);
        digitalWrite(NRF24_CE, LOW);
        jdNrfPowerUp();
        jdNrfSetReg(JD_NRF24_EN_AA, 0x00);
        jdNrfSetReg(JD_NRF24_RF_SETUP, 0x0F);
        byte status = jdNrfGetReg(JD_NRF24_STATUS);
        if (status != 0x00 && status != 0xFF) {
            found = true;
            SPI.setFrequency(8000000);
            break;
        }
    }
    return found;
}

// Shared threat clear timeout
#define THREAT_CLEAR_TIMEOUT_MS 3000


// ═══════════════════════════════════════════════════════════════════════════
//
//  WIFI GUARDIAN — Deauth / Disassoc / Beacon Flood Detection
//
// ═══════════════════════════════════════════════════════════════════════════

namespace WiFiGuardian {

// Promiscuous counters (volatile — updated from interrupt callback)
static volatile uint32_t deauthCount = 0;
static volatile uint32_t disassocCount = 0;
static volatile uint32_t beaconCount = 0;
static volatile int32_t  lastRssi = 0;

// Per-second rate tracking
static uint32_t prevDeauth = 0;
static uint32_t prevDisassoc = 0;
static uint32_t prevBeacon = 0;
static uint32_t deauthRate = 0;
static uint32_t disassocRate = 0;
static uint32_t beaconRate = 0;
static unsigned long lastRateCalc = 0;

// Baseline (learned during calibration)
static uint32_t baseDeauthRate = 0;
static uint32_t baseDisassocRate = 0;
static uint32_t baseBeaconRate = 0;
static uint32_t calSamples = 0;
static uint32_t calDeauthSum = 0;
static uint32_t calDisassocSum = 0;
static uint32_t calBeaconSum = 0;

// Channel hopping
static uint8_t currentChannel = 1;
static const uint8_t hopChannels[] = {1, 6, 11};
static uint8_t hopIndex = 0;
static unsigned long lastHop = 0;
#define HOP_INTERVAL_MS 500

// Threat state
static ThreatLevel threat = THREAT_CALIBRATING;
static unsigned long calStartTime = 0;
#define WIFI_CAL_DURATION_MS 5000
static unsigned long threatClearTimer = 0;

// Thresholds
#define DEAUTH_SUSPICIOUS_DELTA  5
#define DEAUTH_JAMMING_DELTA    20
#define BEACON_SUSPICIOUS_MULT   3
#define BEACON_JAMMING_MULT     10

// Event log
#define MAX_EVENTS 6
struct JdEvent {
    unsigned long timestamp;
    char msg[28];
};
static JdEvent events[MAX_EVENTS];
static int eventHead = 0;
static int eventCount = 0;

// Display
static bool initialized = false;
static volatile bool exitRequested = false;
static unsigned long lastDraw = 0;
static unsigned long lastStatusDraw = 0;
static bool pulseState = false;
static unsigned long lastPulse = 0;

// Promiscuous callback — IRAM for interrupt safety
static void IRAM_ATTR wifiPromiscCB(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    lastRssi = pkt->rx_ctrl.rssi;

    uint8_t frameType = pkt->payload[0];
    if (frameType == 0xA0) deauthCount++;
    else if (frameType == 0xC0) disassocCount++;
    else if (frameType == 0x80) beaconCount++;
}

static void addEvent(const char* msg) {
    JdEvent& e = events[eventHead];
    e.timestamp = millis() / 1000;
    strncpy(e.msg, msg, 27);
    e.msg[27] = '\0';
    eventHead = (eventHead + 1) % MAX_EVENTS;
    if (eventCount < MAX_EVENTS) eventCount++;
}

// Rate bar — full-width teal-to-hotpink gradient fill (label drawn separately above)
static void drawRateBar(int y, uint32_t rate, uint32_t baseline) {
    bool elevated = (rate > baseline + DEAUTH_SUSPICIOUS_DELTA);

    int barX = 10;
    int barW = SCREEN_WIDTH - 20;
    int barH = 14;

    // Border — full overdraw, no pre-clear needed
    tft.drawRect(barX, y, barW, barH, elevated ? HALEHOUND_HOTPINK : HALEHOUND_MAGENTA);

    // Gradient fill
    uint32_t maxRate = max((uint32_t)50, baseline * 15);
    int fillW = constrain((int)((rate * (barW - 2)) / maxRate), 0, barW - 2);
    for (int x = 0; x < fillW; x++) {
        float t = (float)x / (float)(barW - 2);
        tft.drawFastVLine(barX + 1 + x, y + 1, barH - 2, tealToHotPink(t));
    }
    if (fillW < barW - 2) {
        tft.fillRect(barX + 1 + fillW, y + 1, barW - 2 - fillW, barH - 2, TFT_BLACK);
    }

    // Rate number overlaid on right side of bar
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu/s", (unsigned long)rate);
    int tw = strlen(buf) * 6;
    tft.setTextColor(elevated ? TFT_WHITE : HALEHOUND_MAGENTA);
    tft.setTextSize(1);
    tft.setCursor(barX + barW - tw - 4, y + 3);
    tft.print(buf);
}

void setup() {
    if (initialized) return;

    tft.fillScreen(HALEHOUND_BLACK);
    tft.drawBitmap(0, 0, skull_bg_bitmap, SKULL_BG_WIDTH, SKULL_BG_HEIGHT, 0x0041);
    drawStatusBar();
    drawJdIconBar();

    // Glitch title — 10pt Nosifer, matches all modules
    drawGlitchText(SCALE_Y(55), "GUARDIAN", &Nosifer_Regular10pt7b);

    // Reset counters
    deauthCount = 0; disassocCount = 0; beaconCount = 0;
    prevDeauth = 0; prevDisassoc = 0; prevBeacon = 0;
    deauthRate = 0; disassocRate = 0; beaconRate = 0;
    calSamples = 0; calDeauthSum = 0; calDisassocSum = 0; calBeaconSum = 0;
    baseDeauthRate = 0; baseDisassocRate = 0; baseBeaconRate = 0;
    eventHead = 0; eventCount = 0;
    jdSkullFrame = 0;

    threat = THREAT_CALIBRATING;
    exitRequested = false;
    hopIndex = 0;
    currentChannel = hopChannels[0];

    // Initialize WiFi in promiscuous mode
    WiFi.mode(WIFI_OFF);
    delay(50);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifiPromiscCB);

    calStartTime = millis();
    lastRateCalc = millis();
    lastHop = millis();
    lastDraw = 0;
    lastStatusDraw = 0;
    lastPulse = millis();
    pulseState = false;

    drawCenteredText(SCALE_Y(72), "Learning baseline...", HALEHOUND_MAGENTA, 1);

    initialized = true;
}

void loop() {
    if (!initialized) return;

    touchButtonsUpdate();
    if (isJdBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    unsigned long now = millis();

    // Channel hopping
    if (now - lastHop >= HOP_INTERVAL_MS) {
        hopIndex = (hopIndex + 1) % 3;
        currentChannel = hopChannels[hopIndex];
        esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
        lastHop = now;
    }

    // Per-second rate calculation
    if (now - lastRateCalc >= 1000) {
        deauthRate = deauthCount - prevDeauth;
        disassocRate = disassocCount - prevDisassoc;
        beaconRate = beaconCount - prevBeacon;
        prevDeauth = deauthCount;
        prevDisassoc = disassocCount;
        prevBeacon = beaconCount;
        lastRateCalc = now;

        // Calibration phase
        if (threat == THREAT_CALIBRATING) {
            calDeauthSum += deauthRate;
            calDisassocSum += disassocRate;
            calBeaconSum += beaconRate;
            calSamples++;

            int elapsed = now - calStartTime;
            int pct = constrain((elapsed * 100) / WIFI_CAL_DURATION_MS, 0, 100);
            drawCalibrationBar(SCALE_Y(90), pct);

            if (elapsed >= WIFI_CAL_DURATION_MS && calSamples > 0) {
                baseDeauthRate = calDeauthSum / calSamples;
                baseDisassocRate = calDisassocSum / calSamples;
                baseBeaconRate = max(calBeaconSum / calSamples, (uint32_t)1);
                threat = THREAT_CLEAR;
                threatClearTimer = now;
                addEvent("Baseline learned");

                // Redraw full UI
                tft.fillScreen(HALEHOUND_BLACK);
                drawStatusBar();
                drawJdIconBar();
                drawGlitchText(SCALE_Y(55), "GUARDIAN", &Nosifer_Regular10pt7b);
            }
        }

        // Monitoring — assess threat
        if (threat != THREAT_CALIBRATING) {
            ThreatLevel newThreat = THREAT_CLEAR;

            uint32_t attackRate = deauthRate + disassocRate;
            uint32_t attackBase = baseDeauthRate + baseDisassocRate;
            if (attackRate > attackBase + DEAUTH_JAMMING_DELTA) {
                newThreat = THREAT_JAMMING;
                char buf[28];
                snprintf(buf, sizeof(buf), "Deauth %lu/s ch%d", (unsigned long)attackRate, currentChannel);
                addEvent(buf);
            } else if (attackRate > attackBase + DEAUTH_SUSPICIOUS_DELTA) {
                newThreat = THREAT_SUSPICIOUS;
                char buf[28];
                snprintf(buf, sizeof(buf), "Deauth %lu/s ch%d", (unsigned long)attackRate, currentChannel);
                addEvent(buf);
            }

            if (beaconRate > baseBeaconRate * BEACON_JAMMING_MULT) {
                newThreat = THREAT_JAMMING;
                char buf[28];
                snprintf(buf, sizeof(buf), "Bcn flood %lu/s", (unsigned long)beaconRate);
                addEvent(buf);
            } else if (beaconRate > baseBeaconRate * BEACON_SUSPICIOUS_MULT && newThreat < THREAT_SUSPICIOUS) {
                newThreat = THREAT_SUSPICIOUS;
            }

            if (newThreat > THREAT_CLEAR) {
                threat = newThreat;
                threatClearTimer = now;
            } else if (threat > THREAT_CLEAR && now - threatClearTimer >= THREAT_CLEAR_TIMEOUT_MS) {
                threat = THREAT_CLEAR;
                addEvent("Threat cleared");
            }
        }
    }

    // 100ms animation cycle (skull wave + pulse)
    if (now - lastPulse >= 100) {
        pulseState = !pulseState;
        jdSkullFrame++;
        lastPulse = now;
    }

    // Draw at 100ms (10fps — matches Deauther/BLE Spoofer)
    if (now - lastDraw >= 100 && threat != THREAT_CALIBRATING) {
        lastDraw = now;

        int y = CONTENT_Y_START + 20;   // y=58, clear of Nosifer title

        // ── Threat bar — 20px tall for visual impact ──
        drawThreatBar(y, threat, pulseState, 20);
        y += 24;

        // ── DEAUTH ──
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(10, y);
        tft.print("DEAUTH");
        y += 12;
        drawRateBar(y, deauthRate, baseDeauthRate + 1);
        y += 18;

        // ── DISASSOC ──
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("DISASSOC");
        y += 12;
        drawRateBar(y, disassocRate, baseDisassocRate + 1);
        y += 18;

        // ── BEACON ──
        tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("BEACON");
        y += 12;
        drawRateBar(y, beaconRate, baseBeaconRate);
        y += 18;

        // ── Gradient divider ──
        for (int gx = 0; gx < SCREEN_WIDTH; gx++)
            tft.drawFastVLine(gx, y, 2, tealToHotPink((float)gx / SCREEN_WIDTH));
        y += 6;

        // ── Channel + RSSI ──
        tft.fillRect(5, y, SCREEN_WIDTH - 10, 10, TFT_BLACK);
        tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(10, y);
        tft.printf("Ch:%d  RSSI:%d dBm", currentChannel, (int)lastRssi);
        y += 16;

        // ── Gradient divider ──
        for (int gx = 0; gx < SCREEN_WIDTH; gx++)
            tft.drawFastVLine(gx, y, 2, tealToHotPink((float)gx / SCREEN_WIDTH));
        y += 6;

        // ── Event log (5 lines, per-line clear to avoid flicker) ──
        int idx = (eventHead - eventCount + MAX_EVENTS) % MAX_EVENTS;
        for (int i = 0; i < 5; i++) {
            tft.fillRect(5, y + i * 14, SCREEN_WIDTH - 10, 12, TFT_BLACK);
            if (i < eventCount) {
                JdEvent& e = events[(idx + eventCount - 1 - i) % MAX_EVENTS];
                tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
                tft.setCursor(5, y + i * 14);
                tft.printf("[%lus] %s", (unsigned long)e.timestamp, e.msg);
            }
        }
        y += 5 * 14;

        // ── Gradient divider ──
        for (int gx = 0; gx < SCREEN_WIDTH; gx++)
            tft.drawFastVLine(gx, y + 2, 2, tealToHotPink((float)gx / SCREEN_WIDTH));

        // ── Skull meter — anchored near bottom ──
        drawSkullMeter(SCREEN_HEIGHT - 30, threat);
    }

    // Icon bar status (200ms — matches SubAnalyzer)
    if (now - lastStatusDraw >= 200 && threat != THREAT_CALIBRATING) {
        char buf[24];
        snprintf(buf, sizeof(buf), "Ch:%d %s", currentChannel, threatText(threat));
        drawJdIconBarStatus(buf);
        lastStatusDraw = now;
    }
}

bool isExitRequested() { return exitRequested; }

void cleanup() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();
    WiFi.mode(WIFI_OFF);
    initialized = false;
    exitRequested = false;
}

}  // namespace WiFiGuardian


// ═══════════════════════════════════════════════════════════════════════════
//
//  SUBGHZ SENTINEL — CC1101 33-Freq RSSI Baseline Comparison
//
// ═══════════════════════════════════════════════════════════════════════════

namespace SubSentinel {

// CC1101 33-frequency list (same as SubAnalyzer)
static const float freqListMHz[] = {
    300.000, 302.000, 303.875, 304.250, 306.000, 310.000,
    313.000, 315.000, 318.000, 330.000, 345.000,
    390.000, 400.000, 418.000, 426.000, 430.000,
    433.075, 433.420, 433.920, 434.420, 434.775, 438.900,
    779.000, 868.000, 868.350, 900.000, 903.000,
    906.000, 910.000, 915.000, 920.000, 925.000, 928.000
};
static const int FREQ_COUNT = sizeof(freqListMHz) / sizeof(freqListMHz[0]);

// RSSI data
static volatile int8_t rssiNow[33];
static int8_t rssiBaseline[33];
static int8_t rssiDelta[33];
static uint8_t flagCount[33];

// Calibration — running average (saves DRAM)
static int16_t calSum[33];
static int calSweepCount = 0;
#define SS_CAL_DURATION_MS 8000

// Spectrum bars — LED VU meter matching SubAnalyzer
#define SS_BAR_WIDTH   6
#define SS_BAR_GAP     1
#define SS_BAR_STRIDE  (SS_BAR_WIDTH + SS_BAR_GAP)
#define SS_GRAPH_Y     SCALE_Y(68)
#define SS_GRAPH_H     SCALE_Y(150)
#define SS_SEG_H       4
#define SS_SEG_GAP     1
#define SS_SEG_STRIDE  (SS_SEG_H + SS_SEG_GAP)
#define SS_SEG_COUNT   (SS_GRAPH_H / SS_SEG_STRIDE)

static uint8_t prevBarSegs[33];
static uint8_t peakSegs[33];
static unsigned long peakHoldTime[33];

// Dual-core task
static TaskHandle_t ssScanHandle = NULL;
static volatile bool ssScanRunning = false;
static volatile bool ssScanDone = false;
static volatile bool ssFrameReady = false;

// State
static ThreatLevel threat = THREAT_CALIBRATING;
static unsigned long calStartTime = 0;
static unsigned long threatClearTimer = 0;
static bool initialized = false;
static volatile bool exitRequested = false;
static unsigned long lastDraw = 0;
static unsigned long lastStatusDraw = 0;
static bool pulseState = false;
static unsigned long lastPulse = 0;

// Thresholds — tuned for real-world jam detection (defensive tool = low false negatives)
#define SS_DELTA_FLAG_DB      6   // 6dB above baseline = flagged (was 10 — too conservative)
#define SS_SUSPICIOUS_FREQS   2   // 2+ freqs elevated = suspicious (was 3)
#define SS_JAMMING_FREQS      4   // 4+ freqs elevated = jamming (was 8)
#define SS_PERSIST_FRAMES     3   // 3 consecutive frames to flag (was 20 — way too slow)

static void scanAllFreqs() {
    for (int ch = 0; ch < FREQ_COUNT; ch++) {
        ELECHOUSE_cc1101.setMHZ(freqListMHz[ch]);
        cc1101PaSetRx();
        delayMicroseconds(450);
        int rssi1 = ELECHOUSE_cc1101.getRssi();
        delayMicroseconds(150);
        int rssi2 = ELECHOUSE_cc1101.getRssi();
        rssiNow[ch] = (int8_t)constrain(max(rssi1, rssi2), -128, 127);
    }
}

static void ssScanTask(void* param) {
    while (ssScanRunning) {
        if (ssFrameReady) { vTaskDelay(1); continue; }
        scanAllFreqs();
        ssFrameReady = true;
    }
    ssScanHandle = NULL;
    ssScanDone = true;
    vTaskDelete(NULL);
}

static void startScanTask() {
    if (ssScanHandle) return;
    ssScanRunning = true; ssScanDone = false; ssFrameReady = false;
    xTaskCreatePinnedToCore(ssScanTask, "SubSentinel", 4096, NULL, 1, &ssScanHandle, 0);
}

static void stopScanTask() {
    ssScanRunning = false;
    if (ssScanHandle) {
        unsigned long t0 = millis();
        while (!ssScanDone && (millis() - t0 < 500)) vTaskDelay(pdMS_TO_TICKS(10));
        ssScanHandle = NULL;
    }
}

// Draw deviation spectrum — heat palette by POSITION + peak-hold dots (matches SubAnalyzer)
static void drawSpectrumBars() {
    int graphX = (SCREEN_WIDTH - (FREQ_COUNT * SS_BAR_STRIDE - SS_BAR_GAP)) / 2;
    unsigned long now = millis();

    for (int ch = 0; ch < FREQ_COUNT; ch++) {
        int delta = max(0, (int)rssiDelta[ch]);
        int segs = constrain(map(delta, 0, 40, 0, SS_SEG_COUNT), 0, SS_SEG_COUNT);

        int x = graphX + ch * SS_BAR_STRIDE;
        int prev = prevBarSegs[ch];

        // Peak-hold tracking (matches SubAnalyzer exactly)
        if (segs > (int)peakSegs[ch]) {
            peakSegs[ch] = segs;
            peakHoldTime[ch] = now;
        } else if (now - peakHoldTime[ch] > 400) {
            if (peakSegs[ch] > 0) peakSegs[ch]--;
        }

        if (segs != prev) {
            if (segs > prev) {
                for (int s = prev; s < segs; s++) {
                    int sy = SS_GRAPH_Y + SS_GRAPH_H - (s + 1) * SS_SEG_STRIDE;
                    int palIdx = (s * 127) / max(1, SS_SEG_COUNT - 1);
                    tft.fillRect(x, sy, SS_BAR_WIDTH, SS_SEG_H, jdHeatPalette[palIdx]);
                }
            } else {
                for (int s = segs; s < prev; s++) {
                    int sy = SS_GRAPH_Y + SS_GRAPH_H - (s + 1) * SS_SEG_STRIDE;
                    tft.fillRect(x, sy, SS_BAR_WIDTH, SS_SEG_H, TFT_BLACK);
                }
            }
            prevBarSegs[ch] = segs;
        }

        // Peak-hold dot (hot pink, full segment height — matches SubAnalyzer)
        int peakSeg = (int)peakSegs[ch];
        if (peakSeg > segs && peakSeg > 0) {
            int peakY = SS_GRAPH_Y + SS_GRAPH_H - peakSeg * SS_SEG_STRIDE;
            // Erase segment above peak (in case it fell)
            if (peakSeg + 1 <= SS_SEG_COUNT) {
                int aboveY = SS_GRAPH_Y + SS_GRAPH_H - (peakSeg + 1) * SS_SEG_STRIDE;
                tft.fillRect(x, aboveY, SS_BAR_WIDTH, SS_SEG_H, TFT_BLACK);
            }
            tft.fillRect(x, peakY, SS_BAR_WIDTH, SS_SEG_H, HALEHOUND_HOTPINK);
        }
    }
}

void setup() {
    if (initialized) return;

    tft.fillScreen(HALEHOUND_BLACK);
    tft.drawBitmap(0, 0, skull_bg_bitmap, SKULL_BG_WIDTH, SKULL_BG_HEIGHT, 0x0041);
    drawStatusBar();
    drawJdIconBar();
    drawGlitchText(SCALE_Y(55), "SENTINEL", &Nosifer_Regular10pt7b);

    // Deselect other SPI
    pinMode(NRF24_CSN, OUTPUT); digitalWrite(NRF24_CSN, HIGH);
    pinMode(SD_CS, OUTPUT);     digitalWrite(SD_CS, HIGH);

    SPI.end();
    delay(5);

    // Safe check — ELECHOUSE has blocking MISO loops that freeze with no CC1101
    if (!cc1101SafeCheck()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setCursor(10, 100);
        tft.print("CC1101 NOT FOUND");
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(10, 115);
        tft.print("Check SPI wiring");
        delay(2000);
        exitRequested = true;
        initialized = true;
        return;
    }

    ELECHOUSE_cc1101.setSpiPin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, CC1101_CS);
    ELECHOUSE_cc1101.setGDO(CC1101_GDO0, CC1101_GDO2);

    if (!ELECHOUSE_cc1101.getCC1101()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setCursor(10, 100);
        tft.print("CC1101 NOT FOUND");
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(10, 115);
        tft.print("Check SPI wiring");
        delay(2000);
        exitRequested = true;
        initialized = true;
        return;
    }

    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(812.5);
    cc1101PaSetRx();

    memset((void*)rssiNow, 0, sizeof(rssiNow));
    memset(rssiBaseline, 0, sizeof(rssiBaseline));
    memset(rssiDelta, 0, sizeof(rssiDelta));
    memset(flagCount, 0, sizeof(flagCount));
    memset(prevBarSegs, 0, sizeof(prevBarSegs));
    memset(peakSegs, 0, sizeof(peakSegs));
    memset(peakHoldTime, 0, sizeof(peakHoldTime));
    memset(calSum, 0, sizeof(calSum));
    calSweepCount = 0;

    initJdHeatPalette();

    threat = THREAT_CALIBRATING;
    exitRequested = false;
    calStartTime = millis();
    threatClearTimer = millis();
    lastDraw = 0; lastStatusDraw = 0;
    lastPulse = millis(); pulseState = false;

    drawCenteredText(SCALE_Y(72), "Calibrating SubGHz...", HALEHOUND_MAGENTA, 1);

    initialized = true;
    startScanTask();
}

void loop() {
    if (!initialized) return;

    touchButtonsUpdate();
    if (isJdBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    unsigned long now = millis();

    if (now - lastPulse >= 100) {
        pulseState = !pulseState;
        jdSkullFrame++;
        lastPulse = now;
    }

    if (ssFrameReady) {
        if (threat == THREAT_CALIBRATING) {
            for (int ch = 0; ch < FREQ_COUNT; ch++) calSum[ch] += rssiNow[ch];
            calSweepCount++;

            int elapsed = now - calStartTime;
            int pct = constrain((elapsed * 100) / SS_CAL_DURATION_MS, 0, 100);
            drawCalibrationBar(SCALE_Y(90), pct);

            if (elapsed >= SS_CAL_DURATION_MS && calSweepCount >= 10) {
                for (int ch = 0; ch < FREQ_COUNT; ch++) {
                    rssiBaseline[ch] = (int8_t)(calSum[ch] / calSweepCount);
                }
                threat = THREAT_CLEAR;
                threatClearTimer = now;
                memset(flagCount, 0, sizeof(flagCount));

                // Redraw UI
                tft.fillScreen(HALEHOUND_BLACK);
                drawStatusBar();
                drawJdIconBar();
                drawGlitchText(SCALE_Y(55), "SENTINEL", &Nosifer_Regular10pt7b);

                // Draw freq labels below graph area (MAGENTA — matches SubAnalyzer)
                tft.setTextColor(HALEHOUND_MAGENTA);
                tft.setTextSize(1);
                int graphX = (SCREEN_WIDTH - (FREQ_COUNT * SS_BAR_STRIDE - SS_BAR_GAP)) / 2;
                tft.setCursor(graphX, SS_GRAPH_Y + SS_GRAPH_H + 2);
                tft.print("300");
                tft.setCursor(graphX + 16 * SS_BAR_STRIDE, SS_GRAPH_Y + SS_GRAPH_H + 2);
                tft.print("433");
                tft.setCursor(graphX + 29 * SS_BAR_STRIDE, SS_GRAPH_Y + SS_GRAPH_H + 2);
                tft.print("915");
            }
        } else {
            int flaggedCount = 0;
            for (int ch = 0; ch < FREQ_COUNT; ch++) {
                rssiDelta[ch] = rssiNow[ch] - rssiBaseline[ch];
                if (rssiDelta[ch] > SS_DELTA_FLAG_DB) {
                    if (flagCount[ch] < 255) flagCount[ch]++;
                } else {
                    if (flagCount[ch] > 0) flagCount[ch]--;
                }
                if (flagCount[ch] >= SS_PERSIST_FRAMES) flaggedCount++;
            }

            ThreatLevel newThreat = THREAT_CLEAR;
            if (flaggedCount >= SS_JAMMING_FREQS) newThreat = THREAT_JAMMING;
            else if (flaggedCount >= SS_SUSPICIOUS_FREQS) newThreat = THREAT_SUSPICIOUS;

            if (newThreat > THREAT_CLEAR) {
                threat = newThreat;
                threatClearTimer = now;
            } else if (threat > THREAT_CLEAR && now - threatClearTimer >= THREAT_CLEAR_TIMEOUT_MS) {
                threat = THREAT_CLEAR;
            }
        }
        ssFrameReady = false;
    }

    // Draw spectrum as fast as Core 0 produces frames
    if (threat != THREAT_CALIBRATING) {
        drawSpectrumBars();
    }

    // Status at 200ms (matches SubAnalyzer)
    if (now - lastStatusDraw >= 200 && threat != THREAT_CALIBRATING) {
        lastStatusDraw = now;

        int y = SS_GRAPH_Y + SS_GRAPH_H + 14;
        tft.fillRect(5, y, SCREEN_WIDTH - 10, 12, TFT_BLACK);
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setTextSize(1);
        tft.setCursor(5, y);
        int flagged = 0;
        for (int ch = 0; ch < FREQ_COUNT; ch++) {
            if (flagCount[ch] >= SS_PERSIST_FRAMES) flagged++;
        }
        tft.printf("Flagged:%d/%d  %s", flagged, FREQ_COUNT, threatText(threat));

        // Icon bar status
        char buf[24];
        snprintf(buf, sizeof(buf), "%d flagged %s", flagged, threatText(threat));
        drawJdIconBarStatus(buf);
    }
}

bool isExitRequested() { return exitRequested; }

void cleanup() {
    stopScanTask();
    cc1101PaSetIdle();
    spiDeselect();
    initialized = false;
    exitRequested = false;
}

}  // namespace SubSentinel


// ═══════════════════════════════════════════════════════════════════════════
//
//  2.4GHZ WATCHDOG — NRF24 RPD 85-Channel Occupancy Detection
//
// ═══════════════════════════════════════════════════════════════════════════

namespace GHzWatchdog {

#define GW_CHANNELS 85

// ── Scanner-style bar graph layout (EXACT match to 2.4GHz Scanner) ───────
#define GW_BAR_X       10
#define GW_BAR_Y       (CONTENT_Y_START + 4)   // Same as Scanner
#define GW_BAR_W       CONTENT_INNER_W          // 220px on 2.8"
#define GW_BAR_H       SCALE_Y(210)             // SAME height as Scanner

// WiFi channel positions (NRF24 channel numbers)
#define GW_WIFI_CH1    12
#define GW_WIFI_CH6    37
#define GW_WIFI_CH11   62
#define GW_WIFI_CH13   72

// Display-level smoothed values (0-125 range, Scanner-exact exponential decay)
// Written by Core 0 scan task, read by Core 1 display
static uint8_t gwDisplayLevel[GW_CHANNELS];

// Raw RPD per frame — binary 0/1 per channel (for detection logic)
static volatile uint8_t gwRpdRaw[GW_CHANNELS];

// Calibration
static uint32_t calAccum[GW_CHANNELS];   // RPD hit count during calibration
static int calSweepCount = 0;
#define GW_CAL_DURATION_MS 5000

// Baseline: average RPD detection rate per channel (0-100 = % of cal frames with RPD)
static uint8_t gwBaseline[GW_CHANNELS];

// Per-channel elevated counter for targeted jam detection
static uint8_t gwElevated[GW_CHANNELS];

// Detection thresholds — react to ANY jamming
#define GW_ELEVATED_FRAMES    3    // 3 consecutive elevated → channel flagged
#define GW_SUSPICIOUS_CHANS   1    // 1+ flagged channels → SUSPICIOUS
#define GW_JAMMING_CHANS      3    // 3+ flagged channels → JAMMING
#define GW_BROADBAND_JAM_PCT  50   // 50%+ raw active → instant JAMMING

// Dual-core
static TaskHandle_t gwScanHandle = NULL;
static volatile bool gwScanRunning = false;
static volatile bool gwScanDone = false;
static volatile bool gwFrameReady = false;

// State
static ThreatLevel threat = THREAT_CALIBRATING;
static unsigned long calStartTime = 0;
static unsigned long threatClearTimer = 0;
static bool initialized = false;
static volatile bool exitRequested = false;
static unsigned long lastStatusDraw = 0;
static bool pulseState = false;
static unsigned long lastPulse = 0;

// ── Scan task: SINGLE SWEEP, Scanner-exact smoothing, runs on Core 0 ─────
// This is the Scanner's scanDisplay() scan loop, running on a dedicated core.
// Single pass, 200us dwell, binary RPD, exponential smoothing directly into
// gwDisplayLevel — IDENTICAL animation behavior to the 2.4GHz Scanner.
static void gwScanTask(void* param) {
    while (gwScanRunning) {
        if (gwFrameReady) { vTaskDelay(1); continue; }

        for (int ch = 0; ch < GW_CHANNELS; ch++) {
            jdNrfSetChannel(ch);
            jdNrfSetRX();
            delayMicroseconds(200);
            int rpd = jdNrfCarrierDetected() ? 1 : 0;
            digitalWrite(NRF24_CE, LOW);

            // Scanner-EXACT smoothing: (old + rpd*125) / 2
            gwDisplayLevel[ch] = (gwDisplayLevel[ch] + rpd * 125) / 2;
            gwRpdRaw[ch] = rpd;
        }

        gwFrameReady = true;
    }
    gwScanHandle = NULL;
    gwScanDone = true;
    vTaskDelete(NULL);
}

static void startScanTask() {
    if (gwScanHandle) return;
    gwScanRunning = true; gwScanDone = false; gwFrameReady = false;
    xTaskCreatePinnedToCore(gwScanTask, "GHzWatchdog", 4096, NULL, 1, &gwScanHandle, 0);
}

static void stopScanTask() {
    gwScanRunning = false;
    if (gwScanHandle) {
        unsigned long t0 = millis();
        while (!gwScanDone && (millis() - t0 < 500)) vTaskDelay(pdMS_TO_TICKS(10));
        gwScanHandle = NULL;
    }
}

// ── Get bar color — teal to hot pink gradient by position (EXACT Scanner match) ──
static uint16_t gwBarColor(int height, int maxHeight) {
    float ratio = (float)height / (float)maxHeight;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    uint8_t r = (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
    return tft.color565(r, g, b);
}

// ── Draw scanner frame — axes, WiFi markers, channel labels, freq labels ──
static void drawGwFrame() {
    // Y-axis
    tft.drawFastVLine(GW_BAR_X - 2, GW_BAR_Y, GW_BAR_H, HALEHOUND_MAGENTA);

    // X-axis
    tft.drawFastHLine(GW_BAR_X, GW_BAR_Y + GW_BAR_H, GW_BAR_W, HALEHOUND_MAGENTA);

    // WiFi channel marker positions
    int x1  = GW_BAR_X + (GW_WIFI_CH1  * GW_BAR_W / GW_CHANNELS);
    int x6  = GW_BAR_X + (GW_WIFI_CH6  * GW_BAR_W / GW_CHANNELS);
    int x11 = GW_BAR_X + (GW_WIFI_CH11 * GW_BAR_W / GW_CHANNELS);
    int x13 = GW_BAR_X + (GW_WIFI_CH13 * GW_BAR_W / GW_CHANNELS);

    // Dashed vertical lines through graph
    for (int y = GW_BAR_Y; y < GW_BAR_Y + GW_BAR_H; y += 6) {
        tft.drawPixel(x1,  y, HALEHOUND_HOTPINK);
        tft.drawPixel(x6,  y, HALEHOUND_HOTPINK);
        tft.drawPixel(x11, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x13, y, HALEHOUND_VIOLET);
    }

    // Channel labels above graph
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x1 - 2, GW_BAR_Y - 10);
    tft.print("1");
    tft.setCursor(x6 - 2, GW_BAR_Y - 10);
    tft.print("6");
    tft.setCursor(x11 - 6, GW_BAR_Y - 10);
    tft.print("11");
    tft.setTextColor(HALEHOUND_VIOLET, TFT_BLACK);
    tft.setCursor(x13 - 6, GW_BAR_Y - 10);
    tft.print("13");

    // Frequency labels below X-axis
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.setCursor(GW_BAR_X - 5, GW_BAR_Y + GW_BAR_H + 4);
    tft.print("2400");
    tft.setCursor(GW_BAR_X + GW_BAR_W / 2 - 12, GW_BAR_Y + GW_BAR_H + 4);
    tft.print("2442");
    tft.setCursor(GW_BAR_X + GW_BAR_W - 28, GW_BAR_Y + GW_BAR_H + 4);
    tft.print("2484");

    // Hotpink divider below labels
    tft.drawFastHLine(0, GW_BAR_Y + GW_BAR_H + 16, SCREEN_WIDTH, HALEHOUND_HOTPINK);
}

// ── Draw bar graph — 85 gradient bars + skulls + threat (EXACT Scanner style) ──
static void drawGwBarGraph() {
    // Clear bar area
    tft.fillRect(GW_BAR_X, GW_BAR_Y, GW_BAR_W, GW_BAR_H, TFT_BLACK);

    // Redraw WiFi channel markers
    int x1  = GW_BAR_X + (GW_WIFI_CH1  * GW_BAR_W / GW_CHANNELS);
    int x6  = GW_BAR_X + (GW_WIFI_CH6  * GW_BAR_W / GW_CHANNELS);
    int x11 = GW_BAR_X + (GW_WIFI_CH11 * GW_BAR_W / GW_CHANNELS);
    int x13 = GW_BAR_X + (GW_WIFI_CH13 * GW_BAR_W / GW_CHANNELS);

    for (int y = GW_BAR_Y; y < GW_BAR_Y + GW_BAR_H; y += 6) {
        tft.drawPixel(x1,  y, HALEHOUND_HOTPINK);
        tft.drawPixel(x6,  y, HALEHOUND_HOTPINK);
        tft.drawPixel(x11, y, HALEHOUND_HOTPINK);
        tft.drawPixel(x13, y, HALEHOUND_VIOLET);
    }

    // Track peak for display
    int peakChannel = 0;
    uint8_t peakLevel = 0;

    // Draw 85 bars — 2px wide, bottom-aligned, teal→hotpink gradient
    for (int ch = 0; ch < GW_CHANNELS; ch++) {
        uint8_t level = gwDisplayLevel[ch];

        if (level > peakLevel) {
            peakLevel = level;
            peakChannel = ch;
        }

        if (level > 0) {
            int x = GW_BAR_X + (ch * GW_BAR_W / GW_CHANNELS);
            int barH = (level * GW_BAR_H) / 125;
            if (barH > GW_BAR_H) barH = GW_BAR_H;
            if (barH < 4 && level > 0) barH = 4;   // Minimum 4px visible

            int barY = GW_BAR_Y + GW_BAR_H - barH;

            // Gradient bar — row by row from bottom, color by position
            for (int py = 0; py < barH; py++) {
                uint16_t color = gwBarColor(py, GW_BAR_H);
                tft.drawFastHLine(x, barY + barH - 1 - py, 2, color);
            }
        }
    }

    // ── Status area below graph (NO full-area clear — targeted overdraws only) ──
    int statusY = GW_BAR_Y + GW_BAR_H + 6;

    // Hotpink divider
    tft.drawFastHLine(0, statusY - 2, SCREEN_WIDTH, HALEHOUND_HOTPINK);

    // Threat bar — fully overdraws its own region, no pre-clear needed
    drawThreatBar(statusY, threat, pulseState);
    statusY += 16;

    // Peak frequency + active channel count
    int peakFreq = 2400 + peakChannel;
    int activeCount = 0;
    for (int ch = 0; ch < GW_CHANNELS; ch++) {
        if (gwRpdRaw[ch] > 0) activeCount++;
    }
    int activePct = (activeCount * 100) / GW_CHANNELS;

    // Clear just the text line, then overdraw
    tft.fillRect(5, statusY + 2, SCREEN_WIDTH - 10, 10, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(5, statusY + 2);
    tft.print("PEAK:");
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    tft.printf(" %d", peakFreq);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.print("MHz");

    // Active channels on right side
    tft.setTextColor(HALEHOUND_HOTPINK, TFT_BLACK);
    tft.setCursor(SCALE_X(140), statusY + 2);
    tft.printf("%d/%d", activeCount, GW_CHANNELS);
    tft.setTextColor(HALEHOUND_MAGENTA, TFT_BLACK);
    tft.printf(" (%d%%)", activePct);

    statusY += 14;

    // Skull signal meter — 8 skulls with wave animation (exact Scanner match)
    int skullStartX = 10;
    int skullSpacing = SCALE_X(28);

    // Skulls lit = peak level mapped to 0-8
    int litSkulls = (peakLevel * 8) / 4;
    if (litSkulls > JD_NUM_SKULLS) litSkulls = JD_NUM_SKULLS;

    for (int i = 0; i < JD_NUM_SKULLS; i++) {
        int sx = skullStartX + (i * skullSpacing);
        tft.fillRect(sx, statusY, 16, 16, HALEHOUND_BLACK);

        if (i < litSkulls && peakLevel > 0) {
            tft.drawBitmap(sx, statusY, jdSkulls[i], 16, 16,
                skullWaveColor(jdSkullFrame, i));
        } else {
            tft.drawBitmap(sx, statusY, jdSkulls[i], 16, 16, HALEHOUND_GUNMETAL);
        }
    }

    // Skull frame advance EVERY draw call (Scanner-exact animation speed)
    jdSkullFrame++;

    // Clear just the percentage area, then overdraw
    int pctX = skullStartX + (JD_NUM_SKULLS * skullSpacing) + 2;
    tft.fillRect(pctX, statusY + 4, 30, 10, TFT_BLACK);
    int pct = (peakLevel * 100) / 125;
    if (pct > 100) pct = 100;
    tft.setTextColor(HALEHOUND_BRIGHT, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(pctX, statusY + 4);
    tft.printf("%d%%", pct);
}

void setup() {
    if (initialized) return;

    tft.fillScreen(HALEHOUND_BLACK);
    tft.drawBitmap(0, 0, skull_bg_bitmap, SKULL_BG_WIDTH, SKULL_BG_HEIGHT, 0x0041);
    drawStatusBar();
    drawJdIconBar();
    drawGlitchText(SCALE_Y(55), "WATCHDOG", &Nosifer_Regular10pt7b);

    if (!jdNrfInit()) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setTextSize(2);
        drawCenteredText(100, "NRF24 NOT FOUND", HALEHOUND_HOTPINK, 2);
        tft.setTextSize(1);
        drawCenteredText(130, "Check SPI wiring", HALEHOUND_MAGENTA, 1);
        delay(2000);
        exitRequested = true;
        initialized = true;
        return;
    }

    memset(gwDisplayLevel, 0, sizeof(gwDisplayLevel));
    memset((void*)gwRpdRaw, 0, sizeof(gwRpdRaw));
    memset(calAccum, 0, sizeof(calAccum));
    memset(gwBaseline, 0, sizeof(gwBaseline));
    memset(gwElevated, 0, sizeof(gwElevated));
    calSweepCount = 0;

    threat = THREAT_CALIBRATING;
    exitRequested = false;
    calStartTime = millis();
    threatClearTimer = millis();
    lastStatusDraw = 0;
    lastPulse = millis(); pulseState = false;

    drawCenteredText(SCALE_Y(72), "Calibrating 2.4GHz...", HALEHOUND_MAGENTA, 1);

    initialized = true;
    startScanTask();
}

void loop() {
    if (!initialized) return;

    touchButtonsUpdate();
    if (isJdBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    unsigned long now = millis();

    // Pulse for threat bar (Scanner doesn't use this — but threat bar does)
    if (now - lastPulse >= 300) {
        pulseState = !pulseState;
        lastPulse = now;
    }

    if (gwFrameReady) {
        if (threat == THREAT_CALIBRATING) {
            // Accumulate RPD hits per channel for baseline
            for (int ch = 0; ch < GW_CHANNELS; ch++) calAccum[ch] += gwRpdRaw[ch];
            calSweepCount++;

            int elapsed = now - calStartTime;
            int pct = constrain((elapsed * 100) / GW_CAL_DURATION_MS, 0, 100);
            drawCalibrationBar(SCALE_Y(90), pct);

            if (elapsed >= GW_CAL_DURATION_MS && calSweepCount >= 10) {
                // Baseline = % of calibration frames where RPD triggered (0-100)
                for (int ch = 0; ch < GW_CHANNELS; ch++) {
                    gwBaseline[ch] = (calAccum[ch] * 100) / calSweepCount;
                }
                threat = THREAT_CLEAR;
                threatClearTimer = now;
                memset(gwElevated, 0, sizeof(gwElevated));

                // Redraw to Scanner-style layout
                tft.fillScreen(HALEHOUND_BLACK);
                drawStatusBar();
                drawJdIconBar();
                drawGwFrame();
            }
        } else {
            // ── JAM DETECTION — reacts to ANY 2.4GHz jamming ──
            // Per-channel baseline comparison: if a channel that was quiet
            // is now consistently active, it's being jammed.
            int flaggedChans = 0;
            int rawActive = 0;

            for (int ch = 0; ch < GW_CHANNELS; ch++) {
                if (gwRpdRaw[ch]) rawActive++;

                // Channel is elevated if RPD=1 and baseline was quiet (<30%)
                if (gwRpdRaw[ch] && gwBaseline[ch] < 30) {
                    if (gwElevated[ch] < 255) gwElevated[ch]++;
                } else {
                    if (gwElevated[ch] > 0) gwElevated[ch]--;
                }

                if (gwElevated[ch] >= GW_ELEVATED_FRAMES) flaggedChans++;
            }

            int rawPct = (rawActive * 100) / GW_CHANNELS;

            // Assess threat: per-channel OR broadband
            ThreatLevel newThreat = THREAT_CLEAR;
            if (flaggedChans >= GW_JAMMING_CHANS || rawPct >= GW_BROADBAND_JAM_PCT) {
                newThreat = THREAT_JAMMING;
            } else if (flaggedChans >= GW_SUSPICIOUS_CHANS) {
                newThreat = THREAT_SUSPICIOUS;
            }

            if (newThreat > THREAT_CLEAR) {
                threat = newThreat;
                threatClearTimer = now;
            } else if (threat > THREAT_CLEAR && now - threatClearTimer >= THREAT_CLEAR_TIMEOUT_MS) {
                threat = THREAT_CLEAR;
            }
        }
        gwFrameReady = false;
    }

    // Draw bar graph every loop — Scanner draws every frame, so do we
    if (threat != THREAT_CALIBRATING) {
        drawGwBarGraph();
    }

    // Icon bar status 200ms
    if (now - lastStatusDraw >= 200 && threat != THREAT_CALIBRATING) {
        lastStatusDraw = now;
        int activeCount = 0;
        for (int ch = 0; ch < GW_CHANNELS; ch++) {
            if (gwRpdRaw[ch]) activeCount++;
        }
        char buf[24];
        snprintf(buf, sizeof(buf), "%d%% %s", (activeCount * 100) / GW_CHANNELS, threatText(threat));
        drawJdIconBarStatus(buf);
    }
}

bool isExitRequested() { return exitRequested; }

void cleanup() {
    stopScanTask();
    digitalWrite(NRF24_CE, LOW);
    spiDeselect();
    initialized = false;
    exitRequested = false;
}

}  // namespace GHzWatchdog


// ═══════════════════════════════════════════════════════════════════════════
//
//  FULL SPECTRUM — All 3 Radios, Unified Threat Dashboard
//
// ═══════════════════════════════════════════════════════════════════════════

namespace FullSpectrum {

// Per-band threat levels
static ThreatLevel wifiThreat = THREAT_CALIBRATING;
static ThreatLevel subThreat = THREAT_CALIBRATING;
static ThreatLevel ghzThreat = THREAT_CALIBRATING;

// WiFi promiscuous counters
static volatile uint32_t fsDeauthCount = 0;
static volatile uint32_t fsDisassocCount = 0;
static volatile uint32_t fsBeaconCount = 0;
static uint32_t fsPrevDeauth = 0, fsPrevDisassoc = 0, fsPrevBeacon = 0;
static uint32_t fsDeauthRate = 0, fsDisassocRate = 0, fsBeaconRate = 0;
static uint32_t fsBaseDeauth = 0, fsBaseDisassoc = 0, fsBaseBeacon = 1;

// CC1101 data
static int8_t fsSubRssi[33];
static int8_t fsSubBaseline[33];
static uint8_t fsSubFlags[33];

// NRF24 data
static uint8_t fsNrfRpd[85];
static uint8_t fsNrfBaseline[85];

// Time-sharing
static bool radioIsCC1101 = true;
static unsigned long lastRadioSwap = 0;
#define RADIO_SWAP_MS 1000

// Calibration
enum CalPhase { CAL_WIFI = 0, CAL_SUBGHZ, CAL_NRF24, CAL_DONE };
static CalPhase calPhase = CAL_WIFI;
static unsigned long calPhaseStart = 0;
static uint32_t wifiCalSamples = 0, wifiCalDeauthSum = 0, wifiCalBeaconSum = 0;
static int16_t fsSubCalSum[33];
static int subCalCount = 0;
static uint32_t nrfCalAccum[85];
static int nrfCalCount = 0;

// WiFi channel hop
static uint8_t fsHopIdx = 0;
static unsigned long fsLastHop = 0;
static const uint8_t fsHopCh[] = {1, 6, 11};

// Timeline (60s rolling, 1 sample/sec)
#define TIMELINE_LEN 60
static ThreatLevel wifiTimeline[TIMELINE_LEN];
static ThreatLevel subTimeline[TIMELINE_LEN];
static ThreatLevel ghzTimeline[TIMELINE_LEN];
static int timelineIdx = 0;
static unsigned long lastTimelineSample = 0;

// Dual-core
static TaskHandle_t fsScanHandle = NULL;
static volatile bool fsScanRunning = false;
static volatile bool fsScanDone = false;
static volatile bool fsFrameReady = false;
static volatile bool fsScanCC1101 = true;

// State
static ThreatLevel threat = THREAT_CALIBRATING;
static unsigned long threatClearTimer = 0;
static bool initialized = false;
static volatile bool exitRequested = false;
static unsigned long lastDraw = 0;
static unsigned long lastRateCalc = 0;
static unsigned long lastStatusDraw = 0;
static bool pulseState = false;
static unsigned long lastPulse = 0;

static void IRAM_ATTR fsPromiscCB(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t ft = pkt->payload[0];
    if (ft == 0xA0) fsDeauthCount++;
    else if (ft == 0xC0) fsDisassocCount++;
    else if (ft == 0x80) fsBeaconCount++;
}

static void fsScanSubGHz() {
    digitalWrite(NRF24_CSN, HIGH);
    digitalWrite(NRF24_CE, LOW);
    SPI.end(); delay(2);
    if (!cc1101SafeCheck()) return;
    ELECHOUSE_cc1101.setSpiPin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, CC1101_CS);
    ELECHOUSE_cc1101.setGDO(CC1101_GDO0, CC1101_GDO2);
    if (!ELECHOUSE_cc1101.getCC1101()) return;
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(812.5);

    static const float freqs[] = {
        300.000, 302.000, 303.875, 304.250, 306.000, 310.000,
        313.000, 315.000, 318.000, 330.000, 345.000,
        390.000, 400.000, 418.000, 426.000, 430.000,
        433.075, 433.420, 433.920, 434.420, 434.775, 438.900,
        779.000, 868.000, 868.350, 900.000, 903.000,
        906.000, 910.000, 915.000, 920.000, 925.000, 928.000
    };
    for (int ch = 0; ch < 33 && fsScanRunning; ch++) {
        ELECHOUSE_cc1101.setMHZ(freqs[ch]);
        cc1101PaSetRx();
        delayMicroseconds(450);
        int r1 = ELECHOUSE_cc1101.getRssi();
        delayMicroseconds(150);
        int r2 = ELECHOUSE_cc1101.getRssi();
        fsSubRssi[ch] = (int8_t)constrain(max(r1, r2), -128, 127);
    }
    cc1101PaSetIdle();
}

static void fsScanNRF24() {
    digitalWrite(CC1101_CS, HIGH);
    SPI.end(); delay(2);
    SPI.begin(18, 19, 23);
    SPI.setDataMode(SPI_MODE0);
    SPI.setFrequency(8000000);
    SPI.setBitOrder(MSBFIRST);
    delay(2);
    pinMode(NRF24_CE, OUTPUT);
    pinMode(NRF24_CSN, OUTPUT);
    digitalWrite(NRF24_CE, LOW);
    digitalWrite(NRF24_CSN, HIGH);
    jdNrfPowerUp();
    jdNrfSetReg(JD_NRF24_EN_AA, 0x00);
    jdNrfSetReg(JD_NRF24_RF_SETUP, 0x0F);

    for (int ch = 0; ch < 85 && fsScanRunning; ch++) {
        jdNrfSetChannel(ch);
        jdNrfSetRX();
        delayMicroseconds(200);
        fsNrfRpd[ch] = jdNrfCarrierDetected() ? 1 : 0;
        digitalWrite(NRF24_CE, LOW);
    }
}

static void fsScanTask(void* param) {
    while (fsScanRunning) {
        if (fsFrameReady) { vTaskDelay(1); continue; }
        if (fsScanCC1101) fsScanSubGHz(); else fsScanNRF24();
        fsFrameReady = true;
    }
    fsScanHandle = NULL;
    fsScanDone = true;
    vTaskDelete(NULL);
}

static void startScanTask() {
    if (fsScanHandle) return;
    fsScanRunning = true; fsScanDone = false; fsFrameReady = false;
    xTaskCreatePinnedToCore(fsScanTask, "FullSpectrum", 8192, NULL, 1, &fsScanHandle, 0);
}

static void stopScanTask() {
    fsScanRunning = false;
    if (fsScanHandle) {
        unsigned long t0 = millis();
        while (!fsScanDone && (millis() - t0 < 500)) vTaskDelay(pdMS_TO_TICKS(10));
        fsScanHandle = NULL;
    }
}

// Band indicator — filled circle with magenta outline, threat-colored fill, bold status
static void drawBandIndicator(int y, const char* label, ThreatLevel level) {
    tft.fillRect(5, y, SCREEN_WIDTH - 10, 16, TFT_BLACK);
    tft.setTextSize(1);

    // Threat color mapping
    float ratio = 0.0f;
    if (level == THREAT_SUSPICIOUS) ratio = 0.5f;
    else if (level == THREAT_JAMMING) ratio = 1.0f;
    uint16_t fillColor = (level == THREAT_CLEAR) ? tealToHotPink(0.0f) : tealToHotPink(ratio);

    // Bigger dot (r=6) with magenta outline ring
    int dotX = 14;
    int dotY = y + 7;
    tft.fillCircle(dotX, dotY, 6, fillColor);
    tft.drawCircle(dotX, dotY, 6, HALEHOUND_MAGENTA);
    if (level == THREAT_JAMMING) {
        tft.drawCircle(dotX, dotY, 7, HALEHOUND_HOTPINK);
    }

    // Band name in magenta
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(26, y + 3);
    tft.print(label);

    // Status text — right-aligned, color matches threat
    const char* txt = threatText(level);
    int tw = strlen(txt) * 6;
    uint16_t txtColor;
    if (level == THREAT_CLEAR) txtColor = tealToHotPink(0.0f);
    else if (level == THREAT_SUSPICIOUS) txtColor = tealToHotPink(0.5f);
    else txtColor = HALEHOUND_HOTPINK;
    tft.setTextColor(txtColor);
    tft.setCursor(SCREEN_WIDTH - tw - 8, y + 3);
    tft.print(txt);
}

// Mini timeline — 10px tall gradient bars, full brightness, magenta border
static void drawMiniTimeline(int y, ThreatLevel* timeline) {
    int totalW = SCREEN_WIDTH - 20;  // 10px margin each side
    int startX = 10;
    int barH = 10;
    int segW = max(1, totalW / TIMELINE_LEN);

    // Magenta border around entire timeline
    tft.drawRect(startX - 1, y - 1, segW * TIMELINE_LEN + 2, barH + 2, HALEHOUND_MAGENTA);

    for (int i = 0; i < TIMELINE_LEN; i++) {
        int idx = (timelineIdx + i) % TIMELINE_LEN;
        uint16_t color;
        switch (timeline[idx]) {
            case THREAT_CLEAR:       color = tealToHotPink(0.0f); break;
            case THREAT_SUSPICIOUS:  color = tealToHotPink(0.5f); break;
            case THREAT_JAMMING:     color = tealToHotPink(1.0f); break;
            default:                 color = HALEHOUND_GUNMETAL;  break;
        }
        // Full brightness — no dimming. The border provides visual separation.
        tft.fillRect(startX + i * segW, y, segW, barH, color);
    }
}

void setup() {
    if (initialized) return;

    tft.fillScreen(HALEHOUND_BLACK);
    tft.drawBitmap(0, 0, skull_bg_bitmap, SKULL_BG_WIDTH, SKULL_BG_HEIGHT, 0x0041);
    drawStatusBar();
    drawJdIconBar();
    drawGlitchText(SCALE_Y(55), "SPECTRUM", &Nosifer_Regular10pt7b);

    // Reset all
    fsDeauthCount = 0; fsDisassocCount = 0; fsBeaconCount = 0;
    fsPrevDeauth = 0; fsPrevDisassoc = 0; fsPrevBeacon = 0;
    fsDeauthRate = 0; fsDisassocRate = 0; fsBeaconRate = 0;
    fsBaseDeauth = 0; fsBaseDisassoc = 0; fsBaseBeacon = 1;
    memset(fsSubRssi, 0, sizeof(fsSubRssi));
    memset(fsSubBaseline, 0, sizeof(fsSubBaseline));
    memset(fsSubFlags, 0, sizeof(fsSubFlags));
    memset(fsNrfRpd, 0, sizeof(fsNrfRpd));
    memset(fsNrfBaseline, 0, sizeof(fsNrfBaseline));
    memset(fsSubCalSum, 0, sizeof(fsSubCalSum));
    memset(nrfCalAccum, 0, sizeof(nrfCalAccum));
    subCalCount = 0; nrfCalCount = 0;
    wifiCalSamples = 0; wifiCalDeauthSum = 0; wifiCalBeaconSum = 0;
    memset(wifiTimeline, 0, sizeof(wifiTimeline));
    memset(subTimeline, 0, sizeof(subTimeline));
    memset(ghzTimeline, 0, sizeof(ghzTimeline));
    timelineIdx = 0;
    jdSkullFrame = 0;

    wifiThreat = THREAT_CALIBRATING;
    subThreat = THREAT_CALIBRATING;
    ghzThreat = THREAT_CALIBRATING;
    threat = THREAT_CALIBRATING;
    exitRequested = false;
    calPhase = CAL_WIFI;
    calPhaseStart = millis();
    lastRadioSwap = millis();
    fsHopIdx = 0; fsLastHop = millis();
    lastDraw = 0; lastRateCalc = millis();
    lastStatusDraw = 0; lastPulse = millis();
    lastTimelineSample = millis();
    pulseState = false; threatClearTimer = millis();

    // Start WiFi promiscuous
    WiFi.mode(WIFI_OFF);
    delay(50);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(fsPromiscCB);

    drawCenteredText(SCALE_Y(72), "Calibrating all bands...", HALEHOUND_MAGENTA, 1);

    initialized = true;
    fsScanCC1101 = true;
    startScanTask();
}

void loop() {
    if (!initialized) return;

    touchButtonsUpdate();
    if (isJdBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        exitRequested = true;
        return;
    }

    unsigned long now = millis();

    if (now - lastPulse >= 100) {
        pulseState = !pulseState;
        jdSkullFrame++;
        lastPulse = now;
    }

    // WiFi channel hop
    if (now - fsLastHop >= 500) {
        fsHopIdx = (fsHopIdx + 1) % 3;
        esp_wifi_set_channel(fsHopCh[fsHopIdx], WIFI_SECOND_CHAN_NONE);
        fsLastHop = now;
    }

    // WiFi rate calc
    if (now - lastRateCalc >= 1000) {
        fsDeauthRate = fsDeauthCount - fsPrevDeauth;
        fsDisassocRate = fsDisassocCount - fsPrevDisassoc;
        fsBeaconRate = fsBeaconCount - fsPrevBeacon;
        fsPrevDeauth = fsDeauthCount;
        fsPrevDisassoc = fsDisassocCount;
        fsPrevBeacon = fsBeaconCount;
        lastRateCalc = now;
    }

    // Calibration phases
    if (calPhase != CAL_DONE) {
        int totalDuration = 10000;
        int elapsed = now - calPhaseStart;

        if (calPhase == CAL_WIFI) {
            wifiCalDeauthSum += fsDeauthRate;
            wifiCalBeaconSum += fsBeaconRate;
            wifiCalSamples++;
            if (elapsed >= 5000) {
                fsBaseDeauth = (wifiCalSamples > 0) ? wifiCalDeauthSum / wifiCalSamples : 0;
                fsBaseBeacon = max((wifiCalSamples > 0) ? wifiCalBeaconSum / wifiCalSamples : (uint32_t)1, (uint32_t)1);
                wifiThreat = THREAT_CLEAR;
                calPhase = CAL_SUBGHZ;
                fsScanCC1101 = true;
            }
        } else if (calPhase == CAL_SUBGHZ) {
            if (fsFrameReady && fsScanCC1101) {
                for (int ch = 0; ch < 33; ch++) fsSubCalSum[ch] += fsSubRssi[ch];
                subCalCount++;
                fsFrameReady = false;
            }
            if (elapsed >= 8000) {
                for (int ch = 0; ch < 33; ch++) {
                    fsSubBaseline[ch] = (subCalCount > 0) ? (int8_t)(fsSubCalSum[ch] / subCalCount) : -85;
                }
                subThreat = THREAT_CLEAR;
                calPhase = CAL_NRF24;
                fsScanCC1101 = false;
            }
        } else if (calPhase == CAL_NRF24) {
            if (fsFrameReady && !fsScanCC1101) {
                for (int ch = 0; ch < 85; ch++) nrfCalAccum[ch] += fsNrfRpd[ch];
                nrfCalCount++;
                fsFrameReady = false;
            }
            if (elapsed >= 10000) {
                for (int ch = 0; ch < 85; ch++) {
                    fsNrfBaseline[ch] = (nrfCalCount > 0) ? nrfCalAccum[ch] / nrfCalCount : 0;
                }
                ghzThreat = THREAT_CLEAR;
                calPhase = CAL_DONE;
                threat = THREAT_CLEAR;
                threatClearTimer = now;

                // Redraw
                tft.fillScreen(HALEHOUND_BLACK);
                drawStatusBar();
                drawJdIconBar();
                drawGlitchText(SCALE_Y(55), "SPECTRUM", &Nosifer_Regular10pt7b);
            }
        }

        int overallPct = constrain(((now - calPhaseStart) * 100) / totalDuration, 0, 100);
        drawCalibrationBar(SCALE_Y(90), overallPct);
        return;
    }

    // Radio time-sharing
    if (now - lastRadioSwap >= RADIO_SWAP_MS) {
        fsScanCC1101 = !fsScanCC1101;
        lastRadioSwap = now;
    }

    // Process scan frame — research-based thresholds
    // Key insight: normal WiFi occupies NRF24 ch 1-23, 26-48, 51-73
    // GAP channels (24-25, 49-50, 74-84) should be QUIET in normal environments
    // Jamming fills ALL channels including gaps — that's the diagnostic
    if (fsFrameReady) {
        if (fsScanCC1101) {
            // SubGHz: 6dB delta, 3 persist frames, matches standalone SubSentinel
            int flagged = 0;
            for (int ch = 0; ch < 33; ch++) {
                int delta = fsSubRssi[ch] - fsSubBaseline[ch];
                if (delta > SS_DELTA_FLAG_DB) { if (fsSubFlags[ch] < 255) fsSubFlags[ch]++; }
                else { if (fsSubFlags[ch] > 0) fsSubFlags[ch]--; }
                if (fsSubFlags[ch] >= SS_PERSIST_FRAMES) flagged++;
            }
            if (flagged >= SS_JAMMING_FREQS) subThreat = THREAT_JAMMING;
            else if (flagged >= SS_SUSPICIOUS_FREQS) subThreat = THREAT_SUSPICIOUS;
            else subThreat = THREAT_CLEAR;
        } else {
            // 2.4GHz: check gap channels + broadband percentage
            // Gap channels = between WiFi bands (should be silent normally)
            // ch 24-25 (gap between WiFi 1 & 6), ch 49-50 (gap between 6 & 11),
            // ch 74-84 (above WiFi 11/13)
            int active = 0;
            int gapActive = 0;
            for (int ch = 0; ch < 85; ch++) {
                if (fsNrfRpd[ch] > 0) {
                    active++;
                    // Count gap channel activity
                    if ((ch >= 24 && ch <= 25) || (ch >= 49 && ch <= 50) ||
                        ch >= 74) {
                        gapActive++;
                    }
                }
            }
            int pct = (active * 100) / 85;
            // Gap channels active = abnormal (not normal WiFi)
            // Broadband 70%+ = wall of RF
            if (gapActive >= 6 || pct >= 80) ghzThreat = THREAT_JAMMING;
            else if (gapActive >= 3 || pct >= 60) ghzThreat = THREAT_SUSPICIOUS;
            else ghzThreat = THREAT_CLEAR;
        }
        fsFrameReady = false;
    }

    // WiFi threat — deauth/disassoc/beacon flood detection
    {
        uint32_t attackRate = fsDeauthRate + fsDisassocRate;
        if (attackRate > fsBaseDeauth + 15) wifiThreat = THREAT_JAMMING;
        else if (attackRate > fsBaseDeauth + 5) wifiThreat = THREAT_SUSPICIOUS;
        else if (fsBeaconRate > fsBaseBeacon * 8) wifiThreat = THREAT_JAMMING;
        else if (fsBeaconRate > fsBaseBeacon * 3) wifiThreat = THREAT_SUSPICIOUS;
        else wifiThreat = THREAT_CLEAR;
    }

    // Unified = worst
    ThreatLevel worst = THREAT_CLEAR;
    if (wifiThreat > worst) worst = wifiThreat;
    if (subThreat > worst) worst = subThreat;
    if (ghzThreat > worst) worst = ghzThreat;

    if (worst > THREAT_CLEAR) { threat = worst; threatClearTimer = now; }
    else if (threat > THREAT_CLEAR && now - threatClearTimer >= 3000) { threat = THREAT_CLEAR; }

    // Timeline (1/sec)
    if (now - lastTimelineSample >= 1000) {
        wifiTimeline[timelineIdx] = wifiThreat;
        subTimeline[timelineIdx] = subThreat;
        ghzTimeline[timelineIdx] = ghzThreat;
        timelineIdx = (timelineIdx + 1) % TIMELINE_LEN;
        lastTimelineSample = now;
    }

    // Draw at 100ms (10fps)
    if (now - lastDraw >= 100) {
        lastDraw = now;

        int y = CONTENT_Y_START + 18;

        // Unified threat bar (full width gradient)
        drawThreatBar(y, threat, pulseState);
        y += 17;

        // Gradient separator (teal→hotpink, 2px tall)
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            tft.drawFastVLine(x, y, 2, tealToHotPink((float)x / SCREEN_WIDTH));
        }
        y += 4;

        // Per-band indicators (bigger dots, threat-colored text)
        drawBandIndicator(y, "WiFi 2.4GHz", wifiThreat);
        y += 18;
        drawBandIndicator(y, "Sub 300-928", subThreat);
        y += 18;
        drawBandIndicator(y, "NRF 2.4GHz", ghzThreat);
        y += 20;

        // Gradient separator
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            tft.drawFastVLine(x, y, 2, tealToHotPink((float)x / SCREEN_WIDTH));
        }
        y += 4;

        // Timelines — magenta labels, 10px tall bars with borders
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setTextSize(1);
        tft.fillRect(5, y, 30, 10, TFT_BLACK);
        tft.setCursor(5, y + 1); tft.print("WiFi");
        drawMiniTimeline(y + 12, wifiTimeline);
        y += 24;
        tft.fillRect(5, y, 30, 10, TFT_BLACK);
        tft.setCursor(5, y + 1); tft.print("Sub");
        drawMiniTimeline(y + 12, subTimeline);
        y += 24;
        tft.fillRect(5, y, 30, 10, TFT_BLACK);
        tft.setCursor(5, y + 1); tft.print("2.4G");
        drawMiniTimeline(y + 12, ghzTimeline);
        y += 26;

        // Skull row
        drawSkullMeter(y, threat);
        y += 20;

        // Stats line — magenta with rates
        tft.fillRect(5, y, SCREEN_WIDTH - 10, 12, TFT_BLACK);
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setCursor(5, y);
        tft.printf("Deauth:%lu/s Bcn:%lu/s", (unsigned long)fsDeauthRate, (unsigned long)fsBeaconRate);
    }

    // Icon bar status 200ms
    if (now - lastStatusDraw >= 200) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%s", threatText(threat));
        drawJdIconBarStatus(buf);
        lastStatusDraw = now;
    }
}

bool isExitRequested() { return exitRequested; }

void cleanup() {
    stopScanTask();
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();
    WiFi.mode(WIFI_OFF);
    cc1101PaSetIdle();
    digitalWrite(NRF24_CE, LOW);
    spiDeselect();
    initialized = false;
    exitRequested = false;
}

}  // namespace FullSpectrum
