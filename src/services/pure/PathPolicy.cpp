#include "services/StoragePaths.h"

#include <cstring>

namespace {
bool hasRoot(const char* path, const char* root) {
    const size_t rootLength = std::strlen(root);
    return std::strncmp(path, root, rootLength) == 0 &&
           (path[rootLength] == '\0' || path[rootLength] == '/');
}
}

const char* storage_paths::canonical(StoragePathId id) {
    switch (id) {
    case StoragePathId::Subghz: return "/subghz";
    case StoragePathId::Eapol: return "/eapol";
    case StoragePathId::Wardriving: return "/wardriving";
    case StoragePathId::WpLoot: return "/wp_loot";
    case StoragePathId::Loot: return "/loot";
    case StoragePathId::Credentials: return "/loot/credentials";
    case StoragePathId::CredsInput: return "/creds.txt";
    case StoragePathId::IotReconInput: return "/iot_recon.txt";
    case StoragePathId::Firmware: return "/firmware";
    }
    return "/";
}

bool storage_paths::isReadOnly(StoragePathId id) {
    return id == StoragePathId::CredsInput;
}

PathCheck PathPolicy::validate(const char* path, bool writing) {
    if (path == nullptr || path[0] == '\0') {
        return {false, PathError::Empty, nullptr};
    }
    const size_t length = std::strlen(path);
    if (length > kMaxPathLength) {
        return {false, PathError::TooLong, nullptr};
    }
    if (path[0] != '/') {
        return {false, PathError::UnknownRoot, nullptr};
    }
    if (std::strchr(path, '\\') != nullptr) {
        return {false, PathError::Backslash, nullptr};
    }
    if (std::strstr(path, "..") != nullptr) {
        return {false, PathError::Traversal, nullptr};
    }

    StoragePathId roots[] = {
        StoragePathId::Subghz, StoragePathId::Eapol, StoragePathId::Wardriving,
        StoragePathId::WpLoot, StoragePathId::Loot, StoragePathId::CredsInput,
        StoragePathId::IotReconInput, StoragePathId::Firmware,
    };
    for (const StoragePathId root : roots) {
        if (hasRoot(path, storage_paths::canonical(root))) {
            if (writing && storage_paths::isReadOnly(root)) {
                return {false, PathError::ReadOnly, storage_paths::canonical(root)};
            }
            return {true, PathError::None, storage_paths::canonical(root)};
        }
    }
    if (hasRoot(path, storage_paths::canonical(StoragePathId::Credentials))) {
        return {true, PathError::None, storage_paths::canonical(StoragePathId::Credentials)};
    }
    return {false, PathError::UnknownRoot, nullptr};
}
