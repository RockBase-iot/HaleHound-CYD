#include <unity.h>

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "core/BootHealthService.h"
#include "services/FirmwareImageValidator.h"

class VectorImage final : public IImageReader {
public:
    explicit VectorImage(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {}
    size_t size() const override { return bytes_.size(); }
    bool read(size_t offset, void* destination, size_t length) const override {
        if (offset > bytes_.size() || length > bytes_.size() - offset) return false;
        std::memcpy(destination, bytes_.data() + offset, length);
        return true;
    }
    void set(size_t offset, uint8_t value) { bytes_[offset] = value; }
private:
    std::vector<uint8_t> bytes_;
};

static VectorImage makeImage(FirmwareChip chip, uint32_t segmentLength = 4U) {
    std::vector<uint8_t> image(24U + 8U + segmentLength, 0U);
    image[0] = 0xE9U;
    image[1] = 1U;
    image[12] = static_cast<uint8_t>(chip == FirmwareChip::Esp32C5 ? 23U : 0U);
    image[24] = 0x00U;
    image[25] = 0x10U;
    image[28] = static_cast<uint8_t>(segmentLength);
    return VectorImage(std::move(image));
}

void test_validator_accepts_esp32_and_c5_without_filename_dependency() {
    const ImageValidationResult classic = FirmwareImageValidator::validate(
        makeImage(FirmwareChip::Esp32), FirmwareChip::Esp32, 1024U, "bad-name.bin");
    const ImageValidationResult c5 = FirmwareImageValidator::validate(
        makeImage(FirmwareChip::Esp32C5), FirmwareChip::Esp32C5, 1024U, "other-name.bin");
    TEST_ASSERT_TRUE(classic.valid);
    TEST_ASSERT_TRUE(c5.valid);
    TEST_ASSERT_EQUAL_UINT32(classic.imageLength, FirmwareImageValidator::validate(
        makeImage(FirmwareChip::Esp32), FirmwareChip::Esp32, 1024U, "renamed.dat").imageLength);
}

void test_validator_rejects_bad_magic_and_wrong_chip() {
    VectorImage bad = makeImage(FirmwareChip::Esp32);
    bad.set(0U, 0x00U);
    const ImageValidationResult badMagic = FirmwareImageValidator::validate(
        bad, FirmwareChip::Esp32, 1024U, nullptr);
    TEST_ASSERT_EQUAL(ImageValidationError::BadMagic, badMagic.error);
    VectorImage wrong = makeImage(FirmwareChip::Esp32C5);
    const ImageValidationResult wrongChip = FirmwareImageValidator::validate(
        wrong, FirmwareChip::Esp32, 1024U, nullptr);
    TEST_ASSERT_EQUAL(ImageValidationError::WrongChip, wrongChip.error);

    VectorImage shortHeader(std::vector<uint8_t>{0xE9U, 1U});
    const ImageValidationResult shortResult = FirmwareImageValidator::validate(
        shortHeader, FirmwareChip::Esp32, 1024U, nullptr);
    TEST_ASSERT_EQUAL(ImageValidationError::HeaderTruncated, shortResult.error);
}

void test_validator_rejects_truncated_segment_and_small_partition() {
    VectorImage truncated = makeImage(FirmwareChip::Esp32, 4U);
    truncated.set(28U, 64U);
    const ImageValidationResult truncatedResult = FirmwareImageValidator::validate(
        truncated, FirmwareChip::Esp32, 1024U, nullptr);
    TEST_ASSERT_EQUAL(ImageValidationError::SegmentOutOfBounds, truncatedResult.error);
    const ImageValidationResult smallPartition = FirmwareImageValidator::validate(
        makeImage(FirmwareChip::Esp32), FirmwareChip::Esp32, 4U, nullptr);
    TEST_ASSERT_EQUAL(ImageValidationError::PartitionTooSmall, smallPartition.error);
}

class MemoryBootStore final : public IBootHealthStore {
public:
    bool load(BootHealthRecord& output) override {
        if (!present) return false;
        output = record;
        return true;
    }
    bool save(const BootHealthRecord& input) override {
        record = input;
        present = true;
        return saveResult;
    }
    BootHealthRecord record{};
    bool present = false;
    bool saveResult = true;
};

void test_boot_health_enters_safe_mode_after_repeated_pending_boots() {
    MemoryBootStore store;
    BootHealthService health(store, 3U);
    TEST_ASSERT_TRUE(health.begin());
    TEST_ASSERT_FALSE(health.safeMode());
    TEST_ASSERT_TRUE(health.markPending());

    for (int attempt = 0; attempt < 3; ++attempt) {
        BootHealthService next(store, 3U);
        TEST_ASSERT_TRUE(next.begin());
        if (attempt < 2) TEST_ASSERT_FALSE(next.safeMode());
        if (attempt < 2) TEST_ASSERT_TRUE(next.markPending());
    }
    TEST_ASSERT_TRUE(health.begin());
}

void test_healthy_boot_clears_pending_but_explicit_recovery_clears_safe_mode() {
    MemoryBootStore store;
    BootHealthService health(store, 1U);
    TEST_ASSERT_TRUE(health.begin());
    TEST_ASSERT_TRUE(health.markPending());
    BootHealthService failed(store, 1U);
    TEST_ASSERT_TRUE(failed.begin());
    TEST_ASSERT_TRUE(failed.safeMode());
    TEST_ASSERT_TRUE(failed.markHealthy());
    BootHealthService stillSafe(store, 1U);
    TEST_ASSERT_TRUE(stillSafe.begin());
    TEST_ASSERT_TRUE(stillSafe.safeMode());
    TEST_ASSERT_TRUE(stillSafe.clearSafeMode());
    TEST_ASSERT_FALSE(stillSafe.safeMode());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_validator_accepts_esp32_and_c5_without_filename_dependency);
    RUN_TEST(test_validator_rejects_bad_magic_and_wrong_chip);
    RUN_TEST(test_validator_rejects_truncated_segment_and_small_partition);
    RUN_TEST(test_boot_health_enters_safe_mode_after_repeated_pending_boots);
    RUN_TEST(test_healthy_boot_clears_pending_but_explicit_recovery_clears_safe_mode);
    return UNITY_END();
}
