#!/usr/bin/env python3
"""Patch run_host_tests.py so suites can share scripted test doubles.

The RFID suites need a scripted transport and a scripted PN532 link, and both
belong in one place rather than copy-pasted per suite. That needs an include
path, so `test/native/support` is added next to the existing unity shim.

Two edits, applied atomically with a match-count assertion each.

Run from the repository root:
    build-reports/hostenv/Scripts/python.exe build-reports/patch_hostcheck_support.py
"""

from __future__ import annotations

import pathlib
import sys

TARGET = pathlib.Path("scripts/hostcheck/run_host_tests.py")

EDITS: list[tuple[str, str]] = [
    (
        """ROOT = Path(__file__).resolve().parents[2]
UNITY_DIR = Path(__file__).resolve().parent / "unity"
""",
        """ROOT = Path(__file__).resolve().parents[2]
UNITY_DIR = Path(__file__).resolve().parent / "unity"
# Shared test doubles (scripted transports and links). A header rather than a
# compiled source, so a suite includes what it needs and nothing is linked that
# it does not.
SUPPORT_DIR = ROOT / "test" / "native" / "support"
""",
    ),
    (
        """        "-I",
        str(ROOT / "include"),
        "-I",
        str(UNITY_DIR),
""",
        """        "-I",
        str(ROOT / "include"),
        "-I",
        str(UNITY_DIR),
        "-I",
        str(SUPPORT_DIR),
""",
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
