# ESP32-S3 ePaper 1.54 V2 BSP Extraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a reusable ESP-IDF BSP component for the ESP32-S3 ePaper 1.54 V2 board and a smoke-test app that proves applications can use it without copying official examples.

**Architecture:** Keep official examples unchanged under `firmware/official/v2/esp-idf/`. Create a new reusable component at `firmware/bsp/esp32-s3-epaper-1.54-v2/`, moving repeated board-level code behind stable `board_*` APIs. Add `firmware/apps/board_smoke_test/` as the first consumer.

**Tech Stack:** ESP-IDF, C/C++, ESP32-S3, CMake, ESP-IDF components, PCF85063 RTC, SHTC3, FT6336, ES8311/I2S, Micro SD, e-Paper SPI.

---

## File Structure

Create:

- `firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt`: ESP-IDF component definition for all BSP modules.
- `firmware/bsp/esp32-s3-epaper-1.54-v2/README.md`: component usage notes and supported hardware version.
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_pins.h`: single source of truth for V2 pins, dimensions, buses, and I2C addresses.
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_config.h`: component-level constants that are not raw pins.
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_power.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_power.cpp`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_i2c.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_i2c.c`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_epaper.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_epaper.cpp`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_buttons.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_buttons.c`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_sensors.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_sensors.cpp`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_sdcard.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_sdcard.c`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_audio.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_audio.c`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_touch.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_touch.cpp`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board.h`
- `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board.cpp`
- `firmware/apps/board_smoke_test/CMakeLists.txt`
- `firmware/apps/board_smoke_test/main/CMakeLists.txt`
- `firmware/apps/board_smoke_test/main/main.cpp`
- `firmware/apps/board_smoke_test/sdkconfig.defaults`

Copy or adapt implementation from:

- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/board_power_bsp/`
- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/i2c_bsp/`
- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/epaper_driver_bsp/`
- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/button_bsp/`
- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/i2c_equipment/`
- `firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/SensorLib/`
- `firmware/official/v2/esp-idf/04_SD_Card/components/sdcard_bsp/`
- `firmware/official/v2/esp-idf/08_Audio_Test/components/audio_bsp/`
- `firmware/official/v2/esp-idf/08_Audio_Test/components/codec_board/`
- `firmware/official/v2/esp-idf/08_Audio_Test/components/externlib/`
- `firmware/official/v2/esp-idf/13_FT6336_Test/main/ft6336_bsp.*`

Do not modify:

- `firmware/official/v2/esp-idf/**`

---

## Task 1: Create BSP Component Skeleton and Pin Map

**Files:**
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/README.md`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_pins.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_config.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board.cpp`
- Test: shell checks only

- [ ] **Step 1: Write the pin map first**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_pins.h`:

```c
#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_EPD_WIDTH  200
#define BOARD_EPD_HEIGHT 200

#define BOARD_EPD_SPI_HOST SPI2_HOST
#define BOARD_EPD_DC_PIN   GPIO_NUM_10
#define BOARD_EPD_CS_PIN   GPIO_NUM_11
#define BOARD_EPD_SCK_PIN  GPIO_NUM_12
#define BOARD_EPD_MOSI_PIN GPIO_NUM_13
#define BOARD_EPD_RST_PIN  GPIO_NUM_9
#define BOARD_EPD_BUSY_PIN GPIO_NUM_8

#define BOARD_EPD_PWR_PIN GPIO_NUM_6
#define BOARD_AUDIO_PWR_PIN GPIO_NUM_42
#define BOARD_VBAT_PWR_PIN GPIO_NUM_17
#define BOARD_BAT_ADC_PIN GPIO_NUM_4

#define BOARD_BOOT_BUTTON_PIN GPIO_NUM_0
#define BOARD_PWR_BUTTON_PIN  GPIO_NUM_18
#define BOARD_RTC_INT_PIN     GPIO_NUM_5

#define BOARD_I2C_SDA_PIN GPIO_NUM_47
#define BOARD_I2C_SCL_PIN GPIO_NUM_48
#define BOARD_I2C_PORT    I2C_NUM_0
#define BOARD_I2C_SPEED_HZ 400000

#define BOARD_RTC_I2C_ADDR    0x51
#define BOARD_SHTC3_I2C_ADDR  0x70
#define BOARD_FT6336_I2C_ADDR 0x38

#define BOARD_EPD_TP_RST_PIN GPIO_NUM_7
#define BOARD_EPD_TP_INT_PIN GPIO_NUM_21

#define BOARD_SD_CLK_PIN  GPIO_NUM_39
#define BOARD_SD_MISO_PIN GPIO_NUM_40
#define BOARD_SD_MOSI_PIN GPIO_NUM_41

#define BOARD_I2S_MCLK_PIN  GPIO_NUM_14
#define BOARD_I2S_SCLK_PIN  GPIO_NUM_15
#define BOARD_I2S_ASDOUT_PIN GPIO_NUM_16
#define BOARD_I2S_LRCK_PIN  GPIO_NUM_38
#define BOARD_I2S_DSDIN_PIN GPIO_NUM_45
#define BOARD_PA_CTRL_PIN   GPIO_NUM_46
#define BOARD_PA_EN_PIN     GPIO_NUM_42

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Write board config constants**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_config.h`:

```c
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_NAME "ESP32-S3-ePaper-1.54"
#define BOARD_HW_VERSION "V2"
#define BOARD_FLASH_SIZE_MB 8
#define BOARD_PSRAM_SIZE_MB 8
#define BOARD_LVGL_TICK_PERIOD_MS 5

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 3: Create the umbrella board API**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board.h`:

```c
#pragma once

#include "esp_err.h"
#include "board_config.h"
#include "board_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_init(void);
esp_err_t board_init_display(void);
esp_err_t board_init_sensors(void);
esp_err_t board_init_audio(void);
esp_err_t board_init_sdcard(void);
esp_err_t board_init_touch(void);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 4: Add temporary stub implementation**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board.cpp`:

```cpp
#include "board.h"

esp_err_t board_init(void)
{
    return ESP_OK;
}

esp_err_t board_init_display(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sensors(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_audio(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sdcard(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_touch(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
```

- [ ] **Step 5: Add component CMake**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "src/board.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
)
```

- [ ] **Step 6: Add BSP README**

Create `firmware/bsp/esp32-s3-epaper-1.54-v2/README.md`:

```markdown
# ESP32-S3 ePaper 1.54 V2 BSP

Reusable ESP-IDF board support package for the ESP32-S3 ePaper 1.54 V2 hardware.

Supported hardware:

- ESP32-S3-PICO-1-N8R8
- 8MB Flash
- 8MB PSRAM
- 1.54 inch 200 x 200 e-Paper display

The official examples remain under `firmware/official/v2/esp-idf/` and are used as the behavior reference for this BSP.
```

- [ ] **Step 7: Verify skeleton files**

Run:

```bash
test -f firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_pins.h
test -f firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt
rg -n "BOARD_EPD_CS_PIN|BOARD_I2C_SDA_PIN|BOARD_FT6336_I2C_ADDR" firmware/bsp/esp32-s3-epaper-1.54-v2
```

Expected: all commands exit 0 and `rg` prints the three constants.

- [ ] **Step 8: Commit**

```bash
git add firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Add V2 BSP component skeleton"
```

---

## Task 2: Extract Power and I2C Modules

**Files:**
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_power.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_power.cpp`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_i2c.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_i2c.c`
- Modify: `firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt`
- Modify: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board.cpp`
- Test: smoke app compile later; static C/C++ checks now

- [ ] **Step 1: Write power API**

Create `include/board_power.h`:

```c
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_power_init(void);
esp_err_t board_power_epaper_on(void);
esp_err_t board_power_epaper_off(void);
esp_err_t board_power_audio_on(void);
esp_err_t board_power_audio_off(void);
esp_err_t board_power_vbat_hold_on(void);
esp_err_t board_power_vbat_hold_off(void);
void board_enter_deep_sleep(void);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Implement power API from official polarity**

Create `src/board_power.cpp`:

```cpp
#include "board_power.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#include "esp_sleep.h"

static const char *TAG = "board_power";
static bool s_power_initialized = false;

esp_err_t board_power_init(void)
{
    if (s_power_initialized) {
        return ESP_OK;
    }

    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = (1ULL << BOARD_EPD_PWR_PIN) |
                             (1ULL << BOARD_AUDIO_PWR_PIN) |
                             (1ULL << BOARD_VBAT_PWR_PIN);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_RETURN_ON_ERROR(gpio_config(&gpio_conf), TAG, "configure power pins failed");

    s_power_initialized = true;
    return ESP_OK;
}

esp_err_t board_power_epaper_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_EPD_PWR_PIN, 0);
}

esp_err_t board_power_epaper_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_EPD_PWR_PIN, 1);
}

esp_err_t board_power_audio_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_AUDIO_PWR_PIN, 0);
}

esp_err_t board_power_audio_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_AUDIO_PWR_PIN, 1);
}

esp_err_t board_power_vbat_hold_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_VBAT_PWR_PIN, 1);
}

esp_err_t board_power_vbat_hold_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_VBAT_PWR_PIN, 0);
}

void board_enter_deep_sleep(void)
{
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    const uint64_t wake_mask = (1ULL << BOARD_BOOT_BUTTON_PIN) |
                               (1ULL << BOARD_RTC_INT_PIN) |
                               (1ULL << BOARD_PWR_BUTTON_PIN);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(BOARD_RTC_INT_PIN));
    ESP_ERROR_CHECK(rtc_gpio_pullup_en(BOARD_RTC_INT_PIN));
    ESP_ERROR_CHECK(rtc_gpio_hold_en(BOARD_VBAT_PWR_PIN));
    esp_deep_sleep_start();
}
```

- [ ] **Step 3: Write I2C API**

Create `include/board_i2c.h`:

```c
#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_i2c_init(void);
i2c_master_bus_handle_t board_i2c_get_bus(void);
esp_err_t board_i2c_register_device(uint16_t address, i2c_master_dev_handle_t *out_handle);
esp_err_t board_i2c_write_read(i2c_master_dev_handle_t dev_handle,
                               const uint8_t *write_buf,
                               uint8_t write_len,
                               uint8_t *read_buf,
                               uint8_t read_len);
esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, uint8_t len);
esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, const uint8_t *buf, uint8_t len);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 4: Implement I2C API using ESP-IDF new master driver**

Create `src/board_i2c.c`:

```c
#include "board_i2c.h"

#include <string.h>
#include "board_pins.h"
#include "esp_check.h"

static const char *TAG = "board_i2c";
static i2c_master_bus_handle_t s_i2c_bus = NULL;

esp_err_t board_i2c_init(void)
{
    if (s_i2c_bus != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_PIN,
        .scl_io_num = BOARD_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    return i2c_new_master_bus(&bus_config, &s_i2c_bus);
}

i2c_master_bus_handle_t board_i2c_get_bus(void)
{
    return s_i2c_bus;
}

esp_err_t board_i2c_register_device(uint16_t address, i2c_master_dev_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "out_handle is null");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = BOARD_I2C_SPEED_HZ,
    };

    return i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, out_handle);
}

esp_err_t board_i2c_write_read(i2c_master_dev_handle_t dev_handle,
                               const uint8_t *write_buf,
                               uint8_t write_len,
                               uint8_t *read_buf,
                               uint8_t read_len)
{
    return i2c_master_transmit_receive(dev_handle, write_buf, write_len, read_buf, read_len, -1);
}

esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, uint8_t len)
{
    return board_i2c_write_read(dev_handle, &reg, 1, buf, len);
}

esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t write_buf[32];
    ESP_RETURN_ON_FALSE(len + 1 <= sizeof(write_buf), ESP_ERR_INVALID_SIZE, TAG, "i2c write too large");
    write_buf[0] = reg;
    memcpy(&write_buf[1], buf, len);
    return i2c_master_transmit(dev_handle, write_buf, len + 1, -1);
}
```

- [ ] **Step 5: Wire `board_init()` to power and I2C**

Modify `src/board.cpp`:

```cpp
#include "board.h"

#include "board_i2c.h"
#include "board_power.h"
#include "esp_check.h"

static const char *TAG = "board";

esp_err_t board_init(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    ESP_RETURN_ON_ERROR(board_power_vbat_hold_on(), TAG, "vbat hold failed");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    return ESP_OK;
}

esp_err_t board_init_display(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sensors(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_audio(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sdcard(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_touch(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
```

- [ ] **Step 6: Update component CMake**

Modify `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "src/board.cpp"
        "src/board_power.cpp"
        "src/board_i2c.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        esp_driver_gpio
        esp_driver_i2c
        esp_hw_support
)
```

- [ ] **Step 7: Verify no official files changed**

Run:

```bash
git diff --name-only -- firmware/official/v2/esp-idf
```

Expected: no output.

- [ ] **Step 8: Commit**

```bash
git add firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Extract BSP power and I2C modules"
```

---

## Task 3: Extract e-Paper Driver and Display Init

**Files:**
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/include/board_epaper.h`
- Create: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_epaper.cpp`
- Modify: `firmware/bsp/esp32-s3-epaper-1.54-v2/CMakeLists.txt`
- Modify: `firmware/bsp/esp32-s3-epaper-1.54-v2/src/board.cpp`
- Copy/adapt: official `epaper_driver_bsp.cpp/.h` into `src/board_epaper.cpp` private implementation or keep its class inside the new file.
- Test: compile via smoke app later; static checks now

- [ ] **Step 1: Write display API**

Create `include/board_epaper.h`:

```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_EPD_COLOR_WHITE 0xff
#define BOARD_EPD_COLOR_BLACK 0x00

esp_err_t board_epaper_init(void);
void board_epaper_clear(void);
void board_epaper_draw_pixel(uint16_t x, uint16_t y, uint8_t color);
void board_epaper_flush_full(void);
void board_epaper_flush_partial(void);
void *board_epaper_get_driver(void);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Copy official driver implementation**

Copy the implementation logic from:

```text
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/epaper_driver_bsp/epaper_driver_bsp.h
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/epaper_driver_bsp/epaper_driver_bsp.cpp
```

Into `src/board_epaper.cpp`.

Keep the C++ class private to `board_epaper.cpp` or rename it to `BoardEpaperDriver`. Replace `user_config.h` dependencies with constants from `board_pins.h`.

- [ ] **Step 3: Add wrapper functions**

At the bottom of `src/board_epaper.cpp`, provide:

```cpp
static BoardEpaperDriver *s_epaper = nullptr;

esp_err_t board_epaper_init(void)
{
    if (s_epaper != nullptr) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(board_power_epaper_on(), TAG, "epaper power on failed");
    board_epaper_config_t config = {
        .cs = BOARD_EPD_CS_PIN,
        .dc = BOARD_EPD_DC_PIN,
        .rst = BOARD_EPD_RST_PIN,
        .busy = BOARD_EPD_BUSY_PIN,
        .mosi = BOARD_EPD_MOSI_PIN,
        .scl = BOARD_EPD_SCK_PIN,
        .spi_host = BOARD_EPD_SPI_HOST,
        .buffer_len = BOARD_EPD_WIDTH * BOARD_EPD_HEIGHT,
    };
    s_epaper = new BoardEpaperDriver(BOARD_EPD_WIDTH, BOARD_EPD_HEIGHT, config);
    ESP_RETURN_ON_FALSE(s_epaper != nullptr, ESP_ERR_NO_MEM, TAG, "epaper allocation failed");
    s_epaper->init();
    return ESP_OK;
}

void board_epaper_clear(void)
{
    if (s_epaper != nullptr) {
        s_epaper->clear();
    }
}

void board_epaper_draw_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (s_epaper != nullptr) {
        s_epaper->draw_pixel(x, y, color);
    }
}

void board_epaper_flush_full(void)
{
    if (s_epaper != nullptr) {
        s_epaper->display();
    }
}

void board_epaper_flush_partial(void)
{
    if (s_epaper != nullptr) {
        s_epaper->display_partial();
    }
}

void *board_epaper_get_driver(void)
{
    return s_epaper;
}
```

Adjust method names to match the copied class. Do not change the display command sequence during this task.

- [ ] **Step 4: Wire display init**

Modify `src/board.cpp`:

```cpp
#include "board_epaper.h"
```

Replace `board_init_display()` with:

```cpp
esp_err_t board_init_display(void)
{
    return board_epaper_init();
}
```

- [ ] **Step 5: Update CMake**

Add `src/board_epaper.cpp` to `SRCS`.

- [ ] **Step 6: Verify official command sequence preserved**

Run:

```bash
rg -n "EPD_Init|EPD_Clear|EPD_DisplayPart|EPD_SendCommand|EPD_SetLut" firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_epaper.cpp
git diff --name-only -- firmware/official/v2/esp-idf
```

Expected: `rg` finds the copied display routines; second command prints no files.

- [ ] **Step 7: Commit**

```bash
git add firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Extract BSP e-paper driver"
```

---

## Task 4: Extract Button, RTC, and SHTC3 Sensor Modules

**Files:**
- Create: `include/board_buttons.h`
- Create: `src/board_buttons.c`
- Create: `include/board_sensors.h`
- Create: `src/board_sensors.cpp`
- Copy: `SensorLib` into BSP component or a nested private component path under BSP if ESP-IDF component discovery requires it.
- Modify: `CMakeLists.txt`
- Modify: `src/board.cpp`

- [ ] **Step 1: Write buttons API**

Create `include/board_buttons.h`:

```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_buttons_init(void);
uint8_t board_button_boot_repeat_count(void);
uint8_t board_button_pwr_repeat_count(void);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Adapt official button implementation**

Copy logic from:

```text
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/button_bsp/button_bsp.c
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/button_bsp/multi_button.c
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/button_bsp/multi_button.h
```

Into:

```text
firmware/bsp/esp32-s3-epaper-1.54-v2/src/board_buttons.c
firmware/bsp/esp32-s3-epaper-1.54-v2/src/multi_button.c
firmware/bsp/esp32-s3-epaper-1.54-v2/include/multi_button.h
```

Replace `BOOT_BUTTON_PIN` and `PWR_BUTTON_PIN` with `BOARD_BOOT_BUTTON_PIN` and `BOARD_PWR_BUTTON_PIN`.

- [ ] **Step 3: Write sensor API**

Create `include/board_sensors.h`:

```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

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

esp_err_t board_sensors_init(void);
esp_err_t board_rtc_get_time(board_rtc_time_t *out_time);
esp_err_t board_rtc_set_time(const board_rtc_time_t *time);
esp_err_t board_shtc3_read(float *temperature_c, float *humidity_percent);
esp_err_t board_shtc3_read_data(board_shtc3_data_t *out_data);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 4: Copy SensorLib and adapt i2c_equipment**

Copy:

```text
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/SensorLib
```

To:

```text
firmware/bsp/esp32-s3-epaper-1.54-v2/SensorLib
```

Create `src/board_sensors.cpp` by adapting:

```text
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/i2c_equipment/i2c_equipment.cpp
firmware/official/v2/esp-idf/12_RTC_Sleep_Test/components/i2c_equipment/i2c_equipment.h
```

Use `board_i2c_init()` and `board_i2c_get_bus()` instead of the official `i2c_bsp` singleton where practical. If SensorLib requires its own bus abstraction, keep the minimal adapter local to `board_sensors.cpp`.

- [ ] **Step 5: Wire buttons and sensors into board init**

Modify `src/board.cpp`:

```cpp
#include "board_buttons.h"
#include "board_sensors.h"
```

Update:

```cpp
esp_err_t board_init(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    ESP_RETURN_ON_ERROR(board_power_vbat_hold_on(), TAG, "vbat hold failed");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    ESP_RETURN_ON_ERROR(board_buttons_init(), TAG, "buttons init failed");
    return ESP_OK;
}

esp_err_t board_init_sensors(void)
{
    return board_sensors_init();
}
```

- [ ] **Step 6: Update CMake**

Add source files:

```cmake
"src/board_buttons.c"
"src/multi_button.c"
"src/board_sensors.cpp"
```

Add SensorLib source/include directories using the official component pattern:

```cmake
set(sensorlib_src_dirs
    SensorLib/src
    SensorLib/src/touch
    SensorLib/src/platform
    SensorLib/src/bosch
    SensorLib/src/bosch/BMM150
    SensorLib/src/bosch/common
)

set(sensorlib_include_dirs
    SensorLib/src
    SensorLib/src/REG
    SensorLib/src/touch
    SensorLib/src/platform
    SensorLib/src/bosch
    SensorLib/src/bosch/firmware
    SensorLib/src/bosch/BMM150
    SensorLib/src/bosch/common
)
```

Then include those in `idf_component_register()` using `SRC_DIRS` or explicit globbing. Keep all SensorLib paths inside this BSP component.

- [ ] **Step 7: Verify no raw official pin names remain in new code**

Run:

```bash
rg -n "BOOT_BUTTON_PIN|PWR_BUTTON_PIN|ESP32_I2C_SDA_PIN|I2C_RTC_DEV_Address|I2C_SHTC3_DEV_Address" firmware/bsp/esp32-s3-epaper-1.54-v2
```

Expected: no output, except comments that explicitly mention old official names. Remove such comments if they add no value.

- [ ] **Step 8: Commit**

```bash
git add firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Extract BSP buttons and sensors"
```

---

## Task 5: Extract SD Card, Audio, and Touch Modules

**Files:**
- Create: `include/board_sdcard.h`
- Create: `src/board_sdcard.c`
- Create: `include/board_audio.h`
- Create: `src/board_audio.c`
- Create: `include/board_touch.h`
- Create: `src/board_touch.cpp`
- Modify: `CMakeLists.txt`
- Modify: `src/board.cpp`

- [ ] **Step 1: Write SD card API and adapt official implementation**

Create `include/board_sdcard.h`:

```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_sdcard_mount(void);
esp_err_t board_sdcard_unmount(void);
esp_err_t board_sdcard_read_file(const char *path, char *buffer, uint32_t *out_len);
esp_err_t board_sdcard_write_file(const char *path, const char *data);
float board_sdcard_get_capacity_gb(void);

#ifdef __cplusplus
}
#endif
```

Create `src/board_sdcard.c` by adapting:

```text
firmware/official/v2/esp-idf/04_SD_Card/components/sdcard_bsp/sdcard_bsp.c
firmware/official/v2/esp-idf/04_SD_Card/components/sdcard_bsp/sdcard_bsp.h
```

Replace raw pins with `BOARD_SD_CLK_PIN`, `BOARD_SD_MISO_PIN`, and `BOARD_SD_MOSI_PIN`.

- [ ] **Step 2: Write audio API and copy required audio dependencies**

Create `include/board_audio.h`:

```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_audio_init(void);
void board_audio_set_volume(uint8_t volume);
esp_err_t board_audio_play_write(const void *data, uint32_t len);
esp_err_t board_audio_record_read(void *data, uint32_t len);

#ifdef __cplusplus
}
#endif
```

Adapt:

```text
firmware/official/v2/esp-idf/08_Audio_Test/components/audio_bsp/audio_bsp.c
firmware/official/v2/esp-idf/08_Audio_Test/components/audio_bsp/audio_bsp.h
```

Copy required support components into BSP-owned subdirectories if needed:

```text
firmware/official/v2/esp-idf/08_Audio_Test/components/codec_board
firmware/official/v2/esp-idf/08_Audio_Test/components/externlib
```

Keep `canon.pcm` embedded if the official audio code still requires it. If the wrapper only exposes streaming read/write, keep the file out of the public API.

- [ ] **Step 3: Write touch API and adapt FT6336**

Create `include/board_touch.h`:

```c
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool touched;
    uint16_t x;
    uint16_t y;
} board_touch_point_t;

esp_err_t board_touch_init(void);
esp_err_t board_touch_read(board_touch_point_t *out_point);

#ifdef __cplusplus
}
#endif
```

Create `src/board_touch.cpp` by adapting:

```text
firmware/official/v2/esp-idf/13_FT6336_Test/main/ft6336_bsp.cpp
firmware/official/v2/esp-idf/13_FT6336_Test/main/ft6336_bsp.h
```

Use `board_i2c_get_bus()`, `BOARD_FT6336_I2C_ADDR`, `BOARD_EPD_TP_RST_PIN`, `BOARD_EPD_WIDTH`, and `BOARD_EPD_HEIGHT`.

- [ ] **Step 4: Wire optional modules into `board.cpp`**

Update:

```cpp
#include "board_audio.h"
#include "board_sdcard.h"
#include "board_touch.h"
```

Set:

```cpp
esp_err_t board_init_audio(void)
{
    return board_audio_init();
}

esp_err_t board_init_sdcard(void)
{
    return board_sdcard_mount();
}

esp_err_t board_init_touch(void)
{
    return board_touch_init();
}
```

- [ ] **Step 5: Update CMake dependencies**

Add new source files to the BSP component.

Add ESP-IDF requirements likely needed by these modules:

```cmake
fatfs
sdmmc
esp_driver_sdspi
esp_driver_i2s
espressif__esp_codec_dev
```

If dependency names differ in the installed ESP-IDF version, adjust based on the build error. Keep changes scoped to CMake requirements, not source behavior.

- [ ] **Step 6: Verify APIs exist**

Run:

```bash
rg -n "board_sdcard_mount|board_audio_init|board_touch_read" firmware/bsp/esp32-s3-epaper-1.54-v2
git diff --name-only -- firmware/official/v2/esp-idf
```

Expected: first command finds headers and implementations; second command prints no files.

- [ ] **Step 7: Commit**

```bash
git add firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Extract BSP optional peripherals"
```

---

## Task 6: Add Board Smoke Test App

**Files:**
- Create: `firmware/apps/board_smoke_test/CMakeLists.txt`
- Create: `firmware/apps/board_smoke_test/main/CMakeLists.txt`
- Create: `firmware/apps/board_smoke_test/main/main.cpp`
- Create: `firmware/apps/board_smoke_test/sdkconfig.defaults`
- Test: `idf.py build` if ESP-IDF is available

- [ ] **Step 1: Create app CMake**

Create `firmware/apps/board_smoke_test/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "../../bsp/esp32-s3-epaper-1.54-v2"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(board_smoke_test)
```

- [ ] **Step 2: Create main component CMake**

Create `firmware/apps/board_smoke_test/main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp32-s3-epaper-1.54-v2
)
```

If ESP-IDF names the component differently because of punctuation in the directory, inspect the build output and adjust the `REQUIRES` name to the actual component name. Prefer setting an explicit component alias in BSP CMake if supported by this ESP-IDF version.

- [ ] **Step 3: Create smoke test main**

Create `firmware/apps/board_smoke_test/main/main.cpp`:

```cpp
#include <stdio.h>
#include "board.h"
#include "board_buttons.h"
#include "board_epaper.h"
#include "board_sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board_smoke_test";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "starting %s %s smoke test", BOARD_NAME, BOARD_HW_VERSION);

    esp_err_t err = board_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "board_init failed: %s", esp_err_to_name(err));
        return;
    }

    err = board_init_sensors();
    if (err == ESP_OK) {
        board_rtc_time_t now = {};
        if (board_rtc_get_time(&now) == ESP_OK) {
            ESP_LOGI(TAG, "rtc: %04u-%02u-%02u %02u:%02u:%02u week=%u",
                     now.year, now.month, now.day, now.hour, now.minute, now.second, now.week);
        }

        float temperature_c = 0.0f;
        float humidity_percent = 0.0f;
        if (board_shtc3_read(&temperature_c, &humidity_percent) == ESP_OK) {
            ESP_LOGI(TAG, "shtc3: %.2f C %.2f %%", temperature_c, humidity_percent);
        }
    } else {
        ESP_LOGW(TAG, "sensor init skipped or failed: %s", esp_err_to_name(err));
    }

    err = board_init_display();
    if (err == ESP_OK) {
        board_epaper_clear();
        for (uint16_t i = 20; i < 180; ++i) {
            board_epaper_draw_pixel(i, i, BOARD_EPD_COLOR_BLACK);
            board_epaper_draw_pixel(199 - i, i, BOARD_EPD_COLOR_BLACK);
        }
        board_epaper_flush_full();
        ESP_LOGI(TAG, "epaper smoke pattern flushed");
    } else {
        ESP_LOGW(TAG, "display init skipped or failed: %s", esp_err_to_name(err));
    }

    while (true) {
        ESP_LOGI(TAG, "buttons: boot=%u pwr=%u",
                 board_button_boot_repeat_count(),
                 board_button_pwr_repeat_count());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

- [ ] **Step 4: Create sdkconfig defaults**

Create `firmware/apps/board_smoke_test/sdkconfig.defaults`:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
```

- [ ] **Step 5: Build if ESP-IDF is available**

Run:

```bash
cd firmware/apps/board_smoke_test
idf.py set-target esp32s3
idf.py build
```

Expected: build exits 0.

If `idf.py` is not available, run:

```bash
command -v idf.py
```

Expected: non-zero exit confirms ESP-IDF is not installed in this shell. Record that build verification could not run locally.

- [ ] **Step 6: Commit**

```bash
git add firmware/apps/board_smoke_test firmware/bsp/esp32-s3-epaper-1.54-v2
git commit -m "Add BSP smoke test app"
```

---

## Task 7: Documentation and Final Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/official-overview.md`
- Modify: `hardware/pinout.md` if BSP confirms the IO18/IO42 PA_EN ambiguity
- Modify: `firmware/bsp/esp32-s3-epaper-1.54-v2/README.md`

- [ ] **Step 1: Document BSP usage in root README**

Add a short section to `README.md`:

```markdown
## BSP

Reusable V2 BSP component:

- `firmware/bsp/esp32-s3-epaper-1.54-v2/`

Smoke-test app:

- `firmware/apps/board_smoke_test/`

Official examples remain unchanged under `firmware/official/v2/esp-idf/`.
```

- [ ] **Step 2: Expand BSP README**

Update `firmware/bsp/esp32-s3-epaper-1.54-v2/README.md` with:

```markdown
## Basic Usage

Add the BSP as an ESP-IDF extra component and include `board.h`.

```cpp
#include "board.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(board_init());
    ESP_ERROR_CHECK(board_init_display());
}
```

## Power Notes

- E-paper power is enabled by driving `BOARD_EPD_PWR_PIN` low.
- Audio power is enabled by driving `BOARD_AUDIO_PWR_PIN` low.
- VBAT hold is enabled by driving `BOARD_VBAT_PWR_PIN` high.
```

- [ ] **Step 3: Verify official examples untouched**

Run:

```bash
git diff 7408fa4 --name-only -- firmware/official/v2/esp-idf
```

Expected: no output.

- [ ] **Step 4: Verify BSP source references board pins**

Run:

```bash
rg -n "GPIO_NUM_|I2C_.*Address|EPD_.*PIN|Audio_PWR_PIN|VBAT_PWR_PIN" firmware/bsp/esp32-s3-epaper-1.54-v2 -g '!SensorLib/**'
```

Expected: raw `GPIO_NUM_` values appear in `board_pins.h` only, or in unavoidable ESP-IDF API type casts. Old official names do not appear.

- [ ] **Step 5: Verify Git status**

Run:

```bash
git status --short
```

Expected: only README/doc files modified before final commit.

- [ ] **Step 6: Commit docs**

```bash
git add README.md docs/official-overview.md hardware/pinout.md firmware/bsp/esp32-s3-epaper-1.54-v2/README.md
git commit -m "Document reusable BSP"
```

- [ ] **Step 7: Final build/status check**

Run:

```bash
git log --oneline -8
git status --short
```

Expected: recent commits show BSP extraction sequence; status is clean.

If ESP-IDF is available, also rerun:

```bash
cd firmware/apps/board_smoke_test
idf.py build
```

Expected: build exits 0.

---

## Plan Self-Review

- Design coverage: The plan creates the BSP component, central pin/config headers, power/I2C/display/buttons/sensors/SD/audio/touch modules, board umbrella API, smoke test app, docs, and verification steps.
- Official examples remain immutable: Every task includes checks to avoid modifying `firmware/official/v2/esp-idf/**`.
- TDD/build discipline: Embedded BSP behavior needs hardware for full runtime tests, so the plan uses staged build/static verification and a smoke test application. Hardware validation remains a required follow-up when the board is connected.
- Risk callout: Audio dependencies may require CMake adjustment because the official audio example depends on `codec_board`, `externlib`, and `espressif__esp_codec_dev`. Keep fixes local to BSP CMake and copied support components.
