#pragma once

// Scripted BLE central for the host suites.
//
// Two jobs. First, script the peer's side of a session: which advertisements a
// scan returns, whether a connect succeeds, what discovery finds, and what a
// read replies with -- so a client suite reads as the peripheral's behaviour
// rather than as mock bookkeeping.
//
// Second, and more importantly, *account for resources*. `connected()`,
// `scanning()` and `subscriptionCount()` are the same accessors the real
// transport exposes, which is what lets a test assert that a session really did
// hand the controller back instead of merely claiming to. A leak here is the
// white-screen cause FeatureSwitchLedger models, and it cannot be caught by
// inspecting a session's own state.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "drivers/IBleCentral.h"

class FakeBleCentral final : public IBleCentral {
public:
    struct ReadResponse {
        BleStatus status = BleStatus::IoError;
        std::vector<uint8_t> payload;
    };

    struct WriteRecord {
        BleCharacteristicRef ref;
        std::vector<uint8_t> data;
        bool withResponse = true;
    };

    // ---- scripting -------------------------------------------------------

    void addAdvertisement(const BleAdvertisement& advertisement) {
        advertisements_.push_back(advertisement);
    }

    // "AA:BB:CC:DD:EE:FF" -> an advertisement, for tests that do not care about
    // the address bytes beyond using them as a key.
    BleAdvertisement& addAdvertisement(const char* address, int8_t rssi,
                                       const char* name, bool connectable = true) {
        BleAdvertisement advertisement;
        advertisement.address = parseAddress(address);
        advertisement.rssi = rssi;
        advertisement.connectable = connectable;
        if (name != nullptr) {
            std::snprintf(advertisement.name, sizeof(advertisement.name), "%s", name);
        }
        advertisements_.push_back(advertisement);
        return advertisements_.back();
    }

    static BleAddress parseAddress(const char* text) {
        BleAddress address;
        if (text == nullptr) {
            return address;
        }
        unsigned values[6] = {};
        // Accepts the colon-separated form the screens print, and tolerates a
        // shorter string by leaving the remaining bytes zero.
        if (std::sscanf(text, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2],
                        &values[3], &values[4], &values[5]) == 6) {
            for (uint8_t index = 0; index < 6; ++index) {
                address.bytes[index] = static_cast<uint8_t>(values[index]);
            }
        }
        return address;
    }

    void queueRead(BleStatus status, std::initializer_list<uint8_t> payload = {}) {
        ReadResponse response;
        response.status = status;
        response.payload.assign(payload.begin(), payload.end());
        reads_.push_back(std::move(response));
    }

    void queueReadOk(std::initializer_list<uint8_t> payload) {
        queueRead(BleStatus::Ok, payload);
    }

    // A 16-byte characteristic value spelled out as an initializer list in every
    // test buries the point of the test.
    void queueReadOk(const uint8_t* data, std::size_t length) {
        ReadResponse response;
        response.status = BleStatus::Ok;
        response.payload.assign(data, data + length);
        reads_.push_back(std::move(response));
    }

    // ---- scripting a request/response peer -------------------------------
    //
    // A Chameleon Ultra (and any device with a UART-over-GATT link) answers on
    // its notify characteristic when it is written to. Scripting that as "the
    // Nth write produces these notifications" is what makes a client's
    // command/response exchange testable, instead of the test having to know
    // when the client is listening.
    //
    // Each exchange is consumed by one write, in the order it was queued, so two
    // commands on the same write characteristic do not steal each other's reply.
    // Several chunks produce several notifications for that one write, which is
    // how a frame longer than the MTU actually arrives.
    void queueExchange(const BleCharacteristicRef& writeRef,
                       const BleCharacteristicRef& notifyRef,
                       std::initializer_list<std::vector<uint8_t>> chunks) {
        ScriptedExchange exchange;
        exchange.writeRef = writeRef;
        exchange.notifyRef = notifyRef;
        exchange.chunks.assign(chunks.begin(), chunks.end());
        exchanges_.push_back(std::move(exchange));
    }

    void queueReply(const BleCharacteristicRef& writeRef, const BleCharacteristicRef& notifyRef,
                    std::initializer_list<uint8_t> bytes) {
        std::vector<uint8_t> chunk(bytes.begin(), bytes.end());
        queueExchange(writeRef, notifyRef, {chunk});
    }

    // For a reply already built as a vector, which is what a frame encoder
    // returns. Braced initialiser lists still prefer the overload above.
    void queueReply(const BleCharacteristicRef& writeRef, const BleCharacteristicRef& notifyRef,
                    const std::vector<uint8_t>& bytes) {
        queueExchange(writeRef, notifyRef, {bytes});
    }

    // How many scripted exchanges a write has not consumed yet.
    std::size_t pendingExchangeCount() const {
        std::size_t pending = 0;
        for (const ScriptedExchange& exchange : exchanges_) {
            if (!exchange.consumed) {
                ++pending;
            }
        }
        return pending;
    }

    // ---- building a discovered peer -------------------------------------
    //
    // Counting is the fake's job, not the test's: writing straight into
    // `peer().services[0]` leaves `serviceCount` at zero, which silently turns a
    // discovery assertion into a tautology about an empty tree.
    //
    // Both return nullptr when the fixed array is full, and mark the peer
    // truncated, so overflow is visible rather than dropped on the floor.
    BleDiscoveredService* addService(const BleUuid& uuid) {
        if (peer_.serviceCount >= kBleMaxServicesPerPeer) {
            peer_.truncated = true;
            return nullptr;
        }
        BleDiscoveredService& service = peer_.services[peer_.serviceCount];
        service = BleDiscoveredService{};
        service.uuid = uuid;
        ++peer_.serviceCount;
        return &service;
    }

    // The SIG-base profiles, which are the common case.
    BleDiscoveredService* addService(uint16_t uuid) {
        return addService(BleUuid::fromShort(uuid));
    }

    BleDiscoveredCharacteristic* addCharacteristic(uint8_t serviceIndex, const BleUuid& uuid,
                                                   uint8_t properties) {
        if (serviceIndex >= peer_.serviceCount) {
            return nullptr;
        }
        BleDiscoveredService& service = peer_.services[serviceIndex];
        if (service.characteristicCount >= kBleMaxCharsPerService) {
            service.truncated = true;
            return nullptr;
        }
        BleDiscoveredCharacteristic& characteristic =
            service.characteristics[service.characteristicCount];
        characteristic = BleDiscoveredCharacteristic{};
        characteristic.uuid = uuid;
        characteristic.properties = properties;
        ++service.characteristicCount;
        return &characteristic;
    }

    BleDiscoveredCharacteristic* addCharacteristic(uint8_t serviceIndex, uint16_t uuid,
                                                   uint8_t properties) {
        return addCharacteristic(serviceIndex, BleUuid::fromShort(uuid), properties);
    }

    // ---- notification delivery ------------------------------------------

    // Delivers to every live subscription on `ref`. Returns how many callbacks
    // ran; 0 means nobody was listening, which a test can assert on rather than
    // assuming delivery happened.
    uint8_t deliver(const BleCharacteristicRef& ref, const uint8_t* data, uint16_t length) {
        uint8_t delivered = 0;
        for (const Subscription& subscription : subscriptions_) {
            // Exact match on the characteristic. A service-only fallback would
            // deliver a notification to every subscription in the service,
            // which is not what a real stack does -- a notification arrives on
            // one value handle.
            if (subscription.ref == ref) {
                if (subscription.callback != nullptr) {
                    subscription.callback(subscription.context, ref, data, length);
                    ++delivered;
                }
            }
        }
        if (delivered == 0) {
            ++undeliveredNotifications;
        }
        return delivered;
    }

    // ---- knobs -----------------------------------------------------------

    bool beginResult = true;
    BleStatus connectResult = BleStatus::Ok;
    BleStatus discoverResult = BleStatus::Ok;
    BleStatus subscribeResult = BleStatus::Ok;
    BleStatus writeResult = BleStatus::Ok;
    bool readyResult = true;

    BleCentralCaps capsValue = BleCentralCaps{
        true, true, true, false, true, true, true, false};

    // Resource accounting. These are what a teardown test reads.
    int beginCalls = 0;
    int endCalls = 0;
    int startScanCalls = 0;
    int stopScanCalls = 0;
    int connectCalls = 0;
    int disconnectCalls = 0;
    int discoverCalls = 0;
    int readCalls = 0;
    int writeCalls = 0;
    int unscriptedReads = 0;
    int undeliveredNotifications = 0;
    // Whether a scan asked the peer to identify itself. A "passive" tool that
    // silently requests scan responses is doing the opposite of what its label
    // says, and only the transport can tell the two apart.
    int activeScanRequests = 0;
    int passiveScanRequests = 0;

    // The peer discovery returns. Mutate it in a test, e.g.
    //   fake.peer().services[0].uuid = 0xFFF0;
    BleDiscoveredPeer& peer() { return peer_; }
    const BleAddress& connectedAddress() const { return connectedAddress_; }

    // ---- IBleCentral -----------------------------------------------------

    bool begin() override {
        ++beginCalls;
        return beginResult;
    }

    void end() override {
        ++endCalls;
        // A real teardown drops everything. Modelling that here is deliberate:
        // a session that forgets to unsubscribe must not be able to hide behind
        // a fake that keeps the subscription alive.
        connected_ = false;
        scanning_ = false;
        subscriptions_.clear();
        reads_.clear();
        readIndex_ = 0;
        scanCursor_ = 0;
    }

    bool ready() const override { return readyResult; }
    const char* name() const override { return "fake BLE central"; }
    BleCentralCaps caps() const override { return capsValue; }

    BleStatus startScan(uint32_t durationMs, bool active) override {
        (void)durationMs;
        ++startScanCalls;
        if (active) {
            ++activeScanRequests;
        } else {
            ++passiveScanRequests;
        }
        if (scanning_) {
            return BleStatus::Busy;
        }
        scanning_ = true;
        // A new scan starts a new stream: without this, a second scan would
        // report an empty list because the cursor was already at the end.
        scanCursor_ = 0;
        return BleStatus::Ok;
    }

    BleStatus stopScan() override {
        ++stopScanCalls;
        scanning_ = false;
        return BleStatus::Ok;
    }

    bool scanning() const override { return scanning_; }

    uint8_t scanResults(BleAdvertisement* out, uint8_t capacity) override {
        if (out == nullptr || capacity == 0U) {
            return 0;
        }
        // A cursor, not a fresh copy of the list: a scan drains over time, and a
        // caller that loops until it sees zero must be able to reach zero. The
        // cursor is reset by startScan(), the way a new scan starts a new stream.
        const std::size_t remaining = advertisements_.size() - scanCursor_;
        const uint8_t count = static_cast<uint8_t>(
            std::min<std::size_t>(remaining, capacity));
        for (uint8_t index = 0; index < count; ++index) {
            out[index] = advertisements_[scanCursor_ + index];
        }
        scanCursor_ += count;
        return count;
    }

    BleStatus connect(const BleAddress& address, uint32_t timeoutMs) override {
        (void)timeoutMs;
        ++connectCalls;
        if (!address.valid()) {
            return BleStatus::NotFound;
        }
        if (connectResult != BleStatus::Ok) {
            return connectResult;
        }
        connected_ = true;
        connectedAddress_ = address;
        peer_.address = address;
        return BleStatus::Ok;
    }

    BleStatus disconnect() override {
        ++disconnectCalls;
        connected_ = false;
        subscriptions_.clear();
        return BleStatus::Ok;
    }

    bool connected() const override { return connected_; }

    BleStatus discover(BleDiscoveredPeer& out) override {
        ++discoverCalls;
        if (!connected_) {
            out = BleDiscoveredPeer{};
            return BleStatus::Disconnected;
        }
        if (discoverResult != BleStatus::Ok) {
            out = BleDiscoveredPeer{};
            return discoverResult;
        }
        out = peer_;
        out.address = connectedAddress_;
        return BleStatus::Ok;
    }

    BleStatus read(const BleCharacteristicRef& ref, uint8_t* out, uint16_t& length,
                   uint32_t timeoutMs) override {
        (void)timeoutMs;
        ++readCalls;
        if (!connected_) {
            length = 0;
            return BleStatus::Disconnected;
        }
        if (!ref.valid()) {
            length = 0;
            return BleStatus::NoService;
        }
        if (readIndex_ >= reads_.size()) {
            ++unscriptedReads;
            length = 0;
            return BleStatus::IoError;
        }
        const ReadResponse& response = reads_[readIndex_++];
        if (response.status != BleStatus::Ok) {
            length = 0;
            return response.status;
        }
        const uint16_t available = static_cast<uint16_t>(response.payload.size());
        if (available > length) {
            // Report the size needed so the caller can retry; never a silent
            // short read.
            length = available;
            return BleStatus::TooLong;
        }
        std::memcpy(out, response.payload.data(), available);
        length = available;
        return BleStatus::Ok;
    }

    BleStatus write(const BleCharacteristicRef& ref, const uint8_t* data, uint16_t length,
                    bool withResponse) override {
        ++writeCalls;
        WriteRecord record;
        record.ref = ref;
        record.withResponse = withResponse;
        if (data != nullptr && length > 0U) {
            record.data.assign(data, data + length);
        }
        writes_.push_back(std::move(record));
        // The peer answers *before* the caller learns the status, so a client can
        // read the reply as soon as its write returns. That is the scripting
        // contract; a real stack delivers it a moment later, and nothing above
        // this line depends on which.
        for (ScriptedExchange& exchange : exchanges_) {
            if (!exchange.consumed && exchange.writeRef == ref) {
                exchange.consumed = true;
                for (const std::vector<uint8_t>& chunk : exchange.chunks) {
                    deliver(exchange.notifyRef, chunk.data(),
                            static_cast<uint16_t>(chunk.size()));
                }
                break;
            }
        }
        if (!connected_) {
            return BleStatus::Disconnected;
        }
        if (!ref.valid()) {
            return BleStatus::NoService;
        }
        return writeResult;
    }

    BleStatus subscribe(const BleCharacteristicRef& ref, BleNotifyFn callback,
                        void* context) override {
        if (!connected_) {
            return BleStatus::Disconnected;
        }
        if (subscribeResult != BleStatus::Ok) {
            return subscribeResult;
        }
        Subscription subscription;
        subscription.ref = ref;
        subscription.callback = callback;
        subscription.context = context;
        subscriptions_.push_back(subscription);
        return BleStatus::Ok;
    }

    BleStatus unsubscribe(const BleCharacteristicRef& ref) override {
        const std::size_t before = subscriptions_.size();
        subscriptions_.erase(
            std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                           [&ref](const Subscription& s) { return s.ref == ref; }),
            subscriptions_.end());
        return subscriptions_.size() != before ? BleStatus::Ok : BleStatus::NotFound;
    }

    uint8_t subscriptionCount() const override {
        return static_cast<uint8_t>(subscriptions_.size());
    }

    const char* lastError() const override { return lastError_; }

    // ---- inspection ------------------------------------------------------

    std::size_t writeCount() const { return writes_.size(); }
    const WriteRecord& writeAt(std::size_t index) const { return writes_.at(index); }
    const WriteRecord& lastWrite() const { return writes_.back(); }
    void clearWrites() { writes_.clear(); }
    std::size_t advertisementCount() const { return advertisements_.size(); }
    void clearAdvertisements() { advertisements_.clear(); }

private:
    struct Subscription {
        BleCharacteristicRef ref;
        BleNotifyFn callback = nullptr;
        void* context = nullptr;
    };

    struct ScriptedExchange {
        BleCharacteristicRef writeRef;
        BleCharacteristicRef notifyRef;
        std::vector<std::vector<uint8_t>> chunks;
        bool consumed = false;
    };

    std::vector<BleAdvertisement> advertisements_;
    std::vector<ReadResponse> reads_;
    std::vector<WriteRecord> writes_;
    std::vector<Subscription> subscriptions_;
    std::vector<ScriptedExchange> exchanges_;
    std::size_t readIndex_ = 0;
    std::size_t scanCursor_ = 0;
    BleDiscoveredPeer peer_{};
    BleAddress connectedAddress_{};
    bool connected_ = false;
    bool scanning_ = false;
    const char* lastError_ = "";
};
