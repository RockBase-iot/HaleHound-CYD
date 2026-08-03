#include "features/rfid/NtagModel.h"

#include "core/FeatureRunner.h"

namespace {
class NtagToolsFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        stopped_ = false;
        return {FeatureResultCode::Completed, "NTAG tools ready; PN532 I2C is required for I/O"};
    }

    void tick() override {}
    void requestStop(StopReason) override { stopped_ = true; }
    FeatureResult end() override {
        stopped_ = true;
        return {FeatureResultCode::Completed, "NTAG tools stopped"};
    }

private:
    bool stopped_ = true;
};
}

IFeature* createNtagToolsFeature() {
    return new NtagToolsFeature();
}
