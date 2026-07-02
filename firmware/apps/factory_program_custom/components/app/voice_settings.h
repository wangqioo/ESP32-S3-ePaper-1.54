#ifndef VOICE_SETTINGS_H
#define VOICE_SETTINGS_H

#include "ui_sfx.h"

#include <cstdint>

enum class VoiceSettingsItem : uint8_t {
    Sound = 0,
    Volume,
    RecordStartSound,
    RecordMode,
    ClearAll,
};

enum class VoiceRecordMode : uint8_t {
    Hold = 0,
    Tap,
};

struct VoiceSettings {
    bool sfx_enabled = true;
    UiSfxVolume sfx_volume = UiSfxVolume::Medium;
    bool record_start_sfx_enabled = false;
    VoiceRecordMode record_mode = VoiceRecordMode::Hold;
};

constexpr uint8_t kVoiceSettingsItemCount = 5;

VoiceSettingsItem VoiceSettingsItemForRow(uint8_t row);
void ApplyVoiceSettingsItem(VoiceSettings *settings, VoiceSettingsItem item);
const char *VoiceSettingsVolumeLabel(UiSfxVolume volume);
const char *VoiceSettingsRecordModeLabel(VoiceRecordMode mode);

#endif
