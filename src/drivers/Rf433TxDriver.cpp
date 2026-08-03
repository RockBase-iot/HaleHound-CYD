#include "drivers/Rf433TxDriver.h"

#include "features/rf433/Rf433Feature.h"

ProbeResult Rf433TxDriver::probe(const ExpansionRoute& route, uint32_t) {
    if (route.transport != Transport::Gpio || route.controlA < 0) {
        return {PeripheralStatus::Unsupported, "RF433 TX GPIO route unavailable"};
    }
    return confirmed_ ? ProbeResult{PeripheralStatus::Available, "RF433 TX loopback confirmed"}
                      : ProbeResult{PeripheralStatus::NotFound, "RF433 TX requires loopback or manual confirmation"};
}

bool Rf433TxDriver::send(const Rf433Signal& signal, bool loopbackConfirmed) const {
    return confirmed_ && loopbackConfirmed && Rf433Codec::valid(signal) && signal.transmitAuthorized;
}
