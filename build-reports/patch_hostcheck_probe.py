#!/usr/bin/env python3
"""Second-stage patch: make run_host_tests.py reject *broken* toolchains.

Why: ``~/.platformio/packages/toolchain-gccmingw32/bin/g++.exe`` exists on this
machine but the package is missing its 32-bit ``libgmp-10.dll`` / ``libmpfr-4.dll``
etc. ``g++ --version`` prints fine, but every real compile dies with
``error while loading shared libraries`` and a *completely empty* compiler log --
so ``find_compiler()`` happily returns a toolchain that silently fails.

Fix: probe each candidate with an actual compile+link+run of a trivial TU before
accepting it, and fall through to the next candidate (ultimately repo-local
ziglang) when the probe fails.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "scripts" / "hostcheck" / "run_host_tests.py"

OLD = '''def find_compiler(explicit: str | None) -> list[str]:'''

NEW = '''_PROBE_SRC = "int main() { return 0; }\\n"
_skipped: list[str] = []


def _probe(cxx: list[str]) -> bool:
    """Compile+link+run a trivial TU; True only if the toolchain really works.

    A bare ``--version`` check is not enough: a mingw package can be present but
    unloadable (missing libgmp/libmpfr DLLs), in which case it prints a version
    and then fails every compile with an *empty* log.
    """
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "probe.cpp"
        exe = Path(tmp) / "probe.exe"
        src.write_text(_PROBE_SRC, encoding="utf-8")
        try:
            built = subprocess.run(
                [*cxx, "-std=gnu++1z", "-w", str(src), "-o", str(exe)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=300,
            )
        except (OSError, subprocess.SubprocessError):
            return False
        if built.returncode != 0:
            return False
        try:
            ran = subprocess.run(
                [str(exe)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=120,
            )
        except (OSError, subprocess.SubprocessError):
            return False
        return ran.returncode == 0


def _try(candidate: list[str], label: str) -> list[str] | None:
    if _probe(candidate):
        return candidate
    _skipped.append(label)
    return None


def find_compiler(explicit: str | None) -> list[str]:'''

OLD2 = '''    for pattern in (
        str(Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe"),
        str(Path.home() / ".platformio/packages/toolchain-gccmingw64/bin/g++.exe"),
    ):
        if Path(pattern).exists():
            return [pattern]
    found = shutil.which("g++") or shutil.which("clang++")
    if found:
        return [found]
    zig = _zig_prefix()
    if zig:
        return zig
    sys.exit("hostcheck: no host C++ compiler found; pass --cxx <path>")
'''

NEW2 = '''    for pattern in (
        str(Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe"),
        str(Path.home() / ".platformio/packages/toolchain-gccmingw64/bin/g++.exe"),
    ):
        if Path(pattern).exists():
            picked = _try([pattern], pattern)
            if picked:
                return picked
    found = shutil.which("g++") or shutil.which("clang++")
    if found:
        picked = _try([found], found)
        if picked:
            return picked
    zig = _zig_prefix()
    if zig:
        picked = _try(zig, "ziglang venv")
        if picked:
            return picked
    detail = ("; skipped (probe failed): " + ", ".join(_skipped)) if _skipped else ""
    sys.exit(f"hostcheck: no working host C++ compiler found{detail}; pass --cxx <path>")
'''

EDITS = [
    (OLD, NEW, "insert _probe/_try helpers"),
    (OLD2, NEW2, "probe-gate every compiler candidate"),
]


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    for old, new, label in EDITS:
        count = text.count(old)
        if count != 1:
            print(f"ABORT: {label}: expected 1 match, found {count}")
            return 1
        text = text.replace(old, new)
        print(f"  ok  {label}")
    TARGET.write_text(text, encoding="utf-8")
    print(f"patched {TARGET}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
