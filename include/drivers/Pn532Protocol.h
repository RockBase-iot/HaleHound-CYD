#pragma once

// PN532 host-controller frame layer: the PN532's own wire format, as pure
// logic.
//
// The part speaks a small framed protocol over I2C: preamble, length and its
// ones-complement, a direction byte, the command and its parameters, a data
// checksum and a postamble. The reply has the same shape with the opposite
// direction byte.
//
// Why this exists when the repository already links the Adafruit library: its
// inDataExchange() collapses the PN532's own status byte into a bool, which
// merges "the card rejected this key" with "the I2C bus fell over". Key Check
// cannot report honestly behind an interface that cannot tell those two apart,
// and a bus fault silently counted as a rejected key would understate what the
// card protects. The codec here keeps the status byte, and being pure logic it
// is pinned by the host suites instead of only by a card on an antenna.
//
// Source: PN532 User Manual UM0701-02 §7.1 (frames) and Table 13 (status codes).

#include <cstdint>

#include "drivers/IRfidTransport.h"

// Command codes (UM0701-02 §7.2).
constexpr uint8_t kPn532CmdGetFirmwareVersion = 0x02;
constexpr uint8_t kPn532CmdGetGeneralStatus = 0x04;
constexpr uint8_t kPn532CmdReadRegister = 0x06;
constexpr uint8_t kPn532CmdWriteRegister = 0x08;
constexpr uint8_t kPn532CmdWriteGpio = 0x0E;
constexpr uint8_t kPn532CmdSamConfiguration = 0x14;
constexpr uint8_t kPn532CmdPowerDown = 0x16;
constexpr uint8_t kPn532CmdRfConfiguration = 0x32;
constexpr uint8_t kPn532CmdInDataExchange = 0x40;
constexpr uint8_t kPn532CmdInCommunicateThru = 0x42;
constexpr uint8_t kPn532CmdInListPassiveTarget = 0x4A;
constexpr uint8_t kPn532CmdInRelease = 0x52;
constexpr uint8_t kPn532CmdInSelect = 0x54;
constexpr uint8_t kPn532CmdTgInitAsTarget = 0x8C;
constexpr uint8_t kPn532CmdTgGetData = 0x86;
constexpr uint8_t kPn532CmdTgSetData = 0x8E;

// Response codes are the command code plus one (UM0701-02 §7.2).
constexpr uint8_t kPn532RespInDataExchange = 0x41;
constexpr uint8_t kPn532RespInListPassiveTarget = 0x4B;

constexpr uint8_t kPn532HostToPn532 = 0xD4;
constexpr uint8_t kPn532Pn532ToHost = 0xD5;
constexpr uint8_t kPn532Preamble = 0x00;
constexpr uint8_t kPn532StartCode1 = 0x00;
constexpr uint8_t kPn532StartCode2 = 0xFF;
constexpr uint8_t kPn532Postamble = 0x00;
// Default 7-bit address is 0x48 >> 1. Every PN532 breakout ships in this mode.
constexpr uint8_t kPn532I2cAddress = 0x24;
// UM0701-02 §7.1.1: the ACK frame, which carries no direction byte.
constexpr uint8_t kPn532AckLength = 6;
extern const uint8_t kPn532AckFrame[kPn532AckLength];

// Largest host frame payload the length byte can express (TFI + command +
// parameters). kRfidMaxFrame is smaller, so a data-exchange frame always fits.
constexpr uint16_t kPn532MaxPayload = 255;
// Worst-case encoded host frame: 3 preamble bytes + LEN + LCS + payload + DCS +
// postamble.
constexpr uint16_t kPn532MaxFrame = kPn532MaxPayload + 7;

// ---- status --------------------------------------------------------------
// UM0701-02 Table 13. Only the codes this firmware can act on are named;
// everything else decodes to Unknown rather than to a made-up cause.
enum class Pn532Status : uint8_t {
    Ok = 0x00,
    Timeout = 0x01,
    CrcError = 0x02,
    ParityError = 0x03,
    ErroneousBitCount = 0x04,
    FramingError = 0x05,
    AbnormalCollision = 0x06,
    BufferTooSmall = 0x07,
    RfBufferOverflow = 0x09,
    RfFieldNotSwitched = 0x0A,
    RfProtocolError = 0x0B,
    TemperatureError = 0x0D,
    InternalBufferOverflow = 0x0E,
    InvalidParameter = 0x10,
    DepDataFormatMismatch = 0x13,
    MifareAuthError = 0x14,
    UidCheckByteWrong = 0x23,
    OperationNotAllowed = 0x26,
    CommandNotAcceptable = 0x27,
    TargetReleased = 0x29,
    Unknown = 0xFF,
};

Pn532Status pn532StatusFromByte(uint8_t byte);
const char* pn532StatusLabel(Pn532Status status);
// The one place that knows 0x14 means "the card said no" rather than "the bus
// said no". Everything above the transport relies on this mapping to report
// honestly.
RfidStatus pn532StatusToRfid(Pn532Status status);

// ---- host -> PN532 ------------------------------------------------------
// Encodes TFI + command + parameters into a full I2C write payload, including
// preamble, LEN, LCS, direction byte, DCS and postamble. `out` must hold
// kPn532MaxFrame bytes. Returns the byte count, or 0 when it does not fit.
uint16_t pn532EncodeCommand(const uint8_t* command,
                            uint16_t commandLength,
                            uint8_t* out);

// ---- PN532 -> host ------------------------------------------------------
struct Pn532Frame {
    bool valid = false;
    uint8_t direction = 0;  // 0xD5 for a reply from the PN532
    uint8_t code = 0;       // response code, e.g. kPn532RespInDataExchange
    uint8_t length = 0;     // payload bytes following the response code
    uint8_t data[256] = {};
};

// Decodes a reply that starts at its preamble (i.e. the byte after the I2C
// status byte). Checks the preamble pair, LCS and DCS. Any mismatch returns
// false, so a truncated I2C read can never be mistaken for a short-but-good
// reply -- which matters because the difference between a truncated
// InDataExchange reply and a real MIFARE NAK is one byte.
bool pn532DecodeFrame(const uint8_t* raw, uint16_t rawLength, Pn532Frame& out);
// True when `raw` is the ACK frame, allowing for the trailing postamble byte
// some I2C reads include.
bool pn532IsAckFrame(const uint8_t* raw, uint16_t rawLength);

// ---- typed replies -----------------------------------------------------
struct Pn532FirmwareInfo {
    bool valid = false;
    uint8_t ic = 0;
    uint8_t version = 0;
    uint8_t revision = 0;
    uint8_t support = 0;
};

bool pn532ParseFirmwareVersion(const Pn532Frame& frame, Pn532FirmwareInfo& out);

struct Pn532PassiveTarget {
    uint8_t targetNumber = 0;
    uint16_t atqa = 0;
    uint8_t sak = 0;
    uint8_t uid[10] = {};
    uint8_t uidLength = 0;
    uint8_t ats[32] = {};
    uint8_t atsLength = 0;
};

// Number of targets in an InListPassiveTarget reply, 0 when the field is
// empty. Returns 0xFF when `frame` is not an InListPassiveTarget reply at all,
// which is what lets the caller separate "no card" from "reader answered
// something I do not understand".
uint8_t pn532PassiveTargetCount(const Pn532Frame& frame);

// Parses a one-target InListPassiveTarget reply for 106 kbps type A. Returns
// false when the count is not exactly one, when the reply is malformed, or when
// the advertised UID length exceeds kRfidMaxUid.
bool pn532ParsePassiveTarget(const Pn532Frame& frame, Pn532PassiveTarget& out);
