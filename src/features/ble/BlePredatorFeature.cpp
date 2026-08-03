#include "features/ble/BlePredatorModel.h"

#include "core/FeatureRunner.h"

#ifdef ARDUINO
#include <BLEDevice.h>
#endif

namespace {
class BlePredatorFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        model_.advance(BlePredatorPhase::Scan);
        return {FeatureResultCode::Completed, "BLE Predator scan phase ready"};
    }

    void tick() override {}

    void requestStop(StopReason) override { model_.releasePhase(); }

    FeatureResult end() override {
        model_.releasePhase();
#ifdef ARDUINO
        BLEDevice::deinit(false);
#endif
        return {FeatureResultCode::Completed, "BLE resources released with deinit(false)"};
    }

private:
    BlePredatorModel model_;
};

}

IFeature* createBlePredatorFeature() {
    return new BlePredatorFeature();
}
