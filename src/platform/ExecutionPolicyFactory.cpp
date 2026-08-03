#include "platform/ExecutionPolicy.h"

ExecutionMode chooseEsp32Mode(const WorkloadSpec& workload);
ExecutionMode chooseC5Mode(const WorkloadSpec& workload);

ExecutionMode chooseMode(BoardId board, const WorkloadSpec& workload) {
    return board == BoardId::NmCydC5 ? chooseC5Mode(workload)
                                    : chooseEsp32Mode(workload);
}
