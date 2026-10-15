========================================================================================
## v3.9.1 | 2026-07-31
# HaleHound-CYD v3.9.1

A big one — a full NRF24 tuning cockpit, real names across BLE recon, and SkeletonKey learns to mix colors.

Huge thanks to **BKBroiler** — the NRF Tuning panel exists because of their idea to make the NRF24 radios fully tunable.

## What's New

**NRF Tuning panel.** A dedicated control screen for the 2.4 GHz jammers — pick a jammer, pick a sub-mode, and dial it in with gradient drag-sliders and ON/OFF toggles: SPI clock, power (real dBm), modulation (CW/GFSK), data rate, dwell, GFSK packet length, and per-band channel selection. Each jammer shows only the knobs it can actually use — no dead controls — every setting is applied live to the radio, and it's all explained in an on-SD guide you can read on the device or edit on a PC. Your settings persist on the SD card.

Not just sliders — proof. The 2.4 GHz jammers on a spectrum analyzer, tuned from a light touch to a solid wall:



**Stronger 2.4 GHz jamming.** The NRF24 jammers now put out denser, gap-free coverage on the Ebyte E01 radio.

**KeeLoq rolling-code decode.** Capture a KeeLoq remote in the SubGHz tools and HaleHound now decodes the frame — serial, button, and counter — against a built-in manufacturer-key table. First cut of our rolling-code work; more coming.

**BLE recon speaks English.** Services, characteristics, and device manufacturers now resolve to real names from an on-board database instead of raw hex — across both Predator and SkeletonKey. Predator also connects to its targets far more reliably.

**SkeletonKey — deeper dumps + a color mixer.** The GATT tree now shows real characteristic names and full UUIDs, flags every property (including Indicate), and a detail line shows the highlighted characteristic's full UUID and value. When it finds an RGB light (ELK-BLEDOM / Triones), a new COLOR MIX screen lets you blend any color live with R/G/B sliders — not just the four presets.

## Fixes

**Scanner & Analyzer shrug off the jammers.** Running ProtoKill (or any 2.4 GHz jammer) used to leave the Scanner and Spectrum Analyzer silent until a reboot — they now fully reset the radio on entry, so they come back to life every time.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.9.2 | 2026-08-03
# HaleHound-CYD v3.9.2

A tuning-and-reliability release — the SubGHz radio gets the same live-control cockpit the NRF24 got, every NRF tuning knob is now wired straight to the radio, and SD saving plus BLE connects are solid.

## What's New

**SubGHz tuning cockpit.** The CC1101 SubGHz radio now has a full live-tuning panel to match the NRF24 one. Gradient drag-sliders across the SubGHz tools — modulation, data rate, bandwidth, AGC sensitivity, and a +/- frequency fine-tune — applied live to the radio in both raw and packet mode, for the Receiver, Replay, and Jammer. Dial a signal in instead of fighting the defaults.

**NRF Tuning panel — every knob live.** Each 2.4 GHz jammer now shows only the controls it can actually use, and every slider is wired straight to the radio in real time. No dead knobs, no guessing which setting a given jammer honors.

**Chameleon Ultra — auto-connect.** The Chameleon Ultra module now connects to the first Chameleon it finds on its own, instead of making you pick it out of a list.

## Fixes

**Saving to SD is solid again.** Settings and loot write to the card reliably, even after you've been running the radios hard.

**SkeletonKey connects every time.** No more "found it but won't connect."

**Deauth fires the second you hit it.** No more instant-stop.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.9.3 | 2026-08-05
# HaleHound-CYD v3.9.3

Run as many Bluetooth tools as you want — back to back, any order — and it just keeps going. No more choking on its own memory halfway through a session.

## What's New

**Bluetooth tools chain forever now.** Hop between SkeletonKey, Predator, WhisperPair, Airoha, the AirTag tools, and Chameleon — as many as you want, any order — and they all keep opening. Under the hood they share one radio stack now instead of tearing it down and rebuilding it every single time, so the memory stops getting chewed to pieces. That whole "works a few times then won't connect" thing is gone.

**~15 KB more free RAM.** Memory that used to just sit there reserved gets handed back now until something actually needs it — more room to breathe for everything.

**FlockYou's leaner too.** It rides the same shared stack and only fires up GPS when there's room for it, so it opens clean even deep into a heavy session.

## Fixes

**No more "LOW MEMORY" after a run of BLE tools.** Fixed at the root, not taped over.

**And if memory ever does get tight, it backs out clean** instead of hanging on you.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.9.4 | 2026-08-13
# HaleHound-CYD v3.9.4

Last release got the Bluetooth tools to quit choking on memory. This one does the same thing for the WiFi side.

## What's New

**WiFi tools give the radio back now.** Run Evil Portal, the packet monitor, Evil Twin, and the scanners back to back and they clean up after themselves instead of leaving the memory chewed up for whatever you open next. Same fix as the Bluetooth side last time, now on WiFi.

**Loot viewer shows more.** Honeypot, FlockYou, and Predator dumps drop into Loot and show up right in the viewer, so you can page through them on the device.

**3.5" screens sit right.** SkeletonKey and Phantom Flood were overlapping themselves on the 3.5" panel. Laid out properly now.

## Fixes

**AirTag Scream holds up in a crowd now.** No more white screen when the room is packed with signals.

**Evil Twin's buttons line up** with where you actually tap.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v3.9.5 | 2026-08-30
# HaleHound-CYD v3.9.5

KARR module landed, and Replay capture got rebuilt.

## What's New

**KARR — detection and vulnerability assessment.** SouthWest Dealer Services BLE car alarms, the "QT" and "DR" units riding in roughly 2.85 million 2017-and-newer Kia, Honda, Toyota, Mazda and Ford vehicles.

Scan until a unit shows up, tap it, and the module figures out on its own what generation it is and whether it's exploitable. You get a straight verdict — VULNERABLE, PATCHED, or needs a key — and it sends no commands to reach it.

**Replay capture, rebuilt.** The module used to open on a blind all-band sweep. Now it opens on a target picker — seven curated hunts (Rolling Code, RF600/FM, Gate/Garage, Car/Alarm, 315, Fixed, Wide Recon), each carrying its own tuning and its own playbook you can read right on the device. Pick a hunt and it stays in that band instead of spraying across all fourteen frequencies.

Capture got fixed where it counted. FSK and FM fobs were being thrown out by checks written for OOK, so real presses vanished. A short press now catches on the first frame. KeeLoq reads on one press — a field tool does not get a second press on a target.

Saved signals moved off EEPROM onto the card: four slots became sixty-four, in a scrolling paged menu. A color-coded banner tells you what the radio is doing at arm's length — LISTENING, SCANNING, CAPTURED, TRANSMITTING. Once you have something, the bar turns into three labeled buttons: REPLAY, SAVE, CLEAR. The back arrow returns you to the hunt picker instead of dumping you out of the module, and the skull is a real exit. Raw OOK pulse dumps write to the card for reversing a fob at your desk.

**A proper KeeLoq reader for RF600E and FM fobs.** Reads serial and button on a single press.

**Vendor names in the station scanner.** Put the IEEE registry on the SD card and the WiFi station scanner names the hardware it finds instead of showing you bare MAC addresses. Forty thousand entries, searched straight off the card, so it costs nothing in memory. No card, no file — it falls back to the built-in list exactly as before.

Bluetooth tools used to get unreliable once you had opened a few of them in a row. Open them back to back now and they hold.

**Full GATT dumps to the card.** SkeletonKey and Predator write out every service, characteristic, descriptor and value they find. Pull the SD and read the whole tree.

**Chameleon tells you when a read came up short.** Partial NTAG reads get flagged, and crack progress reports honestly as N of 16 instead of implying it finished.

## Fixes

**Replay opens straight away.** Coming back into Replay after another radio module used to click, sit for a few seconds, then open. That stall is gone.

**Dead SubGHz bands are gone.** The 868, 915 and 925 entries are off the list — the radio in these units cannot hear up there, and leaving them in only produced silence that looked like a bug.

**The card mounts after radio work.** Coming out of a radio module and into Loot no longer reads an empty card.

**Long wardriving sessions stay steady.** Every logged row used to chew through memory a piece at a time — a ten-thousand-row drive did roughly a hundred and fifty thousand tiny allocations. Now it's one buffer and no churn.

**Jammers take up less room.** The skull artwork on the jammer screens were eating a large block of memory every time you opened one. Same look, a quarter of the footprint.

**Predator's honeypot says where it is.** On-screen status through each stage instead of a screen that just sits there.

## Boards

E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.
========================================================================================
## v4.0 | 2026-10-15
# HaleHound-CYD v4.0

RFID and NFC landed. That's why this one's a 4.0 - the thing reads and writes cards now, and that's a whole new side of the tool.

**(FUGT)**

## What's New

### RFID / NFC

**The ST25R3916 is in.** New NFC driver, and the module rebuilt into seven screens. It reads MIFARE Classic, NTAG and Ultralight, ISO-DEP and iCLASS. Everything below runs on that.

**iCLASS - read, decode, clone.** Full card read across AA1 and AA2 with 3DES done on the board. It pulls the PACS bits and decodes 26, 35 and 48-bit formats, Corporate 1000 included. Writes both keys. A clone copies the whole card and gets verified by reading it back afterward, not by assuming. The config block is decoded into plain words instead of eight hex bytes, and the write test covers AA2 as well.

**Key Check.** When a card won't talk, sweep all 29 published keys and the hit is usable everywhere else in the module. There's a default-safe way into it for cards that can't be locked.

**MIFARE Classic - dumps, writes and magic cards.** Full card dump with nested authentication, 31 of 32 sectors chaining in a single session. Sector trailer access bits decoded on screen. Keyed write on an ordinary card. Gen1A magic backdoor read and write, keyless, proven against an independent UID. Per-sector key setting, with a trailer guard so a write can't quietly zero a card's keys. Static-nonce detection and a nonce probe, so you know what you're holding before you start.

**Dumps go somewhere useful.** Saved to the card as a real Flipper .nfc and a Proxmark .eml, and they load back in. The saved-dump browser opens them with no card on the antenna.

**NTAG and NDEF.** Read what's on a tag and write your own - URL, text, contact, WiFi, Bluetooth, app, geo and SMS records. Records stack up instead of overwriting each other, and a viewer walks every record on the card with a hex toggle. Reads past 62 bytes now, looks in the right place for the message, and tells you what's actually on there.

**ISO-DEP.** Stopped negotiating 424 kbps, and the whole Type 4 chain started working.

**An on-device guide.** Card in one hand, phone nowhere. Tap GUIDE.

### Readers and access control

**HID reader tooling.** A BLE module for locating and stress-testing Signo and iCLASS SE readers, detected by advertised service UUID with a passive scan and a connect diagnostic. It captures the reader's reply over notifications, logs captures to the card, and reports an honest end-of-transaction status. The Artemis core sweep is split across connects, two commands at a time, so it completes instead of timing out. Card-emulation leak mode has the reader open the conversation while HaleHound answers as the card.

### Network and camera recon

**IoT Recon grew up.** DVR and GoAhead credential exploits, a repaired RTSP digest exchange that had been making every attempt fail, a fixed HTTP path that was testing an endpoint needing no auth, telnet brute, ONVIF, TBK and Uniview enumeration. On screen you get a device list, a scrollable feed, the access URLs, a running exploit tally and proper operator controls.

### Drones

**The drone suite grew up.** Detector, Deauth with per-target selection, Jam, and RID Spoof that bases its fake swarm on your real GPS and leashes it to a 250 metre radius. Scrollable detail, a swarm launch, and a cinematic kill sequence clipped to the content band. Modern model names in the SSID table, with the prefix decode kept out of IRAM.

### KARR

**Two more framings.** 0xBB Counter and 0x55 encrypted. It names an unsupported framing instead of calling a live unit silent, says why a connect failed, and saves that before a power-off. The raw encounter goes to the card, not just the verdict. Max BLE TX power for weak-signal targets, and the title bar strobes on a fresh detection.

### Radios and sensors

**Nice Flor-S keyless rolling-code decode.** And KeeLoq now requires the decrypted function code to match the plaintext button before it believes a read.

**A 24GHz Rd-01 radar module**, plus wider KARR recognition and identity reads.

**FlockYou picks up Axon and Utility body cameras**, and a contaminated OUI table got cleaned out.

### Boards

**AWOK Dual Touch v3.** CC1101 and nRF24 wired onto the panel-shared VSPI bus, the SD card put into service, GPS covering the whole CASIC baud set, and the right touch bus named per board.

**AWOK MINI.** A native 128x128 joystick interface across the whole board - animated boot splash, a living home screen, an honest WiFi analyzer, a pulsing threat state on the Guardian, and it never idles to sleep.

**The 4.0-inch E32R40T** is its own board target now.

### Under the hood

**One NDEF implementation.** The decoder, the eight record builders and the URI prefix table existed four times across the firmware. They're one file now, host-tested by a 50-case harness that builds with plain g++ and no ESP32 toolchain. Every label and value on screen is unchanged on purpose - what the modules gain is the best version of each, including more URI prefixes and long-record support.

**Faster drawing.** Gradient bars batch into fillRects and animation gates went to 30fps.

**The SD runs at 20MHz** instead of the 4MHz library default.

**RAM baselines measured and bumped across all seven board targets**, and a pre-ship smoke test checklist that every release runs on hardware before it goes out.

## Fixes

Battery gauge was reading about 4% low and a full pack never showed 100%. It maps a loaded cell properly now and updates in every module.

Replay hands the radio back when you leave the profile menu, so it doesn't lock up on the way out.

Both AWOK radios are addressable again - two stale guards were stopping either one being reached - and a CC1101 crash and an Init() SPI deadlock are gone.

GPS shows satellites while it's still searching instead of sitting on zero until it locks.

IoT Recon revives the SD bus before the mount instead of after a failed one.

## Boards

E32R35T (3.5"), E32R40T (4.0"), CYD (2.8"), E32R28T (2.8"), NM-RF Hat, AWOK Dual Touch v3, and AWOK MINI.