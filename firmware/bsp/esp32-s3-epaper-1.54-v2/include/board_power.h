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
