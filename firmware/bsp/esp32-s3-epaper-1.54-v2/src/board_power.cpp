#include "board_power.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#include "esp_sleep.h"

static const char *TAG = "board_power";
static bool s_power_initialized = false;

esp_err_t board_power_init(void)
{
    if (s_power_initialized) {
        return ESP_OK;
    }

    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = (1ULL << BOARD_EPD_PWR_PIN) |
                             (1ULL << BOARD_AUDIO_PWR_PIN) |
                             (1ULL << BOARD_VBAT_PWR_PIN);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_RETURN_ON_ERROR(gpio_config(&gpio_conf), TAG, "configure power pins failed");

    s_power_initialized = true;
    return ESP_OK;
}

esp_err_t board_power_epaper_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_EPD_PWR_PIN, 0);
}

esp_err_t board_power_epaper_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_EPD_PWR_PIN, 1);
}

esp_err_t board_power_audio_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_AUDIO_PWR_PIN, 0);
}

esp_err_t board_power_audio_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_AUDIO_PWR_PIN, 1);
}

esp_err_t board_power_vbat_hold_on(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_VBAT_PWR_PIN, 1);
}

esp_err_t board_power_vbat_hold_off(void)
{
    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    return gpio_set_level(BOARD_VBAT_PWR_PIN, 0);
}

void board_enter_deep_sleep(void)
{
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    const uint64_t wake_mask = (1ULL << BOARD_BOOT_BUTTON_PIN) |
                               (1ULL << BOARD_RTC_INT_PIN) |
                               (1ULL << BOARD_PWR_BUTTON_PIN);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(BOARD_RTC_INT_PIN));
    ESP_ERROR_CHECK(rtc_gpio_pullup_en(BOARD_RTC_INT_PIN));
    ESP_ERROR_CHECK(rtc_gpio_hold_en(BOARD_VBAT_PWR_PIN));
    esp_deep_sleep_start();
}
