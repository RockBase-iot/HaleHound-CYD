#pragma once

// ST25R3916 transport -- RESERVED INTERFACE, DELIBERATELY NOT IMPLEMENTED.
//
// The ST25R3916 is a bare 13.56 MHz transceiver: unlike the PN532 it has no
// firmware, so every byte of ISO14443-3/4 has to be driven from the host, and
// the parts that make it worth having are exactly the ones that are missing
// today --
//
//   * iCLASS / HID PACS credential reading (the PN532 silicon cannot do it at
//     all, so it is the one capability the current board cannot reach).
//   * MIFARE Classic back-door ("magic card") writes, which need raw frame
//     control the PN532's own state machine will not give up.
//   * Nested / hardnested nonce capture: the PN532 never exposes the Crypto1
//     nonce, so the 29-key sweep is where Key Check stops today.
//
// The interface is declared now, and every method reports
// RfidStatus::Unsupported with a named reason, for one reason: the protocol
// layer above (Iso14443aFraming, MifareClassicDriver, IsoDepSession) is written
// against IRfidTransport and not against the PN532. Declaring the second
// backend while there is exactly one implementation is what keeps that true.
// Nothing here pretends to work -- `caps()` describes the silicon, not this
// driver, and `available()` says so plainly so a screen can grey out.
//
// What is missing, in order, is in st25r3916BringUpSteps(); it is the spec for
// the day the driver lands. Register-level details live in
// docs/protocol-evidence/st25r3916.md, which is deliberately a plan rather than
// an evidence table.

#include <cstdint>

#include "board/BoardProfile.h"
#include "drivers/IRfidTransport.h"

// The board's SPI route for the part, taken from the ExpansionRoute so the
// route table stays the single description of what is wired where.
struct St25r3916Route {
    int cs = -1;
    int irq = -1;
    bool spi = false;
    bool present = false;
};

St25r3916Route st25r3916RouteForBoard(BoardId board);

// One item of the bring-up plan. `done` is false for every step today; the list
// exists so the work can be tracked and so the UI can say which capability is
// blocked by which step, instead of showing a blanket "not supported".
struct St25r3916BringUpStep {
    const char* name;
    const char* detail;
    // Which capability this step unblocks, in the RfidBackendCapability sense.
    const char* unblocks;
    bool done;
};

// Ordered bring-up plan. Returns the array and writes its length to `count`.
const St25r3916BringUpStep* st25r3916BringUpSteps(uint8_t& count);

// The capability matrix row the silicon would earn once the driver lands. Kept
// separate from the live driver so the UI can show "would support iCLASS" next
// to "this driver does not", rather than one number that means neither.
struct RfidTransportCaps st25r3916SiliconCaps();

class St25r3916Transport final : public IRfidTransport {
public:
    explicit St25r3916Transport(St25r3916Route route) : route_(route) {}

    // Always false. A transport that returns true from begin() without a driver
    // behind it would let a feature start and then fail on first use.
    bool begin() override;
    void end() override;
    bool ready() const override { return false; }

    const char* name() const override { return "ST25R3916 SPI"; }
    // Reports no capability while the driver is missing. This is the honest
    // answer: the capability matrix greys out screens, and a screen that offers
    // iCLASS here would be offering an empty promise.
    RfidTransportCaps caps() const override;
    uint32_t firmwareVersion() override { return 0; }

    RfidStatus fieldOn() override;
    RfidStatus fieldOff() override;
    RfidStatus activateIso14443a(RfidTarget& out, uint32_t timeoutMs) override;
    RfidStatus releaseTarget() override;
    RfidStatus transceive(const uint8_t* tx, uint16_t txLength,
                          uint8_t* rx, uint16_t& rxLength,
                          uint32_t timeoutMs) override;

    // False until a driver exists. Callers use this instead of caps() to decide
    // whether to offer the backend at all.
    bool available() const { return false; }
    // Why it is unavailable, for the picker's detail line.
    const char* unavailableReason() const;

private:
    St25r3916Route route_;
};
