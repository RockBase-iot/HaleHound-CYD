#pragma once

#include <cstddef>

#include "features/FeatureCatalog.h"

enum class NavigationOutcome : uint8_t {
    Pushed,
    Blocked,
    Returned,
    Fault,
    Empty,
};

class NavigationController {
public:
    NavigationOutcome push(FeatureId id);
    NavigationOutcome complete(const FeatureResult& result);
    NavigationOutcome back();

    bool hasCurrent() const { return depth_ != 0; }
    FeatureId current() const { return stack_[depth_ - 1]; }
    size_t depth() const { return depth_; }
    const char* lastError() const { return lastError_; }

private:
    static constexpr size_t kMaxDepth = 8;
    FeatureId stack_[kMaxDepth]{};
    size_t depth_ = 0;
    const char* lastError_ = "";
};
