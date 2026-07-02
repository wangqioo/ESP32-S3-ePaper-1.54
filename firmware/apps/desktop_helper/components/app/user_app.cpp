#include "user_app.h"

#include "desktop_cache.h"
#include "desktop_config.h"
#include "desktop_sleep.h"
#include "desktop_ui.h"
#include "epaper_config.h"
#include "pcf85063a.h"
#include "port_adc.h"
#include "port_i2c.h"
#include "port_lvgl.h"
#include "port_power.h"
#include "port_shtc3.h"
#include "weather_client.h"

#include <assert.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

namespace {

constexpr const char *TAG = "desk_app";
constexpr EventBits_t kEventBootPress = BIT0;
constexpr EventBits_t kEventPowerPress = BIT1;
constexpr EventBits_t kEventPowerLong = BIT2;
constexpr TickType_t kButtonPollTicks = pdMS_TO_TICKS(30);
constexpr int kPowerLongPressMs = 1200;
constexpr int kClockRefreshIntervalMs = 60 * 1000;

I2cMasterBus *i2c_bus = nullptr;
pcf85063a_dev_t rtc = {};
bool rtc_ready = false;
EventGroupHandle_t app_events = nullptr;
DesktopUi ui;
DesktopSnapshot snapshot;
DesktopPanel current_panel = DesktopPanel::Weather;

void ShowLoading(const char *message) {
    if (Lvgl_lock(-1)) {
        ui.ShowLoading(message);
        Lvgl_unlock();
    }
}

void ShowCurrent() {
    if (Lvgl_lock(-1)) {
        if (current_panel == DesktopPanel::Clock) {
            ui.ShowClock(snapshot);
        } else {
            ui.ShowWeather(snapshot);
        }
        Lvgl_unlock();
    }
}

void LogWakeupCause() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "Wakeup cause: timer");
        break;
    case ESP_SLEEP_WAKEUP_EXT0:
        ESP_LOGI(TAG, "Wakeup cause: PWR button");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "Wakeup cause: external GPIO");
        break;
    default:
        ESP_LOGI(TAG, "Wakeup cause: reset/power-on");
        break;
    }
}

bool IsPressed(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

void InitButtonGpio() {
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << BOOT_BUTTON_PIN) | (1ULL << PWR_BUTTON_PIN);
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));
}

void ButtonTask(void *arg) {
    (void)arg;
    bool boot_prev = IsPressed(BOOT_BUTTON_PIN);
    bool pwr_prev = IsPressed(PWR_BUTTON_PIN);
    int pwr_held_ms = 0;
    bool pwr_long_sent = false;

    for (;;) {
        bool boot_now = IsPressed(BOOT_BUTTON_PIN);
        bool pwr_now = IsPressed(PWR_BUTTON_PIN);

        if (boot_prev && !boot_now) {
            xEventGroupSetBits(app_events, kEventBootPress);
        }

        if (pwr_now) {
            pwr_held_ms += 30;
            if (!pwr_long_sent && pwr_held_ms >= kPowerLongPressMs) {
                pwr_long_sent = true;
                xEventGroupSetBits(app_events, kEventPowerLong);
            }
        } else {
            if (pwr_prev && !pwr_long_sent) {
                xEventGroupSetBits(app_events, kEventPowerPress);
            }
            pwr_held_ms = 0;
            pwr_long_sent = false;
        }

        boot_prev = boot_now;
        pwr_prev = pwr_now;
        vTaskDelay(kButtonPollTicks);
    }
}

DesktopTime ReadRtcTime() {
    DesktopTime time;
    if (!rtc_ready) {
        return time;
    }
    pcf85063a_datetime_t now = {};
    if (pcf85063a_get_time_date(&rtc, &now) != ESP_OK) {
        return time;
    }
    if (now.hour > 23 || now.min > 59 || now.month == 0 || now.month > 12 || now.day == 0 || now.day > 31) {
        return time;
    }
    time.year = now.year;
    time.month = now.month;
    time.day = now.day;
    time.hour = now.hour;
    time.minute = now.min;
    time.second = now.sec;
    time.weekday = now.dotw;
    time.valid = true;
    return time;
}

bool SyncRtcFromNetwork() {
    if (!rtc_ready) {
        return false;
    }
    int64_t unix_seconds = 0;
    if (!SyncNetworkClock(&unix_seconds)) {
        return false;
    }
    DesktopTime local;
    if (!ConvertUnixToLocalDesktopTime(unix_seconds, desktop_config::kTimezoneOffsetSeconds, &local) || local.year < 2024) {
        ESP_LOGW(TAG, "network time conversion failed");
        return false;
    }

    pcf85063a_datetime_t rtc_time = {};
    rtc_time.year = local.year;
    rtc_time.month = local.month;
    rtc_time.day = local.day;
    rtc_time.dotw = local.weekday;
    rtc_time.hour = local.hour;
    rtc_time.min = local.minute;
    rtc_time.sec = local.second;

    esp_err_t ret = pcf85063a_set_time_date(&rtc, rtc_time);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC write failed: %s", esp_err_to_name(ret));
        return false;
    }
    ESP_LOGI(TAG, "RTC synced %04u-%02u-%02u %02u:%02u:%02u",
             local.year, local.month, local.day, local.hour, local.minute, local.second);
    snapshot.time = local;
    return true;
}

IndoorSnapshot ReadIndoor() {
    IndoorSnapshot indoor;
    float temp = 0.0f;
    float humi = 0.0f;
    Shtc3_ReadTempHumi(&temp, &humi);
    if (temp > -100.0f && humi >= 0.0f && humi <= 100.0f) {
        indoor.temperature_c = temp;
        indoor.humidity_percent = humi;
        indoor.valid = true;
    }
    return indoor;
}

BatterySnapshot ReadBattery() {
    BatterySnapshot battery;
    battery.voltage = Get_VbatVoltage();
    battery.percent = Get_Batterylevel();
    battery.valid = battery.voltage > 2.0f;
    return battery;
}

void ReadLocalSensors() {
    snapshot.time = ReadRtcTime();
    snapshot.indoor = ReadIndoor();
    snapshot.battery = ReadBattery();
}

void RenderCurrent() {
    ShowCurrent();
}

void TogglePanel() {
    current_panel = current_panel == DesktopPanel::Weather ? DesktopPanel::Clock : DesktopPanel::Weather;
    ESP_LOGI(TAG, "Panel switched to %s", current_panel == DesktopPanel::Clock ? "clock" : "weather");
    ReadLocalSensors();
    RenderCurrent();
}

void RefreshWeather() {
    ESP_LOGI(TAG, "Refreshing desktop helper");
    ShowLoading("Reading sensors");
    ReadLocalSensors();

    WeatherSnapshot cached;
    bool has_cache = LoadWeatherCache(&cached);
    if (has_cache) {
        snapshot.weather = cached;
        snapshot.weather_stale = true;
    }

    snapshot.wifi_attempted = WeatherCredentialsConfigured();
    snapshot.wifi_connected = false;

    if (WeatherCredentialsConfigured()) {
        ShowLoading("Syncing clock");
        if (!SyncRtcFromNetwork()) {
            ESP_LOGW(TAG, "clock sync skipped or failed");
        }

        ShowLoading("Fetching weather");
        WeatherSnapshot fresh;
        if (FetchWeather(&fresh)) {
            snapshot.weather = fresh;
            snapshot.weather_stale = false;
            snapshot.wifi_connected = true;
            SaveWeatherCache(fresh);
        } else if (has_cache) {
            snapshot.weather = cached;
            snapshot.weather_stale = true;
        } else {
            snapshot.weather = {};
            snapshot.weather_stale = false;
        }
    } else {
        ESP_LOGW(TAG, "Wi-Fi credentials empty, offline mode");
    }

    RenderCurrent();
}

void AppTask(void *arg) {
    (void)arg;
    RefreshWeather();

    int refresh_due_ms = 0;
    int clock_due_ms = 0;
    for (;;) {
        EventBits_t events = xEventGroupWaitBits(
            app_events,
            kEventBootPress | kEventPowerPress | kEventPowerLong,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(500));

        if (events & kEventPowerLong) {
            ShowLoading("Sleeping");
            vTaskDelay(pdMS_TO_TICKS(250));
            DesktopEnterPowerSleep();
        }
        if (events & kEventPowerPress) {
            TogglePanel();
        }
        if (events & kEventBootPress) {
            current_panel = DesktopPanel::Weather;
            RefreshWeather();
            refresh_due_ms = 0;
            clock_due_ms = 0;
        }

        refresh_due_ms += 500;
        clock_due_ms += 500;
        if (clock_due_ms >= kClockRefreshIntervalMs) {
            if (PanelNeedsClockRefresh(current_panel)) {
                DesktopTime current_time = ReadRtcTime();
                if (current_time.valid) {
                    snapshot.time = current_time;
                    RenderCurrent();
                }
            }
            clock_due_ms = 0;
        }

        if (refresh_due_ms >= desktop_config::kRefreshIntervalMs) {
            RefreshWeather();
            refresh_due_ms = 0;
            clock_due_ms = 0;
        }
    }
}

}  // namespace

void UserApp_Init() {
    LogWakeupCause();
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_VBAT_ON();
    BoardPower_Audio_OFF();
    InitButtonGpio();

    app_events = xEventGroupCreate();
    assert(app_events);

    i2c_bus = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
    assert(i2c_bus);

    esp_err_t rtc_ret = pcf85063a_init(&rtc, i2c_bus->Get_I2cBusHandle(), PCF85063A_ADDRESS);
    rtc_ready = rtc_ret == ESP_OK;
    if (!rtc_ready) {
        ESP_LOGW(TAG, "RTC init failed: %s", esp_err_to_name(rtc_ret));
    }

    BoardAdc_Init();
    Shtc3_Init(i2c_bus);
}

void UserUi_Init() {
    ui.Init();
    ui.ShowLoading("Starting");
}

void UserApp_Start() {
    xTaskCreatePinnedToCore(ButtonTask, "DeskButtons", 3 * 1024, nullptr, 4, nullptr, 1);
    xTaskCreatePinnedToCore(AppTask, "DeskApp", 8 * 1024, nullptr, 3, nullptr, 1);
}
