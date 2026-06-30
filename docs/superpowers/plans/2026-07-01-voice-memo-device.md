# Voice Memo Device Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn `firmware/apps/factory_program_custom` from the copied factory test program into the first voice memo firmware.

**Architecture:** Keep the verified BSP and startup path, but replace factory-test tasks with focused voice memo modules. Pure logic such as WAV headers, memo filenames, list paging, and duration formatting is separated from hardware access so it can be host-tested before ESP-IDF integration.

**Tech Stack:** ESP-IDF, FreeRTOS, LVGL 9, ES8311 codec through `esp_codec_dev`, SDMMC/FATFS, PCF85063 RTC, FT6336 touch, C++.

---

## File Structure

- Create `firmware/apps/factory_program_custom/components/app/memo_types.h`: shared memo metadata, app state enums, touch action enums.
- Create `firmware/apps/factory_program_custom/components/app/memo_utils.h/.cpp`: pure helpers for WAV headers, duration formatting, filename generation, page calculations, and WAV metadata parsing.
- Create `firmware/apps/factory_program_custom/components/app/memo_storage.h/.cpp`: SD card memo directory creation, memo scanning, sequence allocation, and file removal for failed recordings.
- Create `firmware/apps/factory_program_custom/components/app/memo_recorder.h/.cpp`: streaming WAV recording lifecycle using codec read APIs.
- Create `firmware/apps/factory_program_custom/components/app/memo_player.h/.cpp`: streaming WAV playback lifecycle using codec write APIs.
- Create `firmware/apps/factory_program_custom/components/app/voice_ui.h/.cpp`: LVGL UI for list, recording, detail, errors, and shutdown screen.
- Create `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`: host-testable pure logic tests.
- Modify `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`: compile new modules and exclude host-only tests from ESP-IDF build.
- Modify `firmware/apps/factory_program_custom/components/app/user_app.cpp/.h`: replace factory test orchestration with voice memo app orchestration.
- Keep `firmware/apps/factory_program_custom/main/main.cpp` unchanged unless a display flush integration issue appears.
- Keep `firmware/official/v2/esp-idf/11_FactoryProgram` unchanged.

## Task 1: Pure Memo/WAV Utilities

**Files:**
- Create: `firmware/apps/factory_program_custom/components/app/memo_types.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_utils.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_utils.cpp`
- Create: `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [ ] **Step 1: Write host tests first**

Create `test/test_memo_utils.cpp` that compiles with system `c++` and tests:

- `FormatDuration(0) == "00:00"`
- `FormatDuration(83) == "01:23"`
- `FormatDuration(3723) == "62:03"`
- `BuildMemoFilename("/sdcard/memos", 7, true, 9, 4) == "/sdcard/memos/MEMO_0007_0904.wav"`
- `BuildMemoFilename("/sdcard/memos", 7, false, 0, 0) == "/sdcard/memos/MEMO_0007.wav"`
- `CalculatePageCount(0, 4) == 1`
- `CalculatePageCount(5, 4) == 2`
- `ClampPage(4, 2) == 1`
- A generated WAV header has `RIFF`, `WAVE`, `fmt `, `data`, 16000 Hz, 16-bit, 2 channels, and expected data size.

- [ ] **Step 2: Run host test and confirm RED**

Run:

```bash
c++ -std=c++17 \
  firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp \
  firmware/apps/factory_program_custom/components/app/memo_utils.cpp \
  -I firmware/apps/factory_program_custom/components/app \
  -o /tmp/test_memo_utils && /tmp/test_memo_utils
```

Expected before implementation: compile fails because the utility files or symbols do not exist.

- [ ] **Step 3: Implement minimal pure utilities**

Add:

- `MemoMetadata` with sequence, filename, display time, duration seconds, and byte size.
- `WavFormat` with sample rate, bits per sample, channels.
- `WavHeader` packed 44-byte structure.
- `MakeWavHeader(format, data_size)`.
- `PatchWavSizes(FILE*, data_size)`.
- `FormatDuration(seconds)`.
- `BuildMemoFilename(base_dir, sequence, has_time, hour, minute)`.
- `CalculatePageCount(item_count, page_size)`.
- `ClampPage(page, page_count)`.
- `BytesToDurationSeconds(data_bytes, format)`.
- `ReadWavHeader(FILE*, WavHeader*)`.
- `IsSupportedWavHeader(header, format)`.

- [ ] **Step 4: Run host test and confirm GREEN**

Run the same `c++` command. Expected: all tests pass and print a short success line.

- [ ] **Step 5: ESP-IDF build smoke**

Run:

```bash
. /Users/wq/esp-idf/export.sh >/tmp/voice-memo-export.log 2>&1
cd firmware/apps/factory_program_custom
idf.py build
```

Expected: project builds, proving the new files compile under ESP-IDF.

- [ ] **Step 6: Commit**

Commit:

```bash
git add firmware/apps/factory_program_custom/components/app
git commit -m "Add memo WAV utility layer"
```

## Task 2: Memo Storage Layer

**Files:**
- Create: `firmware/apps/factory_program_custom/components/app/memo_storage.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_storage.cpp`
- Extend: `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [ ] **Step 1: Add tests for filename parsing and sequence allocation helpers**

Extend the host tests for pure helper functions used by storage:

- `ParseMemoSequence("MEMO_0007_0904.wav") == 7`
- `ParseMemoSequence("MEMO_0007.wav") == 7`
- `ParseMemoSequence("bad.wav") == -1`
- `NextSequence({1, 2, 4}) == 5`
- `NextSequence({}) == 1`

- [ ] **Step 2: Run host test and confirm RED**

Run the same host test command. Expected: compile fails because the new helper functions do not exist.

- [ ] **Step 3: Implement storage layer**

Implement `MemoStorage` with:

- `esp_err_t Init()`: creates `/sdcard/memos` if SD card is mounted.
- `esp_err_t Scan(std::vector<MemoMetadata>& out)`: lists `MEMO_*.wav`, reads WAV metadata, sorts by sequence.
- `uint32_t NextSequence(const std::vector<MemoMetadata>& memos)`.
- `std::string BuildPathForSequence(sequence, has_time, hour, minute)`.
- `void RemoveFile(const char* path)`.

Keep filesystem-specific code in `memo_storage.cpp`; pure parse helpers remain in `memo_utils.cpp`.

- [ ] **Step 4: Run host tests**

Expected: host tests pass.

- [ ] **Step 5: ESP-IDF build**

Run `idf.py build` from `firmware/apps/factory_program_custom`. Expected: success.

- [ ] **Step 6: Commit**

Commit:

```bash
git add firmware/apps/factory_program_custom/components/app
git commit -m "Add memo storage layer"
```

## Task 3: Recorder and Player

**Files:**
- Create: `firmware/apps/factory_program_custom/components/app/memo_recorder.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_recorder.cpp`
- Create: `firmware/apps/factory_program_custom/components/app/memo_player.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_player.cpp`
- Modify: `firmware/apps/factory_program_custom/components/port_bsp/port_codec.h` if a small read/write declaration cleanup is needed.
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [ ] **Step 1: Define recorder API**

Implement:

- `esp_err_t MemoRecorder::Start(const char* path, const WavFormat& format)`
- `esp_err_t MemoRecorder::CaptureChunk()`
- `esp_err_t MemoRecorder::Stop(MemoMetadata* out_metadata)`
- `void MemoRecorder::Abort()`
- `bool MemoRecorder::IsRecording() const`
- `uint32_t MemoRecorder::ElapsedSeconds() const`

Recording writes a placeholder WAV header, appends codec data in chunks, then patches RIFF/data sizes on stop.

- [ ] **Step 2: Define player API**

Implement:

- `esp_err_t MemoPlayer::Start(const char* path, const WavFormat& expected_format)`
- `esp_err_t MemoPlayer::PlayChunk()`
- `void MemoPlayer::Stop()`
- `bool MemoPlayer::IsPlaying() const`
- `uint32_t MemoPlayer::ElapsedSeconds() const`
- `uint32_t MemoPlayer::DurationSeconds() const`

Playback validates WAV header, streams file data to codec, and stops at EOF.

- [ ] **Step 3: ESP-IDF build**

Run `idf.py build`. Expected: success.

- [ ] **Step 4: Commit**

Commit:

```bash
git add firmware/apps/factory_program_custom/components/app firmware/apps/factory_program_custom/components/port_bsp/port_codec.h
git commit -m "Add memo recorder and player"
```

## Task 4: Voice Memo UI

**Files:**
- Create: `firmware/apps/factory_program_custom/components/app/voice_ui.h`
- Create: `firmware/apps/factory_program_custom/components/app/voice_ui.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [ ] **Step 1: Implement LVGL page objects**

Create one root screen with reusable objects:

- Header label and status label.
- Four list row containers/labels.
- Detail title/time/duration labels.
- Recording elapsed label.
- Two action buttons for `PLAY/STOP` and `BACK`.
- Shutdown screen labels and simple battery indicator.

- [ ] **Step 2: Implement render methods**

Implement:

- `ShowList(memos, page, battery_percent, sd_ready, message)`
- `ShowRecording(elapsed_seconds)`
- `ShowDetail(memo, playing, elapsed_seconds)`
- `ShowError(message)`
- `ShowShutdown(memo_count, battery_percent, last_memo)`

- [ ] **Step 3: Implement coordinate hit testing**

For the first version, use deterministic coordinate regions:

- List rows: four bands below header.
- Detail buttons: lower-left/lower-right regions.
- Swipe detection: compare touch press and release Y values.

Return actions as `VoiceUiAction`.

- [ ] **Step 4: ESP-IDF build**

Run `idf.py build`. Expected: success.

- [ ] **Step 5: Commit**

Commit:

```bash
git add firmware/apps/factory_program_custom/components/app
git commit -m "Add voice memo LVGL UI"
```

## Task 5: Replace Factory App Orchestration

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/user_app.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/user_app.h`
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [ ] **Step 1: Replace factory global state**

Remove factory UI state and flags. Add:

- SD-ready flag.
- RTC-ready flag.
- `MemoStorage`, `MemoRecorder`, `MemoPlayer`, `VoiceUi`.
- Vector of `MemoMetadata`.
- Current page.
- Selected memo index.
- App state enum.
- Event group bits for BOOT down, BOOT up, PWR long, touch interrupt, and periodic tick.

- [ ] **Step 2: Update initialization**

Keep:

- `BoardPower_Init`, `BoardPower_EPD_ON`, `BoardPower_Audio_ON`, `BoardPower_VBAT_ON`.
- I2C, FT6336, RTC, buttons, SD card, ADC, SHTC3 if still useful for status, touch interrupt, codec.

Remove:

- Factory SD write/read test.
- Forced RTC reset to `2026-01-01 08:00:00`.
- Factory UI setup.

- [ ] **Step 3: Update button behavior**

Use:

- BOOT `OnPressDown`: set record-start event.
- BOOT `OnPressUp`: set record-stop event.
- PWR `OnLongPress`: set shutdown event.

- [ ] **Step 4: Implement app loop**

Use a single app task or small tasks to:

- Start recording on BOOT down when SD is ready.
- Capture chunks while recording.
- Stop and finalize on BOOT up.
- Refresh memo list after save.
- Page on swipe.
- Open detail on row tap.
- Play/stop from detail.
- Draw shutdown screen and power off on PWR long.

- [ ] **Step 5: ESP-IDF build**

Run `idf.py build`. Expected: success.

- [ ] **Step 6: Commit**

Commit:

```bash
git add firmware/apps/factory_program_custom/components/app
git commit -m "Replace factory flow with voice memo app"
```

## Task 6: Final Verification and Cleanup

**Files:**
- Modify only if verification exposes a concrete issue.

- [ ] **Step 1: Run host tests**

Run the memo utils host test. Expected: pass.

- [ ] **Step 2: Run ESP-IDF build**

Run `idf.py build` from `firmware/apps/factory_program_custom`. Expected: success.

- [ ] **Step 3: Remove generated `sdkconfig` if it appears**

If `firmware/apps/factory_program_custom/sdkconfig` is untracked, remove it so `sdkconfig.defaults` remains the source-controlled config seed.

- [ ] **Step 4: Confirm official example unchanged**

Run:

```bash
git diff --name-only -- firmware/official/v2/esp-idf/11_FactoryProgram
```

Expected: no output.

- [ ] **Step 5: Final status**

Run:

```bash
git status --short
git log --oneline -8
```

Expected: clean status after final commits.

- [ ] **Step 6: Commit fixes if needed**

If verification required source changes, commit them with:

```bash
git add <changed-files>
git commit -m "Stabilize voice memo app build"
```
