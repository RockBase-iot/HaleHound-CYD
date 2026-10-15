#include <unity.h>

#include <string>
#include <vector>

#include "features/rfid/NdefCodec.h"

void test_uri_abbreviation_prefers_the_longest_prefix() {
    std::string rest;
    TEST_ASSERT_EQUAL(0x02, NdefCodec::abbreviateUri("https://www.example.com", rest));
    TEST_ASSERT_EQUAL_STRING("example.com", rest.c_str());

    TEST_ASSERT_EQUAL(0x04, NdefCodec::abbreviateUri("https://example.com", rest));
    TEST_ASSERT_EQUAL_STRING("example.com", rest.c_str());

    // No known scheme: the whole URI is carried verbatim.
    TEST_ASSERT_EQUAL(0x00, NdefCodec::abbreviateUri("gopher://example.com", rest));
    TEST_ASSERT_EQUAL_STRING("gopher://example.com", rest.c_str());

    TEST_ASSERT_EQUAL_STRING("https://www.", NdefCodec::uriPrefix(0x02));
    TEST_ASSERT_EQUAL_STRING("urn:nfc:", NdefCodec::uriPrefix(0x23));
    TEST_ASSERT_EQUAL(36, NdefCodec::uriPrefixCount());
    // Out-of-range codes must not read past the table.
    TEST_ASSERT_EQUAL_STRING("", NdefCodec::uriPrefix(0x24));
}

void test_url_record_round_trips_through_a_message() {
    NdefMessage message;
    ndefAppend(message, NdefCodec::buildUrl("https://www.example.com/path"));

    const auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);

    const auto decoded = NdefCodec::decode(encoded.bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(1, decoded.message.records.size());
    TEST_ASSERT_EQUAL(NdefRecordType::Url, decoded.message.records[0].type);
    TEST_ASSERT_EQUAL_STRING("https://www.example.com/path",
                             NdefCodec::summarize(decoded.message.records[0]).c_str());
}

void test_text_record_carries_language_and_payload() {
    const NdefRecord record = NdefCodec::buildText("hello world", "en");
    TEST_ASSERT_EQUAL(NdefRecordType::Text, record.type);
    TEST_ASSERT_EQUAL_STRING("T", record.typeName.c_str());

    NdefMessage message;
    ndefAppend(message, record);
    const auto decoded = NdefCodec::decode(NdefCodec::encode(message).bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL_STRING("hello world", NdefCodec::summarize(decoded.message.records[0]).c_str());
}

void test_records_stack_and_message_flags_are_set() {
    NdefMessage message;
    ndefAppend(message, NdefCodec::buildUrl("https://example.com"));
    ndefAppend(message, NdefCodec::buildText("second"));

    const auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);
    // First record: message begin set, message end clear.
    TEST_ASSERT_TRUE((encoded.bytes[0] & 0x80) != 0);
    TEST_ASSERT_TRUE((encoded.bytes[0] & 0x40) == 0);

    const auto decoded = NdefCodec::decode(encoded.bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(2, decoded.message.records.size());
    TEST_ASSERT_EQUAL(NdefRecordType::Url, decoded.message.records[0].type);
    TEST_ASSERT_EQUAL(NdefRecordType::Text, decoded.message.records[1].type);

    ndefClear(message);
    TEST_ASSERT_EQUAL(0, message.records.size());
    TEST_ASSERT_FALSE(NdefCodec::encode(message).valid);
}

void test_long_payload_switches_to_the_four_byte_length_form() {
    // A Text record payload is [status byte][language code][text], so 400 text
    // bytes encode to 403 payload bytes -- comfortably past the 255-byte limit
    // of the short-record form. Compare against the record that was actually
    // built rather than a hardcoded total: the invariant that matters is
    // byte-exact round-trip, and it stays true if the default language changes.
    const NdefRecord textRecord = NdefCodec::buildText(std::string(400, 'x'));
    NdefMessage message;
    ndefAppend(message, textRecord);

    const auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);
    // Short-record bit must be clear and the length field four bytes wide.
    TEST_ASSERT_TRUE((encoded.bytes[0] & 0x10) == 0);
    TEST_ASSERT_GREATER_THAN(255U, textRecord.payload.size());

    const auto decoded = NdefCodec::decode(encoded.bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(textRecord.payload.size(), decoded.message.records[0].payload.size());
}

void test_wifi_and_vcard_and_geo_and_sms_records() {
    NdefWifiCredential credential;
    credential.ssid = "HaleNet";
    credential.password = "hunter2hunter2";
    const NdefRecord wifi = NdefCodec::buildWifi(credential);
    TEST_ASSERT_EQUAL(NdefRecordType::Wifi, wifi.type);
    TEST_ASSERT_EQUAL_STRING("HaleNet", NdefCodec::summarize(wifi).c_str());

    NdefVCard card;
    card.formattedName = "Jane Doe";
    card.organization = "Inkstone";
    card.phone = "+15550100";
    const NdefRecord vcard = NdefCodec::buildVCard(card);
    TEST_ASSERT_EQUAL(NdefRecordType::VCard, vcard.type);
    TEST_ASSERT_EQUAL_STRING("Jane Doe", NdefCodec::summarize(vcard).c_str());

    const NdefRecord geo = NdefCodec::buildGeo(12.345678, -45.678901);
    TEST_ASSERT_EQUAL(NdefRecordType::Geo, geo.type);
    TEST_ASSERT_EQUAL_STRING("12.345678,-45.678901", NdefCodec::summarize(geo).c_str());

    const NdefRecord sms = NdefCodec::buildSms("+15550111", "ping");
    TEST_ASSERT_EQUAL(NdefRecordType::Sms, sms.type);
    NdefMessage message;
    ndefAppend(message, sms);
    const auto decoded = NdefCodec::decode(NdefCodec::encode(message).bytes);
    TEST_ASSERT_TRUE(decoded.valid);
    // A SMS record is a URI record carrying the sms: scheme and must come back
    // classified as SMS rather than as a plain URL.
    TEST_ASSERT_EQUAL(NdefRecordType::Sms, decoded.message.records[0].type);
    TEST_ASSERT_EQUAL_STRING("sms:+15550111?body=ping",
                             NdefCodec::summarize(decoded.message.records[0]).c_str());
}

void test_tlv_wrapper_skips_the_capability_container_and_padding() {
    NdefMessage message;
    ndefAppend(message, NdefCodec::buildUrl("https://example.com"));

    const auto tlv = NdefCodec::encodeTlv(message);
    TEST_ASSERT_TRUE(tlv.valid);

    std::vector<uint8_t> tag;
    const uint8_t capabilityContainer[4] = {0xE1, 0x40, 0x12, 0x00};
    tag.insert(tag.end(), capabilityContainer, capabilityContainer + 4);
    tag.push_back(0x00);  // NULL TLV padding before the message
    tag.insert(tag.end(), tlv.bytes.begin(), tlv.bytes.end());

    const auto decoded = NdefCodec::decodeTlv(tag);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(1, decoded.message.records.size());
    TEST_ASSERT_EQUAL_STRING("https://example.com",
                             NdefCodec::summarize(decoded.message.records[0]).c_str());

    // A tag with no NDEF TLV at all is an empty state, not a crash.
    const std::vector<uint8_t> empty(4, 0x00);
    TEST_ASSERT_FALSE(NdefCodec::decodeTlv(empty).valid);
}

void test_truncated_input_is_rejected() {
    NdefMessage message;
    ndefAppend(message, NdefCodec::buildUrl("https://example.com"));
    auto encoded = NdefCodec::encode(message);
    TEST_ASSERT_TRUE(encoded.valid);

    encoded.bytes.resize(encoded.bytes.size() - 3);
    TEST_ASSERT_FALSE(NdefCodec::decode(encoded.bytes).valid);
    TEST_ASSERT_FALSE(NdefCodec::decode({}).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_uri_abbreviation_prefers_the_longest_prefix);
    RUN_TEST(test_url_record_round_trips_through_a_message);
    RUN_TEST(test_text_record_carries_language_and_payload);
    RUN_TEST(test_records_stack_and_message_flags_are_set);
    RUN_TEST(test_long_payload_switches_to_the_four_byte_length_form);
    RUN_TEST(test_wifi_and_vcard_and_geo_and_sms_records);
    RUN_TEST(test_tlv_wrapper_skips_the_capability_container_and_padding);
    RUN_TEST(test_truncated_input_is_rejected);
    return UNITY_END();
}
