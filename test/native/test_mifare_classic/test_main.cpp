#include <unity.h>

#include <array>
#include <string>

#include "features/rfid/MifareClassicModel.h"
#include "features/rfid/MifareKeys.h"

namespace {

// Fixture helper: decode a bare 12-hex-digit key literal.
//
// It deliberately does NOT call MifareKeyTable::parse(). Unity's TEST_ASSERT_*
// expands to a bare `return;`, which clang rejects inside a non-void function
// (-Wreturn-mismatch), and a helper that silently returns a sentinel on failure
// would mask a bad fixture. parse() -- including the separator forms -- has its
// own dedicated test below; here we only need the six bytes.
std::array<uint8_t, kMifareKeySize> key(const char* text) {
    std::array<uint8_t, kMifareKeySize> out{};
    for (uint8_t index = 0; index < kMifareKeySize; ++index) {
        const auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            return static_cast<uint8_t>(c - 'A' + 10);
        };
        const uint8_t high = nibble(text[index * 2]);
        const uint8_t low = nibble(text[index * 2 + 1]);
        out[index] = static_cast<uint8_t>((high << 4) | low);
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// Key table provenance and encoding
// ---------------------------------------------------------------------------

void test_key_table_is_the_sourced_twenty_nine() {
    // size is pinned by a static_assert too; this makes a reorder visible here.
    TEST_ASSERT_EQUAL(29, MifareKeyTable::count());
    TEST_ASSERT_EQUAL_STRING("FFFFFFFFFFFF", MifareKeyTable::toString(MifareKeyTable::at(0).bytes).c_str());
    TEST_ASSERT_EQUAL_STRING("A0A1A2A3A4A5", MifareKeyTable::toString(MifareKeyTable::at(2).bytes).c_str());
    TEST_ASSERT_EQUAL_STRING("D3F7D3F7D3F7", MifareKeyTable::toString(MifareKeyTable::at(23).bytes).c_str());
    // Every entry must carry provenance; an unattributed key is a bug.
    for (uint16_t index = 0; index < MifareKeyTable::count(); ++index) {
        TEST_ASSERT_TRUE(MifareKeyTable::at(index).note[0] != '\0');
    }
}

void test_named_keys_match_their_documented_values() {
    TEST_ASSERT_EQUAL_STRING("FFFFFFFFFFFF", MifareKeyTable::toString(MifareKeyTable::transportKey()).c_str());
    TEST_ASSERT_EQUAL_STRING("A0A1A2A3A4A5", MifareKeyTable::toString(MifareKeyTable::madKeyA()).c_str());
    TEST_ASSERT_EQUAL_STRING("D3F7D3F7D3F7", MifareKeyTable::toString(MifareKeyTable::ndefSectorKeyA()).c_str());
    TEST_ASSERT_TRUE(MifareKeyTable::isTransportKey(key("FFFFFFFFFFFF")));
    TEST_ASSERT_FALSE(MifareKeyTable::isTransportKey(key("000000000000")));
    TEST_ASSERT_TRUE(MifareKeyTable::isAllZero(key("000000000000")));
    TEST_ASSERT_TRUE(MifareKeyTable::isAllOnes(key("ffffffffffff")));
}

void test_key_parsing_accepts_tool_formats_and_rejects_short_or_long() {
    std::array<uint8_t, kMifareKeySize> out{};

    TEST_ASSERT_TRUE(MifareKeyTable::parse("ffffffffffff", out));
    TEST_ASSERT_TRUE(MifareKeyTable::isAllOnes(out));
    TEST_ASSERT_TRUE(MifareKeyTable::parse("FF FF FF FF FF FF", out));
    TEST_ASSERT_TRUE(MifareKeyTable::isAllOnes(out));
    TEST_ASSERT_TRUE(MifareKeyTable::parse("A0:A1:A2:A3:A4:A5", out));
    TEST_ASSERT_EQUAL(0xA0, out[0]);
    TEST_ASSERT_EQUAL(0xA5, out[5]);

    // A short key must be rejected, not zero-padded: padding would authenticate
    // with a key the operator never asked for and report a false hit.
    TEST_ASSERT_FALSE(MifareKeyTable::parse("FFFFFFFF", out));
    // A long key must be rejected, not truncated, for the same reason.
    TEST_ASSERT_FALSE(MifareKeyTable::parse("FFFFFFFFFFFF00", out));
    TEST_ASSERT_FALSE(MifareKeyTable::parse("FFFFFFFFFFFFZ", out));
    TEST_ASSERT_FALSE(MifareKeyTable::parse("", out));
}

void test_key_index_lookup_finds_known_and_reports_missing() {
    TEST_ASSERT_EQUAL(0, MifareKeyTable::indexOf(key("FFFFFFFFFFFF")));
    TEST_ASSERT_EQUAL(23, MifareKeyTable::indexOf(key("D3F7D3F7D3F7")));
    TEST_ASSERT_EQUAL(MifareKeyTable::kNotFound, MifareKeyTable::indexOf(key("010203040506")));
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void test_geometry_matches_the_three_family_sizes() {
    TEST_ASSERT_EQUAL(MifareSize::Mini, MifareClassicModel::sizeForBlockCount(20));
    TEST_ASSERT_EQUAL(MifareSize::Classic1K, MifareClassicModel::sizeForBlockCount(64));
    TEST_ASSERT_EQUAL(MifareSize::Classic4K, MifareClassicModel::sizeForBlockCount(256));
    TEST_ASSERT_EQUAL(MifareSize::Unknown, MifareClassicModel::sizeForBlockCount(65));

    TEST_ASSERT_EQUAL(5, MifareClassicModel::sectorCount(MifareSize::Mini));
    TEST_ASSERT_EQUAL(16, MifareClassicModel::sectorCount(MifareSize::Classic1K));
    TEST_ASSERT_EQUAL(40, MifareClassicModel::sectorCount(MifareSize::Classic4K));
    TEST_ASSERT_EQUAL(256, MifareClassicModel::blockCount(MifareSize::Classic4K));
}

void test_four_k_sectors_thirty_two_and_up_are_four_times_larger() {
    MifareSectorLayout layout;

    TEST_ASSERT_TRUE(MifareClassicModel::sectorLayout(MifareSize::Classic4K, 31, layout));
    TEST_ASSERT_EQUAL(124, layout.firstBlock);
    TEST_ASSERT_EQUAL(4, layout.blockCount);
    TEST_ASSERT_EQUAL(127, layout.trailerBlock);

    TEST_ASSERT_TRUE(MifareClassicModel::sectorLayout(MifareSize::Classic4K, 32, layout));
    TEST_ASSERT_EQUAL(128, layout.firstBlock);
    TEST_ASSERT_EQUAL(16, layout.blockCount);
    TEST_ASSERT_EQUAL(143, layout.trailerBlock);

    TEST_ASSERT_TRUE(MifareClassicModel::sectorLayout(MifareSize::Classic4K, 39, layout));
    TEST_ASSERT_EQUAL(240, layout.firstBlock);
    TEST_ASSERT_EQUAL(255, layout.trailerBlock);

    // Out of range must be refused, not clamped onto a real sector.
    TEST_ASSERT_FALSE(MifareClassicModel::sectorLayout(MifareSize::Classic4K, 40, layout));
    TEST_ASSERT_FALSE(MifareClassicModel::sectorLayout(MifareSize::Classic1K, 16, layout));
}

void test_block_to_sector_mapping_is_identity_on_trailers() {
    TEST_ASSERT_EQUAL(0, MifareClassicModel::sectorOfBlock(MifareSize::Classic1K, 3));
    TEST_ASSERT_EQUAL(15, MifareClassicModel::sectorOfBlock(MifareSize::Classic1K, 63));
    TEST_ASSERT_EQUAL(31, MifareClassicModel::sectorOfBlock(MifareSize::Classic4K, 127));
    TEST_ASSERT_EQUAL(32, MifareClassicModel::sectorOfBlock(MifareSize::Classic4K, 128));
    TEST_ASSERT_EQUAL(39, MifareClassicModel::sectorOfBlock(MifareSize::Classic4K, 255));

    TEST_ASSERT_TRUE(MifareClassicModel::isTrailerBlock(MifareSize::Classic1K, 3));
    TEST_ASSERT_FALSE(MifareClassicModel::isTrailerBlock(MifareSize::Classic1K, 4));
    TEST_ASSERT_TRUE(MifareClassicModel::isTrailerBlock(MifareSize::Classic4K, 143));
    // A block past the end of the card is not a trailer, it is out of range.
    TEST_ASSERT_FALSE(MifareClassicModel::isTrailerBlock(MifareSize::Classic1K, 64));
}

// ---------------------------------------------------------------------------
// Access bits: the factory default is the pinned regression vector
// ---------------------------------------------------------------------------

void test_factory_transport_access_bits_decode_exactly() {
    // docs/protocol-evidence/mifare-classic.md §1.2: the worked example over
    // byte6=FF byte7=07 byte8=80 must yield C1=0x0 C2=0x0 C3=0x8, and the three
    // inverted/normal redundancy checks must all pass. If anyone edits the bit
    // packing this test is the tripwire.
    const std::array<uint8_t, kMifareAccessByteCount>& bytes = MifareClassicModel::transportAccessBytes();
    TEST_ASSERT_EQUAL(0xFF, bytes[0]);
    TEST_ASSERT_EQUAL(0x07, bytes[1]);
    TEST_ASSERT_EQUAL(0x80, bytes[2]);
    TEST_ASSERT_EQUAL(0x69, bytes[3]);

    MifareAccessBits bits;
    TEST_ASSERT_TRUE(MifareClassicModel::decodeAccessBits(bytes, bits));
    TEST_ASSERT_TRUE(bits.consistent);
    TEST_ASSERT_EQUAL(0x0, bits.c1);
    TEST_ASSERT_EQUAL(0x0, bits.c2);
    TEST_ASSERT_EQUAL(0x8, bits.c3);
}

void test_factory_transport_grants_full_data_access() {
    const std::array<uint8_t, kMifareAccessByteCount>& bytes = MifareClassicModel::transportAccessBytes();
    MifareAccessBits bits;
    TEST_ASSERT_TRUE(MifareClassicModel::decodeAccessBits(bytes, bits));

    // Data blocks 0..2 decode to 000: read/write/increment/decrement with KeyA
    // or KeyB. This matches the published description of the transport config.
    for (uint8_t block = 0; block < 3; ++block) {
        const MifareBlockPermission permission = MifareClassicModel::blockPermission(bits, block);
        TEST_ASSERT_EQUAL(MifareKeyAccess::KeyAOrB, permission.read);
        TEST_ASSERT_EQUAL(MifareKeyAccess::KeyAOrB, permission.write);
        TEST_ASSERT_EQUAL(MifareKeyAccess::KeyAOrB, permission.increment);
        TEST_ASSERT_EQUAL(MifareKeyAccess::KeyAOrB, permission.decrement);
    }

    // The trailer block decodes to 001: Key A is never readable and is
    // write-only; the access bits are readable and writable with Key A; Key B
    // is readable and writable with Key A. Note every column is *Key A only* --
    // a trailer never accepts Key B to write Key A, so these must not be
    // reported as KeyAOrB.
    const MifareTrailerPermission trailer = MifareClassicModel::trailerPermission(bits);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailer.keyARead);
    TEST_ASSERT_EQUAL(MifareKeyAccess::KeyA, trailer.keyAWrite);
    TEST_ASSERT_EQUAL(MifareKeyAccess::KeyA, trailer.accessBitsRead);
    TEST_ASSERT_EQUAL(MifareKeyAccess::KeyA, trailer.accessBitsWrite);
    TEST_ASSERT_EQUAL(MifareKeyAccess::KeyA, trailer.keyBRead);
    TEST_ASSERT_EQUAL(MifareKeyAccess::KeyA, trailer.keyBWrite);

    TEST_ASSERT_EQUAL(MifareTrailerKeyBAccess::Readable, MifareClassicModel::keyBAccess(bits));
    TEST_ASSERT_FALSE(MifareClassicModel::locksSectorPermanently(bits));

    // Block 3 is the trailer, so the data-block table must not answer for it.
    const MifareBlockPermission trailerAsData = MifareClassicModel::blockPermission(bits, 3);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailerAsData.read);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailerAsData.write);
}

void test_inconsistent_access_bits_are_rejected_rather_than_guessed() {
    // All-zero access bytes break the inverted/normal redundancy, so no
    // permission may be reported from them.
    std::array<uint8_t, kMifareAccessByteCount> bytes = {0x00, 0x00, 0x00, 0x00};
    MifareAccessBits bits;
    TEST_ASSERT_FALSE(MifareClassicModel::decodeAccessBits(bytes, bits));
    TEST_ASSERT_FALSE(bits.consistent);

    const MifareBlockPermission permission = MifareClassicModel::blockPermission(bits, 0);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, permission.read);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, permission.write);
    TEST_ASSERT_EQUAL(MifareTrailerKeyBAccess::Invalid, MifareClassicModel::keyBAccess(bits));
    TEST_ASSERT_FALSE(MifareClassicModel::locksSectorPermanently(bits));

    // Flipping one nibble of a valid pair must also be caught.
    std::array<uint8_t, kMifareAccessByteCount> tampered = MifareClassicModel::transportAccessBytes();
    tampered[0] = 0xFE;
    TEST_ASSERT_FALSE(MifareClassicModel::decodeAccessBits(tampered, bits));
}

void test_a_fully_locked_sector_is_reported_as_permanently_locked() {
    // Trailer triplet 111 -> every write path is "never". Construct the bytes
    // from the packing rules rather than copying a magic constant.
    //   byte6 = ~C2 << 4 | ~C1   with C1 = C2 = 0xF  -> 0x00
    //   byte7 =  C1 << 4 | ~C3   with C3 = 0xF      -> 0xF0
    //   byte8 =  C3 << 4 |  C2                       -> 0xFF
    const std::array<uint8_t, kMifareAccessByteCount> bytes = {0x00, 0xF0, 0xFF, 0x69};
    MifareAccessBits bits;
    TEST_ASSERT_TRUE(MifareClassicModel::decodeAccessBits(bytes, bits));
    TEST_ASSERT_EQUAL(0xF, bits.c1);
    TEST_ASSERT_EQUAL(0xF, bits.c2);
    TEST_ASSERT_EQUAL(0xF, bits.c3);

    TEST_ASSERT_TRUE(MifareClassicModel::locksSectorPermanently(bits));
    TEST_ASSERT_EQUAL(MifareTrailerKeyBAccess::NotReadable, MifareClassicModel::keyBAccess(bits));

    const MifareBlockPermission data = MifareClassicModel::blockPermission(bits, 1);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, data.read);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, data.write);

    const MifareTrailerPermission trailer = MifareClassicModel::trailerPermission(bits);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailer.keyAWrite);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailer.keyBWrite);
    TEST_ASSERT_EQUAL(MifareKeyAccess::None, trailer.accessBitsWrite);
}

// ---------------------------------------------------------------------------
// Sector trailer build/parse
// ---------------------------------------------------------------------------

void test_trailer_round_trips_through_build_and_parse() {
    std::array<uint8_t, kMifareBlockSize> block{};
    MifareClassicModel::buildTrailer(key("A0A1A2A3A4A5"),
                                     key("B0B1B2B3B4B5"),
                                     MifareClassicModel::transportAccessBytes(),
                                     block);

    TEST_ASSERT_EQUAL(0xA0, block[0]);
    TEST_ASSERT_EQUAL(0xA5, block[5]);
    TEST_ASSERT_EQUAL(0xFF, block[6]);
    TEST_ASSERT_EQUAL(0x07, block[7]);
    TEST_ASSERT_EQUAL(0x80, block[8]);
    TEST_ASSERT_EQUAL(0x69, block[9]);
    TEST_ASSERT_EQUAL(0xB0, block[10]);
    TEST_ASSERT_EQUAL(0xB5, block[15]);

    const MifareTrailer trailer = MifareClassicModel::parseTrailer(block);
    TEST_ASSERT_TRUE(trailer.accessValid);
    TEST_ASSERT_EQUAL_STRING("A0A1A2A3A4A5", MifareKeyTable::toString(trailer.keyA).c_str());
    TEST_ASSERT_EQUAL_STRING("B0B1B2B3B4B5", MifareKeyTable::toString(trailer.keyB).c_str());
    TEST_ASSERT_EQUAL(MifareTrailerKeyBAccess::Readable, MifareClassicModel::keyBAccess(trailer.access));
}

void test_key_b_is_reported_unreadable_when_the_trailer_forbids_it() {
    // C1C2C3 (trailer) = 111 forbids every Key B operation, so the UI must say
    // "not readable" instead of showing six zero bytes as if they were the key.
    std::array<uint8_t, kMifareBlockSize> block{};
    const std::array<uint8_t, kMifareAccessByteCount> locked = {0x00, 0xF0, 0xFF, 0x69};
    MifareClassicModel::buildTrailer(key("FFFFFFFFFFFF"), key("000000000000"), locked, block);
    const MifareTrailer trailer = MifareClassicModel::parseTrailer(block);
    TEST_ASSERT_TRUE(trailer.accessValid);
    TEST_ASSERT_EQUAL(MifareTrailerKeyBAccess::NotReadable, MifareClassicModel::keyBAccess(trailer.access));
}

// ---------------------------------------------------------------------------
// Magic card write gate
// ---------------------------------------------------------------------------

void test_magic_write_requires_both_detection_and_confirmation() {
    // A genuine card must never be reachable through the back-door path.
    TEST_ASSERT_FALSE(MifareClassicModel::magicWriteAllowed(false, false));
    TEST_ASSERT_FALSE(MifareClassicModel::magicWriteAllowed(false, true));
    TEST_ASSERT_FALSE(MifareClassicModel::magicWriteAllowed(true, false));
    TEST_ASSERT_TRUE(MifareClassicModel::magicWriteAllowed(true, true));
}

// ---------------------------------------------------------------------------
// Dump assembly
// ---------------------------------------------------------------------------

void test_dump_assembly_reports_only_what_was_read() {
    MifareDump dump;
    const std::array<uint8_t, 10> uid = {0x04, 0x12, 0x34, 0x56, 0x74, 0x08, 0x04, 0x00, 0x00, 0x00};
    TEST_ASSERT_TRUE(MifareClassicModel::beginDump(
        dump, MifareSize::Classic1K, 4, uid, 0x0004, 0x08));

    TEST_ASSERT_EQUAL(64, dump.blocks.size());
    TEST_ASSERT_EQUAL(16, dump.totalSectors);
    TEST_ASSERT_EQUAL(0, dump.recoveredSectors);
    TEST_ASSERT_FALSE(dump.complete());
    // A freshly begun dump has read nothing, so every block is unknown. It must
    // not look like a card of zeroes.
    TEST_ASSERT_EQUAL(64, MifareClassicModel::unknownBlockCount(dump));
    TEST_ASSERT_EQUAL(0x56341204u, MifareClassicModel::uid32(dump));

    std::array<uint8_t, kMifareBlockSize> data{};
    data[0] = 0xAA;
    TEST_ASSERT_TRUE(MifareClassicModel::setBlock(dump, 4, data));
    TEST_ASSERT_EQUAL(63, MifareClassicModel::unknownBlockCount(dump));
    TEST_ASSERT_EQUAL(0xAA, dump.blocks[4].data[0]);

    TEST_ASSERT_TRUE(MifareClassicModel::markBlockUnknown(dump, 4));
    TEST_ASSERT_EQUAL(64, MifareClassicModel::unknownBlockCount(dump));

    // Out-of-range writes are refused rather than wrapping onto another block.
    TEST_ASSERT_FALSE(MifareClassicModel::setBlock(dump, 64, data));
    TEST_ASSERT_FALSE(MifareClassicModel::markBlockUnknown(dump, 64));
}

void test_a_sector_counts_once_even_when_both_keys_are_recovered() {
    MifareDump dump;
    const std::array<uint8_t, 10> uid = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_TRUE(MifareClassicModel::beginDump(
        dump, MifareSize::Classic1K, 4, uid, 0x0004, 0x08));

    TEST_ASSERT_TRUE(MifareClassicModel::setSectorKeyA(dump, 0, key("FFFFFFFFFFFF")));
    TEST_ASSERT_EQUAL(1, dump.recoveredSectors);
    // Key B on the same sector must not bump the counter to 2 of 16.
    TEST_ASSERT_TRUE(MifareClassicModel::setSectorKeyB(dump, 0, key("FFFFFFFFFFFF")));
    TEST_ASSERT_EQUAL(1, dump.recoveredSectors);
    TEST_ASSERT_TRUE(MifareClassicModel::setSectorKeyA(dump, 1, key("FFFFFFFFFFFF")));
    TEST_ASSERT_EQUAL(2, dump.recoveredSectors);

    TEST_ASSERT_FALSE(MifareClassicModel::setSectorKeyA(dump, 16, key("FFFFFFFFFFFF")));
    TEST_ASSERT_EQUAL(2, dump.recoveredSectors);
}

void test_recovery_summary_never_claims_more_than_was_read() {
    MifareDump dump;
    const std::array<uint8_t, 10> uid = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_TRUE(MifareClassicModel::beginDump(
        dump, MifareSize::Classic1K, 4, uid, 0x0004, 0x08));
    TEST_ASSERT_EQUAL_STRING("0 of 16 sectors recovered, 64 block(s) unread",
                             MifareClassicModel::recoverySummary(dump).c_str());

    for (uint8_t sector = 0; sector < 16; ++sector) {
        TEST_ASSERT_TRUE(MifareClassicModel::setSectorKeyA(dump, sector, key("FFFFFFFFFFFF")));
    }
    TEST_ASSERT_TRUE(dump.complete());
    TEST_ASSERT_EQUAL_STRING("16 of 16 sectors recovered, 64 block(s) unread",
                             MifareClassicModel::recoverySummary(dump).c_str());

    TEST_ASSERT_FALSE(MifareClassicModel::beginDump(
        dump, MifareSize::Unknown, 4, uid, 0x0004, 0x08));
    TEST_ASSERT_FALSE(MifareClassicModel::beginDump(
        dump, MifareSize::Classic1K, 0, uid, 0x0004, 0x08));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_key_table_is_the_sourced_twenty_nine);
    RUN_TEST(test_named_keys_match_their_documented_values);
    RUN_TEST(test_key_parsing_accepts_tool_formats_and_rejects_short_or_long);
    RUN_TEST(test_key_index_lookup_finds_known_and_reports_missing);
    RUN_TEST(test_geometry_matches_the_three_family_sizes);
    RUN_TEST(test_four_k_sectors_thirty_two_and_up_are_four_times_larger);
    RUN_TEST(test_block_to_sector_mapping_is_identity_on_trailers);
    RUN_TEST(test_factory_transport_access_bits_decode_exactly);
    RUN_TEST(test_factory_transport_grants_full_data_access);
    RUN_TEST(test_inconsistent_access_bits_are_rejected_rather_than_guessed);
    RUN_TEST(test_a_fully_locked_sector_is_reported_as_permanently_locked);
    RUN_TEST(test_trailer_round_trips_through_build_and_parse);
    RUN_TEST(test_key_b_is_reported_unreadable_when_the_trailer_forbids_it);
    RUN_TEST(test_magic_write_requires_both_detection_and_confirmation);
    RUN_TEST(test_dump_assembly_reports_only_what_was_read);
    RUN_TEST(test_a_sector_counts_once_even_when_both_keys_are_recovered);
    RUN_TEST(test_recovery_summary_never_claims_more_than_was_read);
    return UNITY_END();
}
