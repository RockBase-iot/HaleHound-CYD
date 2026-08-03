#include "drivers/Cc1101Driver.h"

namespace {
Cc1101Driver* activeDriver = nullptr;

bool readVersion(ISpiBackend& backend) {
    if (activeDriver == nullptr) {
        return false;
    }
    backend.transfer(static_cast<uint8_t>(0x31U | 0xC0U));
    activeDriver->lastVersion = backend.transfer(0x00U);
    return true;
}
}

ProbeResult Cc1101Driver::probe(const ExpansionRoute& route,
                                uint32_t timeoutMs) {
    if (route.transport != Transport::Spi || spi_ == nullptr) {
        return {PeripheralStatus::Unsupported, "CC1101 SPI service unavailable"};
    }
    activeDriver = this;
    const SpiResult result = spi_->transact(SpiDeviceId::Cc1101,
                                            readVersion,
                                            timeoutMs);
    activeDriver = nullptr;
    if (result.code == SpiResultCode::Busy || result.code == SpiResultCode::Timeout) {
        return {PeripheralStatus::Busy, "shared SPI busy"};
    }
    if (result.code != SpiResultCode::Completed) {
        return {PeripheralStatus::Fault, "CC1101 SPI probe failed"};
    }
    if (lastVersion == 0x00U || lastVersion == 0xFFU) {
        return {PeripheralStatus::NotFound, "CC1101 VERSION not responding"};
    }
    return {PeripheralStatus::Available, "CC1101 VERSION responded"};
}
