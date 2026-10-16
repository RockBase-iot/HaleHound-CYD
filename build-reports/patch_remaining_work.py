#!/usr/bin/env python3
"""Refresh REMAINING_WORK.md after closing B1-B3 and the host half of B5.

Every replacement is asserted to occur exactly once, so a drifted row is a hard
failure rather than a silently half-updated checklist.

Usage:
    python build-reports/patch_remaining_work.py <passed>/<total>
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
TARGET = ROOT / "REMAINING_WORK.md"

SNAPSHOT_TIME = "2026-10-16 11:05 GMT+8"


def build_edits(suites: str) -> list[tuple[str, str]]:
    return [
        # ── header ────────────────────────────────────────────────────────
        (
            "| 快照时间 | 2026-10-15 16:45 GMT+8 |",
            f"| 快照时间 | {SNAPSHOT_TIME} |",
        ),
        (
            "| 快照依据 | 工作树全量宿主回归 `40/40` 套件通过（`ziglang` clang 21，76 个共享对象）；"
            "7 个新 TU 经真实 C5 工具链 `-fsyntax-only` 通过 |",
            f"| 快照依据 | 工作树全量宿主回归 `{suites}` 套件通过（`ziglang` clang 21）；"
            "7 个新 TU 经真实 C5 工具链 `-fsyntax-only` 通过 |",
        ),
        # ── one-minute conclusion ────────────────────────────────────────
        (
            "- **下一步最高 ROI**：① 新模型/驱动进 catalog（低成本、立刻可用）→ ② `CardDumpFormat` 落 SD\n"
            "  （打通 B1 回归）→ ③ RFID 七屏 UI（R2 —— 驱动前置已经就位，可以直接开工）。",
            "- **BLE 传输层（B1–B3）本轮已补齐**：`SkeletonKeyClient` + `GattTreeView`、\n"
            "  `ChameleonFrame`（逐字节增量接收器）+ `ChameleonUltraClient`、`HidReaderClient`，\n"
            "  各配一套宿主测试；协议数字逐条对过固件源码，记入 `docs/protocol-evidence/chameleon-ultra.md`。\n"
            "  B5 的**宿主侧收口**也已就位（`BleVisitAudit`），只剩真机采样（`X2`）。\n"
            "- **下一步最高 ROI**：① 新模型/驱动进 catalog（低成本、立刻可用）→ ② `CardDumpFormat` 落 SD\n"
            "  （打通 B1 回归）→ ③ RFID 七屏 UI（R2 —— 驱动前置已经就位，可以直接开工）。",
        ),
        # ── X2 ───────────────────────────────────────────────────────────
        (
            "| **X2** | 两台实机上机跑 B1–B4 | `HeapBaseline` / `FeatureSwitchLedger` 只有模型自测数据，"
            "没有真实采样 | 提供 nm-cyd-c5 + esp32-cyd 实机各一台 + SD 卡 + 拨码可切换 |",
            "| **X2** | 两台实机上机跑 B1–B4 | `HeapBaseline` / `FeatureSwitchLedger` 只有模型自测数据，"
            "没有真实采样。**宿主侧桥接已就位**（`BleVisitAudit`：把 `IBleCentral` 的实时资源读数与 heap "
            "采样送进两个模型），缺的只是板子上的读数 | 提供 nm-cyd-c5 + esp32-cyd 实机各一台 + SD 卡 + 拨码可切换 |",
        ),
        # ── B1-B5 ────────────────────────────────────────────────────────
        (
            "| **B1** | SkeletonKey BLE 传输层 | `SkeletonKeyModel` + `GattNameTable`（165 项，已对齐 "
            "Bluetooth SIG 机读清单）就位（12 例测试） | BLE client：连接/服务发现/特征读写/notify 订阅 + "
            "GATT 树→屏渲染 | BLE client 实现 | P1 |",
            "| **B1** | SkeletonKey BLE 传输层 | ✅ **client 已就位**：`SkeletonKeyClient`（连接/发现/读写/"
            "notify）+ `GattTreeView`（扁平行列表、展开折叠、分页且光标跟随页）；`SkeletonKeyModel` + "
            "`GattNameTable`（165 项，对齐 SIG 机读清单）在其下 | 屏接线（`FeatureCatalog` 注册，见 `W1`）+ "
            "真机上机（`X2`） | W1 + X2 | P1 |",
        ),
        (
            "| **B2** | Chameleon Ultra BLE 传输 | `ChameleonUltraModel` 就位（10 例测试，含 `N of 16` "
            "诚实报告） | BLE 连接 + 8 槽读写 + 自动连接策略 + 设备地址解析 | BLE client | P1 |",
            "| **B2** | Chameleon Ultra BLE 传输 | ✅ **client 已就位**：`ChameleonFrame`（帧编解码 + "
            "逐字节增量接收器）+ `ChameleonUltraClient`（8 槽读写、自动连接、设备地址解析、昵称、"
            "启用槽）；`ChameleonUltraModel` 在其下。协议帧/命令号/槽类型/NUS UUID 全部逐条对过固件源码，"
            "记入 `docs/protocol-evidence/chameleon-ultra.md` | STATUS 码表（只知 `0x0000`）+ 真机上机（`X2`） | "
            "X2 | P1 |",
        ),
        (
            "| **B3** | HID reader tooling | `HidReaderModel` 就位（7 例测试） | 被动扫描 + 连接诊断；"
            "**UUID 映射先补证据**（`X3`） | X3 + BLE client | P1 |",
            "| **B3** | HID reader tooling | ✅ **client 已就位**：`HidReaderClient`（被动扫描 + 连接诊断 + "
            "分连接批次的扫描 + 诚实事务报告）；写目标按 GATT 属性位选，不假设回复特征。UUID 映射仍 "
            "`mappingVerified=false`，`0xFFF0` 另标 `uuidCollidesWithAnotherRole`（SIG 清单里它是 "
            "Triones/ELK-BLEDOM LED 服务） | `X3` 证据 + 屏接线；opcode 由调用方提供（本仓库不发明协议） | "
            "X3 + W1 | P1 |",
        ),
        (
            "| **B5** | BLE 内存/白屏收口 | `FeatureSwitchLedger` + `test_ble_switch_ledger` 全绿；"
            "`replay()` 对 120 种排列返回 `Clean` | **真机** B2 用例 + heap 采样 | `X2` | **P0** |",
            "| **B5** | BLE 内存/白屏收口 | ✅ **宿主侧已收口**：`BleVisitAudit` 把 `IBleCentral` 的实时"
            "资源读数（running/connected/scanning/subscriptions + begin/end 配对）桥进 "
            "`FeatureProfile.resourcesHeldAfterEnd`，`profileFromMeasurement()` 让「声明的 0」被实测取代，"
            "`assessBleVisit()` 复用 `HeapBaselineCheck` 的口径；三个 client 的 `end()` 是否真的交还控制器/订阅"
            "由测试实测。`FeatureSwitchLedger` + `test_ble_switch_ledger` 全绿，`replay()` 对 120 种排列 "
            "`Clean` | **真机** B2 用例 + heap 采样（`IBleHeapProbe` 的设备实现只差一个 `ESP.getFreeHeap()`） | "
            "`X2` | **P0** |",
        ),
    ]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: patch_remaining_work.py <passed>/<total>", file=sys.stderr)
        return 2
    suites = sys.argv[1]

    text = TARGET.read_text(encoding="utf-8")
    failures: list[str] = []
    for index, (old, new) in enumerate(build_edits(suites)):
        count = text.count(old)
        if count != 1:
            failures.append(f"edit {index}: expected 1 occurrence, found {count} :: {old[:110]}")
            continue
        text = text.replace(old, new)

    if failures:
        for failure in failures:
            print("MISS:", failure, file=sys.stderr)
        return 1

    TARGET.write_text(text, encoding="utf-8")
    print(f"patched {TARGET.relative_to(ROOT)} ({len(build_edits(suites))} edits, suites={suites})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
