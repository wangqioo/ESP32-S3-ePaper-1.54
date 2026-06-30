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
