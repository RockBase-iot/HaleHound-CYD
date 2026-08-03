#pragma once

#include <cstdint>

#include "board/BoardProfile.h"

enum class WorkloadRole : uint8_t {
    Ui,
    Radio,
    Storage,
    Background,
};

enum class ExecutionMode : uint8_t {
    CooperativeTick,
    PinnedTask,
};

struct WorkloadSpec {
    WorkloadRole role;
    uint8_t priority;
    uint16_t maxSliceMs;
    uint32_t stackBytes;
    bool requiresBackgroundTask;
};

ExecutionMode chooseMode(BoardId board, const WorkloadSpec& workload);
