#include "assistant_ui.h"

#include "assistant_model.h"

#include <lvgl.h>

#include <cstdio>

namespace {

lv_obj_t *screen = nullptr;

lv_obj_t *Label(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const lv_font_t *font, lv_text_align_t align) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    return label;
}

void ButtonHint(lv_obj_t *parent, const char *text) {
    Label(parent, 8, 176, 184, 18, text, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

}  // namespace

void AssistantUi::Init() {
    screen = lv_obj_create(nullptr);
    lv_obj_set_size(screen, 200, 200);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_scr_load(screen);
}

void AssistantUi::Clear() {
    if (screen == nullptr) {
        Init();
    }
    lv_obj_clean(screen);
}

void AssistantUi::ShowHome(bool wifi_configured, bool proxy_configured) {
    Clear();
    Label(screen, 8, 12, 184, 24, "AI ASSISTANT", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 54, 184, 46, "Hold BOOT\nto ask", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    const char *status = !wifi_configured ? "Wi-Fi not set" : (!proxy_configured ? "Proxy not set" : "Ready");
    Label(screen, 8, 122, 184, 28, status, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    ButtonHint(screen, "PWR hold: off");
}

void AssistantUi::ShowRecording(uint32_t elapsed_seconds) {
    Clear();
    Label(screen, 8, 22, 184, 24, "RECORDING", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", static_cast<unsigned long>(elapsed_seconds / 60), static_cast<unsigned long>(elapsed_seconds % 60));
    Label(screen, 8, 82, 184, 34, buffer, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    ButtonHint(screen, "Release BOOT");
}

void AssistantUi::ShowBusy(AssistantState state) {
    Clear();
    Label(screen, 8, 54, 184, 28, AssistantStatusLabel(state).c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 102, 184, 24, "Please wait", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}

void AssistantUi::ShowAnswer(const std::string &answer, const std::string &request_id) {
    Clear();
    Label(screen, 8, 8, 184, 22, "ANSWER", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    auto lines = WrapAssistantText(answer, 18, 5);
    int y = 40;
    for (const auto &line : lines) {
        Label(screen, 10, y, 180, 20, line.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);
        y += 24;
    }
    std::string footer = request_id.empty() ? "PWR back" : ("PWR back #" + request_id.substr(0, 6));
    ButtonHint(screen, footer.c_str());
}

void AssistantUi::ShowError(AssistantError error) {
    Clear();
    Label(screen, 8, 44, 184, 24, "ERROR", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 88, 184, 42, AssistantErrorLabel(error).c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    ButtonHint(screen, "PWR back");
}

void AssistantUi::ShowPowerOff() {
    Clear();
    Label(screen, 8, 78, 184, 28, "POWER OFF", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
}
