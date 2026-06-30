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
