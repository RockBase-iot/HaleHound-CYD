#!/usr/bin/env python3
"""Bring REMAINING_WORK.md up to date after the PN532 driver layer landed.

What changed in the tree this round:
  * four new host suites (36 -> 40), 7 new translation units;
  * IRfidTransport / Pn532Protocol / Pn532Transport / Iso14443aFraming /
    MifareClassicDriver / St25r3916Transport all written and test-covered;
  * R3/R4/R5/R7 are done at the driver layer; what remains for them is UI,
    SD persistence and a second confirmation gate;
  * R1 is reduced to "reserved interface" by BOSS's instruction, and demoted
    to P1 because PN532 can now carry the P0 items;
  * a real bug was found and fixed in Pn532Transport::transceive().

Every replacement asserts its match count so a silent no-op fails loudly.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "REMAINING_WORK.md"

EDITS: list[tuple[str, str, int]] = [
    # 1. Snapshot basis.
    (
        "| 快照依据 | 工作树全量宿主回归 `36/36` 套件通过（`ziglang` clang 21，69 个共享对象） |",
        "| 快照依据 | 工作树全量宿主回归 `40/40` 套件通过（`ziglang` clang 21，76 个共享对象）；"
        "7 个新 TU 经真实 C5 工具链 `-fsyntax-only` 通过 |",
        1,
    ),
    # 2. One-minute conclusion.
    (
        """- **可验证的逻辑层已经基本铺满**：RFID / BLE 的 13 个纯逻辑模型 + 36 套宿主测试全绿。
- **但全部是孤岛**：没有任何一个模型注册进 `FeatureCatalog`，也没有一条真实传输/驱动路径。
  当前状态是 `Model Verified`，离 `Build Verified` 与 `Hardware Verified` 都还差一层。
- **下一步最高 ROI 的不是再加模型，而是补「驱动层」和「接线」**：
  ① 让新模型进 catalog（低成本、立刻可用）→ ② `CardDumpFormat` 落 SD（打通 B1 回归）
  → ③ ST25R3916 / PN532 驱动（RFID 全部 P0 项的共同前置）。""",
        """- **可验证的逻辑层已经基本铺满**：RFID / BLE 的 13 个纯逻辑模型 + 40 套宿主测试全绿。
- **RFID 的 PN532 驱动链本轮已打通**：`IRfidTransport`（传输抽象）→ `Pn532Protocol`（主机帧
  编解码 + 状态码）→ `Pn532Transport`（I2C 命令时序）→ `Iso14443aFraming`（ISO14443-A 字节层
  + CRC_A）→ `MifareClassicDriver`（会话：激活/认证/转储/写前拒绝），每层一套独立宿主测试。
  ST25R3916 按 BOSS 指令**只做预留接口**，见 `docs/protocol-evidence/st25r3916.md`。
- **但 UI 与接线仍是孤岛**：13 个模型 **加上** 本次 5 组驱动文件全部没有注册进
  `FeatureCatalog` —— 无 `FeatureId`、无生命周期工厂、无菜单入口。当前状态是
  `Model Verified` + 部分 `Build Verified`，离 `Hardware Verified` 还差一层。
- **下一步最高 ROI**：① 新模型/驱动进 catalog（低成本、立刻可用）→ ② `CardDumpFormat` 落 SD
  （打通 B1 回归）→ ③ RFID 七屏 UI（R2 —— 驱动前置已经就位，可以直接开工）。""",
        1,
    ),
    # 3. X1 gains the syntax-check result.
    (
        "| 在沙箱外执行 `pio run -e nm-cyd-c5 -e esp32-cyd`，或批准一次性提升删除预算 |",
        "| 在沙箱外执行 `pio run -e nm-cyd-c5 -e esp32-cyd`，或批准一次性提升删除预算。"
        "**本轮 7 个新 TU 已用真实 C5 工具链 `-fsyntax-only` 验证 7/7 通过（不链接、不跑 "
        "`extra_scripts`）** |",
        1,
    ),
    # 4. Section 2 intro becomes the architecture summary.
    (
        """## 2. RFID 主战场（`DEVELOPMENT_PLAN.md` 阶段 7）

上游 v4.0 的绝对重心。模型层已完成，缺的全是驱动与 UI。""",
        """## 2. RFID 主战场（`DEVELOPMENT_PLAN.md` 阶段 7）

上游 v4.0 的绝对重心。**模型层已完成，PN532 驱动层本轮已打通**；剩下的是 UI、落盘与
ISO-DEP。

### 2.0 本轮落地的驱动分层（PN532）

四层，逻辑与硬件完全分离，每层一套独立宿主套件：

```
MifareClassicDriver   会话：激活 / 逐扇区认证 / 转储 / 写前拒绝   ← test_mifare_classic_driver
        ↓ 依赖
Iso14443aFraming      ISO14443-A 字节层：级联 SELECT / CRC_A / NTAG 命令  ← test_iso14443a_framing
        ↓ 依赖
IRfidTransport        传输抽象（wire seam）+ CRC_A 原语
        ↙                                    ↘
Pn532Transport        PN532 I2C 时序 + 状态码诚实映射   ← test_pn532_transport
St25r3916Transport    预留；恒 Unsupported            （无可测行为，故无套件）
        ↓ 依赖
Pn532Protocol         主机帧编解码 + Table 13 状态码     ← test_pn532_protocol
```

三条贯穿全层的设计约束（均已被测试钉死）：

1. **卡判决 ≠ 总线故障。** 状态码 `0x14` → `AuthFailed`（卡拒绝了这把密钥），未定义码 →
   `IoError`。两者若塌成一个返回值，Key Check 会把死掉的总线报成一张在保护密钥的卡。
2. **MIFARE 的 UID quirk 被关在传输层。** 认证帧末尾追加 PICC UID 这一偏离只发生在
   `Pn532Transport::transceive()` 内；`buildAuthenticate()` 保持数据手册的 8 字节形式，
   因此将来接入裸收发器（ST25R3916）时送出的仍是规范帧。
3. **「拒绝」与「发了但失败」必须可区分。** 写路径上的每一条拒绝（块 0 / 尾块 / 访问位
   不允许 / 扇区永久锁死）都发生在**任何帧上行之前**，
   `FakeRfidTransport::requestCount()` 让这一点成为可断言的属性。

> **本轮实测发现并修复的一个真 bug**：`Pn532Transport::transceive()` 在调用
> `dataExchange()` 前把 `rxLength` 清零，而该参数是**调用方容量**的入参 —— 于是任何带载荷
> 的应答都被判成「装不下」而返回 `Unsupported` 并把长度归零。只回状态字节的路径不受影响，
> 所以先前的「全绿」掩盖了它。修复后由
> `test_a_reply_too_large_for_the_caller_buffer_is_refused` 钉住契约的另一半。""",
        1,
    ),
    # 5. R1.
    (
        "| **R1** | ST25R3916 SPI 驱动 | 无（仅 `RfidBackendModel` 里的能力枚举） | "
        "初始化/场开断、FIFO 收发、IRQ 处理、ISO14443-A/B 帧编解码、透明模式、板级 CS/IRQ/电源引脚定义 | "
        "硬件 + 数据手册 | **P0** |",
        "| **R1** | ST25R3916 SPI 驱动 | **预留接口已就位**：`St25r3916Transport`（全方法 "
        "`Unsupported`、`caps()` 全否、`begin()` 恒 `false`）+ 10 步上电计划表 "
        "`st25r3916BringUpSteps()` + `st25r3916SiliconCaps()`。**寄存器驱动未写**（按 BOSS 指令） | "
        "寄存器层十步：SPI 访问自检（读 `0x3F`）→ 直接命令 → IRQ → 场开关 → 软件成帧 → "
        "CRC/奇偶 → 带超时收发 → nonce → iCLASS → 仿真；板级 CS/IRQ/电源引脚需原理图确认 | "
        "硬件 + 数据手册 | **P1**（降级：P0 项已由 PN532 承载） |",
        1,
    ),
    # 6. R3.
    (
        "| **R3** | ISO14443-A 认证链 | `MifareClassicModel` + `MifareKeys` 就位（17 例测试） | "
        "REQA/WUPA → 防冲突 → SELECT → 认证 `0x60/0x61` → 读/写块；重试与超时 | R1 或 PN532 通道 | **P0** |",
        "| **R3** | ISO14443-A 认证链 | **已完成**（PN532 通道）：`Iso14443aFraming`（REQA/WUPA、"
        "级联 SELECT、CRC_A）+ `Pn532Transport`（`InListPassiveTarget` 在硅片内完成整个 "
        "ISO14443-3，认证帧带 UID）+ `MifareClassicDriver::authenticate()`；4 套宿主测试覆盖 | "
        "裸收发器路径仍需 R1 的第 5/6 步（软件成帧 + CRC/奇偶） | — | ✅ |",
        1,
    ),
    # 7. R4.
    (
        "| **R4** | MIFARE Classic 全卡转储 | 转储装配、权限解码、诚实计数（`N of M`）已建模 | "
        "扇区遍历顺序（1K/4K/Mini）+ 失败块占位 + 落盘 | R3 | **P0** |",
        "| **R4** | MIFARE Classic 全卡转储 | **驱动层已完成**：`dumpCard()` 逐扇区走 "
        "`authenticateSectorWithTable()`，失败块记 **unknown 而非零**（有测试钉死「密钥开了但块"
        "读不出 → `recoveredSectors=1`，块全 unknown」）；转储装配/权限解码/诚实计数沿用 "
        "`MifareClassicModel` | **落盘**与进度回显，见 `R10` / `R2` | R10 | **P0**（仅剩落盘） |",
        1,
    ),
    # 8. R5.
    (
        "| **R5** | 密钥写入 | 尾块构造、访问位打包、永久锁死检测已建模 | "
        "驱动写尾块 + 写前权限校验 + 永久报废二次确认 UI | R3 | **P0** |",
        "| **R5** | 密钥写入 | **驱动层已完成**：`writeTrailer()` 写前检查「永久锁死"
        "（C1C2C3=111）」与「访问位写权限」，拒绝不一致的待写入访问字节，写后回读校验"
        "（Key A 读不回 → 只校验访问字节 + 可读时的 Key B）；`writeBlock()` 拒绝块 0 与尾块 | "
        "**永久报废二次确认 UI** + 与 `R2` 接线 | R2 | **P0**（仅剩 UI） |",
        1,
    ),
    # 9. R7.
    (
        "| **R7** | Key Check（29 公开密钥扫描） | `KeyCheckModel` + 29 密钥表就位（12 例测试），"
        "诚实进度上报 | 驱动扫描循环 + 进度回显；嵌套/硬嵌套攻击（当前**只做诚实报告，不做攻击**，"
        "需单独决策） | R3 | **P0** |",
        "| **R7** | Key Check（29 公开密钥扫描） | **驱动原语已完成**："
        "`authenticateSectorWithTable()` 每扇区先试 NFC Forum 键"
        "（`KeyCheckSession::preferredKeyIndex`）再走 29 键表；卡离场时**立即返回 "
        "`NoTarget`/`Timeout`，而不是报「没有公开密钥能打开」**（有测试钉死） | 扫描循环的**屏与"
        "进度回显**；嵌套/硬嵌套攻击（当前**只做诚实报告，不做攻击**，需单独决策；且 PN532 "
        "根本不提供 nonce） | R2 | **P0**（仅剩 UI） |",
        1,
    ),
    # 10. W1.
    (
        "| **W1** | **把 13 个新模型注册进 `FeatureCatalog`** | `NdefCodec` / `CardDumpFormat` / "
        "`IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / "
        "`ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / "
        "`MifareClassicModel` / `KeyCheckModel` / `GattNameTable` **目前全是孤岛**：无 `FeatureId`、"
        "无生命周期工厂、无菜单入口。不接线则一切「已实现」停在测试里 | **P0** |",
        "| **W1** | **把 13 个模型 + 5 组驱动注册进 `FeatureCatalog`** | 模型：`NdefCodec` / "
        "`CardDumpFormat` / `IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / "
        "`ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / "
        "`MifareClassicModel` / `KeyCheckModel` / `GattNameTable`。驱动：`IRfidTransport` + "
        "`RfidTransport` / `Pn532Protocol` / `Pn532Transport` + `WirePn532Link` / "
        "`Iso14443aFraming` / `MifareClassicDriver` / `St25r3916Transport`。**无 `FeatureId`、"
        "无生命周期工厂、无菜单入口**。不接线则一切「已实现」停在测试里 | **P0** |",
        1,
    ),
    # 11. New W6 for the duplicate PN532 driver.
    (
        "| **W5** | CI 构建矩阵 + 格式检查 + 固件尺寸预算 | 目前仓库无 CI | P1 |",
        "| **W5** | CI 构建矩阵 + 格式检查 + 固件尺寸预算 | 目前仓库无 CI | P1 |\n"
        "| **W6** | 收并旧的 `Pn532I2cDriver` | `src/drivers/Pn532I2cDriver.cpp` 目前只有 "
        "`probe()`；新的 `Pn532Transport` 是更完整的实现，两者并存等于同一条 I2C 总线上两套"
        "驱动。需决定：让旧驱动委托新传输，或删除旧驱动 | P1 |",
        1,
    ),
    # 12. Evidence-gate table.
    (
        "| MIFARE 魔术卡后门 | `mifare-classic.md` §4 | `UNSUPPORTED` | 提供可复现型号 + 流程 |",
        "| MIFARE 魔术卡后门 | `mifare-classic.md` §4 | `UNSUPPORTED` | 提供可复现型号 + 流程 |\n"
        "| PN532 / ISO14443-A 字节层 | `pn532.md` | `VERIFIED`（帧、CRC_A、状态码、卡型映射，"
        "宿主侧编解码互证） | — |\n"
        "| PN532 认证帧追加 UID | `pn532.md` §4 | `PARTIALLY VERIFIED`（刻意偏离数据手册） | "
        "实机抓一次 `InDataExchange` 认证帧确认字节形态 |\n"
        "| ST25R3916 寄存器层 | `st25r3916.md` | `RESERVED`（刻意未实现） | 硬件 + 原理图，"
        "见该文档 §2 |",
        1,
    ),
    # 13. Execution order.
    (
        """5. **R1 → R3 → R4/R5/R7** ST25R3916 / PN532 驱动链（RFID P0 的共同前置）
6. **R2** 七屏 UI 重构""",
        """5. ~~R3 / R4(驱动) / R5(驱动) / R7(驱动)~~ **已完成** —— PN532 通道，4 套宿主测试
6. **R2** 七屏 UI 重构（驱动前置已就位，可直接开工）
7. **R8** NTAG 写入侧（PN532 路径；`InDataExchange` 与 `NdefCodec` 均已具备）
8. **R10** dump → SD 落盘 + Loot 查看器（打通 B1）
9. **R9** ISO-DEP / Type 4（只依赖 `IRfidTransport`，可先于 R1 开工）
10. **R1** ST25R3916 寄存器驱动（需硬件；只有 iCLASS 与 nonce 捕获非它不可）""",
        1,
    ),
]


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    for index, (old, new, expected) in enumerate(EDITS, start=1):
        found = text.count(old)
        if found != expected:
            print(f"edit {index}: expected {expected} match(es), found {found}", file=sys.stderr)
            return 1
        text = text.replace(old, new, expected)
    TARGET.write_text(text, encoding="utf-8")
    print(f"patched {TARGET.relative_to(ROOT)}: {len(EDITS)} edit(s) applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
