#include "drivers/IrTxDriver.h"

#include "features/ir/IrFeature.h"

ProbeResult IrTxDriver::probe(const ExpansionRoute& route, uint32_t) {
    if (route.transport != Transport::Gpio || route.controlA < 0) {
        return {PeripheralStatus::Unsupported, "IR TX GPIO route unavailable"};
    }
    return confirmed_ ? ProbeResult{PeripheralStatus::Available, "IR TX loopback confirmed"}
                      : ProbeResult{PeripheralStatus::NotFound, "IR TX requires loopback or manual confirmation"};
}

bool IrTxDriver::send(const IrFrame& frame, bool loopbackConfirmed) const {
    return confirmed_ && loopbackConfirmed && IrCodec::valid(frame);
}
