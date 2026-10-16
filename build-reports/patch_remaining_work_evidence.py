#!/usr/bin/env python3
"""Fold the external-evidence review into REMAINING_WORK.md.

What this adds, and why each one changes a decision rather than restating one:

  * a new section 0.8 -- the four facts from the field review that move
    priorities (upstream relicensed to proprietary; this fork's own LICENSE is
    the MIT-era text; two mutually incompatible GPL libraries are linked in;
    upstream's PN532 is SPI while this fork's stack is I2C-only);
  * four new blockers X6-X9 in section 1;
  * sections 8 and 9 (compliance/distribution, hardware/shared-bus) which did
    not exist at all before;
  * section 10 replacing the old section 8, whose list was mis-numbered
    (7,8,9 appeared twice) -- the ordering is re-derived, not re-typed.

The old "## 8. 建议执行顺序" is cut off at its header and rewritten, because that
block had two items numbered 7, two numbered 8 and two numbered 9.

CRLF is preserved (this file is CRLF); every replacement asserts a unique match,
and the script aborts without writing if any anchor drifted.
"""
from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
TARGET = ROOT / "REMAINING_WORK.md"

applied: list[str] = []
text: str = ""
sep = "\n"


def replace_once(name: str, old: str, new: str) -> None:
    global text
    old_s = old.replace("\n", sep)
    new_s = new.replace("\n", sep)
    found = text.count(old_s)
    if found != 1:
        sys.exit(f"{name}: expected 1 match, found {found}. Nothing written.")
    text = text.replace(old_s, new_s, 1)
    applied.append(name)


SECTION_08 = """## 0.8 外部证据复核（2026-10-16）：说明书 / 许可 / 社区反馈

> 完整取证见 **`docs/upstream-analysis/USER_EVIDENCE_zh.md`**（来源分级 S1/S2/S3、逐条引用、复现命令）。
> 本节只列**改变了本清单判断**的部分，不重复取证内容。

| # | 事实 | 证据（级） | 对清单的影响 |
|---|---|---|---|
| **E1** | 上游 v4.0 的许可是**专有**文本：禁止反向工程、**由二进制衍生**、再分发、商业分发、镜像、**改动 branding/attribution 声明**、**声称作者**；并且**源码与 `.bin` 都不存在**（分发明面只有 `flash.halehound.com`） | 上游 `LICENSE` 全文（S1） | 新增 §8 合规轨道；本 fork「按 release notes 独立实现」的路线得到正面确认，但**对外物料出现硬约束** |
| **E2** | **本 fork 自己的 `LICENSE` 是 MIT**（双著作权：`2026 Jesse C. Hale (HaleHound-CYD Edition)` + `2023 CiferTech (original ESP32-DIV)`）——即上游 v3.4.0 时代的文本 | 本仓 `LICENSE`（S1） | MIT 授权对已获得的版本不可撤销，现状合法；但**不能把 v4.0 的新材料当 MIT 用** |
| **E3** | 本 fork 链接 **`RF24@1.4.11` = GPL-2.0-only** 与 **`arduinoFFT@2.0.4` = GPL-3.0** —— 两者**互不相容**，且都不能进入专有分发 | 包内 `library.json` / `LICENSE`（S1，本地可复跑） | **在当前依赖集下固件二进制不可分发** → `C1`。上游正是靠自写 `nrf24_raw.h` / `halehound_fft.h` 绕开同一问题，并在 LICENSE 里当作卖点 |
| **E4** | 上游 **PN532 走 SPI**（`SS=GPIO17`，DIP 设 SPI；社区手册同表：MOSI/MISO/SCK = IO23/19/18），而本 fork 的 RFID 栈**只有 I2C**（`WirePn532Link`：`Wire.begin` + 100 kHz + ready byte `0x01`，PN532 相关文件 SPI 计数为 0） | 上游 README + 社区手册 + 本仓源码（S1） | `H2` 从「待决策」升级为**接线分歧**；与 `U4`（孤儿 `Pn532I2cDriver`）合并成**一次**决策 |
| **E5** | 上游讨论最热的故障是**共享 VSPI 下 SD 卡识别失败**（#165 九条评论 / #168 七条）与 **CC1101 冻结**（#172，**十四条，最多**）；另有 PSRAM 初始化失败 + Tools/Skimmer 冻结且**串口无输出**（#183）、开机 brownout 重启循环（#182） | 上游 issue 列表（S1） | 新增 `H1`（总线仲裁，**P0**）；`F1/F2/F3` 优先级上调；`F7` 的「低内存优雅退出」需要把「无日志」也算作失败模式 |
| **E6** | 本 fork 的蓝牙菜单**已经有** `Back to Main Menu`（上游 #187 恰好缺这个），但行数 10→15 使竖屏行高 25→16 px | 本仓 `LegacyApp.inc:441` + `§0.7` 实测 | `D7` 分页从「体验优化」升为 **P1**：**这是本 fork 相对上游唯一变差的地方** |
| **E7** | `A10` 里那 16 条空壳中的 **14 条**是上游菜单里**真实存在**的 Settings(9) + Tools(5) | 上游 README 菜单树（S1） | `U5` 的口径改为「补上游已有功能」，不是「清理多余条目」 |

**一句结论**：这轮外部证据**没有推翻任何已完成的工作**，但新增了一条会决定「能不能发货」的轨道（合规），并把三件原本排在后面的系统性问题（共享总线、PN532 总线、菜单分页）提到 P0/P1。

---

"""

ROWS_X = """| **X6** | 依赖许可合规（`C1`） | **`RF24` 是 GPL-2.0-only，`arduinoFFT` 是 GPL-3.0，二者互不相容**；本 fork 的 MIT 声明覆盖不了这个组合 ⟹ **分发二进制在许可上不成立** | 三选一：① 用自研/permissive 实现替换这两个库（照上游 clean-room 做法）② 接受整个固件按 GPL 分发并履行源码义务 ③ 明确永不分发二进制（仅自用） |
| **X7** | 商标与命名边界（`C3`） | `HaleHound™` 是上游商标，且上游许可禁止「声称作者」；本 fork 若要商品化（NM 系列 / WORKBUDDY 品牌），命名与物料必须划线 | 定稿：哪些位置保留**归属性**引用（说明来源），哪些位置必须改用本产品名 |
| **X8** | PN532 总线决策（`H2`） | 上游/社区标准是 **SPI**，本 fork 只有 **I2C** ⟹ 本 fork 目前驱动不了社区那套接线；同时仓里还有一个孤儿 `Pn532I2cDriver` | 决策：① 本 fork 板另设计（I2C）→ 文档必须显式写出与上游接线的差异 ② 兼容社区接线（SPI）→ 补 SPI 链路（PN532 的 SPI 帧格式与 I2C 不同，不是换个 `Wire` 就行） |
| **X9** | 上游 4 板型 vs 本 fork 2 环境（`H4`） | 上游 4 个构建目标；本 fork 只有 `esp32-cyd` / `nm-cyd-c5`，且 `nm-cyd-c5` **不是上游任何一板**（`ST7789_DRIVER=1`、`PART_16MB=1`、`BOARD_HAS_PSRAM=1`），而上游把 ST7789 归为「另一类 CYD」 | 明确目标板清单：只支持 nm-cyd-c5 + esp32-cyd（其余不做，并在文档写明），或补齐上游板型 |
"""

SECTION_8 = """## 8. 合规与分发（2026-10-16 新增；依据 `USER_EVIDENCE_zh.md` §4 / §5）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **C1** | 依赖许可合规 | `nrf24/RF24@1.4.11`：`library.json` 声明 **`GPL-2.0-only`**；`kosme/arduinoFFT@2.0.4`：LICENSE 正文 **GPL v3**。两者不可组合，且都与专有分发不兼容 | 三选一（同 `X6`）：替换 / 全 GPL / 不发二进制。**替换路线需要给出替代实现与回归证据**（上游的 `nrf24_raw.h`、`halehound_fft.h` 是存在性证明，不是可搬运的代码） | BOSS | **P0** |
| **C2** | 第三方声明与衍生声明 | 仓库**无** `THIRD_PARTY_NOTICES`；README 保留了 `By [JesseCHale]` / `HaleHound-CYD by [JesseCHale]` 与 CiferTech 引脚修正说明，但未按 LICENSE 建议形式声明「衍生关系 + 改了什么」 | ① `THIRD_PARTY_NOTICES.md`：逐依赖 SPDX + 版本 + 出处（含 5 个「包内无 LICENSE 文件」的补证结果）② README 加 `Based on HaleHound-CYD by Jesse C. Hale` + 本 fork 修改摘要 | `C1` | **P0** |
| **C3** | 商标与命名边界 | 对外物料使用 `HaleHound` 字样；`HaleHound™` 为上游商标 | 定稿保留/替换清单（同 `X7`），并落到 README、文档标题、未来的产品页 | BOSS | **P0（商品化前）** |
| **C4** | v4.0 专有物料的搬运边界 | 本仓文档与上游大量同名（功能名、菜单名）；README 保留上游 ASCII 横幅 | 复核边界：功能名与菜单结构属功能性描述可保留；**逐字文案、美术资产、ASCII 横幅**不得进入对外销售物料；`README.txt` / `dist/` 内若有上游原文需标注来源与许可 | `C3` | P0 |
| **C5** | 「无源码可审计」的自我约束 | 上游无源码、无二进制；本次全部上游行为描述均为**发布方自述** | 保持 `RELEASE_BRIEF_zh.md` 的标注口径，不把自述外推为已验证事实；将来若拿到实机行为，回填为 S1 | — | P2 |

---

## 9. 硬件兼容与共享总线（2026-10-16 新增；依据 `USER_EVIDENCE_zh.md` §7）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **H1** | 共享 VSPI **总线仲裁层** | **无**。SD + CC1101 + NRF24 + PN532 全挂 GPIO 18/19/23；上游明确声称「同一时刻只有一个设备说话，SPI manager 自动仲裁」 | 一次只允许一个持有者：CS 独占、传输互斥、SD 与外设切换时的重挂载（对应上游 #165/#168 的 SD 消失）；宿主可测（假总线 + 事件序列断言，禁止「两个 CS 同时低」） | — | **P0** |
| **H2** | PN532 总线决策（SPI vs I2C） | 只有 I2C：`WirePn532Link`（100 kHz、ready byte `0x01`）；SPI 链路不存在。注释自称「NM-RF-HAT 走线较长，400 kHz 会勉强」 | 决策并落文档（同 `X8` / `U4`）：若走 I2C，必须在文档里写出**与上游接线的差异**（否则用户照官方图接线必失败）；若走 SPI，需按 PN532 SPI 帧格式新增一条 `IRfidTransport` 实现 | `U4` | **P0** |
| **H3** | 射频引脚**可重指派** | 引脚全是编译期 `-D` 常量 | 引脚→配置项（SD/EEPROM），让「几乎免焊」的 DIY 构型复用同一固件（上游 #179 的功能请求） | `H1` | P1 |
| **H4** | 闪存 / 分区矩阵 | `nm-cyd-c5` 用 `PART_16MB=1`；上游有「16 MB CYD 装不上」（#167） | 4 MB 与 16 MB 两种分区表都要能出包并过 `T5` 的 `FLASH_LAYOUT`；把 #167 写成回归用例 | `T5` | P1 |
| **H5** | 供电巡检（brownout 对策） | 文档只有「独立 3.3 V」这条要求 | 射频初始化失败时给出**指向供电的可操作提示**（而非静默失败）；把 #182 的 brownout 重启循环纳入回归 | — | P1 |

"""

SECTION_10 = """## 10. 建议执行顺序（2026-10-16 重排）

> 旧版此节有两处编号 7、两处 8、两处 9（清单被追加过但没重排）。下面是按当前依赖关系重推的顺序，
> **不是把旧列表抄一遍**。

0. **先做合规决策，再写码**：`C3`（商标/命名）→ `X6`/`C1`（GPL 三选一）→ `C2`（第三方声明）。
   **理由**：`C1` 的结论决定「是否替换 RF24 / arduinoFFT」，而这两个库牵着 2.4GHz 与频谱仪两块的做法 —— 先写码再换库等于返工。
1. **`U3`** `ChameleonUltra` 补 `FeatureId` + descriptor（P0，最小；菜单已在读 catalog，缺它即等于它不存在）
2. **`D7`** 菜单分页/滚动（P1，但这是**本 fork 相对上游唯一变差的地方**；`MenuLayout` 与 `GattTreeView` 都有现成做法）
3. **`H1`** 共享 VSPI 仲裁层（P0 结构性：后续所有射频项都建在它上面）
4. **`U4` + `H2` + `X8`** PN532 总线决策 + 孤儿驱动处置（一次决策同时关掉三张单）
5. **`W2` + `R10`** `CardDumpFormat` → `StorageService` → Loot 查看器（打通 B1 回归 `T1`）
6. **`X1`** 沙箱外 `pio run` + `.bin`（恢复「能出固件」的能力；`H4` 的分区矩阵随后）
7. **`X2` → `T1–T4`** 上机台架回归
8. **`U5`** 16 条空壳：14 条按上游 Settings/Tools 实做，2 条（`SubGhzSavedProfiles` / `SigintWardriving`）摘掉或补实现
9. **`R2`** RFID 七屏 UI（驱动前置已就位）
10. **`F1/F2/F3`** SubGHz 三件（在 `H1` 保护下才稳；上游 #172/#181 正是缺这层）
11. **`R8`** NTAG 写入侧 → **`R9`** ISO-DEP / Type 4 → **`R1`** ST25R3916（需硬件）
12. 其余 P1（含 `D4` Loot 可删除 / `D5` GPS 冷启动提示 / `H3` 引脚可重指派 / `H4` / `H5`）→ 最后 P2
"""


def main() -> None:
    global text, sep
    raw = TARGET.read_bytes()
    text = raw.decode("utf-8")
    sep = "\r\n" if "\r\n" in text else "\n"

    # 1) snapshot header: the test count was stale (48/48 -> 49/49 after U2), and
    #    the external review is now a second source for this snapshot.
    replace_once(
        "snapshot time",
        "| 快照时间 | 2026-10-16 14:21 GMT+8 |",
        "| 快照时间 | 2026-10-16 16:30 GMT+8 |",
    )
    replace_once(
        "snapshot basis",
        "| 快照依据 | 工作树全量宿主回归 `48/48` 套件通过（`ziglang` clang 21）+ **运行时可达性只读走查**（见 §0.5，10 条硬证据） |",
        "| 快照依据 | 工作树全量宿主回归 **`49/49` 套件 / 446 用例**通过（`ziglang` clang 21）+ **运行时可达性只读走查**（§0.5，10 条硬证据）+ **外部证据复核**（§0.8，`docs/upstream-analysis/USER_EVIDENCE_zh.md`） |",
    )
    replace_once(
        "upstream baseline row",
        "| 上游基线 | v4.0（见 `docs/upstream-analysis/RELEASE_BRIEF_zh.md`） |",
        "| 上游基线 | v4.0（release notes 见 `RELEASE_BRIEF_zh.md`；**v4.0 许可已换为专有，v3.4.0 时代是 MIT**，见 §0.8 / §8） |",
    )

    # 2) the highest-ROI bullet gains the track that decides whether anything can ship.
    replace_once(
        "roi bullet",
        "  ④ `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）。",
        "  ④ `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）。\n"
        "- **外部证据新增了一条会决定「能不能发货」的轨道**：依赖里 `RF24` = GPL-2.0-only、\n"
        "  `arduinoFFT` = GPL-3.0，**两者互不相容**⟹ 当前依赖集下固件二进制不可分发（`C1` / `X6`，见 §0.8 / §8）。",
    )

    # 3) new section 0.8
    replace_once(
        "section 0.8",
        "## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）",
        SECTION_08 + "## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）",
    )

    # 4) four new blockers after X5
    replace_once(
        "blockers X6-X9",
        "| **X5** | 其它密钥表来源 | KeeLoq 厂商密钥、iCLASS Corporate 1000 主密钥表 | 提供来源或明确不做 |",
        "| **X5** | 其它密钥表来源 | KeeLoq 厂商密钥、iCLASS Corporate 1000 主密钥表 | 提供来源或明确不做 |\n"
        + ROWS_X.rstrip("\n"),
    )

    # 5) sections 8/9 inserted before the old section 8, and the old section 8's
    #    mis-numbered list replaced wholesale.
    head, marker, _tail = text.partition("## 8. 建议执行顺序".replace("\n", sep))
    if not marker:
        sys.exit("section 8 header not found; nothing written")
    text = head + (SECTION_8 + SECTION_10).replace("\n", sep)
    applied.append("sections 8/9 + rewritten 10")

    TARGET.write_text(text, encoding="utf-8", newline="")

    # verify what we claim: unique numbering in the rewritten order, and no
    # duplicate "## 8."/"## 9." headers left behind.
    out = TARGET.read_text(encoding="utf-8", newline="")
    for n in ("## 8.", "## 9.", "## 10."):
        if out.count(n) != 1:
            sys.exit(f"verify failed: {n} appears {out.count(n)} times")
    steps = [l for l in out.splitlines() if l[:2].strip().isdigit() and ". **" in l]
    nums = [l.split(".")[0].strip() for l in steps]
    if len(set(nums)) != len(nums):
        sys.exit(f"verify failed: duplicate step numbers {nums}")

    print(f"REMAINING_WORK.md lines now: {len(out.splitlines())}")
    print(f"ordered steps: {nums}")
    for name in applied:
        print(f"  applied: {name}")


if __name__ == "__main__":
    main()
