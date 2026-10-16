#include "drivers/Esp32BleCentral.h"

#include <cstring>
#include <map>
#include <string>

// The BLE counterpart of WirePn532Link: the single Arduino-dependent file in
// the stack, with every decision above it (tree model, session, framing)
// host-testable. See the header for which BT host each build environment uses
// and why the adapter has to serve two of them.

#if defined(ARDUINO) && (defined(CONFIG_NIMBLE_ENABLED) || defined(CONFIG_BLUEDROID_ENABLED))

// ---------------------------------------------------------------------------
// Host and version normalisation
// ---------------------------------------------------------------------------
//
// One Arduino BLE library, two Espressif hosts, three API generations, and this
// repository builds all three combinations:
//
//   nm-cyd-c5   Arduino 3.3.6, NimBLE     (C5 lib package is NimBLE-only)
//   esp32-cyd   Arduino 2.0.17, Bluedroid (the legacy BLE screens' host)
//
// Only four things actually differ, and each is named once here so the body of
// the adapter reads as a single implementation.

#if defined(CONFIG_NIMBLE_ENABLED)

// NimBLE: ble_uuid_any_t, with the width in `.u.type` (16 / 32 / 128).
//
// The parameter is `x`, not `u`: the member being reached for is itself called
// `u`, and a macro parameter is substituted everywhere it appears in the body,
// so `#define HH_UUID_LEN(u) ((u).u.type)` would also rewrite the member name
// and expand to `((*native).*native.type)`.
#define HH_UUID_LEN(x)      ((x).u.type)
#define HH_UUID_LEN_16      BLE_UUID_TYPE_16
#define HH_UUID_LEN_32      BLE_UUID_TYPE_32
#define HH_UUID_LEN_128     BLE_UUID_TYPE_128
#define HH_UUID_VAL_16(x)   ((x).u16.value)
#define HH_UUID_VAL_32(x)   ((x).u32.value)
#define HH_UUID_VAL_128(x)  ((x).u128.value)

// ble_addr_t's two named types. Bluedroid's BLE_ADDR_TYPE_* are not declared
// under NimBLE at all, which is why this cannot be a plain constant.
#define HH_ADDR_TYPE_PUBLIC BLE_ADDR_PUBLIC
#define HH_ADDR_TYPE_RANDOM BLE_ADDR_RANDOM

// NimBLE hands back peer addresses least-significant byte first; the seam keeps
// printed order (see BleAddress in IBleCentral.h).
#define HH_ADDR_BYTES_REVERSED 1

// BLEScan::isScanning() is declared under CONFIG_NIMBLE_ENABLED only.
#define HH_BLE_HAS_IS_SCANNING 1

#elif defined(CONFIG_BLUEDROID_ENABLED)

#if ESP_ARDUINO_VERSION_MAJOR >= 3

// Bluedroid 3.x: esp_bt_uuid_t, and the library's own portable aliases for it.
#define HH_UUID_LEN(x)      UUID_LEN(x)
#define HH_UUID_LEN_16      BLE_UUID_16_BITS
#define HH_UUID_LEN_32      BLE_UUID_32_BITS
#define HH_UUID_LEN_128     BLE_UUID_128_BITS
#define HH_UUID_VAL_16(x)   UUID_VAL_16(x)
#define HH_UUID_VAL_32(x)   UUID_VAL_32(x)
#define HH_UUID_VAL_128(x)  UUID_VAL_128(x)

#else

// Bluedroid 2.0.17: no aliases and no UUID_LEN macros exist, so esp_bt_uuid_t
// is reached field by field.
#define HH_UUID_LEN(x)      ((x).len)
#define HH_UUID_LEN_16      ESP_UUID_LEN_16
#define HH_UUID_LEN_32      ESP_UUID_LEN_32
#define HH_UUID_LEN_128     ESP_UUID_LEN_128
#define HH_UUID_VAL_16(x)   ((x).uuid.uuid16)
#define HH_UUID_VAL_32(x)   ((x).uuid.uuid32)
#define HH_UUID_VAL_128(x)  ((x).uuid.uuid128)

#endif

#define HH_ADDR_TYPE_PUBLIC BLE_ADDR_TYPE_PUBLIC
#define HH_ADDR_TYPE_RANDOM BLE_ADDR_TYPE_RANDOM

// esp_bd_addr_t is in printed order, which is already what the seam stores.
#define HH_ADDR_BYTES_REVERSED 0

#endif

namespace {

// Empty, matching what the legacy features pass. This adapter must not name the
// device, because a legacy feature that runs later sets its own name and would
// otherwise inherit ours.
constexpr const char* kControllerName = "";

// Carried over from the legacy code, which calls it a race-condition fix: the
// controller needs a moment after init() before getScan() is usable.
constexpr uint32_t kControllerSettleMs = 150;

// Near-continuous passive listening. 100 / 100 is also the library's own
// default for both hosts, so this is a statement of intent rather than a
// deviation.
constexpr uint16_t kScanIntervalMs = 100;
constexpr uint16_t kScanWindowMs = 100;

// A connect that returns false says nothing about why, so this is only a bound
// on how long the UI is stuck.
constexpr uint32_t kDefaultConnectTimeoutMs = 10000;

// One controller, one instance. `void (*)(BLEScanResults)` carries no context,
// so the scan-completion callback has to reach the instance through a file-scope
// pointer. begin() refuses a second instance rather than letting this be
// silently repointed.
Esp32BleCentral* g_scanCompleteTarget = nullptr;

// The library stores a 128-bit UUID least-significant byte first on *both*
// hosts, and says so itself: its own constructor memrcpy()s a printed string
// into the array, and its toString() prints element 15 first. The seam stores
// printed order, so this reverses on the way across. One implementation covers
// both hosts because HH_UUID_VAL_128 already points at the right array.
BleUuid seamUuid(BLEUUID uuid) {
    const auto* native = uuid.getNative();
    if (native == nullptr) {
        return BleUuid{};
    }
    const auto width = HH_UUID_LEN(*native);
    if (width == HH_UUID_LEN_16) {
        return BleUuid::fromShort(HH_UUID_VAL_16(*native));
    }
    if (width == HH_UUID_LEN_32) {
        return BleUuid::fromLong(HH_UUID_VAL_32(*native));
    }
    if (width == HH_UUID_LEN_128) {
        uint8_t printed[16];
        for (uint8_t index = 0; index < 16; ++index) {
            printed[index] = HH_UUID_VAL_128(*native)[15 - index];
        }
        return BleUuid::fromBytes(printed);
    }
    return BleUuid{};
}

BleAddressType seamAddressType(uint8_t native) {
    if (native == HH_ADDR_TYPE_PUBLIC) {
        return BleAddressType::Public;
    }
    if (native == HH_ADDR_TYPE_RANDOM) {
        return BleAddressType::Random;
    }
    // Includes the two resolvable-private types. The seam has no name for them
    // and inventing "Random" for an RPA would make two different peers compare
    // equal, so unknown stays unknown.
    return BleAddressType::Unknown;
}

uint8_t nativeAddressType(BleAddressType type) {
    switch (type) {
        case BleAddressType::Public:
            return static_cast<uint8_t>(HH_ADDR_TYPE_PUBLIC);
        case BleAddressType::Random:
            return static_cast<uint8_t>(HH_ADDR_TYPE_RANDOM);
        case BleAddressType::Unknown:
            // The controller must pick one; public is the default and a wrong
            // guess surfaces as ConnectFailed rather than as a quiet success.
            return static_cast<uint8_t>(HH_ADDR_TYPE_PUBLIC);
    }
    return static_cast<uint8_t>(HH_ADDR_TYPE_PUBLIC);
}

// The property byte, synthesised from the predicates because no Arduino version
// exposes the declaration byte itself. The seam's bits are the GATT declaration
// bits, so this is a transcription rather than a translation.
uint8_t propertyBits(BLERemoteCharacteristic& characteristic) {
    uint8_t bits = 0;
    if (characteristic.canBroadcast()) {
        bits |= kBlePropBroadcast;
    }
    if (characteristic.canRead()) {
        bits |= kBlePropRead;
    }
    if (characteristic.canWriteNoResponse()) {
        bits |= kBlePropWriteNoResponse;
    }
    if (characteristic.canWrite()) {
        bits |= kBlePropWrite;
    }
    if (characteristic.canNotify()) {
        bits |= kBlePropNotify;
    }
    if (characteristic.canIndicate()) {
        bits |= kBlePropIndicate;
    }
    return bits;
}

// `getName()` and `getManufacturerData()` are `String` on Arduino 3.x and
// `std::string` on 2.0.17. Both have length() and c_str(), which is all this
// needs.
//
// Returns how many bytes were copied. The count has to come from the source's
// own length rather than from scanning for a terminator: manufacturer data is
// raw bytes, and a payload that starts with 0x00 would otherwise be measured as
// empty.
template <typename TextLike>
std::size_t copyClamped(char* out, std::size_t capacity, const TextLike& text) {
    const std::size_t length = text.length();
    const std::size_t take = length < capacity ? length : capacity;
    if (take > 0) {
        std::memcpy(out, text.c_str(), take);
    }
    out[take] = '\0';
    return take;
}

}  // namespace

void Esp32BleCentral::AdvertisementSink::onResult(BLEAdvertisedDevice advertisedDevice) {
    owner_.acceptAdvertisement(advertisedDevice);
}

void Esp32BleCentral::onScanComplete(BLEScanResults results) {
    (void)results;
    if (g_scanCompleteTarget != nullptr) {
        // A bounded scan ends here, on the host's task. Neither Bluedroid
        // version has an isScanning() to ask, and a mirror that is never cleared
        // would leave the UI reporting a scan that finished.
        g_scanCompleteTarget->scanning_ = false;
    }
}

bool Esp32BleCentral::begin() {
    if (ready_) {
        return true;
    }
    if (g_scanCompleteTarget != nullptr && g_scanCompleteTarget != this) {
        lastError_ = "another Esp32BleCentral instance already owns the controller";
        return false;
    }

    if (!BLEDevice::getInitialized()) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        if (!BLEDevice::init(kControllerName)) {
            lastError_ = "BLE controller init failed";
            return false;
        }
#else
        // 2.0.17's init() returns void, so there is nothing to check.
        BLEDevice::init(std::string(kControllerName));
#endif
        delay(kControllerSettleMs);
    }

    scan_ = BLEDevice::getScan();
    if (scan_ == nullptr) {
        lastError_ = "no scan object";
        return false;
    }
    scan_->setActiveScan(false);
    scan_->setInterval(kScanIntervalMs);
    scan_->setWindow(kScanWindowMs);

    g_scanCompleteTarget = this;
    ready_ = true;
    lastError_ = "";
    return true;
}

void Esp32BleCentral::end() {
    if (!ready_) {
        return;
    }

    releaseSubscriptions();

    if (client_ != nullptr) {
        if (client_->isConnected()) {
            client_->disconnect();
        }
        // BLEDevice owns the client's registration; deleteClient() only exists
        // on NimBLE and is not needed, so the object itself is freed here.
        delete client_;
        client_ = nullptr;
    }

    if (scan_ != nullptr) {
        // On NimBLE, stop() invokes the completion callback, which is why
        // g_scanCompleteTarget is still installed at this point.
        scan_->stop();
        // Both versions guard the callback for null before invoking it. Leaving
        // it pointed at advSink_ would leave the host holding a reference to a
        // pool this object owns.
        scan_->setAdvertisedDeviceCallbacks(nullptr);
    }

    // The controller itself is deliberately left initialised. Tearing it down
    // costs a full host restart (and the legacy BLE screens re-init on their own
    // entry anyway), and BLEDevice is shared state this adapter does not own.
    // What end() guarantees is that no scan, no connection and no subscription
    // survives -- which is what FeatureSwitchLedger measures.
    peerAddress_ = BleAddress{};
    mtu_ = 23;
    queueHead_ = 0;
    queueTail_ = 0;
    scanning_ = false;
    connected_ = false;
    ready_ = false;
    lastError_ = "";

    if (g_scanCompleteTarget == this) {
        g_scanCompleteTarget = nullptr;
    }
}

bool Esp32BleCentral::ready() const {
    return ready_;
}

const char* Esp32BleCentral::name() const {
    return "ESP32 BLE";
}

BleCentralCaps Esp32BleCentral::caps() const {
    BleCentralCaps caps{};
    caps.central = true;
    caps.observer = true;
    caps.passiveScan = true;
    // The Arduino scan API exposes no extended-advertising control on either
    // host. Claiming it would be a promise the adapter cannot keep.
    caps.extendedAdv = false;
    caps.writeWithoutResponse = true;
    caps.notify = true;
    caps.indicate = true;
    // Both hosts can bond; this seam has no pairing entry point, so there is no
    // way to ask for it. Reporting false keeps the honest signal in the place a
    // caller will look -- a rejected write comes back as NotPermitted rather
    // than as a silent, failed bond attempt.
    caps.bonding = false;
    return caps;
}

bool Esp32BleCentral::stackScanning() const {
    if (scan_ == nullptr) {
        return false;
    }
#if defined(HH_BLE_HAS_IS_SCANNING)
    // The host is the authority here, which matters because NimBLE's connect()
    // stops a running scan when the controller answers EBUSY.
    return scan_->isScanning();
#else
    // Neither Bluedroid version has isScanning(), so the mirror set by
    // startScan(), stopScan() and onScanComplete() is the answer.
    return scanning_;
#endif
}

BleStatus Esp32BleCentral::startScan(uint32_t durationMs, bool active) {
    if (!ready_ || scan_ == nullptr) {
        return fail(BleStatus::IoError, "controller not started");
    }
    if (stackScanning()) {
        return fail(BleStatus::Busy, "scan already running");
    }

    scan_->setActiveScan(active);
    // wantDuplicates=true is what makes this a live feed rather than a
    // one-shot list: on NimBLE it clears the controller's duplicate filter, and
    // a passive scan then delivers every advertisement as it arrives. With the
    // filter left on, the callback would fire once per device and the drain
    // would look permanently empty.
    scan_->setAdvertisedDeviceCallbacks(&advSink_,
                                        /*wantDuplicates=*/true,
                                        /*shouldParse=*/true);
    queueHead_ = 0;
    queueTail_ = 0;
    advDropped_ = 0;

    // The Arduino duration is in whole seconds, and 0 means "until stopScan()".
    // Rounding up keeps a sub-second request from being read as "for ever".
    const uint32_t seconds = durationMs == 0 ? 0 : (durationMs + 999U) / 1000U;
    scanning_ = true;
    if (!scan_->start(seconds, onScanComplete, /*is_continue=*/false)) {
        scanning_ = false;
        return fail(BleStatus::IoError, "scan did not start");
    }
    lastError_ = "";
    return BleStatus::Ok;
}

BleStatus Esp32BleCentral::stopScan() {
    if (scan_ != nullptr) {
        // Discarding the return is deliberate: 3.x answers bool, 2.0.17 answers
        // void, and neither tells us anything a caller could act on that
        // scanning() does not already say.
        scan_->stop();
    }
    scanning_ = false;
    return BleStatus::Ok;
}

bool Esp32BleCentral::scanning() const {
    return ready_ && stackScanning();
}

uint8_t Esp32BleCentral::scanResults(BleAdvertisement* out, uint8_t capacity) {
    if (out == nullptr || capacity == 0) {
        return 0;
    }
    // Draining, not re-reading: the tail advances past everything handed over,
    // so a caller that loops until it sees zero terminates. A ring that
    // re-served its contents would reproduce, on hardware, the hang the host
    // suite pins against.
    uint8_t written = 0;
    while (written < capacity && queueTail_ != queueHead_) {
        out[written] = queue_[queueTail_];
        queueTail_ = static_cast<uint8_t>((queueTail_ + 1) % kAdvQueueDepth);
        ++written;
    }
    return written;
}

BleStatus Esp32BleCentral::connect(const BleAddress& address, uint32_t timeoutMs) {
    if (!ready_) {
        return fail(BleStatus::IoError, "controller not started");
    }
    if (!address.valid()) {
        return fail(BleStatus::NotFound, "invalid address");
    }
    if (client_ != nullptr && client_->isConnected()) {
        client_->disconnect();
    }

    if (client_ == nullptr) {
        client_ = BLEDevice::createClient();
        if (client_ == nullptr) {
            return fail(BleStatus::IoError, "no GATT client");
        }
    }

    // The seam stores printed order, and both hosts' constructors take it:
    // Bluedroid directly (esp_bd_addr_t is printed order) and NimBLE by
    // reversing internally. So this is a straight copy either way.
    uint8_t raw[6];
    std::memcpy(raw, address.bytes.data(), sizeof(raw));
    const uint8_t type = nativeAddressType(address.type);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #if defined(CONFIG_NIMBLE_ENABLED)
    // NimBLE's BLEAddress carries its own type, and its connect() reads the type
    // off the address rather than off the `type` argument. Constructing with
    // the right type is therefore what actually decides the address type.
    BLEAddress peer(raw, type);
  #else
    BLEAddress peer(raw);
  #endif
    const uint32_t timeout = timeoutMs == 0 ? kDefaultConnectTimeoutMs : timeoutMs;
    const bool connected = client_->connect(peer, type, timeout);
#else
    // 2.0.17's BLEAddress takes no type; the type goes to connect() instead.
    BLEAddress peer(raw);
    const bool connected = client_->connect(peer, static_cast<esp_ble_addr_type_t>(type));
#endif

    if (!connected) {
        // The API answers yes/no, so "the peer refused" and "the peer never
        // answered" are not separable here. ConnectFailed rather than Timeout:
        // claiming the clock ran out would be a guess.
        return fail(BleStatus::ConnectFailed, "connect returned false");
    }

    connected_ = true;
    peerAddress_ = address;

    // No scan bookkeeping here. NimBLE's connect() stops a running scan only if
    // the controller answers EBUSY, and neither Bluedroid version stops one at
    // all, so clearing the mirror on success would be a guess -- and on NimBLE
    // `scanning()` asks the host directly anyway, so the truth is available
    // without one.

    mtu_ = 23;
    if (client_->setMTU(kPreferredMtu)) {
        const uint16_t negotiated = client_->getMTU();
        mtu_ = negotiated > 3 ? negotiated : 23;
    }
    lastError_ = "";
    return BleStatus::Ok;
}

BleStatus Esp32BleCentral::disconnect() {
    releaseSubscriptions();
    if (client_ != nullptr && client_->isConnected()) {
        client_->disconnect();
    }
    connected_ = false;
    peerAddress_ = BleAddress{};
    mtu_ = 23;
    lastError_ = "";
    return BleStatus::Ok;
}

bool Esp32BleCentral::connected() const {
    return connected_;
}

BleStatus Esp32BleCentral::discover(BleDiscoveredPeer& out) {
    out = BleDiscoveredPeer{};
    if (client_ == nullptr || !client_->isConnected()) {
        return fail(BleStatus::Disconnected, "not connected");
    }

    // getServices() runs the service discovery on the first call and blocks
    // until it has an answer.
    std::map<std::string, BLERemoteService*>* services = client_->getServices();
    if (services == nullptr) {
        return fail(BleStatus::IoError, "service search failed");
    }
    if (services->empty()) {
        return fail(BleStatus::NoService, "peer offered no services");
    }

    out.address = peerAddress_;
    out.mtu = mtu_;

    uint8_t serviceCount = 0;
    for (auto& entry : *services) {
        if (serviceCount >= kBleMaxServicesPerPeer) {
            out.truncated = true;
            break;
        }
        BLERemoteService* service = entry.second;
        if (service == nullptr) {
            continue;
        }

        BleDiscoveredService& destination = out.services[serviceCount];
        destination.uuid = seamUuid(service->getUUID());

        // getCharacteristics() runs the discovery; getCharacteristicsByHandle()
        // only reports what it stored. Reading the by-handle map matters because
        // the by-UUID map is keyed on the printed UUID, so two characteristics
        // sharing a UUID inside one service collapse into one entry there --
        // a peer that declares duplicates would otherwise be drawn as if it had
        // fewer characteristics than it does.
        service->getCharacteristics();
        std::map<uint16_t, BLERemoteCharacteristic*>* characteristics =
            service->getCharacteristicsByHandle();
        if (characteristics != nullptr) {
            uint8_t characteristicCount = 0;
            for (auto& characteristicEntry : *characteristics) {
                if (characteristicCount >= kBleMaxCharsPerService) {
                    destination.truncated = true;
                    break;
                }
                BLERemoteCharacteristic* characteristic = characteristicEntry.second;
                if (characteristic == nullptr) {
                    continue;
                }
                BleDiscoveredCharacteristic& slot =
                    destination.characteristics[characteristicCount];
                slot.uuid = seamUuid(characteristic->getUUID());
                slot.properties = propertyBits(*characteristic);
                ++characteristicCount;
            }
            destination.characteristicCount = characteristicCount;
        }

        ++serviceCount;
    }
    out.serviceCount = serviceCount;
    lastError_ = "";
    return BleStatus::Ok;
}

BleStatus Esp32BleCentral::read(const BleCharacteristicRef& ref, uint8_t* out,
                                uint16_t& length, uint32_t timeoutMs) {
    (void)timeoutMs;
    if (out == nullptr) {
        length = 0;
        return fail(BleStatus::IoError, "no output buffer");
    }
    if (client_ == nullptr || !client_->isConnected()) {
        length = 0;
        return fail(BleStatus::Disconnected, "not connected");
    }

    BLERemoteCharacteristic* characteristic = locate(ref);
    if (characteristic == nullptr) {
        length = 0;
        return fail(BleStatus::NoService, "service or characteristic absent");
    }
    if (!characteristic->canRead()) {
        // A fact about the declaration, available before any frame goes out.
        length = 0;
        return fail(BleStatus::NotPermitted, "characteristic is not readable");
    }

    // `String` on 3.x, `std::string` on 2.0.17; both are length-bounded, so a
    // zero byte inside the payload survives. This matters: a Chameleon reply
    // frame carries 0x00 in its status and length fields.
    const auto value = characteristic->readValue();
    if (!client_->isConnected()) {
        length = 0;
        return fail(BleStatus::Disconnected, "link dropped during read");
    }

    const std::size_t received = value.length();
    if (received > length) {
        // The contract is the size needed, not the size we managed to return.
        length = static_cast<uint16_t>(received > 0xFFFFU ? 0xFFFFU : received);
        return fail(BleStatus::TooLong, "value larger than the caller's buffer");
    }
    if (received > 0) {
        std::memcpy(out, value.c_str(), received);
    }
    length = static_cast<uint16_t>(received);
    lastError_ = "";
    return BleStatus::Ok;
}

BleStatus Esp32BleCentral::write(const BleCharacteristicRef& ref, const uint8_t* data,
                                 uint16_t length, bool withResponse) {
    if (client_ == nullptr || !client_->isConnected()) {
        return fail(BleStatus::Disconnected, "not connected");
    }

    BLERemoteCharacteristic* characteristic = locate(ref);
    if (characteristic == nullptr) {
        return fail(BleStatus::NoService, "service or characteristic absent");
    }
    // Checked against the declaration, before any frame goes out, so a
    // property that cannot work is a fact about the peer rather than a write
    // that mysteriously fails.
    if (!characteristic->canWrite() && !characteristic->canWriteNoResponse()) {
        return fail(BleStatus::NotPermitted, "characteristic is not writable");
    }
    if (withResponse && !characteristic->canWrite()) {
        // Write-without-response only. Asking for an acknowledgement would be a
        // different operation from the one the peer advertised, so this is
        // refused rather than silently downgraded -- a caller that wanted the
        // fast path should say so.
        return fail(BleStatus::NotPermitted, "no write-with-response property");
    }

    // writeValue() takes a non-const pointer but does not modify the buffer; it
    // hands it straight to the host, which does not write through it either.
    uint8_t* mutableData = const_cast<uint8_t*>(data);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (characteristic->writeValue(mutableData, length, withResponse)) {
        lastError_ = "";
        return BleStatus::Ok;
    }
    if (!client_->isConnected()) {
        return fail(BleStatus::Disconnected, "link dropped during write");
    }
    // The Arduino layer swallows the ATT status and returns only a bool, so a
    // rejection caused by access permissions is not separable from a host
    // failure here. Reported as IoError rather than NotPermitted on purpose:
    // guessing "the peer refused" is exactly the mistake this status enum
    // exists to prevent, because it makes a broken link look like a protected
    // characteristic.
    return fail(BleStatus::IoError, "write failed on a live link");
#else
    // 2.0.17's writeValue() returns void -- the library reads the ATT status,
    // logs it and drops it. The only fact left is whether the link survived the
    // call, so that is what this reports. Inventing a NotPermitted the layer
    // cannot substantiate would be the same mistake described above.
    characteristic->writeValue(mutableData, length, withResponse);
    if (!client_->isConnected()) {
        return fail(BleStatus::Disconnected, "link dropped during write");
    }
    lastError_ = "";
    return BleStatus::Ok;
#endif
}

BleStatus Esp32BleCentral::subscribe(const BleCharacteristicRef& ref, BleNotifyFn callback,
                                     void* context) {
    if (client_ == nullptr || !client_->isConnected()) {
        return fail(BleStatus::Disconnected, "not connected");
    }
    if (callback == nullptr) {
        return fail(BleStatus::IoError, "no callback");
    }
    if (findSubscription(ref) != nullptr) {
        return BleStatus::Ok;
    }

    BLERemoteCharacteristic* characteristic = locate(ref);
    if (characteristic == nullptr) {
        return fail(BleStatus::NoService, "service or characteristic absent");
    }
    if (!characteristic->canNotify() && !characteristic->canIndicate()) {
        return fail(BleStatus::NotPermitted, "characteristic does not notify");
    }

    for (uint8_t index = 0; index < kMaxSubscriptions; ++index) {
        Subscription& slot = subscriptions_[index];
        if (slot.live) {
            continue;
        }
        // Filled before registering, so a notification that arrives during the
        // registration write already finds a live slot rather than being
        // dropped.
        slot.ref = ref;
        slot.characteristic = characteristic;
        slot.callback = callback;
        slot.context = context;
        slot.live = true;

        // notify_callback is a std::function on every version, so an ordinary
        // capturing lambda works and no static trampoline is needed. The
        // capture is small but does exceed the small-buffer limit, so this
        // allocates -- once, here, on the caller's task. The notification path
        // below allocates nothing, which is what the seam requires of it.
        characteristic->registerForNotify(
            [this, index](BLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
                Subscription& live = subscriptions_[index];
                if (!live.live || live.callback == nullptr) {
                    return;
                }
                const uint16_t clamped =
                    length > 0xFFFFU ? 0xFFFFU : static_cast<uint16_t>(length);
                live.callback(live.context, live.ref, data, clamped);
            });
        lastError_ = "";
        return BleStatus::Ok;
    }

    ++subOverflow_;
    return fail(BleStatus::Busy, "no free subscription slot");
}

BleStatus Esp32BleCentral::unsubscribe(const BleCharacteristicRef& ref) {
    Subscription* slot = findSubscription(ref);
    if (slot == nullptr) {
        return fail(BleStatus::NotFound, "no such subscription");
    }
    detachSubscription(*slot);
    lastError_ = "";
    return BleStatus::Ok;
}

uint8_t Esp32BleCentral::subscriptionCount() const {
    uint8_t count = 0;
    for (uint8_t index = 0; index < kMaxSubscriptions; ++index) {
        if (subscriptions_[index].live) {
            ++count;
        }
    }
    return count;
}

uint16_t Esp32BleCentral::negotiatedMtu() const {
    return mtu_;
}

const char* Esp32BleCentral::lastError() const {
    return lastError_;
}

void Esp32BleCentral::acceptAdvertisement(BLEAdvertisedDevice& device) {
    const uint8_t next = static_cast<uint8_t>((queueHead_ + 1) % kAdvQueueDepth);
    if (next == queueTail_) {
        // Full. Dropping the newest rather than overwriting the oldest keeps
        // the producer from ever moving the consumer's index, which is the one
        // race this ring would otherwise have. The count makes a stalled
        // consumer visible instead of mysterious.
        ++advDropped_;
        return;
    }

    BleAdvertisement& slot = queue_[queueHead_];

    // Only the counted fields need clearing; the tails of `name` and
    // manufacturerData are unreachable because they are length-bounded.
    slot.name[0] = '\0';
    slot.appearance = 0;
    slot.serviceCount = 0;
    slot.servicesTruncated = false;
    slot.manufacturerLength = 0;
    slot.rssi = static_cast<int8_t>(device.getRSSI());

    const uint8_t type = static_cast<uint8_t>(device.getAddressType());
    slot.address.type = seamAddressType(type);

    // getNative() is uint8_t* on Arduino 3.x and esp_bd_addr_t* (== uint8_t(*)[6])
    // on 2.0.17; both point at the same six bytes, so one cast covers them.
    BLEAddress peer = device.getAddress();
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(peer.getNative());
#if HH_ADDR_BYTES_REVERSED
    // NimBLE hands the address back least-significant byte first; the seam wants
    // printed order.
    for (std::size_t index = 0; index < slot.address.bytes.size(); ++index) {
        slot.address.bytes[index] = raw[slot.address.bytes.size() - 1 - index];
    }
#else
    std::memcpy(slot.address.bytes.data(), raw, slot.address.bytes.size());
#endif

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    slot.connectable = device.isConnectable();
#else
    // 2.0.17's BLEAdvertisedDevice exposes no advertising type and no
    // isConnectable(), so whether this peer accepts a connection is simply not
    // answerable from here -- the AD payload the API does expose carries the
    // flags, not the ADV type. Reported as connectable so the session still
    // offers the peer; the alternative is a UI that refuses every device, which
    // trades a possible wasted timeout for a guaranteed dead feature. The
    // failure, if any, surfaces honestly as ConnectFailed.
    slot.connectable = true;
#endif

    if (device.haveName()) {
        (void)copyClamped(slot.name, kBleMaxAdvName, device.getName());
    }
    if (device.haveAppearance()) {
        slot.appearance = device.getAppearance();
    }

    const int serviceCount = device.getServiceUUIDCount();
    for (int index = 0; index < serviceCount; ++index) {
        if (static_cast<std::size_t>(slot.serviceCount) >= kBleMaxAdvServices) {
            slot.servicesTruncated = true;
            break;
        }
        slot.serviceUuids[slot.serviceCount] = seamUuid(device.getServiceUUID(index));
        ++slot.serviceCount;
    }

    if (device.haveManufacturerData()) {
        char bytes[kBleMaxManufacturerData + 1] = {};
        // The byte count comes from the source's length, not from a terminator
        // scan: manufacturer data is raw bytes and a leading 0x00 is ordinary --
        // Apple's popup payloads and BLE beacons both begin with one.
        const std::size_t taken =
            copyClamped(bytes, kBleMaxManufacturerData, device.getManufacturerData());
        if (taken > 0) {
            std::memcpy(slot.manufacturerData, bytes, taken);
        }
        slot.manufacturerLength = static_cast<uint8_t>(taken);
    }

    queueHead_ = next;
}

Esp32BleCentral::Subscription* Esp32BleCentral::findSubscription(
    const BleCharacteristicRef& ref) {
    for (uint8_t index = 0; index < kMaxSubscriptions; ++index) {
        if (subscriptions_[index].live && subscriptions_[index].ref == ref) {
            return &subscriptions_[index];
        }
    }
    return nullptr;
}

BLERemoteCharacteristic* Esp32BleCentral::locate(const BleCharacteristicRef& ref) {
    if (client_ == nullptr || !client_->isConnected()) {
        return nullptr;
    }
    if (!ref.valid()) {
        return nullptr;
    }

    // Resolved by comparing seam UUIDs rather than by handing the library a
    // BLEUUID to look up. The library keys its service and characteristic maps on
    // `BLEUUID::toString()`, so a 16-bit UUID and its 128-bit SIG-base expansion
    // only match there because of a formatting convention this adapter should not
    // depend on -- and the seam's own `BleUuid::fromShort()` produces exactly the
    // 128-bit form, so a peer that declares 0x2A00 as 16 bits and one that
    // declares it as 128 bits both compare equal here.
    std::map<std::string, BLERemoteService*>* services = client_->getServices();
    if (services == nullptr) {
        return nullptr;
    }
    for (auto& entry : *services) {
        BLERemoteService* service = entry.second;
        if (service == nullptr || seamUuid(service->getUUID()) != ref.service) {
            continue;
        }
        service->getCharacteristics();
        std::map<uint16_t, BLERemoteCharacteristic*>* characteristics =
            service->getCharacteristicsByHandle();
        if (characteristics == nullptr) {
            return nullptr;
        }
        for (auto& characteristicEntry : *characteristics) {
            BLERemoteCharacteristic* characteristic = characteristicEntry.second;
            if (characteristic != nullptr &&
                seamUuid(characteristic->getUUID()) == ref.characteristic) {
                return characteristic;
            }
        }
        // The service is here but the characteristic is not. Returning now says
        // so rather than scanning unrelated services for a coincidental match.
        return nullptr;
    }
    return nullptr;
}

void Esp32BleCentral::detachSubscription(Subscription& slot) {
    if (slot.characteristic != nullptr) {
        // registerForNotify() with a null callback is the unregister path on
        // every version: 3.x Bluedroid writes 0x0000 to the CCCD directly, and
        // 3.x NimBLE forwards it to unsubscribe() which does the same. The
        // NimBLE-only subscribe()/unsubscribe() pair would not compile on the
        // other two, so the common entry point is the one used.
        slot.characteristic->registerForNotify(nullptr, /*notifications=*/true,
                                               /*descriptorRequiresRegistration=*/true);
    }
    slot.characteristic = nullptr;
    slot.callback = nullptr;
    slot.context = nullptr;
    slot.live = false;
}

void Esp32BleCentral::releaseSubscriptions() {
    for (uint8_t index = 0; index < kMaxSubscriptions; ++index) {
        if (subscriptions_[index].live) {
            detachSubscription(subscriptions_[index]);
        }
    }
}

BleStatus Esp32BleCentral::fail(BleStatus status, const char* text) {
    lastError_ = text;
    return status;
}

#else  // no Arduino BLE host on this platform (the host build, and any target
       // whose lib package was built without one)

// The host build compiles src/drivers/, so this file has to exist there too. The
// adapter is Unsupported rather than absent, which keeps the seam's question
// ("what can this transport do?") answerable on any platform.

bool Esp32BleCentral::begin() {
    lastError_ = "no BLE controller on this platform";
    return false;
}

void Esp32BleCentral::end() {
    ready_ = false;
    connected_ = false;
}

bool Esp32BleCentral::ready() const {
    return false;
}

const char* Esp32BleCentral::name() const {
    return "ESP32 BLE";
}

BleCentralCaps Esp32BleCentral::caps() const {
    return BleCentralCaps{};
}

BleStatus Esp32BleCentral::startScan(uint32_t, bool) {
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::stopScan() {
    return BleStatus::Unsupported;
}

bool Esp32BleCentral::scanning() const {
    return false;
}

uint8_t Esp32BleCentral::scanResults(BleAdvertisement*, uint8_t) {
    return 0;
}

BleStatus Esp32BleCentral::connect(const BleAddress&, uint32_t) {
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::disconnect() {
    return BleStatus::Unsupported;
}

bool Esp32BleCentral::connected() const {
    return false;
}

BleStatus Esp32BleCentral::discover(BleDiscoveredPeer& out) {
    out = BleDiscoveredPeer{};
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::read(const BleCharacteristicRef&, uint8_t*, uint16_t& length,
                                uint32_t) {
    length = 0;
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::write(const BleCharacteristicRef&, const uint8_t*, uint16_t,
                                 bool) {
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::subscribe(const BleCharacteristicRef&, BleNotifyFn, void*) {
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

BleStatus Esp32BleCentral::unsubscribe(const BleCharacteristicRef&) {
    lastError_ = "no BLE controller on this platform";
    return BleStatus::Unsupported;
}

uint8_t Esp32BleCentral::subscriptionCount() const {
    return 0;
}

uint16_t Esp32BleCentral::negotiatedMtu() const {
    return 23;
}

BleStatus Esp32BleCentral::fail(BleStatus status, const char* text) {
    lastError_ = text;
    return status;
}

#endif
