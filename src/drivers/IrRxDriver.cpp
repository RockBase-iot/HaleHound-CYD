#include "drivers/IrRxDriver.h"

#include "features/ir/IrFeature.h"

ProbeResult IrRxDriver::probe(const ExpansionRoute& route, uint32_t) {
    if (route.transport != Transport::Gpio || route.controlA < 0) {
        return {PeripheralStatus::Unsupported, "IR RX GPIO route unavailable"};
    }
    return confirmed_ ? ProbeResult{PeripheralStatus::Available, "IR RX loopback confirmed"}
                      : ProbeResult{PeripheralStatus::NotFound, "IR RX requires loopback or manual confirmation"};
}

bool IrRxDriver::capture(IrFrame& frame) const {
    return confirmed_ && IrCodec::valid(frame);
}
