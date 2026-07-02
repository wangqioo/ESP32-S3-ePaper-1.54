#include "desktop_sleep.h"

#include "epaper_config.h"
#include "port_power.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr const char *TAG = "desk_sleep";

void PreparePowerButtonWake() {
    ESP_LOGI(TAG, "PWR level before sleep: %d", gpio_get_level(PWR_BUTTON_PIN));
    ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_init(PWR_BUTTON_PIN));
    ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_set_direction(PWR_BUTTON_PIN, RTC_GPIO_MODE_INPUT_ONLY));
    ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_pullup_en(PWR_BUTTON_PIN));
    ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_pulldown_dis(PWR_BUTTON_PIN));
}
}

void DesktopEnterTimedSleep(int minutes) {
    if (minutes <= 0) {
        minutes = 15;
    }
    ESP_LOGI(TAG, "Entering timed deep sleep for %d minutes", minutes);
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(minutes) * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

void DesktopEnterPowerSleep() {
    ESP_LOGI(TAG, "Cutting board power hold before PWR wake sleep");
    BoardPower_VBAT_OFF();
    ESP_LOGI(TAG, "Waiting for PWR release before deep sleep");
    while (gpio_get_level(PWR_BUTTON_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    ESP_LOGI(TAG, "Entering PWR wake deep sleep");
    PreparePowerButtonWake();
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_ext0_wakeup(PWR_BUTTON_PIN, 0));
    esp_deep_sleep_start();
}
