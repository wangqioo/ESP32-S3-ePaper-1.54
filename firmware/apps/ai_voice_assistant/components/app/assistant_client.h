#pragma once

#include "assistant_types.h"

#include <esp_err.h>

#include <string>

struct AssistantResponse {
    bool ok = false;
    std::string text;
    std::string request_id;
    AssistantError error = AssistantError::None;
};

bool AssistantWifiConfigured();
bool AssistantProxyConfigured();
bool ParseAssistantResponseJson(const std::string &json, AssistantResponse *response);
esp_err_t AssistantUploadWav(const char *path, AssistantResponse *response);
