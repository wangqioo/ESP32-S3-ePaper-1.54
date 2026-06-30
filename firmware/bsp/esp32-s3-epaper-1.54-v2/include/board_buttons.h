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
