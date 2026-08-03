#include "platform/ExecutionPolicy.h"

ExecutionMode chooseC5Mode(const WorkloadSpec&) {
    return ExecutionMode::CooperativeTick;
}
