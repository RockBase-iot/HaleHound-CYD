#!/usr/bin/env python3
"""Refresh three dependency cells in REMAINING_WORK.md that the new driver layer
made stale:

  * R2 (七屏 UI) depended on R1 for capability reporting; the capability
    reporting now exists via IRfidTransport::caps(), so its real blocker is W1.
  * R8 (NTAG 写入侧) depended on "PN532 驱动", which now exists.
  * R10 (dump → Loot) depended on R4, whose driver layer is done.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "REMAINING_WORK.md"

EDITS: list[tuple[str, str, int]] = [
    (
        "| 七屏渲染 + 导航接线；按能力矩阵显隐屏项 | R1（能力上报） | **P0** |",
        "| 七屏渲染 + 导航接线；按能力矩阵显隐屏项 | W1（注册） | **P0** |",
        1,
    ),
    (
        "| 擦除 / 永久只读 / 设密码 / 清密码的 **PN532 写入路径** + 危险操作确认 | PN532 驱动 | P1 |",
        "| 擦除 / 永久只读 / 设密码 / 清密码的 **PN532 写入路径** + 危险操作确认 | "
        "PN532 驱动（本轮已就位） | P1 |",
        1,
    ),
    (
        "| `StorageService` 落盘 + Loot 文本查看器（分页/滚动）+ 空目录 B1 回归 | R4 + `X2` | P1 |",
        "| `StorageService` 落盘 + Loot 文本查看器（分页/滚动）+ 空目录 B1 回归 | "
        "R4（驱动层已就位）+ `X2` | P1 |",
        1,
    ),
    (
        "| `IclassPacs` 解码就位（26/35/48-bit + Corporate 1000） | 射频层 + 3DES 密钥路径 | R1 + `X5` | P1 |",
        "| `IclassPacs` 解码就位（26/35/48-bit + Corporate 1000） | 射频层 + 3DES 密钥路径。"
        "**PN532 硅片做不到 iCLASS，只有 ST25R3916 能承载** | R1 + `X5` | P1 |",
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
