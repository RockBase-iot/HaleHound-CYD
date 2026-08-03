#pragma once

#include "core/FeatureTypes.h"

struct FeatureContext;

class IFeature {
public:
    virtual ~IFeature() = default;

    virtual FeatureResult begin(FeatureContext& context) = 0;
    virtual void tick() = 0;
    virtual void requestStop(StopReason reason) = 0;
    virtual FeatureResult end() = 0;
};
