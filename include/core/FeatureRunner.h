#pragma once

#include "core/FeatureRegistry.h"
#include "core/IFeature.h"

struct FeatureContext {
    bool safeMode = false;
};

struct FeatureRunnerHooks {
    bool (*authorize)(const FeatureDescriptor& descriptor);
    bool (*routeSupported)(const FeatureDescriptor& descriptor);
    bool (*probePeripheral)(PeripheralId peripheral);
    bool (*preflightResources)(const FeatureDescriptor& descriptor);
};

class FeatureRunner {
public:
    FeatureRunner(const FeatureRegistry& registry,
                  FeatureContext& context,
                  FeatureRunnerHooks hooks);
    ~FeatureRunner();

    FeatureResult start(FeatureId id);
    void tick();
    void requestStop(StopReason reason);
    FeatureState state() const { return state_; }

private:
    FeatureResult finish(FeatureResult result);
    void releaseFeature();

    const FeatureRegistry& registry_;
    FeatureContext& context_;
    FeatureRunnerHooks hooks_;
    IFeature* feature_ = nullptr;
    FeatureState state_ = FeatureState::Idle;
    bool endCalled_ = false;
};
