#include "desktop_model.h"
#include "desktop_cache.h"

#include <cassert>
#include <string>

int main() {
    assert(std::string(WeatherSymbolForCode(0)) == "SUN");
    assert(std::string(WeatherSymbolForCode(2)) == "CLOUD");
    assert(std::string(WeatherSymbolForCode(61)) == "RAIN");
    assert(std::string(WeatherPhraseForCode(61)) == "Keep an umbrella");

    DesktopTime time;
    assert(FormatClock(time) == "--:--");
    time.valid = true;
    time.hour = 7;
    time.minute = 5;
    time.second = 59;
    time.month = 7;
    time.day = 2;
    assert(FormatClock(time) == "07:05");
    assert(FormatDate(time) == "07/02");
    assert(!PanelNeedsClockRefresh(DesktopPanel::Weather));
    assert(PanelNeedsClockRefresh(DesktopPanel::Clock));

    DesktopTime converted;
    assert(ConvertUnixToLocalDesktopTime(0, 8 * 60 * 60, &converted));
    assert(converted.valid);
    assert(converted.year == 1970);
    assert(converted.month == 1);
    assert(converted.day == 1);
    assert(converted.hour == 8);
    assert(converted.minute == 0);
    assert(converted.second == 0);
    assert(converted.weekday == 4);

    IndoorSnapshot indoor;
    assert(FormatIndoorFooter(indoor) == "IN --");
    indoor.valid = true;
    indoor.temperature_c = 26.1f;
    indoor.humidity_percent = 58.0f;
    assert(FormatIndoorFooter(indoor) == "IN 26.1C 58%");

    DesktopSnapshot snapshot;
    assert(WeatherFreshnessLabel(snapshot) == "OFFLINE");
    snapshot.indoor = indoor;
    assert(BuildWeatherHint(snapshot) == "Indoor mode");
    snapshot.weather.valid = true;
    snapshot.weather.weather_code = 0;
    assert(WeatherFreshnessLabel(snapshot) == "LIVE");
    snapshot.weather_stale = true;
    assert(WeatherFreshnessLabel(snapshot) == "OLD");

    WeatherSnapshot weather;
    weather.valid = true;
    weather.temperature_c = 23.4f;
    weather.weather_code = 3;
    weather.fetched_unix = 123456;
    std::string serialized = SerializeWeatherCache(weather);
    WeatherSnapshot parsed;
    assert(ParseWeatherCache(serialized, &parsed));
    assert(parsed.valid);
    assert(parsed.weather_code == 3);
    assert(parsed.fetched_unix == 123456);
    assert(parsed.temperature_c > 23.3f && parsed.temperature_c < 23.5f);

    return 0;
}
