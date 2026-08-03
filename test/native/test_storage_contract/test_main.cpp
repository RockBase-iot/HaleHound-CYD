#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "services/StoragePaths.h"
#include "services/StorageService.h"

void test_storage_paths_are_canonical_and_credentials_are_separate() {
    TEST_ASSERT_EQUAL_STRING("/subghz", storage_paths::canonical(StoragePathId::Subghz));
    TEST_ASSERT_EQUAL_STRING("/loot/credentials", storage_paths::canonical(StoragePathId::Credentials));
    TEST_ASSERT_TRUE(storage_paths::isReadOnly(StoragePathId::CredsInput));
    TEST_ASSERT_FALSE(storage_paths::isReadOnly(StoragePathId::Credentials));
}

void test_path_policy_rejects_escape_and_unknown_roots() {
    TEST_ASSERT_TRUE(PathPolicy::validate("/loot/item.txt", false).valid);
    TEST_ASSERT_FALSE(PathPolicy::validate("/loot/../secrets.txt", false).valid);
    TEST_ASSERT_FALSE(PathPolicy::validate("\\loot\\item.txt", false).valid);
    TEST_ASSERT_FALSE(PathPolicy::validate("/unknown/item.txt", false).valid);
}

void test_path_policy_rejects_writes_to_legacy_credentials_input() {
    const PathCheck result = PathPolicy::validate("/creds.txt", true);
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(PathError::ReadOnly, result.error);
}

void test_legacy_credentials_map_to_new_credentials_root() {
    char destination[64]{};
    TEST_ASSERT_TRUE(migrateCredentialPath("/creds.txt", destination, sizeof(destination)));
    TEST_ASSERT_EQUAL_STRING("/loot/credentials/legacy_creds.txt", destination);
    TEST_ASSERT_TRUE(isLegacyPskRecord("[PSK] network | secret"));
    TEST_ASSERT_FALSE(isLegacyPskRecord("network | secret"));
}

class FakeStorageBackend final : public IStorageBackend {
public:
    StorageCode begin() override { return beginCode; }
    StorageCode write(const char* path, const uint8_t* data, size_t size) override {
        writes.emplace_back(path);
        bytes.assign(data, data + size);
        return writeCode;
    }
    StorageCode flush(const char*) override { return flushCode; }
    bool verify(const char*, size_t size, uint32_t) override {
        verifiedSize = size;
        return verifyCode;
    }
    StorageCode rename(const char* from, const char* to) override {
        renamedFrom = from;
        renamedTo = to;
        return renameCode;
    }

    StorageCode beginCode = StorageCode::Ok;
    StorageCode writeCode = StorageCode::Ok;
    StorageCode flushCode = StorageCode::Ok;
    bool verifyCode = true;
    StorageCode renameCode = StorageCode::Ok;
    std::vector<std::string> writes;
    std::vector<uint8_t> bytes;
    size_t verifiedSize = 0;
    std::string renamedFrom;
    std::string renamedTo;
};

void test_atomic_write_verifies_before_rename() {
    FakeStorageBackend backend;
    StorageService storage(backend);
    const uint8_t payload[] = {1, 2, 3};
    const StorageResult result = storage.writeAtomic("/loot/item.bin", payload, sizeof(payload));
    TEST_ASSERT_EQUAL(StorageCode::Ok, result.code);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), result.bytes);
    TEST_ASSERT_EQUAL_STRING("/loot/item.bin.tmp", backend.renamedFrom.c_str());
    TEST_ASSERT_EQUAL_STRING("/loot/item.bin", backend.renamedTo.c_str());
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), backend.verifiedSize);
}

void test_atomic_write_does_not_report_success_when_verify_fails() {
    FakeStorageBackend backend;
    backend.verifyCode = false;
    StorageService storage(backend);
    const uint8_t payload[] = {1};
    const StorageResult result = storage.writeAtomic("/loot/item.bin", payload, sizeof(payload));
    TEST_ASSERT_NOT_EQUAL(StorageCode::Ok, result.code);
    TEST_ASSERT_TRUE(backend.renamedTo.empty());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_storage_paths_are_canonical_and_credentials_are_separate);
    RUN_TEST(test_path_policy_rejects_escape_and_unknown_roots);
    RUN_TEST(test_path_policy_rejects_writes_to_legacy_credentials_input);
    RUN_TEST(test_legacy_credentials_map_to_new_credentials_root);
    RUN_TEST(test_atomic_write_verifies_before_rename);
    RUN_TEST(test_atomic_write_does_not_report_success_when_verify_fails);
    return UNITY_END();
}
