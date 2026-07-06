# Launcher 项目代码结构分析

> 参考路径：`.reference/Launcher/`
> 重点：T-Lora Pager 板卡的底层硬件接口

---

## 1. 项目总体结构

```
.reference/Launcher/
├── platformio.ini              # 全局构建配置（所有板卡的公共配置）
├── boards/                     # 各板卡专属配置
│   └── lilygo-t-lora-pager/
│       ├── interface.cpp       # 【核心】板卡硬件接口实现
│       └── platformio.ini      # 板卡专属引脚/构建宏定义
├── include/                    # 全局头文件
│   ├── interface.h             # 硬件接口声明（板卡无关的统一 API）
│   ├── globals.h               # 全局变量、数据结构定义
│   └── pre_compiler.h          # 编译期默认值/条件宏
├── src/                        # 通用业务逻辑
│   ├── main.cpp                # 入口：初始化、FreeRTOS 任务
│   ├── idf/                    # ESP-IDF 原生封装层
│   │   ├── launcher_platform.h/.cpp  # GPIO/定时器/随机数封装
│   │   ├── idf_wifi.h/.cpp           # Wi-Fi 封装
│   │   ├── idf_update.h/.cpp         # OTA/分区写入封装
│   │   ├── idf_http_client.h/.cpp    # HTTP 客户端封装
│   │   └── idf_web_server.h/.cpp     # Web 服务器封装
│   ├── display.h/.cpp          # 显示抽象（基于 tft 对象）
│   ├── tft.h/.cpp              # TFT 驱动适配（多驱动后端）
│   ├── sd_functions.h/.cpp     # SD 卡操作
│   ├── settings.h/.cpp         # NVS 设置持久化
│   ├── powerSave.h/.cpp        # 屏幕亮度/休眠管理
│   ├── partitioner.h/.cpp      # Flash 分区管理
│   ├── onlineLauncher.h/.cpp   # 在线固件下载/安装
│   ├── app_registry.h/.cpp     # 已安装应用注册表
│   ├── massStorage.h/.cpp      # USB MSC 大容量存储模式
│   ├── webInterface.h/.cpp     # Web UI 接口
│   └── wifi_crypto.h/.cpp      # Wi-Fi 密码加密存储
└── lib_modules/                # git submodule 第三方库
    ├── SensorLib/              # BQ27220 燃料计等传感器
    ├── XPowersLib/             # BQ25896 电源管理
    ├── ArduinoJson/
    └── Arduino_GFX/
```

---

## 2. 硬件抽象分层

项目采用两级抽象：

```
业务逻辑层 (src/*.cpp)
        │  调用统一 API
        ▼
接口声明层 (include/interface.h)
        │  弱符号 (weak) 默认空实现在 main.cpp
        ▼
板卡实现层 (boards/lilygo-t-lora-pager/interface.cpp)
        │  调用 ESP-IDF 原生包装
        ▼
IDF 封装层 (src/idf/launcher_platform.h)
        │  调用 esp-idf driver/gpio.h 等
        ▼
ESP-IDF 底层
```

`interface.h` 中声明的函数由每个板卡各自在 `boards/<board>/interface.cpp` 中实现。`main.cpp` 通过 `__attribute__((weak))` 提供空的默认实现，若板卡提供了具体实现则自动覆盖。

---

## 3. T-Lora Pager 硬件接口详解

### 3.1 引脚宏定义（boards/lilygo-t-lora-pager/platformio.ini）

| 功能 | 宏 | GPIO |
|---|---|---|
| 返回按钮 | `BK_BTN` | GPIO 0 |
| 选择/编码器按键 | `SEL_BTN` / `ENCODER_KEY` | GPIO 7 |
| 编码器 A 相 | `ENCODER_INA` | GPIO 40 |
| 编码器 B 相 | `ENCODER_INB` | GPIO 41 |
| TFT 背光 | `TFT_BL` | GPIO 42 |
| TFT CS | `TFT_CS` | GPIO 38 |
| TFT DC | `TFT_DC` | GPIO 37 |
| TFT RST | `TFT_RST` | -1（无） |
| SPI MOSI（共用）| `TFT_MOSI` / `SDCARD_MOSI` | GPIO 34 |
| SPI MISO（共用）| `TFT_MISO` / `SDCARD_MISO` | GPIO 33 |
| SPI CLK（共用）| `TFT_SCLK` / `SDCARD_SCK` | GPIO 35 |
| SD 卡 CS | `SDCARD_CS` | GPIO 21 |
| 键盘背光 | `KEYBOARD_BL` | GPIO 46 |

### 3.2 显示屏（ST7796 TFT）

- **驱动**：`ST7796_DRIVER=1`
- **分辨率**：222 × 480（`TFTWIDTH=222`, `TFT_HEIGHT=480`）
- **接口**：SPI（与 SD 卡共用 MOSI/MISO/CLK 总线，CS 独立）
- **IPS 屏**：`TFT_IPS=1`
- **列/行偏移**：COL_OFS=49，ROW_OFS=0
- **旋转**：`ROTATION=3`（横屏）
- **背光控制**：`analogWrite(TFT_BL, value)` + `analogWrite(KEYBOARD_BL, value)`
  - 最小亮度 `MINBRIGHT=1`
  - PWM 频道 0，8bit，10kHz

### 3.3 旋转编码器

- **库**：`mathertel/RotaryEncoder @ 1.5.3`
- **中断驱动**：

  ```cpp
  attachInterrupt(digitalPinToInterrupt(ENCODER_INA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_INB), checkPosition, CHANGE);
  ```

- **模式**：`FOUR3`（四步/格）
- **按键**：复用 `SEL_BTN`（GPIO 7）/ `ENCODER_KEY`
- **方向映射**：顺时针 → `NextPress`；逆时针 → `PrevPress`

### 3.4 键盘（TCA8418，I2C）

- **库**：`adafruit/Adafruit TCA8418 @ ^1.0.2`
- **地址**：`KB_I2C_ADDRESS`（默认 0x34）
- **矩阵**：4 行 × 10 列 = 40 键
- **键值表**：三层映射（普通 / Shift / Fn），支持字母、数字、符号
- **特殊键**：`KEY_FN`、`SHIFT`、`CAPS_LOCK`、`KEY_ENTER`、`KEY_BACKSPACE`
- **重复触发**：首次延迟 350 ms，之后每 150 ms 重复
- **启用/复位**：通过 GPIO 扩展器（XL9555）的 `EXPANDS_KB_EN`（pin 8）、`EXPANDS_KB_RST`（pin 2）控制

### 3.5 GPIO 扩展器（XL9555，I2C addr 0x20）

- **库**：`ExtensionIOXL9555`（来自 SensorLib）
- **引脚功能**：

| 扩展引脚 | 宏 | 功能 |
|---|---|---|
| 0 | `EXPANDS_DRV_EN` | 驱动使能 |
| 1 | `EXPANDS_AMP_EN` | 音频放大器使能 |
| 2 | `EXPANDS_KB_RST` | 键盘复位 |
| 3 | `EXPANDS_LORA_EN` | LoRa 模块使能 |
| 4 | `EXPANDS_GPS_EN` | GPS 使能 |
| 5 | `EXPANDS_NFC_EN` | NFC 使能 |
| 7 | `EXPANDS_GPS_RST` | GPS 复位 |
| 8 | `EXPANDS_KB_EN` | 键盘使能 |
| 9 | `EXPANDS_GPIO_EN` | 通用 GPIO 使能 |
| 10 | `EXPANDS_SD_DET` | SD 卡插入检测 |
| 11 | `EXPANDS_SD_PULLEN` | SD 上拉使能（输入） |
| 12 | `EXPANDS_SD_EN` | SD 卡电源使能 |

- **初始化顺序**：KB_RST、KB_EN、SD_EN 设为高电平输出；SD_PULLEN 设为输入

### 3.6 电源管理（BQ25896，I2C）

- **库**：`XPowersLib`（`PowersBQ25896`）
- **配置**：
  - 系统掉电电压：3300 mV
  - 输入限流：3250 mA
  - 充电目标电压：4208 mV
  - 预充电电流：64 mA
  - 恒流充电：832 mA
- **功能**：充电使能、OTG 控制、ADC 测量
- **关机**：`PPM.shutdown()`（`powerOff()` 实现）

### 3.7 燃料计（BQ27220，I2C）

- **库**：`bq27220`（来自 SensorLib）
- **电池容量设定**：`BATTERY_DESIGN_CAPACITY = 1500 mAh`
- **SOC 读取**：`bq.getChargePcnt()` → 返回 0-100%（65535 表示错误）
- **`getBattery()`**：封装为上层 API，异常返回 -1

### 3.8 SD 卡

- **接口**：SPI（共用 TFT SPI 总线，独立 CS=GPIO 21）
- **库**：Arduino `SD.h` + `SPIClass sdcardSPI`
- **初始化**：

  ```cpp
  sdcardSPI.begin(_sck, _miso, _mosi, _cs);
  SDM.begin(_cs, sdcardSPI);
  ```

- **SD 上电控制**：通过 XL9555 的 `EXPANDS_SD_EN` 引脚

### 3.9 I2C 总线

所有 I2C 外设共用同一 `Wire` 总线（`Wire.begin(SDA, SCL)`）：

| 外设 | I2C 地址 |
|---|---|
| GPIO 扩展器（XL9555）| 0x20 |
| 电池充电器（BQ25896） | `BQ25896_I2C_ADDRESS` |
| 燃料计（BQ27220）| 默认地址 |
| 键盘（TCA8418）| `KB_I2C_ADDRESS` |

---

## 4. IDF 封装层 API（src/idf/）

### launcher_platform.h — 基础平台 API

```cpp
uint32_t launcherMillis();                      // esp_timer_get_time() / 1000
void     launcherDelayMs(uint32_t ms);          // vTaskDelay
void     launcherGpioOutput(int pin);           // gpio_set_direction OUTPUT
void     launcherGpioInput(int pin);            // gpio_set_direction INPUT
void     launcherGpioInputPullup(int pin);      // INPUT + PULLUP
int      launcherGpioRead(int pin);             // gpio_get_level
void     launcherGpioWrite(int pin, int level); // gpio_set_level
void     launcherConsolePrintf(const char*, ...); // Serial.vprintf
```

### idf_wifi.h — Wi-Fi

```cpp
bool launcherWifiStartSta();
bool launcherWifiConnect(ssid, password, timeout_ms);
LauncherWifiConnectState launcherWifiConnectStatus(...);
int  launcherWifiScan(std::vector<LauncherWifiAp>&);
bool launcherWifiStartAp(ssid, password, channel, max_clients);
bool launcherWifiStop();
bool launcherWifiIsConnected();
bool launcherMdnsStart(host, port);
```

### idf_update.h — OTA/Flash 更新

```cpp
// 流式更新目标：APP / SPIFFS / FAT_VFS / FAT_SYS
bool launcherUpdateBegin(LauncherUpdateTarget, size_t size);
size_t launcherUpdateWrite(const uint8_t*, size_t len);
bool launcherUpdateEnd();
bool launcherUpdateStream(Stream&, size_t, LauncherUpdateTarget, cb);

// 原始分区写入（绕过 OTA 框架）
bool launcherRawUpdateBegin(uint32_t address, size_t partSize, size_t imgSize, bool appImage);
bool launcherRawUpdateStream(...);
bool launcherRawErase(uint32_t address, size_t size);
bool launcherUpdateRepairPartitionTable(...);
```

---

## 5. 输入事件系统

`InputHandler()`（在 FreeRTOS 任务 `taskInputHandler` 中每 10 ms 轮询）统一处理：

```
编码器旋转 ──────────────► NextPress / PrevPress
编码器按键 (GPIO 7) ─────► SelPress
返回按键 (GPIO 0) ───────► EscPress
键盘 (TCA8418) 矩阵 ─────► KeyStroke 结构体
  ├─ w/s/a/d ──────────► UpPress / DownPress / PrevPress / NextPress
  ├─ Enter ────────────► SelPress
  └─ Backspace ────────► EscPress
```

全局导航变量（`volatile bool`）：

- `NextPress`, `PrevPress`, `UpPress`, `DownPress`
- `SelPress`, `EscPress`, `AnyKeyPress`
- `KeyStroke`（含完整按键数据、HID 码）

上层用 `check(btn)` 内联函数消费单次事件（读取并清除标志位）。

---

## 6. 屏幕节电流程

```
checkPowerSaveTime() 每 10 ms 调用一次
    │
    ├─ 超过 dimmerSet 秒无操作 ──► 调暗至 5%（dimmer=true）
    └─ 再过 5 秒 ──────────────► 关闭背光（isScreenOff=true）

wakeUpScreen() — 任意输入时调用
    └─ 恢复亮度，重置计时器

sleepModeOn() — 深度休眠
    ├─ setCpuFrequencyMhz(80)
    ├─ 关闭背光
    └─ 禁用 WDT

sleepModeOff()
    ├─ setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
    ├─ 恢复亮度
    └─ 重新使能 WDT
```

---

## 7. 应用/固件管理

| 功能 | 实现 |
|---|---|
| 应用注册表 | `app_registry.cpp`，存储在 NVS |
| Flash 分区信息 | `partition_table_model.cpp`，读取 `esp_partition` API |
| 分区操作 | `partitioner.cpp`（dump/restore/attach） |
| 安装布局 | `partition_install_layout.cpp`（多分区协调） |
| 在线安装 | `onlineLauncher.cpp`（LauncherHub REST API） |
| SD 卡安装 | `sd_functions.cpp`（本地 .bin 刷写） |
| OTA 启动 | `app_registry.cpp → launcherBootAppByLabel()` |

分区类型：APP（OTA_0）、SPIFFS、FAT（vfs/sys 两个 label）。  
Flash 总大小：16 MB（分区表 `support_files/custom_16Mb.csv`）。

---

## 8. 关键外部库依赖

| 库 | 用途 |
|---|---|
| `mathertel/RotaryEncoder @1.5.3` | 旋转编码器（中断驱动） |
| `adafruit/Adafruit TCA8418 @^1.0.2` | 键盘矩阵扫描 |
| `XPowersLib`（lib_modules submodule）| BQ25896 充电管理 |
| `SensorLib`（lib_modules submodule）| BQ27220 燃料计 + XL9555 GPIO 扩展 |
| `ArduinoJson`（lib_modules submodule）| JSON 序列化（配置/API） |
| `Arduino_GFX`（lib_modules submodule）| 图形库（可选后端） |
| Arduino `SD.h` + `SPI.h` | SD 卡操作 |
| Arduino `LittleFS.h` | 内部文件系统 |
| Arduino `FFat.h` | FAT 分区文件系统 |
| ESP-IDF `esp_ota_ops.h` | OTA 分区管理 |
| ESP-IDF `nvs_handle.hpp` | 设置持久化（NVS） |

---

## 9. 移植要点（给 ESP-IDF wrapper 项目的参考）

1. **GPIO 初始化**：参照 `_setup_gpio()` 中的顺序——先 CS 引脚拉高，再初始化 I2C，再初始化各外设。
2. **XL9555 扩展器**：是 LoRa、GPS、NFC 等外设的使能总闸，必须先初始化。
3. **键盘 TCA8418**：需要在 XL9555 完成初始化（KB_RST、KB_EN）后才能正常工作。
4. **SPI 总线共用**：TFT 和 SD 卡共用 GPIO 33/34/35，CS 分别为 38 和 21，切换时需拉高未选中设备的 CS。
5. **电源关机**：通过 `PPM.shutdown()` 实现，不是 `esp_deep_sleep_start()`。
6. **输入系统**：所有导航事件收敛为 `volatile bool` 全局标志，适合直接移植为消息队列或事件组。
