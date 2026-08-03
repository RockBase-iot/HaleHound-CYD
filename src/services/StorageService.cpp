#include "services/StorageService.h"

#include <cstdio>
#include <cstring>

StorageCode StorageService::begin() {
    if (attempted_) return state_;
    attempted_ = true;
    state_ = backend_.begin();
    return state_;
}

uint32_t StorageService::hash(const uint8_t* data, size_t size) {
    uint32_t value = 2166136261UL;
    for (size_t i = 0; i < size; ++i) {
        value ^= data[i];
        value *= 16777619UL;
    }
    return value;
}

StorageResult StorageService::writeAtomic(const char* path,
                                           const uint8_t* data,
                                           size_t size) {
    const PathCheck check = PathPolicy::validate(path, true);
    if (!check.valid) {
        return {check.error == PathError::ReadOnly ? StorageCode::ReadOnly
                                                    : StorageCode::InvalidPath,
                0U, "path rejected"};
    }
    if (data == nullptr && size != 0U) {
        return {StorageCode::Fault, 0U, "null payload"};
    }
    if (begin() != StorageCode::Ok) {
        return {state_ == StorageCode::Busy ? StorageCode::Unavailable : state_,
                0U, "storage unavailable"};
    }

    char temporary[PathPolicy::kMaxPathLength + 5U]{};
    const int written = std::snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(temporary)) {
        return {StorageCode::InvalidPath, 0U, "temporary path too long"};
    }
    if (backend_.write(temporary, data, size) != StorageCode::Ok ||
        backend_.flush(temporary) != StorageCode::Ok ||
        !backend_.verify(temporary, size, hash(data, size))) {
        return {StorageCode::Fault, 0U, "atomic write verification failed"};
    }
    if (backend_.rename(temporary, path) != StorageCode::Ok) {
        return {StorageCode::Fault, 0U, "atomic rename failed"};
    }
    return {StorageCode::Ok, size, "committed"};
}

StorageCode StorageService::ensureDirectory(const char* path) {
    const PathCheck check = PathPolicy::validate(path, true);
    if (!check.valid) return StorageCode::InvalidPath;
    if (begin() != StorageCode::Ok) return state_;
    return backend_.ensureDirectory(path);
}

StorageResult StorageService::migrateLegacyCredentials() {
    if (begin() != StorageCode::Ok) return {state_, 0U, "storage unavailable"};
    return backend_.migrateLegacyCredentials();
}

#include "platform/StorageBackend.h"

StorageService& globalStorageService() {
    static StorageService service(platformStorageBackend());
    return service;
}
