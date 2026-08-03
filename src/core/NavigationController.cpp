#include "core/NavigationController.h"

NavigationOutcome NavigationController::push(FeatureId id) {
    if (depth_ >= kMaxDepth) {
        return NavigationOutcome::Blocked;
    }
    stack_[depth_++] = id;
    lastError_ = "";
    return NavigationOutcome::Pushed;
}

NavigationOutcome NavigationController::complete(const FeatureResult& result) {
    if (result.code == FeatureResultCode::Blocked) {
        return NavigationOutcome::Blocked;
    }
    if (result.code == FeatureResultCode::Fault) {
        lastError_ = result.detail == nullptr ? "feature fault" : result.detail;
        return NavigationOutcome::Fault;
    }
    return back();
}

NavigationOutcome NavigationController::back() {
    if (depth_ == 0) {
        return NavigationOutcome::Empty;
    }
    --depth_;
    return NavigationOutcome::Returned;
}
