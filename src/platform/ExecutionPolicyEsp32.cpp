#include "platform/ExecutionPolicy.h"

ExecutionMode chooseEsp32Mode(const WorkloadSpec& workload) {
    return workload.requiresBackgroundTask || workload.role == WorkloadRole::Radio
               ? ExecutionMode::PinnedTask
               : ExecutionMode::CooperativeTick;
}
