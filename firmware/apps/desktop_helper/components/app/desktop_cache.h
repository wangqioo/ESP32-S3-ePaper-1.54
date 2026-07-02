#ifndef DESKTOP_CACHE_H
#define DESKTOP_CACHE_H

#include "desktop_model.h"
#include <string>

std::string SerializeWeatherCache(const WeatherSnapshot &weather);
bool ParseWeatherCache(const std::string &data, WeatherSnapshot *weather);
bool LoadWeatherCache(WeatherSnapshot *weather);
bool SaveWeatherCache(const WeatherSnapshot &weather);

#endif

