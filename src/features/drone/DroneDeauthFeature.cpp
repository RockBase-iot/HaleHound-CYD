#include "features/drone/RemoteIdParser.h"

#include "core/FeatureRunner.h"

namespace {
class DroneDeauthFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Blocked, "Drone deauthentication requires explicit offensive policy authorization"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "Drone deauth stopped"}; }
};
}

IFeature* createDroneDeauthFeature() {
    return new DroneDeauthFeature();
}
