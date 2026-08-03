#include <unity.h>

#include "features/rfid/NtagModel.h"

void test_ntag_capacities_and_page_ranges() {
    const auto ntag213 = NtagModel::spec(NtagType::Ntag213);
    const auto ntag215 = NtagModel::spec(NtagType::Ntag215);
    const auto ntag216 = NtagModel::spec(NtagType::Ntag216);
    TEST_ASSERT_EQUAL(144, ntag213.userBytes);
    TEST_ASSERT_EQUAL(504, ntag215.userBytes);
    TEST_ASSERT_EQUAL(888, ntag216.userBytes);
    TEST_ASSERT_TRUE(NtagModel::isUserPage(NtagType::Ntag216, 225));
    TEST_ASSERT_FALSE(NtagModel::isUserPage(NtagType::Ntag216, 226));
    TEST_ASSERT_TRUE(NtagModel::isConfigPage(NtagType::Ntag213, 41));
}

void test_ndef_uri_text_and_multiple_records() {
    std::vector<NdefRecord> records = {
        NdefRecord::uri("https://halehound.com"),
        NdefRecord::text("HaleHound"),
    };
    const auto encoded = NtagModel::encodeNdef(records);
    TEST_ASSERT_GREATER_THAN(10, encoded.size());
    TEST_ASSERT_EQUAL(0x03, encoded[0]);
    TEST_ASSERT_EQUAL(0xFE, encoded.back());
}

void test_lock_warning_and_password_operations() {
    NtagModel model(NtagType::Ntag213);
    TEST_ASSERT_TRUE(model.buildWritePlan(4, 2).requiresConfirmation == false);
    TEST_ASSERT_TRUE(model.buildWritePlan(2, 1).requiresConfirmation);
    TEST_ASSERT_NOT_NULL(model.irreversibleWarning());
    TEST_ASSERT_TRUE(model.setPassword({1, 2, 3, 4}, 4));
    TEST_ASSERT_TRUE(model.passwordEnabled());
    TEST_ASSERT_TRUE(model.removePassword());
    TEST_ASSERT_FALSE(model.passwordEnabled());
}

class FailingTransport final : public INtagPageTransport {
public:
    bool writePage(uint16_t page, const std::array<uint8_t, 4>& data) override {
        lastPage = page;
        lastData = data;
        return true;
    }

    bool readPage(uint16_t page, std::array<uint8_t, 4>& data) override {
        ++readCount;
        if (page == 5) return false;
        data = lastData;
        return true;
    }

    uint16_t lastPage = 0;
    uint8_t readCount = 0;
    std::array<uint8_t, 4> lastData{};
};

void test_page_verification_stops_following_writes() {
    NtagModel model(NtagType::Ntag213);
    FailingTransport transport;
    std::vector<std::array<uint8_t, 4>> pages = {{{1, 2, 3, 4}}, {{5, 6, 7, 8}}};
    const auto result = model.writePages(transport, 4, pages, false);
    TEST_ASSERT_FALSE(result.completed);
    TEST_ASSERT_EQUAL(1, result.pagesWritten);
    TEST_ASSERT_EQUAL_STRING("page verification failed; subsequent pages were not written", result.detail);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ntag_capacities_and_page_ranges);
    RUN_TEST(test_ndef_uri_text_and_multiple_records);
    RUN_TEST(test_lock_warning_and_password_operations);
    RUN_TEST(test_page_verification_stops_following_writes);
    return UNITY_END();
}
