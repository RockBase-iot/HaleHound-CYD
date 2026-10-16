#pragma once

// Chameleon Ultra control over BLE.
//
// The Ultra runs one command protocol over two transports. Over USB CDC and over
// its BLE link it is the same binary frame stream; the difference is that the
// BLE link tunnels it through Nordic's UART service, so a request is a write to
// RX and the reply arrives as notifications on TX. That is why this client is
// mostly the frame codec plus a session: connect, discover the UART service,
// subscribe to TX, then exchange frames.
//
// Two details are worth stating because getting them wrong is silent. The UART
// service is the 128-bit 6E400001-B5A3-F393-E0A9-E50E24DCCA9E -- not the
// 16-bit 0x0001 that Nordic's own documentation's "offset 0x0001" phrasing
// invites, which is a different service. And a reply frame can be 522 bytes
// while a default-MTU notification carries 20, so the reply is reassembled
// across notifications rather than assumed to arrive whole.
//
// Provenance for the protocol itself is in docs/protocol-evidence/chameleon-ultra.md.

#include <cstdint>
#include <string>

#include "drivers/IBleCentral.h"
#include "features/ble/ChameleonFrame.h"
#include "features/rfid/ChameleonUltraModel.h"

class ChameleonUltraClient {
public:
    explicit ChameleonUltraClient(IBleCentral& central);

    // The Nordic UART service and its two characteristics, as the firmware
    // declares them.
    static BleUuid serviceUuid();
    static BleUuid rxUuid();  // host -> device: write
    static BleUuid txUuid();  // device -> host: notify

    bool begin();
    // Drop the subscription and release the controller.
    BleStatus end();

    // ── finding and adopting a unit ────────────────────────────────────────

    // Scan and report how many connectable peers advertise the UART service.
    BleStatus scanForUnits(bool active);
    uint8_t unitCount() const { return unitCount_; }
    // The address of the n-th unit found, in the order the controller listed it.
    bool unitAddressAt(uint8_t index, BleAddress& out) const;
    std::string unitNameAt(uint8_t index) const;

    // Auto-connect: adopt the first unit found, mirroring the model's v3.9.2
    // policy where the first Chameleon seen is used instead of a pick list. A
    // unit already connected is not stolen -- that comes back Busy.
    //
    // The session is driven, not consulted: onDeviceFound() records the address
    // and a failed connect reports through onConnectFailed().
    BleStatus autoConnect(ChameleonSession& session, uint32_t timeoutMs);

    // Connect to a specific unit and set up the command link: discover, find the
    // UART service, subscribe to TX. A peer that answers but does not expose the
    // UART service is NoService, not a successful connection.
    BleStatus connect(const BleAddress& address, uint32_t timeoutMs);
    bool connected() const { return central_.connected(); }
    bool linkReady() const { return linkReady_; }

    // ── a command and its reply ────────────────────────────────────────────

    // Sends `command` and returns the reply frame that carries that command
    // number. A reply for a different command is not accepted as this one's --
    // and if the device answers with a failure status, that is returned as the
    // reply with statusOk() false rather than reported as a link error.
    BleStatus command(uint16_t command, const uint8_t* payload, uint16_t length,
                      ChameleonFrame& reply, uint32_t timeoutMs);

    // ── device identity ────────────────────────────────────────────────────

    // GET_DEVICE_ADDRESS (1012): the nRF DEVICEADDR as a 48-bit big-endian
    // value whose top two bits the firmware forces to 0b11 so it reads as a BLE
    // static address. The type is therefore reported as Random, not Unknown --
    // that is what the two bits mean.
    BleStatus readDeviceAddress(BleAddress& out, uint32_t timeoutMs);
    BleStatus readDeviceAddressText(std::string& out, uint32_t timeoutMs);
    // GET_DEVICE_MODEL (1033): 0 = Ultra, 1 = Lite.
    BleStatus readDeviceModel(uint8_t& out, uint32_t timeoutMs);
    // GET_APP_VERSION (1000): two bytes, major then minor.
    BleStatus readAppVersion(uint8_t& major, uint8_t& minor, uint32_t timeoutMs);

    // ── the eight slots ────────────────────────────────────────────────────

    // GET_SLOT_INFO (1019) plus GET_ENABLED_SLOTS (1023):
    //   GET_SLOT_INFO    - 32 bytes: eight (hf type, lf type) pairs, u16 BE each
    //   GET_ENABLED_SLOTS- 16 bytes: eight (enabled_hf, enabled_lf) pairs
    // Both are folded into the model's slot table. Nicknames are *not* fetched
    // here: each is its own command, so readSlotNickname() is driven by whoever
    // is showing the list. A slot whose kind this build cannot map is left
    // Unknown rather than given a plausible-looking label.
    BleStatus readSlots(ChameleonSlot out[kChameleonSlotCount], uint32_t timeoutMs);

    // SET_SLOT_TAG_TYPE (1004): slot | tag type, u16 big-endian. Writing a type
    // is what fills a slot; it does not switch the slot on.
    BleStatus setSlotTagType(uint8_t slot, uint16_t tagType, uint32_t timeoutMs);
    // SET_SLOT_ENABLE (1006): slot | sense | enable. This is the switch that
    // makes a loaded slot live, and it is per frequency.
    BleStatus setSlotEnabled(uint8_t slot, ChameleonSense sense, bool enabled,
                             uint32_t timeoutMs);

    // SET_SLOT_TAG_NICK (1007): slot | sense | name, name at most 32 UTF-8 bytes
    // with no terminator. A longer nickname is refused rather than truncated.
    BleStatus setSlotNickname(uint8_t slot, ChameleonSense sense, const char* nickname,
                              uint32_t timeoutMs);
    // GET_SLOT_TAG_NICK (1008). A slot with no nickname recorded answers with a
    // failure status, which is reported as BleStatus::NotFound -- an empty slot
    // is not an empty nickname.
    BleStatus readSlotNickname(uint8_t slot, ChameleonSense sense, std::string& out,
                               uint32_t timeoutMs);
    // DELETE_SLOT_TAG_NICK (1021).
    BleStatus clearSlotNickname(uint8_t slot, ChameleonSense sense, uint32_t timeoutMs);

    // SET_ACTIVE_SLOT (1003) / GET_ACTIVE_SLOT (1018).
    BleStatus setActiveSlot(uint8_t slot, uint32_t timeoutMs);
    BleStatus readActiveSlot(uint8_t& out, uint32_t timeoutMs);
    // SAVE_SETTINGS (1013): a slot edit does not survive a reboot without it.
    BleStatus saveSettings(uint32_t timeoutMs);

    // Frames the receiver decoded but nobody asked for, and frames it had to
    // reject. Both are surfaced so a silent link cannot be mistaken for a quiet
    // device.
    uint32_t strayFrames() const { return strayFrames_; }
    // LRC failures plus oversized frames the receiver refused.
    uint32_t rejectedFrames() const {
        return receiver_.lrcFailures() + receiver_.oversizedFrames();
    }

    // ── the mappings, exposed because they are where the honesty lives ─────

    // tag_base_type.h -> the model's slot kinds. Types older than the current
    // scheme are migrated first, since a slot still holding one would otherwise
    // show as Unrecognised when the firmware can name it.
    static ChameleonSlotKind slotKindFor(uint16_t hfType, uint16_t lfType);
    // The old enum values map onto current ones; anything else is unchanged.
    static uint16_t normaliseTagType(uint16_t type);

private:
    // The notify sink. Static because the transport takes a plain function
    // pointer: it is called from the stack's own task, so the client must not
    // allocate here, and all this does is feed the byte stream to the receiver.
    static void onNotify(void* context, const BleCharacteristicRef& ref,
                         const uint8_t* data, uint16_t length);

    BleCharacteristicRef rxRef() const;
    BleCharacteristicRef txRef() const;

    IBleCentral& central_;
    ChameleonFrameReceiver receiver_;
    bool linkReady_ = false;

    // The peer list from the last scan, with the bytes kept alongside the
    // printable address so the type is not lost on the way to connect().
    struct Unit {
        BleAddress address;
        std::string name;
    };
    static constexpr uint8_t kMaxUnits = 16;
    // A drain that trusts the transport to eventually report "nothing left"
    // hangs the UI the first time a transport re-serves a page.
    static constexpr uint8_t kMaxScanPages = 32;
    Unit units_[kMaxUnits];
    uint8_t unitCount_ = 0;

    // Set by the notify sink, read by command().
    ChameleonFrame lastFrame_;
    bool frameArrived_ = false;

    uint32_t strayFrames_ = 0;
};
