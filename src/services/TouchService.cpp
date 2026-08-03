#include "services/TouchService.h"

bool TouchService::sample(SpiTransactionCallback callback, uint32_t timeoutMs) {
    return spi_.transact(SpiDeviceId::Touch, callback, timeoutMs).code ==
           SpiResultCode::Completed;
}
