#ifndef VOICE_UI_H
#define VOICE_UI_H

#include "memo_types.h"

#include "lvgl.h"

#include <array>
#include <string>
#include <vector>

class VoiceUi {
public:
    static constexpr uint8_t kRowsPerPage = 4;

    void Init();
    void ShowList(const std::vector<MemoMetadata> &memos, uint32_t page, uint8_t battery_percent, bool sd_ready, const char *message);
    void ShowRecording(uint32_t elapsed_seconds);
    void ShowDetail(const MemoMetadata &memo, bool playing, uint32_t elapsed_seconds);
    void ShowError(const char *message);
    void ShowShutdown(uint32_t memo_count, uint8_t battery_percent, const MemoMetadata *last_memo);
    VoiceUiAction HitTestTap(uint16_t x, uint16_t y, VoiceAppState state) const;
    VoiceUiAction HitTestSwipe(uint16_t start_y, uint16_t end_y, VoiceAppState state) const;

private:
    void Clear();
    lv_obj_t *CreateLabel(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const lv_font_t *font, lv_text_align_t align);
    lv_obj_t *CreateBox(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
    void SetLabel(lv_obj_t *label, const std::string &text);

    lv_obj_t *screen_ = nullptr;
};

#endif
