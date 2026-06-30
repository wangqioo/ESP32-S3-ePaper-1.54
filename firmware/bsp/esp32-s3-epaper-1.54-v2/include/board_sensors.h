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
