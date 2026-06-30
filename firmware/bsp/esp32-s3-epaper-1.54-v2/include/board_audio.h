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
