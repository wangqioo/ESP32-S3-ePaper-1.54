#ifndef AUDIO_LOOP_POLICY_H
#define AUDIO_LOOP_POLICY_H

#include <cstdint>

uint32_t AppEventWaitTimeoutMs(bool recording, bool playing);
bool ShouldRefreshAudioProgress(uint32_t elapsed_seconds, uint32_t last_elapsed_seconds);
uint8_t SavingAnimationFrame(uint32_t elapsed_seconds);

#endif
