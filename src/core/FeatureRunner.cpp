#include "core/FeatureRunner.h"

#include "core/IFeatureCanvas.h"

namespace {
FeatureResult result(FeatureResultCode code, const char* detail) {
    return {code, detail};
}

bool allowed(bool (*callback)(const FeatureDescriptor&), const FeatureDescriptor& descriptor) {
    return callback == nullptr || callback(descriptor);
}

// Fail closed. A runner with no authorization hook is a runner whose policy gate
// was never installed, and "no gate" must not read as "gate passed" for the
// features the gate exists for. Passive and defensive features still run, so a
// headless or bench runner stays usable -- that is the same asymmetry
// `PolicyService::legacyAllowsOffensive` uses.
bool policyAllowsWithoutHook(const FeatureDescriptor& descriptor) {
    return !PolicyService::isOffensive(descriptor.risk);
}

const char* kNoAuthorizationHook = "no authorization hook installed";
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
    if (hooks_.authorize != nullptr) {
        const Authorization auth = hooks_.authorize(*descriptor);
        if (!auth.allowed()) {
            // The policy's own words, not a generic label: the caller shows this
            // line, and "Blue Team mode active - leave via Device Policy" says
            // something the word "blocked" does not.
            return finish(result(FeatureResultCode::Blocked, auth.detail));
        }
    } else if (!policyAllowsWithoutHook(*descriptor)) {
        return finish(result(FeatureResultCode::Blocked, kNoAuthorizationHook));
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
    if (state_ != FeatureState::Running || feature_ == nullptr) {
        return;
    }
    feature_->tick();
    // Painted after tick(), so the frame that reaches the panel is the frame
    // that was just computed and not the previous one. present() is called only
    // when the feature actually painted: a sprite-backed canvas would otherwise
    // push whatever stale pixels it still held.
    if (context_.canvas != nullptr && feature_->draw(*context_.canvas)) {
        context_.canvas->present();
        ownsScreen_ = true;
    } else {
        ownsScreen_ = false;
    }
}

bool FeatureRunner::exitRequested() const {
    return state_ == FeatureState::Running && feature_ != nullptr &&
           feature_->exitRequested();
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
    ownsScreen_ = false;
    state_ = resultValue.code == FeatureResultCode::Completed ? FeatureState::Idle
                                                               : FeatureState::Failed;
    return resultValue;
}

void FeatureRunner::releaseFeature() {
    delete feature_;
    feature_ = nullptr;
    endCalled_ = false;
    ownsScreen_ = false;
}
