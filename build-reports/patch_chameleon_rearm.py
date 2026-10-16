#!/usr/bin/env python3
"""Makes the Chameleon receiver look for a frame start inside a rejected window.

Symptom this fixes: on a link that comes up mid-stream, a false SOF+LRC1 pair is
followed by a real frame -- and the real frame's SOF lands inside the doomed
header window, so it is consumed and the frame is lost. The test
`test_a_frame_starting_mid_stream_is_found` caught exactly that.

Each replacement asserts it matched, so a drift in the code cannot turn this into
a silent no-op.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "features" / "ble" / "ChameleonFrame.cpp"

HEADER_REJECT_OLD = """            if (rejected) {
                ++resyncs_;
                state_ = State::Idle;
                // The byte that failed might start the next frame.
                if (byte == kChameleonSof) {
                    state_ = State::Lrc1;
                }
                return Feed::Resync;
            }
"""

HEADER_REJECT_NEW = """            if (rejected) {
                ++resyncs_;
                if (!rearmFromPair(header_, kHeaderBytes)) {
                    state_ = State::Idle;
                }
                return Feed::Resync;
            }
"""

LRC3_REJECT_OLD = """        case State::Lrc3: {
            const bool ok = chameleonLrc(frame_.data, frame_.length) == byte;
            state_ = State::Idle;
            if (!ok) {
                ++lrcFailures_;
                if (byte == kChameleonSof) {
                    state_ = State::Lrc1;
                }
                return Feed::Resync;
            }
            ++framesDecoded_;
            return Feed::FrameReady;
        }
"""

LRC3_REJECT_NEW = """        case State::Lrc3: {
            const bool ok = chameleonLrc(frame_.data, frame_.length) == byte;
            state_ = State::Idle;
            if (!ok) {
                ++lrcFailures_;
                // The corrupt frame's payload may itself contain the start of
                // the next one.
                if (!rearmFromPair(frame_.data, frame_.length)) {
                    state_ = byte == kChameleonSof ? State::Lrc1 : State::Idle;
                }
                return Feed::Resync;
            }
            ++framesDecoded_;
            return Feed::FrameReady;
        }
"""

REARM_ANCHOR = """void ChameleonFrameReceiver::reset() {"""

REARM_TEXT = """bool ChameleonFrameReceiver::rearmFromPair(const uint8_t* buffer, std::size_t length) {
    if (buffer == nullptr) {
        return false;
    }
    // Backwards, because the last start in a rejected window is the one most
    // likely to be the real frame.
    for (std::size_t end = length; end >= 2U; --end) {
        const std::size_t start = end - 2U;
        if (buffer[start] != kChameleonSof || buffer[start + 1] != kChameleonLrc1) {
            continue;
        }
        // SOF and LRC1 are behind us; whatever follows is already header bytes.
        // memmove rather than memcpy because `buffer` may be `header_` itself.
        const std::size_t tail = length - (start + 2U);
        const std::size_t carried = tail < kHeaderBytes ? tail : kHeaderBytes;
        std::memmove(header_, buffer + start + 2U, carried);
        headerLength_ = carried;
        state_ = State::Header;
        return true;
    }
    // A trailing SOF with its LRC1 still to come is a frame start too.
    if (length > 0U && buffer[length - 1U] == kChameleonSof) {
        state_ = State::Lrc1;
        headerLength_ = 0;
        return true;
    }
    return false;
}

void ChameleonFrameReceiver::reset() {"""


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        print(f"FAIL {label}: matched {count} times, expected 1")
        raise SystemExit(1)
    return text.replace(old, new)


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    text = replace_once(text, HEADER_REJECT_OLD, HEADER_REJECT_NEW, "header rejection")
    text = replace_once(text, LRC3_REJECT_OLD, LRC3_REJECT_NEW, "lrc3 rejection")
    text = replace_once(text, REARM_ANCHOR, REARM_TEXT, "rearm insertion")
    TARGET.write_text(text, encoding="utf-8")
    print("OK   ChameleonFrame.cpp: rearm on rejected header and rejected payload")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
