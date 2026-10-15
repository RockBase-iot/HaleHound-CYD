#!/usr/bin/env python3
"""Fix three defects in test_pn532_transport before its first run.

  * expectReply() takes an initializer_list, so a byte array cannot be passed
    to it -- the corrupted-frame test has to spell the bytes out.
  * the "not a bus fault" test re-called transceive to compare against an
    unscripted (and therefore Timeout) reply, which tested the fake rather than
    the mapping. Capture the fault status once and compare that.
  * a comment claimed the reader status byte is handed back inside the payload
    when dataExchange() in fact strips it.

Every replacement asserts its match count, so a silent no-op fails loudly.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "test" / "native" / "test_pn532_transport" / "test_main.cpp"

EDITS: list[tuple[str, str, int]] = [
    (
        """    const std::vector<uint8_t> commanded = lastCommand(link);
    TEST_ASSERT_EQUAL(4, commanded.size());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdInDataExchange, commanded[0]);
    TEST_ASSERT_EQUAL_UINT8(0x30U, commanded[2]);
    TEST_ASSERT_EQUAL_UINT8(0x04U, commanded[3]);
""",
        """    // Two bytes of InDataExchange header plus the two card bytes, and nothing
    // else: with the UID appended this would be eight, so the length is what
    // proves the auth quirk stays confined to 0x60/0x61.
    const std::vector<uint8_t> commanded = lastCommand(link);
    TEST_ASSERT_EQUAL(4, commanded.size());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdInDataExchange, commanded[0]);
    TEST_ASSERT_EQUAL_UINT8(0x30U, commanded[2]);
    TEST_ASSERT_EQUAL_UINT8(0x04U, commanded[3]);
""",
        1,
    ),
    (
        """    // The reader status byte is the first payload byte, so the sixteen block
    // bytes start one further in.
    TEST_ASSERT_EQUAL_UINT16(kMifareBlockSize, rxLength);""",
        """    // dataExchange() consumes the reader status byte, so what comes back is
    // the sixteen block bytes and nothing else.
    TEST_ASSERT_EQUAL_UINT16(kMifareBlockSize, rxLength);""",
        1,
    ),
    (
        """    // An unrecognised reader status is a fault, and must give a different
    // answer. If these two ever collapse into one status, Key Check starts
    // reporting a dead bus as a card that protects its keys.
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, {0x08});
    rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::IoError,
                      transport.transceive(auth.data(), auth.size(), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
    TEST_ASSERT_NOT_EQUAL(RfidStatus::AuthFailed,
                          transport.transceive(auth.data(), auth.size(), rx, rxLength, 200));
""",
        """    // An unrecognised reader status is a fault, and must give a different
    // answer. If these two ever collapse into one status, Key Check starts
    // reporting a dead bus as a card that protects its keys.
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, {0x08});
    rxLength = sizeof(rx);
    const RfidStatus fault =
        transport.transceive(auth.data(), auth.size(), rx, rxLength, 200);
    TEST_ASSERT_EQUAL(RfidStatus::IoError, fault);
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
    TEST_ASSERT_NOT_EQUAL(RfidStatus::AuthFailed, fault);
""",
        1,
    ),
    (
        """    link.expectAck();
    link.expectReply(corrupted, sizeof(corrupted));
""",
        """    link.expectAck();
    link.expectReply({0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD5, 0x41, 0x00, 0xEB, 0x00});
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
