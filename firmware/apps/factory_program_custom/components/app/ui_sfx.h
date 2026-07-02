#ifndef UI_SFX_H
#define UI_SFX_H

#include <cstddef>
#include <cstdint>

#include <esp_err.h>

enum class UiSfxEvent {
    Touch,
    Navigate,
    Delete,
    Confirm,
    Cancel,
    Error,
    RecordStart,
    RecordStop,
    SaveDone,
    Power,
};

enum class UiSfxVolume : uint8_t {
    Low = 0,
    Medium,
    High,
};

struct UiSfxSpec {
    uint16_t frequency_hz = 0;
    uint16_t duration_ms = 0;
    int16_t amplitude = 0;
    uint16_t post_delay_ms = 0;
    uint16_t discard_record_ms = 0;
};

void SetUiSfxEnabled(bool enabled);
bool IsUiSfxEnabled();
void SetUiSfxVolume(UiSfxVolume volume);
UiSfxVolume GetUiSfxVolume();
UiSfxSpec UiSfxSpecForEvent(UiSfxEvent event);
size_t UiSfxFrameCount(const UiSfxSpec &spec);
void FillUiSfxPcm(const UiSfxSpec &spec, int16_t *stereo_samples, size_t frames);
esp_err_t PlayUiSfx(UiSfxEvent event);

#endif
