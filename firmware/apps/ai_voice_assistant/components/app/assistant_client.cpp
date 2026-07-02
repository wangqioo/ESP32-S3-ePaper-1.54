#include "assistant_client.h"

#include "assistant_config.h"

#include <cJSON.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr const char *TAG = "assistant_client";
constexpr EventBits_t kWifiConnected = BIT0;
constexpr EventBits_t kWifiFailed = BIT1;

EventGroupHandle_t wifi_events = nullptr;
bool wifi_initialized = false;
int retry_count = 0;

struct HttpBuffer {
    std::string body;
};

void WifiEventHandler(void *, esp_event_base_t event_base, int32_t event_id, void *) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < 2) {
            retry_count++;
            esp_wifi_connect();
        } else if (wifi_events != nullptr) {
            xEventGroupSetBits(wifi_events, kWifiFailed);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP && wifi_events != nullptr) {
        retry_count = 0;
        xEventGroupSetBits(wifi_events, kWifiConnected);
    }
}

esp_err_t HttpEventHandler(esp_http_client_event_t *evt) {
    auto *buffer = static_cast<HttpBuffer *>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_DATA && buffer != nullptr && evt->data != nullptr && evt->data_len > 0) {
        buffer->body.append(static_cast<const char *>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

bool EnsureWifiStack() {
    if (wifi_initialized) {
        return true;
    }
    wifi_events = xEventGroupCreate();
    if (wifi_events == nullptr) {
        return false;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t loop_ret = esp_event_loop_create_default();
    if (loop_ret != ESP_OK && loop_ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "event loop init failed: %s", esp_err_to_name(loop_ret));
        return false;
    }
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, nullptr, nullptr));
    wifi_initialized = true;
    return true;
}

bool ConnectWifi() {
    if (!AssistantWifiConfigured() || !EnsureWifiStack()) {
        return false;
    }

    xEventGroupClearBits(wifi_events, kWifiConnected | kWifiFailed);
    retry_count = 0;

    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), assistant_config::kWifiSsid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char *>(wifi_config.sta.password), assistant_config::kWifiPassword, sizeof(wifi_config.sta.password) - 1);
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
        pdMS_TO_TICKS(assistant_config::kWifiConnectTimeoutMs));

    if (bits & kWifiConnected) {
        return true;
    }
    ESP_LOGW(TAG, "Wi-Fi unavailable");
    esp_wifi_disconnect();
    esp_wifi_stop();
    return false;
}

bool ReadFile(const char *path, std::string *data) {
    if (path == nullptr || data == nullptr) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long size = ftell(file);
    if (size <= 0) {
        fclose(file);
        return false;
    }
    rewind(file);
    data->assign(static_cast<size_t>(size), '\0');
    size_t read = fread(data->data(), 1, data->size(), file);
    fclose(file);
    return read == data->size();
}

}  // namespace

bool AssistantWifiConfigured() {
    return assistant_config::WifiConfigured();
}

bool AssistantProxyConfigured() {
    return assistant_config::ProxyConfigured();
}

bool ParseAssistantResponseJson(const std::string &json, AssistantResponse *response) {
    if (response == nullptr) {
        return false;
    }
    cJSON *root = cJSON_Parse(json.c_str());
    if (root == nullptr) {
        response->ok = false;
        response->error = AssistantError::AiFailed;
        return false;
    }
    cJSON *ok = cJSON_GetObjectItem(root, "ok");
    cJSON *text = cJSON_GetObjectItem(root, "text");
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    cJSON *error = cJSON_GetObjectItem(root, "error");
    response->ok = cJSON_IsTrue(ok);
    if (cJSON_IsString(text)) {
        response->text = text->valuestring;
    }
    if (cJSON_IsString(request_id)) {
        response->request_id = request_id->valuestring;
    }
    if (!response->ok) {
        response->error = cJSON_IsString(error) ? AssistantError::AiFailed : AssistantError::UploadFailed;
    }
    cJSON_Delete(root);
    return response->ok && !response->text.empty();
}

esp_err_t AssistantUploadWav(const char *path, AssistantResponse *response) {
    if (response == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *response = AssistantResponse{};
    if (!AssistantProxyConfigured()) {
        response->error = AssistantError::ProxyUnavailable;
        return ESP_FAIL;
    }
    if (!ConnectWifi()) {
        response->error = AssistantError::WifiUnavailable;
        return ESP_FAIL;
    }

    std::string wav;
    if (!ReadFile(path, &wav)) {
        response->error = AssistantError::StorageFailed;
        esp_wifi_disconnect();
        esp_wifi_stop();
        return ESP_FAIL;
    }

    char url[192];
    snprintf(url, sizeof(url), "http://%s:%u%s", assistant_config::kProxyHost, assistant_config::kProxyPort, assistant_config::kAskPath);

    HttpBuffer buffer;
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = assistant_config::kUploadTimeoutMs;
    config.event_handler = HttpEventHandler;
    config.user_data = &buffer;
    config.disable_auto_redirect = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        response->error = AssistantError::ProxyUnavailable;
        esp_wifi_disconnect();
        esp_wifi_stop();
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_post_field(client, wav.data(), wav.size());
    esp_err_t ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    esp_wifi_disconnect();
    esp_wifi_stop();

    if (ret != ESP_OK || status < 200 || status >= 300) {
        ESP_LOGW(TAG, "upload failed ret=%s status=%d", esp_err_to_name(ret), status);
        response->error = status == 0 ? AssistantError::ProxyUnavailable : AssistantError::UploadFailed;
        return ESP_FAIL;
    }
    if (!ParseAssistantResponseJson(buffer.body, response)) {
        if (response->error == AssistantError::None) {
            response->error = AssistantError::AiFailed;
        }
        return ESP_FAIL;
    }
    return ESP_OK;
}
