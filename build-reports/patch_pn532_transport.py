#!/usr/bin/env python3
"""Patch Pn532Transport.cpp: I2C timing after a write, and UID bookkeeping.

Three edits, applied atomically with a match-count assertion each, because
editing the same file several times in one batch silently drops all but the
last write in this environment.

Run from the repository root:
    build-reports/hostenv/Scripts/python.exe build-reports/patch_pn532_transport.py
"""

from __future__ import annotations

import pathlib
import sys

TARGET = pathlib.Path("src/drivers/Pn532Transport.cpp")

EDITS: list[tuple[str, str]] = [
    # 1. The part needs a moment to digest a command before its ready pin/byte
    #    goes high. The reference driver waits 2 ms here for I2C.
    (
        """    if (!link_.write(encoded, encodedLength)) {
        return false;
    }
    // A command the part accepts is answered with the 6-byte ACK. A malformed
    // one gets no ACK at all, so this timeout is the only way to learn that the
    // host built a bad frame.
    if (!waitReady(timeoutMs)) {""",
        """    if (!link_.write(encoded, encodedLength)) {
        return false;
    }
    // The part needs a moment to digest the command before it raises ready.
    // Two milliseconds is what the reference driver waits on I2C.
    link_.delayMs(2);
    // A command the part accepts is answered with the 6-byte ACK. A malformed
    // one gets no ACK at all, so this timeout is the only way to learn that the
    // host built a bad frame.
    if (!waitReady(timeoutMs)) {""",
    ),
    # 2. Poll with a one-millisecond gap instead of spinning on the bus, and
    #    take one final look at ready before giving up.
    (
        """bool Pn532Transport::waitReady(uint32_t timeoutMs) {
    const uint32_t start = link_.millis();
    for (;;) {
        if (link_.ready()) {
            return true;
        }
        if (link_.millis() - start >= timeoutMs) {
            return false;
        }
    }
}""",
        """bool Pn532Transport::waitReady(uint32_t timeoutMs) {
    const uint32_t start = link_.millis();
    for (;;) {
        if (link_.ready()) {
            return true;
        }
        if (link_.millis() - start >= timeoutMs) {
            return false;
        }
        // Poll with a gap rather than hammering the bus; one requestFrom per
        // millisecond is already far more often than the part answers.
        link_.delayMs(1);
    }
}""",
    ),
    # 3. The UID belongs to the target, so releasing the target has to drop it:
    #    a stale UID would be spliced into the next card's authentication frame.
    (
        """void Pn532Transport::end() {
    initialized_ = false;
    targetNumber_ = 0;
    targetUidLength_ = 0;
    link_.end();
}""",
        """void Pn532Transport::end() {
    initialized_ = false;
    targetNumber_ = 0;
    targetUidLength_ = 0;
    link_.end();
    targetUid_[0] = 0x00;
}""",
    ),
    (
        """    const RfidStatus status = exchangeDiscard(command, sizeof(command), kReplyTimeoutMs);
    targetNumber_ = 0;
    targetUidLength_ = 0;
    return status;""",
        """    const RfidStatus status = exchangeDiscard(command, sizeof(command), kReplyTimeoutMs);
    targetNumber_ = 0;
    // Cleared with the target number, for the same reason: the UID is spliced
    // into the MIFARE authentication frame, and a stale one would be sent to
    // whichever card is selected next.
    targetUidLength_ = 0;
    return status;""",
    ),
]


def main() -> int:
    if not TARGET.exists():
        sys.exit(f"patch: {TARGET} not found; run from the repository root")
    text = TARGET.read_text(encoding="utf-8")
    for index, (old, new) in enumerate(EDITS, start=1):
        count = text.count(old)
        if count != 1:
            sys.exit(f"patch: edit {index} matched {count} times, expected exactly 1")
        text = text.replace(old, new)
    TARGET.write_text(text, encoding="utf-8")
    print(f"patch: applied {len(EDITS)} edits to {TARGET}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
