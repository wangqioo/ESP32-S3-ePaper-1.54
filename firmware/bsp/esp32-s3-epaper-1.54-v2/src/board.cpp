#include "board.h"

#include "board_i2c.h"
#include "board_power.h"
#include "esp_check.h"

static const char *TAG = "board";

esp_err_t board_init(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    ESP_RETURN_ON_ERROR(board_power_vbat_hold_on(), TAG, "vbat hold failed");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    return ESP_OK;
}

esp_err_t board_init_display(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sensors(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_audio(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_sdcard(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_init_touch(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
