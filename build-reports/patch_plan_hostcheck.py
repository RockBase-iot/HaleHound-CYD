#!/usr/bin/env python3
"""Record the host-test breakthrough in DEVELOPMENT_PLAN.md.

Three edits:
  §5.4  the "no host C++ compiler" boundary is closed -> replace with the
        remaining boundary + a new §5.5 listing the nine latent defects the
        first host run exposed.
  §7    add the host runner as a second verifier, with the two traps that cost
        real time (unloadable mingw, force-linking vs archiving).
  §9    mark tasks 4 (half) and 5 done with the measured numbers.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "DEVELOPMENT_PLAN.md"

OLD_54 = """### 5.4 当前无法执行的验证（已知边界）

| 项 | 原因 | 影响 |
|---|---|---|
| `pio run` 真实链接 | 沙箱守卫「每轮 50 文件删除预算」被 pio 包管理器撞穿 | 无法产出 `.bin`；用真实 env 的 `-fsyntax-only` 代替（不覆盖链接与 `extra_scripts`） |
| 30 套 native 套件的**断言执行** | 本机无可用宿主 C++ 编译器 | 目前只能证明「能编、能链接、符号齐全」，断言是否通过未知 |

这两项都必须在进入阶段 9 之前闭合，否则「已实现」永远停在 `Build Verified` 之前。
"""

NEW_54 = """### 5.4 当前无法执行的验证（已知边界）

| 项 | 原因 | 影响 |
|---|---|---|
| `pio run` 真实链接 | 沙箱守卫「每轮 50 文件删除预算」被 pio 包管理器撞穿 | 无法产出 `.bin`；用真实 env 的 `-fsyntax-only` 代替（不覆盖链接与 `extra_scripts`） |
| 上机执行 B1–B4 | 需要两台实机 + SD 卡 + 模块顺序切换 | `HeapBaseline` 目前只有模型自测数据，没有真实采样 |

**已闭合项**：宿主断言执行。`build-reports/hostenv` 内装了 `ziglang`（pip wheel，自带
clang 21 + libc++ sysroot），`scripts/hostcheck/run_host_tests.py` 自动探测它并编译 +
链接 + **执行**全部 native 套件。2026-10-15 实测：**29/29 套件、142 个用例全绿**。

剩余边界仍必须在进入阶段 9 之前闭合，否则「已实现」永远停在 `Build Verified` 之前。

### 5.5 首次宿主跑测暴露的潜伏缺陷（2026-10-15 已全部修复）

这些套件此前从未被编译执行过一次（仓库没有可用宿主编译器），首跑 20/29，9 个失败
全部是真实问题，不是环境噪声：

| 套件 | 症状 | 判定 | 处置 |
|---|---|---|---|
| `test_board_profiles` 等 4 套 | `TEST_ASSERT_EQUAL_UINT32` 未定义 | shim 能力缺口 | `scripts/hostcheck/unity/unity.h` 补齐类型化相等宏 |
| `test_spi_service` | `undefined symbol: WinMain` | **测试文件缺陷**：`main()` 被写在匿名 namespace 内，全局 `main` 符号根本不存在 | 在 `main` 前闭合 namespace，删掉多余的尾部 `}` |
| `test_feature_registry` | `incomplete type 'FeatureRegistry'` | **测试文件缺陷**：`FeatureCatalog.h` 只做前置声明 | 补 `#include "core/FeatureRegistry.h"` |
| `test_ble_switch_ledger` | `replay(nullptr, 0)` 被判定 `InvalidProfile` | **实现缺陷**：空访问序列应满足空真 | `count == 0` 直接返回 `Clean` |
| `test_card_dump_format` | `.nfc` 往返丢块；`uid[0]`、`unknownKeys` 均不符 | **实现缺陷 ×2 + 夹具缺陷 ×1** | 见下表 |
| `test_ndef_codec` | 文本记录负载 400 vs 403 | **断言缺陷**：`buildText` 会前置状态字节 + 语言码 | 改为断言往返字节相等，并显式断言 > 255 以锁定四字节长度形式 |

`CardDumpFormat` 的两个真实缺陷：

1. `toFlipperNfc` 写出的未读块占位 `Block n: ?? ?? … ??` **无法被自己的解析器读回** ——
   该行落入 `unknownKeys`，块槽位丢失、后续块号全部前移，等于静默损坏转储。
   已让 `parseBlockLine` 识别 `??` 占位并保留槽位（`known = false`）。
2. 解析器不认 `Version:` 与 `Data format version:` —— 这两个是它**自己写出来的**已知键，
   被当作 unknown 保留后，每次「存 → 读 → 存」都会在文件里重复累积。已显式消费。

夹具缺陷：`makeClassicDump()` 的 block 0 是 `0..15` 递增填充，却断言
`fromProxmarkEml` 能从中推出 UID `04 12 34 56`。MIFARE Classic 的块 0 是厂商块
（UID 字节 0..3 + BCC + SAK + ATQA），**实现是对的**，已把夹具改成真实厂商块布局。
"""

OLD_7 = """**判定规则**：改动文件与同环境的**未改动文件**报同样的错 → 判定为工具噪声，不作为回归。
"""

NEW_7 = """**次级校验器**：`scripts/hostcheck/run_host_tests.py` + `build-reports/hostenv`

- 用仓库本地 `ziglang` venv 驱动 `python -m ziglang c++`，行为等同 `clang++`。
  安装与调用方式见 `build-reports/README.md`。
- **先探针再采纳**：`~/.platformio` 里的 mingw32 包缺 32 位 `libgmp-10.dll` 等，
  `g++ --version` 正常但每次编译都以**完全空的日志**失败。因此每个候选编译器都要真的
  编译 + 链接 + 运行 `int main(){return 0;}` 才能被选中，否则静默降级到 zig。
- **共享源先归档再链接**：直接强制链接所有 `.o` 会因为 `src/core/AppKernel.cpp` 引用
  `build_src_filter` 排除掉的 `legacySetup` / `legacyLoop` 而报未定义符号。打成
  `libhostcheck.a` 才复刻 PlatformIO `native` 环境「只拉取被引用目标文件」的语义。
  **不要用空桩函数去「修」这个报错** —— 那会掩盖固件侧真实的链接错误。
- 输出每套件 `PASS/FAIL`，失败时给出断言的文件名、行号与期望/实际值。

**判定规则**：改动文件与同环境的**未改动文件**报同样的错 → 判定为工具噪声，不作为回归。
"""

OLD_9 = """| 4 | **闭合 §5.4 两项验证边界**（宿主编译器 + 沙箱外 `pio run`） | 不闭合则一切「已实现」都停在 `Build Verified` 之前 |
| 5 | **主机跑测**：`scripts/hostcheck/run_host_tests.py` 跑通 30 套套件 | 已有脚本，等第 4 项 |
"""

NEW_9 = """| 4 | 🟡 **闭合 §5.4 验证边界**：宿主编译器 ✅ 已闭合；沙箱外 `pio run` 待办 | 不闭合则一切「已实现」都停在 `Build Verified` 之前 |
| 5 | ✅ **主机跑测**：29/29 套件、142 个用例全绿（首次执行；顺带修掉 9 个此前从未被编译过的潜伏失败，见 §5.5） | 断言级验证已可用 |
"""

EDITS = [
    (OLD_54, NEW_54, "§5.4 + new §5.5"),
    (OLD_7, NEW_7, "§7 host runner as second verifier"),
    (OLD_9, NEW_9, "§9 tasks 4-5 status"),
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
