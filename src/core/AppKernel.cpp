#include "core/AppKernel.h"

#include "app/LegacyApp.h"
#include "features/FeatureCatalog.h"

namespace {
LegacyCallbacks defaultLegacyCallbacks() {
    return {legacySetup, legacyLoop, nullptr, nullptr};
}
}

AppKernel::AppKernel()
    : legacyAdapter_(defaultLegacyCallbacks()),
      runner_(featureCatalog(), context_, {}) {}

AppKernel::AppKernel(LegacyCallbacks callbacks)
    : legacyAdapter_(callbacks),
      runner_(featureCatalog(), context_, {}) {}

FeatureResult AppKernel::setup() {
    if (ready_) {
        return {FeatureResultCode::Completed, "already ready"};
    }
    globalBootHealthService().begin();
    globalBootHealthService().markPending();
    context_.safeMode = globalBootHealthService().safeMode();
    const FeatureResult result = legacyAdapter_.begin(context_);
    ready_ = result.code == FeatureResultCode::Completed;
    if (ready_) {
        globalBootHealthService().markHealthy();
    }
    return result;
}

void AppKernel::tick() {
    if (!ready_) {
        return;
    }
    runner_.tick();
    legacyAdapter_.tick();
}
