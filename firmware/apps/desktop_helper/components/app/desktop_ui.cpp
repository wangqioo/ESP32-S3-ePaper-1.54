#include "desktop_ui.h"

#include "desktop_config.h"
#include "lvgl.h"

#include <stdio.h>
#include <string>

namespace {

lv_obj_t *screen = nullptr;

lv_obj_t *Label(lv_obj_t *parent, int x, int y, int w, int h, const char *text, const lv_font_t *font, lv_text_align_t align) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_align(label, align, 0);
    return label;
}

void Clear() {
    if (screen == nullptr) {
        screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
        lv_obj_set_style_border_width(screen, 0, 0);
        lv_obj_set_style_pad_all(screen, 0, 0);
        lv_scr_load(screen);
    }
    lv_obj_clean(screen);
}

std::string WeatherTempText(const DesktopSnapshot &snapshot) {
    if (!snapshot.weather.valid) {
        return "--C";
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.0fC", snapshot.weather.temperature_c);
    return buffer;
}

std::string DateStatusText(const DesktopSnapshot &snapshot) {
    return FormatDate(snapshot.time) + "  " + WeatherFreshnessLabel(snapshot);
}

}  // namespace

void DesktopUi::Init() {
    Clear();
}

void DesktopUi::ShowLoading(const char *message) {
    Clear();
    Label(screen, 8, 12, 184, 24, desktop_config::kLocationName, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 66, 184, 34, "DESK", &lv_font_montserrat_30, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 116, 184, 24, message != nullptr ? message : "Loading", &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 176, 184, 16, "ESP32-S3 ePaper", &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);
}

void DesktopUi::ShowWeather(const DesktopSnapshot &snapshot) {
    Clear();
    std::string date_status = DateStatusText(snapshot);
    std::string temp = WeatherTempText(snapshot);
    std::string hint = BuildWeatherHint(snapshot);
    std::string indoor = FormatIndoorFooter(snapshot.indoor);
    std::string battery = FormatBatteryFooter(snapshot.battery);
    std::string footer = indoor + "  " + battery;

    Label(screen, 8, 4, 184, 18, date_status.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 14, 36, 172, 26, WeatherSymbolForCode(snapshot.weather.weather_code), &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    Label(screen, 0, 72, 200, 48, temp.c_str(), &lv_font_montserrat_40, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 130, 184, 28, hint.c_str(), &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    Label(screen, 4, 178, 192, 15, footer.c_str(), &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);
}

void DesktopUi::ShowClock(const DesktopSnapshot &snapshot) {
    Clear();
    std::string clock = FormatClock(snapshot.time);
    std::string date = FormatDate(snapshot.time);
    std::string weather = WeatherTempText(snapshot) + "  " + WeatherSymbolForCode(snapshot.weather.weather_code);
    std::string indoor = FormatIndoorFooter(snapshot.indoor);
    std::string battery = FormatBatteryFooter(snapshot.battery);
    std::string footer = indoor + "  " + battery;

    Label(screen, 8, 8, 184, 18, desktop_config::kLocationName, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 0, 45, 200, 44, clock.c_str(), &lv_font_montserrat_40, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 98, 184, 20, date.c_str(), &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    Label(screen, 8, 130, 184, 18, weather.c_str(), &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    Label(screen, 4, 178, 192, 15, footer.c_str(), &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);
}
