#pragma once

// Scripted PN532 I2C link for the host suites.
//
// The transport talks to a part that answers in two steps: a 6-byte ACK for
// every accepted command, then the reply frame. The fake hands back pre-queued
// chunks one read at a time, and a virtual clock advances on delayMs() so the
// timeout paths terminate instead of spinning -- which is what makes "the part
// never answered" testable at all.

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "drivers/Pn532Transport.h"

class FakePn532Link final : public IPn532Link {
public:
    // ---- scripting --------------------------------------------------------
    // The reader's ACK for an accepted command.
    void expectAck() {
        pushChunk({0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00});
    }

    void expectReply(std::initializer_list<uint8_t> bytes) {
        pushChunk(bytes);
    }

    // A reply frame built from the parts, so a test does not have to hand
    // assemble the preamble, LEN, LCS and DCS every time.
    void expectReplyFrame(uint8_t responseCode,
                          std::initializer_list<uint8_t> payload = {}) {
        pushChunk(buildReplyFrame(responseCode, payload.begin(), payload.size()));
    }

    // Same, from a buffer: a MIFARE block read is sixteen bytes, and spelling
    // those out as an initializer list buries the point of the test.
    void expectReplyFrame(uint8_t responseCode, const uint8_t* payload,
                          std::size_t length) {
        pushChunk(buildReplyFrame(responseCode, payload, length));
    }

    void clearScript() {
        chunks_.clear();
        chunkIndex_ = 0;
    }

    // ---- knobs ------------------------------------------------------------
    bool beginResult = true;
    bool writeResult = true;
    bool readResult = true;

    // ---- IPn532Link -------------------------------------------------------
    bool begin(uint8_t address, int sda, int scl) override {
        address_ = address;
        sda_ = sda;
        scl_ = scl;
        return beginResult;
    }
    void end() override {}

    bool write(const uint8_t* data, uint16_t length) override {
        writeCount++;
        if (!writeResult) {
            return false;
        }
        lastWrite.assign(data, data + length);
        writes.push_back(lastWrite);
        return true;
    }

    bool read(uint8_t* data, uint16_t length) override {
        readCount++;
        if (!readResult) {
            return false;
        }
        std::memset(data, 0, length);
        if (chunkIndex_ >= chunks_.size()) {
            return false;
        }
        const std::vector<uint8_t>& chunk = chunks_[chunkIndex_++];
        std::memcpy(data, chunk.data(), chunk.size() < length ? chunk.size() : length);
        return true;
    }

    bool ready() override { return chunkIndex_ < chunks_.size(); }
    uint32_t millis() const override { return nowMs_; }
    void delayMs(uint32_t ms) override { nowMs_ += ms; }

    // ---- inspection -------------------------------------------------------
    int writeCount = 0;
    int readCount = 0;
    std::vector<uint8_t> lastWrite;
    std::vector<std::vector<uint8_t>> writes;
    uint8_t address_ = 0;
    int sda_ = -1;
    int scl_ = -1;

private:
    // The PN532 -> host frame shape: preamble, LEN, LCS, TFI, code, payload,
    // DCS, postamble. Shared by both expectReplyFrame overloads so the two
    // cannot drift apart.
    static std::vector<uint8_t> buildReplyFrame(uint8_t responseCode,
                                               const uint8_t* payload,
                                               std::size_t length) {
        std::vector<uint8_t> frame;
        const uint16_t len = static_cast<uint16_t>(length + 2U);
        frame.push_back(0x00);
        frame.push_back(0x00);
        frame.push_back(0xFF);
        frame.push_back(static_cast<uint8_t>(len));
        frame.push_back(static_cast<uint8_t>(~len + 1U));
        frame.push_back(0xD5);
        frame.push_back(responseCode);
        uint8_t sum = 0xD5 + responseCode;
        for (std::size_t i = 0; i < length; ++i) {
            frame.push_back(payload[i]);
            sum = static_cast<uint8_t>(sum + payload[i]);
        }
        frame.push_back(static_cast<uint8_t>(~sum + 1U));
        frame.push_back(0x00);
        return frame;
    }

    void pushChunk(std::initializer_list<uint8_t> bytes) {
        pushChunk(std::vector<uint8_t>(bytes));
    }
    void pushChunk(std::vector<uint8_t> bytes) { chunks_.push_back(std::move(bytes)); }

    std::vector<std::vector<uint8_t>> chunks_;
    size_t chunkIndex_ = 0;
    uint32_t nowMs_ = 0;
};
