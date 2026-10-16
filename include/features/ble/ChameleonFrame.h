#pragma once

// The Chameleon Ultra command frame.
//
// Provenance: RfidResearchGroup/ChameleonUltra, read from the firmware source
// rather than a blog post -- firmware/application/src/utils/netdata.h for the
// wire struct, the SOF byte and the frame layout comment,
// firmware/application/src/utils/dataframe.c for how LRC1/LRC2/LRC3 are
// computed, firmware/application/src/data_cmd.h for the command numbers,
// firmware/application/src/rfid/nfctag/tag_base_type.h for the slot tag types.
// Recorded in docs/protocol-evidence/chameleon-ultra.md.
//
//   SOF     1  0x11
//   LRC1    1  0xEF      LRC over SOF
//   CMD     2  big-endian
//   STATUS  2  big-endian; 0x0000 host -> device
//   LEN     2  big-endian, this build caps at 512
//   LRC2    1            LRC over CMD|STATUS|LEN
//   DATA    LEN
//   LRC3    1            LRC over DATA
//
// LRC is the eight-bit two's complement of the sum of the covered bytes, so
// sum(covered) + LRC wraps to zero. That also means LRC2 covers the same value
// whether it is computed from CMD or from SOF|LRC1|CMD, because LRC1 already
// nulls SOF -- which is why this decoder can validate the header without having
// kept the first two bytes.
//
// Why the receiver is fed one byte at a time: the reply arrives over the Nordic
// UART service's notify characteristic, and a notification is not a frame. With
// the default ATT MTU a notification carries 20 bytes, while a frame can be 522,
// so a single notification can hold part of a frame or a frame and a half. A
// reassembler built on "one notification = one message" works on the bench with
// short replies and breaks the first time a slot table comes back whole.

#include <cstddef>
#include <cstdint>

constexpr uint8_t kChameleonSof = 0x11;
constexpr uint8_t kChameleonLrc1 = 0xEF;
// The firmware allows 4096, but nothing this build sends or accepts is larger
// than a slot table, and a 4 KB receive buffer on the C5 is not worth the RAM.
constexpr uint16_t kChameleonMaxPayload = 512;
constexpr std::size_t kChameleonFrameOverhead = 10;
constexpr uint16_t kChameleonMaxFrameBytes =
    static_cast<uint16_t>(kChameleonMaxPayload + kChameleonFrameOverhead);

// Command numbers, verbatim from data_cmd.h. Only the ones this build issues or
// has to recognise are named; the full map is 1000..6005.
enum class ChameleonCommand : uint16_t {
    GetAppVersion = 1000,
    ChangeDeviceMode = 1001,
    GetDeviceMode = 1002,
    SetActiveSlot = 1003,
    SetSlotTagType = 1004,
    SetSlotEnable = 1006,
    SetSlotTagNick = 1007,
    GetSlotTagNick = 1008,
    SaveSettings = 1013,
    GetDeviceChipId = 1011,
    GetDeviceAddress = 1012,
    GetGitVersion = 1017,
    GetActiveSlot = 1018,
    GetSlotInfo = 1019,
    // data_cmd.h calls 1020 DATA_CMD_WIPE_FDS -- it erases the device's flash
    // data storage. Named as upstream names it, because "Write" would say the
    // opposite of what the command does, and this enum is the only place a
    // caller reads the meaning from.
    WipeFds = 1020,
    DeleteSlotTagNick = 1021,
    GetEnabledSlots = 1023,
    GetBatteryInfo = 1025,
    GetDeviceModel = 1033,
};

// The slot "sense" selector that several commands take as their second byte.
enum class ChameleonSense : uint8_t {
    None = 0,
    Lf = 1,
    Hf = 2,
};

// Success is the one status this build can name. The firmware's full status
// table was not in the evidence set that was read, so a failure is reported by
// its raw code rather than given a plausible-sounding label -- the same rule the
// rest of this repository uses for unverifiable protocol details.
constexpr uint16_t kChameleonStatusSuccess = 0x0000;

// "OK" for success, "fail" otherwise. Never nullptr, never a static buffer --
// the panel formats several of these per frame and a shared buffer would let one
// line overwrite another's text.
const char* chameleonStatusLabel(uint16_t status);
// "OK", or "fail 0xNNNN" with the raw code, since the firmware's status table is
// not in the evidence set. Returns the characters written, excluding the
// terminator; writes "" when the buffer is unusable.
std::size_t formatChameleonStatus(uint16_t status, char* out, std::size_t capacity);
bool chameleonStatusOk(uint16_t status);

// The LRC over `length` bytes. Exposed because it is the one arithmetic rule the
// whole format rests on, and a test pins it directly.
uint8_t chameleonLrc(const uint8_t* data, std::size_t length);

struct ChameleonFrame {
    uint16_t command = 0;
    uint16_t status = 0;
    uint8_t data[kChameleonMaxPayload] = {};
    uint16_t length = 0;

    bool statusOk() const { return chameleonStatusOk(status); }
    // Big-endian u16 read out of `data`; `offset + 2` must be within `length`.
    uint16_t dataU16(uint16_t offset) const;
};

// Encodes a host -> device frame: status is always 0x0000 on this side, as the
// format requires. Returns the number of bytes written, or 0 when the payload
// exceeds kChameleonMaxPayload or the buffer is too small for the whole frame
// (a truncated frame must never be sent).
std::size_t encodeChameleonFrame(uint16_t command, const uint8_t* payload, uint16_t length,
                                 uint8_t* out, std::size_t capacity);

// Decodes one whole frame from the front of `bytes`. Returns the bytes consumed,
// or 0 when the buffer does not begin with a complete, LRC-valid frame.
std::size_t decodeChameleonFrame(const uint8_t* bytes, std::size_t length,
                                 ChameleonFrame& out);

// Incremental receiver, for a byte stream that has no message boundaries.
class ChameleonFrameReceiver {
public:
    enum class Feed : uint8_t {
        NeedMore,   // the byte was consumed, the frame is not complete yet
        FrameReady, // frame() is valid now
        Resync,     // a frame start was rejected; the stream is hunting again
    };

    Feed push(uint8_t byte);

    // Valid only after push() returned FrameReady.
    const ChameleonFrame& frame() const { return frame_; }

    // Counters, so a test can tell "no reply yet" from "the peer is talking
    // nonsense": a resync or an LRC failure is a fact about the link, not a
    // silent drop.
    uint32_t framesDecoded() const { return framesDecoded_; }
    uint32_t lrcFailures() const { return lrcFailures_; }
    uint32_t resyncs() const { return resyncs_; }
    uint32_t oversizedFrames() const { return oversizedFrames_; }

    void reset();

private:
    enum class State : uint8_t { Idle, Lrc1, Header, Data, Lrc3 };

    static constexpr std::size_t kHeaderBytes = 7;  // cmd 2 + status 2 + len 2 + lrc2 1

    bool headerValid() const;
    // Looks for a frame start inside what was just rejected. A false start can
    // hide a real one -- the link comes up mid-stream, so the first bytes after
    // connecting are the tail of somebody else's frame -- and dropping the whole
    // window would cost the next real frame as well. Returns true when the
    // receiver was re-armed.
    bool rearmFromPair(const uint8_t* buffer, std::size_t length);

    State state_ = State::Idle;
    uint8_t header_[kHeaderBytes] = {};
    std::size_t headerLength_ = 0;
    uint16_t dataLength_ = 0;
    ChameleonFrame frame_;
    uint32_t framesDecoded_ = 0;
    uint32_t lrcFailures_ = 0;
    uint32_t resyncs_ = 0;
    uint32_t oversizedFrames_ = 0;
};
