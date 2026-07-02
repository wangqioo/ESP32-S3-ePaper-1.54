# AI Voice Assistant

Current stable release: `v0.1.0`.

Hold BOOT to record a question. Release BOOT to stop recording, write a temporary
WAV file, upload it to the local Mac proxy, and display the returned answer on the
200 x 200 e-paper screen.

## v0.1.0 Features

- Press-and-hold BOOT voice input.
- Release BOOT to stop, save the WAV, upload, and show the AI answer.
- Local Mac proxy for API access, so GLM keys are never built into firmware.
- GLM speech-to-text and answer generation through the proxy.
- 14 px Chinese LVGL font for e-paper answer rendering.
- Proxy-side emoji and decorative symbol filtering for display-safe text.
- Flash storage fallback when the SD card is missing or fails to mount.
- Clear intermediate states: Recording, Saving, Uploading, Answer, Error.
- Safe power behavior: PWR long press is rejected while recording/saving/uploading.

## Controls

- BOOT down: start recording.
- BOOT up: stop recording and enter Saving.
- PWR short press: return from Answer or Error to the home screen.
- PWR long press: power off only when idle, on Answer, or on Error.

## Configuration

For local builds, create
`components/app/assistant_config_local.h`:

```cpp
#pragma once

#define ASSISTANT_WIFI_SSID "your-wifi"
#define ASSISTANT_WIFI_PASSWORD "your-password"
#define ASSISTANT_PROXY_HOST "192.168.x.x"
```

The public defaults live in `components/app/assistant_config.h`:

- `kWifiSsid`
- `kWifiPassword`
- `kProxyHost`
- `kProxyPort`

The public repository keeps the local override ignored by git. Empty Wi-Fi
credentials mean the device shows offline/not-configured status instead of
crashing.

## Proxy

Start the local proxy first. Mock mode is useful for basic firmware testing:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

GLM mode runs only on the Mac proxy. Keep the key in your shell environment,
`.env`, launchd service, or another local secret store:

```bash
GLM_API_KEY=... python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode glm
```

The firmware never contains API keys. The proxy defaults match Voice Keyboard:
`glm-asr-2512` for speech-to-text and `glm-4-flash` for answers.

The proxy normalizes GLM responses before sending them to the ESP32:

- Converts stereo WAV uploads to mono for GLM ASR compatibility.
- Uses an explicit certifi SSL context on macOS.
- Requests compact answers for a 200 x 200 e-paper screen.
- Removes emoji, emoji variation selectors, keycap sequences, and decorative
  symbols such as `TM`, arrows, hearts, and celebration icons.

## Build

```bash
cd firmware/apps/ai_voice_assistant
source /Users/wq/esp-idf/export.sh
idf.py build
```

Flash:

```bash
idf.py -p /dev/cu.usbmodem11101 flash
```

Before flashing, verify the target board MAC is the intended ESP32-S3 e-paper
board. This workspace has previously used MAC `70:04:1d:db:f9:6c` for the target
board.

## Expected Flow

1. Home screen shows Ready when Wi-Fi and proxy settings are configured.
2. Hold BOOT and speak.
3. Release BOOT.
4. Screen switches immediately to Saving while the WAV file is finalized.
5. Screen switches to Uploading while the proxy handles ASR and answer generation.
6. Answer screen shows compact display-safe text.
7. Press PWR shortly to return home.

## Known Notes

- The SD card is optional for this app. If SD mount fails, the app uses `/flash`.
- Long recordings take longer to save because v0.1.0 writes the WAV after release.
  The Saving page exists so this does not look like a freeze.
- If the answer contains rare Chinese characters outside the bundled font subset,
  those characters may not render. The proxy still removes emoji before display.
- If the device cannot reach the proxy, confirm the Mac IP address, firewall, port
  `8787`, and that the proxy health check returns `{"ok":true,"mode":"glm"}`.
