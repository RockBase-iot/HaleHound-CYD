#include "drivers/Pn532Transport.h"

#include <cstdio>
#include <cstring>

#include "board/ExpansionRoute.h"

namespace {

// The PN532 leaves power-down only after a bare write to its own address; the
// first real command after power-up is otherwise swallowed and the init looks
// like a dead part.
constexpr uint8_t kPn532Wakeup[1] = {0x55};
// Settling time after the wake-up write, from the Adafruit driver's own init
// sequence.
constexpr uint32_t kWakeSettleMs = 5;

// How long the host waits for the reader to signal a reply. InListPassiveTarget
// waits out the card poll inside the part, so this has to exceed that.
constexpr uint32_t kReplyTimeoutMs = 1000;
constexpr uint32_t kAckTimeoutMs = 1000;
constexpr uint8_t kFirmwareProbeAttempts = 5;

// The PN532's host-interface buffer is 64 bytes, so a reply never exceeds it
// and a fixed-size read is what the reference drivers do too.
constexpr uint16_t kReplyReadSize = 64;

// RFConfiguration items (UM0701-02 §7.3.1).
constexpr uint8_t kRfConfigField = 0x01;
constexpr uint8_t kRfConfigMaxRetries = 0x05;

// Bounded, deliberately: MxRtyPassiveActivation = 0xFF means "poll until a card
// appears", which turns activateIso14443a() into a blocking call and freezes a
// UI that expects to poll and draw. One retry keeps the call short so "no card
// in field" is a status the screen can render.
constexpr uint8_t kMaxRtyATR = 0xFF;
constexpr uint8_t kMaxRtyPSL = 0x01;
constexpr uint8_t kMaxRtyPassiveActivation = 0x01;

// SAMConfiguration: normal mode, 1 s timeout, IRQ pin used.
constexpr uint8_t kSamNormalMode = 0x01;
constexpr uint8_t kSamTimeout = 0x14;
constexpr uint8_t kSamUseIrq = 0x01;

// A MIFARE authentication frame is 0x60/0x61 + block + 6 key bytes.
constexpr uint16_t kAuthFrameLength = 8;

}  // namespace

Pn532Transport::Pn532Transport(IPn532Link& link, int routeSda, int routeScl)
    : link_(link), sda_(routeSda), scl_(routeScl) {}

bool Pn532Transport::waitReady(uint32_t timeoutMs) {
    const uint32_t start = link_.millis();
    for (;;) {
        if (link_.ready()) {
            return true;
        }
        if (link_.millis() - start >= timeoutMs) {
            return false;
        }
        // Poll with a gap rather than hammering the bus; one requestFrom per
        // millisecond is already far more often than the part answers.
        link_.delayMs(1);
    }
}

bool Pn532Transport::sendCommand(const uint8_t* command, uint16_t length,
                                 uint32_t timeoutMs) {
    uint8_t encoded[kPn532MaxFrame];
    const uint16_t encodedLength = pn532EncodeCommand(command, length, encoded);
    if (encodedLength == 0U) {
        return false;
    }
    if (!link_.write(encoded, encodedLength)) {
        return false;
    }
    // The part needs a moment to digest the command before it raises ready.
    // Two milliseconds is what the reference driver waits on I2C.
    link_.delayMs(2);
    // A command the part accepts is answered with the 6-byte ACK. A malformed
    // one gets no ACK at all, so this timeout is the only way to learn that the
    // host built a bad frame.
    if (!waitReady(timeoutMs)) {
        return false;
    }
    uint8_t ack[kPn532AckLength];
    if (!link_.read(ack, sizeof(ack))) {
        return false;
    }
    return pn532IsAckFrame(ack, sizeof(ack));
}

RfidStatus Pn532Transport::exchange(const uint8_t* command, uint16_t length,
                                    Pn532Frame& frame, uint32_t timeoutMs) {
    frame = Pn532Frame{};
    if (!sendCommand(command, length, timeoutMs)) {
        lastStatus_ = Pn532Status::Unknown;
        return RfidStatus::IoError;
    }
    if (!waitReady(timeoutMs)) {
        return RfidStatus::Timeout;
    }
    uint8_t raw[kReplyReadSize] = {};
    if (!link_.read(raw, sizeof(raw))) {
        return RfidStatus::IoError;
    }
    if (!pn532DecodeFrame(raw, sizeof(raw), frame)) {
        // A reply whose framing does not check out is a host/bus problem, never
        // a card verdict -- so it must not be mapped onto AuthFailed.
        return RfidStatus::FrameError;
    }
    if (frame.direction != kPn532Pn532ToHost) {
        return RfidStatus::FrameError;
    }
    if (frame.code != static_cast<uint8_t>(command[0] + 1U)) {
        // The part answered something other than this command's reply, which
        // happens if a previous exchange was abandoned mid-read.
        return RfidStatus::FrameError;
    }
    return RfidStatus::Ok;
}

RfidStatus Pn532Transport::exchangeDiscard(const uint8_t* command, uint16_t length,
                                           uint32_t timeoutMs) {
    Pn532Frame frame;
    const RfidStatus status = exchange(command, length, frame, timeoutMs);
    if (status != RfidStatus::Ok) {
        return status;
    }
    // Most commands answer with just their code; RFConfiguration and
    // SAMConfiguration are the ones that append a status byte, so a status is
    // read when it is there and not required when it is not.
    if (frame.length >= 1U) {
        const Pn532Status readerStatus = pn532StatusFromByte(frame.data[0]);
        lastStatus_ = readerStatus;
        const RfidStatus mapped = pn532StatusToRfid(readerStatus);
        if (mapped != RfidStatus::Ok) {
            return mapped;
        }
    }
    return RfidStatus::Ok;
}

bool Pn532Transport::writeFirmwareText(uint8_t ic, uint8_t major, uint8_t minor) {
    const int written = std::snprintf(firmwareText_, sizeof(firmwareText_),
                                      "PN5%02X %u.%u", ic, major, minor);
    return written > 0;
}

bool Pn532Transport::begin() {
    if (initialized_) {
        return true;
    }
    if (!link_.begin(kPn532I2cAddress, sda_, scl_)) {
        return false;
    }
    link_.write(kPn532Wakeup, sizeof(kPn532Wakeup));
    link_.delayMs(kWakeSettleMs);

    // The part occasionally misses the first command after a cold start, which
    // is why the reference drivers retry the firmware probe rather than
    // declaring the board dead on the first miss.
    uint32_t version = 0;
    Pn532Status readerStatus = Pn532Status::Unknown;
    bool probed = false;
    for (uint8_t attempt = 0; attempt < kFirmwareProbeAttempts && !probed;
         ++attempt) {
        const uint8_t command[1] = {kPn532CmdGetFirmwareVersion};
        Pn532Frame frame;
        if (exchange(command, sizeof(command), frame, kReplyTimeoutMs) !=
            RfidStatus::Ok) {
            continue;
        }
        Pn532FirmwareInfo info;
        if (!pn532ParseFirmwareVersion(frame, info)) {
            continue;
        }
        readerStatus = pn532StatusFromByte(frame.data[0]);
        (void)readerStatus;
        version = (static_cast<uint32_t>(info.ic) << 24) |
                  (static_cast<uint32_t>(info.version) << 16) |
                  (static_cast<uint32_t>(info.revision) << 8) |
                  static_cast<uint32_t>(info.support);
        writeFirmwareText(info.ic, info.version, info.revision);
        probed = true;
    }
    if (!probed) {
        link_.end();
        return false;
    }
    firmwareVersion_ = version;

    const uint8_t sam[4] = {kPn532CmdSamConfiguration, kSamNormalMode, kSamTimeout,
                            kSamUseIrq};
    if (exchangeDiscard(sam, sizeof(sam), kReplyTimeoutMs) != RfidStatus::Ok) {
        link_.end();
        return false;
    }

    const uint8_t retries[5] = {kPn532CmdRfConfiguration, kRfConfigMaxRetries,
                                kMaxRtyATR, kMaxRtyPSL, kMaxRtyPassiveActivation};
    (void)exchangeDiscard(retries, sizeof(retries), kReplyTimeoutMs);

    initialized_ = true;
    return true;
}

void Pn532Transport::end() {
    initialized_ = false;
    targetNumber_ = 0;
    targetUidLength_ = 0;
    link_.end();
    targetUid_[0] = 0x00;
}

RfidTransportCaps Pn532Transport::caps() const {
    // The silicon supports ISO14443-B and FeliCa, but this driver only builds
    // type A frames -- claiming B would let a screen offer a mode that cannot
    // work. Emulation is claimed because TgInitAsTarget is reachable; the RFID
    // screens do not use it yet.
    return RfidTransportCaps{
        true,   // iso14443a
        false,  // iso14443b   - silicon can, this driver does not
        false,  // iso15693
        true,   // isoDep
        true,   // mifareClassic
        true,   // mifareUltralight
        true,   // ntag
        false,  // iclass      - not supported by the PN532 silicon
        true,   // rawTransceive
        true,   // siliconAnticollision - InListPassiveTarget does the whole of ISO14443-3
        true,   // emulate
        false,  // lfSupport
        false,  // mfkey32NonceCapture - the part never exposes the Crypto1 nonce
    };
}

RfidStatus Pn532Transport::fieldOn() {
    if (!initialized_) {
        return RfidStatus::Unsupported;
    }
    const uint8_t command[3] = {kPn532CmdRfConfiguration, kRfConfigField, 0x01};
    return exchangeDiscard(command, sizeof(command), kReplyTimeoutMs);
}

RfidStatus Pn532Transport::fieldOff() {
    if (!initialized_) {
        return RfidStatus::Unsupported;
    }
    const uint8_t command[3] = {kPn532CmdRfConfiguration, kRfConfigField, 0x00};
    return exchangeDiscard(command, sizeof(command), kReplyTimeoutMs);
}

RfidStatus Pn532Transport::activateIso14443a(RfidTarget& out, uint32_t timeoutMs) {
    out = RfidTarget{};
    if (!initialized_) {
        return RfidStatus::Unsupported;
    }
    // BrTy 0x00 = 106 kbps type A, MaxTg 1: a second card in the field is a
    // collision the operator has to resolve, not something to pick between
    // silently.
    const uint8_t command[3] = {kPn532CmdInListPassiveTarget, 0x01, 0x00};
    const uint32_t wait = (timeoutMs == 0U) ? kReplyTimeoutMs : timeoutMs;
    Pn532Frame frame;
    const RfidStatus status = exchange(command, sizeof(command), frame, wait);
    if (status != RfidStatus::Ok) {
        return status;
    }
    const uint8_t count = pn532PassiveTargetCount(frame);
    if (count == 0U) {
        // Nothing in the field. Not an error: the caller renders "no card".
        targetNumber_ = 0;
        targetUidLength_ = 0;
        return RfidStatus::NoTarget;
    }
    Pn532PassiveTarget target;
    if (!pn532ParsePassiveTarget(frame, target)) {
        targetNumber_ = 0;
        targetUidLength_ = 0;
        return RfidStatus::FrameError;
    }
    targetNumber_ = target.targetNumber;
    targetUidLength_ = target.uidLength;
    std::memcpy(targetUid_, target.uid, target.uidLength);
    out.kind = RfidTargetKind::Iso14443A;
    std::memcpy(out.uid.data(), target.uid, target.uidLength);
    out.uidLength = target.uidLength;
    out.atqa = target.atqa;
    out.sak = target.sak;
    out.atsLength = target.atsLength;
    if (target.atsLength > 0U) {
        std::memcpy(out.ats.data(), target.ats, target.atsLength);
    }
    return RfidStatus::Ok;
}

RfidStatus Pn532Transport::releaseTarget() {
    if (!initialized_ || targetNumber_ == 0U) {
        return RfidStatus::NoTarget;
    }
    const uint8_t command[2] = {kPn532CmdInRelease, targetNumber_};
    const RfidStatus status = exchangeDiscard(command, sizeof(command), kReplyTimeoutMs);
    targetNumber_ = 0;
    // Cleared with the target number, for the same reason: the UID is spliced
    // into the MIFARE authentication frame, and a stale one would be sent to
    // whichever card is selected next.
    targetUidLength_ = 0;
    return status;
}

RfidStatus Pn532Transport::transceive(const uint8_t* tx, uint16_t txLength,
                                     uint8_t* rx, uint16_t& rxLength,
                                     uint32_t timeoutMs) {
    if (!initialized_) {
        rxLength = 0;
        return RfidStatus::Unsupported;
    }
    if (tx == nullptr || txLength == 0U) {
        rxLength = 0;
        return RfidStatus::FrameError;
    }
    if (targetNumber_ == 0U) {
        // Nothing selected: nowhere for the frame to go. Reporting this as a
        // card verdict would be a lie.
        rxLength = 0;
        return RfidStatus::NoTarget;
    }

    // MIFARE authentication is the one place this driver deviates from the
    // datasheet frame. UM0701-02 §7.3.9 documents InDataExchange carrying
    // 0x60/0x61 + block + key[6]; the driver this board's RFID code has always
    // used appends the PICC UID as well, making the frame 10 + uidLength bytes,
    // and that is the form proven against real PN532 silicon. The expansion
    // lives here, in the transport that owns the quirk, so
    // Iso14443aFraming::buildAuthenticate() stays the canonical 8-byte frame
    // and a bare transceiver (the reserved ST25R3916 path) sends that instead.
    uint8_t authFrame[10 + kRfidMaxUid];
    const uint8_t* cardData = tx;
    uint16_t cardLength = txLength;
    if (txLength == kAuthFrameLength &&
        (tx[0] == 0x60U || tx[0] == 0x61U) && targetUidLength_ > 0U &&
        targetUidLength_ <= kRfidMaxUid) {
        std::memcpy(authFrame, tx, txLength);
        std::memcpy(authFrame + txLength, targetUid_, targetUidLength_);
        cardData = authFrame;
        cardLength = static_cast<uint16_t>(txLength + targetUidLength_);
    }

    // rxLength goes in as the caller's capacity and comes back as the byte
    // count, so it must NOT be zeroed here: dataExchange() checks the payload
    // against that capacity and reports Unsupported when it does not fit.
    // Clearing it first made every reply with a payload look too large.
    const RfidStatus status = dataExchange(cardData, cardLength, rx, rxLength, timeoutMs);
    return status;
}

RfidStatus Pn532Transport::dataExchange(const uint8_t* cardData,
                                        uint16_t cardLength,
                                        uint8_t* payload,
                                        uint16_t& payloadLength,
                                        uint32_t timeoutMs) {
    uint8_t command[2 + kRfidMaxFrame];
    if (static_cast<uint32_t>(cardLength) + 2U > sizeof(command)) {
        payloadLength = 0;
        return RfidStatus::Unsupported;
    }
    command[0] = kPn532CmdInDataExchange;
    command[1] = targetNumber_;
    std::memcpy(command + 2, cardData, cardLength);

    Pn532Frame frame;
    const RfidStatus transport =
        exchange(command, static_cast<uint16_t>(cardLength + 2U), frame, timeoutMs);
    if (transport != RfidStatus::Ok) {
        payloadLength = 0;
        return transport;
    }
    if (frame.length == 0U) {
        payloadLength = 0;
        return RfidStatus::FrameError;
    }
    const Pn532Status readerStatus = pn532StatusFromByte(frame.data[0]);
    lastStatus_ = readerStatus;
    const RfidStatus mapped = pn532StatusToRfid(readerStatus);
    if (mapped != RfidStatus::Ok) {
        // The card's verdict. The frame carries no payload in this case, and
        // callers key off the return value -- never off the payload -- so a
        // rejected key cannot be read as a successful read of zero bytes.
        payloadLength = 0;
        return mapped;
    }
    const uint16_t available = static_cast<uint16_t>(frame.length - 1U);
    if (available == 0U) {
        payloadLength = 0;
        return RfidStatus::Ok;
    }
    if (payload == nullptr || available > payloadLength) {
        // Truncating silently would hand the caller a short read that looks
        // like a short card response.
        payloadLength = 0;
        return RfidStatus::Unsupported;
    }
    std::memcpy(payload, frame.data + 1, available);
    payloadLength = available;
    return RfidStatus::Ok;
}

Pn532Route pn532RouteForBoard(BoardId board) {
    const ExpansionRoute& route = findRoute(board, PeripheralId::Pn532);
    Pn532Route out;
    out.i2c = route.transport == Transport::I2c;
    if (out.i2c) {
        out.sda = route.controlA;
        out.scl = route.controlB;
    }
    return out;
}
