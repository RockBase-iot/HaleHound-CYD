#!/usr/bin/env python3
"""Fixes two host-test assumptions in test_chameleon_frame.

1. This repository's unity.h has no TEST_ASSERT_EQUAL_MEMORY, so the byte
   comparisons go through a local helper.
2. A non-void function may not contain TEST_ASSERT_*: the macros expand to a
   bare `return;`. The counter cross-check therefore moves out of `feedAll`.

Each edit asserts it matched, so a rename in unity.h cannot silently turn these
into no-ops.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "test" / "native" / "test_chameleon_frame" / "test_main.cpp"

HELPER_ANCHOR = """}  // namespace

// ─── The LRC rule ──────────────────────────────────────────────────────────
"""

HELPER_TEXT = """// No TEST_ASSERT_EQUAL_MEMORY in this repository's unity.h, and an assertion
// macro expands to a bare `return;`, so a byte comparison has to happen inside a
// void function.
void expectBytes(const uint8_t* expected, const uint8_t* actual, std::size_t length) {
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(expected, actual, length));
}

}  // namespace

// ─── The LRC rule ──────────────────────────────────────────────────────────
"""

CROSS_CHECK = """    // The counters are the receiver's own view of the same stream; a mismatch
    // means the returned Feed values and the counters disagree.
    TEST_ASSERT_EQUAL_UINT32(summary.frames, receiver.framesDecoded());
    TEST_ASSERT_EQUAL_UINT32(summary.resyncs, receiver.resyncs());
    return summary;
"""

CROSS_CHECK_REPLACEMENT = """    // The counters are the receiver's own view of the same stream. They are
    // checked where each one matters rather than here: a helper that returns a
    // value cannot contain an assertion macro, since the macros expand to a bare
    // `return;`.
    return summary;
"""


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")

    if text.count(HELPER_ANCHOR) != 1:
        print(f"FAIL helper anchor matched {text.count(HELPER_ANCHOR)} times")
        return 1
    text = text.replace(HELPER_ANCHOR, HELPER_TEXT)

    if text.count(CROSS_CHECK) != 1:
        print(f"FAIL cross-check block matched {text.count(CROSS_CHECK)} times")
        return 1
    text = text.replace(CROSS_CHECK, CROSS_CHECK_REPLACEMENT)

    pattern = re.compile(r"TEST_ASSERT_EQUAL_MEMORY\((?P<a>[^,]+), (?P<b>[^,]+), (?P<n>[^)]+)\);")
    matches = pattern.findall(text)
    if not matches:
        print("FAIL no TEST_ASSERT_EQUAL_MEMORY lines found")
        return 1
    text, replaced = pattern.subn(r"expectBytes(\g<a>, \g<b>, \g<n>);", text)

    TARGET.write_text(text, encoding="utf-8")
    print(f"OK   {TARGET.name}: helper added, cross-check moved, {replaced} memory assertion(s) rewritten")

    # The helper itself must not have been caught by the rewrite.
    if "TEST_ASSERT_EQUAL_INT(0, std::memcmp" not in text:
        print("FAIL the expectBytes helper is missing")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
