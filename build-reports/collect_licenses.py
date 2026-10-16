#!/usr/bin/env python3
"""Collect the exact licence texts this firmware has to ship with.

A binary that links third-party code has to carry those licences' notices and
texts: MIT and BSD say so in their own terms, and the LGPL says so twice over
(once for the notice, once for the relinking obligation).  Keeping the texts in
`LICENSES/` at the repository root is also what makes `REUSE`-style tooling and
manual review possible without unzipping a build.

Sources, in priority order -- never a transcription:

  1. the licence file inside the installed library, copied byte for byte;
  2. for a library that names a licence but ships no copy (rc-switch and
     TinyGPSPlus both reference the LGPL from their headers only), the canonical
     text, fetched from the URL recorded in audit_licenses.SPDX_TEXT_URLS.

A library that grants nothing, or ships a licence file naming no licence, gets a
marker file instead, so that the gap is visible in the tree rather than silently
absent.

`LICENSES/` is rebuilt from scratch on every run, EXCEPT for the files named in
PRESERVED -- see the warning there.  Anything else in that directory that this
script does not write is deleted, so it stays a generated directory.

Usage:  python build-reports/collect_licenses.py [--offline]
"""

from __future__ import annotations

import argparse
import pathlib
import shutil
import sys
import urllib.error
import urllib.request

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from audit_licenses import (  # noqa: E402
    LIBDEPS, ROOT, SPDX_TEXT_URLS, audit_frameworks, audit_libraries,
    licence_file,
)

OUT = ROOT / "LICENSES"

# A LicenseRef is an SPDX expression, not a filename, and slugging one produces
# something no reader can parse.  A library whose licence is a LicenseRef gets
# a name here that says what the file actually is.
BESPOKE = {
    "SmartRC-CC1101-Driver-Lib": "SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt",
}

# Files this script does NOT own.  They were captured once, are checked in, and
# must survive every rebuild.
#
# HaleHound-CYD-MIT.txt is the upstream's MIT grant exactly as this fork
# received it (upstream commit 56cf737, 2026-02-17).  It cannot be regenerated
# from the working tree, because the top-level LICENSE is now this project's own
# licence -- so a naive rebuild copies the wrong file over the evidence.  That
# is not hypothetical: an earlier revision of this script did exactly that, and
# the only reason it was recoverable is that the original also exists in git
# history (`git show 56cf737:LICENSE`).  Capture it that way, by hand, if this
# file is ever lost:
#
#     git show 56cf737:LICENSE > LICENSES/HaleHound-CYD-MIT.txt
PRESERVED = (
    "HaleHound-CYD-MIT.txt",
)


def fetch(url: str) -> bytes | None:
    try:
        with urllib.request.urlopen(url, timeout=45) as response:
            return response.read()
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        print(f"  ! could not fetch {url}: {exc}")
        return None


def slug(name: str) -> str:
    """Filename-safe, but conservative: only characters a filesystem rejects,
    plus whitespace.  Underscores and dots are left alone, so a reader sees
    "TFT_eSPI" and "GPL-2.0" rather than "TFT-eSPI" and "GPL-2-0"."""
    illegal = '<>:"/\\|?*'
    out = "".join("-" if (c in illegal or c.isspace()) else c for c in name)
    while "--" in out:
        out = out.replace("--", "-")
    return out.strip("-")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--offline", action="store_true",
                        help="do not fetch canonical texts for libraries that ship none")
    args = parser.parse_args()

    kept: dict[str, bytes] = {}
    if OUT.exists():
        for name in PRESERVED:
            path = OUT / name
            if path.is_file():
                kept[name] = path.read_bytes()
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True)

    written: list[tuple[str, str]] = []
    for name, body in kept.items():
        (OUT / name).write_bytes(body)
        written.append((name, "preserved: captured once, not regenerated"))

    # 1. One file per dependency, deduplicated across environments.
    # Bodies are cached by URL, not by library name: rc-switch and TinyGPSPlus
    # both name the LGPL and neither ships it, so a name-keyed cache writes the
    # first one and then fails looking for the second one's copy.
    fetched: dict[str, bytes] = {}
    for entry in audit_libraries():
        if entry["kind"] in ("no-grant", "unidentified"):
            continue
        target = (BESPOKE.get(entry["name"])
                  or f"{slug(entry['name'])}-{slug(entry['spdx'])}.txt")
        destination = OUT / target
        if destination.exists():
            continue

        source = None
        for env in ("esp32-cyd", "nm-cyd-c5", "native"):
            candidate = LIBDEPS / env / entry["name"]
            if candidate.is_dir():
                source = licence_file(candidate)
                if source:
                    break
        if source:
            shutil.copyfile(source, destination)
            written.append((target, source.relative_to(ROOT).as_posix()))
            continue

        # Names a licence but ships no text: take the canonical one.
        url = SPDX_TEXT_URLS.get(entry["spdx"])
        if url is None:
            print(f"  ? {entry['name']}: {entry['spdx']} but no local text and no canonical URL")
            continue
        if url not in fetched:
            if args.offline:
                print(f"  - skipping fetch of {url} (--offline)")
                continue
            body = fetch(url)
            if body is None:
                continue
            fetched[url] = body if body.endswith(b"\n") else body + b"\n"
        destination.write_bytes(fetched[url])
        written.append((target, url))

    # 2. The Arduino core declares LGPL-2.1-or-later in package.json and ships
    #    no text at all, so it needs the canonical copy too.  Named after the
    #    package rather than the human label, so the name is stable; the two
    #    core versions declare the same licence and share one file.
    for framework in audit_frameworks():
        url = SPDX_TEXT_URLS.get(framework["spdx"])
        if url is None:
            continue
        # PlatformIO keeps an @src- mirror of each package it built from
        # source; same licence, and one copy of the text is enough.
        if "@src-" in framework["path"]:
            continue
        target = f"{slug(pathlib.Path(framework['path']).name)}-{slug(framework['spdx'])}.txt"
        destination = OUT / target
        if destination.exists():
            continue
        if url not in fetched:
            if args.offline:
                print(f"  - skipping fetch of {url} (--offline)")
                continue
            body = fetch(url)
            if body is None:
                continue
            fetched[url] = body if body.endswith(b"\n") else body + b"\n"
        destination.write_bytes(fetched[url])
        written.append((target, url))

    # 3. A marker for every library whose grant could not be established, so the
    #    gap is visible in the tree rather than showing up as an absent file.
    #    The two cases are not the same finding and do not get the same text.
    GAP_TEXT = {
        "no-grant": (
            "No licence was found for this library: it declares none in\n"
            "library.properties or library.json, ships no LICENSE/COPYING file,\n"
            "and carries no licence notice in its source headers.  GitHub reports\n"
            "no licence for the upstream repository either.\n\n"
            "Absent a grant, the default is all rights reserved, so this library\n"
            "may not be redistributed -- and it is compiled into the firmware,\n"
            "which makes the firmware a redistribution of it.\n"
        ),
        "unidentified": (
            "This library does ship a licence file, but the text names no licence\n"
            "any tool can identify and states no permission this project can rely\n"
            "on.  Nothing in it reads as a grant of rights, so the default of all\n"
            "rights reserved stands until a person reads the text and records its\n"
            "scope -- which is why the file is called out here rather than being\n"
            "treated as absent.\n"
        ),
    }
    for entry in audit_libraries():
        text = GAP_TEXT.get(entry["kind"])
        if text is None:
            continue
        name = f"{slug(entry['name'])}.LICENCE-UNRESOLVED.txt"
        (OUT / name).write_text(
            f"{entry['name']} {entry['version']}\n"
            f"{'=' * (len(entry['name']) + len(entry['version']) + 1)}\n\n"
            + text +
            "\nThis file exists so the gap is visible in the tree.  It is not a\n"
            "licence, and it is not a substitute for one: see\n"
            "docs/legal/COMPLIANCE_PLAN_zh.md, track A3.\n",
            encoding="utf-8", newline="")
        written.append((name, "generated marker"))

    print(f"\nwrote {len(written)} file(s) into {OUT.relative_to(ROOT)}/")
    for target, origin in written:
        print(f"  {target:52s} <- {origin}")
    if not kept:
        print("\n! none of the preserved files were present; they are evidence and "
              "have to be restored by hand (see PRESERVED in this script).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
