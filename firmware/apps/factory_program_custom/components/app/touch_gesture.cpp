#include "touch_gesture.h"

#include <cstdlib>

namespace {

VoiceUiAction HitTestTap(uint16_t x, uint16_t y, VoiceAppState state) {
    if (state == VoiceAppState::Settings) {
        if (y >= 32 && y < 187) {
            uint8_t row = static_cast<uint8_t>((y - 32) / 31);
            return {VoiceUiActionType::SelectRow, row};
        }
    }

    if (state == VoiceAppState::List) {
        if (y >= 34 && y < 170) {
            uint8_t row = static_cast<uint8_t>((y - 34) / 34);
            return {VoiceUiActionType::SelectRow, row};
        }
    }

    if (state == VoiceAppState::Detail || state == VoiceAppState::Playing) {
        if (y >= 116 && y <= 138) {
            return {VoiceUiActionType::Star, 0};
        }
        if (y >= 148 && y <= 188) {
            if (x < 68) {
                return {VoiceUiActionType::PlayStop, 0};
            }
            if (x < 132) {
                return {VoiceUiActionType::Delete, 0};
            }
            return {VoiceUiActionType::Back, 0};
        }
    }

    if (state == VoiceAppState::DeleteConfirm) {
        if (y >= 140 && y <= 184) {
            if (x < 100) {
                return {VoiceUiActionType::ConfirmDelete, 0};
            }
            return {VoiceUiActionType::Cancel, 0};
        }
    }

    if (state == VoiceAppState::ClearAllConfirm) {
        if (y >= 140 && y <= 184) {
            if (x < 100) {
                return {VoiceUiActionType::ConfirmClearAll, 0};
            }
            return {VoiceUiActionType::Cancel, 0};
        }
    }

    return {};
}

} // namespace

VoiceUiAction ClassifyTouchGesture(uint16_t start_x,
                                   uint16_t start_y,
                                   uint16_t end_x,
                                   uint16_t end_y,
                                   uint16_t max_delta_x,
                                   uint16_t max_delta_y,
                                   VoiceAppState state) {
    int16_t delta_y = static_cast<int16_t>(end_y) - static_cast<int16_t>(start_y);

    if (state == VoiceAppState::List) {
        if (delta_y <= -kTouchPageSwipePx) {
            return {VoiceUiActionType::NextPage, 0};
        }
        if (delta_y >= kTouchPageSwipePx) {
            return {VoiceUiActionType::PreviousPage, 0};
        }
    }

    if (max_delta_x > kTouchTapSlopPx || max_delta_y > kTouchTapSlopPx) {
        return {};
    }

    return HitTestTap(start_x, start_y, state);
}
