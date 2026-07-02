#include "assistant_client.h"

#include "assistant_config.h"

#include <cJSON.h>

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
    (void)path;
    if (response != nullptr) {
        response->error = AssistantError::ProxyUnavailable;
    }
    return ESP_FAIL;
}
