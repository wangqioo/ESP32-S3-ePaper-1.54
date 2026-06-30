# Voice Memo Device Design

## Goal

Build the first application version of the ESP32-S3 ePaper 1.54 board as a voice memo device. The app will be developed from `firmware/apps/factory_program_custom`, keeping the already verified board initialization, display, touch, SD card, audio codec, battery ADC, RTC, and button drivers.

The device records voice notes to the Micro SD card when the user holds BOOT, lets the user browse saved memos on the touch screen, plays selected memos from a detail screen, and shows a polished static shutdown screen before power-off.

## Confirmed Product Behavior

- Press and hold BOOT to start recording.
- Release BOOT to stop recording and save the memo.
- Save recordings as normal `.wav` files on the SD card.
- Do not impose a fixed maximum recording duration.
- Stop recording automatically if the SD card is full, SD card write fails, or the card becomes unavailable.
- Require an SD card for recording. If no SD card is mounted, the app shows a `NO SD CARD` state and refuses to record.
- Show recordings in a paged list, 4 items per page.
- Use upward swipe for next page and downward swipe for previous page.
- Tap a memo row to open a detail page.
- In the detail page, use explicit `PLAY` / `STOP` and `BACK` controls instead of playing immediately from the list.
- Long press PWR to shut down.
- Before shutting down, refresh a custom 200 x 200 e-paper shutdown screen; then cut power so the screen retains the final image.

## Recording Format

The app will write WAV files instead of raw PCM files.

Initial audio format will match the current codec path unless hardware testing proves a change is needed:

- Sample rate: 16000 Hz
- Sample width: 16 bit
- Channels: 2
- Container: WAV

This keeps playback compatible with the existing ES8311 codec path and makes files easy to copy from SD card to a computer.

## File Storage

Recordings live under:

```text
/sdcard/memos/
```

Preferred filename format when RTC time is usable:

```text
MEMO_0001_0904.wav
MEMO_0002_0918.wav
```

Fallback filename format when time is unavailable or unreliable:

```text
MEMO_0001.wav
MEMO_0002.wav
```

The sequence number is the primary ordering key. RTC time is useful for display and filenames, but the app must still work when RTC time is not trustworthy.

## UI

The app replaces the factory test UI with voice memo pages.

### List Page

The main page shows up to 4 recordings per page:

```text
001  09:04  00:12
002  09:18  01:28
003  10:07  00:45
004  10:31  00:08
```

The list also shows basic status such as battery and SD card availability. If there are no memos, it shows an empty state. If the SD card is missing, it shows `NO SD CARD`.

### Recording Page

While BOOT is held, the app shows a recording state:

```text
RECORDING
00:18
Release BOOT to save
```

The elapsed timer counts upward. On release, the app saves the WAV file and returns to the list page with the new memo visible.

### Detail Page

Tapping a row opens a detail page:

```text
MEMO 002
09:18
Duration 01:28

PLAY
BACK
```

During playback, the page changes to show playback state and a `STOP` action:

```text
MEMO 002
Playing...
00:23 / 01:28

STOP
BACK
```

### Shutdown Screen

The shutdown screen uses the approved 200 x 200 fitted status layout:

- Header: `VOICE MEMO` plus an `OFF` status badge.
- Memo count card.
- Battery card with a simple battery icon and percentage.
- Last saved time.
- Last memo duration.
- Compact `READY FOR NEXT MEMO` status message.

This is drawn before power-off and then retained by the e-paper display after power is cut.

## Architecture

Keep `factory_program_custom` as the application working directory, but replace factory-test behavior with voice memo modules.

Recommended module boundaries:

- `voice_app`: top-level app state machine and task orchestration.
- `voice_ui`: LVGL page creation and updates for list, recording, detail, errors, and shutdown screen.
- `memo_storage`: SD card directory creation, memo scanning, sequence allocation, WAV metadata, and delete-safe file operations.
- `memo_recorder`: BOOT hold-to-record flow and streaming WAV write.
- `memo_player`: WAV file playback and playback state.
- Existing `port_bsp`: hardware drivers for display, touch, SD card, codec, RTC, ADC, power, and buttons.

The current factory test logic should be removed or bypassed:

- Factory SD card write/read test using `waveshare.com`.
- Four-corner touch test page.
- BOOT single-click record/playback test.
- Always resetting RTC to `2026-01-01 08:00:00` on boot.
- LED blink task unless it remains useful as a temporary debug indicator.

## Interaction Model

Use button press-down and press-up callbacks for BOOT:

- BOOT press down: start recording if SD card is ready and not already recording or playing.
- BOOT release: stop recording and finalize the WAV file.

Use PWR long press for shutdown:

- Render shutdown screen.
- Wait until display update is complete.
- Cut VBAT power using the existing board power control.

Use touch input for:

- Swipe up/down on list page for paging.
- Tap row on list page for detail page.
- Tap `PLAY`, `STOP`, and `BACK` on detail page.

## Error Handling

Minimum first-version error states:

- `NO SD CARD`: SD card mount failed or card unavailable.
- `SAVE FAILED`: file open, write, or WAV finalization failed.
- `CARD FULL`: write failed due to no space when detectable.
- `PLAY FAILED`: selected file cannot be opened or decoded as expected.
- `LOW BATTERY`: optional later state if a battery protection threshold is added.

On failed recording, partial files should be closed and either deleted or marked invalid so they do not appear as playable memos.

## Implementation Notes

- Current `port_sdcard` only has simple whole-file helpers; voice memo recording needs streaming file operations through standard C file APIs or a storage wrapper.
- WAV writing must reserve and later patch the RIFF and data sizes when recording stops.
- The current codec sample configuration opens playback and record at startup. The first version can reuse this path, but recording and playback should not run at the same time.
- Touch handling in the factory program is coordinate-based. The new app may continue using coordinate regions for the first version, or register LVGL input events if that proves more reliable.
- The app should avoid unnecessary full-screen refreshes during recording. A 1-second elapsed timer update is acceptable for the first version if display behavior is acceptable on hardware.

## Acceptance Criteria

- App builds successfully with ESP-IDF.
- On boot with SD card inserted, list page appears.
- Holding BOOT records audio until BOOT is released.
- Released recording is saved as a playable `.wav` file under `/sdcard/memos/`.
- New memo appears in the list with sequence, time when available, and duration.
- List paging works by up/down swipe with 4 items per page.
- Tapping a memo opens detail page.
- Detail page can play, stop, and go back.
- Booting without SD card shows `NO SD CARD` and does not record.
- Long pressing PWR draws the approved shutdown screen before power-off.
- Official example directory under `firmware/official/v2/esp-idf/11_FactoryProgram` remains unchanged.
