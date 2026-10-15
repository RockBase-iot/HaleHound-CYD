#!/usr/bin/env python3
"""Patch DEVELOPMENT_PLAN.md after the 2026-10-15 RFID/BLE model-layer session.

Every replacement asserts it fires exactly once so a silent miss is impossible.
"""
from __future__ import annotations

import sys
from pathlib import Path

PLAN = Path(__file__).resolve().parent.parent / "DEVELOPMENT_PLAN.md"

EDITS: list[tuple[str, str, str]] = []


def edit(label: str, old: str, new: str) -> None:
    EDITS.append((label, old, new))


# --- 1. §5.4 已闭合项：宿主跑测数字 ---
edit(
    "s5.4-host-count",
    "链接 + **执行**全部 native 套件。2026-10-15 实测：**29/29 套件、142 个用例全绿**。",
    "链接 + **执行**全部 native 套件。2026-10-15 实测：**36/36 套件全绿**\n"
    "（29 套基线 + 本轮 RFID/BLE 新增 7 套；基线首次执行时为 142 个用例）。",
)

# --- 2. 在 §5.5 之后插入 §5.6 ---
edit(
    "s5.6-insert",
    "（UID 字节 0..3 + BCC + SAK + ATQA），**实现是对的**，已把夹具改成真实厂商块布局。\n\n---\n",
    "（UID 字节 0..3 + BCC + SAK + ATQA），**实现是对的**，已把夹具改成真实厂商块布局。\n"
    "\n"
    "### 5.6 模型层跑测暴露的缺陷（2026-10-15，RFID/BLE 差距项）\n"
    "\n"
    "本轮新增 7 套测试后，36/36 全绿。但**先写模型后写测试**的模块里查出了 7 个真实缺陷，\n"
    "其中 5 个只有真的跑断言才会暴露 —— 再次印证「代码看起来合理」不构成任何证据：\n"
    "\n"
    "| 位置 | 缺陷 | 为什么 code review 看不出来 |\n"
    "|---|---|---|\n"
    "| `MifareClassicModel` 尾块权限表 | 把「仅 Key A」错实现成「Key A 或 Key B」 | 语义上「差不多」；**证据文档 §1.3 的表格本来是对的，是实现漂移了** |\n"
    "| `GattNameTable` | 13 处名称错 + 7 个根本不该存在的条目，其中把**服务** `0x1813` 的名字安到了**特征** `0x2A31` 上 | 凭记忆写的 165 条表，看着完全合理 |\n"
    "| `RfidBackendModel::screenLabel` | `BackendPicker` 与 `CardReader` 都返回 `\"Reader\"` | 七屏里两屏同名，顶栏无法区分当前在哪一屏 |\n"
    "| `ChameleonUltraModel::evaluateRead(20, 16)` | 返回 `valid = true` + `\"complete read\"` | 读到的页数比卡上还多，是驱动计数错误，却给出肯定结论 |\n"
    "| `ChameleonSession::onDeviceFound(\"\")` | 采纳空地址，会话进 `Ready` 却没有可连对象 | — |\n"
    "| `SkeletonKeyModel::classifyAppearance` | `0x03C3`(摇杆) / `0x03C5`(数字化仪) 报成 `Keyboard` | 正是该函数注释里说「绝不发明设备类型」要避免的行为 |\n"
    "| `SkeletonSession::canWriteFullDump()` | 只查树不查状态；`requestStop()` 只改状态不清树 → **已停止的会话仍声称可写全量转储** | — |\n"
    "\n"
    "`GattNameTable` 的解法值得固化：**不靠眼睛，拉 Bluetooth SIG 机读清单做 diff**，并把规则\n"
    "设计成零例外 —— SIG 已分配必须逐字一致（SIG 官方就叫 `GAP` / `Battery`，不是\n"
    "`Generic Access` / `Battery Service`），未分配必须带 `(vendor)` / `(member)` 标记，\n"
    "无法核验的条目**删除**而不是写进例外清单。核验器：`build-reports/verify_gatt_names.py`。\n"
    "\n"
    "---\n",
)

# --- 3. §9 任务表：主机跑测数字 ---
edit(
    "s9-row5",
    "| 5 | ✅ **主机跑测**：29/29 套件、142 个用例全绿（首次执行；顺带修掉 9 个此前从未被编译过的潜伏失败，见 §5.5） | 断言级验证已可用 |",
    "| 5 | ✅ **主机跑测**：**36/36 套件全绿**（基线首次执行时 29/29、142 个用例；本轮新增 7 套）。累计修掉 9 个从未被编译过的潜伏失败（§5.5）+ 7 个模型层真实缺陷（§5.6） | 断言级验证已可用 |",
)

# --- 4. §9 任务表：RFID 主动路径现状 ---
edit(
    "s9-row7",
    "| 7 | **RFID 主动路径**：PN532 驱动 → MIFARE Classic 转储 → Key Check → ST25R3916 | 上游 v4.0 的绝对重心；模型层已就位 |",
    "| 7 | **RFID 主动路径**：PN532 驱动 → MIFARE Classic 转储 → Key Check → ST25R3916 | 上游 v4.0 的绝对重心。模型层已就位（`MifareKeys` / `MifareClassicModel` / `KeyCheckModel` + 29 例测试），**缺驱动层** |",
)

# --- 5. §9 任务表：新增模型接线行 ---
edit(
    "s9-row9",
    "| 8 | **`LegacyApp.inc` 分域切分**（Tools/Settings 优先） | 降低后续每项改动的回归面 |",
    "| 8 | **`LegacyApp.inc` 分域切分**（Tools/Settings 优先） | 降低后续每项改动的回归面 |\n"
    "| 9 | **把 13 个新模型接进 `FeatureCatalog`** | `NdefCodec` / `CardDumpFormat` / `IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / `ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / `MifareClassicModel` / `KeyCheckModel` / `GattNameTable` **目前全是孤岛**：无 `FeatureId`、无生命周期工厂、无菜单入口。不接线则一切「已实现」停在测试里 |\n"
    "\n"
    "> 完整剩余工作清单（含阻塞项、证据门、建议执行顺序）见根目录 **`REMAINING_WORK.md`**。",
)


def main() -> int:
    text = PLAN.read_text(encoding="utf-8")
    failures: list[str] = []

    for label, old, new in EDITS:
        count = text.count(old)
        if count != 1:
            failures.append(f"{label}: matched {count} times (expected 1)")
            continue
        text = text.replace(old, new, 1)

    if failures:
        print("ABORT -- no changes written:", file=sys.stderr)
        for f in failures:
            print("  " + f, file=sys.stderr)
        return 1

    PLAN.write_text(text, encoding="utf-8")
    print(f"DEVELOPMENT_PLAN.md patched: {len(EDITS)} edit(s) applied.")
    for label, _, _ in EDITS:
        print(f"  ok  {label}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
