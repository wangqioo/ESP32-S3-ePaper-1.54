#include "desktop_cache.h"

#include <stdio.h>
#include <stdlib.h>

#if __has_include(<nvs.h>)
#include <nvs.h>
#include <esp_log.h>
#define DESKTOP_HAS_NVS 1
#else
#define DESKTOP_HAS_NVS 0
#endif

namespace {

constexpr const char *kNamespace = "desk";
constexpr const char *kWeatherKey = "weather";

}  // namespace

std::string SerializeWeatherCache(const WeatherSnapshot &weather) {
    if (!weather.valid) {
        return "";
    }
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "%.2f,%d,%lld",
             static_cast<double>(weather.temperature_c),
             weather.weather_code,
             static_cast<long long>(weather.fetched_unix));
    return buffer;
}

bool ParseWeatherCache(const std::string &data, WeatherSnapshot *weather) {
    if (weather == nullptr || data.empty()) {
        return false;
    }
    float temp = 0.0f;
    int code = -1;
    long long fetched = 0;
    if (sscanf(data.c_str(), "%f,%d,%lld", &temp, &code, &fetched) != 3) {
        return false;
    }
    weather->temperature_c = temp;
    weather->weather_code = code;
    weather->fetched_unix = fetched;
    weather->valid = true;
    return true;
}

bool LoadWeatherCache(WeatherSnapshot *weather) {
#if DESKTOP_HAS_NVS
    if (weather == nullptr) {
        return false;
    }
    nvs_handle_t handle = 0;
    if (nvs_open(kNamespace, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }
    size_t len = 0;
    esp_err_t ret = nvs_get_str(handle, kWeatherKey, nullptr, &len);
    if (ret != ESP_OK || len == 0) {
        nvs_close(handle);
        return false;
    }
    std::string data(len, '\0');
    ret = nvs_get_str(handle, kWeatherKey, data.data(), &len);
    nvs_close(handle);
    if (ret != ESP_OK) {
        return false;
    }
    if (!data.empty() && data.back() == '\0') {
        data.pop_back();
    }
    return ParseWeatherCache(data, weather);
#else
    (void)weather;
    return false;
#endif
}

bool SaveWeatherCache(const WeatherSnapshot &weather) {
#if DESKTOP_HAS_NVS
    std::string data = SerializeWeatherCache(weather);
    if (data.empty()) {
        return false;
    }
    nvs_handle_t handle = 0;
    if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }
    esp_err_t ret = nvs_set_str(handle, kWeatherKey, data.c_str());
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret == ESP_OK;
#else
    (void)weather;
    return false;
#endif
}

