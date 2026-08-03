#include "services/DisplayService.h"

bool DisplayService::renderRegion(SpiTransactionCallback callback, uint32_t timeoutMs) {
    return spi_.transact(SpiDeviceId::Display, callback, timeoutMs).code ==
           SpiResultCode::Completed;
}
