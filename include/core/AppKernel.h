#pragma once

#include "core/FeatureRunner.h"
#include "core/BootHealthService.h"
#include "core/NavigationController.h"
#include "features/LegacyFeatureAdapter.h"

class AppKernel {
public:
    AppKernel();
    explicit AppKernel(LegacyCallbacks callbacks);

    FeatureResult setup();
    void tick();
    bool isReady() const { return ready_; }
    bool safeMode() const { return context_.safeMode; }

    NavigationController& navigation() { return navigation_; }
    FeatureRunner& runner() { return runner_; }

private:
    FeatureContext context_{};
    LegacyFeatureAdapter legacyAdapter_;
    FeatureRunner runner_;
    NavigationController navigation_;
    bool ready_ = false;
};
