#include "features/subghz/SubReadFeature.h"

#include "core/FeatureRunner.h"
#include "features/subghz/SubFileParser.h"

namespace {
class SubReadFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        stopped_ = false;
        return {FeatureResultCode::Completed, "sub file reader ready"};
    }

    void tick() override {}

    void requestStop(StopReason) override { stopped_ = true; }

    FeatureResult end() override {
        stopped_ = true;
        return {FeatureResultCode::Completed, "sub file reader stopped"};
    }

private:
    bool stopped_ = true;
};
}

IFeature* createSubReadFeature() {
    return new SubReadFeature();
}
