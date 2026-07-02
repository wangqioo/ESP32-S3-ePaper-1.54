#include "ui_sfx.h"

#ifdef ESP_PLATFORM
#include "port_codec.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#include <algorithm>

namespace {

constexpr uint32_t kSampleRate = 16000;
constexpr size_t kChannels = 2;
constexpr uint16_t kMaxDurationMs = 120;
constexpr size_t kMaxFrames = kSampleRate * kMaxDurationMs / 1000;
constexpr size_t kDrainBytes = 2048;

int16_t sfx_buffer[kMaxFrames * kChannels];
#ifdef ESP_PLATFORM
uint8_t drain_buffer[kDrainBytes];
#endif
bool sfx_enabled = true;
UiSfxVolume sfx_volume = UiSfxVolume::Medium;

int16_t ApplyEnvelope(int16_t sample, size_t frame, size_t frames) {
    if (frames == 0) {
        return 0;
    }

    size_t fade_frames = std::min<size_t>(frames / 4, kSampleRate * 6 / 1000);
    if (fade_frames == 0) {
        return sample;
    }

    size_t gain_num = fade_frames;
    if (frame < fade_frames) {
        gain_num = frame;
    } else if (frame + fade_frames >= frames) {
        gain_num = frames - frame - 1;
    }
    return static_cast<int16_t>((static_cast<int32_t>(sample) * static_cast<int32_t>(gain_num)) / static_cast<int32_t>(fade_frames));
}

} // namespace

void SetUiSfxEnabled(bool enabled) {
    sfx_enabled = enabled;
}

bool IsUiSfxEnabled() {
    return sfx_enabled;
}

void SetUiSfxVolume(UiSfxVolume volume) {
    sfx_volume = volume;
}

UiSfxVolume GetUiSfxVolume() {
    return sfx_volume;
}

UiSfxSpec UiSfxSpecForEvent(UiSfxEvent event) {
    switch (event) {
    case UiSfxEvent::Touch:
        return {1800, 28, 4200, 0, 0};
    case UiSfxEvent::Navigate:
        return {1300, 35, 3800, 0, 0};
    case UiSfxEvent::Delete:
        return {650, 55, 4200, 0, 0};
    case UiSfxEvent::Confirm:
        return {1500, 45, 4200, 0, 0};
    case UiSfxEvent::Cancel:
        return {900, 35, 3600, 0, 0};
    case UiSfxEvent::Error:
        return {360, 100, 4600, 0, 0};
    case UiSfxEvent::RecordStart:
        return {1200, 55, 4500, 120, 260};
    case UiSfxEvent::RecordStop:
        return {800, 45, 4200, 0, 0};
    case UiSfxEvent::SaveDone:
        return {1700, 70, 4200, 0, 0};
    case UiSfxEvent::Power:
        return {520, 90, 4200, 0, 0};
    }
    return {};
}

size_t UiSfxFrameCount(const UiSfxSpec &spec) {
    return std::min<size_t>(kMaxFrames, (static_cast<size_t>(spec.duration_ms) * kSampleRate) / 1000);
}

void FillUiSfxPcm(const UiSfxSpec &spec, int16_t *stereo_samples, size_t frames) {
    if (stereo_samples == nullptr || spec.frequency_hz == 0 || spec.amplitude == 0) {
        return;
    }

    uint32_t phase = 0;
    for (size_t frame = 0; frame < frames; ++frame) {
        phase += spec.frequency_hz;
        if (phase >= kSampleRate) {
            phase -= kSampleRate;
        }

        int16_t sample = phase < (kSampleRate / 2) ? spec.amplitude : static_cast<int16_t>(-spec.amplitude);
        sample = ApplyEnvelope(sample, frame, frames);
        stereo_samples[frame * 2] = sample;
        stereo_samples[frame * 2 + 1] = sample;
    }
}

esp_err_t PlayUiSfx(UiSfxEvent event) {
#ifdef ESP_PLATFORM
    if (!sfx_enabled) {
        return ESP_OK;
    }

    UiSfxSpec spec = UiSfxSpecForEvent(event);
    switch (sfx_volume) {
    case UiSfxVolume::Low:
        spec.amplitude = static_cast<int16_t>(spec.amplitude / 2);
        break;
    case UiSfxVolume::High:
        spec.amplitude = static_cast<int16_t>(std::min<int>(spec.amplitude * 3 / 2, 9000));
        break;
    case UiSfxVolume::Medium:
    default:
        break;
    }

    size_t frames = UiSfxFrameCount(spec);
    if (frames == 0) {
        return ESP_OK;
    }

    FillUiSfxPcm(spec, sfx_buffer, frames);
    esp_err_t ret = Codec_PlaybackData(reinterpret_cast<uint8_t *>(sfx_buffer), frames * kChannels * sizeof(int16_t));
    if (spec.post_delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(spec.post_delay_ms));
    }
    if (spec.discard_record_ms > 0) {
        size_t discard_bytes = (static_cast<size_t>(spec.discard_record_ms) * kSampleRate * kChannels * sizeof(int16_t)) / 1000;
        while (discard_bytes > 0) {
            size_t bytes = std::min(discard_bytes, kDrainBytes);
            esp_err_t drain_ret = Codec_RecordData(drain_buffer, bytes);
            if (drain_ret != ESP_OK) {
                return drain_ret;
            }
            discard_bytes -= bytes;
        }
    }
    return ret;
#else
    (void)event;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
