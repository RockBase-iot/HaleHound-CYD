#include "drivers/Nrf24Driver.h"

namespace {
Nrf24Driver* activeDriver = nullptr;

bool readRegisters(ISpiBackend& backend) {
    if (activeDriver == nullptr) {
        return false;
    }
    backend.transfer(0x00U);
    activeDriver->config = backend.transfer(0x00U);
    backend.deselect(SpiDeviceId::Nrf24);
    backend.select(SpiDeviceId::Nrf24);
    backend.transfer(0x07U);
    activeDriver->status = backend.transfer(0x00U);
    return true;
}
}

ProbeResult Nrf24Driver::probe(const ExpansionRoute& route,
                               uint32_t timeoutMs) {
    if (route.transport != Transport::Spi || spi_ == nullptr) {
        return {PeripheralStatus::Unsupported, "nRF24 SPI service unavailable"};
    }
    activeDriver = this;
    const SpiResult result = spi_->transact(SpiDeviceId::Nrf24,
                                            readRegisters,
                                            timeoutMs);
    activeDriver = nullptr;
    if (result.code == SpiResultCode::Busy || result.code == SpiResultCode::Timeout) {
        return {PeripheralStatus::Busy, "shared SPI busy"};
    }
    if (result.code != SpiResultCode::Completed) {
        return {PeripheralStatus::Fault, "nRF24 SPI probe failed"};
    }
    if (config == 0x00U || config == 0xFFU || status == 0xFFU) {
        return {PeripheralStatus::NotFound, "nRF24 registers not responding"};
    }
    return {PeripheralStatus::Available, "nRF24 CONFIG/STATUS responded"};
}
