#pragma once

#include <cstddef>
#include <cstdint>

#include "services/StoragePaths.h"

enum class StorageCode : uint8_t {
    Ok,
    Unavailable,
    NotFound,
    InvalidPath,
    ReadOnly,
    Busy,
    Fault,
    Unsupported,
};

struct StorageResult {
    StorageCode code;
    size_t bytes;
    const char* detail;
};

class IStorageBackend {
public:
    virtual ~IStorageBackend() = default;
    virtual StorageCode begin() = 0;
    virtual StorageCode write(const char* path, const uint8_t* data, size_t size) = 0;
    virtual StorageCode flush(const char* path) = 0;
    virtual bool verify(const char* path, size_t size, uint32_t hash) = 0;
    virtual StorageCode rename(const char* from, const char* to) = 0;
    virtual StorageCode ensureDirectory(const char* path) {
        (void)path;
        return StorageCode::Unsupported;
    }
    virtual StorageResult migrateLegacyCredentials() {
        return {StorageCode::Unsupported, 0U, "credential migration unavailable"};
    }
};

class StorageService {
public:
    explicit StorageService(IStorageBackend& backend) : backend_(backend) {}

    StorageCode begin();
    bool available() const { return state_ == StorageCode::Ok; }
    StorageResult writeAtomic(const char* path,
                              const uint8_t* data,
                              size_t size);
    StorageCode ensureDirectory(const char* path);
    StorageResult migrateLegacyCredentials();

private:
    static uint32_t hash(const uint8_t* data, size_t size);

    IStorageBackend& backend_;
    StorageCode state_ = StorageCode::Busy;
    bool attempted_ = false;
};

StorageService& globalStorageService();
