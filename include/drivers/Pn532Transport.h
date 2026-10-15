#pragma once

// PN532 over I2C: the board's actual RFID reader.
//
// The transport is split in two so the command sequencing -- wake, firmware
// check, SAMConfiguration, InListPassiveTarget, InDataExchange, release -- is
// host-testable with a scripted link instead of only with a card on an antenna.
// `IPn532Link` is the byte-level I2C seam; `WirePn532Link` is the hardware
// implementation of it and is the only part that needs Arduino.
//
// Command and reply formats live in Pn532Protocol.h. The MIFARE and NTAG
// command bytes that ride inside InDataExchange live in Iso14443aFraming.h.
//
// Wiring: the NM-RF-HAT puts the PN532 on I2C (see ExpansionRoutes.cpp). The
// pin pair comes from the ExpansionRoute, not from cyd_config.h, so the route
// table stays the single description of what is plugged in where.

#include <cstdint>

#include "board/BoardProfile.h"
#include "drivers/IRfidTransport.h"
#include "drivers/Pn532Protocol.h"

// Byte-level I2C seam. Ready-state, one write, one read -- nothing above knows
// how many bytes a transfer costs.
//
// `read` returns frame bytes only: the leading I2C ready byte is consumed by
// the link, because every caller would otherwise have to strip it and one of
// them would eventually forget. `ready()` reads that byte on its own.
class IPn532Link {
public:
    virtual ~IPn532Link() = default;
    // Opens the bus. `sda`/`scl` of -1 mean the platform default pair.
    virtual bool begin(uint8_t address, int sda, int scl) = 0;
    virtual void end() = 0;
    virtual bool write(const uint8_t* data, uint16_t length) = 0;
    virtual bool read(uint8_t* data, uint16_t length) = 0;
    // True when the PN532 has a reply waiting, i.e. its I2C ready byte reads
    // 0x01.
    virtual bool ready() = 0;
    virtual uint32_t millis() const = 0;
    virtual void delayMs(uint32_t ms) = 0;
};

// Real hardware. Declared here without pulling in Wire.h so the host build --
// and every suite that only needs the scripted link -- never sees Arduino.
class WirePn532Link final : public IPn532Link {
public:
    bool begin(uint8_t address, int sda, int scl) override;
    void end() override;
    bool write(const uint8_t* data, uint16_t length) override;
    bool read(uint8_t* data, uint16_t length) override;
    bool ready() override;
    uint32_t millis() const override;
    void delayMs(uint32_t ms) override;

private:
    uint8_t address_ = kPn532I2cAddress;
    int sda_ = -1;
    int scl_ = -1;
    bool started_ = false;
};

class Pn532Transport final : public IRfidTransport {
public:
    // `routeSda`/`routeScl` come from the ExpansionRoute for this board. -1
    // means "use the platform default pair" (the classic CYD profile).
    Pn532Transport(IPn532Link& link, int routeSda = -1, int routeScl = -1);

    bool begin() override;
    void end() override;
    bool ready() const override { return initialized_; }

    const char* name() const override { return "PN532 I2C"; }
    RfidTransportCaps caps() const override;
    uint32_t firmwareVersion() override { return firmwareVersion_; }

    RfidStatus fieldOn() override;
    RfidStatus fieldOff() override;

    RfidStatus activateIso14443a(RfidTarget& out, uint32_t timeoutMs) override;
    RfidStatus releaseTarget() override;

    RfidStatus transceive(const uint8_t* tx, uint16_t txLength,
                          uint8_t* rx, uint16_t& rxLength,
                          uint32_t timeoutMs) override;

    // Firmware string for the status line, e.g. "PN532 1.6". Empty before
    // begin() succeeds.
    const char* firmwareText() const { return firmwareText_; }
    // Last status the PN532 itself reported, for the honest-report line.
    Pn532Status lastStatus() const { return lastStatus_; }

private:
    // Encodes and writes `command`, then waits for the reader's 6-byte ACK.
    bool sendCommand(const uint8_t* command, uint16_t length, uint32_t timeoutMs);
    // Sends a command and returns its decoded reply. A reply whose framing does
    // not check out, or whose response code belongs to a different command, is
    // RfidStatus::FrameError -- never a card verdict.
    RfidStatus exchange(const uint8_t* command, uint16_t length,
                        Pn532Frame& frame, uint32_t timeoutMs);
    // For commands whose reply payload is not interesting, e.g. RFConfiguration
    // and SAMConfiguration. Still surfaces a non-zero reader status.
    RfidStatus exchangeDiscard(const uint8_t* command, uint16_t length,
                               uint32_t timeoutMs);
    // One InDataExchange with the card. On a non-zero reader status the return
    // value carries the reason and `payloadLength` is 0, so a rejected command
    // can never be mistaken for a successful zero-length read.
    RfidStatus dataExchange(const uint8_t* cardData, uint16_t cardLength,
                            uint8_t* payload, uint16_t& payloadLength,
                            uint32_t timeoutMs);
    bool waitReady(uint32_t timeoutMs);
    bool writeFirmwareText(uint8_t ic, uint8_t major, uint8_t minor);

    IPn532Link& link_;
    int sda_;
    int scl_;
    bool initialized_ = false;
    // Which target number InListPassiveTarget handed out; InDataExchange and
    // InRelease both address a target by number.
    uint8_t targetNumber_ = 0;
    // Kept because the MIFARE authentication frame this transport sends has to
    // carry the UID; see transceive().
    uint8_t targetUid_[kRfidMaxUid] = {};
    uint8_t targetUidLength_ = 0;
    uint32_t firmwareVersion_ = 0;
    char firmwareText_[16] = {};
    Pn532Status lastStatus_ = Pn532Status::Ok;
};

// The I2C pin pair PN532 is wired to on `board`, taken from the route table.
struct Pn532Route {
    int sda = -1;
    int scl = -1;
    bool i2c = false;
};

Pn532Route pn532RouteForBoard(BoardId board);
