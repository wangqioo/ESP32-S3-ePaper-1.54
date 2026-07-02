# E-Paper Desktop Helper Design

## Goal

Build a second firmware application for the ESP32-S3-ePaper-1.54 board: a low-power e-paper desktop companion with personality. It shows weather, time, indoor temperature/humidity, and battery status on a 200x200 screen, refreshes every 15 minutes, and falls back gracefully when Wi-Fi or weather is unavailable.

## Product Shape

The first screen is a quiet desktop ornament, not a dense dashboard. It uses one large weather symbol, one large outdoor temperature value, and one short human-readable hint. Small secondary data at the bottom shows indoor temperature, humidity, battery, and freshness.

The selected visual direction is "desktop ornament":

- Large weather mark in the center.
- Large temperature below or beside it.
- Short status sentence such as `Good morning`, `Fresh air`, `Keep an umbrella`.
- Compact footer: indoor temperature, relative humidity, battery percentage.
- `OLD` marker when showing cached weather.
- `OFFLINE` marker when no weather has ever been fetched.

## Scope

Version 1 includes:

- Wi-Fi STA connection using compile-time credentials.
- Weather fetch using a simple HTTP weather provider module.
- Cached weather stored in NVS so the device can show the last known weather after failed refreshes.
- SHTC3 indoor temperature/humidity.
- ADC battery percentage.
- RTC time read when available, plus network time sync back to the PCF85063 RTC after Wi-Fi connects.
- 15-minute refresh cycle while staying awake.
- BOOT short press to refresh immediately.
- PWR short press to show a compact status page.
- PWR long press to enter deep sleep / power-off path.
- E-paper page rendering through the existing LVGL/e-paper port.

Version 1 does not include:

- Phone hotspot provisioning.
- Touch navigation.
- Calendar sync.
- Multi-city support.
- HTTPS certificate pinning.
- OTA updates.

## Architecture

Create a new independent app under `firmware/apps/desktop_helper`. It reuses the existing custom factory app's proven BSP components by copying only the app-specific shell and referencing shared-style components under the new app. The voice memo app remains intact.

Core units:

- `desktop_model`: plain C++ structs and formatting helpers for weather, indoor sensor, battery, time, and display freshness.
- `weather_client`: Wi-Fi and HTTP fetch path. It returns a parsed weather snapshot or a typed error.
- `desktop_cache`: NVS persistence for the last successful weather snapshot.
- `desktop_ui`: LVGL screen drawing for the ornament home page, status page, loading page, and error/fallback page.
- `desktop_sleep`: 15-minute wake scheduling and deep sleep entry.
- `user_app`: boot sequence and app state orchestration.

The weather module must be replaceable. The first implementation can use Open-Meteo because it supports HTTP queries without an API key. If a different API is later needed, only `weather_client` should change.

## Data Flow

On boot:

1. Initialize NVS, power rails, I2C, display, ADC, SHTC3, RTC, buttons, and LVGL.
2. Show a loading page.
3. Read indoor temperature/humidity and battery.
4. Read cached weather from NVS.
5. Connect to Wi-Fi.
6. Sync network time by SNTP and write the local UTC+8 time to the PCF85063 RTC when available.
7. Fetch weather.
8. If fetch succeeds, update cache and render fresh weather.
9. If fetch fails, render cached weather with `OLD`.
10. If no cache exists, render offline page with indoor data and time.
11. Stay awake and refresh again after 15 minutes.

Manual refresh:

1. BOOT short press wakes or refreshes while awake.
2. The UI shows `Refreshing`.
3. Sensor values are read again.
4. Wi-Fi/SNTP clock sync runs when configured.
5. Wi-Fi/weather fetch runs.
6. UI updates with fresh or cached weather.

Status page:

1. PWR short press switches to a status page.
2. The status page shows Wi-Fi state, weather freshness, battery voltage/percentage, indoor values, and next refresh interval.
3. Pressing BOOT refreshes from the status page.

## Error Handling

- Wi-Fi missing credentials: skip connection, show cached or offline page.
- Wi-Fi connection timeout: disconnect Wi-Fi, show cached or offline page.
- HTTP failure: show cached or offline page.
- JSON parse failure: show cached or offline page.
- SHTC3 failure: show `--` for indoor values.
- RTC failure: show `--:--`.
- SNTP failure: keep the current RTC value and continue weather refresh.
- RTC write failure: log the failure and continue weather refresh.
- NVS cache failure: continue without cache.

The app must log all failures with `ESP_LOGW` or `ESP_LOGE`, but the screen should stay calm and useful.

## Power Strategy

The app stays awake by default and refreshes every 15 minutes. This is intentional: while the board is connected to USB power, automatic deep sleep makes the USB serial device disappear and PWR wake did not reliably reconnect it. Staying awake gives predictable desktop/development behavior.

- BOOT is used for refresh while the app is awake.
- PWR short press is used for status while awake.
- PWR long press uses the official/custom factory app's `esp_sleep_enable_ext0_wakeup(PWR_BUTTON_PIN, 0)` shutdown path.

Important board note: ESP32 deep-sleep wake configuration and the board-level power button behavior are related but not identical. In automatic refresh sleep, PWR wake was tested with ext1 and with the official-style ext0 path plus explicit RTC GPIO initialization; in both cases USB did not reconnect reliably from the automatic sleep path. For this desktop app, automatic deep sleep is disabled by default. A future battery-first mode can reintroduce timed deep sleep as an explicit option.

RTC alarm wake can be added after the timer wake path is stable. The official RTC sleep example remains the reference for a later RTC interrupt wake implementation.

The external PCF85063 RTC should continue keeping time while the ESP32 is asleep or the board-level power hold is released, as long as the RTC backup supply path is present. The desktop helper therefore uses Wi-Fi only to calibrate the RTC: each refresh attempts SNTP (`ntp.aliyun.com` by default), converts the result to UTC+8 for Shanghai, writes it to PCF85063, and then uses RTC reads for display.

## Configuration

Compile-time defaults live in `desktop_config.h`:

- `kWifiSsid`
- `kWifiPassword`
- `kLatitude`
- `kLongitude`
- `kLocationName` (defaults to `SHANGHAI`, with Shanghai coordinates)
- `kRefreshIntervalMinutes = 15`
- `kNtpServer = ntp.aliyun.com`
- `kTimezoneOffsetSeconds = 8 * 60 * 60`

Empty Wi-Fi credentials are valid and mean offline mode.

## Testing

Host tests cover:

- Weather code to display symbol/text mapping.
- Home page model fallback labels.
- Cache serialization/deserialization.
- Refresh decision helpers.

Device verification covers:

- Build succeeds.
- Flash succeeds to `/dev/cu.usbmodem11101`.
- Serial log confirms app boot, sensor read, weather path, UI render, and sleep scheduling.
- Serial log confirms SNTP sync and PCF85063 RTC write when Wi-Fi is available.
- Serial log confirms periodic refresh without automatic deep sleep.
- With empty Wi-Fi credentials, the device renders an offline/cached page instead of crashing.
- With valid Wi-Fi credentials, the device fetches weather and renders fresh data.
