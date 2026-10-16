#pragma once

// SkeletonKey over a real BLE controller.
//
// SkeletonKeyModel is the state machine, the GATT tree accounting and the dump
// policy; it has no radio. This is the other half: the part that drives an
// IBleCentral through a session and converts what comes back into the model's
// types, so the screen and the SD dump are fed by the same tree.
//
// The split matters because the conversion is where a client goes quietly
// wrong. A UUID that loses its 128-bit form, a truncated discovery presented as
// complete, a read that reports zero bytes because the buffer was one short --
// each of those produces a tree that looks fine and is not. Every one of them
// is a decision here, and every one is pinned by test_skeleton_key_client.

#include <cstdint>
#include <string>
#include <vector>

#include "drivers/IBleCentral.h"
#include "features/ble/SkeletonKeyModel.h"

class SkeletonKeyClient {
public:
    // The scan list is capped rather than grown without bound, and a drain gives
    // up after this many pages. Both bounds exist for the same reason: a poll
    // loop that trusts the transport to eventually say "nothing left" hangs the
    // UI the first time a transport does not.
    static constexpr std::size_t kBleMaxScanList = 32;
    static constexpr uint8_t kMaxScanPages = 32;

    explicit SkeletonKeyClient(IBleCentral& central) : central_(central) {}

    // Whether the radio came up. A controller that refuses to start reports
    // false here rather than failing later at scan time.
    bool begin();

    // Authorise and enter the session. Mirrors SkeletonSession::begin, which
    // refuses an unauthorised start instead of scanning anyway.
    bool beginSession(bool userConfirmed);

    // Scan, collect every connectable advertisement the controller offers, and
    // stop. Returns the transport status; the collected count is deviceCount().
    BleStatus scan(bool active);

    // Poll-style scan for the UI: start it, then drain on each frame.
    BleStatus beginScan(bool active);
    uint8_t drainScanResults();
    BleStatus endScan();

    const std::vector<SkeletonDevice>& devices() const { return devices_; }
    uint16_t deviceCount() const { return static_cast<uint16_t>(devices_.size()); }
    // The address behind `devices_[index]`. The model keeps peers as printable
    // text, so the client retains the bytes alongside them: re-parsing the
    // printed form would lose the address type, and the type is part of the
    // identity.
    bool addressAt(uint16_t index, BleAddress& out) const;

    BleStatus connect(const BleAddress& address, uint32_t timeoutMs);
    // Adopt the strongest connectable device already in the scan list. Returns
    // NotFound when the list is empty, rather than connecting to nothing.
    BleStatus connectToStrongest(uint32_t timeoutMs);
    bool connected() const { return central_.connected(); }

    // Discover the connected peer, build the tree and resolve its names.
    // `out` is only assigned on success: a failed discovery must not leave a
    // half-built tree for the screen to render.
    BleStatus discover(SkeletonTree& out);

    // Read a characteristic into `hexOut` as upper-case hex. A reply larger than
    // the first buffer is retried at the size the transport reported, so a
    // 24-byte value on a 20-byte MTU comes back whole instead of short.
    BleStatus read(const BleCharacteristicRef& ref, std::string& hexOut,
                   uint32_t timeoutMs);

    // Read straight onto a tree entry's `lastValue`, so the panel and a dump
    // taken afterwards agree. False when the indices are out of range.
    BleStatus readIntoTree(SkeletonTree& tree, uint16_t serviceIndex,
                           uint16_t characteristicIndex, uint32_t timeoutMs);

    BleStatus write(const BleCharacteristicRef& ref, const uint8_t* data,
                    uint16_t length, bool withResponse);
    BleStatus writeToTree(const SkeletonTree& tree, uint16_t serviceIndex,
                          uint16_t characteristicIndex, const uint8_t* data,
                          uint16_t length, bool withResponse);

    BleStatus subscribe(const BleCharacteristicRef& ref, BleNotifyFn callback,
                        void* context);
    BleStatus unsubscribe(const BleCharacteristicRef& ref);

    // Release the controller and return the session to Idle. The transport's own
    // accessors are the proof: after this, connected() and subscriptionCount()
    // are what a teardown test reads.
    BleStatus end();

    const SkeletonSession& session() const { return session_; }
    // True only when the last discovery produced a complete, named tree -- which
    // is exactly what SkeletonSession::canWriteFullDump checks.
    bool canWriteFullDump() const { return session_.canWriteFullDump(); }
    const char* lastError() const { return central_.lastError(); }

    // The tree from the most recent successful discovery.
    const SkeletonTree& tree() const { return tree_; }

    // ── conversions, exposed because they are where the honesty lives ──────

    // Canonical `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, or "" when the UUID is
    // unset. A UUID is never silently shortened to its 16-bit offset.
    static std::string uuidText(const BleUuid& uuid);
    // Fills a scan result. A device with no name keeps an empty name rather
    // than being labelled something the advertisement did not say.
    static SkeletonDevice toDevice(const BleAdvertisement& advertisement);
    // Converts a discovery result into the model's tree. Names are resolved
    // here, not at render time, so the screen and the SD dump agree by
    // construction.
    static void buildTree(const BleDiscoveredPeer& peer, SkeletonTree& out);

private:
    IBleCentral& central_;
    SkeletonSession session_;
    SkeletonTree tree_;
    std::vector<SkeletonDevice> devices_;
    // Same indices as `devices_`, so the UI can hand back what it selected.
    std::vector<BleAddress> addresses_;
    // The connected peer's negotiated payload size, learned at discovery. A read
    // starts at this size and retries at the size the peer reports, so a value
    // longer than the MTU comes back whole instead of short.
    uint16_t payloadLimit_ = 20;
};
