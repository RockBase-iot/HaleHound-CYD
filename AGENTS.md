# HaleHound-CYD Project Guide for AI Agents

## Project Overview

**HaleHound-CYD** is a multi-protocol offensive security toolkit built for ESP32 "Cheap Yellow Display" (CYD) platforms. It is an Arduino-based firmware that provides wireless security testing capabilities including WiFi attacks, Bluetooth/BLE operations, SubGHz RF manipulation, RFID/NFC interaction, and signal intelligence (SIGINT) operations.

- **Version**: v3.4.0
- **Author**: JesseCHale (https://github.com/JesseCHale)
- **Website**: https://halehound.com
- **Total Codebase**: ~56,000 lines (C++/Arduino)
- **License**: See LICENSE file

### Hardware Targets

| Board | Build Target | Display | Resolution | MCU |
|-------|-------------|---------|------------|-----|
| ESP32-2432S028 (2.8") | `esp32-cyd` | ILI9341 | 240x320 | ESP32-D0WD |
| QDtech E32R35T (3.5") | `esp32-e32r35t` | ST7796 | 320x480 | ESP32-WROOM-32 |
| QDtech E32R28T (2.8") | `esp32-e32r28t` | ILI9341 | 240x320 | ESP32-WROOM-32 |
| NM-RF-Hat (2.8") | `esp32-cyd-hat` | ILI9341 | 240x320 | ESP32-D0WD |
| NM-CYD-C5 (2.8") | `nm-cyd-c5` | ILI9341 | 240x320 | ESP32-C5 |

### Required External Modules

- **CC1101**: SubGHz radio (300-928 MHz) for RF capture/replay/jamming
- **NRF24L01+PA+LNA**: 2.4GHz radio for WiFi/BLE/Zigbee jamming, MouseJack
- **PN532**: NFC/RFID reader (13.56 MHz) for card scanning/cloning
- **GPS Module** (GT-U7/NEO-6M): For wardriving and location logging

---

## Technology Stack

| Component | Technology |
|-----------|------------|
| **MCU Platform** | ESP32 (ESP32-D0WD-V3) / ESP32-C5 |
| **Framework** | Arduino (via PlatformIO) |
| **Build System** | PlatformIO (espressif32@6.9.0 for ESP32; pioarduino for ESP32-C5) |
| **Display Library** | TFT_eSPI@^2.5.43 |
| **Touch Controller** | XPT2046 (resistive) |
| **JSON Library** | ArduinoJson@^7.0.0 |
| **GPS Parsing** | TinyGPSPlus@^1.0.3 |
| **Software Serial** | EspSoftwareSerial@^8.2.0 |
| **RF Switching** | rc-switch@^2.6.4 |
| **FFT** | arduinoFFT@^2.0.2 |
| **RF24 Driver** | RF24@1.4.11 |
| **Touchscreen Driver** | XPT2046_Touchscreen (PaulStoffregen) |
| **CC1101 Driver** | SmartRC-CC1101-Driver-Lib@^2.5.7 |
| **PN532 Driver** | Adafruit PN532@^1.3.3 |
| **BLE Keyboard** | ESP32 BLE Keyboard@^0.3.2 |

---

## Project Structure

### Main Files

```
HaleHound-CYD.ino          # Main firmware entry point (~4,400 lines)
platformio.ini             # Build configuration
User_Setup.h               # TFT_eSPI display configuration
cyd_config.h               # Master pin configuration & board definitions
shared.h                   # Global state, color palette, extern declarations
```

### Core System Modules

| File | Purpose |
|------|---------|
| `spi_manager.cpp/h` | VSPI bus arbitration (SD/CC1101/NRF24/PN532 share GPIO 18/19/23 or C5 equivalents) |
| `touch_buttons.cpp/h` | Touchscreen input handling & virtual button zones |
| `utils.cpp/h` | Display helpers, EEPROM storage, color mode management |
| `CYD28_TouchscreenR.cpp/h` | XPT2046 touch driver (custom/bit-banged for 2.8") |

### Attack Modules

| Category | Files | Capabilities |
|----------|-------|--------------|
| **WiFi** | `wifi_attacks.cpp/h` | Packet Monitor, Beacon Spammer, Deauther, Probe Sniffer, WiFi Scanner, Captive Portal, Station Scanner, Auth Flood |
| **Bluetooth** | `bluetooth_attacks.cpp/h` | BLE Jammer, BLE Spoofer, BLE Beacon, Sniffer, BLE Scanner, WhisperPair (CVE-2025-36911), AirTag suite, BLE Ducky |
| **NRF24** | `nrf24_attacks.cpp/h`, `nrf24_config.cpp/h` | 2.4GHz Scanner, Spectrum Analyzer, NRF Sniffer, MouseJack, WLAN Jammer, Proto Kill |
| **SubGHz** | `subghz_attacks.cpp/h`, `subconfig.cpp/h` | Replay Attack, Brute Force, SubGHz Jammer, Spectrum Analyzer, Saved Profiles |
| **RFID** | `rfid_attacks.cpp/h` | Card Scanner, Card Reader, Card Clone, Key Brute Force, Card Emulation |
| **SIGINT** | `eapol_capture.cpp/h`, `karma_attack.cpp/h`, `saved_captures.cpp/h` | WPA handshake capture, Karma attacks, captured data browser |
| **Jam Detect** | `jam_detect.cpp/h` | WiFi Guardian, BLE Watchdog, SubGHz Sentinel (defensive) |

### Utility Modules

| File | Purpose |
|------|---------|
| `gps_module.cpp/h`, `gps.h`, `wardriving*.cpp/h` | GPS interface, wardriving screen, location logging |
| `iot_recon.cpp/h` | IoT device reconnaissance |
| `loot_manager.cpp/h`, `wp_loot_viewer.cpp/h` | Captured credential storage & viewing |
| `serial_monitor.cpp/h` | UART passthrough terminal for hardware hacking |
| `firmware_update.cpp/h` | OTA firmware updates from SD card |
| `radio_test.cpp/h` | SPI radio hardware verification tool |

### Asset Files (Data/Headers)

| File | Content |
|------|---------|
| `icon.h` | UI bitmap icons (16x16px skull-themed icons) |
| `skull_bg.h` | Background watermark bitmap |
| `nuke_icon.h` | Nuke mode icon (32x32px) |
| `nosifer_font.h` | Horror/glitch font for UI effects |
| `portal_pages.h` | HTML templates for captive portal |
| `findyou_keys.h` | Pre-generated P-224 EC keypairs for AirTag attacks |
| `ble_database.h` | BLE device manufacturer database |

---

## Build System

### Build Configuration (platformio.ini)

The project uses PlatformIO with multiple environment targets:

```ini
[env:esp32-cyd]        # Standard 2.8" CYD
[env:esp32-e32r35t]    # 3.5" E32R35T (adds -DCYD_E32R35T=1)
[env:esp32-e32r28t]    # 2.8" E32R28T (adds -DCYD_E32R28T=1)
[env:esp32-cyd-hat]    # NM-RF-Hat (adds -DNMRF_HAT=1)
[env:nm-cyd-c5]        # ESP32-C5 variant (adds -DNM_CYD_C5=1)
```

### Key Build Flags

```ini
-DUSER_SETUP_LOADED              # Use project's User_Setup.h
-include User_Setup.h            # Force include display config
-Wl,--allow-multiple-definition  # Allow SDK function overrides (deauth)
-DFAILURE_HANDLING               # RF24 timeout protection
```

### Partition Scheme

Uses `huge_app.csv` for maximum application space (~3MB for firmware).

### Build Commands

```bash
# Build for 2.8" CYD (standard)
pio run -e esp32-cyd

# Build for 3.5" E32R35T
pio run -e esp32-e32r35t

# Build for ESP32-C5 variant
pio run -e nm-cyd-c5

# Build and upload
pio run -e esp32-cyd --target upload

# Monitor serial output
pio device monitor -b 115200
```

---

## Runtime Architecture

### Application Lifecycle

The firmware follows a simple event-driven model:

1. **`setup()`** (in `HaleHound-CYD.ino`):
   - Initializes Serial, TFT display, backlight PWM, and splash screen.
   - Initializes `spiManagerSetup()`, touch buttons, EEPROM settings.
   - Optionally initializes GPS, WiFi/BLE stacks, and Safe Mode check.
   - Displays the main menu.

2. **`loop()`** (in `HaleHound-CYD.ino`):
   - Calls `handleButtons()` every 20ms.
   - **All application logic is driven from `handleButtons()`**, which dispatches to submenu touch handlers.

3. **Feature Activation**:
   - When a menu item is selected, the corresponding handler (e.g., `handleWiFiSubmenuTouch()`) enters a **blocking `while(!exitRequested)` loop**.
   - Inside that loop, the module's `loop()` function is called repeatedly.
   - `feature_active` is set to `true` during this time, which disables screen sleep.
   - On exit, the module's `cleanup()` is called and the menu is redrawn.

### SPI Bus Layout

```
HSPI BUS (Display/Touch) - GPIO 12/13/14/15
├── ILI9341/ST7796 Display (DC=GPIO2, BL=GPIO21/27/25)
└── XPT2046 Touch (CS=GPIO33)

VSPI BUS (Radios/SD) - GPIO 18/19/23
├── SD Card (CS=GPIO5)
├── CC1101 SubGHz (CS=GPIO27/21/9, GDO0=GPIO22/8, GDO2=GPIO35/-1)
├── NRF24L01 2.4GHz (CSN=GPIO4/26/9, CE=GPIO16/8)
└── PN532 NFC (CS=GPIO17 or I2C on C5) - LSBFIRST SPI
```

**CRITICAL**: Only ONE device on VSPI bus can be active at a time. Always use `spi_manager` to select/deselect devices.

### Pin Mapping (2.8" vs 3.5" vs C5)

| Function | 2.8" CYD | 3.5" E32R35T | C5 | Notes |
|----------|----------|--------------|----|-------|
| Display CS | GPIO 15 | GPIO 15 | GPIO 23 | HSPI |
| Backlight | GPIO 21 | GPIO 27 | GPIO 25 | PWM dimmable |
| CC1101 CS | GPIO 27 | GPIO 21 | GPIO 9 | 27 = backlight on 3.5" |
| NRF24 CSN | GPIO 4 | GPIO 26 | GPIO 9 | 4 = amp enable on 3.5" |
| NRF24 CE | GPIO 16 | GPIO 16 | GPIO 8 | |
| Touch SPI | Separate (GPIO 25/32/39) | Shared HSPI | GPIO 1/6/7/2 | Handled by TFT_eSPI or CYD28_TouchscreenR |

### EEPROM Storage Layout

Settings are persisted to EEPROM at defined offsets:

```cpp
#define EEPROM_MAGIC_ADDR       0   // 2 bytes: validation magic
#define EEPROM_BRIGHTNESS_ADDR  2   // 1 byte: backlight level
#define EEPROM_TIMEOUT_ADDR     3   // 2 bytes: screen timeout
#define EEPROM_COLOR_ORDER_ADDR 5   // 1 byte: BGR/RGB swap
#define EEPROM_INVERSION_ADDR   6   // 1 byte: display inversion
#define EEPROM_COLOR_MODE_ADDR  7   // 1 byte: colorblind mode
#define EEPROM_ROTATION_ADDR    8   // 1 byte: screen rotation
#define EEPROM_DISCLAIMER_ADDR  9   // 1 byte: VALHALLA disclaimer accepted
#define EEPROM_BLUE_TEAM_ADDR   10  // 1 byte: Blue team mode
#define EEPROM_PIN_ADDR         11  // 2 bytes: 4-digit PIN
#define EEPROM_PIN_ENABLED_ADDR 13  // 1 byte: PIN lock enabled
#define EEPROM_PA_MODULE_ADDR   14  // 1 byte: CC1101 PA module type
```

---

## Code Style Guidelines

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Constants/Macros | UPPER_SNAKE_CASE | `CYD_SCREEN_WIDTH` |
| Global Variables | lower_snake_case | `current_menu_index` |
| Functions | camelCase | `displayMenu()` |
| Namespaces | PascalCase | `PacketMonitor::` |
| Class/Struct | PascalCase | `Credential` |
| Enum Values | UPPER_SNAKE_CASE | `SPAM_MODE_NUKE` |

### Color Palette (Theme System)

```cpp
// Primary theme colors (runtime adjustable)
HALEHOUND_MAGENTA   // 0x041F - Electric blue (primary accent)
HALEHOUND_HOTPINK   // 0xF81F - Hot pink (highlights)
HALEHOUND_VIOLET    // 0x780F - Purple (secondary accent)
HALEHOUND_GREEN     // 0x780F - Maps to purple in default theme

// Fixed colors
HALEHOUND_DARK      // 0x2841 - Dark background
HALEHOUND_BLACK     // 0x0000 - Pure black
HALEHOUND_GUNMETAL  // 0x18E3 - Dark gray
```

### Module Structure Pattern

All attack modules follow this pattern:

```cpp
namespace ModuleName {
    void setup();           // Initialize module
    void loop();            // Main loop (called repeatedly)
    bool isExitRequested(); // Check if user wants to exit
    void cleanup();         // Release resources, restore state
}
```

### Layout Scaling Macros

Always use scaling macros for UI coordinates to support both 2.8" and 3.5" displays:

```cpp
#define SCALE_Y(y)  ((y) * CYD_SCREEN_HEIGHT / 320)
#define SCALE_X(x)  ((x) * CYD_SCREEN_WIDTH / 240)
#define SCALE_W(w)  ((w) * CYD_SCREEN_WIDTH / 240)
#define SCALE_H(h)  ((h) * CYD_SCREEN_HEIGHT / 320)
```

---

## Testing and Development

### Automated Testing

There is **no automated unit or integration test suite** in this project. Testing is performed manually on hardware.

### Radio Test Tool

Use **Tools > Radio Test** for SPI hardware verification. It tests:
- NRF24 CONFIG register read (should return 0x08)
- CC1101 VERSION register read (should return 0x14)
- Wiring diagnostics with visual diagrams

### Debug Output

Serial debug is available at 115200 baud. Key debug patterns:

```cpp
#if CYD_DEBUG
Serial.println("[Module] Debug message");
#endif
```

### Touch Calibration

First boot after flash runs automatic touch calibration. Can be re-run from **Tools > Touch Calibrate**.

Calibration values are stored in EEPROM and persist across power cycles.

### Safe Mode

Hold BOOT button during power-on to force safe mode (resets EEPROM to defaults). Note: on E32R28T/E32R35T with PA module enabled, GPIO0 is repurposed and the BOOT button is not usable.

---

## Deployment

### Pre-built Binaries

The `flash_package/` directory contains pre-built firmware binaries for each board variant, along with `partitions.bin`, `bootloader.bin`, `boot_app0.bin`, and `FLASH_INSTRUCTIONS.txt` for manual flashing via `esptool.py`.

### CI/CD

There are **no GitHub Actions workflows** for building or releasing this project. The `.github/FUNDING.yml` file is the only repository automation present.

---

## Security Considerations

### VALHALLA Protocol

The firmware implements a legal disclaimer system (VALHALLA Protocol):

1. **Disclaimer Screen**: Shown on first offensive action until accepted
2. **Blue Team Mode**: Optional mode that blocks all offensive operations
3. **PIN Lock**: Optional 4-digit PIN for device access control

### Offensive Module Guards

All offensive modules check `isOffensiveAllowed()` before execution:

```cpp
if (!isOffensiveAllowed()) {
    if (blue_team_mode) { 
        showBlueTeamBlockedScreen(); 
        if (!showDisclaimerScreen()) break; 
    }
    else if (!showDisclaimerScreen()) break;
    if (!isOffensiveAllowed()) break;
}
```

### Responsible Disclosure

- CVE-2025-36911 (WhisperPair) implementation is for authorized security testing only
- All jamming functions operate at maximum legal power limits
- AirTag attacks use pre-generated keys (`findyou_keys.h`) for research purposes

---

## Common Tasks for AI Agents

### Adding a New WiFi Attack Module

1. Declare namespace and functions in `wifi_attacks.h`
2. Implement in `wifi_attacks.cpp`
3. Add menu item to `HaleHound-CYD.ino` WiFi submenu
4. Add handler in `handleWiFiSubmenuTouch()` switch statement
5. Follow module pattern: `setup()` → `loop()` → `cleanup()`

### Adding a New Menu Category

1. Add icon to `icon.h` (16x16px bitmap)
2. Add menu item to `menu_items[]` array
3. Add icon to `bitmap_icons[]` array
4. Create submenu items array
5. Add submenu icons array
6. Update `updateActiveSubmenu()` switch case
7. Create handler function (e.g., `handleNewCategorySubmenuTouch()`)
8. Wire into main touch handling loop

### Modifying Pin Configuration

1. Edit `cyd_config.h` - master pin definitions
2. Update `User_Setup.h` - TFT_eSPI display pins
3. Check `spi_manager.h` - CS pin assignments
4. Verify board-specific `#ifdef` blocks (CYD_28, CYD_35, CYD_C5, NMRF_HAT, CYD_E32R28T, CYD_E32R35T)

### Adding EEPROM Storage

1. Define new address offset (see EEPROM layout above)
2. Add load logic in `loadSettings()` (`utils.cpp`)
3. Add save logic in `saveSettings()` (`utils.cpp`)
4. Add extern declaration in `shared.h` if needed globally

---

## Important Notes

1. **Never call `BLEDevice::deinit(true)`** - Use `deinit(false)` to avoid ESP-IDF bug that breaks reinit
2. **Always use `spiSelect()`/`spiDeselect()`** - Never manipulate CS pins directly for VSPI devices
3. **GPS conflicts with USB serial** - When GPS active, `Serial` RX is unavailable (GPIO3 shared on ESP32, GPIO4/5 on C5)
4. **E32R28T/E32R35T BOOT button** - GPIO0 is repurposed as `CC1101_RX_EN`, cannot be used as button when PA module enabled
5. **PN532 uses LSBFIRST** - Unlike other SPI devices (MSBFIRST). The Adafruit library handles this via `beginTransaction()`
6. **C5 pin differences** - The `nm-cyd-c5` target uses a different GPIO map (e.g., display on GPIO 2/6/7/23/24, CC1101/NRF24 on GPIO 8/9). Always check `cyd_config.h` when modifying code for C5.

---

## References

- Original ESP32-DIV: https://github.com/JesseCHale/ESP32-DIV
- TFT_eSPI Library: https://github.com/Bodmer/TFT_eSPI
- ESP32 Arduino Core: https://docs.espressif.com/projects/arduino-esp32/
- Bruce Firmware (reference for CYD support): https://github.com/pr3y/Bruce
