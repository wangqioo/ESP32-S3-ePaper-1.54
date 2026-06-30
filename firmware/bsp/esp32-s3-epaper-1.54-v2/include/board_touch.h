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
