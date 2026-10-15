#pragma once

// RFID reader transport: the wire, not the protocol.
//
// Everything above this line -- activation, anticollision, MIFARE command
// frames, ISO-DEP I-Blocks -- is byte-level protocol code and lives under
// src/features/rfid/, so the host suites exercise it through a scripted fake
// transport instead of requiring a card on an antenna.
//
// Two back ends implement this interface:
//
//   Pn532Transport      I2C, 13.56 MHz. The reader actually on this board.
//   St25r3916Transport  SPI, reserved. The interface is declared so the
//                       protocol layer stays transport-agnostic, but the
//                       register driver is not written; every call reports
//                       RfidStatus::Unsupported and names what is missing.
//
// Why activation is split from transceive: the PN532 runs its own ISO14443-3
// state machine in firmware, so on that transport `activateIso14443a()` returns
// a finished UID and no REQA/anticollision syllable ever leaves the host. A
// bare transceiver (the ST25R3916) has no such state machine and must be driven
// frame by frame. `caps().siliconAnticollision` is how the layer above picks
// which of the two happens, without either backend leaking its shape upward.

#include <array>
#include <cstdint>

enum class RfidStatus : uint8_t {
    Ok,
    NoTarget,     // nothing in the field
    Timeout,      // a target answered once, then stopped answering
    FrameError,   // malformed response, or a length the command cannot return
    CrcError,
    Collision,    // more than one target, or an anticollision collision
    AuthFailed,   // MIFARE authentication rejected (card NAKed the key)
    IoError,      // bus-level failure: reader NAK, I2C NACK, FIFO overrun
    Unsupported,  // this transport cannot do it
};

// Human label for the honest-report line. Never returns nullptr.
const char* rfidStatusLabel(RfidStatus status);

enum class RfidTargetKind : uint8_t {
    None,
    Iso14443A,
    Iso14443B,
    Iso15693,
};

constexpr uint8_t kRfidMaxUid = 10;
// ATS is length-prefixed and capped at 254 on the wire; a 32-byte slice covers
// every card this firmware meets (MIFARE Plus / DESFire send 15, the emulated
// Type 4 tags in the field send fewer).
constexpr uint8_t kRfidMaxAts = 32;
// One frame ceiling for both readers: the PN532's own buffer is 264 bytes and
// ISO14443-4 FSD for these cards is 256, so 264 is the binding constraint.
constexpr uint16_t kRfidMaxFrame = 264;

struct RfidTransportCaps {
    bool iso14443a;
    bool iso14443b;
    bool iso15693;
    bool isoDep;                // ISO14443-4: Type 4, DESFire, Plus SL3
    bool mifareClassic;
    bool mifareUltralight;
    bool ntag;
    bool iclass;                // 13.56 MHz HID iCLASS
    bool rawTransceive;         // arbitrary frames, not only the library's verbs
    bool siliconAnticollision;  // the reader finishes ISO14443-3 itself
    bool emulate;               // card emulation / target mode
    bool lfSupport;             // 125 kHz
    bool mfkey32NonceCapture;   // can hand back nested-attack nonces
};

struct RfidTarget {
    RfidTargetKind kind = RfidTargetKind::None;
    std::array<uint8_t, kRfidMaxUid> uid{};
    // On-air order, i.e. uid[0] is the byte sent first. The numeric UID every
    // MIFARE tool prints is the reverse of the first four -- see
    // MifareClassicModel::uid32().
    uint8_t uidLength = 0;
    uint16_t atqa = 0;
    uint8_t sak = 0;
    uint8_t atsLength = 0;
    std::array<uint8_t, kRfidMaxAts> ats{};
};

class IRfidTransport {
public:
    virtual ~IRfidTransport() = default;

    // Power the reader up and identify its silicon. False only when the part
    // did not answer at all; a part that answers but lacks a capability says so
    // in caps(), not here, so a reader that cannot do iCLASS still runs the
    // screens it can.
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual bool ready() const = 0;

    // Short name for the header bar and the honest-report line, e.g.
    // "PN532 I2C". Never nullptr.
    virtual const char* name() const = 0;
    virtual RfidTransportCaps caps() const = 0;
    // 0 when the part does not report one.
    virtual uint32_t firmwareVersion() = 0;

    virtual RfidStatus fieldOn() = 0;
    virtual RfidStatus fieldOff() = 0;

    // One antenna poll. `timeoutMs` of 0 means a single shot with no wait.
    virtual RfidStatus activateIso14443a(RfidTarget& out, uint32_t timeoutMs) = 0;
    // Drop the selected target so the next activate can pick a different card.
    virtual RfidStatus releaseTarget() = 0;

    // Raw frame exchange with the active target.
    //
    // `tx` is the command exactly as the ISO14443-3/4 or MIFARE datasheet
    // writes it, WITHOUT CRC. `rx` returns with the CRC already stripped when
    // the reader silicon handled it. `rxLength` is in/out: in = capacity of
    // `rx`, out = bytes actually returned. A transport that appends CRC itself
    // must say so by not requiring the caller to have done it.
    virtual RfidStatus transceive(const uint8_t* tx, uint16_t txLength,
                                  uint8_t* rx, uint16_t& rxLength,
                                  uint32_t timeoutMs) = 0;
};

// CRC_A, ISO/IEC 14443-3.
//
// Parameterisation, so nobody "fixes" a correct constant later: polynomial
// 0x1021, reflected in and out, initial value 0xC6C6, final XOR 0x0000, and the
// catalogue check value over "123456789" is 0xBF05. A reflected implementation
// holds that initial value byte-swapped, which is why the code below starts at
// 0x6363 -- the same CRC, written in the register's own orientation, and the
// form libnfc's CRC_A uses.
uint16_t rfidCrcA(const uint8_t* data, uint16_t length);
// Appends the CRC little-endian at data[length], writing 2 bytes. The caller
// must have room for length + 2.
void rfidAppendCrcA(uint8_t* data, uint16_t length);
// True when the trailing two bytes match the CRC of the bytes before them.
// `length` includes the CRC.
bool rfidCheckCrcA(const uint8_t* data, uint16_t length);
