# AI Voice Assistant Design

## Goal

Build a first AI voice assistant app for the ESP32-S3-ePaper-1.54 board. The device records speech while BOOT is held, uploads the captured audio to a Mac-hosted proxy service, and displays a short AI response on the e-paper screen.

## Product Scope

Version 1 focuses on proving the full device-to-AI loop with minimal moving parts:

- Hold BOOT to record.
- Release BOOT to stop recording and upload.
- Show recording, uploading, thinking, answer, and error states on the 200 x 200 e-paper screen.
- Return text only in V1.
- Reserve a playback affordance in the UI for a future TTS version.
- Keep PWR long press as the known-good shutdown path.

V1 does not include wake word detection, always-listening audio, offline speech recognition, or on-device AI inference.

## Architecture

Create a new ESP-IDF app at `firmware/apps/ai_voice_assistant/` instead of modifying the voice memo app in place. The new app can reuse proven modules and patterns from `factory_program_custom`, especially recorder, codec, UI, button, flash fallback, and power handling.

Use a local Mac proxy service for AI calls. The ESP32 uploads audio to the proxy over HTTP. The proxy owns OpenAI credentials, speech recognition, response generation, and later TTS. This keeps API keys out of firmware and makes debugging network and AI failures easier.

## Device Interaction

- Home: show assistant idle state and connection target.
- BOOT down: start recording and show elapsed time.
- BOOT up: stop recording, finalize WAV, and begin upload.
- During upload/thinking: show a non-interactive busy state.
- Answer: show concise response text, plus a disabled or placeholder play control.
- PWR short press: return to home or cancel current answer/error screen.
- PWR long press: enter the existing board power-off/deep-sleep path after button release.

If recording or uploading is active, PWR long press should not corrupt the WAV file. It should either wait for a safe stop or reject shutdown with an on-screen message.

## Device Data Flow

1. Initialize board power, display, buttons, I2C, codec, and optional storage.
2. Record audio using the known-good microphone path from the voice memo app.
3. Save the WAV temporarily to Flash or SD, using the same storage fallback rules as voice memo.
4. Upload the WAV to the proxy endpoint.
5. Parse a compact JSON response.
6. Render the answer text and save the last answer for recovery after a transient UI refresh.

The upload format should be a simple HTTP POST with WAV bytes. The response should be:

```json
{
  "ok": true,
  "text": "Short answer for the e-paper screen.",
  "request_id": "optional-debug-id"
}
```

Errors should use the same response shape with `ok: false` and an `error` string.

## Mac Proxy Service

The first proxy runs on the development Mac. It exposes one endpoint:

- `POST /ask`: accepts WAV audio and returns a short text answer.

The proxy reads `OPENAI_API_KEY` from the environment. It should not log raw audio content or API keys. It may log request duration, response status, byte counts, and request IDs.

The first implementation can return a fixed mock answer before wiring OpenAI. Once the firmware upload path is stable, the proxy can call OpenAI for transcription and response generation.

## Configuration

Public repository defaults must not include private Wi-Fi credentials, local IPs, or API keys.

Device configuration should keep these values in one file:

- Wi-Fi SSID
- Wi-Fi password
- proxy host
- proxy port
- request timeout

Empty Wi-Fi credentials are valid and mean offline mode.

## UI Requirements

The UI should be readable on a 200 x 200 e-paper panel:

- Use short status labels.
- Wrap or truncate long answers deliberately.
- Prefer one answer screen over scrolling for V1.
- Keep a clear error state for Wi-Fi unavailable, proxy unavailable, upload failure, AI failure, and recording failure.
- Avoid frequent redraws while uploading unless there is an explicit progress indicator.

## Power And Sleep

V1 should not add automatic deep sleep. Keep the same conservative power model used in the current apps:

- The assistant remains awake during active use.
- Long PWR press performs explicit shutdown.
- The screen keeps its last image after power-off.

Automatic sleep can be revisited after the assistant loop is reliable.

## Security And Privacy

- Never put OpenAI API keys in ESP32 firmware.
- Do not commit private Wi-Fi credentials.
- Do not commit local proxy IPs if they identify the private network.
- Avoid storing long-lived recordings unless the user explicitly chooses to keep them.
- Treat temporary WAV files as disposable request artifacts.

## Testing Strategy

Start with host-testable units:

- request/response JSON parsing
- answer text wrapping/truncation policy
- state transitions for idle, recording, uploading, answer, and error
- configuration defaults with empty credentials

Then verify on hardware:

- BOOT hold records audio.
- BOOT release uploads audio.
- proxy receives WAV bytes.
- device displays mock answer.
- Wi-Fi/proxy failure shows a readable error.
- PWR long press still shuts down cleanly.

## Acceptance Criteria

- A new `ai_voice_assistant` app builds with ESP-IDF.
- The app does not modify or break the existing voice memo app.
- With mock proxy mode, the board records audio, uploads it, and displays a returned answer.
- With Wi-Fi credentials empty, the board shows offline mode instead of crashing.
- API keys and private Wi-Fi credentials are absent from the repository.
- README documents the new app and local proxy workflow.
