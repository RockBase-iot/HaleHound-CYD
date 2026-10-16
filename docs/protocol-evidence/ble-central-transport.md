# BLE Central 传输层（`Esp32BleCentral`）Evidence Gate

Status: `VERIFIED`（逐条对照**本机已安装**的 framework 头文件，零推断）

Scope: `include/drivers/Esp32BleCentral.h`、`src/drivers/Esp32BleCentral.cpp` ——
`IBleCentral` 的唯一物理实现（`FakeBleCentral` 只在宿主测试里存在）。

---

## 1. 为什么需要证据门

第一版实现是**照着 Bluedroid 写的**，理由是「仓库里的 legacy BLE 屏
（`src/legacy/bluetooth_attacks.cpp`）用的是 Bluedroid，两套 host 不能共存，所以
transport 必须跟 legacy 对齐」。

这个理由**是错的**，而且真机编译一次就把错暴露了：

```
Esp32BleCentral.cpp:38: error: 'esp_bt_uuid_t' was not declared in this scope
Esp32BleCentral.cpp:42: error: 'ESP_UUID_LEN_16' was not declared
```

`esp_bt_uuid_t` / `ESP_UUID_LEN_*` 是 **Bluedroid 专有**的。也就是说：**主板
`nm-cyd-c5` 根本不是 Bluedroid**，而 legacy 那份代码能在两个 env 上都编过，
不是因为它选了 Bluedroid，而是因为 `Arduino BLE` 这个**库**在 3.x 里同时支持两套
host —— legacy 用的是库，不是 host。

**教训**：BT host 不能从「谁在用 `BLEDevice`」推断，只能从 **`sdkconfig.h`** 读。

---

## 2. 两个 env 各自用的是哪套 host（一手证据）

| env | 板子 | Arduino 版本 | **BT host** | 证据路径 |
|---|---|---|---|---|
| `nm-cyd-c5`（`default_envs`） | ESP32-C5 | **3.3.6** | **NimBLE** | `.pio/packages/framework-arduinoespressif32-libs/esp32c5/qio_qspi/include/sdkconfig.h` |
| `esp32-cyd` | ESP32 | **2.0.17**（`3.20017.241212+sha.dcc1105b`） | **Bluedroid** | `.pio/packages/framework-arduinoespressif32/tools/sdk/esp32/dio_qspi/include/sdkconfig.h` |

```bash
# nm-cyd-c5 —— 只有 NimBLE，没有 Bluedroid
grep -E "CONFIG_BT_(NIMBLE|BLUEDROID)_ENABLED" \
  .pio/packages/framework-arduinoespressif32-libs/esp32c5/qio_qspi/include/sdkconfig.h
#   #define CONFIG_BT_ENABLED 1
#   #define CONFIG_BT_NIMBLE_ENABLED 1          <- 有
#   #define CONFIG_BT_NIMBLE_ROLE_CENTRAL 1     <- central 角色开着
#   （没有 CONFIG_BLUEDROID_ENABLED）

# esp32-cyd
grep -E "CONFIG_(BT_)?BLUEDROID_ENABLED" \
  .pio/packages/framework-arduinoespressif32/tools/sdk/esp32/dio_qspi/include/sdkconfig.h
#   #define CONFIG_BT_BLUEDROID_ENABLED 1
#   #define CONFIG_BLUEDROID_ENABLED CONFIG_BT_BLUEDROID_ENABLED
```

> **`nm-cyd-c5` 用 NimBLE 不是选出来的，是唯一选项**：C5 的 lib 包
> （`framework-arduinoespressif32-libs`）就是按 NimBLE 构建的，里面没有 Bluedroid。
> 所以 header 里那句「Bluedroid 更重，是代价」对 C5 不成立 —— C5 拿不到这个选择，
> 这是 `FeatureSwitchLedger`（B5）要如实记的属性，不是可优化项。

---

## 3. 两个 host 的 API 差异（逐条对照头文件）

`Arduino BLE` 库对上是一套 API，但下面这些**确实不同**。全部读自上表两个路径。

### 3.1 UUID 内部结构

| | NimBLE 3.3.6 | Bluedroid 3.x | Bluedroid 2.0.17 |
|---|---|---|---|
| 类型 | `ble_uuid_any_t` | `esp_bt_uuid_t` | `esp_bt_uuid_t` |
| `getNative()` 返回 | `const ble_uuid_any_t*` | `esp_bt_uuid_t*` | `esp_bt_uuid_t*` |
| 宽度字段 | `u.type`（`BLE_UUID_TYPE_16/32/128`） | `len`（`ESP_UUID_LEN_*`） | `len` |
| 库提供的别名宏 | 有（`UUID_LEN` / `UUID_VAL_16/32/128`、`BLE_UUID_*_BITS`） | 有 | **没有** |

证据：`.../BLE/src/BLEUUID.h`（3.3.6 与 2.0.17 两版）：

```cpp
// 3.3.6 BLEUUID.h:30-54 —— 别名宏按 host 二选一
#if defined(CONFIG_BLUEDROID_ENABLED)
#define UUID_LEN(s)       s.len
#define UUID_VAL_128(s)   s.uuid.uuid128
#endif
#if defined(CONFIG_NIMBLE_ENABLED)
#define UUID_LEN(s)       s.u.type
#define UUID_VAL_128(s)   s.u128.value
#endif
// 2.0.17 BLEUUID.h:23,29,35 —— 没有别名宏，只有裸 esp_bt_uuid_t
//   BLEUUID(esp_bt_uuid_t uuid);   esp_bt_uuid_t* getNative();   esp_bt_uuid_t m_uuid;
```

**结论**：normalise 层必须同时覆盖「有宏」和「没宏」两种，见 `.cpp` 顶部的
`HH_UUID_LEN` / `HH_UUID_VAL_*` 三套定义。

### 3.2 地址字节序（**静默 bug 陷阱**）

| | NimBLE | Bluedroid |
|---|---|---|
| `BLEAddress` 内部 `m_address` | **LSB first（反序）** | **打印序** |
| `getNative()` | `uint8_t*` | 3.x `uint8_t*` / 2.0.17 `esp_bd_addr_t*` |
| 构造函数 | `BLEAddress(uint8_t[6], uint8_t type)`（**内部 `std::reverse_copy`**） | `BLEAddress(esp_bd_addr_t)` |

证据：`.../BLE/src/BLEAddress.cpp`（3.3.6 NimBLE 分支，原文自带警告注释）：

```cpp
/***** NOTE: NimBLE address bytes are in INVERSE ORDER! *****/
String BLEAddress::toString() const {
#if defined(CONFIG_NIMBLE_ENABLED)
  snprintf(res, size, "%02x:%02x:%02x:%02x:%02x:%02x",
           m_address[5], m_address[4], m_address[3], m_address[2], m_address[1], m_address[0]);
#endif   // <- 打印时从 index 5 倒着取，这就是「m_address 是反序」的证明
}
BLEAddress::BLEAddress(uint8_t address[ESP_BD_ADDR_LEN], uint8_t type) {
  std::reverse_copy(address, address + sizeof(m_address), m_address);   // <- 输入按打印序，内部反转
  m_addrType = type;
}
```

seam（`IBleCentral.h`）规定 `bytes[0]` 是打印序的 MSB。因此：

- **`connect()`**：seam 的打印序 → 两个 host 的构造函数都接受打印序（NimBLE 内部反转），
  所以直拷即可。
- **`scanResults()`**：`getNative()` 出来的是**设备视角的原始字节**，NimBLE 是反序，
  必须**反转**后才能进 seam。

> 一个直拷 `memcpy` 在 C5 上会把**每一个地址的六个字节全部交换**。地址全错等于连不上，
> 或者更糟：连到别的设备。这是本文件里唯一「编得过但必错」的地方。

### 3.3 地址类型常量

`BLE_ADDR_TYPE_PUBLIC` / `BLE_ADDR_TYPE_RANDOM` 是 **Bluedroid 专有**
（来自 `esp_gap_ble_api.h` 的 `esp_ble_addr_type_t`）。NimBLE 侧是
`BLE_ADDR_PUBLIC` / `BLE_ADDR_RANDOM`（`nimble/ble.h:281-284`，值为 `0x00..0x03`）。

```bash
grep -rn "define BLE_ADDR_TYPE_PUBLIC" .pio/ 2>/dev/null   # 空 —— 不在 NimBLE 的 include 树里
grep -n "BLE_ADDR_PUBLIC " .pio/packages/framework-arduinoespressif32-libs/esp32c5/include/bt/host/nimble/nimble/nimble/include/nimble/ble.h
```

### 3.4 三个方法签名的漂移

| 方法 | 3.3.6（NimBLE / Bluedroid） | 2.0.17（Bluedroid） |
|---|---|---|
| `BLEClient::connect` | `connect(BLEAddress, uint8_t type = 0, uint32_t timeoutMS)` | `connect(BLEAddress, esp_ble_addr_type_t type = BLE_ADDR_TYPE_PUBLIC)`（**无 timeout**） |
| `BLERemoteCharacteristic::writeValue` | 返回 `bool` | 返回 **`void`** |
| `BLERemoteCharacteristic::readValue` | 返回 `String` | 返回 **`std::string`** |
| `BLEDevice::init` | 返回 `bool` | 返回 **`void`** |
| `BLEScan::stop` | 返回 `bool` | 返回 **`void`** |
| `BLEScan::isScanning` | **仅 NimBLE** 有（`BLEScan.h` 里在 `#if defined(CONFIG_NIMBLE_ENABLED)` 内） | 无 |
| `BLEAdvertisedDevice::isConnectable` | 有 | **无** |

> `writeValue` 的 `bool` → `void` 是**语义降级**：2.0.17 上 peer 侧拒绝和 host 侧故障
> 无法区分。实现里如实记成「链路还活着就是 Ok」，而不是编一个 `NotPermitted` ——
> 编了就把「断链」伪装成「特征受保护」，正是 `BleStatus` 存在的意义所在。

### 3.5 一个**编译期**陷阱：宏参数名撞结构体成员名

NimBLE 的 `ble_uuid_any_t` 里，宽度字段叫 `u`。所以

```cpp
#define HH_UUID_LEN(u) ((u).u.type)     // 错
```

展开 `HH_UUID_LEN(*native)` 时**成员名 `.u` 也被替换**，得到
`((*native).*native.type)`：

```
error: expected unqualified-id before '*' token
error: request for member 'type' in 'native', which is of pointer type 'const ble_uuid_any_t*'
```

修法是参数改名：`#define HH_UUID_LEN(x) ((x).u.type)`。

---

## 4. 校验方式（可复现）

`pio run` 在本沙箱结构性不可用（包管理器要删 >50 文件即撞守卫），所以用
`build-reports/syntax_env_check.py` 直接驱动**真 toolchain + 真 define**：

```bash
# 主板：ESP32-C5 / Arduino 3.3.6 / NimBLE
build-reports/hostenv/Scripts/python.exe build-reports/syntax_env_check.py \
    src/drivers/Esp32BleCentral.cpp --env nm-cyd-c5
# 副板：ESP32 / Arduino 2.0.17 / Bluedroid
build-reports/hostenv/Scripts/python.exe build-reports/syntax_env_check.py \
    src/drivers/Esp32BleCentral.cpp --env esp32-cyd
```

结果（本分支）：

| env | 结果 |
|---|---|
| `nm-cyd-c5` | **PASS**，全量 `13/13 clean` |
| `esp32-cyd` | **PASS**，全量 `13/13 clean`（checker 本轮已修，见下） |
| 宿主 `!ARDUINO` 分支 | **PASS**（`python -m ziglang c++ -std=gnu++17 -Iinclude -c`） |

**`esp32-cyd` 的 checker 本轮被修好**：原来它对这个 env 的**任何** TU 都报
`'assert' was not declared`（`soc/spinlock.h` 直接用 `assert()` 却不 include
`<assert.h>`，真构建靠自己的 forced include 蒙对），导致基线文件也 FAIL，
无法区分回归与工具假象。`prepare()` 现在把 toolchain 自己的
`xtensa-esp32-elf/include` 连同 `assert.h` / `errno.h` 前置，基线恢复 PASS。

---

## 5. 本 gate **没有**覆盖什么（如实声明）

1. **链接**：`-fsyntax-only` 不链接。符号级校验走另一条（xtensa `ld -r` + 符号审计）。
2. **真机行为**：`getNative()` 的字节序结论来自头文件与库实现**读**出来的，不是抓包验证的。
   地址反转这条属于「读码确定、待真机复验」，已列在 `REMAINING_WORK.md` 的 `X2`。
3. **NimBLE 的 live 扫描前提**：`BLEScan::start()` 非阻塞、`wantDuplicates=true`
   （→ `setDuplicateFilter(false)`）时 `onResult` 才**逐条实时**回调，否则只在
   `DISC_COMPLETE` 时补发。证据：`.../BLE/src/BLEScan.cpp`（NimBLE 分支）
   第 672-684 行逐条 `onResult`、第 700-706 行在扫描结束时补发未发过的。
   这一条决定了 `scanResults()` 的 drain 语义成立与否，属于**必须在真机上确认**的项。

---

## 6. 校验器自身的缺口（2026-10-16 补上）—— 它掩盖了一个真实缺陷

做 `U2` 接线时发现 `esp32-cyd` 的 flag 列表里**没有 `ARDUINO`**。

后果不是「报错难看」，而是**静默**：本文件主角 `src/drivers/Esp32BleCentral.cpp`
全文由 `#if defined(ARDUINO) && (CONFIG_NIMBLE_ENABLED || CONFIG_BLUEDROID_ENABLED)`
守卫，`ARDUINO` 未定义 ⟹ 整段被预处理掉 ⟹ `-fsyntax-only` 报 **PASS**。
也就是说 §4 表格里那一行「`esp32-cyd` → `Esp32BleCentral.cpp` PASS」，
当时**一行代码都没编译**。

补上 define 之后立刻暴露的真实缺陷：

```
src/drivers/Esp32BleCentral.cpp:241:10: error: 'BLEDevice' has not been declared
```

**根因**：`include/drivers/Esp32BleCentral.h` 的 Arduino include 段里没有
`<BLEDevice.h>`。NimBLE(3.3.6) 那条路径靠其它 BLE 头**传递引入**到 `BLEDevice`，
所以 C5 编得过；Bluedroid 2.0.17 的 `libraries/BLE/src/BLEDevice.h` 把
`class BLEDevice` 放在自己的 `#if defined(CONFIG_BLUEDROID_ENABLED)` 里，
没有任何别的头会把它带进来。已修，并把这段因果写进头文件注释。

补上的宏（值都取自装好的框架文件，不是推断）：

| 宏 | 值 | 出处 |
|---|---|---|
| `ARDUINO` | `20017` | `package.json` = `3.20017.241212`，`esp_arduino_version.h` MAJOR 2 |
| `ARDUINO_ARCH_ESP32` | — | 框架 `platform.txt` 的 `{build.arch}` |
| `ESP32` | `ESP32` | `platform.txt` 的 `-DESP32=ESP32`；libdeps 的 RF24 用 `defined(ESP32)` 判断架构 |

**这条教训比缺陷本身重要**：`-fsyntax-only` 无法区分「编过」和「被预处理掉了」。
绿灯之前要先确认那个 TU 真的被编译。

## 7. 结论修订（相对 §4）

| 项 | §4 当时 | 现在 |
|---|---|---|
| `esp32-cyd` → `Esp32BleCentral.cpp` | PASS（**空**） | PASS（**真的编译了**，且修掉一个缺陷后） |
| `esp32-cyd` 基线 | 2/2 因 assert/errno 假象而失败 | 2/2 PASS |
