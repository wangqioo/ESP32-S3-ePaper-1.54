#ifndef DESKTOP_MODEL_H
#define DESKTOP_MODEL_H

#include <stdint.h>
#include <string>

struct DesktopTime {
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t weekday = 0;
    bool valid = false;
};

struct IndoorSnapshot {
    float temperature_c = 0.0f;
    float humidity_percent = 0.0f;
    bool valid = false;
};

struct BatterySnapshot {
    float voltage = 0.0f;
    uint8_t percent = 0;
    bool valid = false;
};

struct WeatherSnapshot {
    float temperature_c = 0.0f;
    int weather_code = -1;
    int64_t fetched_unix = 0;
    bool valid = false;
};

struct DesktopSnapshot {
    DesktopTime time;
    IndoorSnapshot indoor;
    BatterySnapshot battery;
    WeatherSnapshot weather;
    bool weather_stale = false;
    bool wifi_attempted = false;
    bool wifi_connected = false;
};

enum class DesktopPanel {
    Weather,
    Clock,
};

const char *WeatherSymbolForCode(int code);
const char *WeatherPhraseForCode(int code);
std::string FormatClock(const DesktopTime &time);
std::string FormatDate(const DesktopTime &time);
bool ConvertUnixToLocalDesktopTime(int64_t unix_seconds, int offset_seconds, DesktopTime *time);
std::string FormatIndoorFooter(const IndoorSnapshot &indoor);
std::string FormatBatteryFooter(const BatterySnapshot &battery);
std::string WeatherFreshnessLabel(const DesktopSnapshot &snapshot);
std::string BuildWeatherHint(const DesktopSnapshot &snapshot);
bool PanelNeedsClockRefresh(DesktopPanel panel);

#endif
