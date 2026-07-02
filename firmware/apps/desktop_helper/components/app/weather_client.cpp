#include "weather_client.h"

#include "desktop_config.h"

#include <string>
#include <cstring>

#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <cJSON.h>
#include <time.h>

namespace {

constexpr const char *TAG = "weather";
constexpr EventBits_t kWifiConnected = BIT0;
constexpr EventBits_t kWifiFailed = BIT1;

EventGroupHandle_t wifi_events = nullptr;
bool wifi_stack_initialized = false;
int retry_count = 0;

struct HttpBuffer {
    std::string body;
};

void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_data;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < 2) {
            retry_count++;
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(wifi_events, kWifiFailed);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_count = 0;
        xEventGroupSetBits(wifi_events, kWifiConnected);
    }
}

bool EnsureWifiStack() {
    if (wifi_stack_initialized) {
        return true;
    }
    wifi_events = xEventGroupCreate();
    if (wifi_events == nullptr) {
        return false;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, nullptr, nullptr));
    wifi_stack_initialized = true;
    return true;
}

bool ConnectWifi() {
    if (!WeatherCredentialsConfigured() || !EnsureWifiStack()) {
        return false;
    }

    xEventGroupClearBits(wifi_events, kWifiConnected | kWifiFailed);
    retry_count = 0;

    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), desktop_config::kWifiSsid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char *>(wifi_config.sta.password), desktop_config::kWifiPassword, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t start_ret = esp_wifi_start();
    if (start_ret != ESP_OK && start_ret != ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG, "wifi start failed: %s", esp_err_to_name(start_ret));
    }
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(
        wifi_events,
        kWifiConnected | kWifiFailed,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(desktop_config::kWifiConnectTimeoutMs));

    if (bits & kWifiConnected) {
        ESP_LOGI(TAG, "Wi-Fi connected");
        return true;
    }
    ESP_LOGW(TAG, "Wi-Fi unavailable");
    return false;
}

esp_err_t HttpEventHandler(esp_http_client_event_t *evt) {
    auto *buffer = static_cast<HttpBuffer *>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_DATA && buffer != nullptr && evt->data != nullptr && evt->data_len > 0) {
        buffer->body.append(static_cast<const char *>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

bool ParseWeatherJson(const std::string &body, WeatherSnapshot *weather) {
    if (weather == nullptr) {
        return false;
    }
    cJSON *root = cJSON_Parse(body.c_str());
    if (root == nullptr) {
        return false;
    }
    cJSON *current = cJSON_GetObjectItem(root, "current");
    cJSON *temp = current ? cJSON_GetObjectItem(current, "temperature_2m") : nullptr;
    cJSON *code = current ? cJSON_GetObjectItem(current, "weather_code") : nullptr;
    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code)) {
        cJSON_Delete(root);
        return false;
    }
    weather->temperature_c = static_cast<float>(temp->valuedouble);
    weather->weather_code = code->valueint;
    weather->fetched_unix = static_cast<int64_t>(time(nullptr));
    weather->valid = true;
    cJSON_Delete(root);
    return true;
}

}  // namespace

bool WeatherCredentialsConfigured() {
    return desktop_config::kWifiSsid != nullptr && desktop_config::kWifiSsid[0] != '\0';
}

bool SyncNetworkClock(int64_t *unix_seconds) {
    if (unix_seconds == nullptr) {
        return false;
    }
    *unix_seconds = 0;
    if (!ConnectWifi()) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(desktop_config::kNtpServer);
    esp_err_t init_ret = esp_netif_sntp_init(&config);
    if (init_ret != ESP_OK) {
        ESP_LOGW(TAG, "SNTP init failed: %s", esp_err_to_name(init_ret));
        esp_wifi_disconnect();
        esp_wifi_stop();
        return false;
    }

    esp_err_t sync_ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(desktop_config::kNtpSyncTimeoutMs));
    esp_netif_sntp_deinit();
    esp_wifi_disconnect();
    esp_wifi_stop();

    if (sync_ret != ESP_OK) {
        ESP_LOGW(TAG, "SNTP sync failed: %s", esp_err_to_name(sync_ret));
        return false;
    }
    time_t now = time(nullptr);
    struct tm utc = {};
    if (gmtime_r(&now, &utc) == nullptr || utc.tm_year + 1900 < 2024) {
        ESP_LOGW(TAG, "SNTP returned invalid time");
        return false;
    }
    *unix_seconds = static_cast<int64_t>(now);
    ESP_LOGI(TAG, "SNTP synced unix=%lld", static_cast<long long>(*unix_seconds));
    return true;
}

bool FetchWeather(WeatherSnapshot *weather) {
    if (weather == nullptr) {
        return false;
    }
    if (!ConnectWifi()) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    char url[256];
    snprintf(url, sizeof(url),
             "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code&timezone=auto",
             desktop_config::kLatitude,
             desktop_config::kLongitude);

    esp_err_t ret = ESP_FAIL;
    int status = 0;
    HttpBuffer buffer;
    for (int attempt = 0; attempt < 2; ++attempt) {
        buffer.body.clear();
        esp_http_client_config_t config = {};
        config.url = url;
        config.method = HTTP_METHOD_GET;
        config.timeout_ms = 8000;
        config.event_handler = HttpEventHandler;
        config.user_data = &buffer;
        config.disable_auto_redirect = false;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == nullptr) {
            break;
        }
        ret = esp_http_client_perform(client);
        status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);
        if (ret == ESP_OK && status >= 200 && status < 300) {
            break;
        }
        ESP_LOGW(TAG, "weather HTTP attempt %d failed: %s status=%d", attempt + 1, esp_err_to_name(ret), status);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
    esp_wifi_disconnect();
    esp_wifi_stop();

    if (ret != ESP_OK || status < 200 || status >= 300) {
        ESP_LOGW(TAG, "weather HTTP failed: %s status=%d", esp_err_to_name(ret), status);
        return false;
    }
    if (!ParseWeatherJson(buffer.body, weather)) {
        ESP_LOGW(TAG, "weather JSON parse failed");
        return false;
    }
    ESP_LOGI(TAG, "weather %.1f code %d", weather->temperature_c, weather->weather_code);
    return true;
}
