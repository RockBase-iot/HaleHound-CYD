#include "features/rf433/Rf433Feature.h"

#include "core/FeatureRunner.h"

namespace {
class Rf433Feature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Completed, "RF433 OOK/ASK feature ready; GPIO confirmation required"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "RF433 feature stopped"}; }
};
}

IFeature* createRf433Feature() {
    return new Rf433Feature();
}
