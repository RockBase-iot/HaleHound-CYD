#include "services/ResourceBroker.h"

namespace {
class MutexGuard {
public:
    MutexGuard(IResourceMutex* mutex, uint32_t timeoutMs)
        : mutex_(mutex), locked_(mutex == nullptr || mutex->lock(timeoutMs)) {}
    ~MutexGuard() {
        if (mutex_ != nullptr && locked_) {
            mutex_->unlock();
        }
    }
    bool locked() const { return locked_; }

private:
    IResourceMutex* mutex_;
    bool locked_;
};
}

ResourceLease::~ResourceLease() {
    release();
}

ResourceLease::ResourceLease(ResourceLease&& other) noexcept
    : broker_(other.broker_),
      resource_(other.resource_),
      owner_(other.owner_),
      code_(other.code_) {
    other.broker_ = nullptr;
    other.code_ = AcquireCode::Released;
}

ResourceLease& ResourceLease::operator=(ResourceLease&& other) noexcept {
    if (this != &other) {
        release();
        broker_ = other.broker_;
        resource_ = other.resource_;
        owner_ = other.owner_;
        code_ = other.code_;
        other.broker_ = nullptr;
        other.code_ = AcquireCode::Released;
    }
    return *this;
}

void ResourceLease::release() {
    if (broker_ == nullptr || code_ != AcquireCode::Acquired) {
        return;
    }
    broker_->release(resource_, owner_);
    broker_ = nullptr;
    code_ = AcquireCode::Released;
}

size_t ResourceBroker::indexOf(ResourceId resource) const {
    const size_t index = static_cast<size_t>(resource);
    return index < kResourceCount ? index : kResourceCount;
}

ResourceLease ResourceBroker::tryAcquire(ResourceId resource,
                                         OwnerId owner,
                                         uint32_t timeoutMs) {
    const size_t index = indexOf(resource);
    if (index >= kResourceCount) {
        return ResourceLease(this, resource, owner, AcquireCode::Invalid);
    }

    MutexGuard guard(mutex_, timeoutMs);
    if (!guard.locked()) {
        return ResourceLease(this, resource, owner, AcquireCode::Timeout);
    }
    Slot& slot = slots_[index];
    if (slot.held) {
        return ResourceLease(this,
                             resource,
                             owner,
                             slot.owner == owner ? AcquireCode::RecursiveDenied
                                                  : (timeoutMs == 0 ? AcquireCode::Busy
                                                                    : AcquireCode::Timeout));
    }
    slot.held = true;
    slot.owner = owner;
    slot.acquiredAt = timeoutMs;
    return ResourceLease(this, resource, owner, AcquireCode::Acquired);
}

void ResourceBroker::release(ResourceId resource, OwnerId owner) {
    const size_t index = indexOf(resource);
    if (index >= kResourceCount) {
        return;
    }
    MutexGuard guard(mutex_, 0);
    if (guard.locked() && slots_[index].held && slots_[index].owner == owner) {
        slots_[index] = Slot{};
    }
}

bool ResourceBroker::recover(ResourceId resource) {
    const size_t index = indexOf(resource);
    if (index >= kResourceCount) {
        return false;
    }
    MutexGuard guard(mutex_, 0);
    if (!guard.locked()) {
        return false;
    }
    slots_[index] = Slot{};
    return true;
}

bool ResourceBroker::isHeld(ResourceId resource) const {
    const size_t index = indexOf(resource);
    return index < kResourceCount && slots_[index].held;
}
