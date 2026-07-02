#ifndef DESKTOP_UI_H
#define DESKTOP_UI_H

#include "desktop_model.h"

class DesktopUi {
public:
    void Init();
    void ShowLoading(const char *message);
    void ShowWeather(const DesktopSnapshot &snapshot);
    void ShowClock(const DesktopSnapshot &snapshot);
};

#endif
