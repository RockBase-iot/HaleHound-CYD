# Progress Log

## Session: 2026-07-27

### Phase 1: 现状与基线调查
- **Status:** complete
- **Started:** 2026-07-27
- Actions taken:
  - 读取并采用 `using-superpowers`、`brainstorming`、`planning-with-files` 工作流。
  - 检查当前 Git 分支、最近提交和工作树状态。
  - 确认项目已开始 PlatformIO 迁移且存在用户未提交改动。
  - 尝试运行 session-catchup，确认本机没有 Python。
  - 对比用户提供的 v3.8.1 README、仓库 README、`DEVELOPMENT_PLAN.md` 与 `platformio.ini`。
  - 确认现有开发计划的硬件矩阵尚未反映本次“两主板 + 可选扩展板”要求。
  - 提取 v3.8.1 完整菜单树、新功能清单、外设与接线约束。
  - 识别 PN532 SPI→I2C、红外新增、外设 Required→Optional 三项关键需求偏移。
  - 审阅现有 `DEVELOPMENT_PLAN.md` 的架构提议，标出可复用部分与必须重写的硬件矩阵。
  - 审阅 `platformio.ini` 和用户正在修改的构建矩阵脚本，确认不触碰现有构建修复。
  - 盘点 `src/include/boards` 迁移结构和 v3.8.1 功能符号。
  - 确认当前硬件能力仍是编译期常量，且 PN532 在 C5 与经典 CYD 上采用不同总线模型。
  - 检查 Git refs，确认仓库中没有可见的 v3.8.1 源码分支或标签。
  - 审阅 `BoardProfile` 与 `App` 过渡层，确认可以在现有骨架上渐进引入运行时外设注册表。
- Files created/modified:
  - `task_plan.md`（新建）
  - `findings.md`（新建）
  - `progress.md`（新建）

### Phase 2: 边界与方案确认
- **Status:** in_progress
- Actions taken:
  - 用户确认没有 v3.8.1 源码，以提供的 README 作为功能验收清单。
  - 克隆并只读核对 NM-RF-HAT 官方仓库、FAQ、用户手册、原理图和引脚图。
  - 确认扩展功能拨码互斥、PN532-I2C、IR/RF433 引脚以及两主板信号映射。
  - 用户确认采用方案 A，并授权对整体应用框架进行较大调整。
  - 用户修正外设模型：以单个 RF/外设模块为最小单元，NM-RF-HAT 仅作为接线参考。
  - 审阅 C5 SPI 配置、现有 SPI manager 和核心绑定任务，确认需统一总线租约与单核调度抽象。
  - 用户确认两块主板不强制使用同一种 `TaskScheduler`：经典 ESP32 可按需调用 `xTaskCreatePinnedToCore()`，C5 使用单核执行策略。
  - 将调度设计修正为统一工作负载契约 + 板级 `ExecutionPolicy` 后端，并保留跨后端一致的资源仲裁、退出和清理约束。
  - 用户确认第 3 节功能注册、生命周期、策略守卫、外设预检和导航设计。
  - 重新核对 v3.8.1 README 的完整菜单树、九类设置项、SD 目录合同、Loot 五类内容和 SD 固件更新入口。
  - 对照当前 EEPROM、Loot、Saved Captures、Wardriving 和 Firmware Update 实现，识别直接 SPI/SD 访问、跨模块 EEPROM 布局和双主板镜像误刷风险。
  - 确认 ESP32-2432S028 为 4 MB Flash、NM-CYD-C5 为 16 MB，当前两环境均选择 `huge_app.csv`；更新与回滚能力必须按板卡和最终镜像尺寸分别验收。
  - 用户确认第 4 节存储、设置、日志和故障恢复设计。
  - 以 README 3.8.1 菜单树为主键核对当前菜单数组、处理分支、模块头文件和实现符号。
  - 初步完成各功能分类：WiFi/NRF24/Jam Detect 基线较完整；BLE、SubGHz、RFID、SIGINT 存在明确缺口；Drone 与 Skimmer 整类缺失。
  - 细化 README 对 BLE Predator、Airoha RACE、NTAG Tools、Flock You、Drone 与 Skimmer 的协议和数据行为要求。
  - 发现 `/creds.txt` 在 README 与当前代码间的输入/捕获语义冲突，确定在需求中保护 README 输入合同并迁移捕获数据。
  - 将 IR/RF433 定位为项目扩展验收项，不混入 README 3.8.1 原生功能完成率。
  - 用户确认第 5 节功能矩阵、缺失功能验收行为和 IR/RF433 扩展边界。
  - 审阅现有五目标构建脚本、构建报告、仓库自动化和 Radio Test，确认当前没有测试/CI 且硬件自检只覆盖 nRF24 与 CC1101。
  - 提取经典 CYD 成功构建的 RAM/Flash 基线；确认 C5 日志停在平台解包阶段，尚无有效编译基线。
  - 用户确认第 6 节双目标构建、自动检查、外设诊断、C5 SPI 压力测试与验收证据设计。
  - 使用 Codex 工作区依赖中的 Python 成功运行 session-catchup，并核对当前规划文件、Git 状态及未同步上下文。
  - 用户确认第 7 节渐进迁移、兼容和发布策略，并授权开始后续代码整合。
  - 新建正式设计文档 `docs/superpowers/specs/2026-07-28-halehound-cyd-3.8.1-refactor-design.md`，作为替代旧五板计划的实施基线。
  - 完成规格自审：占位符扫描为零；旧板和 PN532-SPI 仅出现在明确的退役说明中；`git diff --check` 通过。
  - 仅提交正式设计文档，提交为 `c05832d`；提交包含 1 个文件，最终 `git diff --check` 为 0，文档路径无未提交修改。
- Files created/modified:
  - `task_plan.md`
  - `findings.md`
  - `progress.md`

## Session: 2026-07-28 - Tasks 1-6 implementation

### Completed in workspace
- Added the two-board PlatformIO target contract and removed legacy root environments.
- Added pure native core types, feature lifecycle interfaces, board profiles, independent peripheral routes, and board execution policy selection.
- Added the initial feature catalog and guarded lifecycle runner with failure cleanup and idempotent stop behavior.
- Added `scripts/verify_targets.ps1` and `scripts/audit_boundaries.ps1`.

### Verification
- `esp32-cyd` PlatformIO build: PASS.
- `verify_targets.ps1`: PASS.
- `audit_boundaries.ps1`: PASS.
- `git diff --check`: PASS with existing line-ending warnings.
- native Unity tests: blocked because no host `gcc`/`g++` is installed.
- `nm-cyd-c5` build: blocked while PlatformIO unpacks Arduino 3.3.6 libraries due a Windows missing-directory/long-path extraction error.

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Git 状态检查 | `git status --short` | 识别并保留用户改动 | 已识别 4 项现有改动 | 通过 |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-07-27 | `python` 命令不存在，`py` 也未关联解释器 | 1 | 停止重复尝试，改用 Git/文件手工恢复 |
| 2026-07-27 | `C:\tmp\NM-RF-HAT` 创建被拒绝 | 1 | 改用工作区内 `.codex_tmp\NM-RF-HAT` |
| 2026-07-27 | 分区检查使用不存在的 `boards/nm-cyd-c5.ini`，且 `rg` 在 Windows 下不能直接接收 `*.csv` 路径参数 | 1 | 改为实际配置路径并使用 PowerShell 文件枚举 |
| 2026-07-27 | 并行检索缺失功能时，`rg` 的预期退出码 1 中断整组输出 | 2 | 停止该组合，零匹配改用 `Select-String`，确定匹配项拆分读取 |
| 2026-07-27 | Windows 沙箱 ACL helper 中断并行只读检查 | 1 | 缩小读取分组后成功继续 |
| 2026-07-27 | 聚合构建日志时无匹配文件使 `rg` 再次返回 1 | 3 | 永久停止此检索模式，后续日志使用 `Select-String`/`Get-Content` |
| 2026-07-28 | 系统 `python` 不在 PATH，首次 session-catchup 失败 | 2 | 改用 Codex 工作区依赖提供的显式 Python 路径，恢复检查成功 |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 2：边界与方案确认 |
| Where am I going? | 方案确认、需求设计、实施计划 |
| What's the goal? | 形成 3.8.1 功能与两主板/可选 NM-RF-HAT 架构的可执行需求 |
| What have I learned? | 见 `findings.md` |
| What have I done? | 已完成技能加载、Git 检查和持久化计划初始化 |

## Session: 2026-07-28 - VALHALLA 安全策略修订

### Phase 3: 需求与设计文档
- **Status:** in_progress（等待用户审阅正式规格）
- Actions taken:
  - 将 VALHALLA 明确为所有攻击性功能的统一责任确认协议，而不是普通提示页。
  - 将旧版 `disclaimerAccepted` 与 `blueTeamMode` 两个布尔量收敛为 `ValhallaAccepted` / `BlueTeam` 单一持久化状态。
  - 规定首次启动、拒绝、BOOT 返回、状态缺失、损坏或迁移异常均默认进入 `BlueTeam`。
  - 规定 Blue Team 仅允许 Passive/Defensive 功能，并在重启、固件更新、设置迁移和异常恢复后保持。
  - 规定所有菜单入口、快捷入口、功能间交接和程序化启动统一调用 `PolicyService::authorize()`。
  - 规定主屏持续显示 `VALHALLA` 或 `BLUE TEAM`，状态变化后立即重绘。
  - 将旧 `activateValhalla()` 数据擦除流程与责任确认协议明确解耦。
  - 完成规格自审：必要策略语义全部存在；不存在 `ConsentPending`、占位符或未决项；`git diff --check` 通过。
  - 仅提交正式设计文档，修订提交为 `78247a0`（`docs: clarify VALHALLA safety policy`），未包含构建脚本或规划文件。
- Files modified:
  - `docs/superpowers/specs/2026-07-28-halehound-cyd-3.8.1-refactor-design.md`
  - `task_plan.md`
  - `findings.md`
  - `progress.md`

### Review Command Errors
- 首次检查脚本因 PowerShell 命令字符串转义错误失败；改为逐项检查。
- 第二次检查脚本因过度严格的精确匹配表达式误报；改为语义关键词与禁止项组合检查。

## Session: 2026-07-28 - 详细实施计划

### Phase 4: 实施计划
- **Status:** in_progress
- Actions taken:
  - 用户确认正式设计规格，进入详细实施计划阶段。
  - 采用 `writing-plans`，计划按可测试纵向切片、TDD 和频繁提交拆解。
  - 首次源码/测试目录枚举包含尚不存在的 `test/` 与 `tests/`，导致 `rg --files` 返回 1；后续改为只枚举实际目录，并把测试骨架创建列入首批任务。
  - Windows `rg` 对头文件路径通配再次返回错误；停止该模式，改为 PowerShell 文件枚举与 `Select-String`。
  - 新建主实施计划 `docs/superpowers/plans/2026-07-28-halehound-cyd-3.8.1-master-implementation-plan.md`，拆分为 18 个按依赖执行的任务。
  - 首轮自审发现 native 过滤器与若干服务实现路径不一致；将 ResourceBroker、SPI 状态机、PeripheralRegistry、SettingsRecord 和 FirmwareImageValidator 的可测试算法统一放入 `src/services/pure/`，Arduino 后端留在平台/服务适配层。
  - 完成计划自审：18 个任务、26 个关键需求锚点、0 个禁用占位词；纯逻辑实现路径与 native 过滤器一致；`git diff --check` 通过。
  - 首次暂存检查发现 12 行 Markdown 行尾双空格；已删除并安排重新暂存、复查。
  - 仅提交主实施计划文档，提交为 `78cd83f`（`docs: add 3.8.1 refactor implementation plan`）；提交包含 1 个文件。
- Files created/modified:
  - `docs/superpowers/plans/2026-07-28-halehound-cyd-3.8.1-master-implementation-plan.md`
  - `task_plan.md`
  - `findings.md`
  - `progress.md`

## Session: 2026-07-28 - Tasks 7-9 completion pass

### Completed in workspace
- Task 7 routes `App::setup()` and `App::loop()` through `AppKernel`, with structured navigation outcomes and a single-step `LegacyFeatureAdapter` lifecycle.
- Task 8 provides move-only `ResourceLease`, scoped ownership, recursive-acquire rejection, timeout/busy results, explicit recovery, and mutex injection.
- Task 9 provides ordered SPI transactions, display/touch service wrappers, and an Arduino backend covering the two board pin maps.
- Task 8 hardware integration now uses `ArduinoResourceMutex` backed by a FreeRTOS mutex on Arduino targets; native builds retain a deterministic fake lock.
- Legacy `spi_manager` now obtains and releases a `ResourceId::SharedSpi` lease through `SpiBusService` compatibility sessions. Legacy sessions intentionally leave transaction ownership to existing SD/radio libraries while preserving CS cleanup and arbitration.
- Added a native regression case for a legacy session holding the shared resource until deselection.

### Verification
- `git -c safe.directory=E:/github/HaleHound-CYD diff --check`: PASS (only existing line-ending warnings).
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `pio run -e esp32-cyd`: PASS with exit code 0.
- Native Unity suite remains blocked because host `gcc`/`g++` is not installed.
- `pio run -e nm-cyd-c5` remains blocked before compilation while Arduino 3.3.6 libraries unpack; the failing path is under `esp32h2/include/espressif__esp_matter`.

### Remaining Task 9 migration scope
- Existing legacy feature files still contain direct SPI/SD calls; they remain the explicitly allowed compatibility surface until their feature-specific migration tasks replace them with service transactions.

## Session: 2026-07-28 - Tasks 10-11 implementation pass

### Completed in workspace
- Added `PeripheralRegistry` with on-demand, per-device probing, cached diagnostics, TTL invalidation, and independent `Unverified`/`Available`/`NotFound`/`Busy`/`Fault`/`Unsupported` states.
- Added independent CC1101, nRF24, PN532-I2C, and bounded GPS probe drivers. CC1101 and nRF24 probes use the shared SPI service; they never claim simultaneous presence on the C5 shared CS/control route.
- Unified the PN532 route model to I2C for both board profiles. PN532 probing now asks the controller for a firmware version rather than treating a floating address as an available device.
- Added versioned, packed settings records with CRC32, dual-slot sequence selection, read-back verification, legacy migration, and safe Blue Team defaults.
- Added `PolicyService` with persistent `BlueTeam`/`ValhallaAccepted` modes, offensive authorization gating, mode labels, and a legacy `isOffensiveAllowed()` bridge. Invalid legacy pairs now migrate to Blue Team; the home banner treats unaccepted consent as Blue Team.
- Added TDD fixtures for peripheral caching/isolation and settings/policy migration/authorization.

### Verification
- `pio run -e esp32-cyd`: PASS after the Task 10/11 changes; latest build includes the unified PN532-I2C route and split `SettingsService.cpp`.
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `git diff --check`: PASS (existing line-ending warnings only).
- Native Unity command collected the new tests but skipped execution because this host has no native `gcc`/`g++` toolchain.
- `nm-cyd-c5`: platform dependencies were unpacked successfully into a short-path cache, but every subsequent PlatformIO invocation remained before project compilation with no C5 build artifacts; the stuck helper processes were stopped and the issue is recorded as a local pioarduino/PlatformIO environment blocker, not a source diagnostic.

## Session: 2026-07-28 - Tasks 12-13 implementation pass

### Completed in workspace
- Added canonical SD path policy, read-only `/creds.txt` handling, legacy `[PSK]` migration, one-shot storage initialization, atomic temp/flush/hash/rename writes, and platform-specific SD backend.
- Migrated legacy Loot, Saved Captures, WhisperPair Loot, Captive Portal, and firmware update SD initialization to `StorageService`; new captures use `/loot/credentials/`.
- Added ESP image validation for magic, header, chip id, segment bounds, partition size, revision metadata, and filename-independent results.
- Added persistent boot pending/healthy records with failure threshold Safe Mode and explicit recovery; UI blocks non-diagnostic feature categories while Safe Mode is active.

### Verification
- `pio run -e esp32-cyd -j 4`: PASS (exit code 0); Task 12/13 sources compiled and linked.
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `git diff --check`: PASS with existing line-ending warnings.
- Native Unity tests: SKIPPED/blocked because this host has no native GCC/G++ toolchain; elevated retry confirms no test cases execute.
- `pio run -e nm-cyd-c5 -j 1`: blocked before project compilation while extracting Arduino 3.3.6 `esp32-arduino-libs`; failure is the existing Windows long-path/missing-directory error under `esp32h2/include/espressif__esp_matter`.

## Session: 2026-07-28 - Tasks 14-15 implementation pass

### Completed in workspace
- Expanded `FeatureCatalog` to cover WiFi, Bluetooth, nRF24, SubGHz, RFID, defensive detection, SIGINT, Tools, and Settings entries with stable IDs, risk classes, peripheral contracts, storage paths, and non-null lifecycle factories.
- Added `LegacyFeatureFactories` bindings for the existing WiFi, Bluetooth, nRF24, SubGHz, RFID, defensive, and SIGINT lifecycle namespaces; native builds retain deterministic no-hardware adapters.
- Added native lifecycle coverage for every catalog descriptor and retained a repeatable `IFeature` begin/tick/stop/end contract through the registry seam.
- Added bounded `.sub` parsing for RAW, Princeton, CAME, and Nice documents. Unknown protocols, missing fields, traversal paths, and overlong pulses are rejected; parsed documents are explicitly non-transmitting.
- Added controlled SubGHz reader and Tesla Charge feature shells. Tesla US/EU/BOTH evidence remains `Unsupported` until reviewed payloads and test vectors are supplied.
- Added NTAG213/215/216 and Ultralight EV1 capacity/page models, lock warnings, NDEF URI/Text/multi-record encoding, password lifecycle, and an NTAG tools feature shell with PN532-I2C catalog dependency.
- Added TDD fixtures under `test/native/test_catalog_lifecycle`, `test/native/test_sub_file_parser`, and `test/native/test_ntag_model`.

### Verification
- `pio run -e esp32-cyd -j 4`: PASS; final firmware linked successfully after Task 14/15 changes.
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `git diff --check`: PASS with existing line-ending warnings only.
- Native Unity tests: collected but SKIPPED because this host has no native `gcc`/`g++` toolchain.
- `pio run -e nm-cyd-c5`: blocked before source compilation by the known Arduino 3.3.6 dependency extraction failure under `esp32h2/include/espressif__esp_matter`.

## Session: 2026-07-29 - Tasks 16-17 implementation pass

### Completed in workspace
- Task 16 adds BLE Predator phase gating and `deinit(false)` cleanup, an evidence-gated Airoha Race shell, Flock fingerprint scoring with optional GPS tagging, Remote ID parsing and WiFi/BLE aggregation, passive Drone Detect, policy-blocked Drone active shells, and two-stage Skimmer scoring/confirmation.
- Task 17 adds independent IR TX/RX and RF433 TX/RX peripheral IDs and drivers, GPIO mutex-group routing, raw IR pulse and OOK/ASK RF433 models, and safe feature lifecycle shells.
- Airoha Race is isolated in `src/features/ble/AirohaRaceFeature.cpp`; no inferred payloads or active behavior are included while evidence remains incomplete.

### Verification
- `pio run -e esp32-cyd -j 4`: PASS; firmware linked successfully (37.1% RAM, 77.1% flash).
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `git diff --check`: PASS with existing line-ending warnings only.
- Native Unity tests remain blocked because the host has no GCC/G++ toolchain and the PlatformIO test command hit the existing local urllib3 permission issue.
- `pio run -e nm-cyd-c5 -j 1`: blocked before project compilation while extracting Arduino 3.3.6 libraries; failure is the existing Windows long-path/missing-directory error under `esp32h2/include/espressif__esp_matter`.

## Session: 2026-08-03 - C5 boot image and landscape firmware refresh

### Completed in workspace
- Added target-aware FULL-image packaging: `esp32-cyd` bootloader at `0x1000`, `nm-cyd-c5` bootloader at `0x2000`.
- Added build-freshness rejection and byte-for-byte checks for bootloader, partition table, boot_app0, and firmware segments.
- Reproduced the old C5 image failure: `segment mismatch: bootloader.bin at 0x2000`.
- Rebuilt both targets from the responsive `MenuLayout` source and regenerated both FULL images.
- Restored the formal C5 GitHub dependency URL after using the validated workspace cache for the local build.

### Verification
- `pio run -e nm-cyd-c5 -j 1`: PASS; 36.0% RAM, 72.2% flash, 2,271,254 application bytes.
- C5 package: `BUILD_FRESHNESS=PASS`, `FLASH_LAYOUT=PASS`, `PACKAGE_FIRMWARE=PASS`.
- C5 key bytes: `0x0000=FF`, `0x1000=FF`, `0x2000=E9`, `0x8000=AA 50`, `0x10000=E9`.
- `pio run -e esp32-cyd -j 4`: PASS; responsive menu source compiled and firmware linked.
- ESP32 package: `BUILD_FRESHNESS=PASS`, `FLASH_LAYOUT=PASS`, `PACKAGE_FIRMWARE=PASS`.
- `scripts/verify_targets.ps1`: `TARGET_CONTRACT=PASS`.
- `scripts/audit_boundaries.ps1`: `BOUNDARY_AUDIT=PASS`.
- `git diff --check`: PASS with existing line-ending warnings only.
- Native layout tests: collected, but execution is blocked because the host has no `gcc`/`g++` toolchain.
- Hardware acceptance remains open: flash the new FULL image at `0x0`, confirm C5 boot, and visually/touch-test portrait and landscape menus.

## Session: 2026-10-15 - 宿主跑测闭合（native 断言首次真正执行）

### Completed in workspace
- 在 `build-reports/hostenv/` 安装 `ziglang`（pip wheel，自带 clang 21 + libc++/libc sysroot），
  把「本机没有宿主 C++ 编译器」这个从 2026-07-28 起就阻塞每次验收的长期问题闭合成可执行。
- `scripts/hostcheck/run_host_tests.py` 三处改造：
  - `find_compiler()` 返回**命令前缀 argv**（`python -m ziglang c++`）而不是路径；
  - 每个候选编译器必须**探针编译 + 链接 + 运行** `int main(){return 0;}` 才被采纳
    （`~/.platformio` 的 mingw32 `g++ --version` 正常，但每次编译都以**完全空的日志**失败）；
  - 共享源先归档成 `build-reports/hostcheck/libhostcheck.a` 再链接，复刻 PlatformIO `native`
    环境「只拉取被引用目标文件」的语义 —— 直接强制链接全部 `.o` 会被 `src/core/AppKernel.o`
    拖进 `build_src_filter` 排除的 `legacySetup` / `legacyLoop` 而报未定义符号。
- `scripts/hostcheck/unity/unity.h` 补齐类型化相等宏：
  `TEST_ASSERT_EQUAL_UINT8/16/32/64`、`_INT8/16/32/64`、`_CHAR`、`_HEX8/16/32/64`。

### Verification
- 宿主回归：**29/29 套件、142 个用例全绿**（首次真正执行断言，不是只编译）。
- `nm-cyd-c5`（`riscv32-esp-elf-g++`）`-fsyntax-only`：改动文件 7/7 clean。
- `esp32-cyd`（xtensa）：报 `'assert'/'errno' not declared`；`--baseline` 证明**未改动文件报
  完全相同的错** → 判定为 SDK 头文件配置噪声，不是回归。
- 输出存档 `build-reports/hostcheck-report.txt`。

### 首跑暴露的 9 个潜伏缺陷（已全部修复）
此前没有任何可用宿主编译器，这些套件**一次都没被编译执行过**。首跑 20/29，9 个失败全是真实问题：

| 套件 | 症状 | 判定 |
|---|---|---|
| `test_board_profiles` 等 4 套 | `TEST_ASSERT_EQUAL_UINT32` 未定义 | shim 能力缺口 |
| `test_spi_service` | `undefined symbol: WinMain` | 测试文件缺陷：`main()` 落在匿名 namespace 内，全局 `main` 符号不存在 |
| `test_feature_registry` | `incomplete type 'FeatureRegistry'` | 测试文件缺陷：`FeatureCatalog.h` 只做前置声明 |
| `test_ble_switch_ledger` | `replay(nullptr, 0)` 被判 `InvalidProfile` | 实现缺陷：空访问序列应满足空真 → `count == 0` 直接返回 `Clean` |
| `test_card_dump_format` | `.nfc` 往返丢块、`uid[0]`/`unknownKeys` 不符 | **实现缺陷 ×2 + 夹具缺陷 ×1** |
| `test_ndef_codec` | 文本负载断言 400 实际 403 | 断言缺陷：`buildText` 会前置状态字节 + 语言码 |

`CardDumpFormat` 的两个真实缺陷：① `toFlipperNfc` 写出的未读块占位 `Block n: ?? ?? … ??`
**无法被自己的解析器读回**，落入 `unknownKeys` 导致块槽位丢失、后续块号前移（静默损坏转储）；
② 解析器不认自己写出的 `Version:` / `Data format version:`，每次「存 → 读 → 存」重复累积。
夹具缺陷：`makeClassicDump()` 的 block 0 用 `0..15` 递增填充，却断言能推出 UID —— 实现是对的，
MIFARE Classic 块 0 是厂商块（UID 字节 0..3 + BCC + SAK + ATQA），已把夹具改成真实厂商块布局。

## Session: 2026-10-15 - RFID/BLE 差距项 P0–P1 模型层

### Completed in workspace
新增纯逻辑模型（全部配套宿主断言），对应 `RELEASE_BRIEF_zh.md` §5 差距矩阵 #9–#13、#25：

| 模块 | 文件 | 说明 |
|---|---|---|
| `MifareKeys` | `include/features/rfid/MifareKeys.h` / `src/features/rfid/MifareKeys.cpp` | 29 个公开密钥表，取 Proxmark3 `mfc_default_keys.dic` **前 29 条**（上游称「29 密钥」但仓库无源码可审，按具名公开字典的文件顺序是唯一可复现规则）；`static_assert` 钉住数量 |
| `MifareClassicModel` | `include/features/rfid/MifareClassicModel.h` / `src/features/rfid/MifareClassicModel.cpp` | 几何（Mini/1K/4K）、访问位解码、数据块/尾块权限表、尾块构造、魔术卡写入门禁、转储装配与诚实计数 |
| `KeyCheckModel` | `include/features/rfid/KeyCheckModel.h` / `src/features/rfid/KeyCheckModel.cpp` | 29 密钥扫描会话；进度由**尝试次数**推导而非扇区比例；扇区仅在认证成功时计数 |
| `GattNameTable` | `include/features/ble/GattNameTable.h` / `src/features/ble/GattNameTable.cpp` | 44 服务 / 113 特征 / 15 描述符，逐字对齐 Bluetooth SIG 机读清单 |
| `SkeletonKeyModel`（扩展） | `src/features/ble/SkeletonKeyModel.cpp` | 接入 `GattNameTable::applyNames()`，修正 `classifyAppearance` 与 `canWriteFullDump` |

同批修正 3 个既有模型的真实缺陷：`RfidBackendModel::screenLabel` 两屏同名（顶栏无法区分）、
`ChameleonUltraModel::evaluateRead` 读页数超过卡上页数仍判 `valid`、`ChameleonSession::onDeviceFound("")`
采纳空地址进 `Ready`。

### 证据文档（新增）
- `docs/protocol-evidence/mifare-classic.md`：§1 为 **VERIFIED** 的访问位打包规则
  （`byte6 = ~C2<<4 | ~C1`、`byte7 = C1<<4 | ~C3`、`byte8 = C3<<4 | C2`、`byte9` 保留 `0x69`），
  含 `FF 07 80 69` 的完整推演；§1.4 回归向量表；§4 魔术卡后门标 `UNSUPPORTED` 并写明升级条件。
- `docs/protocol-evidence/gatt-names.md`：§5 说明新表与 `include/ble_database.h`
  （Nordic 来源、名称截断到 14 字符、依赖 `Arduino.h`）的重叠关系与刻意不合并的理由。

### Verification
- 宿主回归：**36/36 套件全绿**（本轮新增 7 套：`test_mifare_classic` / `test_key_check` /
  `test_gatt_names` / `test_skeleton_key` / `test_chameleon_ultra` / `test_hid_reader` / `test_rfid_backend`）。
- 真实工具链 `nm-cyd-c5` `-fsyntax-only`：7/7 clean。
- `build-reports/verify_gatt_names.py` 对 SIG 机读清单做**零例外**核验：158 项逐字一致 +
  7 项带 `(vendor)`/`(member)` 标记；核验方式是拉官方 YAML diff，**不靠眼睛**。
  设计上「SIG 已分配必须逐字一致（SIG 官方就叫 `GAP` / `Battery`，不是 `Generic Access` /
  `Battery Service`），未分配必须带来源标记，无法核验的条目**删除**而不是写进例外清单」。

### 已知边界
- 13 个新模型**全部是孤岛**：未注册进 `FeatureCatalog`（无 `FeatureId`、无生命周期工厂、
  无菜单入口），目前只被各自的测试引用。必须接线才能从 `Model Verified` 走到固件里。
- 缺真实传输/驱动层：ST25R3916、BLE client、PN532 写入路径均未开始。
- 完整剩余工作清单见 **`REMAINING_WORK.md`**。
