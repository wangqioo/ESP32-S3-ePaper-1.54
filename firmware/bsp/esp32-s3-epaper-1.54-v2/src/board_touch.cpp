#include "board_touch.h"

#include "board_i2c.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board_touch";
static i2c_master_dev_handle_t s_touch_dev = NULL;

static esp_err_t board_touch_reset(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BOARD_EPD_TP_RST_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "reset gpio config failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_EPD_TP_RST_PIN, 1), TAG, "reset high failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_EPD_TP_RST_PIN, 0), TAG, "reset low failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_EPD_TP_RST_PIN, 1), TAG, "reset release failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t board_touch_init(void)
{
    if (s_touch_dev != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    ESP_RETURN_ON_FALSE(board_i2c_get_bus() != NULL, ESP_ERR_INVALID_STATE, TAG, "i2c bus unavailable");
    i2c_master_dev_handle_t touch_dev = NULL;
    ESP_RETURN_ON_ERROR(board_i2c_register_device(BOARD_FT6336_I2C_ADDR, &touch_dev),
                        TAG,
                        "register touch failed");
    esp_err_t err = board_touch_reset();
    if (err != ESP_OK) {
        i2c_master_bus_rm_device(touch_dev);
        ESP_LOGE(TAG, "touch reset failed");
        return err;
    }
    s_touch_dev = touch_dev;
    return ESP_OK;
}

esp_err_t board_touch_read(board_touch_point_t *out_point)
{
    ESP_RETURN_ON_FALSE(out_point != NULL, ESP_ERR_INVALID_ARG, TAG, "out_point is null");
    ESP_RETURN_ON_ERROR(board_touch_init(), TAG, "touch init failed");

    out_point->touched = false;
    out_point->x = 0;
    out_point->y = 0;

    uint8_t touch_count = 0;
    ESP_RETURN_ON_ERROR(board_i2c_read_reg(s_touch_dev, 0x02, &touch_count, 1), TAG, "read count failed");
    if (touch_count == 0) {
        return ESP_OK;
    }

    uint8_t buf[4] = {};
    ESP_RETURN_ON_ERROR(board_i2c_read_reg(s_touch_dev, 0x03, buf, sizeof(buf)), TAG, "read point failed");

    uint16_t x = (((uint16_t)buf[0] & 0x0f) << 8) | (uint16_t)buf[1];
    uint16_t y = (((uint16_t)buf[2] & 0x0f) << 8) | (uint16_t)buf[3];
    if (x > BOARD_EPD_WIDTH) {
        x = BOARD_EPD_WIDTH;
    }
    if (y > BOARD_EPD_HEIGHT) {
        y = BOARD_EPD_HEIGHT;
    }

    out_point->touched = true;
    out_point->x = x;
    out_point->y = y;
    return ESP_OK;
}
