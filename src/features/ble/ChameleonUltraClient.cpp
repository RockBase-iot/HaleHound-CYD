#include "features/ble/ChameleonUltraClient.h"

#include <cstring>

namespace {

// The Nordic UART service and its two characteristics, as the Ultra's firmware
// declares them. Spelled as 16 bytes rather than derived from a short offset:
// Nordic's documentation calls this "16-bit offset 0x0001", and 0x0001 is a
// different service entirely.
constexpr uint8_t kNusService[16] = {0x6E, 0x40, 0x00, 0x01, 0xB5, 0xA3, 0xF3, 0x93,
                                     0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E};
constexpr uint8_t kNusRx[16] = {0x6E, 0x40, 0x00, 0x02, 0xB5, 0xA3, 0xF3, 0x93,
                                0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E};
constexpr uint8_t kNusTx[16] = {0x6E, 0x40, 0x00, 0x03, 0xB5, 0xA3, 0xF3, 0x93,
                                0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E};

// The wire sizes the firmware documents for each reply. A reply of the wrong
// size is not a short answer to be used anyway -- these are fixed-layout
// structures, so the wrong length means the reply is not what was asked for.
constexpr uint16_t kSlotInfoBytes = 32;     // eight (hf, lf) u16 pairs
constexpr uint16_t kEnabledSlotsBytes = 16;  // eight (hf, lf) bool pairs
constexpr uint16_t kDeviceAddressBytes = 6;  // nRF DEVICEADDR, U48
constexpr uint16_t kAppVersionBytes = 2;     // major, minor

constexpr std::size_t kMaxNicknameBytes = 32;

}  // namespace

BleUuid ChameleonUltraClient::serviceUuid() {
    return BleUuid::fromBytes(kNusService);
}

BleUuid ChameleonUltraClient::rxUuid() {
    return BleUuid::fromBytes(kNusRx);
}

BleUuid ChameleonUltraClient::txUuid() {
    return BleUuid::fromBytes(kNusTx);
}

ChameleonUltraClient::ChameleonUltraClient(IBleCentral& central) : central_(central) {}

BleCharacteristicRef ChameleonUltraClient::rxRef() const {
    BleCharacteristicRef ref;
    ref.service = serviceUuid();
    ref.characteristic = rxUuid();
    return ref;
}

BleCharacteristicRef ChameleonUltraClient::txRef() const {
    BleCharacteristicRef ref;
    ref.service = serviceUuid();
    ref.characteristic = txUuid();
    return ref;
}

bool ChameleonUltraClient::begin() {
    unitCount_ = 0;
    linkReady_ = false;
    receiver_.reset();
    strayFrames_ = 0;
    return central_.begin();
}

BleStatus ChameleonUltraClient::end() {
    if (central_.scanning()) {
        central_.stopScan();
    }
    if (linkReady_) {
        // Unsubscribe before dropping the link: a subscription left on a dead
        // handle is the leak the ledger's white-screen model is about.
        central_.unsubscribe(txRef());
        linkReady_ = false;
    }
    central_.end();
    unitCount_ = 0;
    receiver_.reset();
    return BleStatus::Ok;
}

// ── the notify sink ────────────────────────────────────────────────────────

void ChameleonUltraClient::onNotify(void* context, const BleCharacteristicRef& ref,
                                    const uint8_t* data, uint16_t length) {
    (void)ref;
    if (context == nullptr || data == nullptr) {
        return;
    }
    ChameleonUltraClient* self = static_cast<ChameleonUltraClient*>(context);
    // One notification is not one frame. Feeding the receiver byte by byte is
    // what makes a reply longer than the MTU assemble, and what makes two frames
    // packed into one notification both arrive.
    for (uint16_t index = 0; index < length; ++index) {
        const ChameleonFrameReceiver::Feed feed = self->receiver_.push(data[index]);
        if (feed == ChameleonFrameReceiver::Feed::FrameReady) {
            self->lastFrame_ = self->receiver_.frame();
            self->frameArrived_ = true;
        }
    }
}

// ── finding and adopting a unit ────────────────────────────────────────────

BleStatus ChameleonUltraClient::scanForUnits(bool active) {
    unitCount_ = 0;
    const BleStatus started = central_.startScan(0, active);
    if (started != BleStatus::Ok) {
        return started;
    }
    const BleUuid expected = serviceUuid();
    BleAdvertisement batch[kBleMaxAdvServices];
    // Bounded, for the same reason the SkeletonKey drain is bounded: a transport
    // that re-serves a page must not be able to spin this loop.
    for (uint8_t page = 0; page < kMaxScanPages; ++page) {
        const uint8_t count = central_.scanResults(batch, kBleMaxAdvServices);
        if (count == 0U) {
            break;
        }
        for (uint8_t index = 0; index < count; ++index) {
            const BleAdvertisement& advertisement = batch[index];
            if (!advertisement.connectable || !advertisement.address.valid()) {
                continue;
            }
            // Identification is by advertised service, not by name: a name is
            // whatever the owner set, and matching on one would connect to
            // anything that happens to share it.
            if (!advertisement.advertises(expected)) {
                continue;
            }
            bool seen = false;
            for (uint8_t unit = 0; unit < unitCount_; ++unit) {
                if (units_[unit].address == advertisement.address) {
                    seen = true;
                    break;
                }
            }
            if (seen || unitCount_ >= kMaxUnits) {
                continue;
            }
            units_[unitCount_].address = advertisement.address;
            units_[unitCount_].name = advertisement.name;
            ++unitCount_;
        }
        if (unitCount_ >= kMaxUnits) {
            break;
        }
    }
    central_.stopScan();
    return BleStatus::Ok;
}

bool ChameleonUltraClient::unitAddressAt(uint8_t index, BleAddress& out) const {
    if (index >= unitCount_) {
        return false;
    }
    out = units_[index].address;
    return true;
}

std::string ChameleonUltraClient::unitNameAt(uint8_t index) const {
    return index < unitCount_ ? units_[index].name : std::string();
}

BleStatus ChameleonUltraClient::connect(const BleAddress& address, uint32_t timeoutMs) {
    const BleStatus connected = central_.connect(address, timeoutMs);
    if (connected != BleStatus::Ok) {
        return connected;
    }
    BleDiscoveredPeer peer;
    const BleStatus discovered = central_.discover(peer);
    if (discovered != BleStatus::Ok) {
        central_.disconnect();
        return discovered;
    }
    const BleUuid expected = serviceUuid();
    bool hasUart = false;
    for (uint8_t index = 0; index < peer.serviceCount; ++index) {
        if (peer.services[index].uuid == expected) {
            hasUart = true;
            break;
        }
    }
    if (!hasUart) {
        // Connected to something that is not speaking the command protocol.
        // Leaving the link up would report success and then time out on the
        // first command.
        central_.disconnect();
        return BleStatus::NoService;
    }
    receiver_.reset();
    const BleStatus subscribed =
        central_.subscribe(txRef(), &ChameleonUltraClient::onNotify, this);
    if (subscribed != BleStatus::Ok) {
        central_.disconnect();
        return subscribed;
    }
    linkReady_ = true;
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::autoConnect(ChameleonSession& session, uint32_t timeoutMs) {
    if (connected()) {
        return BleStatus::Busy;
    }
    // The model's own policy: a unit already held is not replaced.
    if (session.state() == ChameleonState::Ready || session.state() == ChameleonState::Busy) {
        return BleStatus::Busy;
    }
    const BleStatus scanned = scanForUnits(true);
    if (scanned != BleStatus::Ok) {
        return scanned;
    }
    for (uint8_t index = 0; index < unitCount_; ++index) {
        char text[18] = {};
        if (!units_[index].address.write(text, sizeof(text))) {
            continue;
        }
        if (!session.onDeviceFound(text)) {
            // The policy refused to adopt it (already holding one, or no
            // address). That is a decision, not a failure to try the next.
            continue;
        }
        const BleStatus status = connect(units_[index].address, timeoutMs);
        if (status == BleStatus::Ok) {
            return BleStatus::Ok;
        }
        session.onConnectFailed(bleStatusLabel(status));
        // One unit refusing does not end the sweep; the next is tried.
    }
    return BleStatus::NotFound;
}

// ── a command and its reply ────────────────────────────────────────────────

BleStatus ChameleonUltraClient::command(uint16_t command, const uint8_t* payload,
                                        uint16_t length, ChameleonFrame& reply,
                                        uint32_t timeoutMs) {
    (void)timeoutMs;
    if (!central_.connected() || !linkReady_) {
        return BleStatus::Disconnected;
    }
    uint8_t wire[kChameleonMaxFrameBytes];
    const std::size_t written = encodeChameleonFrame(command, payload, length, wire,
                                                     sizeof(wire));
    if (written == 0U) {
        // The payload does not fit a frame, or the frame does not fit the wire
        // buffer. Sending a truncated frame would produce an LRC failure on the
        // peer, which reads as a link problem; refuse here instead.
        return length > kChameleonMaxPayload ? BleStatus::TooLong : BleStatus::IoError;
    }
    frameArrived_ = false;
    const BleStatus sent =
        central_.write(rxRef(), wire, static_cast<uint16_t>(written), false);
    if (sent != BleStatus::Ok) {
        return sent;
    }
    if (!frameArrived_) {
        return BleStatus::Timeout;
    }
    if (lastFrame_.command != command) {
        // A reply to a different command is not this command's answer. Counted,
        // then reported as no answer rather than handed back as one.
        ++strayFrames_;
        return BleStatus::Timeout;
    }
    reply = lastFrame_;
    return BleStatus::Ok;
}

// ── device identity ────────────────────────────────────────────────────────

BleStatus ChameleonUltraClient::readDeviceAddress(BleAddress& out, uint32_t timeoutMs) {
    ChameleonFrame reply;
    const BleStatus status =
        command(static_cast<uint16_t>(ChameleonCommand::GetDeviceAddress), nullptr, 0, reply,
                timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!reply.statusOk()) {
        return BleStatus::IoError;
    }
    if (reply.length != kDeviceAddressBytes) {
        return BleStatus::IoError;
    }
    std::memcpy(out.bytes.data(), reply.data, kDeviceAddressBytes);
    // The firmware forces the top two bits to 0b11, which is what makes this a
    // BLE static random address rather than an untyped 48-bit number. Reporting
    // it as Unknown would lose that, and the type is part of the identity.
    out.type = BleAddressType::Random;
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::readDeviceAddressText(std::string& out, uint32_t timeoutMs) {
    out.clear();
    BleAddress address;
    const BleStatus status = readDeviceAddress(address, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    char text[18] = {};
    if (!address.write(text, sizeof(text))) {
        return BleStatus::IoError;
    }
    out = text;
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::readDeviceModel(uint8_t& out, uint32_t timeoutMs) {
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::GetDeviceModel),
                                     nullptr, 0, reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!reply.statusOk() || reply.length != 1U) {
        return BleStatus::IoError;
    }
    out = reply.data[0];
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::readAppVersion(uint8_t& major, uint8_t& minor,
                                               uint32_t timeoutMs) {
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::GetAppVersion),
                                     nullptr, 0, reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!reply.statusOk() || reply.length != kAppVersionBytes) {
        return BleStatus::IoError;
    }
    major = reply.data[0];
    minor = reply.data[1];
    return BleStatus::Ok;
}

// ── the eight slots ────────────────────────────────────────────────────────

uint16_t ChameleonUltraClient::normaliseTagType(uint16_t type) {
    // tag_base_type.h: the low values 1..8 are the pre-rename enum, and a slot
    // still holding one needs migrating before it can be read. Without this a
    // slot with a perfectly nameable card would show as Unrecognised.
    switch (type) {
        case 1:  // OLD_TAG_TYPE_EM410X
            return 100;
        case 2:  // OLD_TAG_TYPE_MIFARE_Mini
            return 1000;
        case 3:  // OLD_TAG_TYPE_MIFARE_1024
            return 1001;
        case 4:  // OLD_TAG_TYPE_MIFARE_2048
            return 1002;
        case 5:  // OLD_TAG_TYPE_MIFARE_4096
            return 1003;
        case 6:  // OLD_TAG_TYPE_NTAG_213
            return 1100;
        case 7:  // OLD_TAG_TYPE_NTAG_215
            return 1101;
        case 8:  // OLD_TAG_TYPE_NTAG_216
            return 1102;
        default:
            return type;
    }
}

ChameleonSlotKind ChameleonUltraClient::slotKindFor(uint16_t hfType, uint16_t lfType) {
    const uint16_t hf = normaliseTagType(hfType);
    const uint16_t lf = normaliseTagType(lfType);

    switch (hf) {
        case 1001:  // TAG_TYPE_MIFARE_1024
            return ChameleonSlotKind::MifareClassic1K;
        case 1003:  // TAG_TYPE_MIFARE_4096
            return ChameleonSlotKind::MifareClassic4K;
        case 1100:  // NTAG 213
        case 1101:  // NTAG 215
        case 1102:  // NTAG 216
        case 1107:  // NTAG 210
        case 1108:  // NTAG 212
            return ChameleonSlotKind::Ntag21x;
        case 1103:  // MF0ICU1
        case 1104:  // MF0ICU2
        case 1105:  // MF0UL11
        case 1106:  // MF0UL21
            return ChameleonSlotKind::MifareUltralight;
        default:
            break;
    }
    // The HF side is either undefined or a type this build does not name, so fall
    // through to whatever the LF side says.
    switch (lf) {
        case 100:  // TAG_TYPE_EM410X
        case 101:
        case 102:
        case 103:
        case 104:
            return ChameleonSlotKind::Em4100;
        case 200:  // TAG_TYPE_HID_PROX
            return ChameleonSlotKind::HidProx;
        default:
            break;
    }
    if (hf != 0U || lf != 0U) {
        // The device says there is something here and this build cannot say what.
        // "Unrecognised" is the honest answer; the alternatives are a wrong name
        // or an empty slot that is not empty.
        return ChameleonSlotKind::Unknown;
    }
    return ChameleonSlotKind::Empty;
}

BleStatus ChameleonUltraClient::readSlots(ChameleonSlot out[kChameleonSlotCount],
                                         uint32_t timeoutMs) {
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        out[index] = ChameleonSlot{};
        out[index].index = index;
        out[index].kind = ChameleonSlotKind::Empty;
    }

    ChameleonFrame info;
    BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::GetSlotInfo), nullptr,
                               0, info, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!info.statusOk() || info.length != kSlotInfoBytes) {
        // Fixed-layout answer; the wrong size means it is not a slot table.
        return BleStatus::IoError;
    }
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        const uint16_t hf = info.dataU16(static_cast<uint16_t>(index * 4U));
        const uint16_t lf = info.dataU16(static_cast<uint16_t>(index * 4U + 2U));
        out[index].kind = slotKindFor(hf, lf);
        out[index].loaded = hf != 0U || lf != 0U;
    }

    ChameleonFrame enabled;
    status = command(static_cast<uint16_t>(ChameleonCommand::GetEnabledSlots), nullptr, 0,
                     enabled, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!enabled.statusOk() || enabled.length != kEnabledSlotsBytes) {
        return BleStatus::IoError;
    }
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        const uint8_t hfEnabled = enabled.data[index * 2U];
        const uint8_t lfEnabled = enabled.data[index * 2U + 1U];
        out[index].enabled = hfEnabled != 0U || lfEnabled != 0U;
    }
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::setSlotTagType(uint8_t slot, uint16_t tagType,
                                               uint32_t timeoutMs) {
    if (slot >= kChameleonSlotCount) {
        return BleStatus::NotFound;
    }
    const uint8_t payload[3] = {slot, static_cast<uint8_t>(tagType >> 8),
                                static_cast<uint8_t>(tagType & 0xFF)};
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::SetSlotTagType),
                                     payload, sizeof(payload), reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}

BleStatus ChameleonUltraClient::setSlotEnabled(uint8_t slot, ChameleonSense sense,
                                               bool enabled, uint32_t timeoutMs) {
    if (slot >= kChameleonSlotCount || sense == ChameleonSense::None) {
        return BleStatus::NotFound;
    }
    const uint8_t payload[3] = {slot, static_cast<uint8_t>(sense),
                                static_cast<uint8_t>(enabled ? 0x01 : 0x00)};
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::SetSlotEnable),
                                     payload, sizeof(payload), reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}

BleStatus ChameleonUltraClient::setSlotNickname(uint8_t slot, ChameleonSense sense,
                                                const char* nickname, uint32_t timeoutMs) {
    if (slot >= kChameleonSlotCount || sense == ChameleonSense::None) {
        return BleStatus::NotFound;
    }
    const std::size_t length = nickname == nullptr ? 0U : std::strlen(nickname);
    if (length > kMaxNicknameBytes) {
        // The firmware field is 32 bytes with no terminator, so a longer name
        // would be cut by whoever is closest to the wire -- refuse instead, so
        // the caller is not shown a nickname the device never stored.
        return BleStatus::TooLong;
    }
    uint8_t payload[2U + kMaxNicknameBytes] = {};
    payload[0] = slot;
    payload[1] = static_cast<uint8_t>(sense);
    if (length > 0U) {
        std::memcpy(payload + 2, nickname, length);
    }
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::SetSlotTagNick),
                                     payload, static_cast<uint16_t>(2U + length), reply,
                                     timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}

BleStatus ChameleonUltraClient::readSlotNickname(uint8_t slot, ChameleonSense sense,
                                                 std::string& out, uint32_t timeoutMs) {
    out.clear();
    if (slot >= kChameleonSlotCount || sense == ChameleonSense::None) {
        return BleStatus::NotFound;
    }
    const uint8_t payload[2] = {slot, static_cast<uint8_t>(sense)};
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::GetSlotTagNick),
                                     payload, sizeof(payload), reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!reply.statusOk()) {
        // The firmware's documented answer for "nothing recorded in flash" is a
        // failure status. An empty slot is not an empty nickname, so this is
        // reported as not-found rather than as a successful empty string.
        return BleStatus::NotFound;
    }
    if (reply.length > kMaxNicknameBytes) {
        return BleStatus::IoError;
    }
    out.assign(reinterpret_cast<const char*>(reply.data), reply.length);
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::clearSlotNickname(uint8_t slot, ChameleonSense sense,
                                                  uint32_t timeoutMs) {
    if (slot >= kChameleonSlotCount || sense == ChameleonSense::None) {
        return BleStatus::NotFound;
    }
    const uint8_t payload[2] = {slot, static_cast<uint8_t>(sense)};
    ChameleonFrame reply;
    const BleStatus status =
        command(static_cast<uint16_t>(ChameleonCommand::DeleteSlotTagNick), payload,
                sizeof(payload), reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}

BleStatus ChameleonUltraClient::setActiveSlot(uint8_t slot, uint32_t timeoutMs) {
    if (slot >= kChameleonSlotCount) {
        return BleStatus::NotFound;
    }
    const uint8_t payload[1] = {slot};
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::SetActiveSlot),
                                     payload, sizeof(payload), reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}

BleStatus ChameleonUltraClient::readActiveSlot(uint8_t& out, uint32_t timeoutMs) {
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::GetActiveSlot),
                                     nullptr, 0, reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    if (!reply.statusOk() || reply.length != 1U) {
        return BleStatus::IoError;
    }
    if (reply.data[0] >= kChameleonSlotCount) {
        // The protocol counts slots 0..7; anything else is not a slot index, and
        // returning it would let a caller index a table with it.
        return BleStatus::IoError;
    }
    out = reply.data[0];
    return BleStatus::Ok;
}

BleStatus ChameleonUltraClient::saveSettings(uint32_t timeoutMs) {
    ChameleonFrame reply;
    const BleStatus status = command(static_cast<uint16_t>(ChameleonCommand::SaveSettings),
                                     nullptr, 0, reply, timeoutMs);
    if (status != BleStatus::Ok) {
        return status;
    }
    return reply.statusOk() ? BleStatus::Ok : BleStatus::NotPermitted;
}
