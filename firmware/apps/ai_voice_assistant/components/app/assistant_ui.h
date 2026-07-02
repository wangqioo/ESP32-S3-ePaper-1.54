#pragma once

#include "assistant_types.h"

#include <cstdint>
#include <string>

class AssistantUi {
public:
    void Init();
    void ShowHome(bool wifi_configured, bool proxy_configured);
    void ShowRecording(uint32_t elapsed_seconds);
    void ShowBusy(AssistantState state);
    void ShowAnswer(const std::string &answer, const std::string &request_id);
    void ShowError(AssistantError error);
    void ShowPowerOff();

private:
    void Clear();
};
