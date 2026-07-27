#ifndef CYD_CONFIG_H
#define CYD_CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD Master Pin Configuration
// Supports: ESP32-2432S028 (2.8"), QDtech E32R28T (2.8"), QDtech E32R35T (3.5")
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════
//
// BOARD SELECTION: Set by PlatformIO build flags (-DCYD_E32R28T=1, -DCYD_E32R35T=1)
// Default: CYD_28 when no flag specified (backwards compatible)
// ═══════════════════════════════════════════════════════════════════════════

// E32R28T inherits CYD_28 display/touch — same ILI9341, XPT2046, 240x320
#ifdef CYD_E32R28T
  #ifndef CYD_28
    #define CYD_28
  #endif
#endif

// E32R35T inherits CYD_35 display — same ST7796, 320x480
// Touch is XPT2046 resistive on shared HSPI (handled by TFT_eSPI TOUCH_CS)
#ifdef CYD_E32R35T
  #ifndef CYD_35
    #define CYD_35
  #endif
#endif

#ifdef NM_CYD_C5
#ifndef CYD_C5
#define CYD_C5 1
#endif
#endif


#if !defined(CYD_28) && !defined(CYD_35) && !defined(CYD_C5)
  #define CYD_28    // Default: ESP32-2432S028 - 2.8" 320x240 ILI9341
#endif

// ═══════════════════════════════════════════════════════════════════════════
// FIRMWARE VERSION — single source of truth
// ═══════════════════════════════════════════════════════════════════════════

#define FW_VERSION "v3.4.0"

#ifdef CYD_35
  #define FW_EDITION   "E32R35T Edition"
  #define FW_DEVICE    "HaleHound-E32R35T"
#elif defined(NMRF_HAT)
  #define FW_EDITION   "CYD-HAT Edition"
  #define FW_DEVICE    "HaleHound-CYD-HAT"
#elif defined(CYD_E32R28T)
  #define FW_EDITION   "E32R28T Edition"
  #define FW_DEVICE    "HaleHound-E32R28T"
#elif defined(CYD_C5)
  #define FW_EDITION   "NM-CYD-C5 Edition"
  #define FW_DEVICE    "HaleHound-CYD-C5"
#else
  #define FW_EDITION   "CYD Edition"
  #define FW_DEVICE    "HaleHound-CYD"
#endif

#define FW_FULL_VERSION FW_VERSION " " FW_EDITION

// ═══════════════════════════════════════════════════════════════════════════
// BOARD-SPECIFIC SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

#ifdef CYD_28
  #undef  CYD_BOARD_NAME
  #define CYD_BOARD_NAME    "HaleHound-CYD 2.8\""
  #define CYD_SCREEN_WIDTH  240
  #define CYD_SCREEN_HEIGHT 320
  #define CYD_TFT_BL        21    // Backlight on GPIO21
#endif

#ifdef CYD_E32R28T
  // Override board name for E32R28T variant
  #undef  CYD_BOARD_NAME
  #define CYD_BOARD_NAME    "HaleHound-E32R28T 2.8\""
#endif

#ifdef CYD_35
  #undef  CYD_BOARD_NAME
  #define CYD_BOARD_NAME    "HaleHound-E32R35T 3.5\""
  #define CYD_SCREEN_WIDTH  320
  #define CYD_SCREEN_HEIGHT 480
  #define CYD_TFT_BL        27    // Backlight on GPIO27
#endif

#ifdef CYD_C5
  #undef  CYD_BOARD_NAME
  #define CYD_BOARD_NAME    "NM-CYD-C5 2.8\""
  #define CYD_SCREEN_WIDTH  240
  #define CYD_SCREEN_HEIGHT 320
  #define CYD_TFT_BL        25    // Backlight on GPIO25
#endif

#ifdef CYD_C5
#define CYD_TFT_MISO    2
#define CYD_TFT_MOSI    7
#define CYD_TFT_SCLK    6
#define CYD_TFT_CS      23
#define CYD_TFT_DC      24
#define CYD_TFT_RST     -1    // Connected to EN reset
#else
// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY PINS (HSPI) - Same for both boards
// ═══════════════════════════════════════════════════════════════════════════
#define CYD_TFT_MISO    12
#define CYD_TFT_MOSI    13
#define CYD_TFT_SCLK    14
#define CYD_TFT_CS      15
#define CYD_TFT_DC       2
#define CYD_TFT_RST     -1    // Connected to EN reset
#endif

// ═══════════════════════════════════════════════════════════════════════════
// TOUCH CONTROLLER (XPT2046)
// ═══════════════════════════════════════════════════════════════════════════
#ifdef CYD_C5
#define CYD_TOUCH_CS    1
#define CYD_TOUCH_IRQ   -1    // Same on both boards
#else
#define CYD_TOUCH_CS    33
#define CYD_TOUCH_IRQ   36    // Same on both boards
#endif

#ifdef CYD_28
  // 2.8" has SEPARATE touch SPI bus
  #define CYD_TOUCH_MOSI  32
  #define CYD_TOUCH_MISO  39
  #define CYD_TOUCH_CLK   25
#endif

#ifdef CYD_35
  // E32R35T: XPT2046 resistive touch on SHARED HSPI with LCD
  // Handled by TFT_eSPI built-in driver (TOUCH_CS in User_Setup.h)
  // No separate touch SPI — CLK/MOSI/MISO shared on GPIO 14/13/12
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SHARED SPI BUS (VSPI) - SD Card, CC1101, and NRF24L01 share this bus
// Each device has its own CS pin - only one active at a time!
// ═══════════════════════════════════════════════════════════════════════════
#ifdef CYD_C5
#define VSPI_SCK        6    // Shared SPI Clock
#define VSPI_MOSI       7    // Shared SPI MOSI
#define VSPI_MISO       2    // Shared SPI MISO
#else
#define VSPI_SCK        18    // Shared SPI Clock
#define VSPI_MOSI       23    // Shared SPI MOSI
#define VSPI_MISO       19    // Shared SPI MISO
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SD CARD (Shares VSPI bus with radios)
// ═══════════════════════════════════════════════════════════════════════════
//
// WIRING DIAGRAM:
// ┌─────────────┐      ┌─────────────┐
// │  MicroSD    │      │     CYD     │
// │   Card      │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │
// │ GND ────────┼──────┤ GND         │
// │ CLK ────────┼──────┤ GPIO 18     │ (shared VSPI)
// │ MOSI ───────┼──────┤ GPIO 23     │ (shared VSPI)
// │ MISO ───────┼──────┤ GPIO 19     │ (shared VSPI)
// │ CS ─────────┼──────┤ GPIO 5      │ (SD exclusive)
// └─────────────┘      └─────────────┘
//
// NOTE: SD card on CYD is built-in (microSD slot on back of board)
// Perfect for storing DuckyScript payloads!
//
// ═══════════════════════════════════════════════════════════════════════════
#ifdef CYD_C5
#define SD_CS            10    // SD Card Chip Select (built-in slot)
#else
#define SD_CS            5     // SD Card Chip Select (GPIO5)
#endif
#define SD_SCK          VSPI_SCK
#define SD_MOSI         VSPI_MOSI
#define SD_MISO         VSPI_MISO

// Legacy aliases for radio code
#define RADIO_SPI_SCK   VSPI_SCK
#define RADIO_SPI_MOSI  VSPI_MOSI
#define RADIO_SPI_MISO  VSPI_MISO

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 SubGHz RADIO (Red HW-863 Module / E07-433M20S PA Module)
// ═══════════════════════════════════════════════════════════════════════════
//
// WIRING DIAGRAM (ESP32-2432S028 — HW-863):
// ┌─────────────┐      ┌─────────────┐
// │   CC1101    │      │     CYD     │
// │   HW-863    │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │
// │ MOSI ───────┼──────┤ GPIO 23     │
// │ MISO ───────┼──────┤ GPIO 19     │
// │ CS ─────────┼──────┤ GPIO 27     │ (CN1 connector)
// │ GDO0 ───────┼──────┤ GPIO 22     │ (P3 connector) TX to radio
// │ GDO2 ───────┼──────┤ GPIO 35     │ (P3 connector) RX from radio
// └─────────────┘      └─────────────┘
//
// WIRING DIAGRAM (E32R28T — E07-433M20S PA Module):
// ┌─────────────┐      ┌─────────────┐
// │ E07-433M20S │      │   E32R28T   │
// │  CC1101+PA  │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │ (4P SPI connector)
// │ MOSI ───────┼──────┤ GPIO 23     │ (4P SPI connector)
// │ MISO ───────┼──────┤ GPIO 19     │ (4P SPI connector)
// │ CS ─────────┼──────┤ GPIO 27     │ (4P SPI connector CS)
// │ GDO0 ───────┼──────┤ GPIO 22     │ (RGB Red pad) TX to radio
// │ GDO2 ───────┼──────┤ GPIO 35     │ (Expand connector) RX from radio
// │ TX_EN ──────┼──────┤ GPIO 4      │ (Amp enable pad) PA transmit
// │ RX_EN ──────┼──────┤ GPIO 0      │ (BOOT pad) PA receive
// └─────────────┘      └─────────────┘
//
// WIRING DIAGRAM (E32R35T — E07-433M20S PA Module):
// ┌─────────────┐      ┌─────────────┐
// │ E07-433M20S │      │   E32R35T   │
// │  CC1101+PA  │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │ (4P SPI connector)
// │ MOSI ───────┼──────┤ GPIO 23     │ (4P SPI connector)
// │ MISO ───────┼──────┤ GPIO 19     │ (4P SPI connector)
// │ CS ─────────┼──────┤ GPIO 21     │ (SPI peripheral connector CS)
// │ GDO0 ───────┼──────┤ GPIO 22     │ (RGB Red pad) TX to radio
// │ GDO2 ───────┼──────┤ GPIO 35     │ (Expand connector) RX from radio
// │ TX_EN ──────┼──────┤ GPIO 4      │ (Amp enable pad) PA transmit
// │ RX_EN ──────┼──────┤ GPIO 0      │ (BOOT pad) PA receive
// └─────────────┘      └─────────────┘
//
// IMPORTANT: GDO0/GDO2 naming is confusing!
// - GDO0 (GPIO22) = Data going TO the CC1101 (for TX)
// - GDO2 (GPIO35) = Data coming FROM the CC1101 (for RX)
// This matches the HaleHound fix for CiferTech's swapped pins.
//
// ═══════════════════════════════════════════════════════════════════════════

#ifdef CYD_35
  #define CC1101_CS     21    // Chip Select - SPI peripheral connector (E32R35T)
#elif CYD_C5
  #define CC1101_CS     9    // Chip Select - GPIO9 (ESP32-CYD-C5)
#else
  #define CC1101_CS     27    // Chip Select - CN1 connector
#endif

#define CC1101_GDO0     8    // TX data TO radio - P3 connector
#ifdef NMRF_HAT
  #define CC1101_GDO2   22    // Hat: no GDO2 wire — GDO0 outputs same RX data
#elif CYD_C5
  #define CC1101_GDO2   -1    //  ESP32-CYD-C5 - no GDO2 wire, GDO0 outputs same RX data  
#else
  #define CC1101_GDO2   35    // RX data FROM radio - P3 connector (INPUT ONLY)
#endif

// SPI bus aliases
#define CC1101_SCK      RADIO_SPI_SCK
#define CC1101_MOSI     RADIO_SPI_MOSI
#define CC1101_MISO     RADIO_SPI_MISO

// RCSwitch compatibility (HaleHound pin naming)
// REMEMBER: CiferTech had TX/RX swapped - we fixed it!
#ifdef CYD_C5
#define TX_PIN         11
#define RX_PIN         12
#else
#define TX_PIN          CC1101_GDO0   // GPIO22 - enableTransmit()
#define RX_PIN          CC1101_GDO2   // GPIO35 - enableReceive()
#endif

// ═══════════════════════════════════════════════════════════════════════════
// CC1101 PA MODULE SUPPORT (E07-433M20S)
// ═══════════════════════════════════════════════════════════════════════════
// E07-433M20S has an external PA (20dBm) that needs TX_EN/RX_EN control.
// TX_EN HIGH = transmit mode, RX_EN HIGH = receive mode.
// Both LOW = idle/sleep. NEVER both HIGH at the same time.
// Standard HW-863 modules ignore these pins (not connected).
// Enabled via Settings > CC1101 Module > E07 PA MODULE
// ═══════════════════════════════════════════════════════════════════════════
#ifdef CYD_28
  #ifdef CYD_E32R28T
    // E32R28T: GPIO 4 = SC8002B amp enable — doubles as PA TX_EN
    // Amp wakes briefly during CC1101 TX (6.5mA), shuts down on TX_EN LOW
    #define CC1101_TX_EN     4    // PA transmit enable — amp enable pad
    #define CC1101_RX_EN     0    // PA receive enable — BOOT button (pull-up = safe boot)
  #else
    #define CC1101_TX_EN    26    // PA transmit enable — was speaker/amp pin (disabled)
    #define CC1101_RX_EN     0    // PA receive enable — was BOOT button (pull-up = safe boot)
  #endif
#endif
#ifdef CYD_35
  // E32R35T: Same PA module support as E32R28T
  #define CC1101_TX_EN     4    // SC8002B amp enable = PA TX_EN
  #define CC1101_RX_EN     0    // BOOT button = PA RX_EN
#endif

// ═══════════════════════════════════════════════════════════════════════════
// NRF24L01+PA+LNA 2.4GHz RADIO
// ═══════════════════════════════════════════════════════════════════════════
//
// WIRING DIAGRAM (ESP32-2432S028):
// ┌─────────────┐      ┌─────────────┐
// │  NRF24L01   │      │     CYD     │
// │  +PA+LNA    │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │ (add 10uF cap if unstable!)
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │ (shared with CC1101)
// │ MOSI ───────┼──────┤ GPIO 23     │ (shared with CC1101)
// │ MISO ───────┼──────┤ GPIO 19     │ (shared with CC1101)
// │ CSN ────────┼──────┤ GPIO 4      │ (was RGB Red LED)
// │ CE ─────────┼──────┤ GPIO 16     │ (was RGB Green LED)
// │ IRQ ────────┼──────┤ GPIO 17     │ (was RGB Blue LED) OPTIONAL
// └─────────────┘      └─────────────┘
//
// WIRING DIAGRAM (E32R28T — E01-2G4M27SX):
// ┌─────────────┐      ┌─────────────┐
// │ E01-2G4M27  │      │   E32R28T   │
// │    SX       │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │ (add 10uF cap if unstable!)
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │ (shared VSPI)
// │ MOSI ───────┼──────┤ GPIO 23     │ (shared VSPI)
// │ MISO ───────┼──────┤ GPIO 19     │ (shared VSPI)
// │ CSN ────────┼──────┤ GPIO 26     │ (DAC pad — amp shut down)
// │ CE ─────────┼──────┤ GPIO 16     │ (RGB Green pad)
// │ IRQ ────────┼──────┤ N/C         │ (GPIO 17 used for PN532)
// └─────────────┘      └─────────────┘
//
// WIRING DIAGRAM (E32R35T — E01-2G4M27SX):
// ┌─────────────┐      ┌─────────────┐
// │ E01-2G4M27  │      │   E32R35T   │
// │    SX       │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V        │ (add 10uF cap if unstable!)
// │ GND ────────┼──────┤ GND         │
// │ SCK ────────┼──────┤ GPIO 18     │ (shared VSPI)
// │ MOSI ───────┼──────┤ GPIO 23     │ (shared VSPI)
// │ MISO ───────┼──────┤ GPIO 19     │ (shared VSPI)
// │ CSN ────────┼──────┤ GPIO 26     │ (DAC pad — amp shut down)
// │ CE ─────────┼──────┤ GPIO 16     │ (RGB Green pad)
// │ IRQ ────────┼──────┤ N/C         │ (GPIO 17 used for PN532)
// └─────────────┘      └─────────────┘
//
// NOTE: The +PA+LNA version needs clean 3.3V power!
// Add a 10uF capacitor between VCC and GND at the module if you get
// communication errors or the module resets randomly.
//
// ═══════════════════════════════════════════════════════════════════════════

#ifdef NMRF_HAT
  // NM-RF-Hat: physical switch selects CC1101 or NRF24 on same two GPIOs
  #define NRF24_CSN     27    // Shared with CC1101_CS via hat switch
  #define NRF24_CE      22    // Shared with CC1101_GDO0 via hat switch
#elif CYD_C5
  #define NRF24_CSN     9    // Chip Select - GPIO9 (ESP32-CYD-C5)
  #define NRF24_CE      8    // Chip Enable - GPIO8
#elif defined(CYD_E32R28T) || defined(CYD_E32R35T)
  // E32R28T/E32R35T: GPIO 4 used for CC1101_TX_EN (amp enable), NRF24_CSN moves to GPIO 26
  // GPIO 26 = DAC pad — coupling cap to SC8002B amp input (amp shut down = tiny load)
  #define NRF24_CSN     26    // Chip Select - DAC/speaker pad (amp dead)
  #define NRF24_CE      16    // Chip Enable - RGB Green pad
  #define NRF24_IRQ     17    // Interrupt - RGB Blue pad (shared with PN532_CS)
#else
  #define NRF24_CSN      4    // Chip Select - was RGB Red
  #define NRF24_CE      16    // Chip Enable - was RGB Green
  #define NRF24_IRQ     17    // Interrupt - was RGB Blue (OPTIONAL)
#endif


// SPI bus aliases
#define NRF24_SCK       RADIO_SPI_SCK
#define NRF24_MOSI      RADIO_SPI_MOSI
#define NRF24_MISO      RADIO_SPI_MISO

// ═══════════════════════════════════════════════════════════════════════════
// PN532 NFC/RFID MODULE (13.56 MHz)
// ═══════════════════════════════════════════════════════════════════════════
//
// WIRING DIAGRAM:
// ┌─────────────┐      ┌─────────────┐
// │  PN532 V3   │      │     CYD     │
// │ (Elechouse) │      │   ESP32     │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ 3.3V (CN1)  │
// │ GND ────────┼──────┤ GND  (CN1)  │
// │ SCK ────────┼──────┤ GPIO 18     │ (shared VSPI)
// │ MOSI ───────┼──────┤ GPIO 23     │ (shared VSPI)
// │ MISO ───────┼──────┤ GPIO 19     │ (shared VSPI)
// │ SS ─────────┼──────┤ GPIO 17     │ (was NRF24 IRQ — never used)
// └─────────────┘      └─────────────┘
//
// DIP SWITCHES: CH1=OFF, CH2=ON (SPI mode)
//
// NOTE: PN532 uses LSBFIRST SPI (all other VSPI devices use MSBFIRST).
// Safe because Adafruit library brackets every transfer with
// beginTransaction(LSBFIRST)/endTransaction(), and SPI manager
// ensures only one device is active at a time.
//
// ═══════════════════════════════════════════════════════════════════════════
#ifdef CYD_C5
#define PN532_SDA 9  // PN532 with I2C mode, NM-RF-HAT
#define PN532_SCL 8
#else
#define PN532_CS        17    // Chip Select - was NRF24 IRQ (RGB Blue LED, unused)
// SPI bus aliases
#define PN532_SCK       RADIO_SPI_SCK
#define PN532_MOSI      RADIO_SPI_MOSI
#define PN532_MISO      RADIO_SPI_MISO
#endif

// ═══════════════════════════════════════════════════════════════════════════
// GPS NEO-6M MODULE (Software Serial)
// ═══════════════════════════════════════════════════════════════════════════
//
// WIRING: GT-U7 GPS connected to P1 JST connector
// ┌─────────────┐      ┌─────────────┐
// │   GT-U7     │      │  CYD P1     │
// │    GPS      │      │  Connector  │
// ├─────────────┤      ├─────────────┤
// │ VCC ────────┼──────┤ VIN         │
// │ GND ────────┼──────┤ GND         │
// │ TX ─────────┼──────┤ RX (GPIO 3) │ (ESP receives GPS data)
// │ RX ─────────┼──────┤ TX (GPIO 1) │ (not used)
// └─────────────┘      └─────────────┘
//
// NOTE: P1 RX/TX are shared with CH340C USB serial.
// When GPS is active, Serial RX from computer is unavailable.
// Serial.println() debug output still works (UART0 TX on GPIO1).
// Uses HardwareSerial UART2 remapped to GPIO3 for reliable reception.
//
// ═══════════════════════════════════════════════════════════════════════════

#ifdef CYD_C5
#define GPS_RX_PIN      4    // P1 RX pin - ESP32 receives from GPS TX
#define GPS_TX_PIN      5    // Not used - GPS is receive-only
#else
#define GPS_RX_PIN       3    // P1 RX pin - ESP32 receives from GPS TX
#define GPS_TX_PIN      -1    // Not used - GPS is receive-only
#endif
#define GPS_BAUD      9600    // GT-U7 default baud rate

// ═══════════════════════════════════════════════════════════════════════════
// UART SERIAL MONITOR
// ═══════════════════════════════════════════════════════════════════════════
// UART passthrough for hardware hacking - read target device debug ports
// P1 connector: Full duplex via UART0 GPIO3/1 (shared with USB serial)
// Speaker connector: RX only via GPIO26

#define UART_MON_P1_RX        3    // P1 RX pin (shared with USB Serial RX)
#define UART_MON_P1_TX        1    // P1 TX pin (shared with USB Serial TX)
#ifdef CYD_35
  #define UART_MON_SPK_RX    -1    // GPIO 26 is NRF24_CSN on E32R35T — no speaker RX
#elif defined(CYD_E32R28T)
  #define UART_MON_SPK_RX    -1    // GPIO 26 is NRF24_CSN on E32R28T — no speaker RX
#else
  #define UART_MON_SPK_RX    26    // Speaker connector pin (RX only)
#endif
#define UART_MON_DEFAULT_BAUD 115200
#define CYD_HAS_SERIAL_MON    1

// ═══════════════════════════════════════════════════════════════════════════
// BUTTONS
// ═══════════════════════════════════════════════════════════════════════════
// CYD boards only have BOOT button - use touchscreen for navigation

#if defined(CYD_E32R28T) || defined(CYD_E32R35T)
  // E32R28T/E32R35T: GPIO 0 = CC1101_RX_EN — E07 PA module pulls it LOW permanently.
  // Cannot use GPIO 0 as BOOT button. Use touch-only navigation.
  #define BOOT_BUTTON       0    // Still GPIO0 physically, but reads forced false below
  #define BOOT_BUTTON_USABLE 0   // DO NOT check digitalRead(0) — always reads LOW
#else
  #define BOOT_BUTTON       0    // GPIO0 - active LOW, directly readable
  #define BOOT_BUTTON_USABLE 1
#endif

// Helper macro — replaces all direct digitalRead(0) == LOW checks
// When E07 PA module is active, GPIO 0 = CC1101_RX_EN (OUTPUT) — not a button.
// cc1101_pa_module check prevents false BOOT triggers that freeze the UI.
#if BOOT_BUTTON_USABLE
  #define IS_BOOT_PRESSED() (digitalRead(BOOT_BUTTON) == LOW && !cc1101_pa_module)
#else
  #define IS_BOOT_PRESSED() (false)
#endif

// ═══════════════════════════════════════════════════════════════════════════
// TOUCH BUTTON ZONES (Virtual buttons on touchscreen)
// Coordinates for PORTRAIT orientation
// ═══════════════════════════════════════════════════════════════════════════

// For 2.8" (240x320):
#if defined(CYD_28) || defined(CYD_C5)
  // UP button - top left
  #define TOUCH_BTN_UP_X1      0
  #define TOUCH_BTN_UP_Y1      0
  #define TOUCH_BTN_UP_X2     80
  #define TOUCH_BTN_UP_Y2     60

  // DOWN button - bottom left
  #define TOUCH_BTN_DOWN_X1    0
  #define TOUCH_BTN_DOWN_Y1  260
  #define TOUCH_BTN_DOWN_X2   80
  #define TOUCH_BTN_DOWN_Y2  320

  // SELECT button - center
  #define TOUCH_BTN_SEL_X1    80
  #define TOUCH_BTN_SEL_Y1   130
  #define TOUCH_BTN_SEL_X2   160
  #define TOUCH_BTN_SEL_Y2   190

  // BACK button - top right
  #define TOUCH_BTN_BACK_X1  160
  #define TOUCH_BTN_BACK_Y1    0
  #define TOUCH_BTN_BACK_X2  240
  #define TOUCH_BTN_BACK_Y2   60
#endif

// For 3.5" (320x480):
#ifdef CYD_35
  // UP button - top left (starts below icon bar touch zone to avoid overlap)
  #define TOUCH_BTN_UP_X1      0
  #define TOUCH_BTN_UP_Y1     60
  #define TOUCH_BTN_UP_X2    107
  #define TOUCH_BTN_UP_Y2     80

  // DOWN button - bottom left
  #define TOUCH_BTN_DOWN_X1    0
  #define TOUCH_BTN_DOWN_Y1  400
  #define TOUCH_BTN_DOWN_X2  107
  #define TOUCH_BTN_DOWN_Y2  480

  // SELECT button - center
  #define TOUCH_BTN_SEL_X1   107
  #define TOUCH_BTN_SEL_Y1   200
  #define TOUCH_BTN_SEL_X2   213
  #define TOUCH_BTN_SEL_Y2   280

  // BACK button - top right
  #define TOUCH_BTN_BACK_X1  213
  #define TOUCH_BTN_BACK_Y1    0
  #define TOUCH_BTN_BACK_X2  320
  #define TOUCH_BTN_BACK_Y2   80
#endif

// ═══════════════════════════════════════════════════════════════════════════
// FEATURE FLAGS
// ═══════════════════════════════════════════════════════════════════════════

#define CYD_HAS_CC1101      1     // CC1101 SubGHz radio connected
#define CYD_HAS_NRF24       1     // NRF24L01+PA+LNA 2.4GHz radio connected
#define CYD_HAS_GPS         1     // NEO-6M GPS module connected
#define CYD_HAS_SDCARD      1     // SD card ENABLED (shares VSPI with radios)
#define CYD_HAS_RGB_LED     0     // RGB LED DISABLED (pins used for NRF24)
#define CYD_HAS_SPEAKER     0     // Speaker DISABLED (pin used for GPS)
#define CYD_HAS_PCF8574     0     // No I2C button expander (unlike ESP32-DIV)
#define CYD_HAS_PN532       1     // PN532 NFC/RFID reader (13.56 MHz)

// ═══════════════════════════════════════════════════════════════════════════
// E32R28T-SPECIFIC FEATURES
// ═══════════════════════════════════════════════════════════════════════════
#if defined(CYD_E32R28T) || defined(CYD_E32R35T)
  #define CYD_HAS_BATTERY     1     // TP4854 LiPo charge IC on board
  #define CYD_BATTERY_ADC    34     // Battery voltage ADC (input only)
  #define CYD_HAS_AMP         1     // SC8002B audio amp — GPIO 4 = shutdown
  // NOTE: SC8002B amp shutdown is handled by CC1101_TX_EN (GPIO 4)
  // PA init sets GPIO 4 LOW = amp off. Amp wakes briefly during CC1101 TX only.
  // If PA module is DISABLED in settings, setup() must still init GPIO 4 LOW
  // to prevent amp from floating and drawing 6.5mA quiescent.
#else
  #define CYD_HAS_BATTERY     0
  #define CYD_HAS_AMP         0
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SPI BUS SHARING - ACTIVE DEVICES
// ═══════════════════════════════════════════════════════════════════════════
//
// VSPI Bus (GPIO 18/19/23) is SHARED by FOUR devices:
//   ┌──────────┬───────────────────────────┬───────────────────────────────┐
//   │ Device   │ CS Pin                    │ Notes                         │
//   ├──────────┼───────────────────────────┼───────────────────────────────┤
//   │ SD Card  │ GPIO 5                    │ Built-in slot, payload storage│
//   │ CC1101   │ GPIO 27 (28"/E32R28T)     │ SubGHz radio                  │
//   │          │ GPIO 21 (E32R35T)         │ (27 = backlight on 3.5")      │
//   │ NRF24    │ GPIO 4  (28")             │ 2.4GHz radio                  │
//   │          │ GPIO 26 (E32R28T/E32R35T) │ (4 = amp enable)              │
//   │ PN532    │ GPIO 17                   │ NFC/RFID 13.56 MHz (LSBFIRST)│
//   └──────────┴───────────────────────────┴───────────────────────────────┘
//
// IMPORTANT: Only ONE device active at a time!
// Before using a device: Pull its CS LOW, all others HIGH
//
// ═══════════════════════════════════════════════════════════════════════════
// DISABLED/REPURPOSED PINS
// ═══════════════════════════════════════════════════════════════════════════
//
// ── ESP32-2432S028 (Standard CYD 2.8") ──
// RGB LED (DISABLED - pins used for NRF24):
//   RGB_RED   = GPIO 4  → NRF24_CSN
//   RGB_GREEN = GPIO 16 → NRF24_CE
//   RGB_BLUE  = GPIO 17 → PN532_CS (was NRF24_IRQ, never used)
//
// Speaker (DISABLED - pin used for GPS):
//   SPEAKER = GPIO 26 → CC1101_TX_EN (PA module) / GPS_RX_PIN
//
// LDR Light Sensor (AVAILABLE - not repurposed):
//   LDR = GPIO 34 (input only, 12-bit ADC)
//   Could use for: ambient light detection, battery voltage divider
//
// ── QDtech E32R28T (2.8" Type-C with battery) ──
// RGB LED pins remapped:
//   RGB_RED   = GPIO 22 → CC1101_GDO0 (blinks on CC1101 TX = free indicator)
//   RGB_GREEN = GPIO 16 → NRF24_CE
//   RGB_BLUE  = GPIO 17 → PN532_CS
//
// Audio amp (SC8002B) repurposed:
//   AMP_EN    = GPIO 4  → CC1101_TX_EN (amp wakes during CC1101 TX, harmless)
//   DAC_OUT   = GPIO 26 → NRF24_CSN (coupling cap = tiny load, amp shut down)
//
// Battery (TP4854 charge IC):
//   BAT_ADC   = GPIO 34 → Battery voltage reading (input only)

// ═══════════════════════════════════════════════════════════════════════════
// GPIO 34 NOTE
// ═══════════════════════════════════════════════════════════════════════════
// GPIO 34 = LDR (Light Dependent Resistor) on stock ESP32-2432S028 boards.
// GPIO 34 = Battery ADC (TP4854 charge IC) on E32R28T boards.
// Input only, 12-bit ADC on all boards.

// ═══════════════════════════════════════════════════════════════════════════
// LAYOUT HELPERS — Use these instead of hardcoded pixel values!
// All values compute at compile time from CYD_SCREEN_WIDTH / CYD_SCREEN_HEIGHT
// ═══════════════════════════════════════════════════════════════════════════

// Icon bar (top navigation strip — used in almost every module)
#define ICON_BAR_TOP      19                              // Top separator line Y
#define ICON_BAR_Y        20                              // Content start inside icon bar
#define ICON_BAR_BOTTOM   36                              // Bottom separator line Y
#define ICON_BAR_H        16                              // Icon bar height (16px icons)
#define CONTENT_Y_START   38                              // First usable Y below icon bar

// Icon bar TOUCH zones (generous for fat fingers on 3.5")
#ifdef CYD_35
  #define ICON_BAR_TOUCH_TOP     0                          // Touch starts at very top
  #define ICON_BAR_TOUCH_BOTTOM  55                         // Generous bottom for fat fingers
#else
  #define ICON_BAR_TOUCH_TOP     ICON_BAR_Y                 // Match visual bounds on 2.8"
  #define ICON_BAR_TOUCH_BOTTOM  (ICON_BAR_BOTTOM + 4)      // Small padding on 2.8"
#endif

// Padded content area
#define CONTENT_PADDED_X     5                            // Left padding (5px)
#define CONTENT_PADDED_W     (CYD_SCREEN_WIDTH - 10)      // Width with 5px padding each side
#define CONTENT_INNER_X      10                           // Wider left padding (10px)
#define CONTENT_INNER_W      (CYD_SCREEN_WIDTH - 20)      // Width with 10px padding each side

// Graph/visualization areas
#define GRAPH_FULL_W         (CYD_SCREEN_WIDTH - 4)       // Near full width (2px each side)
#define GRAPH_PADDED_W       (CYD_SCREEN_WIDTH - 10)      // Standard graph width

// Menu layout
#define MENU_COLUMN_W        (CYD_SCREEN_WIDTH / 2)       // Half-screen column
#define MENU_COL_LEFT_X      10                           // Left column start
#define MENU_COL_RIGHT_X     (MENU_COL_LEFT_X + MENU_COLUMN_W)

// Dialog boxes (centered)
#define DIALOG_W             (CYD_SCREEN_WIDTH - 20)      // Standard dialog width
#define DIALOG_X             10                           // Dialog left edge
#define DIALOG_CENTER_X      (CYD_SCREEN_WIDTH / 2)       // Horizontal center

// Bottom area positions (relative to screen height)
#define BOTTOM_HINT_Y        (CYD_SCREEN_HEIGHT - 45)     // Hint text area
#define BOTTOM_NAV_Y         (CYD_SCREEN_HEIGHT - 33)     // Navigation text

// Button bar
#define BUTTON_BAR_Y     (CYD_SCREEN_HEIGHT - 37)   // Bottom button bar top edge
#define BUTTON_BAR_H     37                          // Button bar height
#define STATUS_LINE_Y    (CYD_SCREEN_HEIGHT - 18)    // Status text near bottom
#define CONTENT_BOTTOM   (BUTTON_BAR_Y - 2)          // Last usable Y before button bar

// ═══════════════════════════════════════════════════════════════════════════
// LAYOUT SCALING — Converts 2.8" (240x320) coordinates to current screen size
// All values compute at COMPILE TIME (integer math, no runtime cost)
// On 2.8": SCALE_Y(215) = 215, SCALE_X(175) = 175, SCALE_W(220) = 220
// On 3.5": SCALE_Y(215) = 322, SCALE_X(175) = 233, SCALE_W(220) = 293
// ═══════════════════════════════════════════════════════════════════════════

#define SCALE_Y(y)  ((y) * CYD_SCREEN_HEIGHT / 320)
#define SCALE_X(x)  ((x) * CYD_SCREEN_WIDTH / 240)
#define SCALE_W(w)  ((w) * CYD_SCREEN_WIDTH / 240)
#define SCALE_H(h)  ((h) * CYD_SCREEN_HEIGHT / 320)

// Menu button dimensions (auto-scale with column width)
#define MENU_BTN_W          (CYD_SCREEN_WIDTH / 2 - 20)
#define MENU_BTN_H          SCALE_H(60)
#define MENU_ICON_OFFSET_X  ((MENU_BTN_W - 16) / 2)
#define MENU_TEXT_OFFSET_Y  SCALE_H(30)

// Submenu layout
#define SUBMENU_Y_START     SCALE_Y(30)     // First item Y
#define SUBMENU_Y_SPACING   SCALE_Y(28)     // Gap between items
#define SUBMENU_LAST_GAP    SCALE_Y(10)     // Extra gap before "Back" item
#define SUBMENU_TOUCH_W     (CYD_SCREEN_WIDTH - 20)  // Touch hit area width
#define SUBMENU_TOUCH_H     SCALE_Y(25)     // Touch hit area height

// ═══════════════════════════════════════════════════════════════════════════
// TEXT SIZE — Scaled for display physical size
// 2.8" (240x320): size 1 = 6x8px chars, ~40 chars/line
// 3.5" (320x480): size 2 = 12x16px chars, ~26 chars/line (fills the space)
// ═══════════════════════════════════════════════════════════════════════════

// Text size stays at 1 on both boards — setTextSize(2) pixel-doubles and looks bad.
// The 3.5" fills space via layout scaling (SCALE_Y/X), not font scaling.
#define TEXT_SIZE_BODY      1     // Body text, menu items, status info
#define TEXT_SIZE_SMALL     1     // Fine print, dense data, debug info
#define TEXT_LINE_H        12     // Line height for body text
#define TEXT_LINE_H_SMALL  12     // Line height for small text
#define TEXT_CHAR_W         6     // Character width at body size

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG SETTINGS
// ═══════════════════════════════════════════════════════════════════════════

#define CYD_DEBUG           1
#define CYD_DEBUG_BAUD 115200

// ═══════════════════════════════════════════════════════════════════════════
// VALIDATION
// ═══════════════════════════════════════════════════════════════════════════

#if !defined(CYD_28) && !defined(CYD_35) && !defined(CYD_C5)
  #error "CYD_CONFIG: Define either CYD_28 or CYD_35 at the top of cyd_config.h"
#endif

#if defined(CYD_28) && defined(CYD_35)
  #error "CYD_CONFIG: Cannot define both CYD_28 and CYD_35 - choose one"
#endif

#endif // CYD_CONFIG_H
