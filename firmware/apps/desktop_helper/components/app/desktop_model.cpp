#include "desktop_model.h"

#include <stdio.h>
#include <time.h>
#include <cmath>

const char *WeatherSymbolForCode(int code) {
    if (code == 0) {
        return "SUN";
    }
    if (code == 1 || code == 2 || code == 3) {
        return "CLOUD";
    }
    if ((code >= 45 && code <= 48)) {
        return "FOG";
    }
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        return "RAIN";
    }
    if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
        return "SNOW";
    }
    if (code >= 95 && code <= 99) {
        return "STORM";
    }
    return "--";
}

const char *WeatherPhraseForCode(int code) {
    if (code == 0) {
        return "Clear sky";
    }
    if (code == 1 || code == 2 || code == 3) {
        return "Soft clouds";
    }
    if (code >= 45 && code <= 48) {
        return "Misty outside";
    }
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        return "Keep an umbrella";
    }
    if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
        return "Snow watch";
    }
    if (code >= 95 && code <= 99) {
        return "Storm nearby";
    }
    return "Weather offline";
}

std::string FormatClock(const DesktopTime &time) {
    if (!time.valid) {
        return "--:--";
    }
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%02u:%02u", time.hour, time.minute);
    return buffer;
}

std::string FormatDate(const DesktopTime &time) {
    if (!time.valid) {
        return "--/--";
    }
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%02u/%02u", time.month, time.day);
    return buffer;
}

bool ConvertUnixToLocalDesktopTime(int64_t unix_seconds, int offset_seconds, DesktopTime *time) {
    if (time == nullptr) {
        return false;
    }
    time_t adjusted = static_cast<time_t>(unix_seconds + offset_seconds);
    struct tm local = {};
    if (gmtime_r(&adjusted, &local) == nullptr) {
        *time = {};
        return false;
    }
    time->year = static_cast<uint16_t>(local.tm_year + 1900);
    time->month = static_cast<uint8_t>(local.tm_mon + 1);
    time->day = static_cast<uint8_t>(local.tm_mday);
    time->hour = static_cast<uint8_t>(local.tm_hour);
    time->minute = static_cast<uint8_t>(local.tm_min);
    time->second = static_cast<uint8_t>(local.tm_sec);
    time->weekday = static_cast<uint8_t>(local.tm_wday);
    time->valid = true;
    return true;
}

std::string FormatIndoorFooter(const IndoorSnapshot &indoor) {
    if (!indoor.valid) {
        return "IN --";
    }
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "IN %.1fC %.0f%%", indoor.temperature_c, indoor.humidity_percent);
    return buffer;
}

std::string FormatBatteryFooter(const BatterySnapshot &battery) {
    if (!battery.valid) {
        return "BAT --";
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "BAT %u%%", battery.percent);
    return buffer;
}

std::string WeatherFreshnessLabel(const DesktopSnapshot &snapshot) {
    if (snapshot.weather.valid && snapshot.weather_stale) {
        return "OLD";
    }
    if (snapshot.weather.valid) {
        return "LIVE";
    }
    return "OFFLINE";
}

std::string BuildWeatherHint(const DesktopSnapshot &snapshot) {
    if (!snapshot.weather.valid) {
        if (snapshot.indoor.valid) {
            return "Indoor mode";
        }
        return "Waiting for data";
    }
    return WeatherPhraseForCode(snapshot.weather.weather_code);
}

bool PanelNeedsClockRefresh(DesktopPanel panel) {
    return panel == DesktopPanel::Clock;
}
