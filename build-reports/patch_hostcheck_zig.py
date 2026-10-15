#!/usr/bin/env python3
"""Wire a local ``ziglang`` install into scripts/hostcheck/run_host_tests.py.

Background: the host test runner resolves a single compiler *path*, but
``ziglang`` (pip wheel bundling clang 21 + libc++ sysroot) is driven as
``python -m ziglang c++`` -- a command *prefix*, not a path. This patch
generalises the runner to a command prefix and adds zig auto-detection as a
last-resort fallback when no MSVC / mingw-w64 toolchain is present.

Every replacement asserts its match count so a silent no-op cannot happen.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "scripts" / "hostcheck" / "run_host_tests.py"

EDITS: list[tuple[str, str, str]] = [
    # ---- 1. import shlex -------------------------------------------------
    (
        "import os\nimport shutil\n",
        "import os\nimport shlex\nimport shutil\n",
        "add shlex import",
    ),
    # ---- 2. find_compiler -> zig-aware command prefix ---------------------
    (
        '''def find_compiler(explicit: str | None) -> str:
    if explicit:
        return explicit
    env = os.environ.get("HOSTCXX")
    if env:
        return env
    for pattern in (
        str(Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe"),
        str(Path.home() / ".platformio/packages/toolchain-gccmingw64/bin/g++.exe"),
    ):
        if Path(pattern).exists():
            return pattern
    found = shutil.which("g++") or shutil.which("clang++")
    if found:
        return found
    sys.exit("hostcheck: no host C++ compiler found; pass --cxx <path>")
''',
        '''def _zig_prefix() -> list[str] | None:
    """Return the ``python -m ziglang c++`` prefix if a repo-local zig exists.

    ``ziglang`` is a pip wheel that bundles clang 21 plus a full libc++/libc
    sysroot, giving a working host C++ compiler on machines that have neither
    MSVC nor mingw-w64. It is installed into a repo-local venv under
    ``build-reports/hostenv``; see build-reports/README for the install line.
    """
    candidates = (
        ROOT / "build-reports" / "hostenv" / "Scripts" / "python.exe",
        ROOT / "build-reports" / "hostenv" / "bin" / "python",
    )
    for py in candidates:
        if not py.exists():
            continue
        try:
            probe = subprocess.run(
                [str(py), "-m", "ziglang", "version"],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=120,
            )
        except (OSError, subprocess.SubprocessError):
            continue
        if probe.returncode == 0:
            return [str(py), "-m", "ziglang", "c++"]
    return None


def find_compiler(explicit: str | None) -> list[str]:
    """Resolve the host C++ compiler to a *command prefix* (argv list).

    The result is either a bare compiler path (``["g++"]``) or a driver
    invocation (``["python", "-m", "ziglang", "c++"]``). Callers must splice
    it with ``*cxx`` instead of passing it as a single argv element.

    Resolution order: --cxx > $HOSTCXX > PlatformIO mingw32 > PlatformIO
    mingw64 > PATH g++/clang++ > repo-local ziglang venv.
    """
    if explicit:
        return shlex.split(explicit)
    env = os.environ.get("HOSTCXX")
    if env:
        return shlex.split(env)
    for pattern in (
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
''',
        "find_compiler -> command prefix + zig fallback",
    ),
    # ---- 3. compile_command splice --------------------------------------
    (
        '''def compile_command(cxx: str, src: Path, out: Path, extra: list[str]) -> list[str]:
    return [
        cxx,
        *CXX_FLAGS,''',
        '''def compile_command(cxx: list[str], src: Path, out: Path, extra: list[str]) -> list[str]:
    return [
        *cxx,
        *CXX_FLAGS,''',
        "compile_command accepts a command prefix",
    ),
    # ---- 4. build_object signature --------------------------------------
    (
        "def build_object(cxx: str, src: Path, extra: list[str]) -> tuple[Path | None, str]:",
        "def build_object(cxx: list[str], src: Path, extra: list[str]) -> tuple[Path | None, str]:",
        "build_object accepts a command prefix",
    ),
    # ---- 5. compiler banner ---------------------------------------------
    (
        '    print(f"hostcheck: compiler = {cxx}")',
        '    print(f"hostcheck: compiler = {\' \'.join(cxx)}")',
        "compiler banner joins the prefix",
    ),
    # ---- 6. link step splice --------------------------------------------
    (
        '''            code, log = run(
                [
                    cxx,
                    *CXX_FLAGS,''',
        '''            code, log = run(
                [
                    *cxx,
                    *CXX_FLAGS,''',
        "link step splices the command prefix",
    ),
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
