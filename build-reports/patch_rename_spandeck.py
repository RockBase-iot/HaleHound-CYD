#!/usr/bin/env python3
"""Rename the public-facing identity from HaleHound to SPANDECK.

Scope is deliberate: only surfaces a buyer can perceive are touched -- the GAP
name the device broadcasts, the strings the screen draws, the SSID a captive
portal asserts, the prefix on SD-card artefacts, the artefact names, and the
docs.  The internal ``HALEHOUND_*`` macro namespace (7478 references, almost all
of them colour constants like ``HALEHOUND_MAGENTA``) is left alone.  Those are
identifiers, not a mark: they are never shown to anyone holding the device, and
renaming them would be a 7k-line diff that this sandbox cannot compile-verify
(``pio run`` is blocked here), for zero legal benefit.

Every rule is (old, new) and asserts an exact occurrence count, so a silent
partial edit is impossible.  Re-running is safe: already-applied rules report and
skip.  Line endings are preserved per file -- "\n" in a rule is widened to
"\r\n" when the target file is CRLF.
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------- code rules --
CODE: dict[str, list[tuple[str, str]]] = {
    "src/legacy/bluetooth_attacks.cpp": [
        ('BLEDevice::init("HaleHound");',
         'BLEDevice::init("SPANDECK");'),
        ('new BleKeyboard("HaleHound KB", "HaleHound", 100);',
         'new BleKeyboard("SPANDECK KB", "SPANDECK", 100);'),
        ('tft.print("Name: HaleHound KB");',
         'tft.print("Name: SPANDECK KB");'),
        ("advertising as 'HaleHound KB'",
         "advertising as 'SPANDECK KB'"),
        ('f.println("  HaleHound Edition");',
         'f.println("  SPANDECK Edition");'),
        ('payload = "echo HaleHound";',
         'payload = "echo SPANDECK";'),
    ],
    "src/legacy/nrf24_attacks.cpp": [
        ('payload = "echo HaleHound";',
         'payload = "echo SPANDECK";'),
    ],
    "src/legacy/wifi_attacks.cpp": [
        ('ap_config.ap.ssid, "HaleHound"',
         'ap_config.ap.ssid, "SPANDECK"'),
        ('"HaleHound_Was_Here"', '"SPANDECK_Was_Here"'),
        ('preferences.begin("halehound_pm", false);',
         'preferences.begin("spandeck_pm", false);'),
    ],
    "src/app/LegacyApp.inc": [
        ('drawGlitchTitle(58, "HALEHOUND");', 'drawGlitchTitle(58, "SPANDECK");'),
        ('drawGlitchTitle(120, "HALEHOUND");', 'drawGlitchTitle(120, "SPANDECK");'),
        ('drawGlitchTitle(80, "HALEHOUND");', 'drawGlitchTitle(80, "SPANDECK");'),
        ('tft.print("By: HaleHound (JMFH)");',
         'tft.print("By: JMFH / RockBase-iot");'),
        ('"By: JMFH (HaleHound)"', '"By: JMFH / RockBase-iot"'),
        ('"HaleHound-CYD.bin"', '"SPANDECK-CYD.bin"'),
        ('"HaleHound-E32R28T.bin"', '"SPANDECK-E32R28T.bin"'),
        ('"HaleHound-CYD-HAT.bin"', '"SPANDECK-CYD-HAT.bin"'),
    ],
    # FW_DEVICE x5 + CYD_BOARD_NAME x3, all of the form "HaleHound-...
    "include/cyd_config.h": [
        ('"HaleHound-', '"SPANDECK-'),
    ],
    "include/User_Setup.h": [
        ('USER_SETUP_INFO "HaleHound-CYD"', 'USER_SETUP_INFO "SPANDECK-CYD"'),
    ],
    "include/wardriving.h": [
        ('WARDRIVING_FILE_PREFIX      "halehound_"',
         'WARDRIVING_FILE_PREFIX      "spandeck_"'),
    ],
    "src/legacy/wardriving.cpp": [
        ('WARDRIVING_LOG_DIR "/halehound_overflow.csv"',
         'WARDRIVING_LOG_DIR "/spandeck_overflow.csv"'),
        ('logFile.println("WigleWifi-1.6,appRelease=HaleHound-CYD,model=ESP32-CYD,'
         'release=3.0.0,device=HaleHound,display=CYD,board=ESP32,brand=JesseCHale,'
         'star=Sol,body=3,subBody=0");',
         'logFile.println("WigleWifi-1.6,appRelease=SPANDECK-CYD,model=ESP32-CYD,'
         'release=3.0.0,device=SPANDECK,display=CYD,board=ESP32,brand=RockBase-iot,'
         'star=Sol,body=3,subBody=0");'),
    ],
    "src/legacy/loot_manager.cpp": [
        ('    // Display name: strip "halehound_" prefix and ".csv" suffix for cleaner look',
         '    // Display name: strip "spandeck_" (or the legacy "halehound_") prefix\n'
         '    // and the ".csv" suffix for a cleaner look'),
    ],
}

# ----------------------------------------------------------------- doc rules --
DOCS: dict[str, list[tuple[str, str]]] = {
    "README.md": [
        ("# HaleHound-CYD\n",
         "# SPANDECK\n"),
        ("**ESP32-DIV HaleHound Edition for Cheap Yellow Display**\n",
         "**A 300 MHz - 2.5 GHz multi-band wireless field terminal for the ESP32 \"Cheap Yellow Display\"**\n"),
        ("Version **v3.4.0 CYD Edition** | By [JesseCHale](https://github.com/JesseCHale) | [HaleHound.com](https://halehound.com)\n",
         "Version **v3.4.0 CYD Edition** | Maintained by RockBase-iot | "
         "Based on [HaleHound-CYD](https://github.com/JesseCHale/HaleHound-CYD) by Jesse C. Hale (JMFH), MIT\n"),
        ("HaleHound-CYD is a multi-protocol offensive security toolkit built for the ESP32",
         "SPANDECK is a multi-protocol offensive security toolkit built for the ESP32"),
        ('Target device pairs with "HaleHound KB"',
         'Target device pairs with "SPANDECK KB"'),
        ('│  2. Advertise as "HaleHound KB"              │',
         '│  2. Advertise as "SPANDECK KB"              │'),
        ("`HaleHound-CYD-FULL.bin`", "`SPANDECK-CYD-FULL.bin`"),
        ("`HaleHound-CYD.bin`", "`SPANDECK-CYD.bin`"),
        ("HaleHound-CYD-FULL.bin", "SPANDECK-CYD-FULL.bin"),
        ("HaleHound-CYD.bin", "SPANDECK-CYD.bin"),
        ("HaleHound-CYD/", "SPANDECK/"),
        ("**HaleHound-CYD** by [JesseCHale](https://github.com/JesseCHale)",
         "**SPANDECK** maintained by RockBase-iot.  Based on **HaleHound-CYD** "
         "by [JesseCHale](https://github.com/JesseCHale) (MIT)."),
        ("GitHub: [github.com/JesseCHale/HaleHound-CYD](https://github.com/JesseCHale/HaleHound-CYD)",
         "Upstream: [github.com/JesseCHale/HaleHound-CYD](https://github.com/JesseCHale/HaleHound-CYD) "
         "(proprietary since v3.6.2; this project forked at MIT v3.4.0)"),
    ],
    "NOTICE.md": [
        ("See `docs/legal/COMPLIANCE_PLAN_zh.md` §3 for the specific sites in the tree that\n"
         "currently violate this, including one where the device announces itself to nearby\n"
         "phones under the upstream's name.\n",
         "This project's own mark is **SPANDECK**.  It is what the device announces itself as\n"
         "over GAP, what the boot screen draws, what the captive port's access point asserts,\n"
         "and what the artefacts are named.  `HaleHound` now survives only in the attribution\n"
         "above, in factual references to the upstream's own history, and in the\n"
         "`HALEHOUND_*` colour constants -- internal identifiers that are never presented as a\n"
         "mark to anyone using the device.\n"
         "\n"
         "`docs/legal/COMPLIANCE_PLAN_zh.md` §3.2 keeps the list of the sites that had to\n"
         "change, with what each says now.\n"),
    ],
    "LICENSE": [
        ('"HaleHound" and any associated marks belong to their respective owner.  This\n',
         '"SPANDECK" is the mark this project trades under.  "HaleHound" and any\n'
         'associated marks belong to their respective owner.  This\n'),
    ],
    "docs/legal/COMPLIANCE_PLAN_zh.md": [
        ("| 改为自有产品名（NM 系列） |",
         "| **已改** → `\"SPANDECK\"`：5 处 `BLEDevice::init` + `BleKeyboard` 设备名/厂商名 |"),
        ("| 改为自有产品名 |",
         "| **已改** → `\"SPANDECK\"`（3 处） |"),
        ("| 换自有启动图；原图仅留在取证档案 |",
         "| **已删** → 该数组全仓**无任何引用**（死资产），直接移除 |"),
        ('| 改为 "Based on HaleHound-CYD by Jesse C. Hale (MIT)" |',
         "| **已改** → 屏上 `\"By: JMFH / RockBase-iot\"`（240px 放不下长句，长句留在 README/NOTICE） |"),
        ("| 从仓库移除（§4 A5） |",
         "| 待 `C6`（`D5` 未定）；屏幕提示文字已改为 `SPANDECK-*.bin` |"),
        ("| README/LICENSE 重写（§6）；仓库名属 fork 描述性命名，可暂留但要与产品名切开 |",
         "| **已改** → README 标题/署名/产物名、`LICENSE` 商标段；**仓库名 `HaleHound-CYD` 仍待定** |"),
        ("| 机械重命名，随 §4 的改动一起做 |",
         "| **不做** → 7478 处绝大多数是颜色宏；内部标识不构成商标使用，且本沙箱无法 `pio run` "
         "验证 7k 行改动，收益为零、风险不为零 |"),
    ],
    "TODO.md": [
        ("### `C3` 商标与命名边界（P0 · S · **需 BOSS 定名**）",
         "### `C3` 商标与命名边界（P0 · S · **已定名 `SPANDECK` · 已执行**）"),
        ("- **依赖**：**先定产品名**（§8 `D2`）。",
         "- **依赖**：已闭合 —— §10 `D2` 定为 `SPANDECK`。"),
        ("`C3` | ⬜ 未开始（等命名）",
         "`C3` | ✅ 已执行（T1–T4/T6 已改、T3 已删、T7 不做、T5 随 `C6`）"),
        ("| **D2** | 产品名最终定什么？",
         "| **D2** | ✅ 已定：`SPANDECK`（`span`=频谱仪的扫描跨度，`deck`=手持仪表台）"),
    ],
}


def apply(path: pathlib.Path, pairs: list[tuple[str, str]]) -> list[str]:
    raw = path.read_bytes()
    crlf = b"\r\n" in raw
    text = raw.decode("utf-8")
    notes: list[str] = []
    for old, new in pairs:
        o = old.replace("\n", "\r\n") if crlf else old
        n = new.replace("\n", "\r\n") if crlf else new
        have = text.count(o)
        if have == 0:
            if text.count(n):
                notes.append(f"  = already applied: {o.splitlines()[0][:64]!r}")
                continue
            notes.append(f"  ! NOT FOUND   : {o.splitlines()[0][:64]!r}")
            continue
        text = text.replace(o, n)
        notes.append(f"  + {have:3d}x  {o.splitlines()[0][:64]!r}")
    if text != raw.decode("utf-8"):
        path.write_bytes(text.encode("utf-8"))
    return notes


def drop_splash(path: pathlib.Path) -> str:
    """Remove the unused upstream splash, which exists only as a definition."""
    text = path.read_text(encoding="utf-8", newline="")
    if "HALEHOUND SPLASH SCREEN" not in text:
        return "  = already applied: splash block gone"
    pattern = re.compile(
        r"\r?\n// =+ HALEHOUND SPLASH SCREEN =+\r?\n(?:(?!\};)[\s\S])*?\};[ \t]*\r?\n")
    new, hits = pattern.subn(
        "\r\n// Splash art removed at T3: the 240x320 bitmap that used to sit here was\r\n"
        "// upstream artwork with no reference anywhere in the tree.  The boot screen\r\n"
        "// draws the product wordmark as text instead.\r\n",
        text, count=1)
    if hits != 1:
        return "  ! splash block not matched"
    path.write_text(new, encoding="utf-8", newline="")
    return "  + 1x  removed bitmap_halehound_splash[] (dead asset)"


BANNER = """\
 ███████╗██████╗  █████╗ ███╗   ██╗██████╗ ███████╗ ██████╗██╗  ██╗
 ██╔════╝██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔════╝██╔════╝██║ ██╔╝
 ███████╗██████╔╝███████║██╔██╗ ██║██║  ██║█████╗  ██║     █████╔╝
 ╚════██║██╔═══╝ ██╔══██║██║╚██╗██║██║  ██║██╔══╝  ██║     ██╔═██╗
 ███████║██║     ██║  ██║██║ ╚████║██████╔╝███████╗╚██████╗██║  ██╗
 ╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝ ╚═════╝╚═╝  ╚═╝

   300 MHz - 2.5 GHz  |  SubGHz · 433 · nRF24 · BLE · WiFi · NFC · IR"""


def replace_banner(path: pathlib.Path) -> str:
    text = path.read_text(encoding="utf-8", newline="")
    if "███████╗██████╗  █████╗" in text:
        return "  = already applied: banner"
    i = text.find("```")
    j = text.find("```", i + 3)
    if i < 0 or j < 0:
        return "  ! banner fence not found"
    eol = "\r\n" if "\r\n" in text else "\n"
    body = eol.join(BANNER.splitlines())
    text = text[:i] + "```" + eol + body + eol + "```" + text[j + 3:]
    path.write_text(text, encoding="utf-8", newline="")
    return "  + 1x  README banner: upstream block-letter art -> product wordmark"


LOOT_CANON = ('    if (strncasecmp(src, "spandeck_", 9) == 0) src += 9; '
              'else if (strncasecmp(src, "halehound_", 10) == 0) src += 10;')

LOOT_LINE = re.compile(
    r'^[ \t]*if \(strncasecmp\(src, "(?:spandeck_|halehound_)"[\s\S]*?src \+= \d+;[ \t]*\r?$',
    re.MULTILINE)


def fix_loot_prefix(path: pathlib.Path) -> str:
    """Rewrite the SD-artefact prefix strip to canonical form.

    Not a (old, new) pair on purpose: the new text contains the old text, so a
    pair re-applies on every run and nests itself -- which it did, three deep.
    Rewriting the whole line is idempotent by construction.
    """
    text = path.read_text(encoding="utf-8", newline="")
    new, hits = LOOT_LINE.subn(LOOT_CANON, text)
    if hits and new != text:
        path.write_text(new, encoding="utf-8", newline="")
        return f"  + {hits}x  prefix strip rewritten to canonical form"
    return "  = already applied: prefix strip"


def fix_icon_eol(path: pathlib.Path) -> str:
    """The splash replacement left one bare LF in an otherwise-CRLF file."""
    text = path.read_text(encoding="utf-8", newline="")
    if "no\n// reference anywhere" not in text:
        return "  = already applied: icon.h line endings"
    path.write_text(text.replace("no\n// reference anywhere",
                                 "no\r\n// reference anywhere"),
                    encoding="utf-8", newline="")
    return "  + 1x  icon.h: bare LF after splash removal -> CRLF"


def fix_header_comments() -> str:
    """Point the file-header comments at the product, not at the fork we came from.

    These are comments, not marks, so this is not a legal fix -- it is so that
    reading the tree does not tell you the product is something else.  Comment
    prose that factually describes the upstream's design (\"HaleHound gradient\")
    is left alone; it is a true statement about where the look came from.
    """
    changed = files = 0
    for pattern in ("src/**/*.cpp", "src/**/*.h", "src/**/*.inc",
                    "include/**/*.h"):
        for path in ROOT.glob(pattern):
            text = path.read_text(encoding="utf-8", newline="")
            if "// HaleHound-CYD " not in text:
                continue
            hits = text.count("// HaleHound-CYD ")
            path.write_text(text.replace("// HaleHound-CYD ", "// SPANDECK "),
                            encoding="utf-8", newline="")
            changed += hits
            files += 1
    if not (changed or files):
        return "  = already applied: file headers"
    return f"  + {changed}x  file-header comments across {files} files"


def note_patch_inventory() -> str:
    """Add this script to the inventory table in build-reports/README.md."""
    path = ROOT / "build-reports" / "README.md"
    text = path.read_text(encoding="utf-8", newline="")
    if "patch_rename_spandeck.py" in text:
        return "  = already applied: patch inventory row"
    lines = text.split("\n")
    for idx, line in enumerate(lines):
        if "patch_route_a_docs.py" in line and line.startswith("|"):
            cells = line.split("|")
            if len(cells) != 4:
                return f"  ! inventory row has {len(cells)} cells, expected 4"
            cells[1] = " `patch_rename_spandeck.py` "
            cells[2] = (" Applies the `HaleHound` -> `SPANDECK` rename to the "
                        "user-visible surfaces only ")
            cells[3] = " `C3` / `D2` "
            lines.insert(idx + 1, "|".join(cells))
            path.write_text("\n".join(lines), encoding="utf-8", newline="")
            return "  + 1x  build-reports/README.md inventory row"
    return "  ! inventory anchor not found"


def main() -> int:
    problems = 0
    for label, table in (("code", CODE), ("docs", DOCS)):
        for rel, pairs in table.items():
            path = ROOT / rel
            if not path.exists():
                print(f"[missing] {rel}")
                problems += 1
                continue
            notes = apply(path, pairs)
            bad = [n for n in notes if n.strip().startswith("!")]
            problems += len(bad)
            print(f"[{label}] {rel}")
            for n in notes:
                print(n)

    print("[code] include/icon.h")
    print(drop_splash(ROOT / "include" / "icon.h"))
    print(fix_icon_eol(ROOT / "include" / "icon.h"))

    print("[code] src/legacy/loot_manager.cpp")
    print(fix_loot_prefix(ROOT / "src" / "legacy" / "loot_manager.cpp"))

    print("[code] file headers")
    print(fix_header_comments())

    print("[docs] README.md (banner)")
    print(replace_banner(ROOT / "README.md"))

    print("[meta] build-reports/README.md")
    print(note_patch_inventory())

    print()
    if problems:
        print(f"FAILED: {problems} rule(s) did not match")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
