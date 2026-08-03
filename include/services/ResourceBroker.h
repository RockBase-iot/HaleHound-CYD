#pragma once

#include <cstddef>
#include <cstdint>

#include "services/IResourceMutex.h"

enum class ResourceId : uint8_t {
    SharedSpi,
    Display,
    Touch,
    SdCard,
    Storage,
    Wifi,
    Bluetooth,
};

enum class OwnerId : uint8_t {
    Display,
    Touch,
    SdCard,
    Cc1101,
    Nrf24,
    Pn532,
    Storage,
    Feature,
};

enum class AcquireCode : uint8_t {
    Acquired,
    Busy,
    Timeout,
    RecursiveDenied,
    Invalid,
    Released,
    Fault,
};

class ResourceBroker;

class ResourceLease {
public:
    ResourceLease() = default;
    ~ResourceLease();
    ResourceLease(const ResourceLease&) = delete;
    ResourceLease& operator=(const ResourceLease&) = delete;
    ResourceLease(ResourceLease&& other) noexcept;
    ResourceLease& operator=(ResourceLease&& other) noexcept;

    bool acquired() const { return code_ == AcquireCode::Acquired && broker_ != nullptr; }
    AcquireCode code() const { return code_; }
    ResourceId resource() const { return resource_; }
    OwnerId owner() const { return owner_; }
    void release();

private:
    friend class ResourceBroker;
    ResourceLease(ResourceBroker* broker,
                  ResourceId resource,
                  OwnerId owner,
                  AcquireCode code)
        : broker_(broker), resource_(resource), owner_(owner), code_(code) {}

    ResourceBroker* broker_ = nullptr;
    ResourceId resource_ = ResourceId::SharedSpi;
    OwnerId owner_ = OwnerId::Feature;
    AcquireCode code_ = AcquireCode::Invalid;
};

class ResourceBroker {
public:
    explicit ResourceBroker(IResourceMutex* mutex = nullptr)
        : mutex_(mutex) {}

    ResourceLease tryAcquire(ResourceId resource, OwnerId owner, uint32_t timeoutMs);
    bool recover(ResourceId resource);
    bool isHeld(ResourceId resource) const;

private:
    friend class ResourceLease;
    static constexpr size_t kResourceCount = 7;

    struct Slot {
        bool held = false;
        OwnerId owner = OwnerId::Feature;
        uint32_t acquiredAt = 0;
    };

    void release(ResourceId resource, OwnerId owner);
    size_t indexOf(ResourceId resource) const;

    IResourceMutex* mutex_ = nullptr;
    Slot slots_[kResourceCount]{};
};
