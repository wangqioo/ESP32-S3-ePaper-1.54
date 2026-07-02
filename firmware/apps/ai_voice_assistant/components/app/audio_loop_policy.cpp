#include "audio_loop_policy.h"

#include <climits>

uint32_t AppEventWaitTimeoutMs(bool recording, bool playing) {
    return playing ? 0 : 50;
}

bool ShouldRefreshAudioProgress(uint32_t elapsed_seconds, uint32_t last_elapsed_seconds) {
    if (last_elapsed_seconds == UINT_MAX) {
        return true;
    }
    return elapsed_seconds != last_elapsed_seconds;
}

uint8_t SavingAnimationFrame(uint32_t elapsed_seconds) {
    return static_cast<uint8_t>(elapsed_seconds % 4);
}
