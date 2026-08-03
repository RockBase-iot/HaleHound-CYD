#include "drivers/Rf433RxDriver.h"

#include "features/rf433/Rf433Feature.h"

ProbeResult Rf433RxDriver::probe(const ExpansionRoute& route, uint32_t) {
    if (route.transport != Transport::Gpio || route.controlA < 0) {
        return {PeripheralStatus::Unsupported, "RF433 RX GPIO route unavailable"};
    }
    return confirmed_ ? ProbeResult{PeripheralStatus::Available, "RF433 RX loopback confirmed"}
                      : ProbeResult{PeripheralStatus::NotFound, "RF433 RX requires loopback or manual confirmation"};
}

bool Rf433RxDriver::receive(Rf433Signal& signal) const {
    return confirmed_ && Rf433Codec::valid(signal);
}
