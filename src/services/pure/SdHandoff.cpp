#include "services/SdHandoff.h"

namespace {
bool validPath(const char* path) {
    return path != nullptr && path[0] == '/';
}
}  // namespace

SdHandoffResult SdHandoff::fail(SdCode code, const char* detail) {
    lastDetail_ = detail;
    return {code, detail};
}

SdCode SdHandoff::mount() {
    if (!cardPresent_) {
        state_ = SdState::Unmounted;
        lastDetail_ = "no card detected";
        return SdCode::NotMounted;
    }
    if (state_ == SdState::RadioOwnsBus) {
        // Mounting under the radio's feet is exactly the race that corrupts
        // FAT state; refuse instead.
        lastDetail_ = "radio owns the shared SPI bus";
        return SdCode::Busy;
    }
    ++mountCount_;
    state_ = SdState::Mounted;
    lastDetail_ = "mounted";
    return SdCode::Ready;
}

SdCode SdHandoff::unmount() {
    if (state_ == SdState::RadioOwnsBus) {
        lastDetail_ = "radio owns the shared SPI bus";
        return SdCode::Busy;
    }
    state_ = SdState::Unmounted;
    lastDetail_ = "unmounted";
    return SdCode::Ready;
}

SdCode SdHandoff::beginRadioSession() {
    if (state_ == SdState::RadioOwnsBus) {
        lastDetail_ = "radio session already active";
        return SdCode::Invalid;
    }
    // Always drop the volume first: an unmounted card cannot be left mid-write
    // by the radio's bus transactions.
    state_ = SdState::Unmounted;
    state_ = SdState::RadioOwnsBus;
    lastDetail_ = "radio session active";
    return SdCode::Ready;
}

SdCode SdHandoff::endRadioSession() {
    if (state_ != SdState::RadioOwnsBus) {
        lastDetail_ = "no radio session to end";
        return SdCode::Invalid;
    }
    state_ = SdState::Unmounted;
    lastDetail_ = "radio released the bus; volume not mounted";
    return SdCode::Ready;
}

SdHandoffResult SdHandoff::readForLoot(const char* path) {
    if (!validPath(path)) {
        return fail(SdCode::Invalid, "loot path must be absolute");
    }

    if (state_ == SdState::RadioOwnsBus) {
        // The regression: this used to look like an empty directory listing.
        ++blockedReadCount_;
        return fail(SdCode::Busy,
                    "radio still owns the shared SPI bus - retry after teardown");
    }

    if (state_ == SdState::Unmounted) {
        if (mount() != SdCode::Ready) {
            ++blockedReadCount_;
            return fail(SdCode::NotMounted, "card present but mount failed");
        }
        ++remountCount_;
        return {SdCode::Remounted,
                "card was re-mounted after the radio session"};
    }

    return {SdCode::Ready, "mounted"};
}
