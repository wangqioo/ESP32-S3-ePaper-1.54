#include "board.h"

#include "board_audio.h"
#include "board_buttons.h"
#include "board_epaper.h"
#include "board_i2c.h"
#include "board_power.h"
#include "board_sensors.h"
#include "board_sdcard.h"
#include "board_touch.h"
#include "esp_check.h"

static const char *TAG = "board";

esp_err_t board_init(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    ESP_RETURN_ON_ERROR(board_power_vbat_hold_on(), TAG, "vbat hold failed");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    ESP_RETURN_ON_ERROR(board_buttons_init(), TAG, "buttons init failed");
    return ESP_OK;
}

esp_err_t board_init_display(void)
{
    return board_epaper_init();
}

esp_err_t board_init_sensors(void)
{
    return board_sensors_init();
}

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
