# ESP32-S3 ePaper 1.54 V2 BSP 提取设计

## 背景

官方 V2 ESP-IDF 例程已经导入到 `firmware/official/v2/esp-idf/`。这些例程覆盖 ADC、RTC、SHTC3、Micro SD、Wi-Fi、音频、LVGL、工厂程序、睡眠和 FT6336 触摸测试。

当前代码的主要问题是板级代码分散在多个示例工程中：

- `board_power_bsp` 在多个示例中重复出现。
- `epaper_driver_bsp`、`button_bsp`、`i2c_bsp` 等组件在不同 demo 中复制。
- 引脚定义散落在各个 `main/user_config.h`。
- 应用代码需要记住 GPIO、供电极性、初始化顺序和外设地址，后续开发容易拷错或漏配。

## 目标

建立一个独立、可复用的 V2 板级 BSP 组件，让后续应用工程通过统一接口使用板载硬件能力，而不是从官方示例中复制零散代码。

目标路径：

`firmware/bsp/esp32-s3-epaper-1.54-v2/`

## 非目标

- 不修改 `firmware/official/v2/esp-idf/` 下的官方例程。
- 不在第一阶段重写墨水屏、音频、RTC 或触摸驱动算法。
- 不支持 V1 硬件版本。
- 不封装应用业务逻辑、UI 页面或云端通信。
- 不在 BSP 内强制绑定 LVGL；BSP 只提供显示驱动与可选 LVGL 适配层。

## 推荐方案

采用“保守抽取 + 清晰封装”的方案：

1. 从官方示例中抽取稳定重复的板级代码。
2. 收敛所有 V2 引脚、总线、外设地址到统一头文件。
3. 保留官方驱动行为，先做小幅命名修正和 API 包装。
4. 增加一个最小 smoke test 应用验证 BSP 能独立使用。

这个方案的优点是风险低、和官方例程行为一致，同时可以明显减少后续应用开发时的重复配置。

## 目录结构

```text
firmware/
  bsp/
    esp32-s3-epaper-1.54-v2/
      CMakeLists.txt
      include/
        board_pins.h
        board_config.h
        board_power.h
        board_i2c.h
        board_epaper.h
        board_buttons.h
        board_sdcard.h
        board_audio.h
        board_touch.h
        board_sensors.h
        board.h
      src/
        board_power.cpp
        board_i2c.c
        board_epaper.cpp
        board_buttons.c
        board_sdcard.c
        board_audio.c
        board_touch.cpp
        board_sensors.cpp
        board.cpp
      README.md
  apps/
    board_smoke_test/
      CMakeLists.txt
      main/
        CMakeLists.txt
        main.cpp
```

## BSP 边界

### `board_pins.h`

集中定义 V2 硬件引脚和外设地址。

关键内容：

- 墨水屏 SPI：CS、DC、RST、BUSY、SCK、MOSI、SPI host
- 墨水屏触摸：TP_RST、TP_INT
- I2C：SDA、SCL
- RTC：PCF85063 地址、RTC_INT
- SHTC3：I2C 地址
- FT6336：I2C 地址
- SD：CLK、MISO、MOSI
- 音频：I2S MCLK、SCLK、LRCK、DSDIN、ASDOUT、PA_CTRL、PA_EN
- 电源：EPD_PWR、AUDIO_PWR、VBAT_PWR、BAT_ADC
- 按键：BOOT、PWR
- 显示尺寸：200 x 200

### `board_power`

负责板载电源控制和低功耗入口。

对外接口使用清晰命名：

```c
void board_power_init(void);
void board_power_epaper_on(void);
void board_power_epaper_off(void);
void board_power_audio_on(void);
void board_power_audio_off(void);
void board_power_vbat_hold_on(void);
void board_power_vbat_hold_off(void);
void board_enter_deep_sleep(void);
```

官方 `POWEER_*` 拼写保留为内部兼容，不作为新应用推荐接口。

### `board_i2c`

负责初始化共享 I2C 总线，并提供基础读写能力。

```c
esp_err_t board_i2c_init(void);
i2c_master_bus_handle_t board_i2c_get_bus(void);
esp_err_t board_i2c_register_device(uint16_t address, i2c_master_dev_handle_t *out_handle);
```

RTC、SHTC3、FT6336 都复用这条总线。

### `board_sensors`

封装 RTC 与 SHTC3 的板级访问。

第一阶段只暴露常用同步接口：

```c
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t week;
} board_rtc_time_t;

typedef struct {
    float temperature_c;
    float humidity_percent;
} board_shtc3_data_t;

esp_err_t board_rtc_get_time(board_rtc_time_t *out_time);
esp_err_t board_rtc_set_time(const board_rtc_time_t *time);
esp_err_t board_shtc3_read(float *temperature_c, float *humidity_percent);
```

`board_rtc_time_t` 对应官方 `RtcDateTime_t` 字段，但命名放在 BSP 命名空间内，避免应用直接依赖官方示例里的 `i2c_equipment` 类。

### `board_epaper`

封装 1.54 英寸 200 x 200 墨水屏。

第一阶段保留官方 `epaper_driver_display` 的核心实现，在 BSP 层提供更简单的生命周期接口：

```c
esp_err_t board_epaper_init(void);
void board_epaper_clear(void);
void board_epaper_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
void board_epaper_flush_full(void);
void board_epaper_flush_partial(void);
void *board_epaper_get_driver(void);
```

`board_epaper_get_driver()` 用于兼容 LVGL 适配或高级绘制场景。

### `board_buttons`

封装 BOOT 和 PWR 按键。

第一阶段保留官方多击计数能力，整理事件命名：

```c
esp_err_t board_buttons_init(void);
uint8_t board_button_boot_repeat_count(void);
uint8_t board_button_pwr_repeat_count(void);
```

### `board_sdcard`

封装 Micro SD 初始化和基础文件读写。

```c
esp_err_t board_sdcard_mount(void);
esp_err_t board_sdcard_unmount(void);
esp_err_t board_sdcard_read_file(const char *path, char *buffer, uint32_t *out_len);
esp_err_t board_sdcard_write_file(const char *path, const char *data);
float board_sdcard_get_capacity_gb(void);
```

### `board_audio`

封装 ES8311 / I2S 音频初始化和基础播放采集能力。

第一阶段保留官方音频代码接口能力，统一命名：

```c
esp_err_t board_audio_init(void);
void board_audio_set_volume(uint8_t volume);
esp_err_t board_audio_play_write(const void *data, uint32_t len);
esp_err_t board_audio_record_read(void *data, uint32_t len);
```

### `board_touch`

封装 FT6336 触摸。

```c
typedef struct {
    bool touched;
    uint16_t x;
    uint16_t y;
} board_touch_point_t;

esp_err_t board_touch_init(void);
esp_err_t board_touch_read(board_touch_point_t *out_point);
```

`board_touch_read()` 包装官方 `GetTouchPoint(uint16_t *x, uint16_t *y)`，没有触摸时返回 `ESP_OK` 且 `out_point->touched == false`。

### `board.h`

提供面向应用的统一入口。

```c
esp_err_t board_init(void);
esp_err_t board_init_display(void);
esp_err_t board_init_sensors(void);
esp_err_t board_init_audio(void);
esp_err_t board_init_sdcard(void);
esp_err_t board_init_touch(void);
```

`board_init()` 只做基础安全初始化：电源 GPIO、I2C、按键。显示、音频、SD、触摸按需初始化，避免不必要功耗和启动时间。

## Smoke Test 应用

新增 `firmware/apps/board_smoke_test/`，验证 BSP 可被普通应用依赖。

测试流程：

1. 调用 `board_init()`。
2. 打开墨水屏电源并初始化墨水屏。
3. 初始化 I2C。
4. 读取 RTC 时间或确认 RTC 通信可用。
5. 读取 SHTC3 温湿度。
6. 清屏并绘制简单像素或文字图案。
7. 打印 BOOT/PWR 按键状态。

如果没有接 SD 卡、扬声器或触摸面板，smoke test 不应因为这些可选外设失败而无法启动。

## 错误处理

- BSP 对外接口返回 `esp_err_t`。
- 初始化函数可以重复调用；已经初始化时返回 `ESP_OK`。
- 可选外设初始化失败时返回具体错误码，由应用决定是否继续。
- 涉及硬件电源控制的函数不静默失败；GPIO 配置失败时返回错误码或记录 ESP-IDF 日志。

## 测试与验证

第一阶段以构建验证和 smoke test 为主：

- `idf.py build` 编译 BSP smoke test。
- 运行 smoke test，串口日志确认 I2C、RTC/SHTC3 和墨水屏初始化路径。
- 后续接入硬件后，再验证 SD、音频、触摸和深睡眠唤醒。

如果本机没有 ESP-IDF 环境，至少进行文件结构、CMake 依赖和源码静态检查。

## 迁移策略

1. 先抽取最小公共引脚配置和电源/I2C。
2. 再抽取墨水屏驱动，接入 smoke test。
3. 再抽取 RTC/SHTC3。
4. 最后抽取 SD、音频、触摸和按钮事件。
5. 官方例程保持不动，作为行为对照。

每一步都独立提交，方便定位问题和回滚。
