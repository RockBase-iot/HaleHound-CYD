#include "features/ble/ChameleonFrame.h"

#include <cstdio>
#include <cstring>

uint8_t chameleonLrc(const uint8_t* data, std::size_t length) {
    if (data == nullptr) {
        return 0;
    }
    uint32_t sum = 0;
    for (std::size_t index = 0; index < length; ++index) {
        sum += data[index];
    }
    // The two's complement of the sum, mod 256: adding the LRC back to the same
    // bytes wraps to zero, which is what every receiver in the chain relies on.
    return static_cast<uint8_t>((0x100U - (sum & 0xFFU)) & 0xFFU);
}

const char* chameleonStatusLabel(uint16_t status) {
    return chameleonStatusOk(status) ? "OK" : "fail";
}

std::size_t formatChameleonStatus(uint16_t status, char* out, std::size_t capacity) {
    if (out == nullptr || capacity == 0U) {
        return 0;
    }
    if (chameleonStatusOk(status)) {
        std::snprintf(out, capacity, "OK");
    } else {
        // The code is printed rather than named: the firmware's status table was
        // not part of the evidence that was read, so giving 0x0002 a label would
        // be a guess dressed as a fact.
        std::snprintf(out, capacity, "fail 0x%04X", status);
    }
    out[capacity - 1U] = '\0';
    return std::strlen(out);
}

bool chameleonStatusOk(uint16_t status) {
    return status == kChameleonStatusSuccess;
}

uint16_t ChameleonFrame::dataU16(uint16_t offset) const {
    if (static_cast<uint32_t>(offset) + 2U > length) {
        return 0;
    }
    return static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) |
                                 data[offset + 1]);
}

namespace {

void appendU16(uint8_t* out, std::size_t offset, uint16_t value) {
    out[offset] = static_cast<uint8_t>(value >> 8);
    out[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

}  // namespace

std::size_t encodeChameleonFrame(uint16_t command, const uint8_t* payload, uint16_t length,
                                 uint8_t* out, std::size_t capacity) {
    if (out == nullptr || length > kChameleonMaxPayload) {
        return 0;
    }
    const std::size_t total = kChameleonFrameOverhead + static_cast<std::size_t>(length);
    if (capacity < total) {
        // A short buffer means a truncated frame on the wire, which the peer
        // would reject with an LRC failure. Refuse instead of sending it.
        return 0;
    }
    out[0] = kChameleonSof;
    out[1] = kChameleonLrc1;
    appendU16(out, 2, command);
    appendU16(out, 4, kChameleonStatusSuccess);
    appendU16(out, 6, length);
    out[8] = chameleonLrc(out + 2, 6);  // over CMD|STATUS|LEN
    if (length > 0U && payload != nullptr) {
        std::memcpy(out + 9, payload, length);
    }
    out[9 + length] = chameleonLrc(out + 9, length);
    return total;
}

bool ChameleonFrameReceiver::headerValid() const {
    return chameleonLrc(header_, kHeaderBytes - 1U) == header_[kHeaderBytes - 1U];
}

ChameleonFrameReceiver::Feed ChameleonFrameReceiver::push(uint8_t byte) {
    switch (state_) {
        case State::Idle:
            // Hunting for a start-of-frame. Anything else is expected noise, not
            // an error: the link is a serial tunnel and starts mid-stream.
            if (byte == kChameleonSof) {
                state_ = State::Lrc1;
            }
            return Feed::NeedMore;

        case State::Lrc1:
            if (byte == kChameleonLrc1) {
                state_ = State::Header;
                headerLength_ = 0;
                return Feed::NeedMore;
            }
            ++resyncs_;
            // A lone 0x11 is a false start; if this byte is itself a 0x11 it may
            // be the real one, so it is re-armed rather than dropped.
            state_ = byte == kChameleonSof ? State::Lrc1 : State::Idle;
            return Feed::Resync;

        case State::Header: {
            header_[headerLength_++] = byte;
            if (headerLength_ < kHeaderBytes) {
                return Feed::NeedMore;
            }
            const uint16_t length = static_cast<uint16_t>(
                (static_cast<uint16_t>(header_[4]) << 8) | header_[5]);
            bool rejected = false;
            if (!headerValid()) {
                ++lrcFailures_;
                rejected = true;
            } else if (length > kChameleonMaxPayload) {
                // The length field itself would have us read past the buffer, so
                // it is refused before it can be trusted -- a corrupt length that
                // fits is handled by the LRC3.
                ++oversizedFrames_;
                rejected = true;
            }
            if (rejected) {
                ++resyncs_;
                if (!rearmFromPair(header_, kHeaderBytes)) {
                    state_ = State::Idle;
                }
                return Feed::Resync;
            }
            frame_.command = static_cast<uint16_t>(
                (static_cast<uint16_t>(header_[0]) << 8) | header_[1]);
            frame_.status = static_cast<uint16_t>(
                (static_cast<uint16_t>(header_[2]) << 8) | header_[3]);
            frame_.length = length;
            dataLength_ = 0;
            state_ = length == 0U ? State::Lrc3 : State::Data;
            return Feed::NeedMore;
        }

        case State::Data:
            frame_.data[dataLength_++] = byte;
            if (dataLength_ >= frame_.length) {
                state_ = State::Lrc3;
            }
            return Feed::NeedMore;

        case State::Lrc3: {
            const bool ok = chameleonLrc(frame_.data, frame_.length) == byte;
            state_ = State::Idle;
            if (!ok) {
                ++lrcFailures_;
                // The corrupt frame's payload may itself contain the start of
                // the next one.
                if (!rearmFromPair(frame_.data, frame_.length)) {
                    state_ = byte == kChameleonSof ? State::Lrc1 : State::Idle;
                }
                return Feed::Resync;
            }
            ++framesDecoded_;
            return Feed::FrameReady;
        }
    }
    return Feed::NeedMore;
}

bool ChameleonFrameReceiver::rearmFromPair(const uint8_t* buffer, std::size_t length) {
    if (buffer == nullptr) {
        return false;
    }
    // Backwards, because the last start in a rejected window is the one most
    // likely to be the real frame.
    for (std::size_t end = length; end >= 2U; --end) {
        const std::size_t start = end - 2U;
        if (buffer[start] != kChameleonSof || buffer[start + 1] != kChameleonLrc1) {
            continue;
        }
        // SOF and LRC1 are behind us; whatever follows is already header bytes.
        // memmove rather than memcpy because `buffer` may be `header_` itself.
        const std::size_t tail = length - (start + 2U);
        const std::size_t carried = tail < kHeaderBytes ? tail : kHeaderBytes;
        std::memmove(header_, buffer + start + 2U, carried);
        headerLength_ = carried;
        state_ = State::Header;
        return true;
    }
    // A trailing SOF with its LRC1 still to come is a frame start too.
    if (length > 0U && buffer[length - 1U] == kChameleonSof) {
        state_ = State::Lrc1;
        headerLength_ = 0;
        return true;
    }
    return false;
}

void ChameleonFrameReceiver::reset() {
    state_ = State::Idle;
    headerLength_ = 0;
    dataLength_ = 0;
    frame_ = ChameleonFrame{};
    framesDecoded_ = 0;
    lrcFailures_ = 0;
    resyncs_ = 0;
    oversizedFrames_ = 0;
}

std::size_t decodeChameleonFrame(const uint8_t* bytes, std::size_t length,
                                 ChameleonFrame& out) {
    if (bytes == nullptr) {
        return 0;
    }
    ChameleonFrameReceiver receiver;
    for (std::size_t index = 0; index < length; ++index) {
        if (receiver.push(bytes[index]) == ChameleonFrameReceiver::Feed::FrameReady) {
            out = receiver.frame();
            return index + 1U;
        }
    }
    return 0;
}
