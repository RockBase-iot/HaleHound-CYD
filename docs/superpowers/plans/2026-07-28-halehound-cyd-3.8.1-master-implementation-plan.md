# HaleHound-CYD 3.8.1 双主板重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在当前 PlatformIO 工程上实现 README v3.8.1 功能合同，仅正式支持 ESP32-2432S028 与 NM-CYD-C5，并建立可选独立外设探测、C5 共享 SPI 仲裁、持久化 VALHALLA/Blue Team 策略和可追溯验收体系。

**Architecture:** 保留 `src/main.cpp` 薄入口，以 `AppKernel`、`FeatureRegistry`、`FeatureRunner`、`NavigationController` 和服务注册表逐步替换 `LegacyApp.inc` 的阻塞式菜单调度。板卡静态信息、外设路由和运行时外设状态分层；全部 SPI/SD/显示/触摸访问经资源服务，旧功能通过临时适配器按类别迁移。

**Tech Stack:** PlatformIO、Arduino ESP32 Core、ESP32-C5 pioarduino、C++17、FreeRTOS、Unity/PlatformIO native tests、TFT_eSPI、RF24、SmartRC-CC1101、Adafruit PN532、ArduinoJson。

---

## 0. 执行规则与基线

- 设计合同：`docs/superpowers/specs/2026-07-28-halehound-cyd-3.8.1-refactor-design.md`。
- 计划基线提交：`78247a0`。
- README 合同 SHA-256：`52AB85830DB157375D502CD0F2B3AF2673C7AD3135F2A9B2424AB3F0148A43B7`。
- 正式硬件环境只有 `esp32-cyd` 和 `nm-cyd-c5`；`native` 仅用于主机测试，不是固件发布目标。
- 当前 `scripts/build_matrix.ps1` 有用户未提交修改。执行 Task 1 前必须保存 `git diff -- scripts/build_matrix.ps1`，在现状上合并，不得覆盖。
- 每个任务遵循 red -> green -> 双板构建/静态审计 -> 单独提交。发生失败时先记录到 `progress.md`，不得跳过验证继续堆叠修改。
- 功能代码不得直接新增 `SPI.begin/end`、`SD.begin/end`、`EEPROM.begin/end` 或固定 GPIO。过渡期例外只能位于明确列出的 legacy 文件，并由审计基线逐任务减少。
- 未连接外设时，启动、主菜单、Settings、About 和被动功能必须可用；外设功能返回结构化 `NotFound`、`Busy`、`Fault` 或 `Unsupported`。
- 所有 `RiskClass::Offensive` 入口必须先经过 `PolicyService::authorize()`，再进行外设探测或资源申请。

## 1. 目标文件结构

| 路径 | 职责 |
|---|---|
| `include/core/FeatureTypes.h` | 稳定功能 ID、风险级别、结果码和生命周期类型 |
| `include/core/IFeature.h` | 功能生命周期接口 |
| `include/core/FeatureRegistry.h` / `src/core/FeatureRegistry.cpp` | 功能描述符注册与查询 |
| `include/core/FeatureRunner.h` / `src/core/FeatureRunner.cpp` | 统一预检、运行、停止和幂等清理 |
| `include/core/AppKernel.h` / `src/core/AppKernel.cpp` | 初始化、主 tick 和故障边界 |
| `include/core/NavigationController.h` / `src/core/NavigationController.cpp` | 菜单栈与结构化结果导航 |
| `include/core/PolicyService.h` / `src/core/PolicyService.cpp` | VALHALLA、Blue Team、PIN 和风险授权 |
| `include/board/BoardProfile.h` / `src/board/BoardProfiles.cpp` | 两块主板固有静态信息 |
| `include/board/ExpansionRoute.h` / `src/board/ExpansionRoutes.cpp` | 独立外设候选接线和互斥组 |
| `include/platform/ExecutionPolicy.h` | 板无关工作负载契约 |
| `src/platform/ExecutionPolicyEsp32.cpp` | 经典 ESP32 可固定核心的执行后端 |
| `src/platform/ExecutionPolicyC5.cpp` | C5 单核合作式/非固定核心执行后端 |
| `include/services/ResourceBroker.h` / `src/services/pure/ResourceBroker.cpp` | 有界资源租约、所有者和超时 |
| `include/services/SpiBusService.h` / `src/services/pure/SpiBusService.cpp` | 可注入后端的 SPI 事务、CS 去选择、设备配置 |
| `include/services/DisplayService.h` / `src/services/DisplayService.cpp` | 分批渲染与显示租约 |
| `include/services/TouchService.h` / `src/services/TouchService.cpp` | 短触摸采样与触摸租约 |
| `include/services/StorageService.h` / `src/services/StorageService.cpp` | SD 挂载、开放路径和原子文件写入 |
| `include/services/SettingsService.h` / `src/services/SettingsService.cpp` | Arduino 持久化后端与设置服务 |
| `src/services/pure/SettingsRecord.cpp` | 版本化设置、CRC、双记录选择和迁移算法 |
| `include/services/PeripheralRegistry.h` / `src/services/pure/PeripheralRegistry.cpp` | 按需探测、状态缓存和诊断 |
| `include/services/FirmwareImageValidator.h` / `src/services/pure/FirmwareImageValidator.cpp` | 镜像头、芯片和长度验证 |
| `include/drivers/*.h` / `src/drivers/*.cpp` | CC1101、nRF24、PN532-I2C、GPS、IR、RF433 独立驱动 |
| `include/features/LegacyFeatureAdapter.h` / `src/features/LegacyFeatureAdapter.cpp` | 旧 namespace 生命周期适配 |
| `src/features/*` | 新功能与完成迁移后的功能实现 |
| `test/native/*` | 纯逻辑 Unity 测试 |
| `scripts/verify_targets.ps1` | 双固件目标合同检查 |
| `scripts/audit_boundaries.ps1` | 禁止直连资源与旧板符号审计 |
| `docs/acceptance/*` | 双板、外设、SPI 压力与发布证据模板 |

## 2. 里程碑与依赖

| 里程碑 | 任务 | 出口条件 |
|---|---|---|
| M1 构建基线 | 1-2 | 两固件环境 + native 测试可重复运行 |
| M2 应用内核 | 3-7 | 旧主菜单经新内核启动，生命周期与导航主机测试通过 |
| M3 资源与外设 | 8-10 | C5 共享 SPI 服务、独立探测与零外设启动通过 |
| M4 安全与数据 | 11-13 | VALHALLA、设置、SD、更新和恢复测试通过 |
| M5 现有功能迁移 | 14 | 所有现有类别经 Registry/Runner 进入和退出 |
| M6 README 缺失功能 | 15-16 | 缺失功能完成行为验收或明确阻断发布 |
| M7 项目扩展 | 17 | IR/RF433 独立驱动和最小收发验收 |
| M8 发布 | 18 | 双板、零外设、C5 压力和全部证据门槛通过 |

---

### Task 1: 收敛 PlatformIO 双板目标并保护现有构建脚本修改

**Files:**
- Create: `scripts/verify_targets.ps1`
- Modify: `platformio.ini`
- Modify: `scripts/build_matrix.ps1`
- Modify: `include/cyd_config.h`
- Modify: `include/User_Setup.h`
- Test: `scripts/verify_targets.ps1`

- [ ] **Step 1: 保存重叠文件的用户差异**

Run:

```powershell
git -c safe.directory=E:/github/HaleHound-CYD diff -- scripts/build_matrix.ps1
git -c safe.directory=E:/github/HaleHound-CYD status --short
```

Expected: 输出当前用户修改；不得执行还原、覆盖或清理。

- [ ] **Step 2: 写双目标合同检查并确认当前失败**

`scripts/verify_targets.ps1` 读取 `platformio.ini` 和构建脚本，要求固件环境集合严格为：

```powershell
$expected = @("esp32-cyd", "nm-cyd-c5")
$iniTargets = Select-String -Path "platformio.ini" -Pattern '^\[env:([^\]]+)\]$' |
    ForEach-Object { $_.Matches[0].Groups[1].Value } |
    Where-Object { $_ -ne "native" }
$matrixText = Get-Content -Raw "scripts/build_matrix.ps1"
$unexpected = @($iniTargets | Where-Object { $_ -notin $expected })
$missing = @($expected | Where-Object { $_ -notin $iniTargets -or -not $matrixText.Contains('"' + $_ + '"') })
if ($unexpected.Count -gt 0 -or $missing.Count -gt 0) {
    throw "Target contract failed. Unexpected=$($unexpected -join ',') Missing=$($missing -join ',')"
}
Write-Output "TARGET_CONTRACT=PASS"
```

Run: `powershell -ExecutionPolicy Bypass -File scripts/verify_targets.ps1`
Expected: FAIL，列出三个旧环境。

- [ ] **Step 3: 移除旧环境与旧板分支**

从 `platformio.ini` 删除 `esp32-cyd-hat`、`esp32-e32r28t`、`esp32-e32r35t`；在 `scripts/build_matrix.ps1` 保留用户现有修改，只将 `$environments` 收敛为两个目标。删除 `cyd_config.h` 和 `User_Setup.h` 中对应旧板选择分支、`NMRF_HAT` 整体板型分支和 E32 专属能力。

- [ ] **Step 4: 运行目标合同**

Run: `powershell -ExecutionPolicy Bypass -File scripts/verify_targets.ps1`
Expected: `TARGET_CONTRACT=PASS`。

- [ ] **Step 5: 建立双板编译基线**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e nm-cyd-c5
```

Expected: 两个命令均返回 0；若 C5 平台安装仍未完成，记录完整日志并在本任务内解决，不能用旧日志代替。

- [ ] **Step 6: 提交**

```powershell
git add platformio.ini scripts/verify_targets.ps1 scripts/build_matrix.ps1 include/cyd_config.h include/User_Setup.h
git commit -m "build: reduce firmware matrix to two boards"
```

---

### Task 2: 建立 native 测试骨架与核心类型

**Files:**
- Modify: `platformio.ini`
- Create: `include/core/FeatureTypes.h`
- Create: `include/core/IFeature.h`
- Create: `test/native/test_feature_types/test_main.cpp`

- [ ] **Step 1: 写失败测试**

```cpp
#include <unity.h>
#include "core/FeatureTypes.h"

void test_offensive_feature_is_not_passive() {
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(RiskClass::Passive),
        static_cast<int>(RiskClass::Offensive));
}

void test_not_found_result_preserves_code() {
    const FeatureResult result{FeatureResultCode::NotFound, "cc1101"};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(FeatureResultCode::NotFound),
        static_cast<int>(result.code));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_offensive_feature_is_not_passive);
    RUN_TEST(test_not_found_result_preserves_code);
    return UNITY_END();
}
```

Run: `C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f test_feature_types`
Expected: FAIL，`core/FeatureTypes.h` 不存在。

- [ ] **Step 2: 增加 native 环境**

在 `platformio.ini` 增加：

```ini
[env:native]
platform = native
test_framework = unity
test_build_src = yes
build_flags =
    -std=gnu++17
    -Iinclude
build_src_filter =
    -<*>
    +<core/>
    +<board/>
    +<services/pure/>
```

- [ ] **Step 3: 实现稳定核心类型**

`FeatureTypes.h` 至少定义：

```cpp
#pragma once
#include <cstdint>

enum class RiskClass : uint8_t { Passive, Defensive, Offensive, SensitiveData };
enum class FeatureResultCode : uint8_t {
    Completed, Cancelled, Blocked, NotFound, Busy, Fault, Unsupported
};
enum class StopReason : uint8_t {
    UserRequest, Navigation, PolicyBlocked, StartFailure, Fault, Shutdown
};
enum class FeatureState : uint8_t {
    Idle, Preflight, Starting, Running, Stopping, Failed
};

struct FeatureResult {
    FeatureResultCode code;
    const char* detail;
};
```

`IFeature.h` 定义 `begin(FeatureContext&)`、`tick()`、`requestStop()` 和幂等 `end()`，不包含 Arduino、TFT、SPI 或板型头文件。

- [ ] **Step 4: 运行测试和双板构建**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f test_feature_types
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e nm-cyd-c5
```

Expected: native 测试全部 PASS；双板返回 0。

- [ ] **Step 5: 提交**

```powershell
git add platformio.ini include/core test/native/test_feature_types
git commit -m "test: add native core contract harness"
```

---

### Task 3: 拆分 BoardProfile 与 ExpansionRoute

**Files:**
- Replace: `include/board/BoardProfile.h`
- Create: `include/board/ExpansionRoute.h`
- Create: `src/board/BoardProfiles.cpp`
- Create: `src/board/ExpansionRoutes.cpp`
- Test: `test/native/test_board_profiles/test_main.cpp`

- [ ] **Step 1: 写失败测试**

测试必须断言：

```cpp
TEST_ASSERT_EQUAL(BoardId::Esp32Cyd, classic.id);
TEST_ASSERT_EQUAL_UINT32(4U * 1024U * 1024U, classic.flashBytes);
TEST_ASSERT_FALSE(classic.singleCore);
TEST_ASSERT_EQUAL(BoardId::NmCydC5, c5.id);
TEST_ASSERT_EQUAL_UINT32(16U * 1024U * 1024U, c5.flashBytes);
TEST_ASSERT_TRUE(c5.singleCore);
TEST_ASSERT_EQUAL(6, c5.sharedSpi.sck);
TEST_ASSERT_EQUAL(2, c5.sharedSpi.miso);
TEST_ASSERT_EQUAL(7, c5.sharedSpi.mosi);
TEST_ASSERT_EQUAL(Transport::I2c, findRoute(BoardId::NmCydC5, PeripheralId::Pn532).transport);
```

还要静态断言 `BoardProfile` 不包含 `hasCc1101`、`hasNrf24`、`hasPn532` 或 `hatPresent`。

Run: `C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f test_board_profiles`
Expected: FAIL，拆分类型尚不存在。

- [ ] **Step 2: 定义板卡固有数据**

`BoardProfile` 只包含 `BoardId`、名称、MCU、Flash、单核标志、显示/触摸/背光、安全按钮、总线控制器和板载 SD。`BoardProfiles.cpp` 提供 `getBoardProfile(BoardId)`，编译宏只负责选择 `currentBoardProfile()`。

- [ ] **Step 3: 定义独立路由**

`ExpansionRoute` 包含：

```cpp
struct ExpansionRoute {
    PeripheralId peripheral;
    BoardId board;
    Transport transport;
    BusId bus;
    int cs;
    int controlA;
    int controlB;
    uint8_t mutexGroup;
    const char* wiringHint;
};
```

为 CC1101、nRF24、PN532-I2C、IR TX/RX、RF433 TX/RX、GPS 和 SD 分别建立候选路由。路由表中不得出现 HAT 设备 ID。

- [ ] **Step 4: 运行测试、目标审计和双板构建**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f test_board_profiles
Select-String -Path include/board/*.h,src/board/*.cpp -Pattern "NmRfHatDevice","hatPresent","hasCc1101","hasNrf24","hasPn532"
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e nm-cyd-c5
```

Expected: 测试 PASS；`Select-String` 无匹配；双板返回 0。

- [ ] **Step 5: 提交**

```powershell
git add include/board src/board test/native/test_board_profiles
git commit -m "refactor: separate board profiles from peripheral routes"
```

---

### Task 4: 定义板级 ExecutionPolicy

**Files:**
- Create: `include/platform/ExecutionPolicy.h`
- Create: `src/platform/ExecutionPolicyEsp32.cpp`
- Create: `src/platform/ExecutionPolicyC5.cpp`
- Create: `src/platform/ExecutionPolicyFactory.cpp`
- Test: `test/native/test_execution_policy/test_main.cpp`

- [ ] **Step 1: 写负载选择失败测试**

```cpp
WorkloadSpec ui{WorkloadRole::Ui, 10, 4, 4096, false};
WorkloadSpec radio{WorkloadRole::Radio, 4, 2, 6144, true};
TEST_ASSERT_EQUAL(ExecutionMode::PinnedTask, chooseMode(BoardId::Esp32Cyd, radio));
TEST_ASSERT_EQUAL(ExecutionMode::CooperativeTick, chooseMode(BoardId::NmCydC5, radio));
TEST_ASSERT_EQUAL(ExecutionMode::CooperativeTick, chooseMode(BoardId::NmCydC5, ui));
```

Run: native `test_execution_policy`。
Expected: FAIL，接口不存在。

- [ ] **Step 2: 实现工作负载契约**

`WorkloadSpec` 包含角色、优先级、最大切片毫秒、栈大小、后台运行标志和停止令牌。`ExecutionPolicyEsp32` 是唯一允许调用 `xTaskCreatePinnedToCore()` 的位置；`ExecutionPolicyC5` 维护合作式队列，每次 `tick()` 只运行一个有界工作片。

- [ ] **Step 3: 增加静态审计**

在 `scripts/audit_boundaries.ps1` 中检查 `src/features`、`src/core` 和 `src/services`；除 `ExecutionPolicyEsp32.cpp` 外发现 `xTaskCreatePinnedToCore` 即失败。

- [ ] **Step 4: 验证并提交**

Run: native 测试、`scripts/audit_boundaries.ps1` 和双板构建。
Expected: 全部返回 0。

```powershell
git add include/platform src/platform test/native/test_execution_policy scripts/audit_boundaries.ps1
git commit -m "feat: add board-specific execution policies"
```

---

### Task 5: 建立 FeatureRegistry

**Files:**
- Create: `include/core/FeatureRegistry.h`
- Create: `src/core/FeatureRegistry.cpp`
- Create: `include/features/FeatureCatalog.h`
- Create: `src/features/FeatureCatalog.cpp`
- Test: `test/native/test_feature_registry/test_main.cpp`

- [ ] **Step 1: 写注册表失败测试**

测试稳定 ID 唯一、分类顺序稳定、显示名不是主键、所有 Offensive 项声明风险、所有外设依赖使用 `PeripheralId`：

```cpp
const auto* deauth = registry.find(FeatureId::WifiDeauth);
TEST_ASSERT_NOT_NULL(deauth);
TEST_ASSERT_EQUAL(RiskClass::Offensive, deauth->risk);
TEST_ASSERT_EQUAL(0, deauth->requiredPeripheralCount);
const auto* subReplay = registry.find(FeatureId::SubGhzReplay);
TEST_ASSERT_EQUAL(PeripheralId::Cc1101, subReplay->requiredPeripherals[0]);
```

- [ ] **Step 2: 定义描述符与目录**

`FeatureDescriptor` 固定包含 `FeatureId`、`CategoryId`、显示名、`RiskClass`、必需/可选外设、服务、存储需求和 factory。把设计文档第 8 节所有 README 功能以及 IR/RF433 扩展注册为稳定 ID；缺失实现的 factory 返回空，但菜单项仍存在并显示 `Unsupported`，直到对应任务提供实现。

- [ ] **Step 3: 验证目录完整性**

测试精确核对各类别期望 ID 集合，防止菜单字符串存在但描述符缺失。

- [ ] **Step 4: 运行 native 测试和提交**

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native -f test_feature_registry
git add include/core/FeatureRegistry.h src/core/FeatureRegistry.cpp include/features/FeatureCatalog.h src/features/FeatureCatalog.cpp test/native/test_feature_registry
git commit -m "feat: register the 3.8.1 feature catalog"
```

---

### Task 6: 建立 FeatureRunner 与幂等生命周期

**Files:**
- Create: `include/core/FeatureRunner.h`
- Create: `src/core/FeatureRunner.cpp`
- Test: `test/native/test_feature_runner/test_main.cpp`

- [ ] **Step 1: 写失败、阻断和清理测试**

使用 `FakeFeature` 记录调用序列，覆盖：

```text
authorize -> route support -> peripheral probe -> resource preflight -> begin -> tick -> end
```

断言 Policy 阻断时 `beginCount=0`；探测失败时 `beginCount=0`；`begin()` 部分失败时 `endCount=1`；重复停止时 `endCount=1`。

- [ ] **Step 2: 实现状态机**

`FeatureRunner::start(FeatureId)` 严格执行设计第 6.3 节预检顺序。`tick()` 只驱动 Running 实例；`requestStop()` 转入 Stopping；所有失败统一调用 `finish()`，由 `finish()` 保证 `end()` 最多一次并释放上下文租约。

- [ ] **Step 3: 验证**

Run: native `test_feature_runner`。
Expected: 全部调用序列精确匹配；0 failure。

- [ ] **Step 4: 提交**

```powershell
git add include/core/FeatureRunner.h src/core/FeatureRunner.cpp test/native/test_feature_runner
git commit -m "feat: add guarded feature lifecycle runner"
```

---

### Task 7: 建立 AppKernel、NavigationController 与 LegacyFeatureAdapter

**Files:**
- Create: `include/core/AppKernel.h`
- Create: `src/core/AppKernel.cpp`
- Create: `include/core/NavigationController.h`
- Create: `src/core/NavigationController.cpp`
- Create: `include/features/LegacyFeatureAdapter.h`
- Create: `src/features/LegacyFeatureAdapter.cpp`
- Modify: `include/app/App.h`
- Modify: `src/app/App.cpp`
- Modify: `src/app/LegacyApp.cpp`
- Test: `test/native/test_navigation/test_main.cpp`

- [ ] **Step 1: 写导航失败测试**

测试菜单栈 push/pop、`Blocked` 保持当前菜单、`Completed` 返回父菜单、`Fault` 显示错误后可返回，以及任何结果不会直接调用另一个功能。

- [ ] **Step 2: 实现 LegacyFeatureAdapter**

适配器构造参数是四个函数指针：

```cpp
struct LegacyCallbacks {
    void (*setup)();
    void (*loop)();
    bool (*isExitRequested)();
    void (*cleanup)();
};
```

`begin()` 只调用一次 setup；`tick()` 调用一次 loop，不进入内部无限循环；`end()` 至多调用一次 cleanup。无法拆除内部阻塞循环的旧模块不得注册，必须先在对应迁移任务中改成单步 loop。

- [ ] **Step 3: 让 AppKernel 接管入口**

`App::setup()` 构造服务、目录和 Runner 后调用 `kernel.setup()`；`App::loop()` 只调用 `kernel.tick()`。保留 legacy 初始化辅助函数，但 `legacyLoop()` 不再成为顶层循环。

- [ ] **Step 4: 运行测试、双板构建与启动烟测**

Expected: native 导航测试通过；双板编译返回 0；两块实机可显示主菜单并返回。

- [ ] **Step 5: 提交**

```powershell
git add include/core src/core include/features/LegacyFeatureAdapter.h src/features/LegacyFeatureAdapter.cpp include/app src/app
git commit -m "refactor: route the application through AppKernel"
```

---

### Task 8: 建立 ResourceBroker 和作用域租约

**Files:**
- Create: `include/services/ResourceBroker.h`
- Create: `src/services/pure/ResourceBroker.cpp`
- Create: `include/services/IResourceMutex.h`
- Create: `src/services/pure/ResourceGraph.cpp`
- Test: `test/native/test_resource_broker/test_main.cpp`

- [ ] **Step 1: 写租约失败测试**

覆盖同一资源互斥、不同资源并行、同拥有者递归拒绝、超时返回、move 后只有一个 owner、析构释放和显式 recovery：

```cpp
auto first = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
TEST_ASSERT_TRUE(first.acquired());
auto second = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 0);
TEST_ASSERT_EQUAL(AcquireCode::Busy, second.code());
first.release();
TEST_ASSERT_TRUE(broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 0).acquired());
```

- [ ] **Step 2: 实现 move-only ResourceLease**

租约记录资源、拥有者、获取时刻和 broker 指针；析构调用一次释放。broker 记录超时、非法释放和恢复事件，不允许普通布尔 `busLocked` 作为同步原语。

- [ ] **Step 3: 接入 FreeRTOS mutex 后端**

主机测试使用 fake mutex；ESP32/C5 使用 FreeRTOS mutex。互斥实现只在平台适配文件中包含 FreeRTOS 头。

- [ ] **Step 4: 验证并提交**

Run: native broker 测试、双板构建。
Expected: 0 failure。

```powershell
git add include/services/ResourceBroker.h include/services/IResourceMutex.h src/services/pure test/native/test_resource_broker
git commit -m "feat: add scoped resource leases"
```

---

### Task 9: 建立共享 SPI、显示、触摸和 SD 总线服务

**Files:**
- Create: `include/services/SpiBusService.h`
- Create: `include/services/ISpiBackend.h`
- Create: `src/services/pure/SpiBusService.cpp`
- Create: `src/platform/ArduinoSpiBackend.cpp`
- Create: `include/services/DisplayService.h`
- Create: `src/services/DisplayService.cpp`
- Create: `include/services/TouchService.h`
- Create: `src/services/TouchService.cpp`
- Modify: `src/legacy/spi_manager.cpp`
- Modify: `include/spi_manager.h`
- Test: `test/native/test_spi_service/test_main.cpp`

- [ ] **Step 1: 写 SPI 调用序列失败测试**

fake SPI 记录以下精确序列：

```text
lock -> deselect_all -> begin_transaction -> select_target -> transfer
-> deselect_target -> end_transaction -> unlock
```

测试异常和提前返回仍执行后三步；测试 C5 Display、Touch、SD、CC1101、nRF24 均使用 `ResourceId::SharedSpi`。

- [ ] **Step 2: 实现短事务 API**

`SpiBusService::transact(DeviceId, callback, timeoutMs)` 依赖 `ISpiBackend`，负责租约和 CS；主机测试注入 fake，固件注入 `ArduinoSpiBackend`。callback 内禁止 delay/yield；debug 构建记录事务持续时间，超过设备预算返回诊断事件。

- [ ] **Step 3: 包装显示和触摸**

DisplayService 提供小区域批次渲染，TouchService 每次租约只读取一次采样。C5 两者使用同一 SPI 服务；经典 CYD 的独立触摸引脚仍通过服务接口，不让业务层知道总线差异。

- [ ] **Step 4: 将旧 spi_manager 变成兼容桥**

旧 `spiSelect/spiDeselect` 暂时委托新服务，并打印调用方诊断；禁止直接 `SPI.end()`。每完成一个功能迁移就减少兼容桥调用计数。

- [ ] **Step 5: 验证并提交**

Run: native SPI 测试、边界审计、双板构建、C5 显示+触摸连续 10 分钟烟测。
Expected: 无死锁、触摸失效或看门狗复位。

```powershell
git add include/services src/services/pure src/platform/ArduinoSpiBackend.cpp include/spi_manager.h src/legacy/spi_manager.cpp test/native/test_spi_service
git commit -m "feat: arbitrate display touch and shared SPI"
```

---

### Task 10: 建立独立 PeripheralRegistry 与驱动探测

**Files:**
- Create: `include/services/PeripheralRegistry.h`
- Create: `src/services/pure/PeripheralRegistry.cpp`
- Create: `include/drivers/IPeripheralDriver.h`
- Create: `include/drivers/Cc1101Driver.h`
- Create: `include/drivers/Nrf24Driver.h`
- Create: `include/drivers/Pn532I2cDriver.h`
- Create: `include/drivers/GpsDriver.h`
- Create: `src/drivers/Cc1101Driver.cpp`
- Create: `src/drivers/Nrf24Driver.cpp`
- Create: `src/drivers/Pn532I2cDriver.cpp`
- Create: `src/drivers/GpsDriver.cpp`
- Test: `test/native/test_peripheral_registry/test_main.cpp`

- [ ] **Step 1: 写按需探测失败测试**

覆盖初始 `Unverified`、成功 `Available`、无响应 `NotFound`、资源占用 `Busy`、总线异常 `Fault`、无路由 `Unsupported`、缓存过期后重探测。断言一个外设失败不改变其他外设状态。

- [ ] **Step 2: 实现注册表**

`ensureAvailable(PeripheralId)` 选择当前板候选路由、申请所需资源、调用一次无破坏 probe、缓存时间戳和诊断。启动阶段只注册描述符，不批量探测。

- [ ] **Step 3: 实现最小探测**

- CC1101：在 SPI 租约内读取 VERSION/PARTNUM，不发射。
- nRF24：读取 CONFIG/STATUS 并验证非全 0/全 1，不进入 TX。
- PN532：两块板统一走 I2C，读取 firmware version。
- GPS：在有限窗口内识别有效 NMEA 句；没有数据为 `NotFound`，UART 错误为 `Fault`。

- [ ] **Step 4: 零外设验证**

两块板断开所有可选外设启动；浏览完整菜单；依次进入依赖功能并确认提示包含设备、供电、拨码/接线和 Retry/Back。
Expected: 不复位、不隐藏菜单、不污染其他外设状态。

- [ ] **Step 5: 提交**

```powershell
git add include/services/PeripheralRegistry.h src/services/pure/PeripheralRegistry.cpp include/drivers src/drivers test/native/test_peripheral_registry
git commit -m "feat: detect optional peripherals at runtime"
```

---

### Task 11: 实现 SettingsService 与 VALHALLA/Blue Team PolicyService

**Files:**
- Create: `include/services/SettingsService.h`
- Create: `src/services/SettingsService.cpp`
- Create: `src/services/pure/SettingsRecord.cpp`
- Create: `include/core/PolicyService.h`
- Create: `src/core/PolicyService.cpp`
- Modify: `src/legacy/utils.cpp`
- Modify: `include/shared.h`
- Modify: `src/app/LegacyApp.inc`
- Test: `test/native/test_settings_policy/test_main.cpp`

- [ ] **Step 1: 写状态迁移与默认拒绝失败测试**

精确覆盖：

```text
missing record -> BlueTeam
bad CRC -> BlueTeam
unknown schema -> BlueTeam
legacy accepted=true, blue=false -> ValhallaAccepted
all other legacy pairs -> BlueTeam
decline or BOOT -> persisted BlueTeam
accept full disclaimer -> persisted ValhallaAccepted
```

并测试 Blue Team 允许 Passive/Defensive、阻止 Offensive；授权阻断发生在外设探测和资源申请之前。

- [ ] **Step 2: 实现版本化双记录**

`SettingsRecord` 包含 magic、schema、length、sequence、CRC 和设置 payload。写入非活动槽、读回校验成功后才提升 sequence。启动选择最后一个完整有效记录。

- [ ] **Step 3: 实现单一 PolicyMode**

```cpp
enum class PolicyMode : uint8_t { BlueTeam = 0, ValhallaAccepted = 1 };

class PolicyService {
public:
    Authorization authorize(const FeatureDescriptor&) const;
    bool acceptValhalla();
    bool enterBlueTeam();
    PolicyMode mode() const;
};
```

`acceptValhalla()` 只能由完整免责声明页面的 ACCEPT 事件调用；DECLINE、BOOT、写入失败均保持 BlueTeam。

- [ ] **Step 4: 清除重复菜单门禁和危险命名**

删除 `LegacyApp.inc` 各分支复制的 `isOffensiveAllowed()` 流程，入口统一交给 Runner。将旧数据擦除 `activateValhalla()` 重命名为 `confirmEmergencyDataWipe()`；没有独立长按确认时先从菜单移除，不得与拒绝免责声明关联。

- [ ] **Step 5: 主屏模式验收**

主屏持续显示 `VALHALLA` 或 `BLUE TEAM`。切换、重启、设置迁移后立即重绘。测试深链与功能交接无法绕过 PolicyService。

- [ ] **Step 6: 运行测试和提交**

Run: native policy 测试、边界审计、双板构建和两板重启验收。
Expected: 全部通过。

```powershell
git add include/services/SettingsService.h src/services/SettingsService.cpp src/services/pure/SettingsRecord.cpp include/core/PolicyService.h src/core/PolicyService.cpp src/legacy/utils.cpp include/shared.h src/app/LegacyApp.inc test/native/test_settings_policy
git commit -m "feat: enforce persistent VALHALLA safety policy"
```

---

### Task 12: 建立 StorageService、开放路径和凭据迁移

**Files:**
- Create: `include/services/StorageService.h`
- Create: `src/services/StorageService.cpp`
- Create: `include/services/StoragePaths.h`
- Create: `src/services/pure/PathPolicy.cpp`
- Create: `src/services/pure/CredentialsMigration.cpp`
- Modify: `src/legacy/loot_manager.cpp`
- Modify: `src/legacy/saved_captures.cpp`
- Modify: `src/legacy/wp_loot_viewer.cpp`
- Modify: `src/legacy/wifi_attacks.cpp`
- Test: `test/native/test_storage_contract/test_main.cpp`

- [x] **Step 1: 写路径和迁移失败测试**

测试 `/subghz`、`/eapol`、`/wardriving`、`/wp_loot`、`/loot`、`/creds.txt`、`/iot_recon.txt`、`/firmware` 精确映射；拒绝 `..`、反斜杠逃逸和超长路径。测试旧 `[PSK]` 捕获复制到 `/loot/credentials/` 后保留不确定输入。

- [x] **Step 2: 实现 StorageService**

服务拥有一次 SD 初始化；文件句柄离开作用域即关闭。写入流程固定为 temp -> flush -> size/hash verify -> atomic rename。目录缺失按合同返回空状态或按需创建；SD 缺失不阻止启动。

- [x] **Step 3: 迁移现有浏览器和 Captive Portal**

Loot、Saved Captures、WhisperPair Viewer 和 Captive Portal 只调用 StorageService。`/creds.txt` 变为只读 IoT Recon 输入；新捕获写入 `/loot/credentials/`。

- [x] **Step 4: 审计直接 SD 调用**

`scripts/audit_boundaries.ps1` 对已迁移文件发现 `SD.begin/end` 或 `SPI.begin/end` 即失败。

- [x] **Step 5: 验证并提交**

运行 native 存储测试；在无 SD、空 FAT32 SD、只读/损坏文件、断电中断模拟四种条件验收；双板构建通过。

```powershell
git add include/services/StorageService.h include/services/StoragePaths.h src/services src/legacy/loot_manager.cpp src/legacy/saved_captures.cpp src/legacy/wp_loot_viewer.cpp src/legacy/wifi_attacks.cpp test/native/test_storage_contract scripts/audit_boundaries.ps1
git commit -m "feat: centralize storage and credential migration"
```

---

### Task 13: 加固固件更新、Safe Mode 与启动健康记录

**Files:**
- Create: `include/services/FirmwareImageValidator.h`
- Create: `src/services/pure/FirmwareImageValidator.cpp`
- Create: `include/core/BootHealthService.h`
- Create: `src/core/BootHealthService.cpp`
- Modify: `src/legacy/firmware_update.cpp`
- Modify: `include/firmware_update.h`
- Test: `test/native/test_firmware_validator/test_main.cpp`

- [x] **Step 1: 写镜像验证失败测试**

使用小型 fixture 覆盖 bad magic、截断 header、错误 chip id、segment 越界、声明长度不符、分区不足和正确 ESP32/ESP32-C5 镜像。文件名变化不得改变验证结论。

- [x] **Step 2: 实现解析器**

解析 ESP image header 和 app descriptor，返回 `ImageValidationResult`，包含目标芯片、镜像长度、版本和错误码。更新流程必须在 `Update.begin()` 前完成全部验证。

- [x] **Step 3: 实现启动健康与 Safe Mode**

启动时增加 pending 标记；主菜单稳定后提交 healthy。连续异常启动进入 Safe Mode，只初始化显示、触摸、Settings、Policy 和 Diagnostics；不自动恢复上次功能、不删除 SD。

- [ ] **Step 4: 板上验证**（经典 ESP32 已验证；NM-CYD-C5 受 PlatformIO 依赖解包阻塞）

在两板分别尝试本板有效镜像、另一芯片镜像、截断镜像和超分区镜像。
Expected: 仅本板有效镜像可进入最终确认；其余在写 Flash 前被拒绝。

- [x] **Step 5: 提交**

```powershell
git add include/services/FirmwareImageValidator.h src/services/pure/FirmwareImageValidator.cpp include/core/BootHealthService.h src/core/BootHealthService.cpp src/legacy/firmware_update.cpp include/firmware_update.h test/native/test_firmware_validator
git commit -m "feat: validate firmware targets and recover safe boot"
```

---

### Task 14: 迁移现有菜单和功能类别到 Registry/Runner

**Files:**
- Modify: `src/features/FeatureCatalog.cpp`
- Modify: `src/app/LegacyApp.inc`
- Modify: `src/legacy/wifi_attacks.cpp`
- Modify: `src/legacy/bluetooth_attacks.cpp`
- Modify: `src/legacy/nrf24_attacks.cpp`
- Modify: `src/legacy/subghz_attacks.cpp`
- Modify: `src/legacy/rfid_attacks.cpp`
- Modify: `src/legacy/jam_detect.cpp`
- Modify: `src/legacy/eapol_capture.cpp`
- Modify: `src/legacy/karma_attack.cpp`
- Modify: `src/legacy/wardriving.cpp`
- Modify: `src/legacy/iot_recon.cpp`
- Modify: `src/legacy/serial_monitor.cpp`
- Modify: `src/legacy/gps_module.cpp`
- Modify: `src/legacy/radio_test.cpp`
- Test: `test/native/test_catalog_lifecycle/test_main.cpp`

- [ ] **Step 1: 建立精确迁移清单测试**

测试要求以下现有功能均具有非空 factory，且每项风险和外设依赖与设计一致：WiFi 8 项、现有 Bluetooth 项、nRF24 6 项、SubGHz 5 项、RFID 5 项、Jam Detect 4 项、SIGINT 6 项、Tools 5 项和 Settings 9 项。

- [ ] **Step 2: 按低风险到高风险分批适配**

按提交内顺序迁移：

1. Settings、About、Touch Calibrate、GPS、Serial Monitor。
2. WiFi Scanner、Packet Monitor、BLE Scanner、RFID Reader、Saved Captures、Loot。
3. WiFi/SIGINT 剩余项。
4. nRF24、SubGHz、Jam Detect。
5. Bluetooth/RFID 剩余项。

每个 legacy loop 必须变成单次有界工作片；每批运行进入、退出、重复进入三次测试。

- [ ] **Step 3: 替换直接资源访问**

迁移到该批次时同步替换其 `SPI/SD/EEPROM`、底层射频库和固定 GPIO 访问。nRF24、CC1101、PN532 分别只通过独立 driver；GPS 只通过 UART service；显示与触摸只通过 UI 服务。

- [ ] **Step 4: Radio Test 改为正式诊断入口**

Radio Test 依次调用各 `DiagnosticProvider`；不存在设备显示 NotFound，不重置 SPI，不假装 HAT 整体存在。

- [ ] **Step 5: 每批验证**

每批运行 native catalog/lifecycle 测试、边界审计、双板构建、零外设启动和至少一项有外设板测。任何批次失败不得继续下一批。

- [ ] **Step 6: 提交**

按上述五批分别提交，消息格式：

```text
refactor: migrate settings and tools to feature runner
refactor: migrate passive feature lifecycles
refactor: migrate wifi and sigint lifecycles
refactor: migrate spi radio feature lifecycles
refactor: migrate bluetooth and rfid lifecycles
```

---

### Task 15: 补齐 SubGHz 与 NTAG README 功能

**Files:**
- Create: `include/features/subghz/SubFileParser.h`
- Create: `src/features/subghz/SubFileParser.cpp`
- Create: `src/features/subghz/SubReadFeature.cpp`
- Create: `src/features/subghz/TeslaChargeFeature.cpp`
- Create: `include/features/rfid/NtagModel.h`
- Create: `src/features/rfid/NtagToolsFeature.cpp`
- Test: `test/native/test_sub_file_parser/test_main.cpp`
- Test: `test/native/test_ntag_model/test_main.cpp`

- [ ] **Step 1: 写 .sub 解析失败测试**

fixture 覆盖 RAW、Princeton、CAME、Nice、未知协议、超长 pulse、缺字段、嵌套目录和空目录。解析器只生成不可发射的数据模型；发送必须经 Offensive 门禁和 CC1101 preflight。

- [ ] **Step 2: 实现 .Sub Read**

浏览 `/subghz`，显示预览、频率、协议和数据长度；损坏文件显示 Fault 并可返回。发送时重新授权、探测 CC1101、申请 SPI/radio 租约并提供可靠 Stop。

- [ ] **Step 3: 实现 Tesla Charge 的受控功能壳**

US/EU/BOTH 模式使用经过测试向量验证的固定 payload；没有可审计测试向量时 factory 返回 Unsupported，发布门槛保持失败，不使用推测数据。

- [ ] **Step 4: 写 NTAG 模型测试**

覆盖 NTAG213/215/216、Ultralight EV1 容量、用户页范围、配置页、锁定位不可逆警告、NDEF URI/Text/多记录编码和密码设置/移除。

- [ ] **Step 5: 实现 NTAG Tools**

所有 I/O 经 PN532-I2C driver。永久锁定需要单独二次确认；写入前显示页范围和不可逆影响；验证失败不继续后续页。

- [ ] **Step 6: 验证并提交**

运行 parser/model native 测试、双板构建、真实 CC1101/PN532 板测和故障注入。

```powershell
git add include/features/subghz src/features/subghz include/features/rfid src/features/rfid test/native/test_sub_file_parser test/native/test_ntag_model
git commit -m "feat: add sub file reader and NTAG tools"
```

---

### Task 16: 补齐 BLE Predator、Airoha、Flock、Drone 与 Skimmer

**Files:**
- Create: `include/features/ble/BlePredatorModel.h`
- Create: `src/features/ble/BlePredatorFeature.cpp`
- Create: `src/features/ble/AirohaRaceFeature.cpp`
- Create: `include/features/sigint/FlockFingerprint.h`
- Create: `src/features/sigint/FlockYouFeature.cpp`
- Create: `include/features/drone/RemoteIdParser.h`
- Create: `src/features/drone/DroneDetectFeature.cpp`
- Create: `src/features/drone/RidSpoofFeature.cpp`
- Create: `src/features/drone/DroneJellyFeature.cpp`
- Create: `src/features/drone/DroneDeauthFeature.cpp`
- Create: `include/features/skimmer/SkimmerScore.h`
- Create: `src/features/skimmer/SkimmerHuntFeature.cpp`
- Create: `docs/protocol-evidence/airoha-race.md`
- Test: `test/native/test_ble_predator/test_main.cpp`
- Test: `test/native/test_flock_fingerprint/test_main.cpp`
- Test: `test/native/test_remote_id_parser/test_main.cpp`
- Test: `test/native/test_skimmer_score/test_main.cpp`

- [x] **Step 1: 实现 BLE Predator 三阶段状态机**

SCAN 只做威胁分类；RECON 枚举 GATT service/characteristic；HONEYPOT 克隆允许连接的广告/GATT 表面并将 WRITE 事件写入 `/wp_loot`。每次阶段变化释放上一阶段 BLE 资源，退出使用 `BLEDevice::deinit(false)`。

- [x] **Step 2: 建立 Airoha 证据门槛**

`airoha-race.md` 对 README 所列 CVE 逐项记录公开协议来源、输入帧、响应判定、测试向量和授权边界。只有证据和测试向量齐全的操作才能进入 feature；其他操作显示 Unsupported，并使 README 完成矩阵保持未通过。

- [x] **Step 3: 实现 Flock You 数据模型**

将 22 OUI、8 Raven GATT UUID、固件估计规则和可选 GPS 标记放入可测试数据表。测试命中、冲突、未知和无 GPS；输出写 `/loot`。

- [x] **Step 4: 实现 Remote ID 解析与 Drone Detect**

解析 ASTM F3411 OpenDroneID WiFi/BLE 消息，合并 SSID/BLE 指纹。经典 ESP32 可并行接收；C5 用 ExecutionPolicy 交替短采样，UI 每个预算周期获得 tick。

- [x] **Step 5: 实现三项 Drone 攻击入口**

RID Spoof、Drone Jelly、Drone Deauth 均声明 Offensive；Policy 授权必须早于 WiFi/nRF24 初始化。每项提供目标/模式确认、运行计数、显式停止和清理测试；Blue Team 深链测试必须返回 Blocked。

- [x] **Step 6: 实现 Skimmer 两阶段评分**

第一阶段按名称模式、OUI、服务 UUID、信号行为和连接特征输出可解释分数；第二阶段主动确认只对用户选中目标运行。保持 BETA，直到两板现场证据完成。

- [x] **Step 7: 验证和分项提交**

每个子功能单独运行 native fixture、双板构建、Blue Team 拦截和资源清理测试。按 BLE Predator、Airoha、Flock、Drone Detect、Drone active、Skimmer 六个提交交付，避免混合协议回归。

---

### Task 17: 添加独立 IR 与 RF433 扩展

**Files:**
- Create: `include/drivers/IrTxDriver.h`
- Create: `include/drivers/IrRxDriver.h`
- Create: `include/drivers/Rf433TxDriver.h`
- Create: `include/drivers/Rf433RxDriver.h`
- Create: `src/drivers/IrTxDriver.cpp`
- Create: `src/drivers/IrRxDriver.cpp`
- Create: `src/drivers/Rf433TxDriver.cpp`
- Create: `src/drivers/Rf433RxDriver.cpp`
- Create: `src/features/ir/IrFeature.cpp`
- Create: `src/features/rf433/Rf433Feature.cpp`
- Test: `test/native/test_ir_rf433_models/test_main.cpp`

- [x] **Step 1: 写独立能力和互斥测试**

断言 Tx/Rx 是四个独立 PeripheralId；仅连接 Tx 时发送可用、接收 NotFound；仅连接 Rx 时相反。IR 与 RF433 共用 GPIO 路由属于同一 mutexGroup，不能同时获取。

- [x] **Step 2: 实现 IR**

最小功能：接收、协议/原始脉冲显示、保存、加载、发送。无芯片 ID 的线路不以浮空读取宣称 Available；使用受控 loopback 或人工确认。

- [x] **Step 3: 实现 RF433**

最小功能：OOK/ASK 接收、编码信息显示、保存、加载、发送。不得与 CC1101 driver 合并；文件写入 `/loot` 的扩展分类。

- [x] **Step 4: 验证并提交**

Run: native 模型测试、双板构建、Tx-only、Rx-only、loopback、无设备和互斥冲突板测。

```powershell
git add include/drivers src/drivers src/features/ir src/features/rf433 test/native/test_ir_rf433_models
git commit -m "feat: add independent IR and RF433 peripherals"
```

---

### Task 18: 完成边界审计、C5 SPI 压力、验收证据和发布包

**Files:**
- Modify: `scripts/audit_boundaries.ps1`
- Modify: `scripts/build_matrix.ps1`
- Create: `scripts/verify_feature_contract.ps1`
- Create: `docs/acceptance/README.md`
- Create: `docs/acceptance/templates/board-smoke.md`
- Create: `docs/acceptance/templates/peripheral-diagnostic.md`
- Create: `docs/acceptance/templates/c5-spi-stress.md`
- Create: `docs/acceptance/templates/release-manifest.md`
- Modify: `README.md`

- [ ] **Step 1: 完成静态边界审计**

审计正式代码中：

- 旧板宏、`NMRF_HAT` 和 HAT 整体状态为零。
- `src/features` 不直接包含 SPI、SD、EEPROM、TFT_eSPI 或底层射频库。
- `xTaskCreatePinnedToCore` 只存在于经典 ESP32 ExecutionPolicy。
- `BLEDevice::deinit(true)` 为零。
- 所有 Offensive descriptor 可追溯到 PolicyService 覆盖测试。

- [ ] **Step 2: 完成 README 功能合同脚本**

`verify_feature_contract.ps1` 从一个明确的 catalog dump 读取稳定 ID，逐项核对设计第 8 节功能、factory 状态、风险、外设依赖和验收证据文件。任何 Unsupported、缺证据或菜单缺项使脚本返回 1。

- [ ] **Step 3: 运行完整自动验证**

Run:

```powershell
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe test -e native
powershell -ExecutionPolicy Bypass -File scripts/verify_targets.ps1
powershell -ExecutionPolicy Bypass -File scripts/audit_boundaries.ps1
powershell -ExecutionPolicy Bypass -File scripts/verify_feature_contract.ps1
powershell -ExecutionPolicy Bypass -File scripts/build_matrix.ps1
```

Expected: 全部返回 0，build report 只含两块板并记录 commit、平台/Core、依赖版本、RAM、Flash、firmware size 和 SHA-256。

- [ ] **Step 4: 运行两板零外设与外设矩阵**

每块板记录：

- 零外设启动、完整菜单、Settings/About、VALHALLA/Blue Team 重启。
- SD、CC1101、nRF24、PN532-I2C、GPS、IR、RF433 独立探测。
- NotFound、Busy、Fault、Unsupported、退出和重复进入。
- 每个 README 功能的行为证据。

- [ ] **Step 5: 运行 C5 SPI 压力**

至少持续 60 分钟：显示局部刷新、触摸轮询、SD 文件循环、CC1101 或 nRF24 采样交替运行；注入租约超时、拔除外设和中途停止。记录固件 SHA-256、接线/拨码、堆、栈水位、看门狗、错误 CS、事务超时和最终结果。

- [ ] **Step 6: 生成发布包**

分别生成 ESP32-2432S028 与 NM-CYD-C5 包，包含 firmware、bootloader、partition、刷写说明、manifest 和 SHA-256。只有 Task 18 全部通过才允许标记 Stable；否则使用 Development Snapshot、Alpha、Beta 或 RC 的真实状态。

- [ ] **Step 7: 最终提交**

```powershell
git add scripts docs/acceptance README.md
git commit -m "release: add dual-board 3.8.1 acceptance evidence"
```

---

## 3. 每任务共同验证模板

每个任务提交前运行与其相关的 native test，并至少执行：

```powershell
git -c safe.directory=E:/github/HaleHound-CYD diff --check
powershell -ExecutionPolicy Bypass -File scripts/verify_targets.ps1
powershell -ExecutionPolicy Bypass -File scripts/audit_boundaries.ps1
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e esp32-cyd
C:\Users\RockBase-007\.platformio\penv\Scripts\pio.exe run -e nm-cyd-c5
```

对需要硬件的任务，在 `docs/acceptance/` 保存板型、外设型号、接线/拨码、固件 SHA-256、操作步骤、期望、实际和日志路径。没有证据的功能不能标为通过。

## 4. 计划完成判据

实施计划只有在以下条件全部满足时结束：

1. Task 1-18 的复选框全部完成并有对应提交。
2. `LegacyFeatureAdapter` 无注册者，`legacySetup/legacyLoop` 不再被 App 调用。
3. 正式源码通过资源边界审计。
4. README 功能合同脚本无缺项、无 Unsupported、无缺失证据。
5. 两块板零外设启动和独立外设矩阵通过。
6. C5 共享 SPI 压力通过。
7. VALHALLA/Blue Team 默认拒绝、持久化、主屏标识和全部入口覆盖通过。
8. 双板发布包和 manifest 可复现，且不声称移植不存在的上游 3.8.1 源码。
