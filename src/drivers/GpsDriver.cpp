#include "drivers/GpsDriver.h"

ProbeResult GpsDriver::probe(const ExpansionRoute& route, uint32_t timeoutMs) {
    if (route.transport != Transport::Uart || source_ == nullptr) {
        return {PeripheralStatus::Unsupported, "GPS UART source unavailable"};
    }
    const size_t maxBytes = timeoutMs == 0U ? 32U : 128U;
    size_t inspected = 0;
    bool sawSentence = false;
    while (source_->available() > 0U && inspected++ < maxBytes) {
        const int value = source_->read();
        if (value == '$') {
            sawSentence = true;
        }
    }
    return sawSentence
               ? ProbeResult{PeripheralStatus::Available, "GPS NMEA sentence detected"}
               : ProbeResult{PeripheralStatus::NotFound, "GPS NMEA data not found"};
}
