# Chameleon Ultra 协议证据门

Status: `VERIFIED`（帧格式 / LRC 规则 / 命令号 / 槽标签类型 / BLE 传输 UUID）
`PARTIALLY VERIFIED`（状态码表 / 真机行为）

Scope: `ChameleonFrame`（编解码 + 增量接收器）、`ChameleonUltraClient`（BLE 会话与命令）、
`ChameleonUltraModel`（8 槽模型）。

---

## 0. 证据来源（一手）

全部结论来自 **RfidResearchGroup/ChameleonUltra 官方仓库固件源码**，不是博客、不是第三方客户端：

| 结论 | 来源文件 |
|---|---|
| 帧结构、SOF、长度上限、帧长公式 | `firmware/application/src/utils/netdata.h` |
| LRC 规则、LRC1/LRC2/LRC3 各覆盖哪些字节 | `firmware/application/src/utils/dataframe.c` |
| 命令号 | `firmware/application/src/data_cmd.h` |
| 槽标签类型（含旧枚举与迁移宏） | `firmware/application/src/rfid/nfctag/tag_base_type.h` |
| BLE 传输层（NUS 服务与特征 UUID） | `firmware/nrf52_sdk/components/ble/ble_services/ble_nus/ble_nus.h` / `ble_nus.c` |

读取日期：2026-10-16（`main` 分支）。**本仓库不写推断出来的帧**：下面每一条都能指到具体文件。

---

## 1. 帧格式 — VERIFIED

### 1.1 线格式

`netdata.h` 原文（逐字照抄）：

```
 *      0           1           2 3         45               6 7                    8                8 + n           8 + n + 1
 *  SOF(1byte)  LRC(1byte)  CMD(2byte)  Status(2byte)  Data Length(2byte)  Frame Head LRC(1byte)  Data(length)  Frame All LRC(1byte)
 *     0x11       0xEF        cmd(u16)    status(u16)      length(u16)              lrc(u8)          data(u8*)       lrc(u8)
 *
 *  The data length max is 4096, frame length is 1 + 1 + 2 + 2 + 2 + 1 + n + 1 = (10 + n)
 *  So, one frame will be between 10 and 4106 bytes.
 */
```

（原注释里 `8 + n` 那两列的下标写错了，正确偏移是 `9 + n` 与 `9 + n + 1`；
帧长公式 `10 + n` 本身是对的，也就是**头 9 字节 + 载荷 n + 尾 1 字节**。）

| 偏移 | 字段 | 宽度 | 说明 |
|---|---|---|---|
| 0 | SOF | 1 | `0x11`（`NETDATA_FRAME_SOF`） |
| 1 | LRC1 | 1 | `LRC` over `SOF` ⟹ 恒等于 `0xEF` |
| 2 | CMD | 2 | **大端** |
| 4 | STATUS | 2 | **大端**；主机→设备为 `0x0000` |
| 6 | LEN | 2 | **大端**，载荷字节数 |
| 8 | LRC2 | 1 | `LRC` over `SOF\|LRC1\|CMD\|STATUS\|LEN` |
| 9 | DATA | LEN | |
| 9+LEN | LRC3 | 1 | `LRC` over `DATA` |

结构体层面（`netdata.h`，`PACKED`）：

```c
typedef struct {
    uint8_t sof; uint8_t lrc1; uint16_t cmd; uint16_t status; uint16_t len; uint8_t lrc2;
} PACKED netdata_frame_preamble_t;   // sizeof == 9

typedef struct { uint8_t lrc3; } PACKED netdata_frame_postamble_t;   // sizeof == 1
```

**头是 9 字节，不是 8。** 本仓库的 `kChameleonFrameOverhead = 10`（9 头 + 1 尾）与
`ChameleonFrame.h` 里的注释一致。

### 1.2 LRC 规则 — VERIFIED

`dataframe.c` 原文：

```c
static uint8_t compute_lrc(uint8_t *buf, uint16_t bufsize) {
    uint8_t lrc = 0x00;
    for (uint16_t i = 0; i < bufsize; i++) { lrc += buf[i]; }
    return 0x100 - lrc;
}
```

即 **被覆盖字节之和的八位补码**：`sum(covered) + LRC ≡ 0 (mod 256)`。三个 LRC 的覆盖面由
调用点的 `offsetof` 决定：

```c
m_netdata_frame_tx_buf.pre.lrc1 = compute_lrc((uint8_t *)&...pre, offsetof(netdata_frame_preamble_t, lrc1));
m_netdata_frame_tx_buf.pre.lrc2 = compute_lrc((uint8_t *)&...pre, offsetof(netdata_frame_preamble_t, lrc2));
tx_post->lrc3                    = compute_lrc((uint8_t *)&...data, data_length);
```

| LRC | 覆盖 | 常数 |
|---|---|---|
| LRC1 | `SOF` | `0x100 - 0x11 = 0xEF` ✔ 与 `ChameleonFrame.h` 的 `kChameleonLrc1` 一致 |
| LRC2 | `SOF LRC1 CMD STATUS LEN`（7 字节） | 运行时计算 |
| LRC3 | `DATA`（只覆盖载荷） | 运行时计算 |

### 1.3 为什么 LRC2 可以只用 7 字节验证 — VERIFIED（推导自 1.2）

`0x11 + 0xEF = 0x100 ≡ 0 (mod 256)`，所以 `SOF` 与 `LRC1` 对和的贡献恒为 0：

```
LRC(SOF|LRC1|CMD|STATUS|LEN) == LRC(CMD|STATUS|LEN)
```

这正是接收端可以在**没保留前两个字节**的情况下校验帧头的原因 —— 本仓库的
`ChameleonFrameReceiver::kHeaderBytes = 7`（`CMD|STATUS|LEN|LRC2`）就是这条恒等式的直接结果。
如果哪天把 `kHeaderBytes` 改成 9，那不会错，但会白占 2 字节；改成 5 则会漏掉 `LEN` 的进位。

### 1.4 长度上限 — VERIFIED（上游） / 本仓库有意下调

| 项 | 值 | 来源 |
|---|---|---|
| 上游接收上限 | `NETDATA_MAX_DATA_LENGTH = 4096` | `netdata.h` |
| 上游发送拒绝线 | `data_length > 4096` → 报错拒绝 | `data_frame_make()` |
| 上游帧长范围 | 10 … 4106 字节 | `netdata.h` 注释 |
| **本仓库上限** | `kChameleonMaxPayload = 512` | `ChameleonFrame.h` |

本仓库下调到 512 是**有意的偏离**，理由：C5 上 4 KB 的接收缓冲不值得那点 RAM，而本仓库发出的
每条命令的回复都远小于 512（`GET_SLOT_INFO` = 32 B、`GET_ENABLED_SLOTS` = 16 B、
`GET_DEVICE_ADDRESS` = 6 B、`GET_APP_VERSION` = 2 B、昵称 ≤ 32 B）。

**失败的诚实形态**：`command()` 是通用入口，若调用方要了一条超大回复，接收器把它计入
`oversizedFrames()`（`rejectedFrames()` 的一部分）并按超长帧丢弃，**不会**截断成"看起来正常"的短帧。

### 1.5 为什么接收端必须逐字节喂 — VERIFIED（结构决定的必然）

BLE 侧命令流走 Nordic UART 服务的 notify 特征，而**一条 notification 不是一帧**：默认 ATT MTU
下一条 notification 只有 20 字节载荷。所以一帧可能横跨多条 notification，一条 notification 也可能
装下"上一帧的尾巴 + 下一帧的头"。接收端因此是纯字节流状态机（`Idle → Lrc1 → Header → Data → Lrc3`），
而不是"一条消息 = 一帧"。

配套的一条结论（本次由测试逼出来的）：**链接刚建立时，流里可能先出现别人帧的尾巴**，
一个假的 `SOF+LRC1` 对会把紧随其后的真帧一起吃掉 —— 因此被拒绝的窗口要回退重扫
（`rearmFromPair()`），而不是整段丢弃。向量见 §6 的
`test_a_frame_starting_mid_stream_is_found`。

---

## 2. 命令号 — VERIFIED

全部逐字取自 `data_cmd.h`。**区间分配**本身就是文档：

| 区间 | 用途 |
|---|---|
| 1000–1999 | 设备本身（版本、槽、设置、配对） |
| 2000–2999 | HF 读卡器 |
| 3000–3999 | LF 读卡器 |
| 4000–4999 | HF 模拟器 |
| 5000–5999 | LF 模拟器 |
| 6000+ | ISO14443-4 T=CL 模拟 |

本仓库在 `enum class ChameleonCommand` 里命名并使用的：

| 命令 | 值 | 本仓库用途 |
|---|---|---|
| `GetAppVersion` | 1000 | 版本（2 字节：major, minor） |
| `ChangeDeviceMode` | 1001 | 模式切换 |
| `GetDeviceMode` | 1002 | 模式查询 |
| `SetActiveSlot` | 1003 | 切换当前槽 |
| `SetSlotTagType` | 1004 | 填槽（写类型） |
| `SetSlotEnable` | 1006 | **按频率**开关槽 |
| `SetSlotTagNick` | 1007 | 写昵称（≤32 UTF-8 字节，无终止符） |
| `GetSlotTagNick` | 1008 | 读昵称 |
| `GetDeviceChipId` | 1011 | 芯片 ID |
| `GetDeviceAddress` | 1012 | nRF `DEVICEADDR`（U48） |
| `SaveSettings` | 1013 | 落盘（不调用则重启丢失） |
| `GetGitVersion` | 1017 | 固件 git 版本 |
| `GetActiveSlot` | 1018 | 当前槽 |
| `GetSlotInfo` | 1019 | 8×(hf,lf) u16 类型对 = **32 字节** |
| `WipeFds` | 1020 | **擦除** flash data storage |
| `DeleteSlotTagNick` | 1021 | 删昵称 |
| `GetEnabledSlots` | 1023 | 8×(hf,lf) bool 对 = **16 字节** |
| `GetBatteryInfo` | 1025 | 电量 |
| `GetDeviceModel` | 1033 | 0 = Ultra，1 = Lite |

**一处修正（本次）**：1020 原来在本仓库写成 `WriteFds`。`data_cmd.h` 里它是
`DATA_CMD_WIPE_FDS` —— 这条命令**擦除** FDS，不是写。枚举名是调用方读语义的唯一入口，
名字说反了等于把一条破坏性命令伪装成无害命令。已改名为 `WipeFds` 并附注释。

---

## 3. 槽标签类型 — VERIFIED

### 3.1 新枚举（`tag_base_type.h` 逐字）

```c
TAG_TYPE_UNDEFINED = 0,
OLD_TAG_TYPE_EM410X, OLD_TAG_TYPE_MIFARE_Mini, OLD_TAG_TYPE_MIFARE_1024,
OLD_TAG_TYPE_MIFARE_2048, OLD_TAG_TYPE_MIFARE_4096,
OLD_TAG_TYPE_NTAG_213, OLD_TAG_TYPE_NTAG_215, OLD_TAG_TYPE_NTAG_216,   // 1..8

TAG_TYPE_EM410X = 100, TAG_TYPE_EM410X_16, TAG_TYPE_EM410X_32, TAG_TYPE_EM410X_64, TAG_TYPE_EM410X_ELECTRA,
TAG_TYPE_PAC = 150, TAG_TYPE_VIKING = 170, TAG_TYPE_JABLOTRON = 180,
TAG_TYPE_HID_PROX = 200, TAG_TYPE_IOPROX, 
TAG_TYPE_IDTECK = 310,
TAG_TYPE_MIFARE_Mini = 1000, TAG_TYPE_MIFARE_1024, TAG_TYPE_MIFARE_2048, TAG_TYPE_MIFARE_4096,
TAG_TYPE_NTAG_213 = 1100, TAG_TYPE_NTAG_215, TAG_TYPE_NTAG_216, TAG_TYPE_MF0ICU1,
TAG_TYPE_MF0ICU2, TAG_TYPE_MF0UL11, TAG_TYPE_MF0UL21, TAG_TYPE_NTAG_210, TAG_TYPE_NTAG_212,
TAG_TYPE_HF14A_4 = 3000, TAG_TYPE_SEOS,
```

展开成数值（只列本仓库会遇到的）：

| 类型 | 值 | 频段 |
|---|---|---|
| `EM410X` / `_16` / `_32` / `_64` / `_ELECTRA` | 100–104 | LF |
| `PAC` | 150 | LF |
| `VIKING` | 170 | LF |
| `JABLOTRON` | 180 | LF |
| `HID_PROX` | 200 | LF (FSK) |
| `IOPROX` | 201 | LF (FSK) |
| `IDTECK` | 310 | LF (PSK) |
| `MIFARE_Mini/1024/2048/4096` | 1000–1003 | HF |
| `NTAG_213/215/216` | 1100–1102 | HF |
| `MF0ICU1` / `MF0ICU2` / `MF0UL11` / `MF0UL21` | 1103–1106 | HF |
| `NTAG_210` / `NTAG_212` | 1107–1108 | HF |
| `HF14A_4` / `SEOS` | 3000–3001 | HF |

### 3.2 旧枚举必须**先迁移**再映射

固件自己不把旧值当新值看，而是显式迁移（原文宏）：

```c
#define TAG_SPECIFIC_TYPE_OLD2NEW_LF_VALUES {OLD_TAG_TYPE_EM410X, TAG_TYPE_EM410X}
#define TAG_SPECIFIC_TYPE_OLD2NEW_HF_VALUES                        \
    {OLD_TAG_TYPE_MIFARE_Mini,  TAG_TYPE_MIFARE_Mini},             \
    {OLD_TAG_TYPE_MIFARE_1024,  TAG_TYPE_MIFARE_1024},             \
    {OLD_TAG_TYPE_MIFARE_2048,  TAG_TYPE_MIFARE_2048},             \
    {OLD_TAG_TYPE_MIFARE_4096,  TAG_TYPE_MIFARE_4096},             \
    {OLD_TAG_TYPE_NTAG_213,     TAG_TYPE_NTAG_213},                \
    {OLD_TAG_TYPE_NTAG_215,     TAG_TYPE_NTAG_215},                \
    {OLD_TAG_TYPE_NTAG_216,     TAG_TYPE_NTAG_216}
```

因此：`1 → 100`（LF）、`2..8 →` 对应的 HF 新值。**旧值只出现在对应的那一侧**：
旧 `EM410X`(1) 是 LF，旧 `MIFARE_*`/`NTAG_*`(2..8) 是 HF。本仓库的
`ChameleonUltraClient::normaliseTagType()` 就是这张表。

### 3.3 HF 与 LF 是**两个独立区间** —— 这是最容易静默出错的点

`GET_SLOT_INFO` 返回的是 `tag_slot_specific_type_t { tag_hf; tag_lf; }`（原文结构体），
每个槽有 **hf 与 lf 两个各自独立的类型**。所以：

- `slotKindFor(hfType, lfType)` 的判据必须分开：`100`（EM410X）放在 **hf** 位上是"不知道"，
  不是 EM4100；`1001`（MIFARE_1024）放在 **lf** 位上也一样。
- 空槽两边都是 `TAG_TYPE_UNDEFINED = 0`（注意不是旧枚举的 1）。
- 映射不到的类型**留 `Unknown`**，不给一个"看起来合理"的标签。

这三条都由 `test/native/test_chameleon_ble_client` 钉死（含一条专门的回归：
把 LF 的 `100` 传进 hf 参数必须**不再**被判成 EM4100）。

---

## 4. BLE 传输层 — VERIFIED

### 4.1 NUS 的三条 UUID

`ble_nus.c` 原文：

```c
#define BLE_UUID_NUS_TX_CHARACTERISTIC 0x0003
#define BLE_UUID_NUS_RX_CHARACTERISTIC 0x0002
#define NUS_BASE_UUID {{0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x00,0x00,0x40,0x6E}}
```

`NUS_BASE_UUID` 是 SoftDevice 的小端存储序，反写成打印序 = `6E400000-B5A3-F393-E0A9-E50E24DCCA9E`；
`BLE_UUID_NUS_SERVICE = 0x0001`（`ble_nus.h`）被填进前 16 位，`0x0002`/`0x0003` 同法填入特征：

| 角色 | 打印形式 | 属性（原文 `add_char_params`） |
|---|---|---|
| 服务 | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | primary |
| RX（主机→设备） | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` | `write = 1`, `write_wo_resp = 1` |
| TX（设备→主机） | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` | `notify = 1` |

与 `ChameleonUltraClient::serviceUuid()/rxUuid()/txUuid()` 的三个 16 字节常量逐字节一致。

### 4.2 陷阱：Nordic 自己文档里的 "16-bit offset 0x0001"

`ble_nus.h` 里这一行是坑的来源：

```c
#define BLE_UUID_NUS_SERVICE 0x0001 /**< The UUID of the Nordic UART Service. */
```

**`0x0001` 本身不是这个服务。** 它是一个"相对自定义 base UUID 的偏移"，必须与
`NUS_BASE_UUID` 组合才成立。若把它当成 SIG base 上的 16 位 UUID 展开，得到的是
`00000001-0000-1000-8000-00805f9b34fb` —— 一个完全不同的服务。这就是本仓库
`IBleCentral` 承**完整 128 位**、只在确实位于 SIG base 时才 `shortValue()` 的原因。

### 4.3 一条真机才会遇到的坑：首次上电配置会要求配对

`ble_nus.c` 在加特征时按一个设置分叉：

```c
if (settings_get_ble_pairing_enable_first_load()) {
    add_char_params.read_access  = SEC_MITM;
    add_char_params.write_access = SEC_MITM;
} else { ... SEC_OPEN ... }
```

即**"首次加载时启用配对"为真时，NUS 的读写访问是 MITM 保护的**。表现出来是
"连接成功、发现正常、但写入被拒" —— 本仓库的 `BleStatus` 枚举专门区分了
`NotPermitted`（对端拒绝）与 `IoError`（链路层失败），就是为了让这个症状不被报告成别的。
未验证项：该设置出厂默认值，以及需要配对时本机 BLE 栈的行为（见 §5）。

---

## 5. 仍未验证的部分

| 项 | 状态 | 升格条件 |
|---|---|---|
| **STATUS 码表** | 只知道 `0x0000` = 成功 | 读 `app_cmd.c` 的返回状态全集。在此之前失败一律按 `fail 0xNNNN` **原样报码**，不编标签 |
| 真机帧行为（分片、时序、重传） | 未测 | Ultra 实机 + `X2` 台架；用 `framesDecoded()/lrcFailures()/resyncs()/oversizedFrames()` 计数器做量化回归 |
| `settings_get_ble_pairing_enable_first_load()` 出厂值 | 未定 | 读固件 settings 默认值定义 + 真机首连验证 |
| 昵称的 UTF-8 规范化 | 未验证 | 只做长度门（≤32 字节，超长拒绝而非截断），不做规范化 |
| 上游 4096 上限下的大回复 | **本仓库不支持** | 见 §1.4；如要支持，需同时抬 `kChameleonMaxPayload` 并复核 C5 的 RAM 预算 |

---

## 6. 已 pin 的回归向量

### 帧格式（`test/native/test_chameleon_frame`）

| 断言 | 用例 |
|---|---|
| `chameleonLrc({0x11}) == 0xEF`；被覆盖字节之和 + LRC ≡ 0 (mod 256)；空字段 LRC = 0 | `test_the_lrc_makes_the_byte_sum_wrap_to_zero` |
| 无载荷帧 1000 的逐字节向量 = `11 EF 03 E8 00 00 00 00 15 00`（**头 9 + 尾 1**） | `test_a_command_with_no_payload_matches_the_hand_computed_frame` |
| 载荷夹在 LRC2 与 LRC3 之间：帧 1003 = `11 EF 03 EB 00 00 00 01 11 02 FE` | `test_a_payload_is_placed_between_the_two_lrcs` |
| 超长载荷 / 缓冲区不足 ⟹ 返回 0 **拒绝**，不发截断帧 | `test_an_oversized_payload_or_a_short_buffer_is_refused_not_truncated` |
| 编码→解码往返；解码报告消耗长度并忽略其后的字节 | `test_a_frame_round_trips_through_encode_and_decode`、`test_decode_reports_where_the_frame_ended_and_ignores_what_follows` |
| 一帧跨多条 notification：切点落在帧头内与数据内仍能重组 | `test_a_frame_split_across_notifications_is_reassembled` |
| **流中间的帧 + 帧前噪声（含一个巧合的 `SOF+LRC1` 对）仍能找到** —— 即 `rearmFromPair()`（§1.5） | `test_a_frame_starting_mid_stream_is_found` |
| 背靠背两帧都解出 | `test_two_frames_back_to_back_are_both_decoded` |
| 零载荷帧（10 字节）在 LRC3 到达前不产出任何帧 | `test_a_zero_length_frame_completes_on_its_lrc3` |
| 载荷被篡改 1 bit：只丢那一帧，不吞掉下一帧（`lrcFailures()` 与 `resyncs()` 各 +1） | `test_a_corrupt_payload_is_rejected_and_the_stream_recovers` |
| 帧头声明 `LEN=0xFFFF` 且 LRC2 **正确**时，靠上限拦下而不是越界读 64 KB | `test_a_corrupt_header_is_rejected_without_reading_past_the_buffer` |
| 超长帧不阻塞紧随其后的合法帧 | `test_an_oversized_frame_does_not_stop_the_next_valid_one` |
| 大端字段越界时不读 | `test_a_big_endian_field_is_read_only_when_it_fits` |
| 只有本仓库能命名的状态被命名；失败码原样报 `fail 0xNNNN` | `test_only_the_status_this_build_can_name_gets_a_name`、`test_a_failed_response_still_parses_as_a_frame` |

### 命令与传输（`test/native/test_chameleon_ble_client`）

| 断言 | 用例 |
|---|---|
| 三条 NUS UUID 是**128 位常量**（不是 `0x0001`） | `test_the_command_link_is_the_128_bit_nordic_uart_service` |
| 扫描只留"可连接且广告了 UART 服务"的对端 | `test_a_scan_keeps_only_connectable_peers_that_advertise_the_uart_service` |
| 对端没有 UART 服务 ⟹ `NoService`，不算连接成功 | `test_a_peer_without_the_uart_service_is_not_a_connection` |
| 回复按命令号配对；别人的回复不算自己的 | `test_a_command_returns_its_own_reply`、`test_a_reply_for_a_different_command_is_not_accepted` |
| 回复长度不符（`GET_SLOT_INFO` 32 B / `GET_ENABLED_SLOTS` 16 B）⟹ 不当答案用 | `test_a_reply_of_the_wrong_size_is_not_used_as_the_answer` |
| 超过 MTU 的回复从多条 notification 重组 | `test_a_reply_longer_than_the_mtu_is_reassembled_from_notifications` |
| 设备地址必须是 static random（两个强制位），不是"无类型数字" | `test_the_device_address_is_a_static_random_address_not_an_untyped_number` |
| 槽类型来自固件数字；**HF / LF 分开判据** | `test_the_slot_kinds_come_from_the_firmware_tag_numbers` |
| 叫不出名字的类型留 `Unrecognised`，不猜 | `test_a_tag_this_build_cannot_name_is_unrecognised_not_guessed` |
| 旧标号 `1..8` **先迁移**再映射 | `test_an_old_style_tag_number_is_migrated_before_it_is_mapped`、`test_a_slot_table_of_old_numbers_still_names_cards` |
| `end()` 释放订阅与控制器；被留下的扫描也会停 | `test_end_releases_the_subscription_and_the_controller`、`test_end_stops_a_scan_left_running` |
