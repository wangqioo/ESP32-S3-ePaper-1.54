#include "voice_settings.h"

VoiceSettingsItem VoiceSettingsItemForRow(uint8_t row) {
    switch (row) {
    case 0:
        return VoiceSettingsItem::Sound;
    case 1:
        return VoiceSettingsItem::Volume;
    case 2:
        return VoiceSettingsItem::RecordStartSound;
    case 3:
        return VoiceSettingsItem::RecordMode;
    case 4:
    default:
        return VoiceSettingsItem::ClearAll;
    }
}

void ApplyVoiceSettingsItem(VoiceSettings *settings, VoiceSettingsItem item) {
    if (settings == nullptr) {
        return;
    }

    switch (item) {
    case VoiceSettingsItem::Sound:
        settings->sfx_enabled = !settings->sfx_enabled;
        break;
    case VoiceSettingsItem::Volume:
        if (settings->sfx_volume == UiSfxVolume::Low) {
            settings->sfx_volume = UiSfxVolume::Medium;
        } else if (settings->sfx_volume == UiSfxVolume::Medium) {
            settings->sfx_volume = UiSfxVolume::High;
        } else {
            settings->sfx_volume = UiSfxVolume::Low;
        }
        break;
    case VoiceSettingsItem::RecordStartSound:
        settings->record_start_sfx_enabled = !settings->record_start_sfx_enabled;
        break;
    case VoiceSettingsItem::RecordMode:
        settings->record_mode = settings->record_mode == VoiceRecordMode::Hold ? VoiceRecordMode::Tap : VoiceRecordMode::Hold;
        break;
    case VoiceSettingsItem::ClearAll:
        break;
    }
}

const char *VoiceSettingsVolumeLabel(UiSfxVolume volume) {
    switch (volume) {
    case UiSfxVolume::Low:
        return "LOW";
    case UiSfxVolume::Medium:
        return "MID";
    case UiSfxVolume::High:
        return "HIGH";
    }
    return "MID";
}

const char *VoiceSettingsRecordModeLabel(VoiceRecordMode mode) {
    switch (mode) {
    case VoiceRecordMode::Hold:
        return "HOLD";
    case VoiceRecordMode::Tap:
        return "TAP";
    }
    return "HOLD";
}
