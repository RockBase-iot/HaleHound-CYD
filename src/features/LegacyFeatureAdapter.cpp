#include "features/LegacyFeatureAdapter.h"

FeatureResult LegacyFeatureAdapter::begin(FeatureContext&) {
    if (started_) {
        return {FeatureResultCode::Completed, "already started"};
    }
    if (callbacks_.setup != nullptr) {
        callbacks_.setup();
    }
    started_ = true;
    cleanedUp_ = false;
    return {FeatureResultCode::Completed, "started"};
}

void LegacyFeatureAdapter::tick() {
    if (started_ && !cleanedUp_ && callbacks_.loop != nullptr) {
        callbacks_.loop();
    }
}

void LegacyFeatureAdapter::requestStop(StopReason) {
    end();
}

FeatureResult LegacyFeatureAdapter::end() {
    if (!started_ || cleanedUp_) {
        return {FeatureResultCode::Completed, "already ended"};
    }
    if (callbacks_.cleanup != nullptr) {
        callbacks_.cleanup();
    }
    cleanedUp_ = true;
    started_ = false;
    return {FeatureResultCode::Completed, "ended"};
}

bool LegacyFeatureAdapter::exitRequested() const {
    return callbacks_.isExitRequested != nullptr && callbacks_.isExitRequested();
}
