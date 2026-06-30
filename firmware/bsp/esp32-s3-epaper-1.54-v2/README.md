# ESP32-S3 ePaper 1.54 V2 BSP

Reusable ESP-IDF board support package for the ESP32-S3 ePaper 1.54 V2 hardware.

Supported hardware:

- ESP32-S3-PICO-1-N8R8
- 8MB Flash
- 8MB PSRAM
- 1.54 inch 200 x 200 e-Paper display

The official examples remain under `firmware/official/v2/esp-idf/` and are used as the behavior reference for this BSP.

## Basic Usage

Add this directory as an ESP-IDF extra component and include `board.h` from the application.

```cpp
#include "board.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(board_init());
    ESP_ERROR_CHECK(board_init_display());
}
```

See `firmware/apps/board_smoke_test/` for a minimal app that initializes the board, reads RTC/SHTC3 data, draws an e-paper smoke pattern, and logs button state.

## Provided Modules

- `board.h`: high-level board initialization entry points
- `board_pins.h`: V2 pin map and I2C addresses
- `board_power.h`: e-paper, audio, and VBAT hold power control
- `board_i2c.h`: shared I2C bus helpers
- `board_epaper.h`: 1.54 inch e-paper init, clear, draw, and refresh helpers
- `board_sensors.h`: PCF85063 RTC and SHTC3 temperature/humidity helpers
- `board_buttons.h`: BOOT and PWR button scanning
- `board_touch.h`: FT6336 touch controller helpers for touch variants
- `board_sdcard.h`: SDMMC 1-bit Micro SD mount and file helpers
- `board_audio.h`: ES8311 audio codec initialization and cleanup

## Power Notes

- E-paper power is enabled by driving `BOARD_EPD_PWR_PIN` low.
- Audio power is enabled by driving `BOARD_AUDIO_PWR_PIN` low.
- VBAT hold is enabled by driving `BOARD_VBAT_PWR_PIN` high.
- `BOARD_PWR_BUTTON_PIN` is IO18 in the official V2 ESP-IDF examples.
- `BOARD_AUDIO_PWR_PIN` and `BOARD_PA_EN_PIN` currently map to IO42, matching the official `Audio_PWR_PIN` definition.
