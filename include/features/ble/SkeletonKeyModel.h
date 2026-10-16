#pragma once

// SkeletonKey — generic BLE GATT explorer.
//
// Upstream v3.7.0 shipped SkeletonKey: connect to any BLE device, map every
// service and characteristic, read values, send commands, with one-tap profiles
// for common gear (ELK-BLEDOM / Triones RGB controllers) and automatic device
// type identification in the scan list. v3.9.1 deepened the dumps (real
// characteristic names, full UUIDs, every property including Indicate, a detail
// line per characteristic) and added the COLOR MIX screen. v3.9.5 added full
// GATT dumps to the SD card.
//
// The model here is the part that can be verified without a radio: the session
// state machine, the GATT tree accounting, the dump policy and the colour
// mixer. GATT name resolution lives in features/ble/GattNameTable.h, which the
// on-screen tree and the SD dump both go through; nothing here talks to a
// device.

#include <cstdint>
#include <string>
#include <vector>

class IFeature;

enum class SkeletonState : uint8_t {
    Idle,
    Scanning,
    Connecting,
    Discovering,
    Ready,
    Sending,
    Failed,
};

enum class SkeletonDeviceKind : uint8_t {
    Unknown,
    Remote,
    Watch,
    Sensor,
    Light,
    Keyboard,
    Mouse,
    Gamepad,
    Tracker,
};

struct SkeletonDevice {
    std::string address;
    std::string name;
    int rssi = -127;
    SkeletonDeviceKind kind = SkeletonDeviceKind::Unknown;
    uint16_t appearance = 0;
};

struct SkeletonCharacteristic {
    std::string uuid;
    std::string name;
    uint8_t properties = 0;
    std::string lastValue;  // hex, empty until read
};

struct SkeletonService {
    std::string uuid;
    std::string name;
    std::vector<SkeletonCharacteristic> characteristics;
    // True when this service offered more characteristics than the discovery
    // array could hold. Kept per service rather than folded into the tree flag
    // so the panel can mark the row that is actually short, instead of
    // declaring the whole peer partial.
    bool truncated = false;
};

struct SkeletonTree {
    std::vector<SkeletonService> services;
    // True when the peer offered more services or characteristics than the
    // discovery arrays could hold. A partial tree presented as complete is what
    // this flag exists to prevent: it must never reach the SD card as a full
    // dump.
    bool truncated = false;
    uint16_t characteristicCount() const;
    uint16_t readableCount() const;
    uint16_t writableCount() const;
    uint16_t notifyCount() const;
};

struct SkeletonColor {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

class SkeletonKey {
public:
    // Property bits as they appear in a GATT characteristic declaration.
    static constexpr uint8_t kPropertyBroadcast = 0x01;
    static constexpr uint8_t kPropertyRead = 0x02;
    static constexpr uint8_t kPropertyWriteWithoutResponse = 0x04;
    static constexpr uint8_t kPropertyWrite = 0x08;
    static constexpr uint8_t kPropertyNotify = 0x10;
    static constexpr uint8_t kPropertyIndicate = 0x20;
    static constexpr uint8_t kPropertyAuthWrite = 0x40;
    static constexpr uint8_t kPropertyExtended = 0x80;

    // GAP appearance values this build recognises. Anything else stays Unknown
    // rather than being guessed at.
    static SkeletonDeviceKind classifyAppearance(uint16_t appearance);
    static const char* kindLabel(SkeletonDeviceKind kind);

    // Profile templates. v3.9.1's COLOR MIX replaced "four presets only" with a
    // live blend, so the mixer is the primary path and the presets are helpers.
    static SkeletonColor presetWhite();
    static SkeletonColor presetWarm();
    static SkeletonColor presetCool();
    // Three-byte RGB payload for an ELK-BLEDOM / Triones style controller.
    static std::vector<uint8_t> colorPayload(const SkeletonColor& color);
    // Blends two colours; weight is 0..255 for ``to``.
    static SkeletonColor blend(const SkeletonColor& from, const SkeletonColor& to, uint8_t weight);

    // Fills service and characteristic names in from the UUID tables. A name the
    // device itself supplied (a GATT User Description) always wins and is left
    // alone; an unknown UUID is left empty so the caller prints the raw UUID.
    // Idempotent, so it is safe to re-run after every re-discovery.
    static void applyNames(SkeletonTree& tree);
};

// Session state machine plus dump policy. A full dump is written to the card
// only when the tree discovery completed, so a truncated tree is never saved as
// if it were complete.
class SkeletonSession {
public:
    bool begin(bool userConfirmed);
    void requestStop();
    void stop();
    void onScanStarted();
    void onScanComplete(uint16_t deviceCount);
    void onConnectStarted();
    void onConnectFailed(const char* reason);
    // A discovery that came back empty or refused is its own failure: reporting
    // it through onConnectFailed would leave the state machine's error text
    // blaming a connection that in fact succeeded.
    void onDiscoverFailed(const char* reason);
    void onDiscoverComplete(const SkeletonTree& tree);
    void onSendComplete();

    SkeletonState state() const { return state_; }
    const SkeletonTree& tree() const { return tree_; }
    const char* lastError() const { return lastError_; }
    uint16_t deviceCount() const { return deviceCount_; }
    bool discoveryComplete() const { return discoveryComplete_; }
    // True when the tree can be written to SD as a full dump.
    bool canWriteFullDump() const;

private:
    SkeletonState state_ = SkeletonState::Idle;
    SkeletonTree tree_;
    const char* lastError_ = "";
    uint16_t deviceCount_ = 0;
    bool discoveryComplete_ = false;
};

// Short human label for the state, used by the header bar.
const char* skeletonStateLabel(SkeletonState state);

IFeature* createSkeletonKeyFeature();
