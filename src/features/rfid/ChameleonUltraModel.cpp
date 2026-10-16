#include "features/rfid/ChameleonUltraModel.h"

#include "core/FeatureRunner.h"

const char* ChameleonUltraModel::slotKindLabel(ChameleonSlotKind kind) {
    switch (kind) {
        case ChameleonSlotKind::MifareClassic1K:
            return "MIFARE Classic 1K";
        case ChameleonSlotKind::MifareClassic4K:
            return "MIFARE Classic 4K";
        case ChameleonSlotKind::MifareUltralight:
            return "MIFARE Ultralight";
        case ChameleonSlotKind::Ntag21x:
            return "NTAG 21x";
        case ChameleonSlotKind::Iso15693:
            return "ISO 15693";
        case ChameleonSlotKind::Em4100:
            return "EM4100 (LF)";
        case ChameleonSlotKind::HidProx:
            return "HID Prox (LF)";
        case ChameleonSlotKind::Unknown:
            return "Unrecognised";
        case ChameleonSlotKind::Empty:
        default:
            return "Empty";
    }
}

const char* ChameleonUltraModel::stateLabel(ChameleonState state) {
    switch (state) {
        case ChameleonState::Scanning:
            return "SCANNING";
        case ChameleonState::Connecting:
            return "CONNECTING";
        case ChameleonState::Ready:
            return "READY";
        case ChameleonState::Busy:
            return "BUSY";
        case ChameleonState::Failed:
            return "FAILED";
        case ChameleonState::Idle:
        default:
            return "IDLE";
    }
}

ChameleonReadResult ChameleonUltraModel::evaluateRead(uint8_t pagesRead, uint8_t pagesTotal) {
    ChameleonReadResult result;
    result.pagesRead = pagesRead;
    result.pagesTotal = pagesTotal;
    if (pagesTotal == 0) {
        result.detail = "no pages expected";
        return result;
    }
    if (pagesRead == 0) {
        result.detail = "tag did not answer";
        return result;
    }
    if (pagesRead > pagesTotal) {
        // More pages than the card has means the driver miscounted, not that the
        // read went well. Reporting "complete read" here would put a confident
        // label on a number that cannot be true.
        result.detail = "page count inconsistent";
        return result;
    }
    result.partial = pagesRead < pagesTotal;
    result.valid = true;
    result.detail = result.partial ? "partial read" : "complete read";
    return result;
}

Mfkey32Progress ChameleonUltraModel::recordNonce(Mfkey32Progress progress) {
    if (progress.nonces < kChameleonNoncesRequired) {
        ++progress.nonces;
    }
    progress.complete = progress.nonces >= kChameleonNoncesRequired;
    return progress;
}

Mfkey32Progress ChameleonUltraModel::resetProgress() {
    return Mfkey32Progress();
}

void ChameleonSession::begin() {
    state_ = ChameleonState::Scanning;
    address_.clear();
    lastError_ = "";
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        slots_[index].index = index;
        slots_[index].kind = ChameleonSlotKind::Empty;
        slots_[index].nickname.clear();
        slots_[index].loaded = false;
        slots_[index].enabled = false;
    }
}

void ChameleonSession::stop() {
    state_ = ChameleonState::Idle;
    address_.clear();
}

bool ChameleonSession::onDeviceFound(const std::string& address) {
    if (address.empty()) {
        // A found device with no address is not something to connect to; adopting
        // it would put the session in Ready with nothing to talk to.
        return false;
    }
    if (state_ == ChameleonState::Ready || state_ == ChameleonState::Busy) {
        // Already holding a unit; a second one does not take over.
        return false;
    }
    address_ = address;
    state_ = ChameleonState::Ready;
    lastError_ = "";
    return true;
}

void ChameleonSession::onConnectFailed(const char* reason) {
    state_ = ChameleonState::Failed;
    lastError_ = reason == NULL ? "chameleon connect failed" : reason;
}

bool ChameleonSession::setSlot(uint8_t index, ChameleonSlotKind kind, const std::string& nickname) {
    if (index >= kChameleonSlotCount) {
        lastError_ = "slot index out of range";
        return false;
    }
    slots_[index].index = index;
    slots_[index].kind = kind;
    slots_[index].nickname = nickname;
    slots_[index].loaded = kind != ChameleonSlotKind::Empty;
    slots_[index].enabled = kind != ChameleonSlotKind::Empty;
    return true;
}

bool ChameleonSession::clearSlot(uint8_t index) {
    if (index >= kChameleonSlotCount) {
        lastError_ = "slot index out of range";
        return false;
    }
    slots_[index].kind = ChameleonSlotKind::Empty;
    slots_[index].nickname.clear();
    slots_[index].loaded = false;
    slots_[index].enabled = false;
    return true;
}

const ChameleonSlot& ChameleonSession::slot(uint8_t index) const {
    static const ChameleonSlot kInvalid;
    if (index >= kChameleonSlotCount) {
        return kInvalid;
    }
    return slots_[index];
}

uint8_t ChameleonSession::loadedSlotCount() const {
    uint8_t count = 0;
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        if (slots_[index].loaded) {
            ++count;
        }
    }
    return count;
}

uint8_t ChameleonSession::firstFreeSlot() const {
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        if (!slots_[index].loaded) {
            return index;
        }
    }
    return 0xFF;
}

namespace {

class ChameleonUltraFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        session_.begin();
        return {FeatureResultCode::Completed, "Chameleon Ultra scanning"};
    }

    void tick() override {}

    void requestStop(StopReason) override { session_.stop(); }

    FeatureResult end() override {
        session_.stop();
        return {FeatureResultCode::Completed, "Chameleon Ultra released"};
    }

private:
    ChameleonSession session_;
};

}  // namespace

IFeature* createChameleonUltraFeature() {
    return new ChameleonUltraFeature();
}
