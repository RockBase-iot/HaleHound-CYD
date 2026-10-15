==========================================================================================
## v4.0 | HaleHound-CYD v4.0 | 2026-10-15T05:58:03Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.9.5 | HaleHound-CYD v3.9.5 | 2026-08-30T01:47:21Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.9.4 | HaleHound-CYD v3.9.4 | 2026-08-13T16:30:29Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.9.3 | HaleHound-CYD v3.9.3 | 2026-08-05T05:45:21Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.9.2 | HaleHound-CYD v3.9.2 | 2026-08-03T05:59:58Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.9.1 | HaleHound-CYD v3.9.1 | 2026-07-31T16:03:31Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

# HaleHound-CYD v3.9.1

A big one — a full NRF24 tuning cockpit, real names across BLE recon, and SkeletonKey learns to mix colors.

Huge thanks to **BKBroiler** — the NRF Tuning panel exists because of their idea to make the NRF24 radios fully tunable.

## What's New

**NRF Tuning panel.** A dedicated control screen for the 2.4 GHz jammers — pick a jammer, pick a sub-mode, and dial it in with gradient drag-sliders and ON/OFF toggles: SPI clock, power (real dBm), modulation (CW/GFSK), data rate, dwell, GFSK packet length, and per-band channel selection. Each jammer shows only the knobs it can actually use — no dead controls — every setting is applied live to the radio, and it's all explained in an on-SD guide you can read on the device or edit on a PC. Your settings persist on the SD card.

Not just sliders — proof. The 2.4 GHz jammers on a spectrum analyzer, tuned from a light touch to a solid wall:

<div align="center">
<table>
<tr>
<td><img src="https://github.com/JesseCHale/HaleHound-CYD/releases/download/v3.9.1/nrf-jam-1.jpg" width="330"></td>
<td><img src="https://github.com/JesseCHale/HaleHound-CYD/releases/download/v3.9.1/nrf-jam-2.jpg" width="330"></td>
</tr>
<tr>
<td><img src="https://github.com/JesseCHale/HaleHound-CYD/releases/download/v3.9.1/nrf-jam-3.jpg" width="330"></td>
<td><img src="https://github.com/JesseCHale/HaleHound-CYD/releases/download/v3.9.1/nrf-jam-4.jpg" width="330"></td>
</tr>
</table>
</div>

**Stronger 2.4 GHz jamming.** The NRF24 jammers now put out denser, gap-free coverage on the Ebyte E01 radio.

**KeeLoq rolling-code decode.** Capture a KeeLoq remote in the SubGHz tools and HaleHound now decodes the frame — serial, button, and counter — against a built-in manufacturer-key table. First cut of our rolling-code work; more coming.

**BLE recon speaks English.** Services, characteristics, and device manufacturers now resolve to real names from an on-board database instead of raw hex — across both Predator and SkeletonKey. Predator also connects to its targets far more reliably.

**SkeletonKey — deeper dumps + a color mixer.** The GATT tree now shows real characteristic names and full UUIDs, flags every property (including Indicate), and a detail line shows the highlighted characteristic's full UUID and value. When it finds an RGB light (ELK-BLEDOM / Triones), a new COLOR MIX screen lets you blend any color live with R/G/B sliders — not just the four presets.

## Fixes

**Scanner & Analyzer shrug off the jammers.** Running ProtoKill (or any 2.4 GHz jammer) used to leave the Scanner and Spectrum Analyzer silent until a reboot — they now fully reset the radio on entry, so they come back to life every time.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.

<div align="center">
<pre>
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
</pre>
</div>


ASSETS: nrf-jam-1.jpg(151407), nrf-jam-2.jpg(146048), nrf-jam-3.jpg(144871), nrf-jam-4.jpg(150156)
==========================================================================================
## v3.8.2 | HaleHound-CYD v3.8.2 | 2026-07-29T04:45:31Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

# HaleHound-CYD v3.8.2

A polish release — SkeletonKey rescans in place, and the last of the BLE white-screens are gone.

## Fixes

**SkeletonKey rescans without backing out.** Connect to a device, back out, and scan again right from the tool — the rescan button re-arms in place, so you never have to leave and re-enter to hunt for new targets.

**The last BLE white-screens are gone.** Moving between Bluetooth modules in any order — including Flock detection straight into SkeletonKey — no longer trips a white-screen.

**Flock detections land in Loot with GPS.** Saved Flock hits now show up in the Loot viewer with their coordinates.

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.

<div align="center">
<pre>
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
</pre>
</div>


ASSETS: 
==========================================================================================
## v3.8.1 | HaleHound-CYD v3.8.1 | 2026-07-27T02:27:43Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

# HaleHound-CYD v3.8.1

A stability release — Bluetooth is far more reliable, and the white-screens are gone.

## Fixes

**Bluetooth stability overhaul.** BLE recon, attacks, and the Jammer are far more reliable — the white-screens are gone, and everything holds up no matter what order you run the modules or how long you're at it.

**FlockYou saves to SD reliably.**

## Boards
E32R35T (3.5"), CYD (2.8"), E32R28T (2.8"), and the NM-RF Hat.

<div align="center">
<pre>
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
</pre>
</div>


ASSETS: 
==========================================================================================
## v3.8.0 | HaleHound-CYD v3.8.0 | 2026-07-16T22:25:32Z
<div align="center">
<pre>
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
                                                                                  
  
</pre>
</div>

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

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.7.2 | HaleHound-CYD v3.7.2 | 2026-07-10T13:32:18Z
<div align="center">
<pre>
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
                                                                                  
             
</pre>
</div>

# HaleHound-CYD v3.7.2

A stability release — two BLE white-screen crashes fixed, plus the SubGHz capture and CC1101 radio fixes from issue #124.

## Fixes

**BLE white-screen on a failed connect — fixed.** The big one. When a BLE connect attempt failed — the target walked out of range, or a crowded RF room — the connection teardown raced its own cleanup and dropped the firmware to a white screen. SkeletonKey, Predator, AirTag Scream and the other connect-based BLE tools now recover cleanly and keep scanning instead of crashing.

**WiFi → BLE white-screen — fixed.** Jumping from a WiFi module (like Packet Monitor) straight into a BLE module could white-screen on occasion. Chased down a few sneaky missed memory reclaims in the hand-off and cleaned them up — the transition holds now, whatever you had running before it.

**SubGHz capture on a plain CC1101 — fixed (#124).** A signal-strength pre-check on the decode path was throwing away good captures on boards without an LNA — a bare CC1101 reads a low RSSI even on a strong burst. That pre-check is gone from the decode path; a capture that repeat-validates (same value and protocol twice) is kept. Repeat-validation still rejects noise, so accuracy is not loosened.

**CC1101 honors your saved radio setting (#124).** On E32R boards the firmware was force-selecting the E07 PA module on every radio operation, overriding your saved choice. It now honors the PA-module setting you pick, with a board-aware default, and the setting survives a power cycle.

## Boards
E32R35T (3.5"), CYD (2.8"), CYD-HAT, E32R28T.

<div align="center">
<pre>
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
</pre>
</div>

ASSETS: 
==========================================================================================
## v3.7.1 | HaleHound-CYD v3.7.1 | 2026-07-09T03:37:45Z
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

ASSETS: 
==========================================================================================
## v3.7.0 | HaleHound-CYD v3.7.0 | 2026-07-08T13:06:26Z
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

ASSETS: 
==========================================================================================
## v3.6.2 | HaleHound-CYD v3.6.2 | 2026-06-29T03:39:01Z
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

ASSETS: 
==========================================================================================
## v3.6.1 | HaleHound-CYD v3.6.1 | 2026-06-15T01:44:06Z
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


ASSETS: 
==========================================================================================
## v3.6.0 | HaleHound-CYD v3.6.0 | 2026-06-14T20:34:18Z
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

ASSETS: 
==========================================================================================
## v3.5.5 | HaleHound-CYD v3.5.5 | 2026-04-03T17:04:24Z
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

ASSETS: halehound-v355-release.png(10358714)
==========================================================================================
## v3.5.1 | HaleHound-CYD v3.5.1 — CYD35C Pin Fix | 2026-03-28T12:47:26Z
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
ASSETS: 
==========================================================================================
## v3.5.0 | HaleHound-CYD v3.5.0 | 2026-03-27T23:55:49Z
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

ASSETS: 
==========================================================================================
## v3.4.0 | HaleHound-CYD v3.4.0 | 2026-03-15T02:38:23Z
## HaleHound-CYD v3.4.0

### New Board: QDtech E32R35T (3.5")

The E32R35T replaces the old ESP32-3248S035C as the supported 3.5" board. XPT2046 resistive touch, exposed SPI breakout, battery charging via TP4854, and onboard amp — all the hardware a HaleHound needs.

### What's New

- **E32R35T support** — New build target `esp32-e32r35t` with full pin mapping: CC1101 CS on GPIO 21, NRF24 on GPIO 26/16, battery ADC on GPIO 34, amp enable on GPIO 4
- **XPT2046 resistive touch** — Replaces GT911 capacitive. Shares HSPI with the LCD via TFT_eSPI's built-in driver. No extra library needed.
- **Touch calibration** — Save/load from NVS. Auto-recalibrates on rotation change. Calibrate from Settings > Touch Calibration.
- **GPS screen scaled for 3.5"** — Compass, speed arc, satellite bars, crosshairs, and all layout elements properly fill the 320x480 display
- **PA module support** — TX_EN (GPIO 4) and RX_EN (GPIO 0) for CC1101 power amplifier on E32R35T
- **Battery monitoring** — ADC on GPIO 34 for TP4854 charge IC

### Bug Fixes

- **Fixed touch blocking loot file selection** — Edge-trigger flag was consuming every touch before menu selection could see it
- **Tuned touch sensitivity** — Threshold set to 100 for responsive light taps on XPT2046 panels

### Removed

- GT911 capacitive touch driver (TAMC_GT911 library)
- ESP32-3248S035C (old CYD 3.5") board support

### Supported Boards

| Board | Build Target | Flash File |
|-------|-------------|------------|
| CYD 2.8" (ESP32-2432S028) | `esp32-cyd` | HaleHound-CYD |
| QDtech E32R35T (3.5") | `esp32-e32r35t` | HaleHound-E32R35T |
| QDtech E32R28T (2.8") | `esp32-e32r28t` | HaleHound-E32R28T |
| NM-RF-Hat | `esp32-cyd-hat` | HaleHound-CYD-HAT |

### Flashing

Single file: Flash `HaleHound-<board>-FULL.bin` at address `0x0`

Four file: `bootloader.bin` (0x1000) + `partitions.bin` (0x8000) + `boot_app0.bin` (0xe000) + `HaleHound-<board>.bin` (0x10000)
ASSETS: 
==========================================================================================
## v3.3.2 | HaleHound-CYD v3.3.2 — Bug Fixes | 2026-03-14T21:50:43Z
## What's Fixed
- **NRF24 Jammer cleanup** — Scanner/Analyzer no longer dies after WLAN Jammer or ProtoKill (RF24 library CRC bug workaround)
- **Evil Twin hotspot** — Deauth failure recovery no longer kills the AP (#12)
- **Valhalla SD wipe** — Files in subdirectories now actually get deleted
- **RFID Brute Force** — Fixed crash + added scroll to Reader and Brute screens
- **WhisperPair** — Fixed attack chain (connection reuse, SD save, touch navigation)
- **BLE Ducky** — Fixed screen flicker (dirty-flag redraw)

## What's Changed
- **MouseJack** — UI redesign with target frame, action button, device type display
- **Find You** — Removed (replaced by AirTag Detect)
- **BLE-to-NRF24 handoff** — Improved exit checks, heap diagnostics, SPI cleanup

## Flash Package
Pre-built binaries for all supported boards:
- **CYD 2.8"** — HaleHound-CYD.bin / HaleHound-CYD-FULL.bin
- **CYD HAT** — HaleHound-CYD-HAT.bin / HaleHound-CYD-HAT-FULL.bin
- **E32R28T** — HaleHound-E32R28T.bin / HaleHound-E32R28T-FULL.bin

See FLASH_INSTRUCTIONS.txt in the repo for flashing guide.

ASSETS: 
==========================================================================================
## v3.3.0 | v3.3.0 — NRF Sniffer, MouseJack, BLE Ducky | 2026-03-12T03:11:25Z
## What's New

### NRF Promiscuous Sniffer
- Travis Goodspeed promiscuous receive mode on NRF24L01+PA+LNA
- Captures raw 2.4GHz packets from wireless keyboards, mice, drones, IoT sensors
- Channel hopping across all 126 channels (Core 0 task)
- Tap any captured device address to auto-populate MouseJack target
- Nosifer UI with animated skull row, scrollable packet list, hex dump detail view

### MouseJack Keystroke Injection
- Inject keystrokes into Logitech Unifying, Dell, Microsoft wireless keyboards
- Fixed Logitech HID++ packet format: 0xC1 frame type, LRC checksum, 16-bit CRC
- Full HID scancode keymap: a-z, 0-9, symbols, F1-F12, arrows, modifiers
- Pre-built payloads: reverse shell (PowerShell/bash), WiFi exfil, custom string
- Core 0 injection task with 10ms inter-keystroke timing

### BLE Ducky — BLE HID Keyboard Injection
- ESP32 acts as BLE HID keyboard via ESP32-BLE-Keyboard library
- Target pairs with "HaleHound KB", then receives injected keystrokes
- Pre-built payloads: reverse shell, Rick Roll, custom string entry
- 50ms BLE keystroke timing, progress display, pairing status

### AirTag Attack Suite (AirTag Hub)
- **Phantom Flood** — FindMy OF advertisement flood with random public keys
- **AirTag Replay** — Sniff & replay real AirTag BLE identity
- **Find You** — Stealth AirTag clone with P-224 EC key rotation (15-120s key rotation below Apple anti-stalking threshold)

### WhisperPair — CVE-2025-36911 Exploit Chain
- Full attack phase added: scan, probe, and exploit Google Fast Pair devices
- GATT-based Key-Based Pairing characteristic probe for unauthorized pairing
- Loot Viewer for browsing discovered vulnerable devices

### Unified Loot Manager
- Centralized loot storage and browsing across all attack modules
- WhisperPair Loot Viewer with full device detail display
- GPS coordinate tagging for wardriving loot

### E32R28T Board Support
- Full support for QDtech E32R28T 2.8" board
- VALHALLA protocol for board identification
- Custom skull art and per-board skull watermarks (240x320 and 320x480 variants)
- GPIO 0 fix for E32R28T boot behavior
- Pre-compiled binaries now included in flash_package

### BLE Database Integration
- 94 company IDs, 73 service UUIDs, 52 GAP appearances — all PROGMEM binary search
- Source: Nordic Semiconductor bluetooth-numbers-database v1.0.4
- BLE Sniffer now shows manufacturer names and device types

### Dual-Core 2.4GHz Scanner + Analyzer
- NRF24 Scanner and Spectrum Analyzer now run scan on Core 0, draw on Core 1
- Significantly faster scan rates with smooth UI updates

### CC1101 E07-PA Module Support
- PA module control added to all SubGHz operations
- E07-PA wiring diagram added to Radio Test pages
- Proper power amplifier enable/disable for E07 modules

### EAPOL Capture — AP List Pagination
- Scrollable AP list for networks with many access points
- No more truncated scan results on busy networks

## Bug Fixes
- Fixed WiFi/BLE radio handoff — proper teardown for cross-module transitions
- Fixed residual touch exits + GPIO0 bug across all modules
- Fixed BLE Sniffer instant-exit on E32R28T + Classic BT memory release
- Fixed NRF24 SETUP_AW register persistence across ESP32 reset (promiscuous sniffer cleanup)
- Fixed boot NRF24 detection false positive after sniffer use
- Fixed 3.5" CYD bugs: attack popup touch zones, RFID brute crash, wardriving layout
- SubGHz analyzer speed improvements + replay noise filter
- DRAM optimization: reduced BLE sniffer device array, const-qualified icon arrays

## UI Polish
- NRF Sniffer and MouseJack screens: Nosifer headers, rounded frames, skull rows
- Per-board skull watermarks for CYD 2.8", CYD 3.5", and E32R28T
- E07-PA wiring diagram in Radio Test
- Improved boot mismatch warning — mentions E32R28T and HAT by name
- 

## Supported Boards
| Board | Build Target |
|-------|-------------|
| CYD 2.8" (ESP32-2432S028) | esp32-cyd |
| QDtech E32R28T | esp32-e32r28t |
| NM-RF-Hat | esp32-cyd-hat |

**Note:** CYD 3.5" (ESP32-3248S035C) is temporarily excluded from this release due to a DRAM overflow caused by the GT911 touch driver. Will be fixed in a follow-up release.

## Pre-Compiled Binaries

| File | Board | Flash Address |
|------|-------|---------------|
| HaleHound-CYD-FULL.bin | CYD 2.8" | 0x0 (single file) |
| HaleHound-CYD.bin | CYD 2.8" | 0x10000 (4-file method) |
| HaleHound-CYD-HAT-FULL.bin | NM-RF-Hat | 0x0 |
| HaleHound-CYD-HAT.bin | NM-RF-Hat | 0x10000 |
| HaleHound-E32R28T-FULL.bin | QDtech E32R28T | 0x0 |
| HaleHound-E32R28T.bin | QDtech E32R28T | 0x10000 |

Flash instructions: see FLASH_INSTRUCTIONS.txt in the release.

## Requirements
- NRF24L01+PA+LNA module required for NRF Sniffer, MouseJack, WLAN Jammer, Proto Kill
- CC1101 module required for SubGHz attacks (standard HW-863 or E07-PA supported)
- BLE Ducky uses ESP32's built-in Bluetooth — no external hardware needed
- GPS module on GPIO 3 (P1 connector) required for wardriving

ASSETS: 
==========================================================================================
## v3.2.0 | HaleHound-CYD v3.2.0 — VALHALLA Protocol, IoT Recon, RFID/NFC, Jam Detect | 2026-03-06T00:14:03Z
## HaleHound-CYD v3.2.0 — VALHALLA Protocol + 4 New Modules

The biggest single release in HaleHound history. Four entirely new attack modules, a full defensive panic system, device PIN lock, and over 9,000 new lines of code.

### VALHALLA Protocol — Scorched Earth Panic System

One-tap emergency response. Tap the VALHALLA banner on the home screen and hold to confirm:
- **Wipes SD card** — recursive delete, everything gone
- **Locks all offensive tools** — 17 offensive functions gated
- **Enters Blue Team mode** — device switches to defensive-only operation
- **Legal disclaimer** — EEPROM-persisted liability screen with scrollable text. Must accept before any offensive function will run. Cites 47 U.S.C. § 333 and 18 U.S.C. § 1030.
- Blue Team mode persists across reboots until you actively re-accept the disclaimer

### New Modules

- **IoT Recon** — Automated LAN attack suite. WiFi connect via on-screen keyboard, full network scan, service fingerprinting (HTTP, Telnet, SSH, MQTT, RTSP, FTP), credential brute forcing with 40+ default password combos, harvested credential storage. Core 0 scan task for responsive UI during deep scans. *(Shoutout: @CircuitZ for reminding me to add IoT)*

- **PN532 RFID/NFC** — Five attack modes: Card Scanner (UID + type detection), Card Reader (full sector dump with default keys), Card Clone (copy UID to magic Gen1a cards), Key Brute Force (dictionary + sequential attack on all sectors), Card Emulate (replay captured UID). Software SPI to avoid bus conflicts. *(Shoutout: @duggie162-cpu for the RFID recommendation)*

- **Jam Detect** — Defensive RF jamming detection across all 3 radios. *(Shoutout: @ValleytechSolutions for bringing up the idea)*
  - **WiFi Guardian** — 2.4 GHz deauth/disassoc frame counter with per-channel heatmap
  - **SubGHz Sentinel** — CC1101 wideband energy monitoring across 315/433/868/915 MHz
  - **2.4 GHz Watchdog** — NRF24 spectrum-analyzer-style continuous sweep with threshold alerts
  - **Full Spectrum** — All radios simultaneously, split-screen dashboard

- **Captive Portal Upgrade** — PSK capture templates + Core 0 deauth task for continuous client disruption while serving portal pages. *(Shoutout: @MuffDuncan for bringing it to my attention)*

### Renamed

- **Stalkerware Detect** has been renamed to **Lunatic Fringe**

### Security

- **PIN Lock** — 4-digit device lock with configurable auto-lock timeout (30s to 10min or never). Skull animation on lock screen. Persisted in EEPROM.

### Fixes & Improvements

- NRF24 SPI clock reduced to 4 MHz for reliability on noisy power sources
- IoT Recon timeout tuning + harvested credential persistence
- Jam Detect flicker elimination + WiFi Guardian full layout redesign
- PN532 software SPI fix for CYD GPIO conflicts
- IoT max devices reduced 64→48 to fix 3.5" CYD DRAM overflow

### Community

Credit to the people who made this release happen:

- **@MuffDuncan** — For bringing the Captive Portal update to my attention
- **@CircuitZ** — For reminding me to add IoT
- **@ValleytechSolutions** — For bringing up the Jam Detect idea
- **@duggie162-cpu** — For the RFID recommendation, Lunatic Fringe concept, and EAPOL bug reports
- **@duggie162-cpu, @ValleytechSolutions, @ TalkingSasquach, @Notorious-Squirrel, @Man-In-The-Mayhem, @hamspiced** — For the efforts and ideas that created the OPSEC of the HaleHound

---

### Downloads

**Easiest method: Flash the FULL.bin at offset `0x0`**

| File | Board | Size |
|------|-------|------|
| `HaleHound-CYD-FULL.bin` | **2.8" CYD** (ESP32-2432S028) | 2.3 MB |
| `HaleHound-CYD-35-FULL.bin` | **3.5" CYD** (ESP32-3248S035) | 2.4 MB |
| `HaleHound-CYD-HAT-FULL.bin` | **2.8" CYD + NM-RF-Hat** | 2.3 MB |

**Four-file method** (if single-file gives black screen):

| File | Flash Address |
|------|---------------|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xe000` |
| `HaleHound-CYD.bin` / `HaleHound-CYD-35.bin` / `HaleHound-CYD-HAT.bin` | `0x10000` |

Flash with [ESP Web Flasher](https://esp.huhn.me) (Chrome/Edge/Opera) or esptool.

See `flash_package/FLASH_INSTRUCTIONS.txt` for step-by-step guide.
ASSETS: 
==========================================================================================
## v3.1.0 | HaleHound-CYD v3.1.0 — Stalkerware Detect + Full 3.5" CYD Support | 2026-03-01T22:37:12Z
## HaleHound-CYD v3.1.0 — Stalkerware Detect + Full 3.5" CYD Support

### NEW: CYD 3.5" (ESP32-3248S035) is FULLY SUPPORTED

The 3.5" Cheap Yellow Display is now a first-class target. Dedicated build environment (`esp32-cyd-35`), ST7796 display driver, GT911 capacitive touch, and every single module works out of the box. Flash `HaleHound-CYD-35-FULL.bin` at offset `0x0` and you're running. No compromises, no missing features — the full HaleHound arsenal on the bigger screen.

### New Features

- **Stalkerware Detect** — Multi-platform BLE tracker scanner that detects Google FMDN (`0xFEAA`), Samsung SmartTag (`0xFD5A`), Tile (`0xFEED`), Chipolo (`0xFE33`), and Apple AirTag (`0x004C`) trackers via BLE advertisements. Scrollable list view with platform icon, RSSI proximity bars, and signal strength. Tap any tracker for detail view with MAC, platform, battery estimate, and status. Auto-rescans every 5 seconds with alert flash on new finds. Tracks up to 20 unique devices simultaneously. *(Concept: Duggie)*

- **CYD 3.5" Build Target** — Full support for the ESP32-3248S035 with 320x480 ST7796 display and GT911 capacitive touchscreen. Dedicated PlatformIO environment (`esp32-cyd-35`), separate User_Setup, and all UI layouts adapted for the larger resolution. Every attack module, every animation, every screen — all working on the 3.5".

- **NM-RF-Hat Build Target** — Dedicated build for CYD boards using the NM-RF-Hat companion hat (shared GPIO 22/27 for CC1101 and NRF24 via physical switch).

### Bug Fixes

- **EAPOL back button double-fire** — Fixed duplicate `touchButtonsUpdate()` causing back button to fire twice, exiting the module unexpectedly *(Reported: Duggie)*
- **Stalkerware Detect back button** — Removed duplicate touch handling from main loop for clean single-path navigation

### Community

- Added **Duggie** to community contributors — Stalkerware Detect concept + EAPOL back button bug report

---

### Downloads — 3 Boards, One Firmware Each

**Easiest method: Flash the FULL.bin at offset `0x0`**

| File | Board | Size |
|------|-------|------|
| `HaleHound-CYD-FULL.bin` | **2.8" CYD** (ESP32-2432S028) | 2.1 MB |
| `HaleHound-CYD-35-FULL.bin` | **3.5" CYD** (ESP32-3248S035) | 2.2 MB |
| `HaleHound-CYD-HAT-FULL.bin` | **2.8" CYD + NM-RF-Hat** | 2.1 MB |

**Four-file method** (if single-file gives black screen):

| File | Flash Address |
|------|---------------|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xe000` |
| `HaleHound-CYD.bin` / `HaleHound-CYD-35.bin` / `HaleHound-CYD-HAT.bin` | `0x10000` |

Flash with [ESP Web Flasher](https://esp.huhn.me) (Chrome/Edge/Opera) or esptool.

See `FLASH_INSTRUCTIONS.txt` for step-by-step guide.

ASSETS: 
==========================================================================================
## v3.0.0 | HaleHound-CYD v3.0.0 | 2026-02-28T23:22:29Z
## HaleHound-CYD v3.0.0

### Dual-Core Engine
- Beacon Spammer, Beacon Nuke, Deauther, Auth Flood now run radio TX on Core 0
- BLE Spoofer, BLE Beacon, BLE Jammer offloaded to Core 0
- PacketMonitor FFT sampling on Core 0 for responsive UI
- SubGHz Jammer, SubGHz Replay FFT, SubGHz Analyzer on Core 0
- WLAN Jammer and ProtoKill already on Core 0
- All 13 attack modules now use dual-core: Core 0 = radio, Core 1 = display/touch

### SubGHz Analyzer Overhaul
- Replaced waterfall display with 33 LED VU meter spectrum bars
- Covers all CC1101 bands: 300-348 MHz, 387-464 MHz, 779-928 MHz
- Exponential smoothing for buttery animation at ~50 FPS
- Heat palette colors by bar height (purple to blue to green to yellow to red to white)
- Peak-hold falling dots in hot pink

### Evil Twin Improvements
- Captured credentials now log to SD card for persistent storage
- Fixed clone bug where SSID was cleared before handoff to CaptivePortal

### Bug Fixes
- Fixed PacketMonitor crash on exit (double vTaskDelete)
- Improved task exit safety across all dual-core modules

### Flash Instructions
Two methods available -- see FLASH_INSTRUCTIONS.txt for details:
- **Method A** (easiest): Flash HaleHound-CYD-FULL.bin to address 0x0
- **Method B** (fallback): Flash 4 individual files to specific addresses
- **HAT variant**: Use HaleHound-CYD-HAT-FULL.bin or HaleHound-CYD-HAT.bin for boards with NM-RF-Hat

Use [ESP Web Flasher](https://esp.huhn.me) (Chrome/Edge/Opera) or esptool.
ASSETS: 
==========================================================================================
## v2.9.0 | v2.9.0 — Wardriving BLE Scan, Open Net Tracking, GPS Auto-Init | 2026-02-25T06:10:35Z
## What's New in v2.9.0

### Wardriving — BLE Device Logging
Wardriving now scans **both WiFi and BLE** in alternating cycles. WiFi networks and BLE devices are logged to the same WiGLE-compatible CSV file. The ESP32 radio switches between WiFi and BLE automatically — no user action needed.

### Wardriving — Open Network Count
The wardriving screen now shows how many **open (unencrypted) WiFi networks** have been found. Open networks light up in hot pink so they stand out.

### Wardriving — Faster Scan Interval
Scan interval dropped from 5 seconds to 2.5 seconds. More scans per session = more networks captured while moving.

### Wardriving — GPS Auto-Init
GPS now initializes automatically when you enter the wardriving screen. No need to visit the GPS screen first — just plug in and go.

### Wardriving — Redesigned Display
New stats layout shows: NETWORKS, OPEN, BLE, DUPES, SCANS, STATUS (WIFI/BLE phase), GPS fix, SATS, LAT, LON, SPEED (km/h from GPS), and session TIME.

### Wardriving — WiGLE v1.6 CSV Format
CSV output upgraded to WiGLE v1.6 with Frequency, RCOIs, MfgrId, and Type columns. WiFi rows include channel-to-frequency conversion. BLE rows include manufacturer company ID.

### WiFi State Desync Fix
Replaced raw `esp_wifi_stop()` calls with `WiFi.mode(WIFI_OFF)` across all modules. Prevents a bug where the Arduino WiFi wrapper loses track of radio state, silently breaking WiFi for subsequent modules.

### CSV Path Fix
Fixed a bug where wardriving log files with GPS timestamps landed in the SD card root instead of the `/wardriving/` directory.

---

## Flash Instructions

See `FLASH_INSTRUCTIONS.txt` in the release assets or the [README](https://github.com/JesseCHale/HaleHound-CYD#flashing) for full details.

**Method A — Single File (easiest):**
Flash `HaleHound-CYD-FULL.bin` to address `0x0`

**Method B — Four Files (if Method A gives black screen):**
| Address | File |
|---------|------|
| 0x1000 | bootloader.bin |
| 0x8000 | partitions.bin |
| 0xe000 | boot_app0.bin |
| 0x10000 | HaleHound-CYD.bin |

Browser flasher: https://esp.huhn.me (Chrome/Edge/Opera only)
ASSETS: 
==========================================================================================
## v2.8.0 | HaleHound-CYD v2.8.0 | 2026-02-22T05:29:41Z
## HaleHound-CYD v2.8.0

### Dual-Core Persuasion Engine
All 3 NRF24 2.4GHz modules (BLE Exorcist, ProtoKill, WLAN Attitude Adjuster) completely rewritten with dual-core architecture. Radio runs on Core 0, display runs on Core 1 — full duty cycle with zero display overhead. Massive range and performance improvement.

### ADV KILL Mode
New BLE Exorcist mode — casts out demons on the 3 advertising channels. Effective at 20+ feet in testing. Devices get counseled into taking a break.

### NM-RF-Hat Support (CYD-HAT Edition)
Full support for the NM-RF-Hat companion board. Separate CYD-HAT firmware with correct pin mapping for hat hardware. **Flash the right one for your board.**

### Wrong-Firmware Detection
Boot screen now validates NRF24 on expected pins. If you flash the wrong firmware (CYD on a hat board or vice versa), it tells you immediately instead of silently failing.

### GPS Tactical Instrument Panel
Compass rose with heading needle, speed arc gauge, gradient satellite signal bars, skull fix indicator, GPS accuracy in feet.

### Bug Fixes
- Packet monitor no longer dead after running beacon spammer
- CC1101 GDO0 pin direction fix for continuous carrier mode

### Downloads
- **`HaleHound-CYD-FULL.bin`** — Standard CYD (no hat). Flash at address `0x0`
- **`HaleHound-CYD-HAT-FULL.bin`** — CYD with NM-RF-Hat. Flash at address `0x0`
- Individual files (bootloader, partitions, firmware) included for advanced users
ASSETS: 
==========================================================================================
## v2.7.0 | HaleHound-CYD v2.7.0 | 2026-02-20T03:01:35Z
# HaleHound-CYD v2.7.0

**ESP32-DIV HaleHound Edition for Cheap Yellow Display**

---

## New Modules

### WhisperPair — CVE-2025-36911 Fast Pair Vulnerability Scanner
BLE attack module targeting Google Fast Pair devices. Scans for Fast Pair service advertisements, then connects via GATT to check if the Key-Based Pairing characteristic is accessible outside of pairing mode — a condition that allows unauthorized pairing. Full HaleHound visual treatment — Nosifer glitch title, electric blue primary, hot pink result banners.



### AirTag Detect — Apple FindMy Tracker Detection
Passive BLE scanner that detects AirTags, FindMy accessories, and compatible third-party trackers. Filters for Apple manufacturer data (0x4C) with FindMy type bytes (0x12/0x19). Shows MAC, RSSI proximity bars, battery level, distance estimate, and status byte. Auto-rescans every 5 seconds with alert flash on new tracker detection. Tracks up to 20 unique devices.
![IMG_4160](https://github.com/user-attachments/assets/9f19c026-6ca3-4382-8722-f18173063de6)

### Auth Flood — 802.11 Authentication Frame Flood
Overwhelms target AP client tables with fake authentication requests from random MAC addresses. Scan for targets, tap to select, then flood. Features an 85-bar equalizer animation with ProtoKill-style cyan-to-hotpink gradient sweep during attack. Full radio init validation with test frame before declaring ready.

---

## New Tool

### Radio Test — SPI Hardware Verification
Interactive diagnostic tool for NRF24L01+ and CC1101 radios from the Tools menu. Tests SPI communication, reads chip registers (NRF24 CONFIG 0x08, CC1101 VERSION 0x14), and provides smart failure diagnostics — wiring issues vs dead chip vs clone detection. Includes 4-page wiring block diagrams with KiCad-style layout showing colored trace lines, solder dots, and chip boxes for NRF24, GPS, and CC1101. Battery voltage readout.
![IMG_4186](https://github.com/user-attachments/assets/9132a073-b30d-451b-a137-94f3b9f0ce5f)
![IMG_4182 2](https://github.com/user-attachments/assets/fac268c8-b0ce-4c11-b457-eeee5a05975e)
![IMG_4187](https://github.com/user-attachments/assets/651b0a54-ed1e-4815-b1ed-65aadad8a679)
![IMG_4188](https://github.com/user-attachments/assets/7351c5f9-98e6-40a5-a3be-54f86348e4f0)
![IMG_4189](https://github.com/user-attachments/assets/2d9f3e99-3bc6-44db-b427-9a5140eb22c2)

---

## Visual Overhaul

### Electric Blue Unified Theme
Complete color overhaul replacing HALEHOUND_CYAN with HALEHOUND_MAGENTA (electric blue 0x041F) as primary UI color across every module. Hot pink highlights for selected items. 358 color swaps across 17 files — zero logic changes. Every screen, every module, unified look.

---

## Bug Fixes

- **Wardriving GPS linkage** — GPS UART2 was closed after GPS screen exit, causing NO FIX in wardriving. Added gpsStartBackground()/gpsStopBackground() to reopen UART2 independently without full GPS scan screen
- **Wardriving WiFi scan errors** — Errors now display on TFT instead of dead Serial, with retry logic on first scan failure
- **ProtoKill channel mapping** — WiFi was using protocol channel numbers (1-12) instead of NRF24 offsets, only jamming 2401-2412MHz. Now covers ch 1/6/11 with 5MHz hops across each 22MHz-wide channel. RC expanded to full 2.4GHz band. Zigbee converted from 802.15.4 channel numbers to correct NRF24 offsets
- **Auth Flood radio init** — Channel switch now inside init with test frame verification before declaring ready. Fixes first-start failure
- **Auth Flood crash bombs** — Removed ALL ESP_ERROR_CHECK crash bombs from WiFi modules. Errors are handled gracefully now
- **Auth Flood debounce** — 300ms state transition cooldown prevents double-fire on touch

---

## Full Changelog

fe3d397 Add WhisperPair CVE-2025-36911 Fast Pair vulnerability scanner
37a8a7e Electric blue color overhaul — unified theme across all screens
26f1cd5 Fix ProtoKill channel mapping — WiFi/RC/Zigbee now hit correct frequencies
a89fa16 Add AirTag Detect — Apple FindMy tracker detection module
4380360 Auth Flood overhaul — radio init fix, UI redesign, crash bomb removal
3dc7c10 Add Radio Test tool + fix wardriving GPS linkage and WiFi scan
82572ed Auth Flood equalizer + wiring block diagrams
5204f4a v2.7.0 release — version bump, README updates
ASSETS: 
==========================================================================================
## v2.6.0 | HaleHound-CYD v2.6.0 — Runtime Screen Rotation | 2026-02-18T19:09:55Z
## What's New

Single firmware binary replaces all rotation variants. One file works on every CYD board — no more guessing which bin to download.

### Runtime Screen Rotation
- **Settings > Rotation** — pick Standard, 180°, 90° CW, or 90° CCW
- Touch automatically recalibrates after every rotation change
- Saved to EEPROM — survives power cycles
- Portrait orientation only — 90° options compensate for sideways-mounted LCD panels, not landscape mode

### Single Binary Flash
- One `HaleHound-CYD-FULL.bin` for all boards (flash to 0x0)
- Four-file method also available for boards with flash mode issues
- Old rotation variant bins (Standard, 180, 90CW, 90CCW) are removed

### Dynamic Screen Dimensions
- All UI elements now use runtime screen dimensions instead of hardcoded pixel values
- Fixes layout issues when switching between rotations

## Download

Grab `HaleHound-CYD-v2.6.0-flash-package.zip` below — contains everything you need to flash. See FLASH_INSTRUCTIONS.txt inside the zip for step-by-step guide.

## First Boot

1. Flash the firmware
2. Touch calibration runs automatically — tap the 4 crosshairs
3. If display is upside-down or sideways: Settings > Rotation
4. Done
ASSETS: 