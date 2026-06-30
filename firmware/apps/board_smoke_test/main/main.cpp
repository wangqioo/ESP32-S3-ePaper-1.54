#include <algorithm>
#include <cstdint>

#include "board.h"
#include "board_buttons.h"
#include "board_epaper.h"
#include "board_pins.h"
#include "board_sensors.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

const char *TAG = "board_smoke_test";

void log_rtc_if_available()
{
    board_rtc_time_t rtc_time = {};
    esp_err_t err = board_rtc_get_time(&rtc_time);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "RTC unavailable: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG,
             "RTC: %04u-%02u-%02u %02u:%02u:%02u week=%u",
             static_cast<unsigned>(rtc_time.year),
             static_cast<unsigned>(rtc_time.month),
             static_cast<unsigned>(rtc_time.day),
             static_cast<unsigned>(rtc_time.hour),
             static_cast<unsigned>(rtc_time.minute),
             static_cast<unsigned>(rtc_time.second),
             static_cast<unsigned>(rtc_time.week));
}

void log_shtc3_if_available()
{
    board_shtc3_data_t shtc3_data = {};
    esp_err_t err = board_shtc3_read_data(&shtc3_data);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SHTC3 unavailable: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG,
             "SHTC3: temperature=%.2f C humidity=%.2f%%",
             static_cast<double>(shtc3_data.temperature_c),
             static_cast<double>(shtc3_data.humidity_percent));
}

void draw_x_pattern()
{
    board_epaper_clear();

    constexpr uint16_t width = 200;
    constexpr uint16_t height = 200;
    constexpr uint16_t limit = std::min(width, height);

    for (uint16_t i = 0; i < limit; ++i) {
        board_epaper_draw_pixel(i, i, BOARD_EPD_COLOR_BLACK);
        board_epaper_draw_pixel(width - 1 - i, i, BOARD_EPD_COLOR_BLACK);

        if (i + 1 < height) {
            board_epaper_draw_pixel(i, i + 1, BOARD_EPD_COLOR_BLACK);
            board_epaper_draw_pixel(width - 1 - i, i + 1, BOARD_EPD_COLOR_BLACK);
        }
        if (i > 0) {
            board_epaper_draw_pixel(i, i - 1, BOARD_EPD_COLOR_BLACK);
            board_epaper_draw_pixel(width - 1 - i, i - 1, BOARD_EPD_COLOR_BLACK);
        }
    }

    board_epaper_flush_full();
}

} // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Board: %s hw=%s", BOARD_NAME, BOARD_HW_VERSION);

    ESP_ERROR_CHECK(board_init());

    esp_err_t sensor_err = board_init_sensors();
    if (sensor_err == ESP_OK) {
        log_rtc_if_available();
        log_shtc3_if_available();
    } else {
        ESP_LOGW(TAG, "Sensor init unavailable: %s", esp_err_to_name(sensor_err));
    }

    esp_err_t display_err = board_init_display();
    if (display_err == ESP_OK) {
        draw_x_pattern();
        ESP_LOGI(TAG, "Display smoke pattern drawn");
    } else {
        ESP_LOGW(TAG, "Display init unavailable: %s", esp_err_to_name(display_err));
    }

    while (true) {
        ESP_LOGI(TAG,
                 "button repeat counts: boot=%u pwr=%u",
                 static_cast<unsigned>(board_button_boot_repeat_count()),
                 static_cast<unsigned>(board_button_pwr_repeat_count()));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
