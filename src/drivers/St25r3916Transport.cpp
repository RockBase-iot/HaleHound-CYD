#include "drivers/St25r3916Transport.h"

#include "board/ExpansionRoute.h"

namespace {

// The bring-up order matters and is not arbitrary: the interrupt path has to be
// trusted before FIFO reads can be trusted, and the field has to be under
// control before anything can be activated. Each step names what it unblocks so
// a screen can say *why* it is grey rather than just that it is.
//
// `done` is false throughout. Replacing a false with a true means the step was
// written, compiled on the real toolchain, and exercised on hardware; nothing
// in this file claims otherwise.
const St25r3916BringUpStep kBringUp[] = {
    {"SPI register access",
     "Mode-definition and IO-configuration registers, then read a known "
     "register (IC identity 0x3F) back to prove the bus and the chip select.",
     "the backend appearing at all",
     false},
    {"Direct command path",
     "Direct commands (Set Default, Analog Preset, Calibrate, Adjust "
     "Regulators) and the oscillator settle wait before any field work.",
     "field control",
     false},
    {"Interrupt routing",
     "IRQ mask for RXE/ TXE / collision / FIFO water level, and a bounded wait "
     "on the IRQ pin instead of polling a register.",
     "every timing-sensitive step below",
     false},
    {"Field on/off",
     "Regulator and driver configuration, then fieldOn/fieldOff through the "
     "auxiliary and driver registers, with DAC-based power ramping.",
     "ISO14443 activation",
     false},
    {"ISO14443-A framing in software",
     "REQA/WUPA, anticollision and SELECT built by Iso14443aFraming and clocked "
     "through the FIFO. This part has no activation state machine, so the whole "
     "of ISO14443-3 is host code -- which is why the framing layer was written "
     "transport-agnostic from the start.",
     "card activation",
     false},
    {"CRC and parity configuration",
     "Hardware CRC-A in transmit and receive, and 4-bit (short frame) handling "
     "for REQA/WUPA, which do not carry a CRC at all.",
     "card activation",
     false},
    {"Transceive with timeout",
     "FIFO load, transmit, wait on IRQ with a real timeout, FIFO drain, and a "
     "mapping from CIU error flags onto RfidStatus.",
     "MIFARE Classic, NTAG, ISO-DEP",
     false},
    {"Crypto1 nonce exposure",
     "Return the tagged nonce/response pairs nested and hardnested need, which "
     "the PN532 never gives up.",
     "the nested attack, and honest mfkey32 capture",
     false},
    {"iCLASS mode",
     "ISO15693-style 13.56 MHz framing at 52 kbps plus the 3DES block exchange "
     "so an iCLASS credential can be decrypted on the board.",
     "iCLASS / HID credential reading",
     false},
    {"Card emulation",
     "Target mode so the reader can present a credential rather than only read "
     "one.",
     "HF emulation",
     false},
};

}  // namespace

St25r3916Route st25r3916RouteForBoard(BoardId board) {
    // There is no ST25R3916 route in ExpansionRoutes.cpp today, because the
    // part is not on either board. Rather than invent pins, this reports the
    // absence and the caller keeps the backend greyed out.
    St25r3916Route out;
    out.present = false;
    (void)board;
    return out;
}

const St25r3916BringUpStep* st25r3916BringUpSteps(uint8_t& count) {
    count = static_cast<uint8_t>(sizeof(kBringUp) / sizeof(kBringUp[0]));
    return kBringUp;
}

RfidTransportCaps st25r3916SiliconCaps() {
    // What the part can do once the driver exists. Kept separate from caps() so
    // the picker can distinguish "the silicon cannot" (PN532 and iCLASS) from
    // "the driver cannot yet" (here).
    return RfidTransportCaps{
        true,   // iso14443a
        true,   // iso14443b
        true,   // iso15693
        true,   // isoDep
        true,   // mifareClassic
        true,   // mifareUltralight
        true,   // ntag
        true,   // iclass
        true,   // rawTransceive
        false,  // siliconAnticollision - there is no state machine to delegate to
        true,   // emulate
        false,  // lfSupport - 13.56 MHz part
        true,   // mfkey32NonceCapture
    };
}

RfidTransportCaps St25r3916Transport::caps() const {
    // No capability until the driver exists. Reporting the silicon's abilities
    // here would let the UI offer iCLASS on a backend that cannot read a byte.
    RfidTransportCaps none{};
    none.iso14443a = false;
    none.iso14443b = false;
    none.iso15693 = false;
    none.isoDep = false;
    none.mifareClassic = false;
    none.mifareUltralight = false;
    none.ntag = false;
    none.iclass = false;
    none.rawTransceive = false;
    none.siliconAnticollision = false;
    none.emulate = false;
    none.lfSupport = false;
    none.mfkey32NonceCapture = false;
    return none;
}

const char* St25r3916Transport::unavailableReason() const {
    if (!route_.present) {
        return "no ST25R3916 on this board; driver reserved, not written";
    }
    return "ST25R3916 register driver not implemented";
}

bool St25r3916Transport::begin() {
    // Deliberately false. Returning true here would let a feature start and
    // then fail at the first frame, which is exactly the kind of failure that
    // looks like a card problem.
    return false;
}

void St25r3916Transport::end() {}

RfidStatus St25r3916Transport::fieldOn() {
    return RfidStatus::Unsupported;
}

RfidStatus St25r3916Transport::fieldOff() {
    return RfidStatus::Unsupported;
}

RfidStatus St25r3916Transport::activateIso14443a(RfidTarget& out, uint32_t timeoutMs) {
    (void)timeoutMs;
    out = RfidTarget{};
    return RfidStatus::Unsupported;
}

RfidStatus St25r3916Transport::releaseTarget() {
    return RfidStatus::Unsupported;
}

RfidStatus St25r3916Transport::transceive(const uint8_t* tx, uint16_t txLength,
                                         uint8_t* rx, uint16_t& rxLength,
                                         uint32_t timeoutMs) {
    (void)tx;
    (void)txLength;
    (void)rx;
    (void)timeoutMs;
    rxLength = 0;
    return RfidStatus::Unsupported;
}
