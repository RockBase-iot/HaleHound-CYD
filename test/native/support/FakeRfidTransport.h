#pragma once

// Scripted RFID transport for the host suites.
//
// Responses are queued in the order the driver asks for them, so a test reads
// as the card's side of the conversation. Every request is recorded, which is
// what lets a test assert not only what the driver *concluded* but whether a
// frame was sent at all -- the difference between "refused" and "attempted and
// failed" is the entire point of the write-refusal tests.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "drivers/IRfidTransport.h"

class FakeRfidTransport final : public IRfidTransport {
public:
    struct Response {
        RfidStatus status = RfidStatus::IoError;
        std::vector<uint8_t> payload;
    };

    // ---- scripting --------------------------------------------------------
    void queue(RfidStatus status, std::initializer_list<uint8_t> payload = {}) {
        Response response;
        response.status = status;
        response.payload.assign(payload.begin(), payload.end());
        responses_.push_back(std::move(response));
    }

    void queueOk(std::initializer_list<uint8_t> payload) {
        queue(RfidStatus::Ok, payload);
    }

    // Same, from a buffer. A MIFARE block read is 16 bytes; spelling those out
    // as an initializer list in every test buries the point of the test.
    void queueOk(const uint8_t* data, std::size_t length) {
        Response response;
        response.status = RfidStatus::Ok;
        response.payload.assign(data, data + length);
        responses_.push_back(std::move(response));
    }

    void queueStatus(RfidStatus status) { queue(status); }

    void setTarget(uint16_t atqa, uint8_t sak, std::initializer_list<uint8_t> uid) {
        target_.kind = RfidTargetKind::Iso14443A;
        target_.atqa = atqa;
        target_.sak = sak;
        target_.uidLength = static_cast<uint8_t>(uid.size());
        std::memset(target_.uid.data(), 0, target_.uid.size());
        std::copy(uid.begin(), uid.end(), target_.uid.begin());
    }

    // ---- knobs ------------------------------------------------------------
    RfidStatus activateResult = RfidStatus::Ok;
    bool beginResult = true;
    bool readyResult = true;
    int releaseCalls = 0;
    int activateCalls = 0;
    int transceiveCalls = 0;
    // Set when the driver sent a frame with no queued response, so a test that
    // forgot to script a reply fails loudly instead of silently.
    int unscriptedTransceives = 0;

    RfidTransportCaps capsValue = RfidTransportCaps{
        true, true, false, true, true, true, true, false, true, true, false, false, false};

    // ---- IRfidTransport ---------------------------------------------------
    bool begin() override { return beginResult; }
    void end() override {}
    bool ready() const override { return readyResult; }
    const char* name() const override { return "fake RFID transport"; }
    RfidTransportCaps caps() const override { return capsValue; }
    uint32_t firmwareVersion() override { return 0x01060000UL; }

    RfidStatus fieldOn() override { return RfidStatus::Ok; }
    RfidStatus fieldOff() override { return RfidStatus::Ok; }

    RfidStatus activateIso14443a(RfidTarget& out, uint32_t timeoutMs) override {
        (void)timeoutMs;
        ++activateCalls;
        if (activateResult != RfidStatus::Ok) {
            out = RfidTarget{};
            return activateResult;
        }
        out = target_;
        return RfidStatus::Ok;
    }

    RfidStatus releaseTarget() override {
        ++releaseCalls;
        return RfidStatus::Ok;
    }

    RfidStatus transceive(const uint8_t* tx, uint16_t txLength,
                          uint8_t* rx, uint16_t& rxLength,
                          uint32_t timeoutMs) override {
        (void)timeoutMs;
        ++transceiveCalls;
        Request request;
        request.frame.assign(tx, tx + txLength);
        requests_.push_back(std::move(request));
        if (responseIndex_ >= responses_.size()) {
            ++unscriptedTransceives;
            rxLength = 0;
            return RfidStatus::IoError;
        }
        const Response& response = responses_[responseIndex_++];
        if (response.status != RfidStatus::Ok) {
            rxLength = 0;
            return response.status;
        }
        const uint16_t available = static_cast<uint16_t>(response.payload.size());
        if (available > rxLength) {
            // The driver offered less room than the card sent: a driver bug, so
            // say so rather than truncating.
            rxLength = 0;
            return RfidStatus::FrameError;
        }
        std::memcpy(rx, response.payload.data(), available);
        rxLength = available;
        return RfidStatus::Ok;
    }

    // ---- inspection -------------------------------------------------------
    size_t requestCount() const { return requests_.size(); }
    const std::vector<uint8_t>& requestAt(size_t index) const {
        return requests_.at(index).frame;
    }
    const std::vector<uint8_t>& lastRequest() const { return requests_.back().frame; }
    void clearRequests() { requests_.clear(); }

private:
    struct Request {
        std::vector<uint8_t> frame;
    };

    std::vector<Response> responses_;
    std::vector<Request> requests_;
    size_t responseIndex_ = 0;
    RfidTarget target_{};
};
