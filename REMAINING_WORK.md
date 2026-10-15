# 剩余开发工作清单（Remaining Work）

| 项 | 值 |
|---|---|
| 快照时间 | 2026-10-15 16:45 GMT+8 |
| 分支 | `nm-cyd-c5` |
| 快照依据 | 工作树全量宿主回归 `36/36` 套件通过（`ziglang` clang 21，69 个共享对象） |
| 上游基线 | v4.0（见 `docs/upstream-analysis/RELEASE_BRIEF_zh.md`） |
| 主计划 | `DEVELOPMENT_PLAN.md` 阶段 5–9 |

> **本清单只记「还没做」的事。** 已完成的模型层与测试覆盖见 `progress.md`；
> 差距分级（P0/P1/P2/D）口径见 `RELEASE_BRIEF_zh.md` §5。

---

## 0. 一分钟结论

- **可验证的逻辑层已经基本铺满**：RFID / BLE 的 13 个纯逻辑模型 + 36 套宿主测试全绿。
- **但全部是孤岛**：没有任何一个模型注册进 `FeatureCatalog`，也没有一条真实传输/驱动路径。
  当前状态是 `Model Verified`，离 `Build Verified` 与 `Hardware Verified` 都还差一层。
- **下一步最高 ROI 的不是再加模型，而是补「驱动层」和「接线」**：
  ① 让新模型进 catalog（低成本、立刻可用）→ ② `CardDumpFormat` 落 SD（打通 B1 回归）
  → ③ ST25R3916 / PN532 驱动（RFID 全部 P0 项的共同前置）。

---

## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）

| ID | 事项 | 为什么阻塞 | 需要 BOSS 做什么 |
|---|---|---|---|
| **X1** | 沙箱外跑一次 `pio run` | 本沙箱守卫「每轮 50 文件删除预算」被 PlatformIO 包管理器结构性撞穿，`pio run` 在产出 `.bin` 前必失败（见 `DEVELOPMENT_PLAN.md` §5.4） | 在沙箱外执行 `pio run -e nm-cyd-c5 -e esp32-cyd`，或批准一次性提升删除预算 |
| **X2** | 两台实机上机跑 B1–B4 | `HeapBaseline` / `FeatureSwitchLedger` 只有模型自测数据，没有真实采样 | 提供 nm-cyd-c5 + esp32-cyd 实机各一台 + SD 卡 + 拨码可切换 |
| **X3** | HID reader 服务 UUID 证据 | `0xFD40`(Signo) / `0xFFF0`(iCLASS SE) 映射无权威来源；`0xFFF0` 在 SIG 清单里是 Triones/ELK-BLEDOM 厂商服务，跨语境撞号 | 提供设备实测广告包、或厂商文档、或明确接受「凭经验映射」并注明来源等级 |
| **X4** | 魔术卡后门证据 | 写 UID 后门命令未公开验证，已按证据门标 `UNSUPPORTED`（`docs/protocol-evidence/mifare-classic.md` §4） | 提供可复现的魔术卡型号 + 后门流程，否则保持 `UNSUPPORTED` |
| **X5** | 其它密钥表来源 | KeeLoq 厂商密钥、iCLASS Corporate 1000 主密钥表 | 提供来源或明确不做 |

---

## 2. RFID 主战场（`DEVELOPMENT_PLAN.md` 阶段 7）

上游 v4.0 的绝对重心。模型层已完成，缺的全是驱动与 UI。

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **R1** | ST25R3916 SPI 驱动 | 无（仅 `RfidBackendModel` 里的能力枚举） | 初始化/场开断、FIFO 收发、IRQ 处理、ISO14443-A/B 帧编解码、透明模式、板级 CS/IRQ/电源引脚定义 | 硬件 + 数据手册 | **P0** |
| **R2** | RFID 七屏 UI 重构 | `RfidBackendModel` 有屏幕枚举、可用性矩阵、`screenLabel()` 已修重名 | 七屏渲染 + 导航接线；按能力矩阵显隐屏项 | R1（能力上报） | **P0** |
| **R3** | ISO14443-A 认证链 | `MifareClassicModel` + `MifareKeys` 就位（17 例测试） | REQA/WUPA → 防冲突 → SELECT → 认证 `0x60/0x61` → 读/写块；重试与超时 | R1 或 PN532 通道 | **P0** |
| **R4** | MIFARE Classic 全卡转储 | 转储装配、权限解码、诚实计数（`N of M`）已建模 | 扇区遍历顺序（1K/4K/Mini）+ 失败块占位 + 落盘 | R3 | **P0** |
| **R5** | 密钥写入 | 尾块构造、访问位打包、永久锁死检测已建模 | 驱动写尾块 + 写前权限校验 + 永久报废二次确认 UI | R3 | **P0** |
| **R6** | 魔术卡写入 | `magicWriteAllowed(backdoor, confirmed)` 门禁就位 | 后门检测证据（`X4`）+ 驱动写入 | X4 | P1 |
| **R7** | Key Check（29 公开密钥扫描） | `KeyCheckModel` + 29 密钥表就位（12 例测试），诚实进度上报 | 驱动扫描循环 + 进度回显；嵌套/硬嵌套攻击（当前**只做诚实报告，不做攻击**，需单独决策） | R3 | **P0** |
| **R8** | NTAG Tools 写入侧 | `NtagModel`（容量/锁位/密码）+ `NdefCodec` 就位 | 擦除 / 永久只读 / 设密码 / 清密码的 **PN532 写入路径** + 危险操作确认 | PN532 驱动 | P1 |
| **R9** | ISO-DEP / Type 4 全链 | 无 | I-Block 分块与 CRC、RATS/ATS、APDU 封装、NDEF TLV 映射（TLV 层可复用 `NdefCodec`） | R1 | P1 |
| **R10** | RFID dump → Loot + 机内查看 | `CardDumpFormat` 双向（`.nfc`/`.eml`，含 `??` 占位）就位 | `StorageService` 落盘 + Loot 文本查看器（分页/滚动）+ 空目录 B1 回归 | R4 + `X2` | P1 |
| **R11** | iCLASS 射频 + 3DES | `IclassPacs` 解码就位（26/35/48-bit + Corporate 1000） | 射频层 + 3DES 密钥路径 | R1 + `X5` | P1 |

---

## 3. BLE 收口（`DEVELOPMENT_PLAN.md` 阶段 6）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **B1** | SkeletonKey BLE 传输层 | `SkeletonKeyModel` + `GattNameTable`（165 项，已对齐 Bluetooth SIG 机读清单）就位（12 例测试） | BLE client：连接/服务发现/特征读写/notify 订阅 + GATT 树→屏渲染 | BLE client 实现 | P1 |
| **B2** | Chameleon Ultra BLE 传输 | `ChameleonUltraModel` 就位（10 例测试，含 `N of 16` 诚实报告） | BLE 连接 + 8 槽读写 + 自动连接策略 + 设备地址解析 | BLE client | P1 |
| **B3** | HID reader tooling | `HidReaderModel` 就位（7 例测试） | 被动扫描 + 连接诊断；**UUID 映射先补证据**（`X3`） | X3 + BLE client | P1 |
| **B4** | BLE 名称库单一化 | `GattNameTable`（SIG 原文，可宿主测试）与 `include/ble_database.h`（Nordic 来源，名称截断到 14 字符，依赖 `Arduino.h`，只被 `src/legacy/bluetooth_attacks.cpp` 用）**并存** | 决定是否让 legacy 表委托新表（独立迁移，未盲改） | **BOSS 决策** | P1 |
| **B5** | BLE 内存/白屏收口 | `FeatureSwitchLedger` + `test_ble_switch_ledger` 全绿；`replay()` 对 120 种排列返回 `Clean` | **真机** B2 用例 + heap 采样 | `X2` | **P0** |

---

## 4. 台架回归（`DEVELOPMENT_PLAN.md` 阶段 6 / §5.3）

| ID | 用例 | 现状 | 通过标准 |
|---|---|---|---|
| **T1** | B1 射频后 Loot 读卡 | 模型就位 | 读到卡内既有文件，**不得空目录**；出现 `SdCode::Remounted`/`Ready`，不得 `Busy`/`NotMounted` |
| **T2** | B2 BLE 任意顺序切换 | `replay()` 已可离线验证 120 种排列 | 屏幕始终有内容（无白屏/无残留帧）；heap 不持续下降 |
| **T3** | B3 heap 基线 | `HeapBaseline` + `test_heap_baseline` 就位 | 全部 `WithinBudget`；回退 ≥8% 记 warn，≥15% 或 free <24 KiB 记 critical |
| **T4** | B4 连续运行 30 分钟 | 无 | 无看门狗复位，关键循环无持续堆下降 |
| **T5** | 沙箱外产出 `.bin` + SHA256 + 刷写清单 | `scripts/package_firmware.ps1` 就位 | `BUILD_FRESHNESS` / `FLASH_LAYOUT` / `PACKAGE_FIRMWARE` 三项 PASS | 

---

## 5. 接线与注册（低成本、高杠杆，建议最先做）

| ID | 事项 | 说明 | 级别 |
|---|---|---|---|
| **W1** | **把 13 个新模型注册进 `FeatureCatalog`** | `NdefCodec` / `CardDumpFormat` / `IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / `ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / `MifareClassicModel` / `KeyCheckModel` / `GattNameTable` **目前全是孤岛**：无 `FeatureId`、无生命周期工厂、无菜单入口。不接线则一切「已实现」停在测试里 | **P0** |
| **W2** | `CardDumpFormat` 接到 `StorageService` | 打通 `R10` 与 `T1`（B1 回归） | P0 |
| **W3** | `LegacyApp.inc` 分域切分（Tools / Settings 优先） | 降低后续每项改动的回归面 | P1 |
| **W4** | 版本号单一来源 | 同步固件、菜单、构建产物、README | P1 |
| **W5** | CI 构建矩阵 + 格式检查 + 固件尺寸预算 | 目前仓库无 CI | P1 |

---

## 6. 其余 P1 / P2（`RELEASE_BRIEF_zh.md` §5 未覆盖部分）

| ID | 事项 | 备注 | 级别 |
|---|---|---|---|
| **F1** | SubGHz 频谱分析仪 | 33 段 + peak-hold + CAL，需 RMT/CC1101 采样 + UI | P1 |
| **F2** | SubGHz Replay 目标选择器 + RSSI 门控 | 死频段移除已在 `RadioTuningModel` 固化 `offered=false` | P1 |
| **F3** | SubGHz / NRF 调参驾驶舱接线 | `RadioTuningModel` 已就位，缺 UI 接线 | P1 |
| **F4** | 统一 Loot Manager + GPS 打标 | 大类目，含 Honeypot / FlockYou / Predator 查看器扩展 | P1 |
| **F5** | Wardriving 双核 2× + 增量内存 | 消除逐行分配 | P1 |
| **F6** | Fast Touch Engine | 触摸层改造 | P1 |
| **F7** | 内存归还 + 低内存优雅退出 | **必须由 `HeapBaseline` 给出前后百分比**，不接受「感觉省了」 | P1 |
| **F8** | KeeLoq 滚动码解码 + RF600E/FM 读取 | 需厂商密钥表（`X5`） | P1 |
| **F9** | IoT Recon（厂商指纹 + SD 流式字典 + Digest） | 大工程，含主动利用路径 | P1 |
| **F10** | Drone Toolkit 四件套 | Detect / Jelly / Deauth / RID Spoof；**P2 独立审批** | P2 |
| **F11** | Radio Test 重构 | NRF 126 信道 + CC1101 RSSI 基线 + 接线图 | P2 |
| **F12** | Flock You WiFi 模式 + OUI 扩充 | 已有 `FlockFingerprint` 基础 | P2 |
| **F13** | Skimmer Hunter 主动确认 pass | 已有 `SkimmerScore` 基础 | P2 |
| **F14** | Rd-01 24GHz 雷达 | 需新硬件 | P2 |

---

## 7. 证据门（未闭合前不得实现，见 `docs/protocol-evidence/`）

| 项 | 文档 | 当前状态 | 解锁条件 |
|---|---|---|---|
| MIFARE 魔术卡后门 | `mifare-classic.md` §4 | `UNSUPPORTED` | 提供可复现型号 + 流程 |
| HID reader 服务 UUID | 待补 | 无权威来源 | `X3` |
| Airoha RACE 载荷 | `airoha-race.md` | `UNSUPPORTED` | 提供抓包与可复现步骤 |
| Tesla Charge US/EU/BOTH | — | `Unsupported` | 提供已审阅载荷与测试向量 |
| iCLASS Corporate 1000 主密钥 | — | 无 | `X5` |
| KeeLoq 厂商密钥 | — | 无 | `X5` |

> 规则（沿用 `airoha-race.md` 的口径）：**无法核验的协议部分显式标 `UNSUPPORTED` 并写明升级条件，
> 不猜、不用占位实现冒充完成。**

---

## 8. 建议执行顺序

1. **W1** 新模型进 `FeatureCatalog`（低成本，让已有成果从「测试里」变成「固件里」）
2. **W2 + R10** `CardDumpFormat` → `StorageService` → Loot 查看器（打通 B1）
3. **X1** 沙箱外 `pio run` + `.bin`（恢复「能出固件」的能力）
4. **X2 → T1–T4** 上机台架回归（B1–B4）
5. **R1 → R3 → R4/R5/R7** ST25R3916 / PN532 驱动链（RFID P0 的共同前置）
6. **R2** 七屏 UI 重构
7. **B1/B2/B3** BLE 传输层
8. **R9** ISO-DEP / Type 4
9. 其余 P1 → P2
