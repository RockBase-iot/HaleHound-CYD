========================================================================================
## v3.5.0 | 2026-03-27
## What's New

### New Modules

**BLE Predator** — GATT Reconnaissance + Honeypot Credential Trap

Three-phase BLE attack. SCAN discovers nearby devices and classifies them by threat tier (RED/YELLOW). RECON connects via GATT client, enumerates all services and characteristics, caches read values. HONEYPOT clones the target as a connectable GATTS server — logs every CONNECT/READ/WRITE/DISCONNECT event. WRITE captures contain credential data (PINs, tokens, passwords). 3-mode on-screen keyboard for device name filtering. Pulsing LIVE indicator, color-coded event log, SD card loot save.

**Flock You** — Flock Safety Surveillance Camera Detector

Passive SIGINT module that detects Flock Safety ALPR cameras, Raven/ShotSpotter gunshot sensors, and associated infrastructure via BLE advertisement fingerprinting. 22 OUI prefixes, 4 BLE name patterns, XUNTONG manufacturer ID, 8 Raven GATT service UUIDs with firmware version estimation. Dual-core BLE scan. SD card save with GPS coordinates.

### Upgrades

**Radio Test Overhaul**
- NRF24: 126-channel spectrum scan (30 sweeps via testRPD()) + TX test
- CC1101: RSSI baseline on 315/433.92 MHz, 3-second signal detection
- GPS: inline test with gradient progress bar
- 4-button layout: NRF24 / CC1101 / GPS / WIRING

*Radio Test upgrade source code by Duggie*

**AP-Locked 2.4GHz Spectrum Analyzer**
- Tap AP Select to WiFi scan, pick an AP, locks analyzer to that channel's NRF24 range (~22 channels)
- Zoomed view with wider bar spacing, channel + SSID in title bar

**Fast Touch Engine**
- Bypasses TFT_eSPI getTouch() 5x validation loop with direct raw reads cached per frame
- 100-200x faster touch response — quick taps register instantly

**Jammer Flicker Fix**
- Draw-once standby pattern on all 4 jammer screens (BLE, WLAN, ProtoKill, SubGHz)
- Idle refresh reduced from 30ms to 200ms when not jamming

**BLE Database** — 94 company IDs, 73 service UUIDs, 52 GAP appearances for device identification

### New Hardware Support

**ESP32-3248S035C (CYD35C)** — 3.5" Capacitive Touch
- GT911 capacitive touch controller (I2C) — same ST7796 display as E32R35T
- Edge-triggered debounce tuned for capacitive panels
- Build: `pio run -e esp32-cyd35c`

**3.5" UI Scaling** — BLE Spoofer, Beacon, Sniffer + SubGHz Jammer/Brute Force touch zones properly scaled for 320x480

### Removed

**BLE Ducky** — T-vK/ESP32-BLE-Keyboard library is abandoned (240+ open issues, broken Windows reconnect). Replaced by BLE Predator honeypot.

### Menu Changes
- BLE Predator replaces BLE Scanner + Sniffer in Bluetooth menu
- AirTag hub renamed to **Lunatic Fringe** hub (Tracker Scan, AirTag Detect, Phantom Flood, AirTag Replay)
- Flock You added to SIGINT menu

### Supported Boards

| Board | Build Target | Status |
|-------|-------------|--------|
| ESP32-2432S028 (2.8") | `esp32-cyd` | Fully Tested |
| QDtech E32R35T (3.5") | `esp32-e32r35t` | Fully Tested |
| ESP32-3248S035C (3.5" Cap Touch) | `esp32-cyd35c` | Fully Tested |
| QDtech/Hosyond E32R28T (2.8") | `esp32-e32r28t` | Fully Tested |
| NM-RF-Hat (2.8") | `esp32-cyd-hat` | Supported |

### Credits

**Duggie** — Radio Test upgrade source code (spectrum scan, TX test, signal detection)
========================================================================================
## v3.5.1 | 2026-03-28
## v3.5.1 Patch Release

**CYD35C (3248S035C) Pin Mapping Fix**

Fixed incorrect pin assignments on the 3.5" capacitive touch build target that caused CC1101 SubGHz radio to fail. If you're running a 3248S035C board, update immediately.

### Changes
- CC1101 chip select corrected from GPIO 21 to GPIO 26 on 3248S035C (GPIO 27 is backlight, GPIO 26 is speaker pad — speaker disabled for SubGHz)
- NRF24 chip select moved from GPIO 4 to GPIO 21 to avoid conflict with CC1101 TX enable
- E32R35T gets its own splash screen watermark
- Flash instructions updated with CYD35C board and correct version
- All version references updated to v3.5.1

### Firmware Downloads
- **HaleHound-CYD.bin** — CYD 2.8" (ESP32-2432S028)
- **HaleHound-CYD-HAT.bin** — NM-RF-Hat
- **HaleHound-E32R28T.bin** — QDtech E32R28T 2.8"
- **HaleHound-E32R35T.bin** — QDtech E32R35T 3.5" Resistive
- **HaleHound-CYD35C.bin** — ESP32-3248S035C 3.5" Capacitive

Ref: issue #5

Full flash instructions included in the repo.
========================================================================================
## v3.5.5 | 2026-04-03
![HaleHound v3.5.5](https://github.com/JesseCHale/HaleHound-CYD/releases/download/v3.5.5/halehound-v355-release.png)

## What's New

### New Modules

**Airoha RACE** — CVE-2025-20700/20701/20702
Unauthenticated BLE GATT exploit for Airoha-based Bluetooth audio devices. Targets Sony XM4/XM5/XM6, Marshall, JBL, Jabra, Beyerdynamic — anything running an Airoha chipset. Extracts link keys, BD_ADDR, firmware version, and flash memory without pairing. Two-phase scan: BLE discovery then GATT service probe. Attack report with SD card loot save.

**Tesla Charge Port Opener**
Opens the charge port on any Tesla. Static 43-byte OOK payload, zero authentication, zero rolling code. Region selector: US (315 MHz), EU (433.92 MHz), or both.

**.Sub Read** — Flipper .sub File Browser + Transmitter
Browse and transmit Flipper Zero .sub files from the SD card. Supports RAW, Princeton, CAME, Nice FLO across the full CC1101 frequency range (300-348, 387-464, 779-928 MHz). Nested folder navigation. No .sub files required — drop them in /subghz/ on the SD when you have them.

**Battery Monitor**
LiPo battery voltage and percentage monitoring for boards with TP4854 charge IC (E32R28T, E32R35T). ADC on GPIO 34.

### SubGHz Replay — Major Overhaul

- Complete UI revamp: Nosifer glitch title, 6-icon bar, skull watermark, transparent capture panel
- RSSI gating — noise floor frames rejected, real remotes only
- Drain loop — RMT RX buffer no longer floods with garbage
- Repeat validation — requires signal decoded twice before accepting
- CLEAR button — tap to discard capture and resume listening
- On-screen QWERTY keyboard for custom profile naming
- Auto-scan ON by default

### SubGHz Brute Force — De Bruijn Fix

De Bruijn sequence generator was wired up but never called. Now active — overlapping code windows exhaust N-bit keyspace efficiently.

### Proto Kill — GFSK Noise Mode

New toggle: CW carrier (original narrowband spike) or GFSK modulated noise (random 32-byte packets at 2Mbps filling ~2MHz bandwidth per channel). Toggle with SELECT when not jamming.

### Bug Fixes

- **EEPROM address collision** — Three modules used different EEPROM sizes and overlapping addresses. Settings commit with begin(512) was nuking all SubGHz saved profiles. Unified to 5120 bytes, moved WiFi portal addresses above SubGHz range.
- **SPI bus leak** — Added SPI.end() to all NRF24 init failure paths (BLE Jammer, WLAN Jammer, Proto Kill, MouseJack). Prevents SPI bus lock when radio init fails on Core 0.
- **SubGHz Replay RMT restart** — resumeRmtRx() wasn't uninstalling RX driver before re-init, causing capture to silently stop after clearing a signal.
- **SPI speed optimizations** — Flash SPI 40→80 MHz, display SPI 40→55 MHz, NRF24 raw SPI 8→10 MHz.

### Other

- README complete rewrite — trimmed, all 40+ modules documented, community credits
- Ebyte PA module independent 3.3V buck converter requirement documented
- Supported boards consolidated to 4 targets
- Version bump to v3.5.5

### Firmware Downloads

| File | Board |
|------|-------|
| HaleHound-CYD-FULL.bin | CYD 2.8" (ESP32-2432S028) |
| HaleHound-E32R35T-FULL.bin | QDtech E32R35T 3.5" |
| HaleHound-E32R28T-FULL.bin | QDtech E32R28T 2.8" |
| HaleHound-CYD-HAT-FULL.bin | NM-RF-Hat |

Flash at address 0x0 with [ESP Web Flasher](https://esp.huhn.me) or esptool.

Four-file method also available — download all assets below.
========================================================================================
## v3.6.0 | 2026-06-14
```
              _                                                    _
         .k$$$$$g,                                             ,g$$$$$k.
      .k$$$$$$$$$$$a.                                       .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                  ,?^?$?`    `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                                ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                               j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                                :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'   ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',    `                              .    ,'       _ ?kiSSI?
    ?i$?` _   k$        .                               :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?

     ██░ ██  ▄▄▄       ██▓    ▓█████  ██░ ██  ▒█████   █    ██  ███▄    █ ▓█████▄
    ▓██░ ██▒▒████▄    ▓██▒    ▓█   ▀ ▓██░ ██▒▒██▒  ██▒ ██  ▓██▒ ██ ▀█   █ ▒██▀ ██▌
    ▒██▀▀██░▒██  ▀█▄  ▒██░    ▒███   ▒██▀▀██░▒██░  ██▒▓██  ▒██░▓██  ▀█ ██▒░██   █▌
    ░▓█ ░██ ░██▄▄▄▄██ ▒██░    ▒▓█  ▄ ░▓█ ░██ ▒██   ██░▓▓█  ░██░▓██▒  ▐▌██▒░▓█▄   ▌
    ░▓█▒░██▓ ▓█   ▓██▒░██████▒░▒████▒░▓█▒░██▓░ ████▓▒░▒▒█████▓ ▒██░   ▓██░░▒████▓
     ▒ ░░▒░▒ ▒▒   ▓▒█░░ ▒░▓  ░░░ ▒░ ░ ▒ ░░▒░▒░ ▒░▒░▒░ ░▒▓▒ ▒ ▒ ░ ▒░   ▒ ▒  ▒▒▓  ▒
     ▒ ░▒░ ░  ▒   ▒▒ ░░ ░ ▒  ░ ░ ░  ░ ▒ ░▒░ ░  ░ ▒ ▒░ ░░▒░ ░ ░ ░ ░░   ░ ▒░ ░ ▒  ▒
     ░  ░░ ░  ░   ▒     ░ ░      ░    ░  ░░ ░░ ░ ░ ▒   ░░░ ░ ░    ░   ░ ░  ░ ░  ░
     ░  ░  ░      ░  ░    ░  ░   ░  ░ ░  ░  ░    ░ ░     ░              ░    ░
    
              _                                                   _
         .k$$$$$g,                                           ,g$$$$$k.
      .k$$$$$$$$$$$a.                                     .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                 ,?^?$?`   `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                               ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                              j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                              :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'  ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',  .                                .    ,'       _ ?kiSSI?
    ?i$?` _  k$        .:                              :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?
```

# HaleHound-CYD v3.6.0

This one's stacked. A four-tool drone toolkit, a real SubGHz spectrum analyzer, NFC Tools-grade NTAG writing, the new Skimmer Hunter, and a rebuilt WiFi jammer.

## What's New

### New Modules

**Drone Toolkit** — a full anti-drone menu with four tools:
- **Drone Detect** — runs two radios at once: the NRF24 sniffs for drone BLE presence while the ESP32 sweeps WiFi in promiscuous mode, on a dual-core engine so neither side blocks the other. Flags drones by ASTM F3411 Remote ID plus SSID and BLE fingerprint.
- **Drone Jelly** — full-screen jam with a live equalizer readout.
- **Drone Deauth** — targeted deauth against a drone's control link.
- **RID Spoof** — broadcasts an ASTM F3411 Remote ID beacon over WiFi (Beacon and NaN) and BLE.

**SubGHz Spectrum Analyzer** — a 33-bar live spectrum across the 433 front-end with peak-hold dots and a waveform graph under it. Band focus (433.92 / 315 / WIDE) narrows the sweep and the RX bandwidth together, so the noise floor drops and close signals separate instead of smearing. Noise-floor calibration measures each channel's floor and then shows only what rises above it, and a GAIN dial pulls weak signals up the bars. Full controls under How to Use.

**RFID NTAG Tools** — an NFC Tools-style tag suite for NTAG213 / 215 / 216. Model ID, full page dump, NDEF decode (URL and Text), a multi-record composer, and the write side: erase, permanent read-only lock, password set and remove, and auth-on-write. Full controls under How to Use.

**Skimmer Hunter (beta)** — a Bluetooth credit-card-skimmer hunter. Multi-signal scoring plus an active confirmation pass to cut false positives. It's built and ships in this release for field testing — treat it as beta until the hardware validation pass is done.

### Overhauls

**WLAN Jammer** — rebuilt on GFSK noise output with a PA-level toggle and a tight 1/6/11 pattern, plus an all-channels mode with tuned per-channel dwell. The approaches that didn't pan out are documented in the source so they don't get retried.

**Captive Portal** — six new templates, real brand logos, and a fix for the SSID getting overwritten when you switch templates.

## How to Use

### SubGHz Spectrum Analyzer
**SubGHz > Spectrum Analyzer.** Live sweep of the 433 front-end — 33 spectrum bars with peak-hold dots, a waveform graph under them, and a peak-frequency readout at the bottom.

**Top icons:** power = start/stop the sweep, undo = clear the display, antenna = CAL on/off (the ON/OFF label sits right next to it, lit pink = on), back arrow on the far left = exit. The physical BOOT button also exits.

**Bottom strip — three tap zones:** left third `<` = previous band, right third `>` = next band, center = GAIN. Each GAIN tap steps 1x -> 2x -> 3x -> 4x -> back to 1x (shown in yellow) and lifts weak signals higher up the bars.

**Bands:** 433.92 (426-442 MHz, the default and the radio's matched home), 315 (307-323 MHz, US car/garage fobs — weaker since it's off the 433 match, but it'll hear a strong fob held close), and WIDE (300-464 MHz, the whole usable range in one sweep). The front-end is matched for 433, so 868/915 are deaf and aren't offered.

**CAL (noise-floor calibration):** off by default — bars show raw RSSI. Tap the antenna to turn it on, keep the band quiet for the ~0.3s it spends measuring each channel's floor ("CALIBRATING... keep the band quiet"), and from then on the bars show only signal above that floor — empty channels sit flat and a real transmission jumps out. Changing bands drops CAL back to off (the old floor doesn't apply to a new band), so re-arm it after you switch. Run RAW to eyeball a whole band, flip to CAL to make one fob or sensor pop out of the noise.

**Reading it:** tallest bar = strongest channel. The bottom line gives the peak frequency, how far it stands above the floor (`d:` in dB), the noise floor, raw RSSI, and SCAN/PAUSE.

### RFID NTAG Tools
**RFID > NTAG Tools.** Works on NTAG213 / 215 / 216 (and Ultralight EV1). It's a nine-item touch menu, and the flow is stage-then-write — you build the records first, then flash them to the tag in one pass. **Write Tag** and **Clear Records** stay dimmed until you've staged at least one record.

- **Read / Info + Dump** — hold a tag to the reader; shows the model, how much user memory it has, its lock/password state, and a full page dump (saved to SD if a card is in). If the tag carries NDEF records, a later Write Tag can re-clone them.
- **Add URL** — type a URL; it's staged as an NDEF record (common prefixes like `https://` get packed short automatically).
- **Add Text** — type a text record; staged the same way.
- **Write Tag [N rec]** — hold a tag; writes the N staged records onto it. If the tag is password-protected, it tells you it needs the password.
- **Clear Records** — empties the staging buffer. Does not touch the tag.
- **Erase** — wipes the NDEF message off the tag. Re-writable afterward. Asks you to confirm.
- **Make Read-Only** — permanently locks the tag against any future write. No undo — it confirms first. A locked tag still reads, it just never writes again.
- **Set Password** — sets a 4-byte write password (PWD_AUTH). Write it down; you need it to remove the password or to write the tag later.
- **Remove Password** — clears the password. You enter the current one to do it.

Text, URL, and password entry all use the on-screen keyboard (lower / upper / symbols).

## Boards

Four targets in the flash package. Each board gets an app bin (to update over an existing install) and a `-FULL` bin (bootloader + partitions + app, for a fresh flash):

- **HaleHound-CYD** — 2.8" CYD (ESP32-2432S028, CH340 USB)
- **HaleHound-CYD-HAT** — 2.8" CYD wired for the NM RF Hat
- **HaleHound-E32R28T** — QDtech E32R28T (2.8", Type-C, battery)
- **HaleHound-E32R35T** — QDtech E32R35T (3.5")

## Credits

- **ToxxikHalo** — for proposing the SubGHz Spectrum Analyzer. The concept came from his idea.
- **JboHack** — the Drone Toolkit's drone spoofing is built on his Nyan drone-spoofing project.

```
                                                            ⣀⡀⣀⢀⡀⢀⡀⣀    ⣀⢀⡀ ⡀⢀ ⣀⢀⡀⢀⡀
                                                     ⢀⡀⣛⠘⠿⢦⠤⣽⢟⣿⢻⡟⣾⡏⣹⣿⣿⢻⡟⣿⣾⣿⣿⡿⣿⢿⣿⣌⣿⢸⡷⢤⡄⢚⢀⡀
                                              ⣀⢀⣀⣀⣀⢀⣀⣈⣁⣉⣀⣀⣀⣀⣉⣈⣁⣀⣀⣀⣀⣉⣀⣁⣀⣁⣉⣉⣀⣈⣀⣀⣀⣉⣈⣉⣀⣀⣀⣀⣈⣈⣁⣀⣀⣀ ⣀
                                           ⣤⠐⢶⢻⡟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⡇
                                       ⢀⣀⣠⣄⣤⣀⣤⣤⣤⣤⣤⣤⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣍⣭⣭⣭⣭⣥⣭⣭⣭⣭⣥⣭⣭⣭⣬⣭⣭⣥⣤⣤⣤⣤⣤⣤⣄⣤⣀⣄⣀⡀
                                      ⣴⠸⠟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⠿⠿⠉⠉
                                  ⢀⡀⣶⣦⣶⣴⣶⣴⣤⣴⣶⣶⣼⣦⣿⣶⣾⣶⣶⣾⣧⣿⣾⣷⣾⣷⣿⣶⣶⣶⣶⣶⣦⣶⣶⣶⣶⣷⣿⣶⣶⣶⣶⣶⣶⣿⣾⣷⣴⣦⣶⣦⣤⣤⣦⣴⣦⣴⢠⣦⣤⡆⣴⣤⣤⣤⣤⣤⣄
                                  ⠸⠇⠿⠻⢿⠿⠿⠿⠿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠟⠛⠃
                               ⡤⢰⣶⣶⣶⣾⣷⣿⣶⣿⣾⣷⣿⣶⣿⣶⣷⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣷⣿⣿⣿⣾⣿⣿⣷⣿⣾⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣾⣷⣿⣧⣶⣰⣶⣤⣤⣤⣴⣶⣴⣶⣶⣶⣤⣶⣶
                               ⠛⠘⠛⠛⠛⢿⠿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢿⠿⠿⠿⠿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⣿⡿⣿⠿⠿⠛⠛⠛⠛⠛⠛⠛⠛⠋⠛⠛⠛⠛⠛⠛⠁⠛⠘⠃⠐⠂
                           ⢠⠶⣾⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣾⣿⣿⣷⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣶⣿⣾⣷⣿⣷⣾⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⠶⣶⣶⣦⣤⣀
                             ⠉⠉⠛⠈⠛⠙⠋⠙⠛⠛⠛⠛⣿⠿⢿⣻⣿⢿⡟⢿⠿⠿⠿⠿⢿⡿⣿⠿⠿⢿⡿⣿⠿⣿⢿⡿⢿⡟⠿⠿⣿⣿⡿⣿⠿⣿⢿⡟⠿⠿⠿⠻⡿⢿⡟⠛⠛⠛⠛⠛⠛⠛⠋⠙⠋⠛⠋⠛⠉⠛⠋ ⠘⠁⠉ ⠋⠉⠁⠈⠉⠉
                        ⢰⢿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⡿⣿⣿⡾⢻⡟⣿⠿⣿ ⡆
                       ⣀⢀⡀⣀⡘⢋⣀ ⣉⣉⣉⣈⣁⣙⡛⣁⣉⣉⣉⣉⣛⢛⣛⣛⣋⣛⣛⣛⣻⣿⣿⡟⣿⣿⣿⣿⡟⣿⣿⣿⣻⣟⣻⣛⣛⢛⣻⣿⣟⣛⣿⣿⣻⡟⣿⣟⣿⢛⣛⢛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣉⡋⣛⡙⣛⠉⡀⠘⠁⠉⡈⠁⣈⡁⢉ ⠁⠈⠃
                     ⠋⠠⠏⠘⠁⡿⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⡿⣿⢻⡿⣹⡇⣿ ⣓ ⡀
                        ⢀⣀⣀⣈⣉⣩⣅⣄⣀⣭⣉⣍⣁⣈⣉⣩⣍⣩⣉⣭⣭⣿⣩⣍⣽⣭⣩⣭⣭⣭⣍⣭⣉⣍⣩⣍⣭⣉⣭⣩⣍⣩⣉⣭⣩⣍⣩⣍⣩⣉⣉⣉⣍⣉⣉⣭⣉⣉⣉⣍⣉⣉⣉⣩⣁⣉⢉⣉⢈⡁⣉⣀⣉⣉⣉⣁⣀⣈⣀⣈⣀⣁⣉⡁⠈ ⠁ ⠁
                       ⠂⠚⠉⠛⠩⠟⠟⠻⣿⢿⣿⢿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⡿⠿⣿⢿⣿⣿⡿⣿⣿⣿⢹⣿⠿⠏⣿⢩⣿⣿⣿⣿⣿⣿⢿⡟⢹⡟⣤
                        ⣠⣤⣤⣤⣤⣤⣤⣴⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣭⣼⣿⣿⣿⣿⣭⣭⣭⣭⣭⣭⣭⣤⣤⣤⣤⣤⣬⣥⣤⣄⣬⣭⣥⣬⣥⣭⣅⣤⣤⣬⣤⣄⣠⣄⣤ ⣤⣠⣤⣤⣴⣮⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⣠⢀⣄⣤⣤⣤⣠⣤⣤⣀
                      ⠈⠉⠛⠛⠿⠿⠛⠿⠿⠿⠿⠿⢿⠿⣿⠟⠿⠸⠟⢹⡇⢿⠘⠿⠻⠿⠟⠻⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⣿⢿⡿⢿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢻⡟⠛⠃⠛ ⠚⠘⠏⠛⠙⠛⠻⠿⠿⠿⠛⠹⠟⠙⠃⣿⠰⡾⠛⠋⠿⠣⣿⠻⡿⠻⠇⠰
                        ⢴⣤⣶⣶⣶⣶⣦⣶⣤⣤⣴⣤⢤⡄            ⣤⢠⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣦⣶⣤⣤⣴⣶⣶⣶⣶⣶⣶⣦⣤⣠⣄         ⣤⠠⡆⠠⣤⣤⢠⣶⣶⡶⣤⡤⢤       ⣠⡄⣀
                        ⠘⠋⠛⠛⠉⠋⠉⠿⠉⠿ ⠁              ⠉⠈⠁⠉⠉⠉⠉⠉⠉⠉⠋⠹⠿⠻⠏⠻⠿⠿⠻⠿⠿⠿⠟⠛⠛⠃           ⠈⠁⠘⠁⠉⠉⠛⠙⠃⠛ ⠋     ⠰⠆⠈ ⠴
                        ⢀⡶⣶⡐⢶ ⠆                           ⠠⠤⣦ ⣤⢲⣶⣶⣶⣶⣤⣤⢴⣦⢶⣶⣤⣶⣶                ⡴⢐⡆⢶⣰⣶
                        ⠈⠁⠋ ⠂  ⠛                            ⠋⠘⠛⠘⠋⠛⠛⠋⠙⠛⠛⠋⠚⠋⠛⠘⠃                 ⠈⠁⠈⠁          ⠚
                                                          ⢰⣷⣦⣴⣿⣾⣷⣶⣦        ⢰⡆⢲⡆
                               ⠛                          ⠉⠁⠉⠉⠋⠙⠋⠁         ⠈⠁⠈⠁                             ⠛
                               ⣀ ⣀                   ⢀ ⣸⠹⣿⣿⡟⣿⢿⣿⢿⡽⠿⠞⢿⠃        ⣹⡟⣿⠇⣀                        ⢀⡀⣀
                               ⣉ ⠉                   ⢀ ⡀⢀⣀⢈⡁⣉ ⠉⠈⠁  ⢀ ⡀       ⠉ ⠈ ⡉ ⡀                      ⠈⠁⢉
                             ⢘⡃⣿ ⣛⢀⡀                  ⠈⣧⢸⡿⢿⣷⣹⡇     ⠘⠈⠃        ⠠⠶⢶⡿⢹⡟⣀                   ⢀⡀⢘⡇⣿ ⣟⢀⡀⢀⡀
                             ⣀⣀⣀⣀⣁⢈⠁               ⢠⣄⣀⣀⣉⣈⣀⣈⣁⠉        ⡀ ⡀          ⢀⡀⣈⣀⣄⢀⡀⢀⡀             ⢀⣁⣀⣁⣈⣀⣁ ⡁⠈
                           ⢠⡤⢼⣿⣿⣯⡿⠸⡇⣤⠄⢤      ⣤⢠⡄⢿⠶⠿⢾⡟⣿⡏⣿⢫⣿⠹⡏⣤         ⠐⠃⠠      ⣤⠐⠶⠸⡇⢿⠻⠿⠸⡇⠙⠃⣤⠠⡤       ⢤⡄⣤⠈⠛⢽⠏⢿⠛⠿⠠⡇⢤⠄
                             ⣠⣄⣤⣤⣤⣤⣤⣤⣄   ⣠⡄⣤⣀⣤⢀⣄⣠⣤⣄⢀⣄⡀     ⡀          ⢀ ⣠⡄⢀         ⣤⣀⣤        ⡀⢀ ⡀ ⡀⣠⣤⣤⣤⣤⣤⣤⣀⣤⣤
                      ⠐ ⠒ ⣶⠰⠦⠉⠹⠟⢹⡿⠿⠋⠾⠛⠿⠛⠿⠉⠉⠉⠉⠉⠙⠛⢿⠟⢿⠸⡟⠻⠟⠛⠿⠟⠘⠇          ⠉⠁⠈ ⠈ ⠂  ⠶ ⠆⠘⠏⠉⠉⠉⠰⠆⠰⠆⠴⠈⠇⠘⠇⠿ ⡿⠘⠇⠉⠁⠿⠹⡿⠿⠋⠿⠉⠉⠰⠆⠰⠂
                     ⣶⣴⣶⢶⡄⣀  ⢀⡀   ⢀⡀       ⢤⢠⣦⣤⡴⠲⡦⢤    ⣀                ⢀ ⣤⢀⡄⣠⡄⣠ ⡄              ⠴⠄⣶⣦⡴⢦  ⢠⣦⣶⣦⣴⣄⣀  ⣀⡀
                     ⠉⠛⠛⠿⠆⠿⠰⠇⠰⠟⠛⠛⠛⠰⠆⠶⠆       ⠛⠈⠁⠘⠃⠈  ⠰⠆⠴ ⠦⠰⠆            ⠈ ⠉⠈⠁  ⠉⠰⠷⠰⠇⠶ ⠶           ⠁    ⠾ ⠁⠸⠏  ⠶⠰⠆⠰ ⠶
                    ⠰⢶⣶⣿⣷⣶⣶⢶⣶⣶⣄⣀⡠⠦⠰⠦⠶⠄⠄       ⣀⡀          ⠠⡄⣤              ⠰⠦⠶⢤⣶⣤⡤⠠⡄⢤       ⠠⠶⣺⡷⣶⣶⣶⢶⡶⠶⠄        ⠠⣤⡤
                      ⠉⠉⠙⠛⠛⠚⠛⠉⠈⠛                   ⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛                   ⠙⠛⠘⠃⠛ ⠛⠘⠃     ⠈ ⠘⠉⠁ ⠁        ⠐⠛  ⠛⠂
                      ⠤⠤⢤⠄                             ⢀⠰⣶⠴⠦⣶⡆⣶⢠⡄  ⢀   ⡀  ⣀ ⡀⣶⡆⣶⣦⣤⢰⡦⢶⠆⣠         ⣒⢂⣶⢐⡒⠂
                                                     ⠘⠁⠉    ⠉ ⠉⠈⠁⠈⠁⠉⠈⠉⠈⠃  ⠙⠈⠃⠈⠃⠉ ⠉⠈⠁  ⠉         ⠉⠈⠛⠈⠃
                           ⢀⡀⢀⡀⣀                     ⢀⡀⣿⢈⣿⠸⣇⣴⣴⣦⣴⣦⣹⡇⣶    ⣶⠆⣿⣶⣦⣶⡄⣿⢈⣿⣿⡿⢿⡿⠿                        ⢀⡀
                           ⠈⠁⠈⠁⠉                ⣀ ⢀⢀⡀⢈⡁⣈⣈⣁⣈⣁⣀    ⠉ ⢉   ⡀⣉ ⢉  ⢀⣀⠉⢈⣁⣈⡀⣉⠁⣀                        ⠈⠁
                           ⢀⡀⢀⡀                 ⠉⠃⠉⠘⠋⢻⣿⣿⣿⣿⣻⡟⣿⠰⠶⠶⠶⣶⡶⠿⢻⣄⢀⡟⠿⠿⠿⠾⠷⣿⡟⣿⢻⡿⣿⣿⣿⣟⣿⢀⡄
                             ⠈⠁                ⡀⣠⡄⣠⢀⡀⣀⡅⣈⣩⣭⢀⡀⢀    ⠠⡀ ⠈⠉ ⡅⢠⣤⣤⣠⡄⣈⡀⣀ ⣄⣈⡀⣈⡉⠁ ⡁
                                               ⠁⠿⠁⠼⢸⡿⠻⡿⢿⢿⠟⠛⠛⣿⠤⣤   ⠁⠚⠛⠓ ⠁⠛⠹⣿⢩⡗⢤⡜⠋⠻⠟⢽⠇⢼⠟⠻⠺⡇
                                              ⣤⣄⢠⡄⣤⢀⡄      ⠄   ⢠⡄   ⢀⡀⣤⣴⣦⣤⣤⣴⣆⢰⡄ ⢀⡀
                                              ⠛⠛⠚⠳⣿⠸⡇⠺⠏⠈⠁ ⠰⠆     ⠻⠏⢿⠈⠁⠈⠋⠛⠙⢿⠻⡿⠒⠁⠶⠈⠻⠠⠆  ⠶  ⠰⠆
                                              ⢰⡆⣴⡆⠤                   ⢰⣦⣴⡶⣶⢶⡦⣄ ⣀
                                              ⠈⠁   ⠸⠇⠺⠇⠾           ⠿    ⠉ ⠛⠘⠋⠈⠁⠿ ⠶  ⠶ ⠶
                                                                        ⣀⡀

                                                                     ⡀⢀⡀
                                                                    ⠈⠉⠈⠁
                                                               ⢀⡀⢀⡀  ⡶⢰⡆
                                                               ⠈⠁⠈⠁ ⠈⠉⠈⠁


                                                   ⠠⡤   ⠠⡤⠠⡄⢤

                                                   ⠰⠆⠰⠆⠶⠐⠶⠰⠆⠶
                                                                     ⣄
                                                       ⠺⠐⠇⠐⠆⠲ ⠶⠐⠇⠰⠂⠲ ⠇⠐⠇⠲ ⠲⠐⠆⠸⠂⠾ ⠖

                                                        ⠘⠛⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠛⠐⠃
```
========================================================================================
## v3.6.1 | 2026-06-15
> **Flash in your browser — no install needed:** https://flash.halehound.com

```
              _                                                    _
         .k$$$$$g,                                             ,g$$$$$k.
      .k$$$$$$$$$$$a.                                       .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                  ,?^?$?`    `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                                ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                               j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                                :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'   ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',    `                              .    ,'       _ ?kiSSI?
    ?i$?` _   k$        .                               :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?

     ██░ ██  ▄▄▄       ██▓    ▓█████  ██░ ██  ▒█████   █    ██  ███▄    █ ▓█████▄
    ▓██░ ██▒▒████▄    ▓██▒    ▓█   ▀ ▓██░ ██▒▒██▒  ██▒ ██  ▓██▒ ██ ▀█   █ ▒██▀ ██▌
    ▒██▀▀██░▒██  ▀█▄  ▒██░    ▒███   ▒██▀▀██░▒██░  ██▒▓██  ▒██░▓██  ▀█ ██▒░██   █▌
    ░▓█ ░██ ░██▄▄▄▄██ ▒██░    ▒▓█  ▄ ░▓█ ░██ ▒██   ██░▓▓█  ░██░▓██▒  ▐▌██▒░▓█▄   ▌
    ░▓█▒░██▓ ▓█   ▓██▒░██████▒░▒████▒░▓█▒░██▓░ ████▓▒░▒▒█████▓ ▒██░   ▓██░░▒████▓
     ▒ ░░▒░▒ ▒▒   ▓▒█░░ ▒░▓  ░░░ ▒░ ░ ▒ ░░▒░▒░ ▒░▒░▒░ ░▒▓▒ ▒ ▒ ░ ▒░   ▒ ▒  ▒▒▓  ▒
     ▒ ░▒░ ░  ▒   ▒▒ ░░ ░ ▒  ░ ░ ░  ░ ▒ ░▒░ ░  ░ ▒ ▒░ ░░▒░ ░ ░ ░ ░░   ░ ▒░ ░ ▒  ▒
     ░  ░░ ░  ░   ▒     ░ ░      ░    ░  ░░ ░░ ░ ░ ▒   ░░░ ░ ░    ░   ░ ░  ░ ░  ░
     ░  ░  ░      ░  ░    ░  ░   ░  ░ ░  ░  ░    ░ ░     ░              ░    ░
    
              _                                                   _
         .k$$$$$g,                                           ,g$$$$$k.
      .k$$$$$$$$$$$a.                                     .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                 ,?^?$?`   `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                               ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                              j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                              :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'  ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',  .                                .    ,'       _ ?kiSSI?
    ?i$?` _  k$        .:                              :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?
```

# HaleHound-CYD v3.6.1

Maintenance fix for v3.6.0. Two bugs down, fresh HALEHOUND art, and your saved RFID dumps now show up in Loot.

## What's New

**HALEHOUND artwork** — the graffiti skull watermark now rides on every CYD board.

**RFID dumps in Loot** — saved tag dumps (NTAG, page dumps, keys) land in a new RFID category under Loot and open right on the device in the text viewer. No more pulling the card to read them.

## Fixes

**RFID no longer drops the SD card** — reading or saving a tag used to leave the SD card unreadable for the rest of the session (Loot and Wardriving would come up empty until a reboot). The shared SPI bus is now handed back cleanly, so RFID, SD logging, and saved dumps all work in one session.

**Evil Portal broadcasts your SSID** — the portal was coming up under the ESP32's default name (ESP_xxxxxx) no matter what SSID you set. It now broadcasts the name you choose.

## Boards

Four targets in the flash package. Each gets an app bin (update over an existing install) and a `-FULL` bin (bootloader + partitions + app, for a fresh flash):

- **HaleHound-CYD** — 2.8" CYD (ESP32-2432S028, CH340 USB)
- **HaleHound-CYD-HAT** — 2.8" CYD wired for the NM RF Hat
- **HaleHound-E32R28T** — QDtech E32R28T (2.8", Type-C, battery)
- **HaleHound-E32R35T** — QDtech E32R35T (3.5")

```
                                                            ⣀⡀⣀⢀⡀⢀⡀⣀    ⣀⢀⡀ ⡀⢀ ⣀⢀⡀⢀⡀
                                                     ⢀⡀⣛⠘⠿⢦⠤⣽⢟⣿⢻⡟⣾⡏⣹⣿⣿⢻⡟⣿⣾⣿⣿⡿⣿⢿⣿⣌⣿⢸⡷⢤⡄⢚⢀⡀
                                              ⣀⢀⣀⣀⣀⢀⣀⣈⣁⣉⣀⣀⣀⣀⣉⣈⣁⣀⣀⣀⣀⣉⣀⣁⣀⣁⣉⣉⣀⣈⣀⣀⣀⣉⣈⣉⣀⣀⣀⣀⣈⣈⣁⣀⣀⣀ ⣀
                                           ⣤⠐⢶⢻⡟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⡇
                                       ⢀⣀⣠⣄⣤⣀⣤⣤⣤⣤⣤⣤⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣍⣭⣭⣭⣭⣥⣭⣭⣭⣭⣥⣭⣭⣭⣬⣭⣭⣥⣤⣤⣤⣤⣤⣤⣄⣤⣀⣄⣀⡀
                                      ⣴⠸⠟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⠿⠿⠉⠉
                                  ⢀⡀⣶⣦⣶⣴⣶⣴⣤⣴⣶⣶⣼⣦⣿⣶⣾⣶⣶⣾⣧⣿⣾⣷⣾⣷⣿⣶⣶⣶⣶⣶⣦⣶⣶⣶⣶⣷⣿⣶⣶⣶⣶⣶⣶⣿⣾⣷⣴⣦⣶⣦⣤⣤⣦⣴⣦⣴⢠⣦⣤⡆⣴⣤⣤⣤⣤⣤⣄
                                  ⠸⠇⠿⠻⢿⠿⠿⠿⠿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠟⠛⠃
                               ⡤⢰⣶⣶⣶⣾⣷⣿⣶⣿⣾⣷⣿⣶⣿⣶⣷⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣷⣿⣿⣿⣾⣿⣿⣷⣿⣾⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣾⣷⣿⣧⣶⣰⣶⣤⣤⣤⣴⣶⣴⣶⣶⣶⣤⣶⣶
                               ⠛⠘⠛⠛⠛⢿⠿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢿⠿⠿⠿⠿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⣿⡿⣿⠿⠿⠛⠛⠛⠛⠛⠛⠛⠛⠋⠛⠛⠛⠛⠛⠛⠁⠛⠘⠃⠐⠂
                           ⢠⠶⣾⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣾⣿⣿⣷⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣶⣿⣾⣷⣿⣷⣾⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⠶⣶⣶⣦⣤⣀
                             ⠉⠉⠛⠈⠛⠙⠋⠙⠛⠛⠛⠛⣿⠿⢿⣻⣿⢿⡟⢿⠿⠿⠿⠿⢿⡿⣿⠿⠿⢿⡿⣿⠿⣿⢿⡿⢿⡟⠿⠿⣿⣿⡿⣿⠿⣿⢿⡟⠿⠿⠿⠻⡿⢿⡟⠛⠛⠛⠛⠛⠛⠛⠋⠙⠋⠛⠋⠛⠉⠛⠋ ⠘⠁⠉ ⠋⠉⠁⠈⠉⠉
                        ⢰⢿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⡿⣿⣿⡾⢻⡟⣿⠿⣿ ⡆
                       ⣀⢀⡀⣀⡘⢋⣀ ⣉⣉⣉⣈⣁⣙⡛⣁⣉⣉⣉⣉⣛⢛⣛⣛⣋⣛⣛⣛⣻⣿⣿⡟⣿⣿⣿⣿⡟⣿⣿⣿⣻⣟⣻⣛⣛⢛⣻⣿⣟⣛⣿⣿⣻⡟⣿⣟⣿⢛⣛⢛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣉⡋⣛⡙⣛⠉⡀⠘⠁⠉⡈⠁⣈⡁⢉ ⠁⠈⠃
                     ⠋⠠⠏⠘⠁⡿⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⡿⣿⢻⡿⣹⡇⣿ ⣓ ⡀
                        ⢀⣀⣀⣈⣉⣩⣅⣄⣀⣭⣉⣍⣁⣈⣉⣩⣍⣩⣉⣭⣭⣿⣩⣍⣽⣭⣩⣭⣭⣭⣍⣭⣉⣍⣩⣍⣭⣉⣭⣩⣍⣩⣉⣭⣩⣍⣩⣍⣩⣉⣉⣉⣍⣉⣉⣭⣉⣉⣉⣍⣉⣉⣉⣩⣁⣉⢉⣉⢈⡁⣉⣀⣉⣉⣉⣁⣀⣈⣀⣈⣀⣁⣉⡁⠈ ⠁ ⠁
                       ⠂⠚⠉⠛⠩⠟⠟⠻⣿⢿⣿⢿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⡿⠿⣿⢿⣿⣿⡿⣿⣿⣿⢹⣿⠿⠏⣿⢩⣿⣿⣿⣿⣿⣿⢿⡟⢹⡟⣤
                        ⣠⣤⣤⣤⣤⣤⣤⣴⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣭⣼⣿⣿⣿⣿⣭⣭⣭⣭⣭⣭⣭⣤⣤⣤⣤⣤⣬⣥⣤⣄⣬⣭⣥⣬⣥⣭⣅⣤⣤⣬⣤⣄⣠⣄⣤ ⣤⣠⣤⣤⣴⣮⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⣠⢀⣄⣤⣤⣤⣠⣤⣤⣀
                      ⠈⠉⠛⠛⠿⠿⠛⠿⠿⠿⠿⠿⢿⠿⣿⠟⠿⠸⠟⢹⡇⢿⠘⠿⠻⠿⠟⠻⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⣿⢿⡿⢿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢻⡟⠛⠃⠛ ⠚⠘⠏⠛⠙⠛⠻⠿⠿⠿⠛⠹⠟⠙⠃⣿⠰⡾⠛⠋⠿⠣⣿⠻⡿⠻⠇⠰
                        ⢴⣤⣶⣶⣶⣶⣦⣶⣤⣤⣴⣤⢤⡄            ⣤⢠⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣦⣶⣤⣤⣴⣶⣶⣶⣶⣶⣶⣦⣤⣠⣄         ⣤⠠⡆⠠⣤⣤⢠⣶⣶⡶⣤⡤⢤       ⣠⡄⣀
                        ⠘⠋⠛⠛⠉⠋⠉⠿⠉⠿ ⠁              ⠉⠈⠁⠉⠉⠉⠉⠉⠉⠉⠋⠹⠿⠻⠏⠻⠿⠿⠻⠿⠿⠿⠟⠛⠛⠃           ⠈⠁⠘⠁⠉⠉⠛⠙⠃⠛ ⠋     ⠰⠆⠈ ⠴
                        ⢀⡶⣶⡐⢶ ⠆                           ⠠⠤⣦ ⣤⢲⣶⣶⣶⣶⣤⣤⢴⣦⢶⣶⣤⣶⣶                ⡴⢐⡆⢶⣰⣶
                        ⠈⠁⠋ ⠂  ⠛                            ⠋⠘⠛⠘⠋⠛⠛⠋⠙⠛⠛⠋⠚⠋⠛⠘⠃                 ⠈⠁⠈⠁          ⠚
                                                          ⢰⣷⣦⣴⣿⣾⣷⣶⣦        ⢰⡆⢲⡆
                               ⠛                          ⠉⠁⠉⠉⠋⠙⠋⠁         ⠈⠁⠈⠁                             ⠛
                               ⣀ ⣀                   ⢀ ⣸⠹⣿⣿⡟⣿⢿⣿⢿⡽⠿⠞⢿⠃        ⣹⡟⣿⠇⣀                        ⢀⡀⣀
                               ⣉ ⠉                   ⢀ ⡀⢀⣀⢈⡁⣉ ⠉⠈⠁  ⢀ ⡀       ⠉ ⠈ ⡉ ⡀                      ⠈⠁⢉
                             ⢘⡃⣿ ⣛⢀⡀                  ⠈⣧⢸⡿⢿⣷⣹⡇     ⠘⠈⠃        ⠠⠶⢶⡿⢹⡟⣀                   ⢀⡀⢘⡇⣿ ⣟⢀⡀⢀⡀
                             ⣀⣀⣀⣀⣁⢈⠁               ⢠⣄⣀⣀⣉⣈⣀⣈⣁⠉        ⡀ ⡀          ⢀⡀⣈⣀⣄⢀⡀⢀⡀             ⢀⣁⣀⣁⣈⣀⣁ ⡁⠈
                           ⢠⡤⢼⣿⣿⣯⡿⠸⡇⣤⠄⢤      ⣤⢠⡄⢿⠶⠿⢾⡟⣿⡏⣿⢫⣿⠹⡏⣤         ⠐⠃⠠      ⣤⠐⠶⠸⡇⢿⠻⠿⠸⡇⠙⠃⣤⠠⡤       ⢤⡄⣤⠈⠛⢽⠏⢿⠛⠿⠠⡇⢤⠄
                             ⣠⣄⣤⣤⣤⣤⣤⣤⣄   ⣠⡄⣤⣀⣤⢀⣄⣠⣤⣄⢀⣄⡀     ⡀          ⢀ ⣠⡄⢀         ⣤⣀⣤        ⡀⢀ ⡀ ⡀⣠⣤⣤⣤⣤⣤⣤⣀⣤⣤
                      ⠐ ⠒ ⣶⠰⠦⠉⠹⠟⢹⡿⠿⠋⠾⠛⠿⠛⠿⠉⠉⠉⠉⠉⠙⠛⢿⠟⢿⠸⡟⠻⠟⠛⠿⠟⠘⠇          ⠉⠁⠈ ⠈ ⠂  ⠶ ⠆⠘⠏⠉⠉⠉⠰⠆⠰⠆⠴⠈⠇⠘⠇⠿ ⡿⠘⠇⠉⠁⠿⠹⡿⠿⠋⠿⠉⠉⠰⠆⠰⠂
                     ⣶⣴⣶⢶⡄⣀  ⢀⡀   ⢀⡀       ⢤⢠⣦⣤⡴⠲⡦⢤    ⣀                ⢀ ⣤⢀⡄⣠⡄⣠ ⡄              ⠴⠄⣶⣦⡴⢦  ⢠⣦⣶⣦⣴⣄⣀  ⣀⡀
                     ⠉⠛⠛⠿⠆⠿⠰⠇⠰⠟⠛⠛⠛⠰⠆⠶⠆       ⠛⠈⠁⠘⠃⠈  ⠰⠆⠴ ⠦⠰⠆            ⠈ ⠉⠈⠁  ⠉⠰⠷⠰⠇⠶ ⠶           ⠁    ⠾ ⠁⠸⠏  ⠶⠰⠆⠰ ⠶
                    ⠰⢶⣶⣿⣷⣶⣶⢶⣶⣶⣄⣀⡠⠦⠰⠦⠶⠄⠄       ⣀⡀          ⠠⡄⣤              ⠰⠦⠶⢤⣶⣤⡤⠠⡄⢤       ⠠⠶⣺⡷⣶⣶⣶⢶⡶⠶⠄        ⠠⣤⡤
                      ⠉⠉⠙⠛⠛⠚⠛⠉⠈⠛                   ⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛                   ⠙⠛⠘⠃⠛ ⠛⠘⠃     ⠈ ⠘⠉⠁ ⠁        ⠐⠛  ⠛⠂
                      ⠤⠤⢤⠄                             ⢀⠰⣶⠴⠦⣶⡆⣶⢠⡄  ⢀   ⡀  ⣀ ⡀⣶⡆⣶⣦⣤⢰⡦⢶⠆⣠         ⣒⢂⣶⢐⡒⠂
                                                     ⠘⠁⠉    ⠉ ⠉⠈⠁⠈⠁⠉⠈⠉⠈⠃  ⠙⠈⠃⠈⠃⠉ ⠉⠈⠁  ⠉         ⠉⠈⠛⠈⠃
                           ⢀⡀⢀⡀⣀                     ⢀⡀⣿⢈⣿⠸⣇⣴⣴⣦⣴⣦⣹⡇⣶    ⣶⠆⣿⣶⣦⣶⡄⣿⢈⣿⣿⡿⢿⡿⠿                        ⢀⡀
                           ⠈⠁⠈⠁⠉                ⣀ ⢀⢀⡀⢈⡁⣈⣈⣁⣈⣁⣀    ⠉ ⢉   ⡀⣉ ⢉  ⢀⣀⠉⢈⣁⣈⡀⣉⠁⣀                        ⠈⠁
                           ⢀⡀⢀⡀                 ⠉⠃⠉⠘⠋⢻⣿⣿⣿⣿⣻⡟⣿⠰⠶⠶⠶⣶⡶⠿⢻⣄⢀⡟⠿⠿⠿⠾⠷⣿⡟⣿⢻⡿⣿⣿⣿⣟⣿⢀⡄
                             ⠈⠁                ⡀⣠⡄⣠⢀⡀⣀⡅⣈⣩⣭⢀⡀⢀    ⠠⡀ ⠈⠉ ⡅⢠⣤⣤⣠⡄⣈⡀⣀ ⣄⣈⡀⣈⡉⠁ ⡁
                                               ⠁⠿⠁⠼⢸⡿⠻⡿⢿⢿⠟⠛⠛⣿⠤⣤   ⠁⠚⠛⠓ ⠁⠛⠹⣿⢩⡗⢤⡜⠋⠻⠟⢽⠇⢼⠟⠻⠺⡇
                                              ⣤⣄⢠⡄⣤⢀⡄      ⠄   ⢠⡄   ⢀⡀⣤⣴⣦⣤⣤⣴⣆⢰⡄ ⢀⡀
                                              ⠛⠛⠚⠳⣿⠸⡇⠺⠏⠈⠁ ⠰⠆     ⠻⠏⢿⠈⠁⠈⠋⠛⠙⢿⠻⡿⠒⠁⠶⠈⠻⠠⠆  ⠶  ⠰⠆
                                              ⢰⡆⣴⡆⠤                   ⢰⣦⣴⡶⣶⢶⡦⣄ ⣀
                                              ⠈⠁   ⠸⠇⠺⠇⠾           ⠿    ⠉ ⠛⠘⠋⠈⠁⠿ ⠶  ⠶ ⠶
                                                                        ⣀⡀

                                                                     ⡀⢀⡀
                                                                    ⠈⠉⠈⠁
                                                               ⢀⡀⢀⡀  ⡶⢰⡆
                                                               ⠈⠁⠈⠁ ⠈⠉⠈⠁


                                                   ⠠⡤   ⠠⡤⠠⡄⢤

                                                   ⠰⠆⠰⠆⠶⠐⠶⠰⠆⠶
                                                                     ⣄
                                                       ⠺⠐⠇⠐⠆⠲ ⠶⠐⠇⠰⠂⠲ ⠇⠐⠇⠲ ⠲⠐⠆⠸⠂⠾ ⠖

                                                        ⠘⠛⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠛⠐⠃
```
========================================================================================
## v3.6.2 | 2026-06-29
```
              _                                                    _
         .k$$$$$g,                                             ,g$$$$$k.
      .k$$$$$$$$$$$a.                                       .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                  ,?^?$?`    `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                                ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                               j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                                :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'   ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',    `                              .    ,'       _ ?kiSSI?
    ?i$?` _   k$        .                               :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?

     ██░ ██  ▄▄▄       ██▓    ▓█████  ██░ ██  ▒█████   █    ██  ███▄    █ ▓█████▄
    ▓██░ ██▒▒████▄    ▓██▒    ▓█   ▀ ▓██░ ██▒▒██▒  ██▒ ██  ▓██▒ ██ ▀█   █ ▒██▀ ██▌
    ▒██▀▀██░▒██  ▀█▄  ▒██░    ▒███   ▒██▀▀██░▒██░  ██▒▓██  ▒██░▓██  ▀█ ██▒░██   █▌
    ░▓█ ░██ ░██▄▄▄▄██ ▒██░    ▒▓█  ▄ ░▓█ ░██ ▒██   ██░▓▓█  ░██░▓██▒  ▐▌██▒░▓█▄   ▌
    ░▓█▒░██▓ ▓█   ▓██▒░██████▒░▒████▒░▓█▒░██▓░ ████▓▒░▒▒█████▓ ▒██░   ▓██░░▒████▓
     ▒ ░░▒░▒ ▒▒   ▓▒█░░ ▒░▓  ░░░ ▒░ ░ ▒ ░░▒░▒░ ▒░▒░▒░ ░▒▓▒ ▒ ▒ ░ ▒░   ▒ ▒  ▒▒▓  ▒
     ▒ ░▒░ ░  ▒   ▒▒ ░░ ░ ▒  ░ ░ ░  ░ ▒ ░▒░ ░  ░ ▒ ▒░ ░░▒░ ░ ░ ░ ░░   ░ ▒░ ░ ▒  ▒
     ░  ░░ ░  ░   ▒     ░ ░      ░    ░  ░░ ░░ ░ ░ ▒   ░░░ ░ ░    ░   ░ ░  ░ ░  ░
     ░  ░  ░      ░  ░    ░  ░   ░  ░ ░  ░  ░    ░ ░     ░              ░    ░
    
              _                                                   _
         .k$$$$$g,                                           ,g$$$$$k.
      .k$$$$$$$$$$$a.                                     .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                 ,?^?$?`   `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                               ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                              j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                              :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'  ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',  .                                .    ,'       _ ?kiSSI?
    ?i$?` _  k$        .:                              :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?
```

# HaleHound-CYD v3.6.2

A stability pass — several white-screen crashes squashed across Deauth, Wardriving, and Packet Monitor, plus a leaner build.

## What's New

**Lighter build** — de-duplicated the skull artwork, fonts, and icons, reclaiming ~180 KB of flash for headroom.

**Tidier menu** — removed a redundant "Saved Captures" shortcut; your captures still live under Loot.

## Fixes

**Deauth no longer white-screens under load** — a heavy transmit burst could crash Deauth to a white screen while it tried to restart its radio. The transmit-buffer backpressure that set off the restart loop is handled cleanly now, so it powers through.

**Deauth keeps running after other modules** — launching Deauth right after a BLE or 2.4 GHz tool used to make it quit on its own a few seconds later. It was short on memory; that's freed up, and it stays alive.

**Wardriving survives BLE scanning** — the combined WiFi + BLE wardrive could white-screen a dozen networks in. The BLE scanner now sets up once and tears down cleanly instead of leaking memory each pass.

**Packet Monitor arrows are back** — the channel up/down arrows were getting buried by the background skull graphic. The skull now renders behind them.

## Boards

Four targets in the flash package. Each gets an app bin (update over an existing install) and a `-FULL` bin (bootloader + partitions + app, for a fresh flash):

- **HaleHound-CYD** — 2.8" CYD (ESP32-2432S028, CH340 USB)
- **HaleHound-CYD-HAT** — 2.8" CYD wired for the NM RF Hat
- **HaleHound-E32R28T** — QDtech E32R28T (2.8", Type-C, battery)
- **HaleHound-E32R35T** — QDtech E32R35T (3.5")

```
                                                            ⣀⡀⣀⢀⡀⢀⡀⣀    ⣀⢀⡀ ⡀⢀ ⣀⢀⡀⢀⡀
                                                     ⢀⡀⣛⠘⠿⢦⠤⣽⢟⣿⢻⡟⣾⡏⣹⣿⣿⢻⡟⣿⣾⣿⣿⡿⣿⢿⣿⣌⣿⢸⡷⢤⡄⢚⢀⡀
                                              ⣀⢀⣀⣀⣀⢀⣀⣈⣁⣉⣀⣀⣀⣀⣉⣈⣁⣀⣀⣀⣀⣉⣀⣁⣀⣁⣉⣉⣀⣈⣀⣀⣀⣉⣈⣉⣀⣀⣀⣀⣈⣈⣁⣀⣀⣀ ⣀
                                           ⣤⠐⢶⢻⡟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⡇
                                       ⢀⣀⣠⣄⣤⣀⣤⣤⣤⣤⣤⣤⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣍⣭⣭⣭⣭⣥⣭⣭⣭⣭⣥⣭⣭⣭⣬⣭⣭⣥⣤⣤⣤⣤⣤⣤⣄⣤⣀⣄⣀⡀
                                      ⣴⠸⠟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⠿⠿⠉⠉
                                  ⢀⡀⣶⣦⣶⣴⣶⣴⣤⣴⣶⣶⣼⣦⣿⣶⣾⣶⣶⣾⣧⣿⣾⣷⣾⣷⣿⣶⣶⣶⣶⣶⣦⣶⣶⣶⣶⣷⣿⣶⣶⣶⣶⣶⣶⣿⣾⣷⣴⣦⣶⣦⣤⣤⣦⣴⣦⣴⢠⣦⣤⡆⣴⣤⣤⣤⣤⣤⣄
                                  ⠸⠇⠿⠻⢿⠿⠿⠿⠿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠟⠛⠃
                               ⡤⢰⣶⣶⣶⣾⣷⣿⣶⣿⣾⣷⣿⣶⣿⣶⣷⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣷⣿⣿⣿⣾⣿⣿⣷⣿⣾⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣾⣷⣿⣧⣶⣰⣶⣤⣤⣤⣴⣶⣴⣶⣶⣶⣤⣶⣶
                               ⠛⠘⠛⠛⠛⢿⠿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢿⠿⠿⠿⠿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⣿⡿⣿⠿⠿⠛⠛⠛⠛⠛⠛⠛⠛⠋⠛⠛⠛⠛⠛⠛⠁⠛⠘⠃⠐⠂
                           ⢠⠶⣾⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣾⣿⣿⣷⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣶⣿⣾⣷⣿⣷⣾⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⠶⣶⣶⣦⣤⣀
                             ⠉⠉⠛⠈⠛⠙⠋⠙⠛⠛⠛⠛⣿⠿⢿⣻⣿⢿⡟⢿⠿⠿⠿⠿⢿⡿⣿⠿⠿⢿⡿⣿⠿⣿⢿⡿⢿⡟⠿⠿⣿⣿⡿⣿⠿⣿⢿⡟⠿⠿⠿⠻⡿⢿⡟⠛⠛⠛⠛⠛⠛⠛⠋⠙⠋⠛⠋⠛⠉⠛⠋ ⠘⠁⠉ ⠋⠉⠁⠈⠉⠉
                        ⢰⢿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⡿⣿⣿⡾⢻⡟⣿⠿⣿ ⡆
                       ⣀⢀⡀⣀⡘⢋⣀ ⣉⣉⣉⣈⣁⣙⡛⣁⣉⣉⣉⣉⣛⢛⣛⣛⣋⣛⣛⣛⣻⣿⣿⡟⣿⣿⣿⣿⡟⣿⣿⣿⣻⣟⣻⣛⣛⢛⣻⣿⣟⣛⣿⣿⣻⡟⣿⣟⣿⢛⣛⢛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣉⡋⣛⡙⣛⠉⡀⠘⠁⠉⡈⠁⣈⡁⢉ ⠁⠈⠃
                     ⠋⠠⠏⠘⠁⡿⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⡿⣿⢻⡿⣹⡇⣿ ⣓ ⡀
                        ⢀⣀⣀⣈⣉⣩⣅⣄⣀⣭⣉⣍⣁⣈⣉⣩⣍⣩⣉⣭⣭⣿⣩⣍⣽⣭⣩⣭⣭⣭⣍⣭⣉⣍⣩⣍⣭⣉⣭⣩⣍⣩⣉⣭⣩⣍⣩⣍⣩⣉⣉⣉⣍⣉⣉⣭⣉⣉⣉⣍⣉⣉⣉⣩⣁⣉⢉⣉⢈⡁⣉⣀⣉⣉⣉⣁⣀⣈⣀⣈⣀⣁⣉⡁⠈ ⠁ ⠁
                       ⠂⠚⠉⠛⠩⠟⠟⠻⣿⢿⣿⢿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⡿⠿⣿⢿⣿⣿⡿⣿⣿⣿⢹⣿⠿⠏⣿⢩⣿⣿⣿⣿⣿⣿⢿⡟⢹⡟⣤
                        ⣠⣤⣤⣤⣤⣤⣤⣴⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣭⣼⣿⣿⣿⣿⣭⣭⣭⣭⣭⣭⣭⣤⣤⣤⣤⣤⣬⣥⣤⣄⣬⣭⣥⣬⣥⣭⣅⣤⣤⣬⣤⣄⣠⣄⣤ ⣤⣠⣤⣤⣴⣮⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⣠⢀⣄⣤⣤⣤⣠⣤⣤⣀
                      ⠈⠉⠛⠛⠿⠿⠛⠿⠿⠿⠿⠿⢿⠿⣿⠟⠿⠸⠟⢹⡇⢿⠘⠿⠻⠿⠟⠻⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⣿⢿⡿⢿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢻⡟⠛⠃⠛ ⠚⠘⠏⠛⠙⠛⠻⠿⠿⠿⠛⠹⠟⠙⠃⣿⠰⡾⠛⠋⠿⠣⣿⠻⡿⠻⠇⠰
                        ⢴⣤⣶⣶⣶⣶⣦⣶⣤⣤⣴⣤⢤⡄            ⣤⢠⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣦⣶⣤⣤⣴⣶⣶⣶⣶⣶⣶⣦⣤⣠⣄         ⣤⠠⡆⠠⣤⣤⢠⣶⣶⡶⣤⡤⢤       ⣠⡄⣀
                        ⠘⠋⠛⠛⠉⠋⠉⠿⠉⠿ ⠁              ⠉⠈⠁⠉⠉⠉⠉⠉⠉⠉⠋⠹⠿⠻⠏⠻⠿⠿⠻⠿⠿⠿⠟⠛⠛⠃           ⠈⠁⠘⠁⠉⠉⠛⠙⠃⠛ ⠋     ⠰⠆⠈ ⠴
                        ⢀⡶⣶⡐⢶ ⠆                           ⠠⠤⣦ ⣤⢲⣶⣶⣶⣶⣤⣤⢴⣦⢶⣶⣤⣶⣶                ⡴⢐⡆⢶⣰⣶
                        ⠈⠁⠋ ⠂  ⠛                            ⠋⠘⠛⠘⠋⠛⠛⠋⠙⠛⠛⠋⠚⠋⠛⠘⠃                 ⠈⠁⠈⠁          ⠚
                                                          ⢰⣷⣦⣴⣿⣾⣷⣶⣦        ⢰⡆⢲⡆
                               ⠛                          ⠉⠁⠉⠉⠋⠙⠋⠁         ⠈⠁⠈⠁                             ⠛
                               ⣀ ⣀                   ⢀ ⣸⠹⣿⣿⡟⣿⢿⣿⢿⡽⠿⠞⢿⠃        ⣹⡟⣿⠇⣀                        ⢀⡀⣀
                               ⣉ ⠉                   ⢀ ⡀⢀⣀⢈⡁⣉ ⠉⠈⠁  ⢀ ⡀       ⠉ ⠈ ⡉ ⡀                      ⠈⠁⢉
                             ⢘⡃⣿ ⣛⢀⡀                  ⠈⣧⢸⡿⢿⣷⣹⡇     ⠘⠈⠃        ⠠⠶⢶⡿⢹⡟⣀                   ⢀⡀⢘⡇⣿ ⣟⢀⡀⢀⡀
                             ⣀⣀⣀⣀⣁⢈⠁               ⢠⣄⣀⣀⣉⣈⣀⣈⣁⠉        ⡀ ⡀          ⢀⡀⣈⣀⣄⢀⡀⢀⡀             ⢀⣁⣀⣁⣈⣀⣁ ⡁⠈
                           ⢠⡤⢼⣿⣿⣯⡿⠸⡇⣤⠄⢤      ⣤⢠⡄⢿⠶⠿⢾⡟⣿⡏⣿⢫⣿⠹⡏⣤         ⠐⠃⠠      ⣤⠐⠶⠸⡇⢿⠻⠿⠸⡇⠙⠃⣤⠠⡤       ⢤⡄⣤⠈⠛⢽⠏⢿⠛⠿⠠⡇⢤⠄
                             ⣠⣄⣤⣤⣤⣤⣤⣤⣄   ⣠⡄⣤⣀⣤⢀⣄⣠⣤⣄⢀⣄⡀     ⡀          ⢀ ⣠⡄⢀         ⣤⣀⣤        ⡀⢀ ⡀ ⡀⣠⣤⣤⣤⣤⣤⣤⣀⣤⣤
                      ⠐ ⠒ ⣶⠰⠦⠉⠹⠟⢹⡿⠿⠋⠾⠛⠿⠛⠿⠉⠉⠉⠉⠉⠙⠛⢿⠟⢿⠸⡟⠻⠟⠛⠿⠟⠘⠇          ⠉⠁⠈ ⠈ ⠂  ⠶ ⠆⠘⠏⠉⠉⠉⠰⠆⠰⠆⠴⠈⠇⠘⠇⠿ ⡿⠘⠇⠉⠁⠿⠹⡿⠿⠋⠿⠉⠉⠰⠆⠰⠂
                     ⣶⣴⣶⢶⡄⣀  ⢀⡀   ⢀⡀       ⢤⢠⣦⣤⡴⠲⡦⢤    ⣀                ⢀ ⣤⢀⡄⣠⡄⣠ ⡄              ⠴⠄⣶⣦⡴⢦  ⢠⣦⣶⣦⣴⣄⣀  ⣀⡀
                     ⠉⠛⠛⠿⠆⠿⠰⠇⠰⠟⠛⠛⠛⠰⠆⠶⠆       ⠛⠈⠁⠘⠃⠈  ⠰⠆⠴ ⠦⠰⠆            ⠈ ⠉⠈⠁  ⠉⠰⠷⠰⠇⠶ ⠶           ⠁    ⠾ ⠁⠸⠏  ⠶⠰⠆⠰ ⠶
                    ⠰⢶⣶⣿⣷⣶⣶⢶⣶⣶⣄⣀⡠⠦⠰⠦⠶⠄⠄       ⣀⡀          ⠠⡄⣤              ⠰⠦⠶⢤⣶⣤⡤⠠⡄⢤       ⠠⠶⣺⡷⣶⣶⣶⢶⡶⠶⠄        ⠠⣤⡤
                      ⠉⠉⠙⠛⠛⠚⠛⠉⠈⠛                   ⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛                   ⠙⠛⠘⠃⠛ ⠛⠘⠃     ⠈ ⠘⠉⠁ ⠁        ⠐⠛  ⠛⠂
                      ⠤⠤⢤⠄                             ⢀⠰⣶⠴⠦⣶⡆⣶⢠⡄  ⢀   ⡀  ⣀ ⡀⣶⡆⣶⣦⣤⢰⡦⢶⠆⣠         ⣒⢂⣶⢐⡒⠂
                                                     ⠘⠁⠉    ⠉ ⠉⠈⠁⠈⠁⠉⠈⠉⠈⠃  ⠙⠈⠃⠈⠃⠉ ⠉⠈⠁  ⠉         ⠉⠈⠛⠈⠃
                           ⢀⡀⢀⡀⣀                     ⢀⡀⣿⢈⣿⠸⣇⣴⣴⣦⣴⣦⣹⡇⣶    ⣶⠆⣿⣶⣦⣶⡄⣿⢈⣿⣿⡿⢿⡿⠿                        ⢀⡀
                           ⠈⠁⠈⠁⠉                ⣀ ⢀⢀⡀⢈⡁⣈⣈⣁⣈⣁⣀    ⠉ ⢉   ⡀⣉ ⢉  ⢀⣀⠉⢈⣁⣈⡀⣉⠁⣀                        ⠈⠁
                           ⢀⡀⢀⡀                 ⠉⠃⠉⠘⠋⢻⣿⣿⣿⣿⣻⡟⣿⠰⠶⠶⠶⣶⡶⠿⢻⣄⢀⡟⠿⠿⠿⠾⠷⣿⡟⣿⢻⡿⣿⣿⣿⣟⣿⢀⡄
                             ⠈⠁                ⡀⣠⡄⣠⢀⡀⣀⡅⣈⣩⣭⢀⡀⢀    ⠠⡀ ⠈⠉ ⡅⢠⣤⣤⣠⡄⣈⡀⣀ ⣄⣈⡀⣈⡉⠁ ⡁
                                               ⠁⠿⠁⠼⢸⡿⠻⡿⢿⢿⠟⠛⠛⣿⠤⣤   ⠁⠚⠛⠓ ⠁⠛⠹⣿⢩⡗⢤⡜⠋⠻⠟⢽⠇⢼⠟⠻⠺⡇
                                              ⣤⣄⢠⡄⣤⢀⡄      ⠄   ⢠⡄   ⢀⡀⣤⣴⣦⣤⣤⣴⣆⢰⡄ ⢀⡀
                                              ⠛⠛⠚⠳⣿⠸⡇⠺⠏⠈⠁ ⠰⠆     ⠻⠏⢿⠈⠁⠈⠋⠛⠙⢿⠻⡿⠒⠁⠶⠈⠻⠠⠆  ⠶  ⠰⠆
                                              ⢰⡆⣴⡆⠤                   ⢰⣦⣴⡶⣶⢶⡦⣄ ⣀
                                              ⠈⠁   ⠸⠇⠺⠇⠾           ⠿    ⠉ ⠛⠘⠋⠈⠁⠿ ⠶  ⠶ ⠶
                                                                        ⣀⡀

                                                                     ⡀⢀⡀
                                                                    ⠈⠉⠈⠁
                                                               ⢀⡀⢀⡀  ⡶⢰⡆
                                                               ⠈⠁⠈⠁ ⠈⠉⠈⠁


                                                   ⠠⡤   ⠠⡤⠠⡄⢤

                                                   ⠰⠆⠰⠆⠶⠐⠶⠰⠆⠶
                                                                     ⣄
                                                       ⠺⠐⠇⠐⠆⠲ ⠶⠐⠇⠰⠂⠲ ⠇⠐⠇⠲ ⠲⠐⠆⠸⠂⠾ ⠖

                                                        ⠘⠛⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠛⠐⠃
```
========================================================================================
## v3.7.0 | 2026-07-08
```
              _                                                    _
         .k$$$$$g,                                             ,g$$$$$k.
      .k$$$$$$$$$$$a.                                       .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                  ,?^?$?`    `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                                ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                               j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                                :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'   ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',    `                              .    ,'       _ ?kiSSI?
    ?i$?` _   k$        .                               :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?

     ██░ ██  ▄▄▄       ██▓    ▓█████  ██░ ██  ▒█████   █    ██  ███▄    █ ▓█████▄
    ▓██░ ██▒▒████▄    ▓██▒    ▓█   ▀ ▓██░ ██▒▒██▒  ██▒ ██  ▓██▒ ██ ▀█   █ ▒██▀ ██▌
    ▒██▀▀██░▒██  ▀█▄  ▒██░    ▒███   ▒██▀▀██░▒██░  ██▒▓██  ▒██░▓██  ▀█ ██▒░██   █▌
    ░▓█ ░██ ░██▄▄▄▄██ ▒██░    ▒▓█  ▄ ░▓█ ░██ ▒██   ██░▓▓█  ░██░▓██▒  ▐▌██▒░▓█▄   ▌
    ░▓█▒░██▓ ▓█   ▓██▒░██████▒░▒████▒░▓█▒░██▓░ ████▓▒░▒▒█████▓ ▒██░   ▓██░░▒████▓
     ▒ ░░▒░▒ ▒▒   ▓▒█░░ ▒░▓  ░░░ ▒░ ░ ▒ ░░▒░▒░ ▒░▒░▒░ ░▒▓▒ ▒ ▒ ░ ▒░   ▒ ▒  ▒▒▓  ▒
     ▒ ░▒░ ░  ▒   ▒▒ ░░ ░ ▒  ░ ░ ░  ░ ▒ ░▒░ ░  ░ ▒ ▒░ ░░▒░ ░ ░ ░ ░░   ░ ▒░ ░ ▒  ▒
     ░  ░░ ░  ░   ▒     ░ ░      ░    ░  ░░ ░░ ░ ░ ▒   ░░░ ░ ░    ░   ░ ░  ░ ░  ░
     ░  ░  ░      ░  ░    ░  ░   ░  ░ ░  ░  ░    ░ ░     ░              ░    ░
    
              _                                                   _
         .k$$$$$g,                                           ,g$$$$$k.
      .k$$$$$$$$$$$a.                                     .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                 ,?^?$?`   `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                               ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                              j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                              :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'  ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',  .                                .    ,'       _ ?kiSSI?
    ?i$?` _  k$        .:                              :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?
```

# HaleHound-CYD v3.7.0

A big one — a whole new BLE tool, dual-core wardriving, and a major IoT Recon upgrade.

## What's New

**SkeletonKey — new BLE module.** Connect to any Bluetooth Low Energy device, map every service and characteristic, read its values, and send commands. Built-in profiles for common BLE gear (ELK-BLEDOM / Triones RGB controllers) give one-tap control, and it auto-identifies device types (remote, watch, sensor, light…) right in the scan list.

**Dual-core wardriving.** WiFi scanning now runs on its own core while the UI stays smooth — roughly 2× the scan rate, with a live nets/min RATE meter.

**IoT Recon overhaul.** Vendor fingerprinting (Hikvision, Dahua, Uniview, Amcrest), brand-aware credential ordering, unlimited SD-streamed wordlists, Sweep/Deep scan modes, and real RTSP/HTTP Digest auth with crack verification.

**AirTag Scream** — detection and UI improvements.

## Fixes

**3.5" re-calibration on every boot** — the E32R35T saved its cal matrix but not the "done" flag, so it re-ran calibration at every power-up. Calibrate once now and it sticks.

**BLE stability** — fixed a white-screen when switching between BLE Predator and other BT/WiFi modules; hardened WhisperPair's SD save on marginal cards.

**Wardriving stability** — eliminated the dense-RF-area crashes.

## Boards
E32R35T (3.5"), CYD (2.8"), CYD-HAT, E32R28T.

```
                                                            ⣀⡀⣀⢀⡀⢀⡀⣀    ⣀⢀⡀ ⡀⢀ ⣀⢀⡀⢀⡀
                                                     ⢀⡀⣛⠘⠿⢦⠤⣽⢟⣿⢻⡟⣾⡏⣹⣿⣿⢻⡟⣿⣾⣿⣿⡿⣿⢿⣿⣌⣿⢸⡷⢤⡄⢚⢀⡀
                                              ⣀⢀⣀⣀⣀⢀⣀⣈⣁⣉⣀⣀⣀⣀⣉⣈⣁⣀⣀⣀⣀⣉⣀⣁⣀⣁⣉⣉⣀⣈⣀⣀⣀⣉⣈⣉⣀⣀⣀⣀⣈⣈⣁⣀⣀⣀ ⣀
                                           ⣤⠐⢶⢻⡟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⡇
                                       ⢀⣀⣠⣄⣤⣀⣤⣤⣤⣤⣤⣤⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣍⣭⣭⣭⣭⣥⣭⣭⣭⣭⣥⣭⣭⣭⣬⣭⣭⣥⣤⣤⣤⣤⣤⣤⣄⣤⣀⣄⣀⡀
                                      ⣴⠸⠟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⠿⠿⠉⠉
                                  ⢀⡀⣶⣦⣶⣴⣶⣴⣤⣴⣶⣶⣼⣦⣿⣶⣾⣶⣶⣾⣧⣿⣾⣷⣾⣷⣿⣶⣶⣶⣶⣶⣦⣶⣶⣶⣶⣷⣿⣶⣶⣶⣶⣶⣶⣿⣾⣷⣴⣦⣶⣦⣤⣤⣦⣴⣦⣴⢠⣦⣤⡆⣴⣤⣤⣤⣤⣤⣄
                                  ⠸⠇⠿⠻⢿⠿⠿⠿⠿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠟⠛⠃
                               ⡤⢰⣶⣶⣶⣾⣷⣿⣶⣿⣾⣷⣿⣶⣿⣶⣷⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣷⣿⣿⣿⣾⣿⣿⣷⣿⣾⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣾⣷⣿⣧⣶⣰⣶⣤⣤⣤⣴⣶⣴⣶⣶⣶⣤⣶⣶
                               ⠛⠘⠛⠛⠛⢿⠿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢿⠿⠿⠿⠿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⣿⡿⣿⠿⠿⠛⠛⠛⠛⠛⠛⠛⠛⠋⠛⠛⠛⠛⠛⠛⠁⠛⠘⠃⠐⠂
                           ⢠⠶⣾⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣾⣿⣿⣷⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣶⣿⣾⣷⣿⣷⣾⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⠶⣶⣶⣦⣤⣀
                             ⠉⠉⠛⠈⠛⠙⠋⠙⠛⠛⠛⠛⣿⠿⢿⣻⣿⢿⡟⢿⠿⠿⠿⠿⢿⡿⣿⠿⠿⢿⡿⣿⠿⣿⢿⡿⢿⡟⠿⠿⣿⣿⡿⣿⠿⣿⢿⡟⠿⠿⠿⠻⡿⢿⡟⠛⠛⠛⠛⠛⠛⠛⠋⠙⠋⠛⠋⠛⠉⠛⠋ ⠘⠁⠉ ⠋⠉⠁⠈⠉⠉
                        ⢰⢿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⡿⣿⣿⡾⢻⡟⣿⠿⣿ ⡆
                       ⣀⢀⡀⣀⡘⢋⣀ ⣉⣉⣉⣈⣁⣙⡛⣁⣉⣉⣉⣉⣛⢛⣛⣛⣋⣛⣛⣛⣻⣿⣿⡟⣿⣿⣿⣿⡟⣿⣿⣿⣻⣟⣻⣛⣛⢛⣻⣿⣟⣛⣿⣿⣻⡟⣿⣟⣿⢛⣛⢛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣉⡋⣛⡙⣛⠉⡀⠘⠁⠉⡈⠁⣈⡁⢉ ⠁⠈⠃
                     ⠋⠠⠏⠘⠁⡿⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⡿⣿⢻⡿⣹⡇⣿ ⣓ ⡀
                        ⢀⣀⣀⣈⣉⣩⣅⣄⣀⣭⣉⣍⣁⣈⣉⣩⣍⣩⣉⣭⣭⣿⣩⣍⣽⣭⣩⣭⣭⣭⣍⣭⣉⣍⣩⣍⣭⣉⣭⣩⣍⣩⣉⣭⣩⣍⣩⣍⣩⣉⣉⣉⣍⣉⣉⣭⣉⣉⣉⣍⣉⣉⣉⣩⣁⣉⢉⣉⢈⡁⣉⣀⣉⣉⣉⣁⣀⣈⣀⣈⣀⣁⣉⡁⠈ ⠁ ⠁
                       ⠂⠚⠉⠛⠩⠟⠟⠻⣿⢿⣿⢿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⡿⠿⣿⢿⣿⣿⡿⣿⣿⣿⢹⣿⠿⠏⣿⢩⣿⣿⣿⣿⣿⣿⢿⡟⢹⡟⣤
                        ⣠⣤⣤⣤⣤⣤⣤⣴⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣭⣼⣿⣿⣿⣿⣭⣭⣭⣭⣭⣭⣭⣤⣤⣤⣤⣤⣬⣥⣤⣄⣬⣭⣥⣬⣥⣭⣅⣤⣤⣬⣤⣄⣠⣄⣤ ⣤⣠⣤⣤⣴⣮⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⣠⢀⣄⣤⣤⣤⣠⣤⣤⣀
                      ⠈⠉⠛⠛⠿⠿⠛⠿⠿⠿⠿⠿⢿⠿⣿⠟⠿⠸⠟⢹⡇⢿⠘⠿⠻⠿⠟⠻⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⣿⢿⡿⢿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢻⡟⠛⠃⠛ ⠚⠘⠏⠛⠙⠛⠻⠿⠿⠿⠛⠹⠟⠙⠃⣿⠰⡾⠛⠋⠿⠣⣿⠻⡿⠻⠇⠰
                        ⢴⣤⣶⣶⣶⣶⣦⣶⣤⣤⣴⣤⢤⡄            ⣤⢠⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣦⣶⣤⣤⣴⣶⣶⣶⣶⣶⣶⣦⣤⣠⣄         ⣤⠠⡆⠠⣤⣤⢠⣶⣶⡶⣤⡤⢤       ⣠⡄⣀
                        ⠘⠋⠛⠛⠉⠋⠉⠿⠉⠿ ⠁              ⠉⠈⠁⠉⠉⠉⠉⠉⠉⠉⠋⠹⠿⠻⠏⠻⠿⠿⠻⠿⠿⠿⠟⠛⠛⠃           ⠈⠁⠘⠁⠉⠉⠛⠙⠃⠛ ⠋     ⠰⠆⠈ ⠴
                        ⢀⡶⣶⡐⢶ ⠆                           ⠠⠤⣦ ⣤⢲⣶⣶⣶⣶⣤⣤⢴⣦⢶⣶⣤⣶⣶                ⡴⢐⡆⢶⣰⣶
                        ⠈⠁⠋ ⠂  ⠛                            ⠋⠘⠛⠘⠋⠛⠛⠋⠙⠛⠛⠋⠚⠋⠛⠘⠃                 ⠈⠁⠈⠁          ⠚
                                                          ⢰⣷⣦⣴⣿⣾⣷⣶⣦        ⢰⡆⢲⡆
                               ⠛                          ⠉⠁⠉⠉⠋⠙⠋⠁         ⠈⠁⠈⠁                             ⠛
                               ⣀ ⣀                   ⢀ ⣸⠹⣿⣿⡟⣿⢿⣿⢿⡽⠿⠞⢿⠃        ⣹⡟⣿⠇⣀                        ⢀⡀⣀
                               ⣉ ⠉                   ⢀ ⡀⢀⣀⢈⡁⣉ ⠉⠈⠁  ⢀ ⡀       ⠉ ⠈ ⡉ ⡀                      ⠈⠁⢉
                             ⢘⡃⣿ ⣛⢀⡀                  ⠈⣧⢸⡿⢿⣷⣹⡇     ⠘⠈⠃        ⠠⠶⢶⡿⢹⡟⣀                   ⢀⡀⢘⡇⣿ ⣟⢀⡀⢀⡀
                             ⣀⣀⣀⣀⣁⢈⠁               ⢠⣄⣀⣀⣉⣈⣀⣈⣁⠉        ⡀ ⡀          ⢀⡀⣈⣀⣄⢀⡀⢀⡀             ⢀⣁⣀⣁⣈⣀⣁ ⡁⠈
                           ⢠⡤⢼⣿⣿⣯⡿⠸⡇⣤⠄⢤      ⣤⢠⡄⢿⠶⠿⢾⡟⣿⡏⣿⢫⣿⠹⡏⣤         ⠐⠃⠠      ⣤⠐⠶⠸⡇⢿⠻⠿⠸⡇⠙⠃⣤⠠⡤       ⢤⡄⣤⠈⠛⢽⠏⢿⠛⠿⠠⡇⢤⠄
                             ⣠⣄⣤⣤⣤⣤⣤⣤⣄   ⣠⡄⣤⣀⣤⢀⣄⣠⣤⣄⢀⣄⡀     ⡀          ⢀ ⣠⡄⢀         ⣤⣀⣤        ⡀⢀ ⡀ ⡀⣠⣤⣤⣤⣤⣤⣤⣀⣤⣤
                      ⠐ ⠒ ⣶⠰⠦⠉⠹⠟⢹⡿⠿⠋⠾⠛⠿⠛⠿⠉⠉⠉⠉⠉⠙⠛⢿⠟⢿⠸⡟⠻⠟⠛⠿⠟⠘⠇          ⠉⠁⠈ ⠈ ⠂  ⠶ ⠆⠘⠏⠉⠉⠉⠰⠆⠰⠆⠴⠈⠇⠘⠇⠿ ⡿⠘⠇⠉⠁⠿⠹⡿⠿⠋⠿⠉⠉⠰⠆⠰⠂
                     ⣶⣴⣶⢶⡄⣀  ⢀⡀   ⢀⡀       ⢤⢠⣦⣤⡴⠲⡦⢤    ⣀                ⢀ ⣤⢀⡄⣠⡄⣠ ⡄              ⠴⠄⣶⣦⡴⢦  ⢠⣦⣶⣦⣴⣄⣀  ⣀⡀
                     ⠉⠛⠛⠿⠆⠿⠰⠇⠰⠟⠛⠛⠛⠰⠆⠶⠆       ⠛⠈⠁⠘⠃⠈  ⠰⠆⠴ ⠦⠰⠆            ⠈ ⠉⠈⠁  ⠉⠰⠷⠰⠇⠶ ⠶           ⠁    ⠾ ⠁⠸⠏  ⠶⠰⠆⠰ ⠶
                    ⠰⢶⣶⣿⣷⣶⣶⢶⣶⣶⣄⣀⡠⠦⠰⠦⠶⠄⠄       ⣀⡀          ⠠⡄⣤              ⠰⠦⠶⢤⣶⣤⡤⠠⡄⢤       ⠠⠶⣺⡷⣶⣶⣶⢶⡶⠶⠄        ⠠⣤⡤
                      ⠉⠉⠙⠛⠛⠚⠛⠉⠈⠛                   ⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛                   ⠙⠛⠘⠃⠛ ⠛⠘⠃     ⠈ ⠘⠉⠁ ⠁        ⠐⠛  ⠛⠂
                      ⠤⠤⢤⠄                             ⢀⠰⣶⠴⠦⣶⡆⣶⢠⡄  ⢀   ⡀  ⣀ ⡀⣶⡆⣶⣦⣤⢰⡦⢶⠆⣠         ⣒⢂⣶⢐⡒⠂
                                                     ⠘⠁⠉    ⠉ ⠉⠈⠁⠈⠁⠉⠈⠉⠈⠃  ⠙⠈⠃⠈⠃⠉ ⠉⠈⠁  ⠉         ⠉⠈⠛⠈⠃
                           ⢀⡀⢀⡀⣀                     ⢀⡀⣿⢈⣿⠸⣇⣴⣴⣦⣴⣦⣹⡇⣶    ⣶⠆⣿⣶⣦⣶⡄⣿⢈⣿⣿⡿⢿⡿⠿                        ⢀⡀
                           ⠈⠁⠈⠁⠉                ⣀ ⢀⢀⡀⢈⡁⣈⣈⣁⣈⣁⣀    ⠉ ⢉   ⡀⣉ ⢉  ⢀⣀⠉⢈⣁⣈⡀⣉⠁⣀                        ⠈⠁
                           ⢀⡀⢀⡀                 ⠉⠃⠉⠘⠋⢻⣿⣿⣿⣿⣻⡟⣿⠰⠶⠶⠶⣶⡶⠿⢻⣄⢀⡟⠿⠿⠿⠾⠷⣿⡟⣿⢻⡿⣿⣿⣿⣟⣿⢀⡄
                             ⠈⠁                ⡀⣠⡄⣠⢀⡀⣀⡅⣈⣩⣭⢀⡀⢀    ⠠⡀ ⠈⠉ ⡅⢠⣤⣤⣠⡄⣈⡀⣀ ⣄⣈⡀⣈⡉⠁ ⡁
                                               ⠁⠿⠁⠼⢸⡿⠻⡿⢿⢿⠟⠛⠛⣿⠤⣤   ⠁⠚⠛⠓ ⠁⠛⠹⣿⢩⡗⢤⡜⠋⠻⠟⢽⠇⢼⠟⠻⠺⡇
                                              ⣤⣄⢠⡄⣤⢀⡄      ⠄   ⢠⡄   ⢀⡀⣤⣴⣦⣤⣤⣴⣆⢰⡄ ⢀⡀
                                              ⠛⠛⠚⠳⣿⠸⡇⠺⠏⠈⠁ ⠰⠆     ⠻⠏⢿⠈⠁⠈⠋⠛⠙⢿⠻⡿⠒⠁⠶⠈⠻⠠⠆  ⠶  ⠰⠆
                                              ⢰⡆⣴⡆⠤                   ⢰⣦⣴⡶⣶⢶⡦⣄ ⣀
                                              ⠈⠁   ⠸⠇⠺⠇⠾           ⠿    ⠉ ⠛⠘⠋⠈⠁⠿ ⠶  ⠶ ⠶
                                                                        ⣀⡀

                                                                     ⡀⢀⡀
                                                                    ⠈⠉⠈⠁
                                                               ⢀⡀⢀⡀  ⡶⢰⡆
                                                               ⠈⠁⠈⠁ ⠈⠉⠈⠁


                                                   ⠠⡤   ⠠⡤⠠⡄⢤

                                                   ⠰⠆⠰⠆⠶⠐⠶⠰⠆⠶
                                                                     ⣄
                                                       ⠺⠐⠇⠐⠆⠲ ⠶⠐⠇⠰⠂⠲ ⠇⠐⠇⠲ ⠲⠐⠆⠸⠂⠾ ⠖

                                                        ⠘⠛⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠛⠐⠃
```
========================================================================================
## v3.7.1 | 2026-07-09
```
              _                                                    _
         .k$$$$$g,                                             ,g$$$$$k.
      .k$$$$$$$$$$$a.                                       .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                  ,?^?$?`    `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                                ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                               j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                                :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'   ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',    `                              .    ,'       _ ?kiSSI?
    ?i$?` _   k$        .                               :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?

     ██░ ██  ▄▄▄       ██▓    ▓█████  ██░ ██  ▒█████   █    ██  ███▄    █ ▓█████▄
    ▓██░ ██▒▒████▄    ▓██▒    ▓█   ▀ ▓██░ ██▒▒██▒  ██▒ ██  ▓██▒ ██ ▀█   █ ▒██▀ ██▌
    ▒██▀▀██░▒██  ▀█▄  ▒██░    ▒███   ▒██▀▀██░▒██░  ██▒▓██  ▒██░▓██  ▀█ ██▒░██   █▌
    ░▓█ ░██ ░██▄▄▄▄██ ▒██░    ▒▓█  ▄ ░▓█ ░██ ▒██   ██░▓▓█  ░██░▓██▒  ▐▌██▒░▓█▄   ▌
    ░▓█▒░██▓ ▓█   ▓██▒░██████▒░▒████▒░▓█▒░██▓░ ████▓▒░▒▒█████▓ ▒██░   ▓██░░▒████▓
     ▒ ░░▒░▒ ▒▒   ▓▒█░░ ▒░▓  ░░░ ▒░ ░ ▒ ░░▒░▒░ ▒░▒░▒░ ░▒▓▒ ▒ ▒ ░ ▒░   ▒ ▒  ▒▒▓  ▒
     ▒ ░▒░ ░  ▒   ▒▒ ░░ ░ ▒  ░ ░ ░  ░ ▒ ░▒░ ░  ░ ▒ ▒░ ░░▒░ ░ ░ ░ ░░   ░ ▒░ ░ ▒  ▒
     ░  ░░ ░  ░   ▒     ░ ░      ░    ░  ░░ ░░ ░ ░ ▒   ░░░ ░ ░    ░   ░ ░  ░ ░  ░
     ░  ░  ░      ░  ░    ░  ░   ░  ░ ░  ░  ░    ░ ░     ░              ░    ░
    
              _                                                   _
         .k$$$$$g,                                           ,g$$$$$k.
      .k$$$$$$$$$$$a.                                     .a$$$$$$$$$$$k.
    .J$$$$$?'   `?$?^?,                                 ,?^?$?`   `?$$$$$L.
   JS$$SI!a,  _.JS$   ?,                               ,?   $SL._  ,a$!IS$$SL
  k$$$SI!:?$$$$$$$$$xu$$j                              j$$ux$$$$$$$$$?:!IS$$$k
 :I$$SI:J$$?*"$$$$4^?*?:                              :?*?^4$$$$"*?$$L:iIS$$I:
 :IS$$SiJ?`  _.'$?`/'  ':                            :'    '/'?$'._  `?LiS$$SI:
  ?ISSik? _        ',  .                                .    ,'       _ ?kiSSI?
    ?i$?` _  k$        .:                              :.        $k   _ `?$i?
      '?I:-?z$$I   _._.'                                  ._._   I$$z?-:I?'
     '*?- '?$$a louSxuS?                               ?xuSxuol a$$?' -?*'
           i$$$$$$$$$$$S                               S$$$$$$$$$$$i
              ?$$$?-                                       -?$$$?
```

# HaleHound-CYD v3.7.1

A reliability release — the intermittent SD-card read is fixed, plus WiFi-side Flock camera detection and a much wider Flock database.

## Fixes

**SD card "won't read" — fixed.** The big one. After running a radio module (SubGHz, jammer, ProtoKill…), Loot and Saved Captures could intermittently fail to read the SD card — "no card," or slow to open. A leftover radio chip-select was holding the shared SPI bus; the SD now clears the radios before it mounts, so it reads every time. 

**WhisperPair SD hardening.** Dropped a bus-destabilizing call from the save path, so a failed save can no longer knock the SD offline for the next module.

**3.5" Flock You layout.** On the E32R35T the screen no longer bunches everything into the top — the header, stats, and device list now scale to the full panel.

**BLE stability in dense RF.** Extended the advertisement log-flood suppression to two more library files.

## What's New

**Flock You — WiFi detection.** Flock You now spots Flock cameras on WiFi, not just Bluetooth. Tap the center skull to flip between BLE and WiFi; WiFi mode fingerprints the camera's probe requests (transmitter OUI plus the LiteON/Flock vendor element), so a hit is a genuine hit — not a lookalike. Both modes feed the same device list, GPS log, and SD save.

**Wider Flock database.** The Flock OUI list nearly doubled — 22 to 42 prefixes from the community collection — and a bad placeholder prefix was dropped. Better BLE coverage, fewer false alarms.

## Boards
E32R35T (3.5"), CYD (2.8"), CYD-HAT, E32R28T.

```
                                                            ⣀⡀⣀⢀⡀⢀⡀⣀    ⣀⢀⡀ ⡀⢀ ⣀⢀⡀⢀⡀
                                                     ⢀⡀⣛⠘⠿⢦⠤⣽⢟⣿⢻⡟⣾⡏⣹⣿⣿⢻⡟⣿⣾⣿⣿⡿⣿⢿⣿⣌⣿⢸⡷⢤⡄⢚⢀⡀
                                              ⣀⢀⣀⣀⣀⢀⣀⣈⣁⣉⣀⣀⣀⣀⣉⣈⣁⣀⣀⣀⣀⣉⣀⣁⣀⣁⣉⣉⣀⣈⣀⣀⣀⣉⣈⣉⣀⣀⣀⣀⣈⣈⣁⣀⣀⣀ ⣀
                                           ⣤⠐⢶⢻⡟⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⢠⡇
                                       ⢀⣀⣠⣄⣤⣀⣤⣤⣤⣤⣤⣤⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣭⣍⣭⣭⣭⣭⣥⣭⣭⣭⣭⣥⣭⣭⣭⣬⣭⣭⣥⣤⣤⣤⣤⣤⣤⣄⣤⣀⣄⣀⡀
                                      ⣴⠸⠟⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⠿⠿⠉⠉
                                  ⢀⡀⣶⣦⣶⣴⣶⣴⣤⣴⣶⣶⣼⣦⣿⣶⣾⣶⣶⣾⣧⣿⣾⣷⣾⣷⣿⣶⣶⣶⣶⣶⣦⣶⣶⣶⣶⣷⣿⣶⣶⣶⣶⣶⣶⣿⣾⣷⣴⣦⣶⣦⣤⣤⣦⣴⣦⣴⢠⣦⣤⡆⣴⣤⣤⣤⣤⣤⣄
                                  ⠸⠇⠿⠻⢿⠿⠿⠿⠿⡿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠟⠛⠃
                               ⡤⢰⣶⣶⣶⣾⣷⣿⣶⣿⣾⣷⣿⣶⣿⣶⣷⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣷⣿⣿⣿⣾⣿⣿⣷⣿⣾⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣾⣷⣿⣧⣶⣰⣶⣤⣤⣤⣴⣶⣴⣶⣶⣶⣤⣶⣶
                               ⠛⠘⠛⠛⠛⢿⠿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢿⠿⠿⠿⠿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⣿⡿⣿⠿⠿⠛⠛⠛⠛⠛⠛⠛⠛⠋⠛⠛⠛⠛⠛⠛⠁⠛⠘⠃⠐⠂
                           ⢠⠶⣾⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣾⣿⣿⣷⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣶⣿⣾⣷⣿⣷⣾⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⠶⣶⣶⣦⣤⣀
                             ⠉⠉⠛⠈⠛⠙⠋⠙⠛⠛⠛⠛⣿⠿⢿⣻⣿⢿⡟⢿⠿⠿⠿⠿⢿⡿⣿⠿⠿⢿⡿⣿⠿⣿⢿⡿⢿⡟⠿⠿⣿⣿⡿⣿⠿⣿⢿⡟⠿⠿⠿⠻⡿⢿⡟⠛⠛⠛⠛⠛⠛⠛⠋⠙⠋⠛⠋⠛⠉⠛⠋ ⠘⠁⠉ ⠋⠉⠁⠈⠉⠉
                        ⢰⢿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢿⡿⣿⣿⡾⢻⡟⣿⠿⣿ ⡆
                       ⣀⢀⡀⣀⡘⢋⣀ ⣉⣉⣉⣈⣁⣙⡛⣁⣉⣉⣉⣉⣛⢛⣛⣛⣋⣛⣛⣛⣻⣿⣿⡟⣿⣿⣿⣿⡟⣿⣿⣿⣻⣟⣻⣛⣛⢛⣻⣿⣟⣛⣿⣿⣻⡟⣿⣟⣿⢛⣛⢛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣛⣉⡋⣛⡙⣛⠉⡀⠘⠁⠉⡈⠁⣈⡁⢉ ⠁⠈⠃
                     ⠋⠠⠏⠘⠁⡿⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⡿⣿⢻⡿⣹⡇⣿ ⣓ ⡀
                        ⢀⣀⣀⣈⣉⣩⣅⣄⣀⣭⣉⣍⣁⣈⣉⣩⣍⣩⣉⣭⣭⣿⣩⣍⣽⣭⣩⣭⣭⣭⣍⣭⣉⣍⣩⣍⣭⣉⣭⣩⣍⣩⣉⣭⣩⣍⣩⣍⣩⣉⣉⣉⣍⣉⣉⣭⣉⣉⣉⣍⣉⣉⣉⣩⣁⣉⢉⣉⢈⡁⣉⣀⣉⣉⣉⣁⣀⣈⣀⣈⣀⣁⣉⡁⠈ ⠁ ⠁
                       ⠂⠚⠉⠛⠩⠟⠟⠻⣿⢿⣿⢿⣿⣿⣿⣿⣿⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⣿⣿⢿⣿⡿⠿⣿⢿⣿⣿⡿⣿⣿⣿⢹⣿⠿⠏⣿⢩⣿⣿⣿⣿⣿⣿⢿⡟⢹⡟⣤
                        ⣠⣤⣤⣤⣤⣤⣤⣴⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣭⣼⣿⣿⣿⣿⣭⣭⣭⣭⣭⣭⣭⣤⣤⣤⣤⣤⣬⣥⣤⣄⣬⣭⣥⣬⣥⣭⣅⣤⣤⣬⣤⣄⣠⣄⣤ ⣤⣠⣤⣤⣴⣮⣤⣤⣤⣤⣤⣤⣤⣤⣤⣄⡀⣠⢀⣄⣤⣤⣤⣠⣤⣤⣀
                      ⠈⠉⠛⠛⠿⠿⠛⠿⠿⠿⠿⠿⢿⠿⣿⠟⠿⠸⠟⢹⡇⢿⠘⠿⠻⠿⠟⠻⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⣿⢿⡿⢿⠿⠿⠿⠿⠿⠿⠿⠿⠿⢻⡟⠛⠃⠛ ⠚⠘⠏⠛⠙⠛⠻⠿⠿⠿⠛⠹⠟⠙⠃⣿⠰⡾⠛⠋⠿⠣⣿⠻⡿⠻⠇⠰
                        ⢴⣤⣶⣶⣶⣶⣦⣶⣤⣤⣴⣤⢤⡄            ⣤⢠⣶⣶⣶⣶⣶⣶⣶⣶⣶⣶⣦⣶⣤⣤⣴⣶⣶⣶⣶⣶⣶⣦⣤⣠⣄         ⣤⠠⡆⠠⣤⣤⢠⣶⣶⡶⣤⡤⢤       ⣠⡄⣀
                        ⠘⠋⠛⠛⠉⠋⠉⠿⠉⠿ ⠁              ⠉⠈⠁⠉⠉⠉⠉⠉⠉⠉⠋⠹⠿⠻⠏⠻⠿⠿⠻⠿⠿⠿⠟⠛⠛⠃           ⠈⠁⠘⠁⠉⠉⠛⠙⠃⠛ ⠋     ⠰⠆⠈ ⠴
                        ⢀⡶⣶⡐⢶ ⠆                           ⠠⠤⣦ ⣤⢲⣶⣶⣶⣶⣤⣤⢴⣦⢶⣶⣤⣶⣶                ⡴⢐⡆⢶⣰⣶
                        ⠈⠁⠋ ⠂  ⠛                            ⠋⠘⠛⠘⠋⠛⠛⠋⠙⠛⠛⠋⠚⠋⠛⠘⠃                 ⠈⠁⠈⠁          ⠚
                                                          ⢰⣷⣦⣴⣿⣾⣷⣶⣦        ⢰⡆⢲⡆
                               ⠛                          ⠉⠁⠉⠉⠋⠙⠋⠁         ⠈⠁⠈⠁                             ⠛
                               ⣀ ⣀                   ⢀ ⣸⠹⣿⣿⡟⣿⢿⣿⢿⡽⠿⠞⢿⠃        ⣹⡟⣿⠇⣀                        ⢀⡀⣀
                               ⣉ ⠉                   ⢀ ⡀⢀⣀⢈⡁⣉ ⠉⠈⠁  ⢀ ⡀       ⠉ ⠈ ⡉ ⡀                      ⠈⠁⢉
                             ⢘⡃⣿ ⣛⢀⡀                  ⠈⣧⢸⡿⢿⣷⣹⡇     ⠘⠈⠃        ⠠⠶⢶⡿⢹⡟⣀                   ⢀⡀⢘⡇⣿ ⣟⢀⡀⢀⡀
                             ⣀⣀⣀⣀⣁⢈⠁               ⢠⣄⣀⣀⣉⣈⣀⣈⣁⠉        ⡀ ⡀          ⢀⡀⣈⣀⣄⢀⡀⢀⡀             ⢀⣁⣀⣁⣈⣀⣁ ⡁⠈
                           ⢠⡤⢼⣿⣿⣯⡿⠸⡇⣤⠄⢤      ⣤⢠⡄⢿⠶⠿⢾⡟⣿⡏⣿⢫⣿⠹⡏⣤         ⠐⠃⠠      ⣤⠐⠶⠸⡇⢿⠻⠿⠸⡇⠙⠃⣤⠠⡤       ⢤⡄⣤⠈⠛⢽⠏⢿⠛⠿⠠⡇⢤⠄
                             ⣠⣄⣤⣤⣤⣤⣤⣤⣄   ⣠⡄⣤⣀⣤⢀⣄⣠⣤⣄⢀⣄⡀     ⡀          ⢀ ⣠⡄⢀         ⣤⣀⣤        ⡀⢀ ⡀ ⡀⣠⣤⣤⣤⣤⣤⣤⣀⣤⣤
                      ⠐ ⠒ ⣶⠰⠦⠉⠹⠟⢹⡿⠿⠋⠾⠛⠿⠛⠿⠉⠉⠉⠉⠉⠙⠛⢿⠟⢿⠸⡟⠻⠟⠛⠿⠟⠘⠇          ⠉⠁⠈ ⠈ ⠂  ⠶ ⠆⠘⠏⠉⠉⠉⠰⠆⠰⠆⠴⠈⠇⠘⠇⠿ ⡿⠘⠇⠉⠁⠿⠹⡿⠿⠋⠿⠉⠉⠰⠆⠰⠂
                     ⣶⣴⣶⢶⡄⣀  ⢀⡀   ⢀⡀       ⢤⢠⣦⣤⡴⠲⡦⢤    ⣀                ⢀ ⣤⢀⡄⣠⡄⣠ ⡄              ⠴⠄⣶⣦⡴⢦  ⢠⣦⣶⣦⣴⣄⣀  ⣀⡀
                     ⠉⠛⠛⠿⠆⠿⠰⠇⠰⠟⠛⠛⠛⠰⠆⠶⠆       ⠛⠈⠁⠘⠃⠈  ⠰⠆⠴ ⠦⠰⠆            ⠈ ⠉⠈⠁  ⠉⠰⠷⠰⠇⠶ ⠶           ⠁    ⠾ ⠁⠸⠏  ⠶⠰⠆⠰ ⠶
                    ⠰⢶⣶⣿⣷⣶⣶⢶⣶⣶⣄⣀⡠⠦⠰⠦⠶⠄⠄       ⣀⡀          ⠠⡄⣤              ⠰⠦⠶⢤⣶⣤⡤⠠⡄⢤       ⠠⠶⣺⡷⣶⣶⣶⢶⡶⠶⠄        ⠠⣤⡤
                      ⠉⠉⠙⠛⠛⠚⠛⠉⠈⠛                   ⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛                   ⠙⠛⠘⠃⠛ ⠛⠘⠃     ⠈ ⠘⠉⠁ ⠁        ⠐⠛  ⠛⠂
                      ⠤⠤⢤⠄                             ⢀⠰⣶⠴⠦⣶⡆⣶⢠⡄  ⢀   ⡀  ⣀ ⡀⣶⡆⣶⣦⣤⢰⡦⢶⠆⣠         ⣒⢂⣶⢐⡒⠂
                                                     ⠘⠁⠉    ⠉ ⠉⠈⠁⠈⠁⠉⠈⠉⠈⠃  ⠙⠈⠃⠈⠃⠉ ⠉⠈⠁  ⠉         ⠉⠈⠛⠈⠃
                           ⢀⡀⢀⡀⣀                     ⢀⡀⣿⢈⣿⠸⣇⣴⣴⣦⣴⣦⣹⡇⣶    ⣶⠆⣿⣶⣦⣶⡄⣿⢈⣿⣿⡿⢿⡿⠿                        ⢀⡀
                           ⠈⠁⠈⠁⠉                ⣀ ⢀⢀⡀⢈⡁⣈⣈⣁⣈⣁⣀    ⠉ ⢉   ⡀⣉ ⢉  ⢀⣀⠉⢈⣁⣈⡀⣉⠁⣀                        ⠈⠁
                           ⢀⡀⢀⡀                 ⠉⠃⠉⠘⠋⢻⣿⣿⣿⣿⣻⡟⣿⠰⠶⠶⠶⣶⡶⠿⢻⣄⢀⡟⠿⠿⠿⠾⠷⣿⡟⣿⢻⡿⣿⣿⣿⣟⣿⢀⡄
                             ⠈⠁                ⡀⣠⡄⣠⢀⡀⣀⡅⣈⣩⣭⢀⡀⢀    ⠠⡀ ⠈⠉ ⡅⢠⣤⣤⣠⡄⣈⡀⣀ ⣄⣈⡀⣈⡉⠁ ⡁
                                               ⠁⠿⠁⠼⢸⡿⠻⡿⢿⢿⠟⠛⠛⣿⠤⣤   ⠁⠚⠛⠓ ⠁⠛⠹⣿⢩⡗⢤⡜⠋⠻⠟⢽⠇⢼⠟⠻⠺⡇
                                              ⣤⣄⢠⡄⣤⢀⡄      ⠄   ⢠⡄   ⢀⡀⣤⣴⣦⣤⣤⣴⣆⢰⡄ ⢀⡀
                                              ⠛⠛⠚⠳⣿⠸⡇⠺⠏⠈⠁ ⠰⠆     ⠻⠏⢿⠈⠁⠈⠋⠛⠙⢿⠻⡿⠒⠁⠶⠈⠻⠠⠆  ⠶  ⠰⠆
                                              ⢰⡆⣴⡆⠤                   ⢰⣦⣴⡶⣶⢶⡦⣄ ⣀
                                              ⠈⠁   ⠸⠇⠺⠇⠾           ⠿    ⠉ ⠛⠘⠋⠈⠁⠿ ⠶  ⠶ ⠶
                                                                        ⣀⡀

                                                                     ⡀⢀⡀
                                                                    ⠈⠉⠈⠁
                                                               ⢀⡀⢀⡀  ⡶⢰⡆
                                                               ⠈⠁⠈⠁ ⠈⠉⠈⠁


                                                   ⠠⡤   ⠠⡤⠠⡄⢤

                                                   ⠰⠆⠰⠆⠶⠐⠶⠰⠆⠶
                                                                     ⣄
                                                       ⠺⠐⠇⠐⠆⠲ ⠶⠐⠇⠰⠂⠲ ⠇⠐⠇⠲ ⠲⠐⠆⠸⠂⠾ ⠖

                                                        ⠘⠛⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠓⠘⠃⠛ ⠛⠘⠃⠘⠃⠛⠐⠛⠐⠃
```
========================================================================================
## v3.7.2 | 2026-07-10
# HaleHound-CYD v3.7.2

A stability release — two BLE white-screen crashes fixed, plus the SubGHz capture and CC1101 radio fixes from issue #124.

## Fixes

**BLE white-screen on a failed connect — fixed.** The big one. When a BLE connect attempt failed — the target walked out of range, or a crowded RF room — the connection teardown raced its own cleanup and dropped the firmware to a white screen. SkeletonKey, Predator, AirTag Scream and the other connect-based BLE tools now recover cleanly and keep scanning instead of crashing.

**WiFi → BLE white-screen — fixed.** Jumping from a WiFi module (like Packet Monitor) straight into a BLE module could white-screen on occasion. Chased down a few sneaky missed memory reclaims in the hand-off and cleaned them up — the transition holds now, whatever you had running before it.

**SubGHz capture on a plain CC1101 — fixed (#124).** A signal-strength pre-check on the decode path was throwing away good captures on boards without an LNA — a bare CC1101 reads a low RSSI even on a strong burst. That pre-check is gone from the decode path; a capture that repeat-validates (same value and protocol twice) is kept. Repeat-validation still rejects noise, so accuracy is not loosened.

**CC1101 honors your saved radio setting (#124).** On E32R boards the firmware was force-selecting the E07 PA module on every radio operation, overriding your saved choice. It now honors the PA-module setting you pick, with a board-aware default, and the setting survives a power cycle.

## Boards
E32R35T (3.5"), CYD (2.8"), CYD-HAT, E32R28T.
========================================================================================
## v3.8.0 | 2026-07-16
# HaleHound-CYD v3.8.0

A big feature release — full Chameleon Ultra control, GPS-stamped NFC tags, and a BLE recon→attack chain.

## New

**Chameleon Ultra control (over Bluetooth).** Your HaleHound is now a portable controller for an RRG Chameleon Ultra — scan and connect, read HF and LF cards, clone a card into a slot, recover MIFARE keys (mfkey32 nonce capture, saved to SD), set up emulation, manage all 8 slots, and read/decode NDEF straight off a tag.

**Huge thanks to BkBroiler for the idea** — he built his own Chameleon Ultra module first, and it was such a great idea I followed suit. Full credit to him for the concept.

**RFID reader, your choice.** RFID opens a backend picker — PN-532, Chameleon Ultra, or ST25R3916 (coming soon). PN-532 and Chameleon now share the same NFC toolset: read, decode, and write NDEF either way.

**Write NFC tags — 8 record types, GPS-aware.** URL, Text, vCard, WiFi (WPA2), Bluetooth, Android app, geo location, and SMS. Pick geo and it can grab your live GPS fix and stamp it right onto the tag.

**Recon straight into attack — no second scan.** Predator hands a device it already found to SkeletonKey; the target carries over. A shared classifier labels AirTags, Tiles, SmartTags, Chipolo, Apple/Google Find My, Flock Safety cameras, and drone Remote-ID in one pass.

## Improvements

**Radio Test — new PN-532 self-test, no more false failures.** One-tap PN-532 check, and every probe leaves the SPI bus clean so a failed test can't make the next radio look dead.

**Readable GATT names + live progress bars.** Human names beside raw IDs in GATT/SkeletonKey; tag reads, Device Info, and GPS lock now show progress bars.

## Fixes

**SkeletonKey no longer white-screens on Back.** **GPS stops fighting the radio** on E32R boards.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.8.1 | 2026-07-27
# HaleHound-CYD v3.8.1

A stability release — Bluetooth is far more reliable, and the white-screens are gone.

## Fixes

**Bluetooth stability overhaul.** BLE recon, attacks, and the Jammer are far more reliable — the white-screens are gone, and everything holds up no matter what order you run the modules or how long you're at it.

**FlockYou saves to SD reliably.**

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.8.2 | 2026-07-29
# HaleHound-CYD v3.8.2

A polish release — SkeletonKey rescans in place, and the last of the BLE white-screens are gone.

## Fixes

**SkeletonKey rescans without backing out.** Connect to a device, back out, and scan again right from the tool — the rescan button re-arms in place, so you never have to leave and re-enter to hunt for new targets.

**The last BLE white-screens are gone.** Moving between Bluetooth modules in any order — including Flock detection straight into SkeletonKey — no longer trips a white-screen.

**Flock detections land in Loot with GPS.** Saved Flock hits now show up in the Loot viewer with their coordinates.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.