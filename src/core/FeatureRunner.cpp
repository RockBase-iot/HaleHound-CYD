#include "core/FeatureRunner.h"

namespace {
FeatureResult result(FeatureResultCode code, const char* detail) {
    return {code, detail};
}

bool allowed(bool (*callback)(const FeatureDescriptor&), const FeatureDescriptor& descriptor) {
    return callback == nullptr || callback(descriptor);
}
}

FeatureRunner::FeatureRunner(const FeatureRegistry& registry,
                             FeatureContext& context,
                             FeatureRunnerHooks hooks)
    : registry_(registry), context_(context), hooks_(hooks) {}

FeatureRunner::~FeatureRunner() {
    finish(result(FeatureResultCode::Cancelled, "runner destroyed"));
}

FeatureResult FeatureRunner::start(FeatureId id) {
    if (state_ != FeatureState::Idle && state_ != FeatureState::Failed) {
        return result(FeatureResultCode::Busy, "feature already running");
    }

    const auto* descriptor = registry_.find(id);
    if (descriptor == nullptr) {
        return result(FeatureResultCode::Unsupported, "feature not registered");
    }

    state_ = FeatureState::Preflight;
    if (!allowed(hooks_.authorize, *descriptor)) {
        return finish(result(FeatureResultCode::Blocked, "policy blocked"));
    }
    if (!allowed(hooks_.routeSupported, *descriptor)) {
        return finish(result(FeatureResultCode::Unsupported, "route unavailable"));
    }
    if (hooks_.probePeripheral != nullptr) {
        for (uint8_t index = 0; index < descriptor->requiredPeripheralCount; ++index) {
            if (!hooks_.probePeripheral(descriptor->requiredPeripherals[index])) {
                return finish(result(FeatureResultCode::NotFound, "required peripheral missing"));
            }
        }
    }
    if (!allowed(hooks_.preflightResources, *descriptor)) {
        return finish(result(FeatureResultCode::Busy, "resource preflight failed"));
    }

    if (descriptor->factory == nullptr) {
        return finish(result(FeatureResultCode::Unsupported, "feature implementation missing"));
    }

    state_ = FeatureState::Starting;
    feature_ = descriptor->factory();
    if (feature_ == nullptr) {
        return finish(result(FeatureResultCode::Fault, "feature allocation failed"));
    }
    endCalled_ = false;
    const FeatureResult beginResult = feature_->begin(context_);
    if (beginResult.code != FeatureResultCode::Completed) {
        return finish(beginResult);
    }
    state_ = FeatureState::Running;
    return beginResult;
}

void FeatureRunner::tick() {
    if (state_ == FeatureState::Running && feature_ != nullptr) {
        feature_->tick();
    }
}

void FeatureRunner::requestStop(StopReason reason) {
    if (feature_ != nullptr && state_ == FeatureState::Running) {
        state_ = FeatureState::Stopping;
        feature_->requestStop(reason);
    }
    finish(result(FeatureResultCode::Cancelled, "stopped"));
}

FeatureResult FeatureRunner::finish(FeatureResult resultValue) {
    if (feature_ != nullptr) {
        if (!endCalled_) {
            endCalled_ = true;
            feature_->end();
        }
        releaseFeature();
    }
    state_ = resultValue.code == FeatureResultCode::Completed ? FeatureState::Idle
                                                               : FeatureState::Failed;
    return resultValue;
}

void FeatureRunner::releaseFeature() {
    delete feature_;
    feature_ = nullptr;
    endCalled_ = false;
}
