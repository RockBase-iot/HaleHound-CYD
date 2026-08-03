#include <unity.h>

#include "features/subghz/SubFileParser.h"

void test_raw_file_is_non_transmitting_model() {
    const auto result = SubFileParser::parse(
        "Filetype: Flipper SubGhz RAW File\n"
        "Version: 1\n"
        "Frequency: 433920000\n"
        "Protocol: RAW\n"
        "RAW_Data: 350 -700 350 -700\n");
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(SubProtocol::Raw, result.document.protocol);
    TEST_ASSERT_EQUAL_UINT32(433920000U, result.document.frequencyHz);
    TEST_ASSERT_EQUAL(4, result.document.pulses.size());
    TEST_ASSERT_FALSE(result.document.transmitAllowed);
}

void test_supported_structured_protocols_are_classified() {
    const char* protocols[] = {"Princeton", "CAME", "Nice"};
    const SubProtocol expected[] = {SubProtocol::Princeton, SubProtocol::Came, SubProtocol::Nice};
    for (size_t index = 0; index < 3; ++index) {
        std::string text = "Frequency: 315000000\nProtocol: ";
        text += protocols[index];
        text += "\nKey: 0x12345678\nBit: 24\nTE: 350\n";
        const auto result = SubFileParser::parse(text);
        TEST_ASSERT_TRUE(result.valid);
        TEST_ASSERT_EQUAL(expected[index], result.document.protocol);
    }
}

void test_unknown_and_malformed_input_is_safe() {
    auto unknown = SubFileParser::parse("Frequency: 433920000\nProtocol: FutureWire\n");
    TEST_ASSERT_FALSE(unknown.valid);
    TEST_ASSERT_EQUAL(SubProtocol::Unknown, unknown.document.protocol);

    auto tooLong = SubFileParser::parse(
        "Frequency: 433920000\nProtocol: RAW\nRAW_Data: 999999999\n");
    TEST_ASSERT_FALSE(tooLong.valid);

    auto missing = SubFileParser::parse("Protocol: RAW\nRAW_Data: 100 -100\n");
    TEST_ASSERT_FALSE(missing.valid);
}

void test_nested_and_empty_directory_inputs() {
    std::vector<SubFileInput> files = {
        {"nested/doorbell.sub", "Frequency: 433920000\nProtocol: RAW\nRAW_Data: 100 -100\n"},
        {"nested/ignored.txt", "not a sub file"},
    };
    auto directory = SubFileParser::parseDirectory(files);
    TEST_ASSERT_TRUE(directory.valid);
    TEST_ASSERT_EQUAL(1, directory.documents.size());
    TEST_ASSERT_TRUE(SubFileParser::parseDirectory({}).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_raw_file_is_non_transmitting_model);
    RUN_TEST(test_supported_structured_protocols_are_classified);
    RUN_TEST(test_unknown_and_malformed_input_is_safe);
    RUN_TEST(test_nested_and_empty_directory_inputs);
    return UNITY_END();
}
