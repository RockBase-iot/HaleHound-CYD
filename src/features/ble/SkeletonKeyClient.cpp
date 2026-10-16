#include "features/ble/SkeletonKeyClient.h"

#include <cstring>

namespace {

// The largest value this client will hold for one characteristic read. 512 is
// the Chameleon frame ceiling and comfortably above any GATT value a BLE peer
// will hand back, so a TooLong here means the peer is not speaking GATT.
constexpr uint16_t kMaxValueBytes = 512;

void appendHex(std::string& out, const uint8_t* data, uint16_t length) {
    static const char kDigits[] = "0123456789ABCDEF";
    out.reserve(out.size() + static_cast<std::size_t>(length) * 2U);
    for (uint16_t index = 0; index < length; ++index) {
        out.push_back(kDigits[data[index] >> 4]);
        out.push_back(kDigits[data[index] & 0x0FU]);
    }
}

}  // namespace

std::string SkeletonKeyClient::uuidText(const BleUuid& uuid) {
    char text[37] = {};
    if (!uuid.write(text, sizeof(text))) {
        return std::string();
    }
    return std::string(text);
}

SkeletonDevice SkeletonKeyClient::toDevice(const BleAdvertisement& advertisement) {
    SkeletonDevice device;
    char address[18] = {};
    if (advertisement.address.write(address, sizeof(address))) {
        device.address = address;
    }
    // Empty stays empty: a device that did not advertise a name is not
    // "Unknown", it is unnamed, and the panel prints the address instead.
    device.name = advertisement.name;
    device.rssi = advertisement.rssi;
    device.appearance = advertisement.appearance;
    device.kind = SkeletonKey::classifyAppearance(advertisement.appearance);
    return device;
}

void SkeletonKeyClient::buildTree(const BleDiscoveredPeer& peer, SkeletonTree& out) {
    out = SkeletonTree{};
    // The peer's own flag plus every per-service flag: a service that filled its
    // characteristic array is a partial service, and a partial service makes the
    // whole tree partial.
    out.truncated = peer.truncated;
    for (uint8_t serviceIndex = 0; serviceIndex < peer.serviceCount; ++serviceIndex) {
        const BleDiscoveredService& discovered = peer.services[serviceIndex];
        SkeletonService service;
        service.uuid = uuidText(discovered.uuid);
        if (discovered.truncated) {
            out.truncated = true;
        }
        for (uint8_t charIndex = 0; charIndex < discovered.characteristicCount; ++charIndex) {
            const BleDiscoveredCharacteristic& discoveredChar =
                discovered.characteristics[charIndex];
            SkeletonCharacteristic characteristic;
            characteristic.uuid = uuidText(discoveredChar.uuid);
            characteristic.properties = discoveredChar.properties;
            service.characteristics.push_back(characteristic);
        }
        out.services.push_back(service);
    }
    // Resolve names here rather than at render time, so the screen and the SD
    // dump cannot disagree about what a UUID was called.
    SkeletonKey::applyNames(out);
}

bool SkeletonKeyClient::begin() {
    return central_.begin();
}

bool SkeletonKeyClient::beginSession(bool userConfirmed) {
    devices_.clear();
    addresses_.clear();
    tree_ = SkeletonTree{};
    return session_.begin(userConfirmed);
}

BleStatus SkeletonKeyClient::beginScan(bool active) {
    // 0 means "until stopScan()": the UI drives the duration.
    const BleStatus status = central_.startScan(0, active);
    if (status == BleStatus::Ok) {
        session_.onScanStarted();
    }
    return status;
}

bool SkeletonKeyClient::addressAt(uint16_t index, BleAddress& out) const {
    if (index >= addresses_.size()) {
        return false;
    }
    out = addresses_[index];
    return true;
}

uint8_t SkeletonKeyClient::drainScanResults() {
    BleAdvertisement batch[kBleMaxAdvServices];
    uint8_t added = 0;
    // The drain ends when the transport has nothing left, when the list is full,
    // or after this many pages -- whichever comes first. The page bound is not
    // decoration: a transport that re-serves the same page would otherwise spin
    // this loop for ever, which is a hung UI and, on a battery device, a flat
    // battery.
    for (uint8_t page = 0; page < kMaxScanPages; ++page) {
        const uint8_t count = central_.scanResults(batch, kBleMaxAdvServices);
        if (count == 0U) {
            break;
        }
        for (uint8_t index = 0; index < count; ++index) {
            const BleAdvertisement& advertisement = batch[index];
            // A non-connectable beacon has nothing to offer a GATT explorer:
            // offering it would only lead to a connect that times out.
            if (!advertisement.connectable || !advertisement.address.valid()) {
                continue;
            }
            // One row per peer. A repeat advertisement with a stronger signal
            // updates the row in place rather than appearing twice.
            bool merged = false;
            for (std::size_t existing = 0; existing < addresses_.size(); ++existing) {
                if (addresses_[existing] == advertisement.address) {
                    if (advertisement.rssi > devices_[existing].rssi) {
                        devices_[existing].rssi = advertisement.rssi;
                    }
                    merged = true;
                    break;
                }
            }
            if (merged || devices_.size() >= kBleMaxScanList) {
                continue;
            }
            devices_.push_back(toDevice(advertisement));
            addresses_.push_back(advertisement.address);
            ++added;
        }
        if (devices_.size() >= kBleMaxScanList) {
            break;
        }
    }
    return added;
}

BleStatus SkeletonKeyClient::endScan() {
    return central_.stopScan();
}

BleStatus SkeletonKeyClient::scan(bool active) {
    const BleStatus started = beginScan(active);
    if (started != BleStatus::Ok) {
        return started;
    }
    drainScanResults();
    const BleStatus stopped = endScan();
    if (stopped != BleStatus::Ok) {
        return stopped;
    }
    session_.onScanComplete(deviceCount());
    return BleStatus::Ok;
}

BleStatus SkeletonKeyClient::connect(const BleAddress& address, uint32_t timeoutMs) {
    session_.onConnectStarted();
    const BleStatus status = central_.connect(address, timeoutMs);
    if (status != BleStatus::Ok) {
        session_.onConnectFailed(bleStatusLabel(status));
    }
    return status;
}

BleStatus SkeletonKeyClient::connectToStrongest(uint32_t timeoutMs) {
    if (devices_.empty()) {
        return BleStatus::NotFound;
    }
    std::size_t best = 0;
    for (std::size_t index = 1; index < devices_.size(); ++index) {
        if (devices_[index].rssi > devices_[best].rssi) {
            best = index;
        }
    }
    return connect(addresses_[best], timeoutMs);
}

BleStatus SkeletonKeyClient::discover(SkeletonTree& out) {
    if (!central_.connected()) {
        session_.onDiscoverFailed("not connected");
        return BleStatus::Disconnected;
    }
    BleDiscoveredPeer peer;
    const BleStatus status = central_.discover(peer);
    if (status != BleStatus::Ok) {
        session_.onDiscoverFailed(bleStatusLabel(status));
        return status;
    }
    payloadLimit_ = peer.payloadLimit();

    SkeletonTree tree;
    buildTree(peer, tree);
    session_.onDiscoverComplete(tree);
    if (session_.state() != SkeletonState::Ready) {
        // Services came back empty: that is a failed discovery, not a tree with
        // no rows to render.
        return BleStatus::NoService;
    }
    tree_ = tree;
    out = tree;
    return BleStatus::Ok;
}

BleStatus SkeletonKeyClient::read(const BleCharacteristicRef& ref, std::string& hexOut,
                                  uint32_t timeoutMs) {
    hexOut.clear();
    uint8_t buffer[kMaxValueBytes];
    uint16_t capacity = payloadLimit_;
    if (capacity == 0U || capacity > kMaxValueBytes) {
        capacity = kMaxValueBytes;
    }
    uint16_t length = capacity;
    BleStatus status = central_.read(ref, buffer, length, timeoutMs);
    if (status == BleStatus::TooLong) {
        // `length` now carries the size the peer needed. Retry once at that
        // size rather than reporting a short read as the value.
        if (length == 0U || length > kMaxValueBytes) {
            return status;
        }
        uint16_t retryLength = length;
        status = central_.read(ref, buffer, retryLength, timeoutMs);
        if (status != BleStatus::Ok) {
            return status;
        }
        length = retryLength;
    }
    if (status != BleStatus::Ok) {
        return status;
    }
    appendHex(hexOut, buffer, length);
    return BleStatus::Ok;
}

BleStatus SkeletonKeyClient::readIntoTree(SkeletonTree& tree, uint16_t serviceIndex,
                                          uint16_t characteristicIndex,
                                          uint32_t timeoutMs) {
    if (serviceIndex >= tree.services.size() ||
        characteristicIndex >= tree.services[serviceIndex].characteristics.size()) {
        return BleStatus::NotFound;
    }
    SkeletonCharacteristic& characteristic =
        tree.services[serviceIndex].characteristics[characteristicIndex];

    BleCharacteristicRef ref;
    if (!BleUuid::fromText(characteristic.uuid.c_str(), ref.characteristic) ||
        !BleUuid::fromText(tree.services[serviceIndex].uuid.c_str(), ref.service)) {
        // The tree entry is not addressable, so there is nothing to read. This
        // is a refusal, not an empty value.
        return BleStatus::NoService;
    }
    std::string hex;
    const BleStatus status = read(ref, hex, timeoutMs);
    if (status == BleStatus::Ok) {
        characteristic.lastValue = hex;
    }
    return status;
}

BleStatus SkeletonKeyClient::write(const BleCharacteristicRef& ref, const uint8_t* data,
                                   uint16_t length, bool withResponse) {
    return central_.write(ref, data, length, withResponse);
}

BleStatus SkeletonKeyClient::writeToTree(const SkeletonTree& tree, uint16_t serviceIndex,
                                        uint16_t characteristicIndex, const uint8_t* data,
                                        uint16_t length, bool withResponse) {
    if (serviceIndex >= tree.services.size() ||
        characteristicIndex >= tree.services[serviceIndex].characteristics.size()) {
        return BleStatus::NotFound;
    }
    BleCharacteristicRef ref;
    if (!BleUuid::fromText(
            tree.services[serviceIndex].characteristics[characteristicIndex].uuid.c_str(),
            ref.characteristic) ||
        !BleUuid::fromText(tree.services[serviceIndex].uuid.c_str(), ref.service)) {
        return BleStatus::NoService;
    }
    return write(ref, data, length, withResponse);
}

BleStatus SkeletonKeyClient::subscribe(const BleCharacteristicRef& ref,
                                       BleNotifyFn callback, void* context) {
    return central_.subscribe(ref, callback, context);
}

BleStatus SkeletonKeyClient::unsubscribe(const BleCharacteristicRef& ref) {
    return central_.unsubscribe(ref);
}

BleStatus SkeletonKeyClient::end() {
    // Stop the scan before dropping the controller: a live scan that outlives
    // its session is one of the two white-screen causes the ledger models.
    if (central_.scanning()) {
        central_.stopScan();
    }
    central_.end();
    session_.stop();
    // The peer list is session state. Leaving it behind would let the next visit
    // connect to a peer that is no longer there.
    devices_.clear();
    addresses_.clear();
    tree_ = SkeletonTree{};
    payloadLimit_ = 20;
    return BleStatus::Ok;
}
