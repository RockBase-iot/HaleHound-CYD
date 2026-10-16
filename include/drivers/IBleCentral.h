#pragma once

// BLE central (client) transport: the radio, not the protocol.
//
// Same split as the RFID side. Everything above this line -- the GATT tree
// model, the ELK-BLEDOM colour protocol, the Chameleon Ultra command framing,
// HID reader diagnostics -- is protocol code and lives under src/features/ble/,
// so the host suites drive a session against a scripted fake instead of
// requiring a real peripheral on the bench.
//
// One implementation ships:
//
//   Esp32BleCentral   Arduino-ESP32 BLE library, central role. It is the only
//                     part that needs Arduino, and the only part the host
//                     suites cannot execute -- compile-verified only, which is
//                     stated rather than implied.
//
// Why the status enum separates "the peer said no" from "the link failed": a
// GATT write rejected by access permissions is a fact about the device, while a
// write that never reached it is a fact about the link. Collapsing the two makes
// a dead connection look like a protected characteristic -- the same mistake the
// RFID status enum exists to prevent, where a dead bus was reported as a card
// guarding its key.
//
// Why UUIDs are 128-bit even though most profiles are 16-bit: the Chameleon
// Ultra speaks Nordic's UART service at 6E400001-B5A3-F393-E0A9-E50E24DCCA9E.
// Nordic's own documentation notes that this is "16-bit offset 0x0001", and
// that is a trap -- 0x0001 is not that service, it is a different one, and a
// client built on the short form would silently address the wrong
// characteristic. The seam therefore carries the whole value and derives the
// short form only where the UUID really is on the SIG base.

#include <array>
#include <cstddef>
#include <cstdint>

enum class BleStatus : uint8_t {
    Ok,
    NotFound,      // no such peer in the scan results
    NoService,     // connected, but the service or characteristic is absent
    NotPermitted,  // the peer rejected it: access denied, or auth required
    TooLong,       // value longer than the negotiated MTU allows
    ConnectFailed,
    Disconnected,
    Timeout,
    Busy,          // a scan or connect is already in progress
    IoError,       // stack-level failure: no controller, allocation refused
    Unsupported,   // this transport cannot do it
};

// Human label for the honest-report line. Never returns nullptr.
const char* bleStatusLabel(BleStatus status);

enum class BleAddressType : uint8_t {
    Public,
    Random,
    Unknown,
};

// A BLE UUID, stored as its full 128 bits in printed order.
//
// The SIG base is `0000xxxx-0000-1000-8000-00805f9b34fb`: a UUID is expressible
// as 16 bits exactly when its bytes match that shape, which `isShort()` checks
// rather than assumes. Everything else -- Nordic's UART service, Apple's,
// Google's -- stays 128-bit.
struct BleUuid {
    std::array<uint8_t, 16> bytes{};

    // Expands a SIG short UUID to its base form.
    static BleUuid fromShort(uint16_t value);
    // From 16 bytes already in printed (big-endian) order.
    static BleUuid fromBytes(const uint8_t* sixteen);

    // False for an all-zero UUID, which is what an unset struct holds and is
    // never a real service.
    bool valid() const;
    // True when this UUID sits on the SIG base and so has a 16-bit form.
    bool isShort() const;
    // The 16-bit value. Meaningful only when isShort(); 0 otherwise, so a caller
    // that skips the check gets the nameless-UUID answer instead of a wrong one.
    uint16_t shortValue() const;

    // Canonical `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`: 36 characters plus a
    // terminator. False when the buffer is short, leaving `out` empty rather
    // than truncated -- a half-printed UUID is worse than none.
    bool write(char* out, std::size_t capacity) const;

    bool operator==(const BleUuid& other) const;
    bool operator!=(const BleUuid& other) const;
};

// A BLE address kept as bytes rather than a string, so a session can compare and
// order peers without re-parsing, and so the address *type* is never lost. On
// air the LSB goes first; `bytes[0]` is the MSB as every tool prints it, and
// `write()` is the only place that ordering is decided.
struct BleAddress {
    std::array<uint8_t, 6> bytes{};
    BleAddressType type = BleAddressType::Unknown;

    // False for an all-zero address, which is what an unfilled struct looks
    // like and is never a real peer.
    bool valid() const;

    // "AA:BB:CC:DD:EE:FF" -- 17 characters plus a terminator. False when the
    // buffer is short, leaving `out` empty rather than truncated.
    bool write(char* out, std::size_t capacity) const;

    bool operator==(const BleAddress& other) const;
    bool operator!=(const BleAddress& other) const;
};

constexpr std::size_t kBleMaxAdvName = 32;
constexpr std::size_t kBleMaxAdvServices = 8;
constexpr std::size_t kBleMaxManufacturerData = 31;

// One scan result. Fixed-size and POD: a scan list is allocated once, not grown
// per advertisement, because the BLE callback runs on the stack's own task and
// must not allocate.
struct BleAdvertisement {
    BleAddress address;
    int8_t rssi = -127;
    char name[kBleMaxAdvName + 1] = {};
    uint16_t appearance = 0;
    // Advertised service UUIDs. A peripheral advertising more than this many is
    // reported with `servicesTruncated` set rather than silently cut short.
    BleUuid serviceUuids[kBleMaxAdvServices] = {};
    uint8_t serviceCount = 0;
    bool servicesTruncated = false;
    uint8_t manufacturerData[kBleMaxManufacturerData] = {};
    uint8_t manufacturerLength = 0;
    // False for a non-connectable beacon: connecting would only time out, so the
    // session must not offer it.
    bool connectable = false;

    // True when the advertisement lists `uuid`, in either form.
    bool advertises(const BleUuid& uuid) const;
};

// A characteristic, addressed the way the stack addresses it.
struct BleCharacteristicRef {
    BleUuid service;
    BleUuid characteristic;

    // Convenience for the SIG-base profiles, which are the common case.
    static BleCharacteristicRef fromShort(uint16_t service, uint16_t characteristic);

    bool valid() const;
    bool operator==(const BleCharacteristicRef& other) const;
    bool operator!=(const BleCharacteristicRef& other) const;
};

// Property bits exactly as they appear in a GATT characteristic declaration.
// Duplicated from SkeletonKey's constants on purpose: this layer is below that
// one and must not depend on it.
enum : uint8_t {
    kBlePropBroadcast = 0x01,
    kBlePropRead = 0x02,
    kBlePropWriteNoResponse = 0x04,
    kBlePropWrite = 0x08,
    kBlePropNotify = 0x10,
    kBlePropIndicate = 0x20,
};

// Discovery output, sized so a whole peer fits in one struct on a device with
// 320 KB of RAM. A peer with more services than this sets `truncated`, and the
// tree model must show that rather than presenting a partial tree as complete.
constexpr uint8_t kBleMaxServicesPerPeer = 16;
constexpr uint8_t kBleMaxCharsPerService = 16;

struct BleDiscoveredCharacteristic {
    BleUuid uuid;
    uint8_t properties = 0;
};

struct BleDiscoveredService {
    BleUuid uuid;
    BleDiscoveredCharacteristic characteristics[kBleMaxCharsPerService];
    uint8_t characteristicCount = 0;
    bool truncated = false;
};

struct BleDiscoveredPeer {
    BleAddress address;
    BleDiscoveredService services[kBleMaxServicesPerPeer];
    uint8_t serviceCount = 0;
    bool truncated = false;
    // Negotiated ATT MTU. The default 23 yields a 20-byte payload, which is what
    // a write must be split to unless the peer agreed to more.
    uint16_t mtu = 23;

    uint16_t payloadLimit() const { return mtu > 3 ? static_cast<uint16_t>(mtu - 3) : 20; }
};

// Notification sink. Invoked synchronously from the stack's own task, so an
// implementation must not allocate and the receiving session must not block.
// `context` is whatever the session passed to subscribe().
using BleNotifyFn = void (*)(void* context, const BleCharacteristicRef& ref,
                             const uint8_t* data, uint16_t length);

struct BleCentralCaps {
    bool central;
    bool observer;  // can scan without connecting
    bool passiveScan;
    bool extendedAdv;
    bool writeWithoutResponse;
    bool notify;
    bool indicate;
    bool bonding;
};

class IBleCentral {
public:
    virtual ~IBleCentral() = default;

    // Bring the controller up. False only when there is no usable controller at
    // all; a controller that answers but cannot, say, bond says so in caps().
    virtual bool begin() = 0;
    // Release the controller. Every session's end() must reach this, or the next
    // BLE feature starts on top of a live stack -- the white-screen cause the
    // FeatureSwitchLedger models.
    virtual void end() = 0;
    virtual bool ready() const = 0;

    // Short name for the header bar, e.g. "ESP32 BLE". Never nullptr.
    virtual const char* name() const = 0;
    virtual BleCentralCaps caps() const = 0;

    // Scanning. `durationMs` of 0 means "until stopScan()". `active` requests
    // scan responses, which most peers only answer to when connectable.
    virtual BleStatus startScan(uint32_t durationMs, bool active) = 0;
    virtual BleStatus stopScan() = 0;
    virtual bool scanning() const = 0;

    // Copies up to `capacity` results into `out` and returns how many were
    // written. A session can poll this every frame without the list growing.
    virtual uint8_t scanResults(BleAdvertisement* out, uint8_t capacity) const = 0;

    virtual BleStatus connect(const BleAddress& address, uint32_t timeoutMs) = 0;
    virtual BleStatus disconnect() = 0;
    virtual bool connected() const = 0;

    // Service and characteristic discovery for the connected peer. `out` is
    // overwritten; a partial tree sets `truncated`.
    virtual BleStatus discover(BleDiscoveredPeer& out) = 0;

    // `length` is in/out: in = capacity of `out`, out = bytes returned. A reply
    // that does not fit is BleStatus::TooLong with `length` set to the size
    // needed, never a silent short read.
    virtual BleStatus read(const BleCharacteristicRef& ref, uint8_t* out,
                           uint16_t& length, uint32_t timeoutMs) = 0;

    virtual BleStatus write(const BleCharacteristicRef& ref, const uint8_t* data,
                            uint16_t length, bool withResponse) = 0;

    virtual BleStatus subscribe(const BleCharacteristicRef& ref, BleNotifyFn callback,
                               void* context) = 0;
    virtual BleStatus unsubscribe(const BleCharacteristicRef& ref) = 0;
    // Live subscriptions, so a teardown can be checked instead of assumed.
    virtual uint8_t subscriptionCount() const = 0;

    // Last failure in words, for the honest-report line. Never nullptr.
    virtual const char* lastError() const = 0;
};
