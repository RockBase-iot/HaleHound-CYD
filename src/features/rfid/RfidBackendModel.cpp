#include "features/rfid/RfidBackendModel.h"

#include "core/FeatureRunner.h"

namespace {

// PN532 over I2C as wired on both board profiles in this repository.
constexpr RfidBackendCapability kPn532 = {
    true,   // mifareClassic
    true,   // mifareUltralight
    true,   // ntag
    true,   // isoDep
    false,  // iclass     - not supported by the PN532 silicon
    false,  // magicGen1A - no backdoor write path in this driver
    true,   // nestedAuthentication
    true,   // keyedWrite
    true,   // flipperNfcExport
    true,   // proxmarkEmlExport
    false,  // hfEmulation - PN532 card emulation is not wired up here
    false,  // lfSupport  - the NM-RF-HAT routes LF to the Chameleon, not the PN532
    false,  // mfkey32NonceCapture
};

// Chameleon Ultra reached over Bluetooth.
constexpr RfidBackendCapability kChameleonUltra = {
    true,   // mifareClassic
    true,   // mifareUltralight
    true,   // ntag
    true,   // isoDep
    false,  // iclass
    true,   // magicGen1A
    true,   // nestedAuthentication
    true,   // keyedWrite
    false,  // flipperNfcExport - dumps come back over BLE, exported by the host
    false,  // proxmarkEmlExport
    true,   // hfEmulation
    true,   // lfSupport
    true,   // mfkey32NonceCapture
};

// ST25R3916 as added in v4.0.
constexpr RfidBackendCapability kSt25r3916 = {
    true,   // mifareClassic
    true,   // mifareUltralight
    true,   // ntag
    true,   // isoDep
    true,   // iclass
    true,   // magicGen1A
    true,   // nestedAuthentication
    true,   // keyedWrite
    true,   // flipperNfcExport
    true,   // proxmarkEmlExport
    false,  // hfEmulation
    false,  // lfSupport - the ST25R3916 is a 13.56 MHz part
    false,  // mfkey32NonceCapture
};

const RfidScreen kScreens[RfidBackendModel::kScreenCount] = {
    RfidScreen::BackendPicker,
    RfidScreen::CardScanner,
    RfidScreen::CardReader,
    RfidScreen::CardDump,
    RfidScreen::KeyCheck,
    RfidScreen::CardWrite,
    RfidScreen::Guide,
};

}  // namespace

RfidBackendCapability RfidBackendModel::capability(RfidBackend backend) {
    switch (backend) {
        case RfidBackend::ChameleonUltra:
            return kChameleonUltra;
        case RfidBackend::St25r3916:
            return kSt25r3916;
        case RfidBackend::Pn532:
        default:
            return kPn532;
    }
}

const char* RfidBackendModel::backendLabel(RfidBackend backend) {
    switch (backend) {
        case RfidBackend::ChameleonUltra:
            return "Chameleon Ultra";
        case RfidBackend::St25r3916:
            return "ST25R3916";
        case RfidBackend::Pn532:
        default:
            return "PN-532";
    }
}

uint8_t RfidBackendModel::backendCount() {
    return 3;
}

RfidBackend RfidBackendModel::backendAt(uint8_t index) {
    switch (index) {
        case 1:
            return RfidBackend::ChameleonUltra;
        case 2:
            return RfidBackend::St25r3916;
        case 0:
        default:
            return RfidBackend::Pn532;
    }
}

RfidScreen RfidBackendModel::screenAt(uint8_t index) {
    if (index >= kScreenCount) {
        return RfidScreen::BackendPicker;
    }
    return kScreens[index];
}

const char* RfidBackendModel::screenLabel(RfidScreen screen) {
    switch (screen) {
        // "Reader" here and again on CardReader meant the header bar could not
        // tell the back-end picker from the card reader. Labels are the only
        // thing distinguishing seven screens, so they have to be unique.
        case RfidScreen::BackendPicker:
            return "Backend";
        case RfidScreen::CardScanner:
            return "Scanner";
        case RfidScreen::CardReader:
            return "Reader";
        case RfidScreen::CardDump:
            return "Dump";
        case RfidScreen::KeyCheck:
            return "Key Check";
        case RfidScreen::CardWrite:
            return "Write";
        case RfidScreen::Guide:
        default:
            return "Guide";
    }
}

bool RfidBackendModel::screenAvailable(RfidBackend backend, RfidScreen screen) {
    const RfidBackendCapability caps = capability(backend);
    switch (screen) {
        case RfidScreen::BackendPicker:
        case RfidScreen::Guide:
            return true;
        case RfidScreen::CardScanner:
        case RfidScreen::CardReader:
            return caps.mifareClassic || caps.ntag || caps.mifareUltralight;
        case RfidScreen::CardDump:
            return caps.mifareClassic || caps.ntag;
        case RfidScreen::KeyCheck:
            return caps.nestedAuthentication || caps.magicGen1A || caps.mfkey32NonceCapture;
        case RfidScreen::CardWrite:
            return caps.keyedWrite || caps.magicGen1A;
        default:
            return false;
    }
}

bool RfidBackendModel::canExportDump(RfidBackend backend, bool isMifareClassic) {
    const RfidBackendCapability caps = capability(backend);
    if (isMifareClassic) {
        return caps.mifareClassic && caps.flipperNfcExport && caps.proxmarkEmlExport;
    }
    return caps.ntag && caps.flipperNfcExport;
}

namespace {

class RfidBackendFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Completed, "RFID reader picker ready"};
    }

    void tick() override {}

    void requestStop(StopReason) override {}

    FeatureResult end() override {
        return {FeatureResultCode::Completed, "RFID back end released"};
    }
};

}  // namespace

IFeature* createRfidBackendFeature() {
    return new RfidBackendFeature();
}
