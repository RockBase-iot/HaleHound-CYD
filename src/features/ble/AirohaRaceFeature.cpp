#include "features/ble/BlePredatorModel.h"

#include "core/FeatureRunner.h"

namespace {
class AirohaRaceFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Unsupported,
                "Airoha Race evidence and test vectors are incomplete"};
    }

    void tick() override {}
    void requestStop(StopReason) override {}

    FeatureResult end() override {
        return {FeatureResultCode::Completed, "Airoha Race stopped"};
    }
};
}

IFeature* createAirohaRaceFeature() {
    return new AirohaRaceFeature();
}
