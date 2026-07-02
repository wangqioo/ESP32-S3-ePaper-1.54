#ifndef MEMO_TYPES_H
#define MEMO_TYPES_H

#include <cstdint>
#include <string>

struct WavFormat {
    uint32_t sample_rate = 16000;
    uint16_t bits_per_sample = 16;
    uint16_t channels = 2;
};

struct MemoMetadata {
    uint32_t sequence = 0;
    std::string path;
    std::string display_time;
    uint32_t duration_seconds = 0;
    uint32_t data_bytes = 0;
    bool starred = false;
};

enum class VoiceAppState {
    List,
    Recording,
    Saving,
    Detail,
    Playing,
    DeleteConfirm,
    Settings,
    ClearAllConfirm,
    Error,
    Shutdown,
};

enum class VoiceUiActionType {
    None,
    NextPage,
    PreviousPage,
    SelectRow,
    PlayStop,
    Delete,
    Star,
    ConfirmDelete,
    ConfirmClearAll,
    Cancel,
    Back,
};

struct VoiceUiAction {
    VoiceUiActionType type = VoiceUiActionType::None;
    uint8_t row = 0;
};

#endif
