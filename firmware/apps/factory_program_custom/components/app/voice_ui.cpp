#include "voice_ui.h"

#include "memo_utils.h"

#include <algorithm>
#include <cstdio>

void VoiceUi::Init() {
    if (screen_ == nullptr) {
        screen_ = lv_obj_create(nullptr);
        lv_obj_set_size(screen_, 200, 200);
        lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(screen_, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_pad_all(screen_, 0, LV_PART_MAIN);
    }
    lv_screen_load(screen_);
}

void VoiceUi::ShowList(const std::vector<MemoMetadata> &memos, uint32_t page, uint8_t battery_percent, bool storage_ready, const char *storage_label, const char *message) {
    Clear();

    char header[32];
    std::snprintf(header, sizeof(header), "MEMOS %u/%u", static_cast<unsigned>(page + 1),
                  static_cast<unsigned>(CalculatePageCount(memos.size(), kRowsPerPage)));
    CreateLabel(screen_, 6, 4, 104, 20, header, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);

    char status[24];
    if (storage_ready) {
        std::snprintf(status, sizeof(status), "%s %u%%", storage_label == nullptr ? "STOR" : storage_label, battery_percent);
    } else {
        std::snprintf(status, sizeof(status), "NO STORE");
    }
    CreateLabel(screen_, 112, 4, 82, 20, status, &lv_font_montserrat_14, LV_TEXT_ALIGN_RIGHT);

    lv_obj_t *line = CreateBox(screen_, 5, 27, 190, 2);
    lv_obj_set_style_bg_color(line, lv_color_black(), LV_PART_MAIN);

    if (!storage_ready) {
        CreateLabel(screen_, 10, 78, 180, 28, "NO STORAGE", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
        CreateLabel(screen_, 10, 116, 180, 20, "Storage init failed", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
        return;
    }

    if (memos.empty()) {
        CreateLabel(screen_, 10, 72, 180, 28, "NO MEMOS", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
        CreateLabel(screen_, 10, 110, 180, 20, "Hold BOOT to record", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
        return;
    }

    uint32_t start = page * kRowsPerPage;
    for (uint8_t row = 0; row < kRowsPerPage; ++row) {
        uint32_t index = start + row;
        int32_t y = 34 + row * 34;
        lv_obj_t *row_box = CreateBox(screen_, 5, y, 190, 30);
        lv_obj_set_style_border_width(row_box, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(row_box, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_color(row_box, lv_color_white(), LV_PART_MAIN);

        if (index >= memos.size()) {
            continue;
        }

        const MemoMetadata &memo = memos[index];
        char row_text[48];
        std::snprintf(row_text, sizeof(row_text), "%c%03u %s %s",
                      memo.starred ? '*' : ' ',
                      static_cast<unsigned>(memo.sequence),
                      memo.display_time.empty() ? "--:--" : memo.display_time.c_str(),
                      FormatDuration(memo.duration_seconds).c_str());
        CreateLabel(row_box, 4, 4, 182, 20, row_text, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    }

    if (message != nullptr && message[0] != '\0') {
        CreateLabel(screen_, 8, 174, 184, 18, message, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    }
}

void VoiceUi::ShowRecording(uint32_t elapsed_seconds) {
    Clear();
    CreateLabel(screen_, 8, 16, 184, 24, "RECORDING", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 58, 184, 26, "*", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 90, 184, 34, FormatDuration(elapsed_seconds).c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 154, 184, 20, "Release BOOT to save", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowSaving(uint8_t frame) {
    Clear();
    const char *spinner[] = {"|", "/", "-", "\\"};
    CreateLabel(screen_, 8, 24, 184, 24, "SAVING", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 74, 184, 34, spinner[frame % 4], &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 132, 184, 24, "Writing memo", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowDetail(const MemoMetadata &memo, bool playing, uint32_t elapsed_seconds) {
    Clear();
    char title[24];
    std::snprintf(title, sizeof(title), "MEMO %03u", static_cast<unsigned>(memo.sequence));
    CreateLabel(screen_, 8, 10, 184, 24, title, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 42, 184, 20, memo.display_time.empty() ? "--:--" : memo.display_time.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    std::string duration = playing ? (FormatDuration(elapsed_seconds) + " / " + FormatDuration(memo.duration_seconds))
                                   : ("Duration " + FormatDuration(memo.duration_seconds));
    CreateLabel(screen_, 8, 70, 184, 20, duration.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 95, 184, 18, playing ? "Playing..." : "Ready", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *star = CreateBox(screen_, 46, 117, 108, 24);
    lv_obj_set_style_border_width(star, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(star, lv_color_black(), LV_PART_MAIN);
    CreateLabel(star, 0, 4, 108, 16, memo.starred ? "UNSTAR" : "STAR", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *left = CreateBox(screen_, 8, 151, 54, 31);
    lv_obj_set_style_border_width(left, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(left, lv_color_black(), LV_PART_MAIN);
    CreateLabel(left, 0, 7, 54, 16, playing ? "STOP" : "PLAY", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *middle = CreateBox(screen_, 73, 151, 54, 31);
    lv_obj_set_style_border_width(middle, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(middle, lv_color_black(), LV_PART_MAIN);
    CreateLabel(middle, 0, 7, 54, 16, "DEL", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *right = CreateBox(screen_, 138, 151, 54, 31);
    lv_obj_set_style_border_width(right, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(right, lv_color_black(), LV_PART_MAIN);
    CreateLabel(right, 0, 7, 54, 16, "BACK", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowDeleteConfirm(const MemoMetadata &memo) {
    Clear();
    char title[24];
    std::snprintf(title, sizeof(title), "DELETE %03u?", static_cast<unsigned>(memo.sequence));
    CreateLabel(screen_, 8, 18, 184, 24, title, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 54, 184, 20, memo.display_time.empty() ? "--:--" : memo.display_time.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    std::string duration = "Duration " + FormatDuration(memo.duration_seconds);
    CreateLabel(screen_, 8, 82, 184, 20, duration.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 112, 184, 18, "This cannot be undone", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *left = CreateBox(screen_, 14, 145, 78, 34);
    lv_obj_set_style_border_width(left, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(left, lv_color_black(), LV_PART_MAIN);
    CreateLabel(left, 0, 8, 78, 18, "DELETE", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *right = CreateBox(screen_, 108, 145, 78, 34);
    lv_obj_set_style_border_width(right, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(right, lv_color_black(), LV_PART_MAIN);
    CreateLabel(right, 0, 8, 78, 18, "CANCEL", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowSettings(const VoiceSettings &settings, uint32_t memo_count) {
    Clear();
    CreateLabel(screen_, 6, 4, 104, 20, "SETTINGS", &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    CreateLabel(screen_, 112, 4, 82, 20, "PWR BACK", &lv_font_montserrat_14, LV_TEXT_ALIGN_RIGHT);

    lv_obj_t *line = CreateBox(screen_, 5, 27, 190, 2);
    lv_obj_set_style_bg_color(line, lv_color_black(), LV_PART_MAIN);

    constexpr uint8_t kSettingsRows = 5;
    const char *labels[kSettingsRows] = {
        "SOUND",
        "VOLUME",
        "REC BEEP",
        "REC MODE",
        "CLEAR UNSTAR",
    };
    char values[kSettingsRows][20] = {};
    std::snprintf(values[0], sizeof(values[0]), "%s", settings.sfx_enabled ? "ON" : "OFF");
    std::snprintf(values[1], sizeof(values[1]), "%s", VoiceSettingsVolumeLabel(settings.sfx_volume));
    std::snprintf(values[2], sizeof(values[2]), "%s", settings.record_start_sfx_enabled ? "ON" : "OFF");
    std::snprintf(values[3], sizeof(values[3]), "%s", VoiceSettingsRecordModeLabel(settings.record_mode));
    std::snprintf(values[4], sizeof(values[4]), "%u MEMOS", static_cast<unsigned>(memo_count));

    for (uint8_t row = 0; row < kSettingsRows; ++row) {
        int32_t y = 32 + row * 31;
        lv_obj_t *row_box = CreateBox(screen_, 5, y, 190, 27);
        lv_obj_set_style_border_width(row_box, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(row_box, lv_color_black(), LV_PART_MAIN);
        CreateLabel(row_box, 5, 4, 98, 18, labels[row], &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
        CreateLabel(row_box, 105, 4, 78, 18, values[row], &lv_font_montserrat_14, LV_TEXT_ALIGN_RIGHT);
    }
}

void VoiceUi::ShowClearAllConfirm(uint32_t memo_count) {
    Clear();
    CreateLabel(screen_, 8, 18, 184, 24, "DELETE UNSTAR?", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    char count_text[32];
    std::snprintf(count_text, sizeof(count_text), "%u memos", static_cast<unsigned>(memo_count));
    CreateLabel(screen_, 8, 58, 184, 20, count_text, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 88, 184, 34, "Starred memos stay", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *left = CreateBox(screen_, 14, 145, 78, 34);
    lv_obj_set_style_border_width(left, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(left, lv_color_black(), LV_PART_MAIN);
    CreateLabel(left, 0, 8, 78, 18, "DELETE", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *right = CreateBox(screen_, 108, 145, 78, 34);
    lv_obj_set_style_border_width(right, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(right, lv_color_black(), LV_PART_MAIN);
    CreateLabel(right, 0, 8, 78, 18, "CANCEL", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowError(const char *message) {
    Clear();
    CreateLabel(screen_, 8, 64, 184, 26, "ERROR", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    CreateLabel(screen_, 8, 102, 184, 40, message == nullptr ? "Unknown" : message, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::ShowShutdown(uint32_t memo_count, uint8_t battery_percent, const MemoMetadata *last_memo) {
    Clear();
    CreateLabel(screen_, 8, 7, 116, 20, "VOICE MEMO", &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *off = CreateBox(screen_, 147, 6, 42, 20);
    lv_obj_set_style_border_width(off, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(off, 10, LV_PART_MAIN);
    CreateLabel(off, 0, 3, 42, 14, "OFF", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *line = CreateBox(screen_, 8, 31, 184, 2);
    lv_obj_set_style_bg_color(line, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *memos = CreateBox(screen_, 9, 41, 86, 48);
    lv_obj_set_style_border_width(memos, 2, LV_PART_MAIN);
    CreateLabel(memos, 6, 5, 74, 12, "MEMOS", &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    char memo_count_text[12];
    std::snprintf(memo_count_text, sizeof(memo_count_text), "%u", static_cast<unsigned>(memo_count));
    CreateLabel(memos, 6, 20, 74, 22, memo_count_text, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *battery = CreateBox(screen_, 105, 41, 86, 48);
    lv_obj_set_style_border_width(battery, 2, LV_PART_MAIN);
    CreateLabel(battery, 6, 5, 74, 12, "BAT", &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    char bat_text[12];
    std::snprintf(bat_text, sizeof(bat_text), "%u%%", battery_percent);
    CreateLabel(battery, 6, 22, 74, 18, bat_text, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *last = CreateBox(screen_, 9, 98, 182, 46);
    lv_obj_set_style_border_width(last, 2, LV_PART_MAIN);
    CreateLabel(last, 6, 5, 170, 12, "LAST SAVED", &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    const char *time = (last_memo == nullptr || last_memo->display_time.empty()) ? "--:--" : last_memo->display_time.c_str();
    CreateLabel(last, 6, 21, 84, 18, time, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
    std::string duration = last_memo == nullptr ? "--:--" : FormatDuration(last_memo->duration_seconds);
    CreateLabel(last, 104, 24, 70, 14, duration.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_RIGHT);

    CreateLabel(screen_, 8, 160, 184, 28, "READY FOR\nNEXT MEMO", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void VoiceUi::Clear() {
    Init();
    lv_obj_clean(screen_);
}

lv_obj_t *VoiceUi::CreateLabel(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const lv_font_t *font, lv_text_align_t align) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    return label;
}

lv_obj_t *VoiceUi::CreateBox(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);
    return box;
}

void VoiceUi::SetLabel(lv_obj_t *label, const std::string &text) {
    if (label != nullptr) {
        lv_label_set_text(label, text.c_str());
    }
}
