#!/usr/bin/env python3
"""Bring REMAINING_WORK.md §8/§1/§10 and build-reports/README.md up to this
round's findings, and index this script itself.

Track A of TODO.md was rewritten wholesale in the same commit; these are the
two files it left stale:

  * REMAINING_WORK.md still listed "go full GPL" as an option -- in three
    places: the §8 C1 row, the §1 blocker X6, and the §10 ordering line. That
    option does not exist: RF24 is GPL-2.0-only and arduinoFFT is GPL-3.0, and
    those two are mutually incompatible, so there is no single licence that can
    cover the combined binary. It also still said the repository has no
    THIRD_PARTY_NOTICES, and recorded one distribution blocker where there are
    now three.

  * build-reports/README.md documents the tooling in this directory but knew
    nothing about the licence audit, which is the one script here that is also
    a gate.

Anchors are short, distinctive fragments rather than whole table rows: these are
Chinese-language cells on lines hundreds of characters long, and a whole-row
match fails on a single character.

Unlike its siblings, this script is idempotent rather than one-shot: an edit
whose result is already present is reported as applied and skipped, so the
script can be extended and re-run after the first pass instead of needing a
second near-identical file for each follow-up fix.
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
report: list[str] = []


def apply(path: pathlib.Path, name: str, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8", newline="")
    sep = "\r\n" if "\r\n" in text else "\n"
    old_s = old.replace("\n", sep)
    new_s = new.replace("\n", sep)

    if new_s in text:
        report.append(f"already applied  {name}  ({path.relative_to(ROOT).as_posix()})")
        return
    found = text.count(old_s)
    if found != 1:
        sys.exit(f"ABORT, nothing written: {name} expected 1 match, found {found}")
    path.write_text(text.replace(old_s, new_s, 1), encoding="utf-8", newline="")
    report.append(f"applied          {name}  ({path.relative_to(ROOT).as_posix()})")


remaining = ROOT / "REMAINING_WORK.md"
readme = ROOT / "build-reports" / "README.md"

# ---------------------------------------------------------------------------
# REMAINING_WORK.md §8
# ---------------------------------------------------------------------------

apply(
    remaining, "§8 C1 options",
    "三选一（同 `X6`）：替换 / 全 GPL / 不发二进制。",
    "**二选一**（同 `X6`）：替换 / 不发二进制。~~全 GPL~~ 2026-10-16 证伪 —— "
    "GPL-2.0-only 与 GPL-3.0 **互不相容**，不存在能覆盖整体二进制的许可。",
)

apply(
    remaining, "§8 C2 status",
    "仓库**无** `THIRD_PARTY_NOTICES`；",
    "仓库**已有** `THIRD_PARTY_NOTICES.md` / `NOTICE.md` / `LICENSES/`（14 份逐字副本）；",
)

apply(
    remaining, "§8 C2 remaining",
    "① `THIRD_PARTY_NOTICES.md`：逐依赖 SPDX + 版本 + 出处（含 5 个「包内无 LICENSE 文件」的补证结果）② README 加 `Based on HaleHound-CYD by Jesse C. Hale` + 本 fork 修改摘要",
    "① ✅ 本轮完成（5 个「包内无 LICENSE 文件」的依赖已逐个留档：2 个补 SPDX 原文、2 个立占位标记、1 个记复合许可）② README 的归属行与修改摘要**未做**，随 `C3`/`C4` 一起",
)

apply(
    remaining, "§8 C6-C10 rows",
    "保持 `RELEASE_BRIEF_zh.md` 的标注口径，不把自述外推为已验证事实；将来若拿到实机行为，回填为 S1 | — | P2 |",
    "保持 `RELEASE_BRIEF_zh.md` 的标注口径，不把自述外推为已验证事实；将来若拿到实机行为，回填为 S1 | — | P2 |\n"
    "| **C6** | 已提交的 `.bin` 移出仓库（**止血**） | `flash_package/*.bin`（10 个）+ `dist/*.bin` 共 **11 个、18.8 MB** 在 git 中；`dist/` 虽已 ignore，但文件在加 ignore 前已提交，仍被跟踪 | `git rm --cached flash_package/*.bin` + `.gitignore` 补该路径。**仓库公开 = 正在分发**，这些二进制同时带着 `C1` 的不可分发组合与 `C3` 的商标文件名 | 无（但需 BOSS 确认） | **P0** |\n"
    "| **C7** | LGPL-2.1 的静态链接义务 | Arduino core / IDF / `rc-switch` / `TinyGPSPlus` 均为 **LGPL-2.1-or-later**；LGPL 允许静态链接但要求接收方可**重新链接** | ① `LICENSES/` 已备 ② 写「如何重新链接」说明（工具链 + 完整 flag 可从 `build-reports/_syntax_*.rsp` 取真值）③ 声明按请求提供 `.o` ④ 或把后两者换成自写实现 | 无 | **P0** |\n"
    "| **C8** | 逐文件 SPDX 头 | 目前只有仓库级声明，而 MIT/BSD 的归属义务是**分发条件** | 自写文件加 `SPDX-License-Identifier` + 我方 `SPDX-FileCopyrightText`；**改自上游的文件必须保留上游著作权行**。政策见 `COMPLIANCE_PLAN_zh.md §6.2`。**单独一次提交**，避免千行 diff | `C2` | P1 |\n"
    "| **C9** | 依赖许可门禁进 CI | `audit_licenses.py` 已有「有阻断项即 `exit 1`」语义，但未接任何流程 | 接入 `K2` 的 CI 矩阵；新增依赖须同步更新 `LICENSES/` 与 `THIRD_PARTY_NOTICES.md` | `K2` | P1 |\n"
    "| **C10** | SmartRC 许可澄清 | 包内 `LICENSE.txt` 是 **2021 年的自定义文本**（授权范围未写明），**早于上游仓库改标 MIT** | 升级到随附 MIT 文本的版本，或向作者取书面确认；结论回填 `THIRD_PARTY_NOTICES.md` | 无 | P1 |\n"
    "\n"
    "> **2026-10-16 复核更正（本节据此重排）**：\n"
    "> ① `C1` 的选项由「三选一」改为「二选一」—— 见上，全 GPL 路线已被证伪；\n"
    "> ② 阻断项由 1 个变 **3 个**：新增 `ESP32 BLE Keyboard` **完全无授权**（原表只记了它「包内无 LICENSE 文件」，未意识到那意味着「无授权」），以及 `C6` 的「已提交二进制即分发行为」；\n"
    "> ③ **许可链已查清并修正**：本 fork 于 2026-03-16（`68bbfe6`）分叉，**在上游换证之前**（上游 v3.5.0 仍是 MIT、v3.6.2 已是专有）；本仓库带着上游 93 个提交的历史，`LICENSES/HaleHound-CYD-MIT.txt` 即那时的 MIT 原文；上游后来改写了历史，两边 `git merge-base` 为空。\n"
    "> 完整依据、商标分级（T1–T7）与处置矩阵见 `docs/legal/COMPLIANCE_PLAN_zh.md`。",
)

# ---------------------------------------------------------------------------
# REMAINING_WORK.md §1 (blocker X6) and §10 (ordering)
# ---------------------------------------------------------------------------

apply(
    remaining, "§1 X6 options",
    "三选一：① 用自研/permissive 实现替换这两个库（照上游 clean-room 做法）② 接受整个固件按 GPL 分发并履行源码义务 ③ 明确永不分发二进制（仅自用）",
    "**二选一**：① 用自研/permissive 实现替换这两个库（照上游 clean-room 做法，但**代码不可搬运**）或 ② 明确永不分发二进制（仅自用）。"
    "~~③ 接受整个固件按 GPL 分发~~ 2026-10-16 证伪 —— GPL-2.0-only 与 GPL-3.0 **互不相容**，不存在覆盖整体的许可。",
)

apply(
    remaining, "§10 ordering",
    "`C3`（商标/命名）→ `X6`/`C1`（GPL 三选一）→ `C2`（第三方声明）。",
    "`C3`（商标/命名）→ `X6`/`C1`（copyleft 二选一）→ `C2`（第三方声明 ✅ 已完成）→ `C6`（移出已提交的 `.bin`）。",
)

# ---------------------------------------------------------------------------
# build-reports/README.md
# ---------------------------------------------------------------------------

apply(
    readme, "README licence-audit section",
    "## `patch_*.py` — applied-and-verified source edits",
    """## `audit_licenses.py` / `collect_licenses.py` — the licence audit

What this firmware links against, and what may be shipped alongside it.

`audit_licenses.py` reads each installed dependency's licence from the most
specific source that exists — a `library.properties` / `library.json` /
`package.json` declaration, a `LICENSE` / `COPYING` file, or a source header —
classifies it, prints a table and writes `license-audit.json`. It **exits
non-zero while any dependency blocks distribution**, so it works as a gate rather
than as a report somebody has to remember to read. It exits non-zero today.

`collect_licenses.py` turns that result into `LICENSES/`: the exact texts a
distribution has to carry, copied byte for byte from the installed library where
one is shipped, and from the canonical SPDX text where a library names a licence
without shipping it. Libraries that grant nothing get a marker file instead, so
the gap is visible in the tree. One file there is not generated — see `PRESERVED`
in that script, and the warning attached to it.

Two classification traps are handled explicitly, because the obvious rule gets
both of them wrong:

- Match on the **earliest** occurrence in the text, not on pattern order. The
  canonical GPLv2 text closes by suggesting "the GNU *Lesser* General Public
  License", and GPLv3's section 13 names the "GNU *Affero* General Public
  License" — so scanning in pattern order reports RF24 as LGPL and arduinoFFT as
  AGPL. Both are plain GPL.
- Test the **distinguishing** clause for BSD. BSD-3 contains BSD-2's first clause
  verbatim, so earliest-match alone reports a three-clause file as two-clause.

Result on 2026-10-16: **2 strong copyleft that are incompatible with each other,
1 with no licence at all, 2 weak copyleft, 5 permissive** — the firmware cannot
be redistributed as built. Evidence, the trademark inventory and the plan are in
`docs/legal/COMPLIANCE_PLAN_zh.md`.

## `patch_*.py` — applied-and-verified source edits""",
)

apply(
    readme, "README patch-table row",
    "| `patch_u2_docs.py` | U2 docs: `REMAINING_WORK.md` §0.7, the two stale claims in this file, the evidence doc §6 |\n"
    "| `patch_compliance_docs.py` | the licence/trademark pass: `REMAINING_WORK.md` §8 (the \"full GPL\" option deleted, C6-C10 added, the relicense window recorded) and this file's licence-audit section |",
    "| `patch_u2_docs.py` | U2 docs: `REMAINING_WORK.md` §0.7, the two stale claims in this file, the evidence doc §6 |\n"
    "| `patch_compliance_docs.py` | the licence/trademark pass: `REMAINING_WORK.md` §8 (the \"full GPL\" option deleted, C6-C10 added, the relicense window recorded), §1 X6 and §10 (the same correction), and this file's licence-audit section |",
)

apply(
    readme, "README patch-table row (evidence script)",
    "| `patch_compliance_docs.py` | the licence/trademark pass: `REMAINING_WORK.md` §8 (the \"full GPL\" option deleted, C6-C10 added, the relicense window recorded), §1 X6 and §10 (the same correction), and this file's licence-audit section |",
    "| `patch_compliance_docs.py` | the licence/trademark pass: `REMAINING_WORK.md` §8 (the \"full GPL\" option deleted, C6-C10 added, the relicense window recorded), §1 X6 and §10 (the same correction), and this file's licence-audit section |\n"
    "| `patch_remaining_work_evidence.py` | `REMAINING_WORK.md` §0.8 (the seven facts that moved priorities), blockers X6-X9, and the new §8/§9 on compliance and the shared bus, written from the upstream field evidence |",
)

for line in report:
    print(line)
print(f"\n{len(report)} edit group(s); "
      f"{sum(1 for l in report if l.startswith('applied'))} written this run.")
