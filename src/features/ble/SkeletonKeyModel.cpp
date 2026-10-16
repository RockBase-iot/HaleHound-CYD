#include "features/ble/SkeletonKeyModel.h"

#include "core/FeatureRunner.h"
#include "features/ble/GattNameTable.h"

uint16_t SkeletonTree::characteristicCount() const {
    uint16_t total = 0;
    for (const SkeletonService& service : services) {
        total = static_cast<uint16_t>(total + service.characteristics.size());
    }
    return total;
}

uint16_t SkeletonTree::readableCount() const {
    uint16_t total = 0;
    for (const SkeletonService& service : services) {
        for (const SkeletonCharacteristic& characteristic : service.characteristics) {
            if ((characteristic.properties & (SkeletonKey::kPropertyRead |
                                              SkeletonKey::kPropertyNotify |
                                              SkeletonKey::kPropertyIndicate)) != 0) {
                ++total;
            }
        }
    }
    return total;
}

uint16_t SkeletonTree::writableCount() const {
    uint16_t total = 0;
    for (const SkeletonService& service : services) {
        for (const SkeletonCharacteristic& characteristic : service.characteristics) {
            if ((characteristic.properties & (SkeletonKey::kPropertyWrite |
                                              SkeletonKey::kPropertyWriteWithoutResponse)) != 0) {
                ++total;
            }
        }
    }
    return total;
}

uint16_t SkeletonTree::notifyCount() const {
    uint16_t total = 0;
    for (const SkeletonService& service : services) {
        for (const SkeletonCharacteristic& characteristic : service.characteristics) {
            if ((characteristic.properties &
                 (SkeletonKey::kPropertyNotify | SkeletonKey::kPropertyIndicate)) != 0) {
                ++total;
            }
        }
    }
    return total;
}

SkeletonDeviceKind SkeletonKey::classifyAppearance(uint16_t appearance) {
    // Bluetooth SIG appearance values. Only the categories this build is sure of
    // are mapped; everything else falls through to Unknown so the scan list
    // never invents a device type.
    switch (appearance) {
        case 0x0041:  // Watch: Sports Watch
        case 0x0042:  // Watch: Smart Watch
            return SkeletonDeviceKind::Watch;
        case 0x0180:  // Remote Control
        case 0x0181:
            return SkeletonDeviceKind::Remote;
        case 0x0309:  // Thermometer
            return SkeletonDeviceKind::Sensor;
        case 0x03C1:  // HID: Keyboard
            return SkeletonDeviceKind::Keyboard;
        case 0x03C2:  // HID: Mouse
            return SkeletonDeviceKind::Mouse;
        case 0x03C4:  // HID: Gamepad
            return SkeletonDeviceKind::Gamepad;
        default:
            break;
    }
    // 0x03C3 (Joystick) and 0x03C5 (Digitizer/Tablet) live in this block but
    // have no kind of their own in the enum. Reporting them as Keyboard -- which
    // is what a blanket 0x03C0..0x03C5 range would do -- is precisely the
    // invented device type this function exists to avoid, so they stay Unknown.
    if (appearance >= 0x0C40 && appearance <= 0x0C4F) {
        return SkeletonDeviceKind::Light;
    }
    return SkeletonDeviceKind::Unknown;
}

const char* SkeletonKey::kindLabel(SkeletonDeviceKind kind) {
    switch (kind) {
        case SkeletonDeviceKind::Remote:
            return "Remote";
        case SkeletonDeviceKind::Watch:
            return "Watch";
        case SkeletonDeviceKind::Sensor:
            return "Sensor";
        case SkeletonDeviceKind::Light:
            return "Light";
        case SkeletonDeviceKind::Keyboard:
            return "Keyboard";
        case SkeletonDeviceKind::Mouse:
            return "Mouse";
        case SkeletonDeviceKind::Gamepad:
            return "Gamepad";
        case SkeletonDeviceKind::Tracker:
            return "Tracker";
        case SkeletonDeviceKind::Unknown:
        default:
            return "Unknown";
    }
}

SkeletonColor SkeletonKey::presetWhite() {
    SkeletonColor color;
    color.red = 255;
    color.green = 255;
    color.blue = 255;
    return color;
}

SkeletonColor SkeletonKey::presetWarm() {
    SkeletonColor color;
    color.red = 255;
    color.green = 180;
    color.blue = 110;
    return color;
}

SkeletonColor SkeletonKey::presetCool() {
    SkeletonColor color;
    color.red = 140;
    color.green = 190;
    color.blue = 255;
    return color;
}

std::vector<uint8_t> SkeletonKey::colorPayload(const SkeletonColor& color) {
    std::vector<uint8_t> payload;
    payload.push_back(color.red);
    payload.push_back(color.green);
    payload.push_back(color.blue);
    return payload;
}

SkeletonColor SkeletonKey::blend(const SkeletonColor& from, const SkeletonColor& to, uint8_t weight) {
    SkeletonColor mixed;
    // Integer blend so the on-device sliders and the host test agree exactly.
    const uint16_t weight16 = weight;
    const uint16_t inverse = static_cast<uint16_t>(255 - weight16);
    mixed.red = static_cast<uint8_t>((from.red * inverse + to.red * weight16) / 255);
    mixed.green = static_cast<uint8_t>((from.green * inverse + to.green * weight16) / 255);
    mixed.blue = static_cast<uint8_t>((from.blue * inverse + to.blue * weight16) / 255);
    return mixed;
}

void SkeletonKey::applyNames(SkeletonTree& tree) {
    for (SkeletonService& service : tree.services) {
        if (service.name.empty()) {
            service.name = GattNameTable::resolve(service.uuid, GattUuidKind::Service);
        }
        for (SkeletonCharacteristic& characteristic : service.characteristics) {
            if (characteristic.name.empty()) {
                characteristic.name =
                    GattNameTable::resolve(characteristic.uuid, GattUuidKind::Characteristic);
            }
        }
    }
}

const char* skeletonStateLabel(SkeletonState state) {
    switch (state) {
        case SkeletonState::Idle:
            return "IDLE";
        case SkeletonState::Scanning:
            return "SCANNING";
        case SkeletonState::Connecting:
            return "CONNECTING";
        case SkeletonState::Discovering:
            return "DISCOVERING";
        case SkeletonState::Ready:
            return "READY";
        case SkeletonState::Sending:
            return "SENDING";
        case SkeletonState::Failed:
        default:
            return "FAILED";
    }
}

bool SkeletonSession::begin(bool userConfirmed) {
    if (!userConfirmed) {
        lastError_ = "not authorised";
        return false;
    }
    state_ = SkeletonState::Scanning;
    tree_.services.clear();
    discoveryComplete_ = false;
    deviceCount_ = 0;
    lastError_ = "";
    return true;
}

void SkeletonSession::requestStop() {
    if (state_ != SkeletonState::Idle) {
        state_ = SkeletonState::Idle;
    }
}

void SkeletonSession::stop() {
    state_ = SkeletonState::Idle;
    tree_.services.clear();
    discoveryComplete_ = false;
}

void SkeletonSession::onScanStarted() {
    state_ = SkeletonState::Scanning;
    lastError_ = "";
}

void SkeletonSession::onScanComplete(uint16_t deviceCount) {
    deviceCount_ = deviceCount;
    state_ = deviceCount == 0 ? SkeletonState::Failed : SkeletonState::Scanning;
    if (deviceCount == 0) {
        lastError_ = "no devices found";
    }
}

void SkeletonSession::onConnectStarted() {
    state_ = SkeletonState::Connecting;
}

void SkeletonSession::onConnectFailed(const char* reason) {
    state_ = SkeletonState::Failed;
    lastError_ = reason == NULL ? "connect failed" : reason;
}

void SkeletonSession::onDiscoverFailed(const char* reason) {
    state_ = SkeletonState::Failed;
    tree_.services.clear();
    discoveryComplete_ = false;
    lastError_ = reason == NULL ? "discovery failed" : reason;
}

void SkeletonSession::onDiscoverComplete(const SkeletonTree& tree) {
    tree_ = tree;
    // Resolve names as the tree lands, not at render time: the SD dump and the
    // on-screen tree then agree by construction.
    SkeletonKey::applyNames(tree_);
    discoveryComplete_ = true;
    state_ = tree_.services.empty() ? SkeletonState::Failed : SkeletonState::Ready;
    if (tree_.services.empty()) {
        lastError_ = "no services discovered";
    }
}

void SkeletonSession::onSendComplete() {
    state_ = discoveryComplete_ ? SkeletonState::Ready : SkeletonState::Failed;
}

bool SkeletonSession::canWriteFullDump() const {
    // A dump needs a *live, fully discovered* session. Checking the tree alone
    // is not enough: requestStop() leaves the tree in place while moving to
    // Idle, so a stopped session would otherwise still claim it could write.
    //
    // And "fully discovered" has to mean the whole tree: a discovery that ran
    // out of room mid-peer is a partial tree, so it is refused rather than
    // written to the card under the name "full dump".
    return state_ == SkeletonState::Ready && discoveryComplete_ &&
           !tree_.services.empty() && !tree_.truncated;
}

namespace {

class SkeletonKeyFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        authorised_ = session_.begin(true);
        return authorised_
                   ? FeatureResult{FeatureResultCode::Completed, "SkeletonKey ready"}
                   : FeatureResult{FeatureResultCode::Blocked, session_.lastError()};
    }

    void tick() override {}

    void requestStop(StopReason) override { session_.requestStop(); }

    FeatureResult end() override {
        session_.stop();
        return {FeatureResultCode::Completed, "SkeletonKey closed"};
    }

private:
    SkeletonSession session_;
    bool authorised_ = false;
};

}  // namespace

IFeature* createSkeletonKeyFeature() {
    return new SkeletonKeyFeature();
}
