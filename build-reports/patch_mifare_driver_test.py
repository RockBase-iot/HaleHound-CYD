#!/usr/bin/env python3
"""Fix the three wrong expectations in test_mifare_classic_driver.

Both mismatches were the test's fault, not the driver's:

  * classifyCard() requires ATQA 0044 for the Ultralight/NTAG row; the suite
    was feeding a Classic ATQA 0004 and then expecting the NTAG name.
  * unknownBlockCount() counts every block the dump did not recover, not only
    the ones a read attempt failed on -- so a one-sector dump reports the whole
    card as unknown.

Each replacement asserts its match count so a silent no-op cannot pass.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "test" / "native" / "test_mifare_classic_driver" / "test_main.cpp"

EDITS: list[tuple[str, str, int]] = [
    # 1. A dedicated helper for the tag's ATQA, next to the Classic one.
    (
        """void activateCard(FakeRfidTransport& transport, uint8_t sak) {
    transport.setTarget(0x0004, sak, {0x04, 0x12, 0x34, 0x56});
    transport.activateResult = RfidStatus::Ok;
}
""",
        """void activateCard(FakeRfidTransport& transport, uint8_t sak) {
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
""",
        1,
    ),
    # 2. Use it, and pin the coarseness decision from the other side too.
    (
        """    FakeRfidTransport transportTag;
    MifareClassicDriver driverTag(transportTag);
    activateCard(transportTag, kSakUltralight);
    TEST_ASSERT_EQUAL(RfidStatus::Ok, driverTag.activate(200));
    TEST_ASSERT_EQUAL(Iso14443aCardType::UltralightOrNtag, driverTag.cardType());
    TEST_ASSERT_EQUAL(MifareSize::Unknown, driverTag.size());
    TEST_ASSERT_FALSE(driverTag.supportsSectors());
""",
        """    FakeRfidTransport transportTag;
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
""",
        1,
    ),
    # 3. unknownBlockCount() over a one-sector dump is the whole card.
    (
        """    TEST_ASSERT_EQUAL_UINT16(4, MifareClassicModel::unknownBlockCount(dump));
    TEST_ASSERT_EQUAL_UINT8(0, dump.recoveredSectors);
""",
        """    // unknownBlockCount() counts every block the dump did not recover, so
    // with only one sector touched and nothing read it is the whole card.
    TEST_ASSERT_EQUAL_UINT16(64, MifareClassicModel::unknownBlockCount(dump));
    TEST_ASSERT_EQUAL_UINT8(0, dump.recoveredSectors);
""",
        1,
    ),
    # 4. Sector 0 recovered leaves the other sixty blocks unattempted.
    (
        """    TEST_ASSERT_EQUAL_UINT16(0, MifareClassicModel::unknownBlockCount(dump));
}
""",
        """    // The other sixty blocks were never attempted, so they are unknown; only
    // sector 0 was recovered. The count describes what the dump holds, not
    // which reads failed.
    TEST_ASSERT_EQUAL_UINT16(60, MifareClassicModel::unknownBlockCount(dump));
}
""",
        1,
    ),
]


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    for index, (old, new, expected) in enumerate(EDITS, start=1):
        found = text.count(old)
        if found != expected:
            print(f"edit {index}: expected {expected} match(es), found {found}", file=sys.stderr)
            return 1
        text = text.replace(old, new, expected)
    TARGET.write_text(text, encoding="utf-8")
    print(f"patched {TARGET.relative_to(ROOT)}: {len(EDITS)} edit(s) applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
