#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include "desktop_model.h"

bool WeatherCredentialsConfigured();
bool SyncNetworkClock(int64_t *unix_seconds);
bool FetchWeather(WeatherSnapshot *weather);

#endif
