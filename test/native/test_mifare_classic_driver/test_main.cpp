// Host suite for MifareClassicDriver -- the MIFARE Classic session over a
// scripted IRfidTransport.
//
// What this pins, in the order the driver has to get it right:
//
//   1. the card type decides the geometry, and a card with no Classic sectors
//      advertises none;
//   2. authentication is per sector, and a read outside the authenticated
//      sector is refused rather than attempted;
//   3. a sector is recovered only when its authentication succeeded, and a
//      block that could not be read is recorded as unknown rather than zeros;
//   4. every write the card cannot accept is refused BEFORE the frame is sent.
//
// (4) is the one that matters. A MIFARE NAK on a write is indistinguishable
// from a card that left the field, so "the card said no" and "we never asked"
// have to be decisions taken on this side. FakeRfidTransport's request log is
// what makes that assertable: a refusal with no frame sent is a different
// claim from a refusal that produced one, and only the log tells them apart.

#include <unity.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "features/rfid/Iso14443aFraming.h"
#include "features/rfid/MifareClassicDriver.h"
#include "features/rfid/MifareClassicModel.h"
#include "FakeRfidTransport.h"

namespace {

constexpr uint8_t kSakMifare1K = 0x08;
constexpr uint8_t kSakMifare4K = 0x18;
constexpr uint8_t kSakUltralight = 0x00;

constexpr uint8_t kMifareCommandAuthenticateA = 0x60;
constexpr uint8_t kMifareCommandRead = 0x30;
constexpr uint8_t kMifareCommandWrite = 0xA0;

using Block = std::array<uint8_t, kMifareBlockSize>;
using Key = std::array<uint8_t, kMifareKeySize>;

const Key& factoryKey() {
    static const Key key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return key;
}

uint8_t invertNibble(uint8_t nibble) {
    return static_cast<uint8_t>((~nibble) & 0x0FU);
}

// Access bytes for a chosen (C1, C2, C3) triplet, using the packing documented
// in MifareClassicModel::decodeAccessBits:
//   byte 6 = ~C2 << 4 | ~C1
//   byte 7 =  C1 << 4 | ~C3
//   byte 8 =  C3 << 4 |  C2
// Building the bytes here rather than pasting literals means the test states
// the permission it wants in the datasheet's own terms.
std::array<uint8_t, kMifareAccessByteCount> accessBytesFor(uint8_t c1, uint8_t c2, uint8_t c3) {
    std::array<uint8_t, kMifareAccessByteCount> bytes{};
    bytes[0] = static_cast<uint8_t>((invertNibble(c2) << 4) | invertNibble(c1));
    bytes[1] = static_cast<uint8_t>((c1 << 4) | invertNibble(c3));
    bytes[2] = static_cast<uint8_t>((c3 << 4) | c2);
    bytes[3] = 0x69;  // reserved; decodeAccessBits does not read it
    return bytes;
}

Block trailerBlockFor(uint8_t c1, uint8_t c2, uint8_t c3) {
    Block block{};
    MifareClassicModel::buildTrailer(factoryKey(), factoryKey(), accessBytesFor(c1, c2, c3), block);
    return block;
}

// A data block whose write permission is Key B only, so an attempt with Key A
// has to be refused on this side.
constexpr uint8_t kC1WriteKeyBOnly = 0x0F;  // 100 on every block
constexpr uint8_t kC2WriteKeyBOnly = 0x00;
constexpr uint8_t kC3WriteKeyBOnly = 0x00;

// Trailer triplet 111 on every block, including the trailer itself: the sector
// can never be re-keyed or unlocked again.
constexpr uint8_t kC1PermanentlyLocked = 0x0F;
constexpr uint8_t kC2PermanentlyLocked = 0x0F;
constexpr uint8_t kC3PermanentlyLocked = 0x0F;

void activateCard(FakeRfidTransport& transport, uint8_t sak) {
    transport.setTarget(0x0004, sak, {0x04, 0x12, 0x34, 0x56});
    transport.activateResult = RfidStatus::Ok;
}

// An Ultralight / NTAG answers ATQA 0044 with SAK 00. The SAK alone does not
// identify it -- the low ATQA byte is what separates that row from a card the
// classifier declines to name.
void activateTag(FakeRfidTransport& transport) {
    transport.setTarget(0x0044, 0x00, {0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC});
    transport.activateResult = RfidStatus::Ok;
}

std::array<uint8_t, kRfidMaxUid> uidBytes() {
    std::array<uint8_t, kRfidMaxUid> uid{};
    uid[0] = 0x04;
    uid[1] = 0x12;
    uid[2] = 0x34;
    uid[3] = 0x56;
    return uid;
}

MifareDump begunDump() {
    MifareDump dump;
    MifareClassicModel::beginDump(dump, MifareSize::Classic1K, 4, uidBytes(), 0x0004,
                                  kSakMifare1K);
    return dump;
}

// Whether any frame the driver sent was a write command.
bool sentAWriteFrame(const FakeRfidTransport& transport) {
    for (size_t i = 0; i < transport.requestCount(); ++i) {
        if (!transport.requestAt(i).empty() &&
            transport.requestAt(i)[0] == kMifareCommandWrite) {
            return true;
        }
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// Activation and geometry
// ---------------------------------------------------------------------------

void test_card_type_decides_the_geometry() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);

    TEST_ASSERT_EQUAL(RfidStatus::Ok, driver.activate(200));
    TEST_ASSERT_TRUE(driver.active());
    TEST_ASSERT_EQUAL(Iso14443aCardType::MifareClassic1K, driver.cardType());
    TEST_ASSERT_EQUAL(MifareSize::Classic1K, driver.size());
    TEST_ASSERT_TRUE(driver.supportsSectors());
    TEST_ASSERT_EQUAL_STRING("MIFARE Classic 1K", driver.cardTypeLabel());

    FakeRfidTransport transport4k;
    MifareClassicDriver driver4k(transport4k);
    activateCard(transport4k, kSakMifare4K);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driver4k.activate(200));
    TEST_ASSERT_EQUAL(MifareSize::Classic4K, driver4k.size());

    // A tag with no MIFARE Classic sectors must advertise no geometry at all.
    // The dump screens key off supportsSectors() to refuse it, so a default of
    // 1K here is what would make a 4K-sized walk run against an NTAG.
    FakeRfidTransport transportTag;
    MifareClassicDriver driverTag(transportTag);
    activateTag(transportTag);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driverTag.activate(200));
    TEST_ASSERT_EQUAL(Iso14443aCardType::UltralightOrNtag, driverTag.cardType());
    TEST_ASSERT_EQUAL(MifareSize::Unknown, driverTag.size());
    TEST_ASSERT_FALSE(driverTag.supportsSectors());

    // SAK 00 with a Classic ATQA is not an NTAG. The classifier stays coarse
    // rather than guessing a capacity the card never reported.
    FakeRfidTransport transportAmbiguous;
    MifareClassicDriver driverAmbiguous(transportAmbiguous);
    activateCard(transportAmbiguous, kSakUltralight);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driverAmbiguous.activate(200));
    TEST_ASSERT_EQUAL(Iso14443aCardType::Unknown, driverAmbiguous.cardType());
    TEST_ASSERT_EQUAL(MifareSize::Unknown, driverAmbiguous.size());
}

void test_an_empty_field_is_reported_not_assumed() {
    FakeRfidTransport transport;
    transport.activateResult = RfidStatus::NoTarget;
    MifareClassicDriver driver(transport);

    TEST_ASSERT_EQUAL(RfidStatus::NoTarget, driver.activate(200));
    TEST_ASSERT_FALSE(driver.active());
    TEST_ASSERT_EQUAL(MifareSize::Unknown, driver.size());
    TEST_ASSERT_FALSE(driver.supportsSectors());
}

void test_reactivating_drops_the_previous_target() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);

    TEST_ASSERT_EQUAL(RfidStatus::Ok, driver.activate(200));
    TEST_ASSERT_EQUAL(0, transport.releaseCalls);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driver.activate(200));
    // The second activation released the first target rather than leaving two
    // selections live on one reader.
    TEST_ASSERT_EQUAL(1, transport.releaseCalls);
}

void test_release_forgets_the_authenticated_sector() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    transport.queueOk({});
    TEST_ASSERT_TRUE(driver.authenticate(0, MifareKeySlot::KeyA, factoryKey()).authenticated);
    TEST_ASSERT_EQUAL(0, driver.authenticatedSector());

    driver.release();
    TEST_ASSERT_FALSE(driver.active());
    TEST_ASSERT_EQUAL(-1, driver.authenticatedSector());
    TEST_ASSERT_EQUAL(1, transport.releaseCalls);

    // Releasing twice must not poke the reader again.
    driver.release();
    TEST_ASSERT_EQUAL(1, transport.releaseCalls);
}

// ---------------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------------

void test_authentication_is_tracked_per_sector() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // Success carries no ACK byte: the reader runs Crypto1 and reports the
    // verdict as its own status. Looking for a 0x0A nibble here would report
    // every successful authentication as a failure.
    transport.queueOk({});
    const MifareAuthResult ok = driver.authenticate(4, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_TRUE(ok.authenticated);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, ok.status);
    TEST_ASSERT_EQUAL(MifareKeySlot::KeyA, ok.slot);
    TEST_ASSERT_EQUAL(1, driver.authenticatedSector());

    // The frame really was an authentication: command, block, then the key.
    TEST_ASSERT_EQUAL(1, transport.requestCount());
    TEST_ASSERT_EQUAL(8, transport.requestAt(0).size());
    TEST_ASSERT_EQUAL_UINT8(kMifareCommandAuthenticateA, transport.requestAt(0)[0]);
    TEST_ASSERT_EQUAL_UINT8(0x04U, transport.requestAt(0)[1]);

    // A key the card rejects moves the tracked sector back to none.
    transport.queueStatus(RfidStatus::AuthFailed);
    const MifareAuthResult rejected =
        driver.authenticate(4, MifareKeySlot::KeyB, factoryKey());
    TEST_ASSERT_FALSE(rejected.authenticated);
    TEST_ASSERT_EQUAL(RfidStatus::AuthFailed, rejected.status);
    TEST_ASSERT_EQUAL(-1, driver.authenticatedSector());
    // 0x61 is the Key B form of the same command.
    TEST_ASSERT_EQUAL_UINT8(0x61U, transport.requestAt(1)[0]);
}

void test_authentication_needs_a_selected_card() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);

    const MifareAuthResult auth = driver.authenticate(0, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(auth.authenticated);
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget, auth.status);

    const MifareAuthResult walked = driver.authenticateSectorWithTable(0);
    TEST_ASSERT_FALSE(walked.authenticated);
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget, walked.status);

    // Neither call reached the reader.
    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);
}

void test_the_table_walk_refuses_a_card_with_no_sectors() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakUltralight);
    driver.activate(200);

    const MifareAuthResult walked = driver.authenticateSectorWithTable(0);
    TEST_ASSERT_FALSE(walked.authenticated);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, walked.status);
    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);
}

// ---------------------------------------------------------------------------
// Reads
// ---------------------------------------------------------------------------

void test_read_is_refused_outside_the_authenticated_sector() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // Nothing authenticated yet: a read is a caller mistake, and saying so
    // beats a generic read error that reads like a hardware fault.
    Block data{};
    const MifareReadResult tooEarly = driver.readBlock(1, data);
    TEST_ASSERT_EQUAL(RfidStatus::AuthFailed, tooEarly.status);
    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);

    transport.queueOk({});
    TEST_ASSERT_TRUE(driver.authenticate(0, MifareKeySlot::KeyA, factoryKey()).authenticated);

    const uint8_t payload[kMifareBlockSize] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                               0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    transport.queueOk(payload, kMifareBlockSize);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driver.readBlock(1, data).status);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(data.data(), payload, kMifareBlockSize));
    TEST_ASSERT_EQUAL_UINT8(kMifareCommandRead, transport.lastRequest()[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, transport.lastRequest()[1]);

    // Block 5 lives in sector 1. MIFARE does not carry authentication across
    // sectors, so this is refused without a frame going out.
    const int framesSoFar = transport.transceiveCalls;
    TEST_ASSERT_EQUAL(RfidStatus::AuthFailed, driver.readBlock(5, data).status);
    TEST_ASSERT_EQUAL(framesSoFar, transport.transceiveCalls);
}

void test_a_short_block_reply_is_a_framing_error() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    transport.queueOk({});
    driver.authenticate(0, MifareKeySlot::KeyA, factoryKey());

    // MIFARE blocks are fixed at 16 bytes, so four bytes back is a framing
    // fault rather than a successful small read.
    transport.queueOk({0x01, 0x02, 0x03, 0x04});
    Block data{};
    data.fill(0xEE);
    const MifareReadResult shortReply = driver.readBlock(1, data);
    TEST_ASSERT_EQUAL(RfidStatus::FrameError, shortReply.status);
    for (uint8_t i = 0; i < kMifareBlockSize; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0x00U, data[i]);
    }
}

void test_a_block_outside_the_card_is_refused() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    transport.queueOk({});
    driver.authenticate(0, MifareKeySlot::KeyA, factoryKey());

    Block data{};
    // A 1K card has 64 blocks; 64 is off the end.
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, driver.readBlock(64, data).status);
    TEST_ASSERT_EQUAL(1, transport.transceiveCalls);
}

// ---------------------------------------------------------------------------
// Writes -- the refusals have to happen before the frame goes out
// ---------------------------------------------------------------------------

void test_write_refuses_block_zero_and_trailers_before_sending() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    Block data{};
    data.fill(0xAB);

    // Block 0 is the manufacturer block: read-only on any genuine card.
    const MifareWriteResult manufacturer =
        driver.writeBlock(0, data, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(manufacturer.written);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, manufacturer.status);

    // A trailer goes through writeTrailer(), which checks the lock state first.
    const MifareWriteResult trailer =
        driver.writeBlock(3, data, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(trailer.written);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, trailer.status);

    // Nothing was sent for either: no authentication, no write probe.
    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);
    TEST_ASSERT_FALSE(sentAWriteFrame(transport));
}

void test_write_is_refused_when_the_access_bits_forbid_it() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // Every block in this sector is 100: readable with either key, writable
    // with Key B only. We authenticate with Key A.
    const Block trailer =
        trailerBlockFor(kC1WriteKeyBOnly, kC2WriteKeyBOnly, kC3WriteKeyBOnly);
    transport.queueOk({});                              // authenticate
    transport.queueOk(trailer.data(), kMifareBlockSize);  // read the trailer

    Block data{};
    data.fill(0x5A);
    const MifareWriteResult result =
        driver.writeBlock(1, data, MifareKeySlot::KeyA, factoryKey());

    TEST_ASSERT_FALSE(result.written);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, result.status);
    TEST_ASSERT_NOT_NULL(std::strstr(result.detail, "access bits"));
    // Two frames went out -- the authentication and the trailer read -- and
    // neither was a write.
    TEST_ASSERT_EQUAL(2, transport.transceiveCalls);
    TEST_ASSERT_EQUAL_UINT8(kMifareCommandRead, transport.requestAt(1)[0]);
    TEST_ASSERT_FALSE(sentAWriteFrame(transport));
}

void test_a_write_the_card_does_not_store_is_reported_as_a_failure() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    const Block trailer = trailerBlockFor(0x00, 0x00, 0x08);
    Block data{};
    for (uint8_t i = 0; i < kMifareBlockSize; ++i) {
        data[i] = static_cast<uint8_t>(0xA0U + i);
    }
    Block stale = data;
    stale[0] ^= 0xFFU;  // what the card actually holds afterwards

    transport.queueOk({});                                // authenticate
    transport.queueOk(trailer.data(), kMifareBlockSize);   // trailer read
    transport.queueOk({});                                // write acknowledged
    transport.queueOk(stale.data(), kMifareBlockSize);     // read-back disagrees

    const MifareWriteResult result =
        driver.writeBlock(1, data, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(result.written);
    TEST_ASSERT_EQUAL(RfidStatus::IoError, result.status);
    TEST_ASSERT_TRUE(sentAWriteFrame(transport));
}

void test_a_write_is_read_back_and_confirmed() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // The factory transport trailer, spelled out here so a change to the
    // packing constants shows up as this test failing rather than as a
    // permission table that quietly means something else.
    const Block trailer = trailerBlockFor(0x00, 0x00, 0x08);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, trailer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x07U, trailer[7]);
    TEST_ASSERT_EQUAL_UINT8(0x80U, trailer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x69U, trailer[9]);

    Block data{};
    for (uint8_t i = 0; i < kMifareBlockSize; ++i) {
        data[i] = static_cast<uint8_t>(0xA0U + i);
    }

    transport.queueOk({});                                // authenticate
    transport.queueOk(trailer.data(), kMifareBlockSize);   // trailer read
    transport.queueOk({});                                // write acknowledged
    transport.queueOk(data.data(), kMifareBlockSize);      // read-back agrees

    const MifareWriteResult result =
        driver.writeBlock(1, data, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_TRUE(result.written);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, result.status);
    TEST_ASSERT_EQUAL(4, transport.transceiveCalls);
    TEST_ASSERT_EQUAL_UINT8(kMifareCommandWrite, transport.requestAt(2)[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, transport.requestAt(2)[1]);
    // The write frame carries the block number and the sixteen data bytes.
    TEST_ASSERT_EQUAL(18, transport.requestAt(2).size());
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(transport.requestAt(2).data() + 2, data.data(), 16));
}

// ---------------------------------------------------------------------------
// Trailer writes
// ---------------------------------------------------------------------------

void test_write_trailer_refuses_a_permanently_locked_sector() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // Triplet 111 freezes Key A, Key B and the access bits for good. There is
    // no recovery path, so the write must never reach the card.
    const Block locked =
        trailerBlockFor(kC1PermanentlyLocked, kC2PermanentlyLocked, kC3PermanentlyLocked);
    transport.queueOk({});
    transport.queueOk(locked.data(), kMifareBlockSize);

    MifareTrailer requested;
    requested.keyA = factoryKey();
    requested.keyB = factoryKey();
    requested.accessBytes = MifareClassicModel::transportAccessBytes();

    const MifareWriteResult result =
        driver.writeTrailer(0, requested, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(result.written);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, result.status);
    TEST_ASSERT_NOT_NULL(std::strstr(result.detail, "permanently locked"));
    TEST_ASSERT_EQUAL(2, transport.transceiveCalls);
    TEST_ASSERT_FALSE(sentAWriteFrame(transport));
}

void test_write_trailer_refuses_inconsistent_access_bytes() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // The sector is fine; the bytes we are being asked to install are not. The
    // normal/inverted pairs disagree, which would leave the sector unreadable.
    const Block current = trailerBlockFor(0x00, 0x00, 0x08);
    transport.queueOk({});
    transport.queueOk(current.data(), kMifareBlockSize);

    MifareTrailer requested;
    requested.keyA = factoryKey();
    requested.keyB = factoryKey();
    requested.accessBytes = {0x00, 0x00, 0x00, 0x00};

    const MifareWriteResult result =
        driver.writeTrailer(0, requested, MifareKeySlot::KeyA, factoryKey());
    TEST_ASSERT_FALSE(result.written);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, result.status);
    TEST_ASSERT_EQUAL(2, transport.transceiveCalls);
    TEST_ASSERT_FALSE(sentAWriteFrame(transport));
}

// ---------------------------------------------------------------------------
// The magic-card gate
// ---------------------------------------------------------------------------

void test_magic_uid_write_is_gated_on_both_conditions() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    const std::array<uint8_t, 10> uid = uidBytes();

    // No back door detected: the request never becomes a card operation.
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, driver.writeMagicUid(uid, 4, false, true).status);
    // Back door found but the operator has not accepted the risk.
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, driver.writeMagicUid(uid, 4, true, false).status);
    // Both conditions met, and it still refuses: the gen1A command sequence is
    // behind the evidence gate, so a call site exists but sends nothing.
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, driver.writeMagicUid(uid, 4, true, true).status);
    // A length that cannot be a UID is rejected before the gate is consulted.
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, driver.writeMagicUid(uid, 0, true, true).status);

    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);
}

// ---------------------------------------------------------------------------
// Dumps
// ---------------------------------------------------------------------------

void test_a_sector_opens_only_when_its_authentication_succeeds() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    MifareDump dump = begunDump();
    transport.queueOk({});                       // the table's first key opens sector 0
    for (uint8_t i = 0; i < 4; ++i) {
        transport.queueStatus(RfidStatus::IoError);  // ...but every block read fails
    }

    TEST_ASSERT_EQUAL_UINT8(0, driver.readSectorIntoDump(dump, 0));

    // The key was recovered -- the sector did open -- but no block is claimed.
    // "The key opened the sector" and "the blocks were read" are separate
    // facts, and recording zeros instead would make an unreadable sector look
    // like an erased one.
    TEST_ASSERT_TRUE(dump.sectors[0].keyARecovered);
    TEST_ASSERT_EQUAL_UINT8(1, dump.recoveredSectors);
    for (uint16_t block = 0; block < 4; ++block) {
        TEST_ASSERT_FALSE(dump.blocks[block].known);
    }
    // unknownBlockCount() counts every block the dump did not recover, so
    // with only one sector touched and nothing read it is the whole card.
    TEST_ASSERT_EQUAL_UINT16(64, MifareClassicModel::unknownBlockCount(dump));
}

void test_a_recovered_sector_records_every_block() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    MifareDump dump = begunDump();
    transport.queueOk({});  // authentication
    for (uint8_t block = 0; block < 4; ++block) {
        uint8_t payload[kMifareBlockSize];
        std::memset(payload, static_cast<int>(0x10 + block), kMifareBlockSize);
        transport.queueOk(payload, kMifareBlockSize);
    }

    TEST_ASSERT_EQUAL_UINT8(4, driver.readSectorIntoDump(dump, 0));
    for (uint16_t block = 0; block < 4; ++block) {
        TEST_ASSERT_TRUE(dump.blocks[block].known);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(0x10 + block), dump.blocks[block].data[0]);
    }
    // The other sixty blocks were never attempted, so they are unknown; only
    // sector 0 was recovered. The count describes what the dump holds, not
    // which reads failed.
    TEST_ASSERT_EQUAL_UINT16(60, MifareClassicModel::unknownBlockCount(dump));
}

void test_dump_card_marks_everything_unknown_when_the_card_leaves_the_field() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakMifare1K);
    driver.activate(200);

    // The card is lifted after activation. The table walk has to stop on the
    // first attempt and report the card's absence -- continuing through all 29
    // keys and then claiming "no published key opened this sector" would be a
    // lie the operator cannot act on.
    for (uint8_t sector = 0; sector < 16; ++sector) {
        transport.queueStatus(RfidStatus::NoTarget);
    }

    int progressCalls = 0;
    const MifareDump dump = driver.dumpCard(
        [](const MifareDump&, void* user) { ++*static_cast<int*>(user); }, &progressCalls);

    TEST_ASSERT_EQUAL(16, progressCalls);
    TEST_ASSERT_EQUAL(MifareSize::Classic1K, dump.size);
    TEST_ASSERT_EQUAL_UINT8(4, dump.uidLength);
    TEST_ASSERT_EQUAL_UINT8(0, dump.recoveredSectors);
    TEST_ASSERT_EQUAL_UINT8(16, dump.totalSectors);
    TEST_ASSERT_FALSE(dump.complete());
    TEST_ASSERT_EQUAL_UINT16(64, MifareClassicModel::unknownBlockCount(dump));
}

void test_dump_card_refuses_a_card_with_no_sectors() {
    FakeRfidTransport transport;
    MifareClassicDriver driver(transport);
    activateCard(transport, kSakUltralight);
    driver.activate(200);

    const MifareDump dump = driver.dumpCard();
    TEST_ASSERT_EQUAL(MifareSize::Unknown, dump.size);
    TEST_ASSERT_EQUAL_UINT8(0, dump.totalSectors);
    TEST_ASSERT_FALSE(dump.complete());
    TEST_ASSERT_EQUAL(0, transport.transceiveCalls);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_card_type_decides_the_geometry);
    RUN_TEST(test_an_empty_field_is_reported_not_assumed);
    RUN_TEST(test_reactivating_drops_the_previous_target);
    RUN_TEST(test_release_forgets_the_authenticated_sector);
    RUN_TEST(test_authentication_is_tracked_per_sector);
    RUN_TEST(test_authentication_needs_a_selected_card);
    RUN_TEST(test_the_table_walk_refuses_a_card_with_no_sectors);
    RUN_TEST(test_read_is_refused_outside_the_authenticated_sector);
    RUN_TEST(test_a_short_block_reply_is_a_framing_error);
    RUN_TEST(test_a_block_outside_the_card_is_refused);
    RUN_TEST(test_write_refuses_block_zero_and_trailers_before_sending);
    RUN_TEST(test_write_is_refused_when_the_access_bits_forbid_it);
    RUN_TEST(test_a_write_the_card_does_not_store_is_reported_as_a_failure);
    RUN_TEST(test_a_write_is_read_back_and_confirmed);
    RUN_TEST(test_write_trailer_refuses_a_permanently_locked_sector);
    RUN_TEST(test_write_trailer_refuses_inconsistent_access_bytes);
    RUN_TEST(test_magic_uid_write_is_gated_on_both_conditions);
    RUN_TEST(test_a_sector_opens_only_when_its_authentication_succeeds);
    RUN_TEST(test_a_recovered_sector_records_every_block);
    RUN_TEST(test_dump_card_marks_everything_unknown_when_the_card_leaves_the_field);
    RUN_TEST(test_dump_card_refuses_a_card_with_no_sectors);
    return UNITY_END();
}
