#include <unity.h>

#include <string>

#include "features/rfid/CardDumpFormat.h"

namespace {

CardDump makeClassicDump() {
    CardDump dump;
    dump.kind = CardDumpKind::MifareClassic;
    dump.deviceType = "Mifare Classic";
    dump.mifareType = "1K";
    dump.uidLength = 4;
    dump.uid[0] = 0x04;
    dump.uid[1] = 0x12;
    dump.uid[2] = 0x34;
    dump.uid[3] = 0x56;
    dump.atqa = 0x0004;
    dump.sak = 0x08;

    // Sector 0 block 0 is the MIFARE manufacturer block: UID bytes 0..3, the
    // BCC over them, then SAK and ATQA. fromProxmarkEml() derives the header
    // from exactly these offsets, so the fixture has to look like a real card
    // rather than a bare 0..15 ramp. Bytes 8..15 keep a recognisable ramp so
    // the round-trip tests can prove all sixteen bytes survived.
    CardDumpBlock block0;
    const uint8_t manufacturer[kCardDumpBlockSize] = {0x04, 0x12, 0x34, 0x56, 0x74, 0x08, 0x04, 0x00,
                                                      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    for (size_t index = 0; index < kCardDumpBlockSize; ++index) {
        block0.data[index] = manufacturer[index];
    }
    dump.blocks.push_back(block0);

    CardDumpBlock block1;
    for (size_t index = 0; index < kCardDumpBlockSize; ++index) {
        block1.data[index] = 0xAA;
    }
    dump.blocks.push_back(block1);

    // Sector 1 trailer with the default key written twice.
    CardDumpBlock trailer;
    const uint8_t trailerBytes[kCardDumpBlockSize] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                     0xFF, 0x07, 0x80, 0x69, 0xFF, 0xFF,
                                                     0xFF, 0xFF, 0xFF, 0xFF};
    for (size_t index = 0; index < kCardDumpBlockSize; ++index) {
        trailer.data[index] = trailerBytes[index];
    }
    dump.blocks.push_back(trailer);

    // A block the reader could not authenticate.
    CardDumpBlock unknown;
    unknown.known = false;
    dump.blocks.push_back(unknown);

    dump.unknownKeys.push_back("Signature: DEADBEEF");
    return dump;
}

}  // namespace

void test_flipper_nfc_round_trip_preserves_header_blocks_and_unknown_keys() {
    const CardDump original = makeClassicDump();
    const auto text = CardDumpFormat::toFlipperNfc(original);
    TEST_ASSERT_TRUE(text.valid);
    TEST_ASSERT_TRUE(text.text.find("Filetype: Flipper NFC device") == 0);
    TEST_ASSERT_TRUE(text.text.find("Signature: DEADBEEF") != std::string::npos);

    const auto loaded = CardDumpFormat::fromFlipperNfc(text.text);
    TEST_ASSERT_TRUE(loaded.valid);
    TEST_ASSERT_EQUAL(CardDumpKind::MifareClassic, loaded.dump.kind);
    TEST_ASSERT_EQUAL(4, loaded.dump.uidLength);
    TEST_ASSERT_EQUAL_STRING("04 12 34 56", CardDumpFormat::formatUid(loaded.dump).c_str());
    TEST_ASSERT_EQUAL(0x0004, loaded.dump.atqa);
    TEST_ASSERT_EQUAL(0x08, loaded.dump.sak);
    TEST_ASSERT_EQUAL_STRING("1K", loaded.dump.mifareType.c_str());
    TEST_ASSERT_EQUAL(4, loaded.dump.blocks.size());
    TEST_ASSERT_TRUE(loaded.dump.blocks[0].known);
    TEST_ASSERT_EQUAL(0x0F, loaded.dump.blocks[0].data[15]);
    TEST_ASSERT_EQUAL(0xAA, loaded.dump.blocks[1].data[0]);
    // The unauthenticated block must survive as unknown, not as zeros.
    TEST_ASSERT_FALSE(loaded.dump.blocks[3].known);
    TEST_ASSERT_EQUAL(1, loaded.dump.unknownKeys.size());
    TEST_ASSERT_EQUAL_STRING("Signature: DEADBEEF", loaded.dump.unknownKeys[0].c_str());
}

void test_proxmark_eml_round_trip_and_manufacturer_block_derivation() {
    const CardDump original = makeClassicDump();
    const auto text = CardDumpFormat::toProxmarkEml(original);
    TEST_ASSERT_TRUE(text.valid);
    // An .eml cannot express "unknown", so the unknown block is dropped and
    // only the three known blocks are written.
    TEST_ASSERT_EQUAL(3, text.text.size() / 48U + (text.text.size() % 48U != 0 ? 1 : 0));

    const auto loaded = CardDumpFormat::fromProxmarkEml(text.text);
    TEST_ASSERT_TRUE(loaded.valid);
    TEST_ASSERT_EQUAL(3, loaded.dump.blocks.size());
    // UID, SAK and ATQA come out of block 0, the manufacturer block.
    TEST_ASSERT_EQUAL(4, loaded.dump.uidLength);
    TEST_ASSERT_EQUAL(0x04, loaded.dump.uid[0]);
    TEST_ASSERT_EQUAL(0x56, loaded.dump.uid[3]);
    TEST_ASSERT_EQUAL(0x08, loaded.dump.sak);
    TEST_ASSERT_EQUAL(0x0004, loaded.dump.atqa);
    TEST_ASSERT_EQUAL_STRING("Mini", loaded.dump.mifareType.c_str());
}

void test_eml_with_only_unknown_blocks_is_refused() {
    CardDump dump;
    dump.kind = CardDumpKind::MifareClassic;
    CardDumpBlock unknown;
    unknown.known = false;
    dump.blocks.push_back(unknown);
    const auto text = CardDumpFormat::toProxmarkEml(dump);
    TEST_ASSERT_FALSE(text.valid);
}

void test_malformed_nfc_is_rejected_rather_than_half_loaded() {
    auto missingHeader = CardDumpFormat::fromFlipperNfc("UID: 04 12 34 56\nBlock 0: " + std::string(47, '0'));
    TEST_ASSERT_FALSE(missingHeader.valid);

    auto badUid = CardDumpFormat::fromFlipperNfc(
        "Filetype: Flipper NFC device\nUID: 04 12 ZZ 56\nBlock 0: " + std::string(47, '0'));
    TEST_ASSERT_FALSE(badUid.valid);

    auto shortBlock = CardDumpFormat::fromFlipperNfc(
        "Filetype: Flipper NFC device\nUID: 04 12 34 56\nBlock 0: 00 01 02\n");
    TEST_ASSERT_FALSE(shortBlock.valid);

    auto wrongBlockSize = CardDumpFormat::fromProxmarkEml("00 01 02 03\n");
    TEST_ASSERT_FALSE(wrongBlockSize.valid);
}

void test_extension_dispatch_and_mifare_size_mapping() {
    const CardDump original = makeClassicDump();
    const auto nfc = CardDumpFormat::toFlipperNfc(original);
    const auto eml = CardDumpFormat::toProxmarkEml(original);
    TEST_ASSERT_TRUE(CardDumpFormat::parseByExtension("card.nfc", nfc.text).valid);
    TEST_ASSERT_TRUE(CardDumpFormat::parseByExtension("card.eml", eml.text).valid);
    TEST_ASSERT_FALSE(CardDumpFormat::parseByExtension("card.bin", nfc.text).valid);
    TEST_ASSERT_FALSE(CardDumpFormat::parseByExtension("card", nfc.text).valid);

    TEST_ASSERT_EQUAL(20, CardDumpFormat::blocksForMifareType("Mini"));
    TEST_ASSERT_EQUAL(64, CardDumpFormat::blocksForMifareType("1K"));
    TEST_ASSERT_EQUAL(256, CardDumpFormat::blocksForMifareType("4K"));
    TEST_ASSERT_EQUAL(0, CardDumpFormat::blocksForMifareType("Unspecified"));

    TEST_ASSERT_EQUAL_STRING("MIFARE Classic", CardDumpFormat::kindLabel(CardDumpKind::MifareClassic));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_flipper_nfc_round_trip_preserves_header_blocks_and_unknown_keys);
    RUN_TEST(test_proxmark_eml_round_trip_and_manufacturer_block_derivation);
    RUN_TEST(test_eml_with_only_unknown_blocks_is_refused);
    RUN_TEST(test_malformed_nfc_is_rejected_rather_than_half_loaded);
    RUN_TEST(test_extension_dispatch_and_mifare_size_mapping);
    return UNITY_END();
}
