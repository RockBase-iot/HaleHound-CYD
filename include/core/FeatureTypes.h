#pragma once

#include <cstdint>

enum class RiskClass : uint8_t {
    Passive,
    Defensive,
    Offensive,
    SensitiveData,
};

enum class FeatureResultCode : uint8_t {
    Completed,
    Cancelled,
    Blocked,
    NotFound,
    Busy,
    Fault,
    Unsupported,
};

enum class StopReason : uint8_t {
    UserRequest,
    Navigation,
    PolicyBlocked,
    StartFailure,
    Fault,
    Shutdown,
};

enum class FeatureState : uint8_t {
    Idle,
    Preflight,
    Starting,
    Running,
    Stopping,
    Failed,
};

struct FeatureResult {
    FeatureResultCode code;
    const char* detail;
};
