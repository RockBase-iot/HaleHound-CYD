#include "features/rfid/MifareClassicDriver.h"

#include <cstring>

#include "features/rfid/KeyCheckModel.h"  // KeyCheckSession::preferredKeyIndex

namespace {

// The PN532 answers a MIFARE command in tens of milliseconds; a second is
// generous headroom for a card that is being moved by hand.
constexpr uint32_t kAuthTimeoutMs = 500;
constexpr uint32_t kReadTimeoutMs = 500;
constexpr uint32_t kWriteTimeoutMs = 1000;

// SAK says what the card is; the geometry follows from that. Anything not in
// this list stays Unknown, and the dump screens refuse it rather than assuming
// a 1K layout -- the assumption is what makes a 4K dump silently truncate.
MifareSize sizeForCardType(Iso14443aCardType type) {
    switch (type) {
        case Iso14443aCardType::MifareClassic1K:
        case Iso14443aCardType::SmartMxClassic1K:
        case Iso14443aCardType::InfineonClassic1K:
            return MifareSize::Classic1K;
        case Iso14443aCardType::MifareClassic4K:
        case Iso14443aCardType::SmartMxClassic4K:
            return MifareSize::Classic4K;
        case Iso14443aCardType::MifareMini:
            return MifareSize::Mini;
        default:
            return MifareSize::Unknown;
    }
}

}  // namespace

bool MifareClassicDriver::allowsAccess(MifareKeyAccess access, MifareKeySlot slot) {
    switch (access) {
        case MifareKeyAccess::None:
            return false;
        case MifareKeyAccess::KeyA:
            return slot == MifareKeySlot::KeyA;
        case MifareKeyAccess::KeyB:
            return slot == MifareKeySlot::KeyB;
        case MifareKeyAccess::KeyAOrB:
            return true;
        default:
            return false;
    }
}

const char* MifareClassicDriver::cardTypeLabel() const {
    return Iso14443aFraming::cardTypeLabel(cardType_);
}

RfidStatus MifareClassicDriver::activate(uint32_t timeoutMs) {
    release();
    RfidTarget target;
    const RfidStatus status = transport_.activateIso14443a(target, timeoutMs);
    if (status != RfidStatus::Ok) {
        return status;
    }
    target_ = target;
    cardType_ = Iso14443aFraming::classifyCard(target.atqa, target.sak);
    size_ = sizeForCardType(cardType_);
    active_ = true;
    return RfidStatus::Ok;
}

void MifareClassicDriver::release() {
    if (active_) {
        transport_.releaseTarget();
    }
    active_ = false;
    authenticatedSector_ = -1;
    cardType_ = Iso14443aCardType::Unknown;
    size_ = MifareSize::Unknown;
    target_ = RfidTarget{};
}

MifareAuthResult MifareClassicDriver::authenticate(
    uint8_t block,
    MifareKeySlot slot,
    const std::array<uint8_t, kMifareKeySize>& key) {
    MifareAuthResult result;
    result.slot = slot;
    result.key = key;
    if (!active_) {
        result.status = RfidStatus::NoTarget;
        result.detail = "no card selected";
        return result;
    }
    uint8_t frame[8] = {};
    const uint16_t length =
        Iso14443aFraming::buildAuthenticate(slot, block, key.data(), frame);
    if (length == 0U) {
        result.status = RfidStatus::FrameError;
        result.detail = "authentication frame not built";
        return result;
    }
    uint8_t rx[kMifareBlockSize] = {};
    uint16_t rxLength = sizeof(rx);
    const RfidStatus status =
        transport_.transceive(frame, length, rx, rxLength, kAuthTimeoutMs);
    result.status = status;
    result.authenticated = (status == RfidStatus::Ok);
    if (result.authenticated) {
        // Success carries no ACK byte on this transport: the reader performs
        // the Crypto1 exchange itself and reports the verdict as its own status
        // code. Looking for a 0x0A nibble here would report every successful
        // authentication as a failure.
        result.detail = "authenticated";
        const uint8_t sector = MifareClassicModel::sectorOfBlock(size_, block);
        authenticatedSector_ = (sector == 0xFFU) ? -1 : static_cast<int>(sector);
    } else {
        result.detail = rfidStatusLabel(status);
        authenticatedSector_ = -1;
    }
    return result;
}

MifareAuthResult MifareClassicDriver::authenticateSectorWithTable(uint8_t sector) {
    MifareAuthResult result;
    result.status = RfidStatus::NoTarget;
    result.detail = "no card selected";
    if (!active_) {
        return result;
    }
    if (!supportsSectors()) {
        result.status = RfidStatus::Unsupported;
        result.detail = "this card type does not carry MIFARE Classic sectors";
        return result;
    }
    MifareSectorLayout layout;
    if (!MifareClassicModel::sectorLayout(size_, sector, layout)) {
        result.status = RfidStatus::Unsupported;
        result.detail = "sector is outside this card";
        return result;
    }

    for (uint8_t slotIndex = 0; slotIndex < 2U; ++slotIndex) {
        const MifareKeySlot slot =
            (slotIndex == 0U) ? MifareKeySlot::KeyA : MifareKeySlot::KeyB;
        // The NFC Forum key goes first. On a card formatted by a phone that
        // turns a 29-key walk into a single attempt, which is the difference
        // between a dump taking one second and taking ten.
        const uint16_t hint = KeyCheckSession::preferredKeyIndex(slot, sector);
        const bool hintUsable =
            hint != MifareKeyTable::kNotFound && hint < MifareKeyTable::count();
        if (hintUsable) {
            MifareAuthResult attempt =
                authenticate(layout.firstBlock, slot, MifareKeyTable::at(hint).bytes);
            if (attempt.authenticated) {
                attempt.keyIndex = hint;
                return attempt;
            }
            if (attempt.status == RfidStatus::NoTarget ||
                attempt.status == RfidStatus::Timeout) {
                // The card left the field. Reporting "no published key opened
                // this sector" here would be a lie, and the caller has to know
                // to re-activate instead of trusting the result.
                return attempt;
            }
        }
        for (uint16_t index = 0; index < MifareKeyTable::count(); ++index) {
            if (hintUsable && index == hint) {
                continue;
            }
            MifareAuthResult attempt =
                authenticate(layout.firstBlock, slot, MifareKeyTable::at(index).bytes);
            if (attempt.authenticated) {
                attempt.keyIndex = index;
                return attempt;
            }
            if (attempt.status == RfidStatus::NoTarget ||
                attempt.status == RfidStatus::Timeout) {
                return attempt;
            }
        }
    }
    result.status = RfidStatus::AuthFailed;
    result.detail = "no published key opened this sector";
    return result;
}

MifareReadResult MifareClassicDriver::readBlock(
    uint8_t block,
    std::array<uint8_t, kMifareBlockSize>& out) {
    MifareReadResult result;
    out.fill(0);
    if (!active_) {
        result.status = RfidStatus::NoTarget;
        result.detail = "no card selected";
        return result;
    }
    const uint8_t sector = MifareClassicModel::sectorOfBlock(size_, block);
    if (sector == 0xFFU) {
        result.status = RfidStatus::Unsupported;
        result.detail = "block is outside this card";
        return result;
    }
    if (authenticatedSector_ != static_cast<int>(sector)) {
        // MIFARE keeps authentication per sector, so this is a caller mistake,
        // not a card failure. Saying which it is matters: one is a bug, the
        // other is a card to re-seat.
        result.status = RfidStatus::AuthFailed;
        result.detail = "sector not authenticated";
        return result;
    }
    uint8_t frame[2] = {};
    const uint16_t length = Iso14443aFraming::buildReadBlock(block, frame);
    uint8_t rx[kMifareBlockSize] = {};
    uint16_t rxLength = sizeof(rx);
    const RfidStatus status =
        transport_.transceive(frame, length, rx, rxLength, kReadTimeoutMs);
    result.status = status;
    if (status != RfidStatus::Ok) {
        result.detail = rfidStatusLabel(status);
        return result;
    }
    if (rxLength != kMifareBlockSize) {
        // MIFARE blocks are fixed at 16 bytes, so a short reply is a framing
        // fault, not a successful small read.
        result.status = RfidStatus::FrameError;
        result.detail = "card returned the wrong block length";
        return result;
    }
    std::memcpy(out.data(), rx, kMifareBlockSize);
    result.detail = "read";
    return result;
}

bool MifareClassicDriver::readAccessBits(uint8_t sector,
                                        MifareAccessBits& bits,
                                        MifareTrailer& trailer) {
    MifareSectorLayout layout;
    if (!MifareClassicModel::sectorLayout(size_, sector, layout)) {
        return false;
    }
    std::array<uint8_t, kMifareBlockSize> block{};
    if (readBlock(layout.trailerBlock, block).status != RfidStatus::Ok) {
        return false;
    }
    trailer = MifareClassicModel::parseTrailer(block);
    std::array<uint8_t, kMifareAccessByteCount> accessBytes{};
    for (uint8_t i = 0; i < kMifareAccessByteCount; ++i) {
        accessBytes[i] = block[kMifareAccessByteOffset + i];
    }
    return MifareClassicModel::decodeAccessBits(accessBytes, bits);
}

uint8_t MifareClassicDriver::readSectorIntoDump(MifareDump& dump, uint8_t sector) {
    MifareSectorLayout layout;
    if (!MifareClassicModel::sectorLayout(size_, sector, layout)) {
        return 0;
    }
    const MifareAuthResult auth = authenticateSectorWithTable(sector);
    if (!auth.authenticated) {
        for (uint8_t i = 0; i < layout.blockCount; ++i) {
            MifareClassicModel::markBlockUnknown(
                dump, static_cast<uint16_t>(layout.firstBlock + i));
        }
        return 0;
    }
    // Record the key before reading the blocks. A sector whose key opened but
    // whose blocks partly failed is still a sector the operator can return to,
    // and the dump has to say so rather than showing it as untouched.
    if (auth.slot == MifareKeySlot::KeyA) {
        MifareClassicModel::setSectorKeyA(dump, sector, auth.key);
    } else {
        MifareClassicModel::setSectorKeyB(dump, sector, auth.key);
    }
    uint8_t read = 0;
    for (uint8_t i = 0; i < layout.blockCount; ++i) {
        const uint16_t block = static_cast<uint16_t>(layout.firstBlock + i);
        std::array<uint8_t, kMifareBlockSize> data{};
        if (readBlock(static_cast<uint8_t>(block), data).status == RfidStatus::Ok) {
            MifareClassicModel::setBlock(dump, block, data);
            ++read;
        } else {
            MifareClassicModel::markBlockUnknown(dump, block);
        }
    }
    return read;
}

MifareDump MifareClassicDriver::dumpCard(MifareDumpProgress progress, void* user) {
    MifareDump dump;
    if (!active_ || !supportsSectors()) {
        return dump;
    }
    std::array<uint8_t, kRfidMaxUid> uid{};
    std::memcpy(uid.data(), target_.uid.data(), target_.uidLength);
    if (!MifareClassicModel::beginDump(dump, size_, target_.uidLength, uid,
                                       target_.atqa, target_.sak)) {
        return MifareDump{};
    }
    const uint8_t sectors = MifareClassicModel::sectorCount(size_);
    for (uint8_t sector = 0; sector < sectors; ++sector) {
        readSectorIntoDump(dump, sector);
        if (progress != nullptr) {
            progress(dump, user);
        }
    }
    return dump;
}

MifareWriteResult MifareClassicDriver::writeBlock(
    uint8_t block,
    const std::array<uint8_t, kMifareBlockSize>& data,
    MifareKeySlot slot,
    const std::array<uint8_t, kMifareKeySize>& key) {
    MifareWriteResult result;
    if (!active_) {
        result.status = RfidStatus::NoTarget;
        result.detail = "no card selected";
        return result;
    }
    if (!supportsSectors()) {
        result.status = RfidStatus::Unsupported;
        result.detail = "this card type does not carry MIFARE Classic sectors";
        return result;
    }
    if (block == 0U) {
        // The manufacturer block. A normal MIFARE write can never change it,
        // and pretending to attempt it would train the operator to read a NAK
        // as a hardware fault.
        result.status = RfidStatus::Unsupported;
        result.detail = "block 0 is read-only; rewriting the UID needs the magic-card path";
        return result;
    }
    if (MifareClassicModel::isTrailerBlock(size_, block)) {
        result.status = RfidStatus::Unsupported;
        result.detail = "use writeTrailer() for a sector trailer";
        return result;
    }
    const uint8_t sector = MifareClassicModel::sectorOfBlock(size_, block);
    MifareSectorLayout layout;
    if (sector == 0xFFU ||
        !MifareClassicModel::sectorLayout(size_, sector, layout)) {
        result.status = RfidStatus::Unsupported;
        result.detail = "block is outside this card";
        return result;
    }

    const MifareAuthResult auth = authenticate(layout.firstBlock, slot, key);
    if (!auth.authenticated) {
        result.status = auth.status;
        result.detail = "authentication failed; nothing was written";
        return result;
    }

    MifareAccessBits bits;
    MifareTrailer trailer;
    if (!readAccessBits(sector, bits, trailer)) {
        result.status = RfidStatus::IoError;
        result.detail = "could not read the access bits; refusing to write blind";
        return result;
    }
    if (!bits.consistent) {
        result.status = RfidStatus::Unsupported;
        result.detail = "the sector's access bits are inconsistent; refusing to write";
        return result;
    }
    const uint8_t blockInSector = static_cast<uint8_t>(block - layout.firstBlock);
    const MifareBlockPermission permission =
        MifareClassicModel::blockPermission(bits, blockInSector);
    if (!allowsAccess(permission.write, slot)) {
        // Decided here, before the frame goes out. A MIFARE NAK on a write is
        // indistinguishable from a card that left the field, so a blocked write
        // has to be a decision rather than an inference.
        result.status = RfidStatus::Unsupported;
        result.detail = "the access bits forbid this key from writing this block";
        return result;
    }

    uint8_t frame[18] = {};
    const uint16_t length =
        Iso14443aFraming::buildWriteBlock(static_cast<uint8_t>(block), data.data(), frame);
    uint8_t rx[kMifareBlockSize] = {};
    uint16_t rxLength = sizeof(rx);
    const RfidStatus status =
        transport_.transceive(frame, length, rx, rxLength, kWriteTimeoutMs);
    if (status != RfidStatus::Ok) {
        result.status = status;
        result.detail = rfidStatusLabel(status);
        return result;
    }
    // Read it back. A write the card acknowledged but did not apply is
    // otherwise indistinguishable from a successful one.
    std::array<uint8_t, kMifareBlockSize> verify{};
    if (readBlock(static_cast<uint8_t>(block), verify).status != RfidStatus::Ok ||
        verify != data) {
        result.status = RfidStatus::IoError;
        result.detail = "the card did not store what was written";
        return result;
    }
    result.written = true;
    result.status = RfidStatus::Ok;
    result.detail = "written and read back";
    return result;
}

MifareWriteResult MifareClassicDriver::writeTrailer(uint8_t sector,
                                                   const MifareTrailer& trailer,
                                                   MifareKeySlot slot,
                                                   const std::array<uint8_t, kMifareKeySize>& key) {
    MifareWriteResult result;
    if (!active_ || !supportsSectors()) {
        result.status = RfidStatus::Unsupported;
        result.detail = "no MIFARE Classic card selected";
        return result;
    }
    MifareSectorLayout layout;
    if (!MifareClassicModel::sectorLayout(size_, sector, layout)) {
        result.status = RfidStatus::Unsupported;
        result.detail = "sector is outside this card";
        return result;
    }
    const MifareAuthResult auth = authenticate(layout.firstBlock, slot, key);
    if (!auth.authenticated) {
        result.status = auth.status;
        result.detail = "authentication failed; nothing was written";
        return result;
    }

    MifareAccessBits bits;
    MifareTrailer current;
    if (!readAccessBits(sector, bits, current)) {
        result.status = RfidStatus::IoError;
        result.detail = "could not read the access bits; refusing to write blind";
        return result;
    }
    if (bits.consistent && MifareClassicModel::locksSectorPermanently(bits)) {
        // Triplet 111 freezes Key A, Key B and the access bits for good. There
        // is no recovery path, so this is refused rather than attempted.
        result.status = RfidStatus::Unsupported;
        result.detail = "sector is permanently locked; writing it would be irreversible";
        return result;
    }

    // The bits that are about to be installed are decoded first: writing an
    // inconsistent pair leaves the sector unreadable, and that is not something
    // to discover after the fact.
    MifareAccessBits nextBits;
    if (!MifareClassicModel::decodeAccessBits(trailer.accessBytes, nextBits) ||
        !nextBits.consistent) {
        result.status = RfidStatus::Unsupported;
        result.detail = "the requested access bytes are inconsistent";
        return result;
    }
    // Replacing a trailer always rewrites the access bits, so the access-bits
    // write permission is the governing one, not the Key A or Key B write
    // permission. On every row of the verified table that permission is Key A.
    const MifareTrailerPermission permission = MifareClassicModel::trailerPermission(bits);
    if (!allowsAccess(permission.accessBitsWrite, slot)) {
        result.status = RfidStatus::Unsupported;
        result.detail = "the access bits forbid this key from rewriting the trailer";
        return result;
    }

    std::array<uint8_t, kMifareBlockSize> block{};
    MifareClassicModel::buildTrailer(trailer.keyA, trailer.keyB, trailer.accessBytes, block);
    uint8_t frame[18] = {};
    const uint16_t length =
        Iso14443aFraming::buildWriteBlock(layout.trailerBlock, block.data(), frame);
    uint8_t rx[kMifareBlockSize] = {};
    uint16_t rxLength = sizeof(rx);
    const RfidStatus status =
        transport_.transceive(frame, length, rx, rxLength, kWriteTimeoutMs);
    if (status != RfidStatus::Ok) {
        result.status = status;
        result.detail = rfidStatusLabel(status);
        return result;
    }

    // Verify what can be verified. Key A never reads back -- a trailer read
    // returns zeros there on every card -- so only the access bytes and, when
    // the new bits permit reading it, Key B are compared. The detail string
    // says which of the two happened, rather than implying a full check.
    std::array<uint8_t, kMifareBlockSize> verify{};
    if (readBlock(layout.trailerBlock, verify).status != RfidStatus::Ok) {
        result.status = RfidStatus::IoError;
        result.detail = "the trailer could not be read back";
        return result;
    }
    bool mismatch = false;
    for (uint8_t i = kMifareAccessByteOffset;
         i < kMifareAccessByteOffset + kMifareAccessByteCount; ++i) {
        if (verify[i] != block[i]) {
            mismatch = true;
        }
    }
    const bool keyBReadable =
        MifareClassicModel::keyBAccess(nextBits) == MifareTrailerKeyBAccess::Readable;
    if (keyBReadable) {
        for (uint8_t i = kMifareKeyBOffset; i < kMifareBlockSize; ++i) {
            if (verify[i] != block[i]) {
                mismatch = true;
            }
        }
    }
    if (mismatch) {
        result.status = RfidStatus::IoError;
        result.detail = "the trailer did not read back as written";
        return result;
    }
    result.written = true;
    result.status = RfidStatus::Ok;
    result.detail = keyBReadable ? "trailer written; access bytes and Key B verified"
                                 : "trailer written; access bytes verified (Key B not readable)";
    return result;
}

MifareWriteResult MifareClassicDriver::writeMagicUid(const std::array<uint8_t, 10>& uid,
                                                     uint8_t uidLength,
                                                     bool backdoorDetected,
                                                     bool userConfirmed) {
    MifareWriteResult result;
    (void)uid;
    if (uidLength == 0U || uidLength > uid.size()) {
        result.status = RfidStatus::Unsupported;
        result.detail = "invalid UID length";
        return result;
    }
    if (!MifareClassicModel::magicWriteAllowed(backdoorDetected, userConfirmed)) {
        result.status = RfidStatus::Unsupported;
        result.detail = backdoorDetected
                            ? "back door detected, but the operator has not confirmed the risk"
                            : "no gen1A back door detected on this card";
        return result;
    }
    // Both preconditions are met and execution still stops here. The command
    // sequence is an undocumented vendor extension, and
    // docs/protocol-evidence/mifare-classic.md §4 keeps it behind the evidence
    // gate. Passing the gate is not a licence to guess a frame that could brick
    // a card, so this reports Unsupported with the reason.
    result.status = RfidStatus::Unsupported;
    result.detail = "gen1A back door is not implemented (evidence gate still open)";
    return result;
}
