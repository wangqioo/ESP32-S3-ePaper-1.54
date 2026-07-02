# Desktop Helper Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an independent e-paper desktop helper application with Wi-Fi weather, indoor SHTC3 readings, battery status, and 15-minute refresh.

**Architecture:** Add a new app at `firmware/apps/desktop_helper` so the stable voice memo app stays untouched. Reuse the existing `port_bsp` style and keep product logic in small modules: model, cache, weather, UI, sleep, and app orchestration.

**Tech Stack:** ESP-IDF 5.5, ESP32-S3, LVGL v9, ESP HTTP client, cJSON, NVS, FreeRTOS, existing e-paper/I2C/SHTC3/ADC/power BSP.

---

## Files

- Create: `firmware/apps/desktop_helper/CMakeLists.txt`
- Create: `firmware/apps/desktop_helper/main/CMakeLists.txt`
- Create: `firmware/apps/desktop_helper/main/main.cpp`
- Create: `firmware/apps/desktop_helper/sdkconfig.defaults`
- Create: `firmware/apps/desktop_helper/partitions.csv`
- Create: `firmware/apps/desktop_helper/components/app/CMakeLists.txt`
- Create: `firmware/apps/desktop_helper/components/app/desktop_config.h`
- Create: `firmware/apps/desktop_helper/components/app/desktop_model.h`
- Create: `firmware/apps/desktop_helper/components/app/desktop_model.cpp`
- Create: `firmware/apps/desktop_helper/components/app/desktop_cache.h`
- Create: `firmware/apps/desktop_helper/components/app/desktop_cache.cpp`
- Create: `firmware/apps/desktop_helper/components/app/weather_client.h`
- Create: `firmware/apps/desktop_helper/components/app/weather_client.cpp`
- Create: `firmware/apps/desktop_helper/components/app/desktop_ui.h`
- Create: `firmware/apps/desktop_helper/components/app/desktop_ui.cpp`
- Create: `firmware/apps/desktop_helper/components/app/desktop_sleep.h`
- Create: `firmware/apps/desktop_helper/components/app/desktop_sleep.cpp`
- Create: `firmware/apps/desktop_helper/components/app/user_app.h`
- Create: `firmware/apps/desktop_helper/components/app/user_app.cpp`
- Create: `firmware/apps/desktop_helper/components/app/test/test_desktop_model.cpp`
- Copy/adapt: `firmware/apps/desktop_helper/components/port_bsp` from the custom factory app.
- Copy/adapt: `firmware/apps/desktop_helper/components/externlib` from the custom factory app if required by the BSP.

## Tasks

### Task 1: App Skeleton

- [x] Create `desktop_helper` project structure.
- [x] Copy the proven main/LVGL startup path from `factory_program_custom`.
- [x] Build a blank page that proves display initialization still works.
- [x] Verify with `idf.py build`.

### Task 2: Pure Model and Tests

- [x] Write host tests for:
  - weather code mapping to symbol and phrase,
  - stale/fresh/offline labels,
  - battery footer formatting,
  - cache serialization round trip.
- [x] Run tests and confirm they fail before implementation.
- [x] Implement `desktop_model` and `desktop_cache` pure helpers.
- [x] Run tests and confirm they pass.

### Task 3: UI

- [x] Implement `desktop_ui` with:
  - loading page,
  - desktop ornament home page,
  - status page,
  - offline fallback page.
- [x] Keep text inside the 200x200 layout.
- [x] Avoid dense dashboard styling.

### Task 4: Hardware Read Path

- [x] Initialize power, I2C, ADC, SHTC3, RTC, and buttons.
- [x] Read indoor temperature/humidity.
- [x] Read battery percentage and voltage when available.
- [x] Read RTC time when available.
- [x] Render hardware values without Wi-Fi.

### Task 5: Wi-Fi and Weather

- [x] Add compile-time Wi-Fi/location config.
- [x] Connect STA with timeout.
- [x] Fetch weather from Open-Meteo over HTTP.
- [x] Parse current temperature/weather code.
- [x] Cache successful weather snapshot in NVS.
- [x] Fallback to cached or offline display on failure.

### Task 6: Buttons and Sleep

- [x] BOOT short press triggers immediate refresh while awake.
- [x] PWR short press toggles status page.
- [x] PWR long press enters deep sleep.
- [x] Automatic refresh interval is 15 minutes.
- [x] Keep automatic mode awake so USB power/development use does not get stuck in deep sleep.

### Task 7: Device Verification

- [x] Build from `firmware/apps/desktop_helper`.
- [x] Flash only `/dev/cu.usbmodem11101`.
- [x] Monitor logs.
- [x] Confirm RTC, SHTC3, display, offline weather fallback, and timer sleep behavior.
- [x] Test conservative PWR ext0 wake build and document that PWR does not reliably reconnect USB from automatic timed sleep.
- [x] Flash final awake periodic-refresh build.
- [x] Verify startup and no automatic deep sleep from monitor logs.
- [x] Fix build/runtime issues until boot, render, fallback, and awake periodic-refresh behavior are stable.
