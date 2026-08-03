#include "services/SpiBusService.h"

OwnerId SpiBusService::ownerFor(SpiDeviceId device) {
    switch (device) {
        case SpiDeviceId::Display:
            return OwnerId::Display;
        case SpiDeviceId::Touch:
            return OwnerId::Touch;
        case SpiDeviceId::Sd:
            return OwnerId::SdCard;
        case SpiDeviceId::Cc1101:
            return OwnerId::Cc1101;
        case SpiDeviceId::Nrf24:
            return OwnerId::Nrf24;
        case SpiDeviceId::Pn532:
            return OwnerId::Pn532;
    }
    return OwnerId::Feature;
}

SpiResult SpiBusService::transact(SpiDeviceId device,
                                  SpiTransactionCallback callback,
                                  uint32_t timeoutMs) {
    const SpiResult start = beginLegacySession(device, timeoutMs, true);
    if (start.code != SpiResultCode::Completed) {
        return start;
    }

    SpiResult result{SpiResultCode::Completed, "completed"};
    if (callback == nullptr) {
        result = {SpiResultCode::Invalid, "SPI callback missing"};
    } else if (!callback(backend_)) {
        result = {SpiResultCode::Fault, "SPI callback failed"};
    }

    const SpiResult finish = endLegacySession();
    if (result.code == SpiResultCode::Completed &&
        finish.code != SpiResultCode::Completed) {
        return finish;
    }
    return result;
}

SpiResult SpiBusService::beginLegacySession(SpiDeviceId device,
                                             uint32_t timeoutMs,
                                             bool manageTransaction) {
    if (legacyActive_) {
        return {SpiResultCode::Busy, "SPI session already active"};
    }

    legacyLease_ = broker_.tryAcquire(resourceFor(device), ownerFor(device), timeoutMs);
    if (!legacyLease_.acquired()) {
        return {legacyLease_.code() == AcquireCode::Timeout ? SpiResultCode::Timeout
                                                             : SpiResultCode::Busy,
                "shared SPI unavailable"};
    }

    if (!backend_.lock(timeoutMs)) {
        legacyLease_.release();
        return {SpiResultCode::Timeout, "SPI backend lock timeout"};
    }
    legacyLocked_ = true;
    backend_.deselectAll();
    if (manageTransaction && !backend_.beginTransaction(device)) {
        backend_.unlock();
        legacyLocked_ = false;
        legacyLease_.release();
        return {SpiResultCode::Fault, "SPI transaction setup failed"};
    }
    legacyTransactionStarted_ = manageTransaction;
    if (!backend_.select(device)) {
        if (legacyTransactionStarted_) {
            backend_.endTransaction();
        }
        backend_.unlock();
        legacyTransactionStarted_ = false;
        legacyLocked_ = false;
        legacyLease_.release();
        return {SpiResultCode::Fault, "SPI target select failed"};
    }

    legacyDevice_ = device;
    legacySelected_ = true;
    legacyActive_ = true;
    return {SpiResultCode::Completed, "started"};
}

SpiResult SpiBusService::endLegacySession() {
    if (!legacyActive_) {
        return {SpiResultCode::Invalid, "SPI session inactive"};
    }

    if (legacySelected_) {
        backend_.deselect(legacyDevice_);
        legacySelected_ = false;
    }
    if (legacyTransactionStarted_) {
        backend_.endTransaction();
        legacyTransactionStarted_ = false;
    }
    if (legacyLocked_) {
        backend_.unlock();
        legacyLocked_ = false;
    }
    legacyLease_.release();
    legacyActive_ = false;
    return {SpiResultCode::Completed, "ended"};
}
