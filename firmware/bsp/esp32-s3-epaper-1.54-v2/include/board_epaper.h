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
