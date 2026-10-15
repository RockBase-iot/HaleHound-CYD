#!/usr/bin/env python3
"""Refresh docs/upstream-analysis/RELEASE_BRIEF_zh.md after the 2026-10-15
RFID/BLE model-layer session.

Only the parts that are now factually stale are touched:
  * the gap matrix (#11/#12/#13/#25) and its statistics line,
  * the module / test-suite inventories in section 6,
  * the two verification rows that still claim native tests cannot run,
  * blockers 8.1 and 8.4, both since closed,
  * the recommended ordering in section 9.

Every exact-string edit asserts a unique match, so a silent miss is impossible.
"""
from __future__ import annotations

import sys
from pathlib import Path

DOC = Path(__file__).resolve().parent.parent / "docs" / "upstream-analysis" / "RELEASE_BRIEF_zh.md"

EDITS: list[tuple[str, str, str]] = []


def edit(label: str, old: str, new: str) -> None:
    EDITS.append((label, old, new))


# ---------------------------------------------------------------- gap matrix
edit(
    "gap-11",
    "| 11 | **MIFARE Classic 全卡转储 / 密钥写入 / 魔术卡** | v4.0 | 无 | 需 ISO14443-A 认证链 | P0 |",
    "| 11 | **MIFARE Classic 全卡转储 / 密钥写入 / 魔术卡** | v4.0 | **模型层已补** `MifareClassicModel` + `MifareKeys`（17 例测试） | 模型 ✅ / ISO14443-A 认证链 ❌；魔术卡后门按证据门标 `UNSUPPORTED` | P0 |",
)

edit(
    "gap-12",
    "| 12 | **Key Check（29 公开密钥扫描）** | v4.0 | 无 | 依赖 #11 | P0 |",
    "| 12 | **Key Check（29 公开密钥扫描）** | v4.0 | **模型层已补** `KeyCheckModel`（12 例测试） | 模型 ✅ / 驱动扫描 ❌；密钥表规则已写明（proxmark3 `mfc_default_keys.dic` 前 29 条） | P0 |",
)

edit(
    "gap-13",
    "| 13 | **NTAG Tools 写入侧**（擦除/永久只读/设密码/清密码） | v3.6.0 | 无（仅 NdefCodec 编解码） | 需 PN532 写入路径 | P1 |",
    "| 13 | **NTAG Tools 写入侧**（擦除/永久只读/设密码/清密码） | v3.6.0 | 容量/锁位/密码模型已有（`NtagModel` + `NdefCodec`），**写入路径无** | 需 PN532 写入路径 + 危险操作二次确认 | P1 |",
)

edit(
    "gap-25",
    "| 25 | **BLE 名称数据库**（厂商/服务/GAP） | v3.3.0/v3.9.1 | 无 | 需 PROGMEM 二分表 | P1 |",
    "| 25 | **BLE 名称数据库**（厂商/服务/GAP） | v3.3.0/v3.9.1 | **模型已补** `GattNameTable`（165 项，已逐字对齐 Bluetooth SIG 机读清单） | 模型 ✅ / PROGMEM 化与 legacy `ble_database.h` 合并 ❌ | P1 |",
)

edit(
    "gap-stats",
    "**统计**：已闭合 9 · P0 待补 6 · P1 待补 14 · P2 待补 5 · 有意偏离 4。",
    "**统计**（按上表逐行口径，2026-10-15 更新）：**已闭合 5** · **模型层已补、缺驱动/接线 7**\n"
    "（其中 P0 三项：#9 / #11 / #12）· **P0 待补 3**（#10 / #16 / #17）· **P1 待补 12** ·\n"
    "**P2 待补 4** · **有意偏离 4**。\n"
    "\n"
    "> 「模型层已补」= 纯逻辑经宿主断言验证，但**没有真实传输/驱动，也未接入 `FeatureCatalog`**，\n"
    "> 因此**不等于功能可用**。剩余工作量与建议执行顺序见根目录 `REMAINING_WORK.md`。",
)

# ------------------------------------------------------------------ section 6
edit(
    "s6-note",
    "## 6. 本轮已交付的开发\n\n### 6.1 新增纯逻辑模块（9 个，2553 行）",
    "## 6. 本轮已交付的开发\n"
    "\n"
    "> **范围说明**：§6.1–§6.3 记录的是 2026-10-15 上午那一轮（9 个模块）。同日下午又补了\n"
    "> 4 个模型与 7 套测试，见 §6.1b / §6.3b。\n"
    "\n"
    "### 6.1 新增纯逻辑模块（9 个，2553 行）",
)

edit(
    "s6.1b-insert",
    "| `HidReaderModel` | `src/features/ble/HidReaderModel.{h,cpp}` | 126 | v4.0 HID reader tooling |\n",
    "| `HidReaderModel` | `src/features/ble/HidReaderModel.{h,cpp}` | 126 | v4.0 HID reader tooling |\n"
    "\n"
    "### 6.1b 同日追加的纯逻辑模块（4 个，1533 行）\n"
    "\n"
    "| 模块 | 文件 | 行数 | 对应上游动作 |\n"
    "|---|---|---|---|\n"
    "| `MifareClassicModel` | `src/features/rfid/MifareClassicModel.{h,cpp}` | 606 | v4.0 MIFARE Classic 全卡转储 / 密钥写入 |\n"
    "| `GattNameTable` | `src/features/ble/GattNameTable.{h,cpp}` | 451 | v3.3.0/v3.9.1 BLE 名称数据库 |\n"
    "| `KeyCheckModel` | `src/features/rfid/KeyCheckModel.{h,cpp}` | 251 | v4.0 Key Check（29 公开密钥） |\n"
    "| `MifareKeys` | `src/features/rfid/MifareKeys.{h,cpp}` | 225 | v4.0 公开密钥表 |\n"
    "\n"
    "沿用的「把诚实性约束固化成不可绕过代码路径」的做法同样适用：\n"
    "\n"
    "- `KeyCheckModel`：进度由**尝试次数**推导而不是扇区比例；扇区**只在认证成功时计数**；\n"
    "  汇总固定为「N of M sectors recovered, K need a nested/hardnested attack」——\n"
    "  **不做攻击也不假装做了**。\n"
    "- `MifareClassicModel`：尾块 `C1C2C3 = 111` 的扇区报告为**永久锁死**；\n"
    "  魔术卡写入要求 `backdoorDetected && userConfirmed` **两个条件同时成立**；\n"
    "  `setSectorKeyA/B` 有去重守卫，一个扇区只计一次（即使 A、B 都恢复）。\n"
    "- `GattNameTable`：165 项逐字对齐 Bluetooth SIG 机读清单；未分配项必须带 `(vendor)` /\n"
    "  `(member)` 标记；**无法核验的条目直接删除，不写进例外清单**（核验器\n"
    "  `build-reports/verify_gatt_names.py` 做零例外比对）。\n",
)

edit(
    "s6.3b-insert",
    "| `test_karr_classifier` | 8 | framing 分类、**永不命令**、不稳定=Patched、加密 framing 永不 Vulnerable、未知 framing 命名、key 优先、连接失败原因、从不回答 |\n",
    "| `test_karr_classifier` | 8 | framing 分类、**永不命令**、不稳定=Patched、加密 framing 永不 Vulnerable、未知 framing 命名、key 优先、连接失败原因、从不回答 |\n"
    "\n"
    "### 6.3b 同日追加的 native 测试（7 套，共 75 个用例）\n"
    "\n"
    "| 套件 | 用例数 | 关键断言 |\n"
    "|---|---|---|\n"
    "| `test_mifare_classic` | 17 | `FF 07 80 69` 解码为 `C1=0,C2=0,C3=8`、尾块权限「仅 Key A」、反码不一致被拒、`00 F0 FF 69` 报告永久锁死、尾块往返、魔术卡双条件门禁、转储诚实计数 |\n"
    "| `test_key_check` | 12 | 进度按尝试次数、扇区仅在认证成功时计数、密钥表索引与断言一致、诚实汇总文案 |\n"
    "| `test_skeleton_key` | 12 | GATT 树计数、名称解析接入、`classifyAppearance` 不发明设备类型、停止后不可再写全量转储 |\n"
    "| `test_chameleon_ultra` | 10 | 8 槽读写、auto-connect 拒绝二次接管、空地址不被采纳、超读不判 valid、`N of 16` 诚实报告 |\n"
    "| `test_gatt_names` | 9 | 表非空且严格升序无重复、44/113/15 计数钉住、SIG 原文（`GAP`/`Battery`）、未分配项带来源标记 |\n"
    "| `test_rfid_backend` | 8 | 七屏 `screenLabel()` 两两不同、能力矩阵、按后端显隐屏项 |\n"
    "| `test_hid_reader` | 7 | 每连接最多 2 条命令、仅 `replyCaptured` 才标 completed、被动不主动发命令 |\n",
)

# -------------------------------------------------------------- section 7
edit(
    "s7-native-run",
    "| 25 套 native 测试实际执行 | `run_host_tests.py` / `pio test -e native` | ❌ **本机无可用宿主 C++ 编译器**（见 §8.1） |",
    "| **36 套 native 测试实际执行** | `scripts/hostcheck/run_host_tests.py`（仓库本地 `ziglang` / clang 21） | ✅ **36/36 套件全绿**（2026-10-15，含新增 7 套；§8.1 已闭合） |",
)

# -------------------------------------------------------------- section 8
edit(
    "s8.1-head",
    "### 8.1 native 测试本机跑不起来\n\n- `toolchain-gccmingw32/libexec/.../cc1plus.exe` 存在，但**同目录缺 `libgmp-10.dll` /",
    "### 8.1 native 测试本机跑不起来 ✅ 已闭合（2026-10-15）\n"
    "\n"
    "> **解法**：不装系统级工具链，改用仓库本地 `build-reports/hostenv/` 里的 `ziglang`\n"
    "> （pip wheel，自带 clang 21 + libc++/libc sysroot），由 `scripts/hostcheck/run_host_tests.py`\n"
    "> 驱动。实测 **36/36 套件全绿**。\n"
    "> 下面的 mingw32 分析保留，因为它解释了为什么**必须探针编译再采纳**——该包的\n"
    "> `g++ --version` 正常，但每次编译都以**完全空的日志**失败，靠 `Path.exists()` 或\n"
    "> `--version` 都检测不出来。\n"
    "\n"
    "- `toolchain-gccmingw32/libexec/.../cc1plus.exe` 存在，但**同目录缺 `libgmp-10.dll` /",
)

edit(
    "s8.4-body",
    "### 8.4 合规审批流未闭环\n"
    "\n"
    "`findings.md` 记录：当前多个攻击入口各自复制 `isOffensiveAllowed()` / `showDisclaimerScreen()`，\n"
    "且 **Blue Team 拦截后仍可再次接受免责声明解除 Blue Team**。目标是 `PolicyService` 单点授权、\n"
    "覆盖菜单/深链/功能间交接。本轮新增的 6 个功能描述了 `RequiresPolicy = true`（KARR 为\n"
    "`Passive`），但**门禁收口本身没做**——这是 P0，且属于需要你确认的合规范围。",
    "### 8.4 合规审批流 ✅ 已闭合（`PolicyService` 单点授权）\n"
    "\n"
    "> **结算**：攻击入口各自复制 `isOffensiveAllowed()` / `showDisclaimerScreen()` 的做法已收敛到\n"
    "> `PolicyService` 唯一门禁，**Blue Team 粘滞**（拦截后不能靠再次接受免责声明解除），\n"
    "> 由 `test_policy_single_point` 覆盖。详见 `DEVELOPMENT_PLAN.md` §3.3 与 §9 任务 1。\n"
    "\n"
    "（以下为收口前的现状记录，保留作背景）\n"
    "\n"
    "`findings.md` 记录：多个攻击入口各自复制 `isOffensiveAllowed()` / `showDisclaimerScreen()`，\n"
    "且 Blue Team 拦截后仍可再次接受免责声明解除 Blue Team。",
)

S9_HEADING = "## 9. 下一步建议排序"
S9_NEW = """## 9. 下一步建议排序（2026-10-15 更新）

**完整清单见根目录 `REMAINING_WORK.md`**（含阻塞项、证据门、逐项依赖）。这里只给排序理由。

| 序 | 事项 | 理由 |
|---|---|---|
| 1 | **把 13 个新模型接进 `FeatureCatalog`** | 目前全是孤岛（无 `FeatureId` / 无生命周期工厂 / 无菜单入口）。不接线则一切「已实现」停在测试里，成本最低、杠杆最高 |
| 2 | **`CardDumpFormat` → `StorageService` → Loot 查看器** | 打通缺口 #15 与台架用例 B1（射频后 Loot 读卡）；上游连续三次加码的方向 |
| 3 | **沙箱外跑一次 `pio run`**（§8.2） | 不恢复出 `.bin` 的能力，一切验收都停在 `Build Verified` 之前 |
| 4 | **双主板台架回归 B1–B4**（缺口 #16/#17/#18） | 只能上机验证；`HeapBaseline` / `FeatureSwitchLedger` 目前只有模型自测数据 |
| 5 | **RFID 驱动链**：PN532 → ISO14443-A 认证 → MIFARE 转储 → Key Check → ST25R3916 | 上游 v4.0 的绝对重心；模型层已就位，缺的全是驱动与射频 |
| 6 | **七屏 UI 重构** + `RfidBackendModel` 能力矩阵接线 | 依赖 5 的能力上报 |
| 7 | **BLE client 传输层**（SkeletonKey / Chameleon / HID reader） | 三套模型已就位（29 例测试），卡在 `X3` UUID 证据与 BLE client 实现 |
| 8 | 其余 P1：触摸层（Fast Touch）/ 内存归还 / Wardriving 双核 / SubGHz 频谱 / Replay 选择器 / KeeLoq / IoT Recon | 按硬件齐备度排 |

**已结清的旧建议**：① `PolicyService` 单点授权 ✅；② 基线提到 v4.0 ✅；③ 宿主编译器决策 ✅
（用仓库本地 `ziglang` 解决，未在机器上加装系统级工具链）。
"""


def main() -> int:
    text = DOC.read_text(encoding="utf-8")
    failures: list[str] = []

    for label, old, new in EDITS:
        count = text.count(old)
        if count != 1:
            failures.append(f"{label}: matched {count} times (expected 1)")
            continue
        text = text.replace(old, new, 1)

    # section 9 is replaced wholesale, bounded by the next horizontal rule.
    if text.count(S9_HEADING) != 1:
        failures.append(f"s9-heading: matched {text.count(S9_HEADING)} times (expected 1)")
    else:
        start = text.index(S9_HEADING)
        end = text.index("\n---\n", start)
        text = text[:start] + S9_NEW + text[end + 1 :]

    if failures:
        print("ABORT -- no changes written:", file=sys.stderr)
        for f in failures:
            print("  " + f, file=sys.stderr)
        return 1

    DOC.write_text(text, encoding="utf-8")
    print(f"RELEASE_BRIEF_zh.md patched: {len(EDITS)} exact edit(s) + section 9.")
    for label, _, _ in EDITS:
        print(f"  ok  {label}")
    print("  ok  s9-wholesale")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
