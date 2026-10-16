#pragma once

#include "core/BootHealthService.h"
#include "core/FeatureRunner.h"
#include "core/NavigationController.h"
#include "features/LegacyFeatureAdapter.h"

class IFeatureCanvas;

class AppKernel {
public:
    AppKernel();
    explicit AppKernel(LegacyCallbacks callbacks);

    FeatureResult setup();
    void tick();
    bool isReady() const { return ready_; }
    bool safeMode() const { return context_.safeMode; }

    // Installs the surface features paint into. Null (the default) means
    // headless: the runner then never calls draw(), which is the correct
    // behaviour for the host suites and for a build with no display -- as
    // opposed to a canvas that pretends to accept pixels and discards them.
    void setCanvas(IFeatureCanvas* canvas) { context_.canvas = canvas; }

    NavigationController& navigation() { return navigation_; }
    FeatureRunner& runner() { return runner_; }

private:
    FeatureContext context_{};
    LegacyFeatureAdapter legacyAdapter_;
    FeatureRunner runner_;
    NavigationController navigation_;
    bool ready_ = false;
};

// The one kernel. Same shape as globalBootHealthService(): a function-local
// static behind an accessor.
//
// Not a convenience -- a correctness requirement. The kernel owns the policy
// instance, the registry and the runner; the legacy menu needs the runner to
// start anything, and if it built its own it would also build its own policy
// and its own settings store. The authorization gate is only a single point
// while there is exactly one kernel to reach it through.
AppKernel& globalAppKernel();
