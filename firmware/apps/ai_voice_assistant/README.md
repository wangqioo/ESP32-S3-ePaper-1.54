# AI Voice Assistant

Hold BOOT to record a question. Release BOOT to stop recording, write a temporary
WAV file, upload it to the local Mac proxy, and display the returned answer on the
200 x 200 e-paper screen.

PWR short press returns from an answer or error screen to home. PWR long press
powers off when the app is idle, on an answer screen, or on an error screen.
During recording or upload, shutdown is rejected so the WAV file is not corrupted.

## Configuration

Edit `components/app/assistant_config.h` locally:

- `kWifiSsid`
- `kWifiPassword`
- `kProxyHost`
- `kProxyPort`

The public repository keeps these values empty. Empty Wi-Fi credentials mean the
device shows offline/not-configured status instead of crashing.

## Proxy

Start the local proxy first:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

OpenAI mode runs only on the Mac proxy:

```bash
OPENAI_API_KEY=... python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode openai
```

The firmware never contains API keys.

## Build

```bash
cd firmware/apps/ai_voice_assistant
source /Users/wq/esp-idf/export.sh
idf.py build
```

Before flashing, verify the target board MAC is the intended ESP32-S3 e-paper
board. This workspace has previously used MAC `70:04:1d:db:f9:6c` for the target
board.
