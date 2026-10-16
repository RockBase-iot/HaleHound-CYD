#!/usr/bin/env python3
"""Report the grant a header actually makes, not the base version.  (C7)

`rc-switch` and `TinyGPSPlus` ship no licence file, so the audit reads their
source headers.  Both headers say:

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

The audit reported "LGPL-2.1".  That identifier is deprecated, and the bare
GPL/LGPL ids in SPDX mean *-only*: so the row said the recipient may not
relicense under LGPL-3.0, when the header explicitly says they may.  The two
grants impose the same relinking obligation, so this never changed a
conclusion -- but it stated the narrower grant as if it were the wider one,
and it contradicted REMAINING_WORK/TODO, which already said LGPL-2.1-or-later.

The version tag has to be read from the wording, so `or_later()` looks for it.
It is applied to header notices ONLY.  Every GPL/LGPL licence *text* contains
"any later version" in its "How to Apply These Terms" appendix, so running
this against a full licence file would silently upgrade an -only library:
RF24 ships the GPLv2 text (which mentions later versions) and declares
"GPL-2.0-only", and the narrower declaration has to win there.

Both SPDX texts for 2.1 are byte-identical (md5 310c7c93cf5181c6b0cc8229a1f3c6f6),
so this changes identifiers and filenames, not file contents.

Usage:  python build-reports/patch_lgpl_or_later.py
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
AUDIT = "build-reports/audit_licenses.py"

EDITS: list[tuple[str, str]] = [
    # 1. The detector, placed next to the code that uses it.
    (
        r'''def scan_headers(lib: pathlib.Path, max_files: int = 60) -> tuple[str, str, str]:
    """Some libraries ship no licence file but do carry a header notice."""
''',
        r'''# The bare GPL/LGPL identifiers in SPDX mean "-only", because the version
# tags carry the whole meaning.  A notice that offers "any later version"
# grants something wider than that, so it has to be named differently -- an
# -or-later library may be relicensed under the next version, which an -only
# one may not.
OR_LATER = {
    "LGPL-2.1": "LGPL-2.1-or-later",
    "LGPL-3.0": "LGPL-3.0-or-later",
    "GPL-2.0": "GPL-2.0-or-later",
    "GPL-3.0": "GPL-3.0-or-later",
    "AGPL-3.0": "AGPL-3.0-or-later",
}


def or_later(text: str) -> bool:
    """True when the notice offers the recipient the next version too."""
    return re.search(
        r"either\s+version\s+[0-9.]+\s+of\s+the\s+License,\s*"
        r"or\s*\(?at\s+your\s+option\)?\s*any\s+later\s+version",
        text, re.IGNORECASE | re.DOTALL) is not None


def scan_headers(lib: pathlib.Path, max_files: int = 60) -> tuple[str, str, str]:
    """Some libraries ship no licence file but do carry a header notice."""
''',
    ),
    # 2. Apply it.  Header notices only -- see the module docstring for why a
    #    licence file must not be run through this.
    (
        r'''        spdx, kind = classify(read_text(path, limit=4096))
        if spdx != "UNKNOWN":
            return spdx, kind, path.relative_to(lib).as_posix()
''',
        r'''        body = read_text(path, limit=4096)
        spdx, kind = classify(body)
        if spdx != "UNKNOWN":
            if or_later(body):
                spdx = OR_LATER.get(spdx, spdx)
            return spdx, kind, path.relative_to(lib).as_posix()
''',
    ),
]


def main() -> int:
    path = ROOT / AUDIT
    if not path.is_file():
        raise SystemExit(f"FAIL {AUDIT}: not found")
    text = path.read_text(encoding="utf-8", newline="")
    original = text

    for index, (old, new) in enumerate(EDITS, start=1):
        label = f"{AUDIT} edit {index}"
        if new in text:
            print(f"  = {label}: already applied")
            continue
        found = text.count(old)
        if found != 1:
            raise SystemExit(
                f"FAIL {label}: expected exactly 1 occurrence, found {found}.  "
                f"Anchor was:\n{old[:200]}")
        text = text.replace(old, new, 1)
        print(f"  + {label}: applied")

    if text != original:
        path.write_text(text, encoding="utf-8", newline="")
    print("\n1 file(s) processed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
