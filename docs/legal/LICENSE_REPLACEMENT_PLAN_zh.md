# 依赖替换实施方案 — A 路线（`nm-cyd-c5`）

| 项 | 值 |
|---|---|
| 定位 | BOSS 已选定 **D1 = A 路线（替换依赖，使二进制可自由分发）**；本文是该路线的可执行方案 |
| 依据 | `docs/legal/COMPLIANCE_PLAN_zh.md`（许可链 · 商标 · 依赖审计）、`THIRD_PARTY_NOTICES.md`、`build-reports/license-audit.json` |
| 复跑命令 | `python build-reports/audit_licenses.py`（有阻断项时退出码 **1**） |
| 状态 | 方案已定，**代码未改** —— 本文不改变当前审计结论，只把替换拆到可验收 |

---

## §0 结论：是**三项**替换，不是两项

`TODO.md` 的 `C1` 原文只把 **RF24** 与 **arduinoFFT** 列为待替换项。这是**不充分的**：审计里 `as built`
不可分发有**两类**原因，换掉其中一类，二进制依然不可分发。

| # | 库 | 许可 | 分类 | 为什么单独就足以阻断 |
|---|---|---|---|---|
| 1 | `nrf24/RF24@1.4.11` | `GPL-2.0-only` | strong copyleft | 与 GPL-3.0 **互不相容** |
| 2 | `kosme/arduinoFFT@2.0.4` | `GPL-3.0` | strong copyleft | 同上（`only` vs `only` 无法合并） |
| 3 | `t-vk/ESP32 BLE Keyboard@0.3.2` | **无任何授权** | no-grant | 编进固件 = **无许可再分发** |

> 只做 1+2：`strong copyleft` 归零，但 `no grant at all: 1` 仍在 ⟹ 退出码仍是 1。
> 只做 3：同理反向。
> **`C1` 的验收标准因此要改**：不能只看 `strong copyleft` 计数，必须两类**同时**归零
> （已在新脚本的退出码注释里写明为何要键在 `verdicts` 而不是 copyleft 计数上）。

另有 4 项**不阻断**但各有义务，不在 A 路线的替换范围内，单列在 §6：

| 库 | 许可 | 义务 |
|---|---|---|
| `rc-switch`、`TinyGPSPlus` | `LGPL-2.1-or-later` | 静态链接必须允许重新链接（`C7`） |
| `framework-arduinoespressif32`(+`-libs`)、ESP-IDF | `LGPL-2.1-or-later` | 同上 |
| `SmartRC-CC1101-Driver-Lib` | 自定义（`LicenseRef-SmartRC-bespoke-permissive`） | **要求指明来源**（已由 `NOTICE.md` / `THIRD_PARTY_NOTICES.md` 满足） |

---

## §1 三项的实测耦合面

全部为本轮实跑 `grep` 得出，不是估计。

| 目标 | 引入点 | 调用点 | 独占 API |
|---|---|---|---|
| RF24 | `include/nrf24_config.h:11` **唯一一处** `#include <RF24.h>` | `nrf24Radio.` 共 **160 处**，分布在 4 个文件 | **21 个方法** + 2 个枚举 |
| arduinoFFT | `src/legacy/subghz_attacks.cpp:15`、`src/legacy/wifi_attacks.cpp:20`（`include/subconfig.h:12` 是**无用** include） | 2 个文件 | **3 个方法 × 2 种签名风格** |
| ESP32 BLE Keyboard | `src/legacy/bluetooth_attacks.cpp:8851` **唯一一处** | **5 个方法**，单一功能（BLE Ducky） | `begin/end/isConnected/press/release/write` |

调用点明细：

- RF24 的 4 个文件：`src/legacy/nrf24_config.cpp`(53)、`src/legacy/nrf24_attacks.cpp`(59)、
  `src/legacy/bluetooth_attacks.cpp`(24)、`bluetooth_attacks.cpp`(24，**根目录副本，不在 `src_dir` 下，
  不参与编译** —— 见 §6.4)。
- `src_dir = src`（`platformio.ini:4`）⟹ 根目录那份不编译，但它会让所有 `grep` 计数翻倍，
  做替换统计时**必须先排除**，否则会误判面积。

---

## §2 替换 1：`arduinoFFT` → 自写 radix-2（最便宜，先做）

### §2.1 为什么自写而不是换库

候选与淘汰理由：

| 候选 | 许可 | 结论 |
|---|---|---|
| 自写 radix-2（迭代 Cooley-Tukey） | 自己 | **推荐** —— 零新依赖、保持 `double`、宿主可测 |
| ESP-DSP（`dsps_*`） | Apache-2.0 | 可用，但只支持 `float32`，且要把 IDF 组件依赖面铺进 `esp32-cyd`（2.0.17）那条老环境 |
| kissfft | BSD-3-Clause | 可用，但为 3 个方法引入一份 C 库不划算 |
| `ArduinoFFT` 降版到 MIT 时期 | —— | **不可行**：该库从未以 MIT 发布过 |

### §2.2 必须支持**两种**调用风格（这是最容易漏的一点）

两个使用文件写的是同一个库的**两套 API**（2.0.4 同时保留）：

```cpp
// src/legacy/subghz_attacks.cpp:196, 302-304 —— 数组随调用传入
static ArduinoFFT<double> FFTSUB = ArduinoFFT<double>();
FFTSUB.windowing(vRealSUB, FFT_SAMPLES_SUB, FFTWindow::Hamming, FFTDirection::Forward);
FFTSUB.compute(vRealSUB, vImagSUB, FFT_SAMPLES_SUB, FFTDirection::Forward);
FFTSUB.complexToMagnitude(vRealSUB, vImagSUB, FFT_SAMPLES_SUB);

// src/legacy/wifi_attacks.cpp:43, 171-173 —— 数组在构造时绑定
static ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFT_SAMPLES, samplingFrequency);
FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
FFT.compute(FFTDirection::Forward);
FFT.complexToMagnitude();
```

只实现一套会让另一个文件编译不过。shim 需提供：默认构造 + 4 参构造、`windowing` ×2、
`compute` ×2、`complexToMagnitude` ×2，外加上游头文件里声明过的 `FFTWindow` / `FFTDirection` 枚举。

### §2.3 交付与验收

| 项 | 内容 |
|---|---|
| 文件 | `include/hound_fft.h`（自写，MIT + 本仓著作权行） |
| 改动 | 2 个 `#include` 换名；删 `include/subconfig.h:12` 的无用 include；`platformio.ini` 去掉 `kosme/arduinoFFT@2.0.4` |
| 验收（可执行） | ① 新增宿主套件：同一输入（含全零、单频、Hamming 窗）下与 `arduinoFFT` 参考输出断言一致（容差按 `double` 定）② 两端 `-fsyntax-only` 绿 ③ `audit_licenses.py` 中 `arduinoFFT` 消失且 `strong copyleft` 由 2 降至 1 |
| 规模 | **S** |

> 先做它的理由：纯算法、无硬件依赖、宿主上可完全验证。它能把"替换流程本身"先跑通一遍，
> 且不依赖 `X1`/`X2`（沙箱外构建 / 真机）。

---

## §3 替换 2：`RF24` → RadioLib(MIT) + 本仓适配层

### §3.1 选型证据（一手）

`jgromes/RadioLib`：GitHub API 实测 `license.spdx_id = **MIT**`、**2614** stars，
`src/modules/nRF24/nRF24.{h,cpp}`（27 172 + 19 725 字节）⟹ **官方支持 nRF24L01**。

> 注意与"自写 clean-room"的区别：上游用自写 `nrf24_raw.h` 绕开 RF24，但**上游 v3.6.2 起是专有许可**，
> 那份代码**不可搬运**。自写同功能是合法的（协议是 Nordic 公开数据手册，独立实现不侵权），
> 但成本高得多 —— 见 §3.3。

### §3.2 API 覆盖度对照（21 个方法逐个核过）

| 本仓使用 | RadioLib 对应 | 结论 |
|---|---|---|
| `begin()` | `begin(ConfigFSK_t)` | ✅ 参数形态不同，适配器内完成 |
| `setPALevel()` | `setOutputPower(int8_t)` | ✅ 需档位映射 |
| `setDataRate()` | `setBitRate(float)` | ✅ |
| `setChannel()` / `getChannel()` | `setFrequency(float)` | ✅ 需换算，`getChannel` 自适应器记 |
| `setAutoAck()` | `setAutoAck(bool)` | ✅ 同名同义 |
| `setPayloadSize()` | —— | ⚠️ 无直接对应（走 `getPacketLength` 动态长度） |
| `setAddressWidth()` | `setAddressWidth(uint8_t)` | ✅ **含 2 字节**，promiscuous 的 `SETUP_AW=0x00` 技巧可用 |
| `openWritingPipe()` | `setTransmitPipe(const uint8_t*)` | ✅ |
| `write(buf,len)` | `transmit(data,len,addr)` | ✅ |
| `stopListening()` / `startListening()` | `standby()` / `startReceive()` | ✅ |
| `powerDown()` | `sleep()` | ✅ |
| `disableCRC()` | `setCrcFiltering(false)` | ✅ |
| `setCRCLength()` | —— | ⚠️ 只有开/关，无 1/2 字节选择 |
| `isChipConnected()` | `getStatus(mask)` | ✅ |
| `testRPD()` | `isCarrierDetected()` | ✅ |
| `isPVariant()` | —— | ⚠️ 无，需读寄存器 |
| `flush_tx()` | —— | ⚠️ 无 |
| **`startConstCarrier()` / `stopConstCarrier()`** | —— | ❌ **无** |

关键发现：`SPItransfer(uint8_t cmd, bool write, const uint8_t* out, uint8_t* in, uint8_t n)`
是 **public**（`nRF24.h:498`）⟹ 四处缺口（const carrier、CRC 长度、`isPVariant`、`flush_tx`）
都可以由**我们自己**按数据手册发命令字节补上，不必 fork RadioLib。

| 缺口 | 补法（我们自己写） |
|---|---|
| const carrier | `CONFIG` 置 `PLL_LOCK`/`CONT_WAVE` 位，`RF_SETUP` 置 `CONT_WAVE` + 载波电平；`stopConstCarrier` 复位 |
| `setCRCLength(0/1/2)` | 写 `CONFIG` 的 `CRCO` + `EN_CRC` |
| `isPVariant()` | 写 `SETUP_AW` 后回读，+ 变体支持 2/4 字节（这是公认的探测法） |
| `flush_tx()` | 发 `FLUSH_TX`(0xE1) 命令 |
| `setPayloadSize()` | 写 `RX_PW_Px`（接收管道）；发送侧用 `transmit` 的 `len` |

### §3.3 为什么不选"完全自写驱动"

按数据手册自写要覆盖 21 个方法 + `CE/CSN` 时序 + `STATUS` 轮询 + 3 档速率/4 档功率 + 管道寻址 +
`promiscuous` 的非法地址宽度技巧。这些**没有宿主可测的路径**（只能上机），而 `X1`/`X2` 都还没做
（沙箱结构性无法 `pio run`；真机未接）。RadioLib 的寄存器与时序已被广泛使用，把风险集中在
**§3.2 那 5 处自写补齐**上，是可接受得多的交换。

### §3.4 交付与验收

| 项 | 内容 |
|---|---|
| 文件 | `include/drivers/HoundNrf24.h` + `.cpp`（适配层，对外仍是 `nrf24_config.h` 那套函数名） |
| 改动 | `include/nrf24_config.h` 不再暴露 `RF24` 类型与 `rf24_pa_dbm_e` / `rf24_datarate_e`；4 个调用文件改用本仓枚举 |
| 验收（可执行） | ① `platformio.ini` 去掉 `nrf24/RF24@1.4.11` ② `audit_licenses.py` 中 `RF24` 消失、`strong copyleft` 由 1 降至 **0** ③ 两端 `-fsyntax-only` 绿 ④ **真机**：`Radio Test` 自检 + 2.4GHz 扫描有读数 + 载波模式可开可关（补 `X2`） |
| 规模 | **M**（其中"上机验证"是不可压缩的部分） |

---

## §4 替换 3：`ESP32 BLE Keyboard` → 自写 HID 层

### §4.1 它无授权，而且已经需要打补丁才能编译

- `library.properties` / `library.json` **无 `license` 字段**，包内无 `LICENSE`，源码头无许可声明；
  GitHub API 报 `spdx_id = None`。
- 本仓已在 `scripts/patch_ble_keyboard_arduino3.py` 里**改它的源码**（`BLEDevice::init(String(...))`）
  才能在 Arduino 3.x 下编译 ⟹ 该库实际上已停止维护，**这本身就是移除它的第二个理由**。
- 越界点 `T1` 也在这里：`src/legacy/bluetooth_attacks.cpp:9153`
  `new BleKeyboard("HaleHound KB", "HaleHound", 100)` —— **把商标广播给附近所有手机**。
  替换与 `C3` 改名是**同一次改动**，不要分两次做。

### §4.2 耦合面只有 5 个方法

`bleKb->` 的全部调用（`src/legacy/bluetooth_attacks.cpp`）：`isConnected()`、`press(uint8_t)`、
`release(uint8_t)`、`write(uint8_t)`、`begin()`、`end()`。没有 `print`、没有媒体键、没有电池服务
—— 就是标准 boot-keyboard 8 字节 report 的按下/释放 + 单字符写入。

### §4.3 两条环境走的是不同的 BT host（决定了实现形态）

**实测结论：NimBLE-Arduino 不提供 HID 支持。** `h2zero/NimBLE-Arduino`（Apache-2.0，1132 stars）
的 19 个示例目录里没有任何 HID 示例，README 全文无 `hid` / `keyboard` / `mouse`。⟹ HID over GATT
要**自己实现**（HID 是应用层 profile，不是 stack 功能）。

| env | BT host | HID 来源 |
|---|---|---|
| `esp32-cyd`（ESP32 / core 2.0.17） | Bluedroid | core 自带 `BLEHIDDevice`（LGPL-2.1-or-later，弱 copyleft，可静态链接） |
| `nm-cyd-c5`（C5 / core 3.3.6） | **NimBLE** | **无** —— 需自写 HID over GATT |

> host 判定依据是 `sdkconfig.h`，不是"谁调了 `BLEDevice::init()`"（`nm-cyd-c5` 的 lib 包就是 NimBLE 构建的，
> 拿不到 Bluedroid）。这条在 `REMAINING_WORK §0.6` 有完整取证。

自写要落的东西（规范公开，都是标准件）：HID Service `0x1812` + HID Information `0x2A4A` +
Report Map `0x2A4B` + HID Control Point `0x2A4C` + Report `0x2A4D` + Protocol Mode `0x2A4E`，
以及 boot-keyboard report descriptor 与键码映射。

### §4.4 交付与验收

| 项 | 内容 |
|---|---|
| 文件 | `include/drivers/IHoundBleHid.h`（抽象）+ 两条实现（Bluedroid / NimBLE） |
| 改动 | `platformio.ini` 去掉 `t-vk/ESP32 BLE Keyboard@0.3.2`；删 `scripts/patch_ble_keyboard_arduino3.py` 与 `extra_scripts` 里的那行；`src/legacy/bluetooth_attacks.cpp:8851/8902/9153` 换成本仓类型 |
| 验收（可执行） | ① `audit_licenses.py` 的 `no grant at all` 归零、**退出码 0** ② 两端语法检查绿 ③ 真机：BLE Ducky 能在被配对主机上打出按键；广播名**不含** `HaleHound`（与 `C3` 合验） |
| 规模 | **M–L**（NimBLE 侧自写是主要成本） |

---

## §5 顺序与里程碑

```
阶段 1  arduinoFFT  → 自写 hound_fft.h        S   宿主可证，不依赖上机
阶段 2  RF24        → RadioLib + 适配层       M   需真机（X2）
阶段 3  BLE Keyboard→ 自写 HID（含 T1 改名）   M–L  需真机 + 与 C3 合并
```

顺序理由：**先做能在宿主上验收的**。阶段 1 让"替换 → 审计计数下降 → 宿主测试全绿"这条链路
先跑通一次；阶段 2、3 都要等真机，若把它们放前面，整条路线会被 `X1`/`X2` 卡住。

每阶段完成时都要同时更新：
`platformio.ini` 的 `lib_deps`、`LICENSES/`（复跑 `collect_licenses.py`，删不用的、加新引入的）、
`THIRD_PARTY_NOTICES.md`、`build-reports/license-audit.json`、`TODO.md §9`。

**A 路线完成态**：`audit_licenses.py` **退出码 0**，且依赖表里**没有任何** `strong-copyleft` /
`no-grant` / `unidentified` 行。

---

## §6 不阻断但必须留着说的事

1. **`C7` 的重链接义务**（`rc-switch`、`TinyGPSPlus`、Arduino core、IDF 预编译库）。
   A 路线**不解决**它 —— 那是"如何履行"，不是"要不要替换"。若想把义务面压到最小，
   可以把 `rc-switch` / `TinyGPSPlus` 也换成自写实现（`C7` 的备选 ④）。
2. **`SmartRC` 的归属条件**（本轮已落地：分类改为 `bespoke-permissive`，`LICENSES/` 换成一手文本副本）。
   它**要求指明来源**。`NOTICE.md` 与 `THIRD_PARTY_NOTICES.md` 已经做了这件事，所以审计把它打成
   **NOTE 而不是 VERDICT**（不阻断，但不许静默）。**改动这两份文件时要保住这条引用。**
3. **`LGPL-2.1` vs `LGPL-2.1-or-later`（本轮修正）**。`rc-switch` 与 `TinyGPSPlus` 的头文件写的是
   "either version 2.1 of the License, or (at your option) any later version" ⟹ 是 **or-later**。
   审计原先把它们报成裸 `LGPL-2.1`（SPDX 里 deprecated，含义是 `-only`），把**用户可以升级**
   这件事说反了。已改为按措辞判版本，**且只对源文件头生效** —— 因为每份 GPL/LGPL 正文的
   "How to Apply" 附录里都有 "any later version"，拿它去套一份 licence 文件会把 RF24 的
   `GPL-2.0-only`（它自带 GPLv2 全文）误升级成 `-or-later`。两份 SPDX 2.1 文本**逐字节相同**
   （md5 `310c7c93cf5181c6b0cc8229a1f3c6f6`），所以这次改动只动了标识符与文件名，没动内容。
4. **根目录 `bluetooth_attacks.cpp` 是死副本**（与 `src/legacy/bluetooth_attacks.cpp` **同为 359 569 字节**）。
   `src_dir = src` ⟹ 它不参与编译，只污染全仓 `grep` 计数（RF24 计数因此从 136 虚增到 160）。
   建议随某次改动删掉，但**不要在合规提交里顺手删** —— 那会把"移除文件"混进"许可修订"，
   让 review 分不清哪些删除是授权决策、哪些只是清理。
5. **本地化 `RadioLib` 的引入方式**：一旦加入 `lib_deps`，`LICENSES/` 与
   `THIRD_PARTY_NOTICES.md` 必须同步（RadioLib 是 MIT，属 permissive，不影响退出码，
   但归属义务仍在）。`C9` 的 CI 门禁要做成"新增依赖未登记即红灯"。

---

## §7 复核命令（照抄可跑）

```bash
cd E:/github/HaleHound-CYD

# 阻断现状（有阻断项时退出码 1）
build-reports/hostenv/Scripts/python.exe build-reports/audit_licenses.py

# 耦合面：RF24
grep -rn '#include *[<"]RF24' --include=*.cpp --include=*.h --include=*.inc src/ include/
grep -rho 'nrf24Radio\.[A-Za-z_]*' --include=*.cpp --include=*.inc src/ | sort | uniq -c | sort -rn

# 耦合面：arduinoFFT（注意两套签名都要覆盖）
grep -rn 'arduinoFFT\|FFTWindow\|FFTDirection' --include=*.cpp --include=*.h --include=*.inc src/ include/

# 耦合面：BLE Keyboard
grep -rn 'BleKeyboard\|bleKb->' --include=*.cpp --include=*.inc src/

# 重建许可文本目录后，证据文件必须仍然一致
build-reports/hostenv/Scripts/python.exe build-reports/collect_licenses.py
git hash-object LICENSES/HaleHound-CYD-MIT.txt   # 22ec18153d4830c2dad2e12efd1ca1a5035fec3b
git rev-parse  56cf737:LICENSE                    # 同上
```

---

**本文只回答"A 路线怎么做、怎么算做完"**；许可边界的法律判断见 `docs/legal/COMPLIANCE_PLAN_zh.md`，
商标与命名（`C3` / `D2`）见同文件 §3，任务状态见 `TODO.md §9`。
