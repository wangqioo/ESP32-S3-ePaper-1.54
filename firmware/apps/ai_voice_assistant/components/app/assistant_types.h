#pragma once

#include <cstdint>
#include <string>

enum class AssistantState {
    Idle,
    Recording,
    Saving,
    Uploading,
    Thinking,
    Answer,
    Error,
    ShuttingDown,
};

enum class AssistantEvent {
    BootDown,
    BootUp,
    UploadStarted,
    UploadSucceeded,
    UploadFailed,
    PowerShort,
    PowerLong,
};

enum class AssistantError {
    None,
    WifiUnavailable,
    ProxyUnavailable,
    UploadFailed,
    AiFailed,
    RecordFailed,
    StorageFailed,
    Busy,
};

struct AssistantSession {
    AssistantState state = AssistantState::Idle;
    AssistantError error = AssistantError::None;
    std::string answer;
    std::string request_id;
    uint32_t elapsed_seconds = 0;
};
