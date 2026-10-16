#pragma once

// The one Arduino-dependent file in the BLE stack, and the only piece of it the
// host suites cannot execute. Everything above the seam -- the GATT tree model,
// SkeletonKey's session, Chameleon Ultra's framing, the HID reader's diagnostic
// -- runs against `FakeBleCentral` on the bench, so this file is
// compile-verified and bench-verified but never host-tested. Stated, not
// implied.
//
// Which BT host is underneath. This repository has two build environments and
// they are built on *different* Espressif Bluetooth hosts:
//
//   nm-cyd-c5  ESP32-C5, Arduino 3.3.6, **NimBLE**. Not a preference -- the C5
//              library package
//              (framework-arduinoespressif32-libs/esp32c5/.../sdkconfig.h)
//              ships `CONFIG_BT_NIMBLE_ENABLED=1` and no Bluedroid at all, so
//              NimBLE is the only host this chip can be built with here.
//
//   esp32-cyd  ESP32, Arduino 2.0.17, **Bluedroid**. `sdkconfig.h` defines
//              `CONFIG_BLUEDROID_ENABLED`. This is the host the repository's
//              legacy BLE screens drive.
//
// The Arduino BLE library presents one API over both, which is why a single
// adapter can serve both. `Esp32BleCentral.cpp` carries the normalisation for
// the four places they genuinely differ -- UUID internals, address byte order,
// the address-type constants, and three method signatures -- and nothing above
// this file needs to care.
//
// The cost is honest and belongs in the ledger: Bluedroid is the heavier host
// (tens of KB of RAM more than NimBLE) and the C5 does not get the choice.
// `FeatureSwitchLedger` is the place that cost shows up, which is why B5 exists.
//
// Four API facts this adapter is built around, all read from the framework
// headers actually installed for this project rather than from documentation
// (.pio/c5-core/packages/framework-arduinoespressif32 = Arduino 3.3.6, NimBLE,
// for the `nm-cyd-c5` env; .pio/packages/framework-arduinoespressif32 =
// Arduino 2.0.17, Bluedroid, for the `esp32-cyd` env). All three must compile.
//
//   1. `BLERemoteCharacteristic` exposes no `getProperties()` in any of them.
//      The property byte the seam carries is therefore *synthesised* from the
//      six `canXxx()` predicates, which is why the seam's property bits mirror
//      the GATT declaration bit for bit.
//
//   2. `BLEAdvertisedDeviceCallbacks::onResult()` takes the device **by
//      value** and is called from the host's own task. The scan drain is built
//      on a fixed ring filled by that callback, never on the framework's
//      `BLEScanResults` list -- that list has no cursor, and re-reading it would
//      reproduce exactly the never-terminating drain the host suite pins
//      against.
//
//   3. `notify_callback` is a `std::function` in all of them, so the
//      notification bridge can be an ordinary capturing lambda. It must be
//      re-registered rather than assumed across reconnects, which is what
//      `releaseSubscriptions()` enforces on every teardown path.
//
//   4. The peer address is stored in *printed* order and the two hosts disagree
//      about which way round they hand it back. NimBLE keeps it
//      least-significant byte first (its own `BLEAddress::toString()` prints
//      `m_address[5]` down to `m_address[0]`, which is the proof); Bluedroid
//      keeps printed order. `Esp32BleCentral.cpp` reverses on the way out for
//      NimBLE only. A straight `memcpy`, which is what this file did before the
//      stack was identified, silently swaps every byte of every address on the
//      C5 -- and an address that is wrong in all six bytes is an address that
//      connects to nothing, or worse, to something.
//
// Provenance and the full API diff: docs/protocol-evidence/ble-central-transport.md.

#include "drivers/IBleCentral.h"

#ifdef ARDUINO
// Pulled in explicitly rather than relied on transitively: the whole adapter is
// conditioned on CONFIG_NIMBLE_ENABLED / CONFIG_BLUEDROID_ENABLED from here.
#include <sdkconfig.h>
#include <Arduino.h>
#include <BLEAddress.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteCharacteristic.h>
#include <BLERemoteService.h>
#include <BLEScan.h>
#include <esp_arduino_version.h>
#endif

class Esp32BleCentral final : public IBleCentral {
public:
    // Concurrent notifications. Bluedroid allows far more, NimBLE fewer, but a
    // firmware that is holding more than a handful of subscriptions at once is
    // doing something this seam does not model; the overflow is counted rather
    // than silently dropped.
    static constexpr uint8_t kMaxSubscriptions = 8;

    // Advertisements buffered between drains. One is sacrificed to the
    // full/empty distinction, so the usable depth is one less.
    static constexpr uint8_t kAdvQueueDepth = 8;

    // 517 is the ATT maximum; a peer that refuses it leaves us at its own value,
    // which `discover()` reports honestly rather than assuming 517.
    static constexpr uint16_t kPreferredMtu = 517;

    bool begin() override;
    void end() override;
    bool ready() const override;

    const char* name() const override;
    BleCentralCaps caps() const override;

    BleStatus startScan(uint32_t durationMs, bool active) override;
    BleStatus stopScan() override;
    bool scanning() const override;

    uint8_t scanResults(BleAdvertisement* out, uint8_t capacity) override;

    BleStatus connect(const BleAddress& address, uint32_t timeoutMs) override;
    BleStatus disconnect() override;
    bool connected() const override;

    BleStatus discover(BleDiscoveredPeer& out) override;

    BleStatus read(const BleCharacteristicRef& ref, uint8_t* out, uint16_t& length,
                   uint32_t timeoutMs) override;
    BleStatus write(const BleCharacteristicRef& ref, const uint8_t* data, uint16_t length,
                    bool withResponse) override;

    BleStatus subscribe(const BleCharacteristicRef& ref, BleNotifyFn callback,
                        void* context) override;
    BleStatus unsubscribe(const BleCharacteristicRef& ref) override;
    uint8_t subscriptionCount() const override;

    const char* lastError() const override;

    // MTU actually negotiated on the live link, or 23 before one exists. Exposed
    // so the B5 ledger can see a link that silently stayed at the default.
    uint16_t negotiatedMtu() const;

    // Advertisements the ring had to drop because nothing drained it. A
    // non-zero value means a consumer stalled; it is a number rather than a
    // silence so a UI freeze is diagnosable instead of mysterious.
    uint32_t droppedAdvertisements() const { return advDropped_; }

    // subscribe() calls that found no free slot.
    uint32_t subscriptionOverflows() const { return subOverflow_; }

private:
#ifdef ARDUINO
    // The scan sink. A nested class rather than a file-scope one because C++11
    // gives a nested class access to its enclosing class's privates, which is
    // what lets onResult() put an advertisement straight into the ring without
    // a friend declaration or a global.
    class AdvertisementSink final : public BLEAdvertisedDeviceCallbacks {
    public:
        explicit AdvertisementSink(Esp32BleCentral& owner) : owner_(owner) {}
        void onResult(BLEAdvertisedDevice advertisedDevice) override;

    private:
        Esp32BleCentral& owner_;
    };

    struct Subscription {
        BleCharacteristicRef ref;
        BLERemoteCharacteristic* characteristic = nullptr;
        BleNotifyFn callback = nullptr;
        void* context = nullptr;
        bool live = false;
    };

    // Stack-task entry point: copies into the ring, allocates nothing.
    void acceptAdvertisement(BLEAdvertisedDevice& device);
    Subscription* findSubscription(const BleCharacteristicRef& ref);
    // Resolves a ref against the live link by comparing seam UUIDs, never by
    // asking the library to look one up by string. The library keys its own maps
    // on `BLEUUID::toString()`, and a 16-bit UUID and its 128-bit SIG-base
    // expansion are only the same string because of a formatting convention this
    // adapter should not depend on.
    BLERemoteCharacteristic* locate(const BleCharacteristicRef& ref);
    // Detaches one slot from the peer. Extracted because end(), disconnect()
    // and unsubscribe() must all take the same route: a slot marked free
    // without the CCCD being written would leave the peer notifying a callback
    // that no longer exists.
    void detachSubscription(Subscription& slot);
    void releaseSubscriptions();

    // Whether the host still has a scan running. NimBLE's `BLEScan` answers
    // this itself; neither Bluedroid version has `isScanning()`, so there the
    // mirror below is the answer. Note that this is *not* a version test --
    // `isScanning()` is declared under `CONFIG_NIMBLE_ENABLED` in the 3.x
    // header, so a 3.x Bluedroid build does not have it either.
    bool stackScanning() const;

    // `void (*)(BLEScanResults)` carries no user context -- the framework's own
    // signature -- so the instance is reached through a file-scope pointer that
    // begin() installs. There is exactly one BLE controller, so one instance is
    // the honest model, and begin() refuses a second one rather than letting the
    // pointer be quietly repointed.
    static void onScanComplete(BLEScanResults results);

    AdvertisementSink advSink_{*this};
    BLEScan* scan_ = nullptr;
    BLEClient* client_ = nullptr;
    Subscription subscriptions_[kMaxSubscriptions];

    // Single producer (the host's task) against a single consumer (whoever
    // calls scanResults()). Same handoff the legacy sniffer screens already use
    // for the same reason. The indices are bytes so a torn read is impossible,
    // and `scanning_` is written from both tasks for the same cause.
    BleAdvertisement queue_[kAdvQueueDepth];
    volatile uint8_t queueHead_ = 0;
    volatile uint8_t queueTail_ = 0;
    volatile bool scanning_ = false;

    BleAddress peerAddress_;
    uint16_t mtu_ = 23;
#endif

    // Counters rather than silence: a stalled scan consumer and a full
    // subscription table are both diagnosable facts. They sit outside the
    // platform guard so the accessors above have something to read on any
    // build.
    uint32_t advDropped_ = 0;
    uint32_t subOverflow_ = 0;

    BleStatus fail(BleStatus status, const char* text);

    bool ready_ = false;
    bool connected_ = false;
    const char* lastError_ = "not started";
};
