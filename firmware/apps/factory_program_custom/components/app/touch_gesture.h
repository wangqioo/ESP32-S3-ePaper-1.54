#ifndef TOUCH_GESTURE_H
#define TOUCH_GESTURE_H

#include "memo_types.h"

#include <cstdint>

constexpr int16_t kTouchTapSlopPx = 12;
constexpr int16_t kTouchPageSwipePx = 24;

VoiceUiAction ClassifyTouchGesture(uint16_t start_x,
                                   uint16_t start_y,
                                   uint16_t end_x,
                                   uint16_t end_y,
                                   uint16_t max_delta_x,
                                   uint16_t max_delta_y,
                                   VoiceAppState state);

#endif
