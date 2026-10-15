#include "drivers/Pn532Protocol.h"

const uint8_t kPn532AckFrame[kPn532AckLength] = {0x00, 0x00, 0xFF,
                                                0x00, 0xFF, 0x00};

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

Pn532Status pn532StatusFromByte(uint8_t byte) {
    switch (byte) {
        case 0x00:
            return Pn532Status::Ok;
        case 0x01:
            return Pn532Status::Timeout;
        case 0x02:
            return Pn532Status::CrcError;
        case 0x03:
            return Pn532Status::ParityError;
        case 0x04:
            return Pn532Status::ErroneousBitCount;
        case 0x05:
            return Pn532Status::FramingError;
        case 0x06:
            return Pn532Status::AbnormalCollision;
        case 0x07:
            return Pn532Status::BufferTooSmall;
        case 0x09:
            return Pn532Status::RfBufferOverflow;
        case 0x0A:
            return Pn532Status::RfFieldNotSwitched;
        case 0x0B:
            return Pn532Status::RfProtocolError;
        case 0x0D:
            return Pn532Status::TemperatureError;
        case 0x0E:
            return Pn532Status::InternalBufferOverflow;
        case 0x10:
            return Pn532Status::InvalidParameter;
        case 0x13:
            return Pn532Status::DepDataFormatMismatch;
        case 0x14:
            return Pn532Status::MifareAuthError;
        case 0x23:
            return Pn532Status::UidCheckByteWrong;
        case 0x26:
            return Pn532Status::OperationNotAllowed;
        case 0x27:
            return Pn532Status::CommandNotAcceptable;
        case 0x29:
            return Pn532Status::TargetReleased;
        default:
            return Pn532Status::Unknown;
    }
}

const char* pn532StatusLabel(Pn532Status status) {
    switch (status) {
        case Pn532Status::Ok:
            return "ok";
        case Pn532Status::Timeout:
            return "card did not answer";
        case Pn532Status::CrcError:
            return "card CRC error";
        case Pn532Status::ParityError:
            return "card parity error";
        case Pn532Status::ErroneousBitCount:
            return "bad bit count during anticollision";
        case Pn532Status::FramingError:
            return "framing error";
        case Pn532Status::AbnormalCollision:
            return "abnormal bit collision";
        case Pn532Status::BufferTooSmall:
            return "reader buffer too small";
        case Pn532Status::RfBufferOverflow:
            return "reader RF buffer overflow";
        case Pn532Status::RfFieldNotSwitched:
            return "RF field not switched on";
        case Pn532Status::RfProtocolError:
            return "RF protocol error";
        case Pn532Status::TemperatureError:
            return "reader overheating";
        case Pn532Status::InternalBufferOverflow:
            return "reader internal buffer overflow";
        case Pn532Status::InvalidParameter:
            return "invalid command parameter";
        case Pn532Status::DepDataFormatMismatch:
            return "data format does not match the specification";
        case Pn532Status::MifareAuthError:
            return "authentication rejected by the card";
        case Pn532Status::UidCheckByteWrong:
            return "UID check byte wrong";
        case Pn532Status::OperationNotAllowed:
            return "operation not allowed in this reader state";
        case Pn532Status::CommandNotAcceptable:
            return "command not acceptable in this context";
        case Pn532Status::TargetReleased:
            return "card was released by the initiator";
        case Pn532Status::Unknown:
        default:
            return "unrecognised reader status";
    }
}

RfidStatus pn532StatusToRfid(Pn532Status status) {
    switch (status) {
        case Pn532Status::Ok:
            return RfidStatus::Ok;
        case Pn532Status::Timeout:
            return RfidStatus::Timeout;
        case Pn532Status::CrcError:
        case Pn532Status::ParityError:
            return RfidStatus::CrcError;
        case Pn532Status::ErroneousBitCount:
        case Pn532Status::FramingError:
        case Pn532Status::DepDataFormatMismatch:
            return RfidStatus::FrameError;
        case Pn532Status::AbnormalCollision:
            return RfidStatus::Collision;
        // The distinction the whole honest-reporting contract rests on: an
        // authentication error is the card's verdict on the key, not a fault.
        case Pn532Status::MifareAuthError:
            return RfidStatus::AuthFailed;
        // UidCheckByteWrong means the wrong card answered the select, so the
        // activation must be redone -- FrameError, never a card verdict.
        case Pn532Status::UidCheckByteWrong:
            return RfidStatus::FrameError;
        case Pn532Status::TargetReleased:
            return RfidStatus::NoTarget;
        case Pn532Status::RfFieldNotSwitched:
            return RfidStatus::IoError;
        case Pn532Status::OperationNotAllowed:
        case Pn532Status::CommandNotAcceptable:
        case Pn532Status::InvalidParameter:
            return RfidStatus::Unsupported;
        default:
            return RfidStatus::IoError;
    }
}

// ---------------------------------------------------------------------------
// Framing
// ---------------------------------------------------------------------------

uint16_t pn532EncodeCommand(const uint8_t* command,
                            uint16_t commandLength,
                            uint8_t* out) {
    if (command == nullptr || out == nullptr || commandLength == 0U) {
        return 0;
    }
    // LEN counts the direction byte as well as the command and its parameters.
    const uint16_t len = static_cast<uint16_t>(commandLength + 1U);
    if (len > kPn532MaxPayload) {
        return 0;
    }
    out[0] = kPn532Preamble;
    out[1] = kPn532StartCode1;
    out[2] = kPn532StartCode2;
    out[3] = static_cast<uint8_t>(len);
    // LCS is the ones' complement of LEN, i.e. the two's complement of LEN.
    out[4] = static_cast<uint8_t>(~len + 1U);
    out[5] = kPn532HostToPn532;
    uint8_t sum = kPn532HostToPn532;
    for (uint16_t i = 0; i < commandLength; ++i) {
        out[6U + i] = command[i];
        sum = static_cast<uint8_t>(sum + command[i]);
    }
    // DCS covers TFI, command and parameters, and is the two's complement of
    // their sum -- not the ones' complement, despite what several ports print.
    out[6U + commandLength] = static_cast<uint8_t>(~sum + 1U);
    out[7U + commandLength] = kPn532Postamble;
    return static_cast<uint16_t>(commandLength + 8U);
}

bool pn532DecodeFrame(const uint8_t* raw, uint16_t rawLength, Pn532Frame& out) {
    out = Pn532Frame{};
    // Shortest possible frame: 3 preamble bytes + LEN + LCS + code + DCS +
    // postamble, with an empty payload.
    if (raw == nullptr || rawLength < 7U) {
        return false;
    }
    if (raw[0] != kPn532Preamble || raw[1] != kPn532StartCode1 ||
        raw[2] != kPn532StartCode2) {
        return false;
    }
    const uint8_t len = raw[3];
    if (raw[4] != static_cast<uint8_t>(~len + 1U)) {
        return false;
    }
    if (static_cast<uint16_t>(len) + 4U > rawLength) {
        return false;
    }
    // The checksum covers everything between LEN and DCS: direction byte,
    // response code and payload.
    uint8_t sum = 0;
    for (uint16_t i = 0; i < len; ++i) {
        sum = static_cast<uint8_t>(sum + raw[5U + i]);
    }
    const uint16_t dcsIndex = static_cast<uint16_t>(5U + len);
    if (raw[dcsIndex] != static_cast<uint8_t>(~sum + 1U)) {
        return false;
    }
    if (len == 0U) {
        // A frame must at least carry the direction byte.
        return false;
    }
    out.direction = raw[5];
    if (len >= 2U) {
        out.code = raw[6];
        out.length = static_cast<uint8_t>(len - 2U);
        for (uint8_t i = 0; i < out.length; ++i) {
            out.data[i] = raw[7U + i];
        }
    } else {
        out.length = 0;
    }
    out.valid = true;
    return true;
}

bool pn532IsAckFrame(const uint8_t* raw, uint16_t rawLength) {
    if (raw == nullptr || rawLength < kPn532AckLength) {
        return false;
    }
    for (uint16_t i = 0; i < kPn532AckLength; ++i) {
        if (raw[i] != kPn532AckFrame[i]) {
            return false;
        }
    }
    // An I2C read that includes the trailing postamble is still an ACK, but a
    // read with anything else after the ACK is not: that would mean two frames
    // arrived and the host only looked at the first.
    if (rawLength > kPn532AckLength && raw[kPn532AckLength] != kPn532Postamble) {
        return false;
    }
    return rawLength <= kPn532AckLength + 1U;
}

// ---------------------------------------------------------------------------
// Typed replies
// ---------------------------------------------------------------------------

bool pn532ParseFirmwareVersion(const Pn532Frame& frame, Pn532FirmwareInfo& out) {
    out = Pn532FirmwareInfo{};
    if (!frame.valid || frame.code != kPn532CmdGetFirmwareVersion + 1U) {
        return false;
    }
    if (frame.length < 4U) {
        return false;
    }
    out.ic = frame.data[0];
    out.version = frame.data[1];
    out.revision = frame.data[2];
    out.support = frame.data[3];
    out.valid = true;
    return true;
}

uint8_t pn532PassiveTargetCount(const Pn532Frame& frame) {
    if (!frame.valid || frame.code != kPn532RespInListPassiveTarget) {
        return 0xFFU;
    }
    if (frame.length < 1U) {
        return 0xFFU;
    }
    return frame.data[0];
}

bool pn532ParsePassiveTarget(const Pn532Frame& frame, Pn532PassiveTarget& out) {
    out = Pn532PassiveTarget{};
    if (pn532PassiveTargetCount(frame) != 1U) {
        return false;
    }
    // 106 kbps type A layout (UM0701-02 §7.3.5.1):
    //   NbTg, Tg, SENS_RES[2], SEL_RES, NFCIDLength, NFCID[], [ATSLength, ATS[]]
    if (frame.length < 6U) {
        return false;
    }
    out.targetNumber = frame.data[1];
    // ATQA arrives least significant byte first, exactly as the card sent it.
    out.atqa = static_cast<uint16_t>(frame.data[2]) |
               (static_cast<uint16_t>(frame.data[3]) << 8);
    out.sak = frame.data[4];
    out.uidLength = frame.data[5];
    if (out.uidLength == 0U || out.uidLength > kRfidMaxUid) {
        return false;
    }
    if (static_cast<uint16_t>(frame.length) < 6U + out.uidLength) {
        return false;
    }
    for (uint8_t i = 0; i < out.uidLength; ++i) {
        out.uid[i] = frame.data[6U + i];
    }
    const uint16_t atsIndex = static_cast<uint16_t>(6U + out.uidLength);
    if (static_cast<uint16_t>(frame.length) > atsIndex) {
        const uint8_t atsLength = frame.data[atsIndex];
        // ATSLength excludes the length byte. Accepted only if the whole ATS
        // arrived; a partial ATS is a truncated read, not a short ATS.
        if (atsLength <= kRfidMaxAts &&
            static_cast<uint16_t>(frame.length) >=
                static_cast<uint16_t>(atsIndex) + 1U + atsLength) {
            out.atsLength = atsLength;
            for (uint8_t i = 0; i < atsLength; ++i) {
                out.ats[i] = frame.data[atsIndex + 1U + i];
            }
        }
    }
    return true;
}
