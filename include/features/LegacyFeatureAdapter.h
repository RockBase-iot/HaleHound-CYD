#pragma once

#include "core/FeatureRunner.h"

struct LegacyCallbacks {
    void (*setup)();
    void (*loop)();
    bool (*isExitRequested)();
    void (*cleanup)();
};

class LegacyFeatureAdapter final : public IFeature {
public:
    explicit LegacyFeatureAdapter(LegacyCallbacks callbacks)
        : callbacks_(callbacks) {}

    FeatureResult begin(FeatureContext& context) override;
    void tick() override;
    void requestStop(StopReason reason) override;
    FeatureResult end() override;
    bool exitRequested() const;

private:
    LegacyCallbacks callbacks_{};
    bool started_ = false;
    bool cleanedUp_ = false;
};
