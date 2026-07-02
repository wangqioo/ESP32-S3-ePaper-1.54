# AI Voice Assistant Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an independent ESP32-S3 e-paper AI voice assistant app that records while BOOT is held, uploads the WAV to a Mac proxy, and displays a short answer.

**Architecture:** Add a new app at `firmware/apps/ai_voice_assistant` so the stable voice memo app remains untouched. Reuse the proven recorder, storage fallback, power, codec, button, and LVGL startup patterns from `factory_program_custom`, and reuse Wi-Fi/HTTP patterns from `desktop_helper`. Keep OpenAI credentials only in the Mac proxy; the firmware only talks to the local proxy.

**Tech Stack:** ESP-IDF 5.5, ESP32-S3, FreeRTOS, LVGL v9, ESP HTTP client, cJSON, NVS, existing e-paper/audio/storage/power BSP, Python 3 standard library for the first Mac proxy, OpenAI HTTP API behind environment variables.

---

## Reference Context

- Design spec: `docs/superpowers/specs/2026-07-03-ai-voice-assistant-design.md`
- Voice memo app to copy proven audio/storage/power logic from: `firmware/apps/factory_program_custom`
- Weather app to copy Wi-Fi/HTTP shape from: `firmware/apps/desktop_helper`
- Official OpenAI docs checked for proxy planning:
  - Speech-to-text accepts multipart uploads to `/v1/audio/transcriptions` with WAV support and transcription models such as `gpt-4o-transcribe` and `gpt-4o-mini-transcribe`.
  - Responses API creates model responses through `POST /responses` and accepts text input.

## File Structure

- Create: `tools/ai_proxy/server.py`  
  Local Mac HTTP server. Accepts `POST /ask`, returns compact JSON, supports `mock` and `openai` modes, never logs audio bytes or API keys.
- Create: `tools/ai_proxy/test_proxy_contract.py`  
  Host tests for request parsing, mock response shape, missing method errors, and OpenAI credential gating.
- Create: `tools/ai_proxy/README.md`  
  Documents how to run mock mode, OpenAI mode, firewall/network setup, and safe environment variables.
- Create: `firmware/apps/ai_voice_assistant/CMakeLists.txt`
- Create: `firmware/apps/ai_voice_assistant/main/CMakeLists.txt`
- Create: `firmware/apps/ai_voice_assistant/main/main.cpp`
- Create: `firmware/apps/ai_voice_assistant/sdkconfig.defaults`
- Create: `firmware/apps/ai_voice_assistant/partitions.csv`
- Copy/adapt: `firmware/apps/ai_voice_assistant/components/port_bsp` from `firmware/apps/factory_program_custom/components/port_bsp`
- Copy/adapt: `firmware/apps/ai_voice_assistant/components/externlib` from `firmware/apps/factory_program_custom/components/externlib`
- Create: `firmware/apps/ai_voice_assistant/components/app/CMakeLists.txt`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_config.h`  
  Public-safe compile-time settings with empty defaults for Wi-Fi and proxy host.
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_types.h`  
  Small enums and structs shared by model, client, UI, and app orchestration.
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_model.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_model.cpp`  
  Pure state transitions, error labels, answer wrapping/truncation helpers.
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_client.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_client.cpp`  
  Wi-Fi connect, WAV upload, HTTP response buffering, JSON parse.
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_ui.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_ui.cpp`  
  200 x 200 e-paper screens for home, recording, uploading, thinking, answer, and error.
- Create: `firmware/apps/ai_voice_assistant/components/app/user_app.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/user_app.cpp`  
  Hardware initialization, button events, recorder lifecycle, upload worker, power-off safety.
- Copy/adapt: `memo_recorder.*`, `memo_player.*` only if playback/TTS placeholder needs no compile-time stubs. V1 should copy `memo_recorder.*`, `memo_storage.*`, `memo_types.h`, `memo_utils.*`, `power_sleep_policy.*`, and `audio_loop_policy.*` from `factory_program_custom`.
- Create: `firmware/apps/ai_voice_assistant/components/app/test/test_assistant_model.cpp`
- Modify: `README.md`  
  Add app summary and point to app/proxy docs.
- Create: `firmware/apps/ai_voice_assistant/README.md`  
  Build/flash/run workflow, configuration, and hardware interaction guide.

## Task 1: Mac Proxy Contract In Mock Mode

**Files:**
- Create: `tools/ai_proxy/server.py`
- Create: `tools/ai_proxy/test_proxy_contract.py`
- Create: `tools/ai_proxy/README.md`

- [ ] **Step 1: Write host tests for the proxy contract**

Create `tools/ai_proxy/test_proxy_contract.py`:

```python
import json
import os
import tempfile
import threading
import time
import unittest
import urllib.error
import urllib.request

import server


class ProxyContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.httpd = server.create_server("127.0.0.1", 0, mode="mock")
        cls.port = cls.httpd.server_address[1]
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.thread.join(timeout=2)

    def post_wav(self, payload=b"RIFF....WAVEfmt "):
        req = urllib.request.Request(
            f"http://127.0.0.1:{self.port}/ask",
            data=payload,
            headers={"Content-Type": "audio/wav"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=3) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))

    def test_mock_ask_returns_compact_success_json(self):
        status, body = self.post_wav()
        self.assertEqual(status, 200)
        self.assertEqual(body["ok"], True)
        self.assertIsInstance(body["text"], str)
        self.assertLessEqual(len(body["text"]), 120)
        self.assertIn("request_id", body)

    def test_wrong_path_returns_json_error(self):
        req = urllib.request.Request(
            f"http://127.0.0.1:{self.port}/missing",
            data=b"RIFF....WAVEfmt ",
            headers={"Content-Type": "audio/wav"},
            method="POST",
        )
        with self.assertRaises(urllib.error.HTTPError) as raised:
            urllib.request.urlopen(req, timeout=3)
        self.assertEqual(raised.exception.code, 404)
        body = json.loads(raised.exception.read().decode("utf-8"))
        self.assertEqual(body["ok"], False)
        self.assertIn("error", body)

    def test_openai_mode_requires_api_key(self):
        old_key = os.environ.pop("OPENAI_API_KEY", None)
        try:
            with self.assertRaises(RuntimeError):
                server.create_server("127.0.0.1", 0, mode="openai")
        finally:
            if old_key is not None:
                os.environ["OPENAI_API_KEY"] = old_key


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests and verify they fail before implementation**

Run:

```bash
python3 -m unittest tools/ai_proxy/test_proxy_contract.py
```

Expected: fail with `ModuleNotFoundError: No module named 'server'`.

- [ ] **Step 3: Implement the mock proxy**

Create `tools/ai_proxy/server.py`:

```python
#!/usr/bin/env python3
import argparse
import json
import os
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


MAX_WAV_BYTES = 2 * 1024 * 1024


def json_bytes(body):
    return json.dumps(body, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


class AssistantProxyHandler(BaseHTTPRequestHandler):
    server_version = "AiVoiceProxy/0.1"

    def log_message(self, fmt, *args):
        print("%s - %s" % (self.address_string(), fmt % args))

    def send_json(self, status, body):
        payload = json_bytes(body)
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"ok": True, "mode": self.server.mode})
            return
        self.send_json(404, {"ok": False, "error": "not_found"})

    def do_POST(self):
        request_id = uuid.uuid4().hex[:12]
        started = time.monotonic()
        if self.path != "/ask":
            self.send_json(404, {"ok": False, "error": "not_found", "request_id": request_id})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_json(400, {"ok": False, "error": "bad_content_length", "request_id": request_id})
            return
        if length <= 0:
            self.send_json(400, {"ok": False, "error": "empty_audio", "request_id": request_id})
            return
        if length > MAX_WAV_BYTES:
            self.send_json(413, {"ok": False, "error": "audio_too_large", "request_id": request_id})
            return
        audio = self.rfile.read(length)
        if not audio.startswith(b"RIFF") or b"WAVE" not in audio[:16]:
            self.send_json(400, {"ok": False, "error": "not_wav", "request_id": request_id})
            return
        if self.server.mode == "mock":
            text = "Mock answer: I heard your question. OpenAI mode can be enabled after upload is stable."
        else:
            text = ask_openai(audio, request_id)
        elapsed_ms = int((time.monotonic() - started) * 1000)
        print(f"request_id={request_id} mode={self.server.mode} bytes={length} elapsed_ms={elapsed_ms}")
        self.send_json(200, {"ok": True, "text": text[:180], "request_id": request_id})


def ask_openai(audio, request_id):
    raise RuntimeError("openai mode is implemented in Task 8")


def create_server(host, port, mode):
    if mode == "openai" and not os.environ.get("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is required for openai mode")
    httpd = ThreadingHTTPServer((host, port), AssistantProxyHandler)
    httpd.mode = mode
    return httpd


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--mode", choices=("mock", "openai"), default=os.environ.get("AI_PROXY_MODE", "mock"))
    args = parser.parse_args()
    httpd = create_server(args.host, args.port, args.mode)
    print(f"AI voice proxy listening on http://{args.host}:{args.port} mode={args.mode}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Run tests and verify they pass**

Run:

```bash
python3 -m unittest tools/ai_proxy/test_proxy_contract.py
```

Expected: `OK`.

- [ ] **Step 5: Document proxy mock mode**

Create `tools/ai_proxy/README.md`:

```markdown
# AI Voice Proxy

This local Mac proxy keeps API keys off the ESP32 firmware.

Run mock mode:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

Health check:

```bash
curl http://127.0.0.1:8787/health
```

Ask endpoint:

```bash
curl -X POST http://127.0.0.1:8787/ask \
  -H 'Content-Type: audio/wav' \
  --data-binary @example.wav
```

OpenAI mode uses `OPENAI_API_KEY` from the environment. Do not commit real keys,
Wi-Fi passwords, local IP addresses, or captured recordings.
```

- [ ] **Step 6: Commit**

```bash
git add tools/ai_proxy
git commit -m "feat: add AI voice proxy mock contract"
```

## Task 2: Pure Assistant Model

**Files:**
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_types.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_model.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_model.cpp`
- Create: `firmware/apps/ai_voice_assistant/components/app/test/test_assistant_model.cpp`

- [ ] **Step 1: Write host tests for state and text helpers**

Create `firmware/apps/ai_voice_assistant/components/app/test/test_assistant_model.cpp`:

```cpp
#include "../assistant_model.h"

#include <cassert>
#include <string>
#include <vector>

int main() {
    assert(AssistantStatusLabel(AssistantState::Idle) == std::string("Ready"));
    assert(AssistantStatusLabel(AssistantState::Recording) == std::string("Recording"));
    assert(AssistantStatusLabel(AssistantState::Uploading) == std::string("Uploading"));
    assert(AssistantErrorLabel(AssistantError::WifiUnavailable) == std::string("Wi-Fi unavailable"));

    auto wrapped = WrapAssistantText("Shanghai weather and clock are ready", 12, 3);
    assert(wrapped.size() == 3);
    assert(wrapped[0] == "Shanghai");
    assert(wrapped[1] == "weather and");
    assert(wrapped[2] == "clock are...");

    AssistantSession session;
    assert(session.state == AssistantState::Idle);
    ApplyAssistantEvent(session, AssistantEvent::BootDown);
    assert(session.state == AssistantState::Recording);
    ApplyAssistantEvent(session, AssistantEvent::BootUp);
    assert(session.state == AssistantState::Uploading);
    ApplyAssistantAnswer(session, "你好，我在。", "abc123");
    assert(session.state == AssistantState::Answer);
    assert(session.answer == "你好，我在。");
    assert(session.request_id == "abc123");
    ApplyAssistantEvent(session, AssistantEvent::PowerShort);
    assert(session.state == AssistantState::Idle);
    return 0;
}
```

- [ ] **Step 2: Run test and verify it fails before implementation**

Run:

```bash
cd firmware/apps/ai_voice_assistant/components/app/test 2>/dev/null || true
c++ -std=c++17 test_assistant_model.cpp ../assistant_model.cpp -I.. -o /tmp/test_assistant_model
```

Expected: fail because the files do not exist yet.

- [ ] **Step 3: Implement assistant types**

Create `firmware/apps/ai_voice_assistant/components/app/assistant_types.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>

enum class AssistantState {
    Idle,
    Recording,
    Uploading,
    Thinking,
    Answer,
    Error,
    ShuttingDown,
};

enum class AssistantEvent {
    BootDown,
    BootUp,
    UploadStarted,
    UploadSucceeded,
    UploadFailed,
    PowerShort,
    PowerLong,
};

enum class AssistantError {
    None,
    WifiUnavailable,
    ProxyUnavailable,
    UploadFailed,
    AiFailed,
    RecordFailed,
    StorageFailed,
    Busy,
};

struct AssistantSession {
    AssistantState state = AssistantState::Idle;
    AssistantError error = AssistantError::None;
    std::string answer;
    std::string request_id;
    uint32_t elapsed_seconds = 0;
};
```

- [ ] **Step 4: Implement assistant model**

Create `firmware/apps/ai_voice_assistant/components/app/assistant_model.h`:

```cpp
#pragma once

#include "assistant_types.h"

#include <string>
#include <vector>

std::string AssistantStatusLabel(AssistantState state);
std::string AssistantErrorLabel(AssistantError error);
std::vector<std::string> WrapAssistantText(const std::string &text, size_t max_chars, size_t max_lines);
void ApplyAssistantEvent(AssistantSession &session, AssistantEvent event);
void ApplyAssistantError(AssistantSession &session, AssistantError error);
void ApplyAssistantAnswer(AssistantSession &session, const std::string &answer, const std::string &request_id);
```

Create `firmware/apps/ai_voice_assistant/components/app/assistant_model.cpp`:

```cpp
#include "assistant_model.h"

#include <algorithm>
#include <sstream>

std::string AssistantStatusLabel(AssistantState state) {
    switch (state) {
        case AssistantState::Idle: return "Ready";
        case AssistantState::Recording: return "Recording";
        case AssistantState::Uploading: return "Uploading";
        case AssistantState::Thinking: return "Thinking";
        case AssistantState::Answer: return "Answer";
        case AssistantState::Error: return "Error";
        case AssistantState::ShuttingDown: return "Power off";
    }
    return "Ready";
}

std::string AssistantErrorLabel(AssistantError error) {
    switch (error) {
        case AssistantError::None: return "";
        case AssistantError::WifiUnavailable: return "Wi-Fi unavailable";
        case AssistantError::ProxyUnavailable: return "Proxy unavailable";
        case AssistantError::UploadFailed: return "Upload failed";
        case AssistantError::AiFailed: return "AI failed";
        case AssistantError::RecordFailed: return "Record failed";
        case AssistantError::StorageFailed: return "Storage failed";
        case AssistantError::Busy: return "Busy";
    }
    return "Error";
}

std::vector<std::string> WrapAssistantText(const std::string &text, size_t max_chars, size_t max_lines) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string line;
    while (stream >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= max_chars) {
            line += " " + word;
        } else {
            lines.push_back(line);
            line = word;
            if (lines.size() + 1 >= max_lines) {
                break;
            }
        }
    }
    if (!line.empty() && lines.size() < max_lines) {
        lines.push_back(line);
    }
    if (lines.size() == max_lines && stream >> word && max_chars > 3) {
        std::string &last = lines.back();
        if (last.size() > max_chars - 3) {
            last.resize(max_chars - 3);
        }
        last += "...";
    }
    return lines;
}

void ApplyAssistantEvent(AssistantSession &session, AssistantEvent event) {
    switch (event) {
        case AssistantEvent::BootDown:
            if (session.state == AssistantState::Idle || session.state == AssistantState::Answer || session.state == AssistantState::Error) {
                session = AssistantSession{};
                session.state = AssistantState::Recording;
            } else {
                session.error = AssistantError::Busy;
            }
            break;
        case AssistantEvent::BootUp:
            if (session.state == AssistantState::Recording) {
                session.state = AssistantState::Uploading;
            }
            break;
        case AssistantEvent::UploadStarted:
            session.state = AssistantState::Uploading;
            break;
        case AssistantEvent::UploadSucceeded:
            session.state = AssistantState::Answer;
            session.error = AssistantError::None;
            break;
        case AssistantEvent::UploadFailed:
            ApplyAssistantError(session, AssistantError::UploadFailed);
            break;
        case AssistantEvent::PowerShort:
            if (session.state == AssistantState::Answer || session.state == AssistantState::Error) {
                session = AssistantSession{};
            }
            break;
        case AssistantEvent::PowerLong:
            if (session.state == AssistantState::Recording || session.state == AssistantState::Uploading || session.state == AssistantState::Thinking) {
                session.error = AssistantError::Busy;
            } else {
                session.state = AssistantState::ShuttingDown;
            }
            break;
    }
}

void ApplyAssistantError(AssistantSession &session, AssistantError error) {
    session.state = AssistantState::Error;
    session.error = error;
}

void ApplyAssistantAnswer(AssistantSession &session, const std::string &answer, const std::string &request_id) {
    session.state = AssistantState::Answer;
    session.error = AssistantError::None;
    session.answer = answer;
    session.request_id = request_id;
}
```

- [ ] **Step 5: Run model test and verify it passes**

Run:

```bash
cd firmware/apps/ai_voice_assistant/components/app/test
c++ -std=c++17 test_assistant_model.cpp ../assistant_model.cpp -I.. -o /tmp/test_assistant_model
/tmp/test_assistant_model
```

Expected: exit code `0`.

- [ ] **Step 6: Commit**

```bash
git add firmware/apps/ai_voice_assistant/components/app/assistant_types.h \
        firmware/apps/ai_voice_assistant/components/app/assistant_model.h \
        firmware/apps/ai_voice_assistant/components/app/assistant_model.cpp \
        firmware/apps/ai_voice_assistant/components/app/test/test_assistant_model.cpp
git commit -m "feat: add assistant state model"
```

## Task 3: Firmware App Skeleton

**Files:**
- Create app structure under `firmware/apps/ai_voice_assistant`
- Copy/adapt: BSP and externlib from `factory_program_custom`
- Create: `firmware/apps/ai_voice_assistant/components/app/CMakeLists.txt`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_config.h`

- [ ] **Step 1: Copy the known-good project shell**

Run:

```bash
mkdir -p firmware/apps/ai_voice_assistant
cp firmware/apps/factory_program_custom/CMakeLists.txt firmware/apps/ai_voice_assistant/CMakeLists.txt
cp -R firmware/apps/factory_program_custom/main firmware/apps/ai_voice_assistant/main
cp firmware/apps/factory_program_custom/sdkconfig.defaults firmware/apps/ai_voice_assistant/sdkconfig.defaults
cp firmware/apps/factory_program_custom/partitions.csv firmware/apps/ai_voice_assistant/partitions.csv
mkdir -p firmware/apps/ai_voice_assistant/components
cp -R firmware/apps/factory_program_custom/components/port_bsp firmware/apps/ai_voice_assistant/components/port_bsp
cp -R firmware/apps/factory_program_custom/components/externlib firmware/apps/ai_voice_assistant/components/externlib
```

- [ ] **Step 2: Rename the project**

Edit `firmware/apps/ai_voice_assistant/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS components/externlib)
project(ai_voice_assistant)
```

- [ ] **Step 3: Add public-safe config defaults**

Create `firmware/apps/ai_voice_assistant/components/app/assistant_config.h`:

```cpp
#pragma once

#include <cstdint>

namespace assistant_config {

inline constexpr const char *kWifiSsid = "";
inline constexpr const char *kWifiPassword = "";
inline constexpr const char *kProxyHost = "";
inline constexpr uint16_t kProxyPort = 8787;
inline constexpr uint32_t kWifiConnectTimeoutMs = 12000;
inline constexpr uint32_t kUploadTimeoutMs = 30000;
inline constexpr const char *kAskPath = "/ask";
inline constexpr const char *kTempWavPath = "/flash/assistant_last.wav";

inline bool WifiConfigured() {
    return kWifiSsid != nullptr && kWifiSsid[0] != '\0';
}

inline bool ProxyConfigured() {
    return kProxyHost != nullptr && kProxyHost[0] != '\0';
}

}  // namespace assistant_config
```

- [ ] **Step 4: Add app CMake using voice memo and desktop dependencies**

Create `firmware/apps/ai_voice_assistant/components/app/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "user_app.cpp"
        "assistant_model.cpp"
        "assistant_client.cpp"
        "assistant_ui.cpp"
        "memo_utils.cpp"
        "memo_storage.cpp"
        "memo_recorder.cpp"
        "audio_loop_policy.cpp"
        "power_sleep_policy.cpp"
    PRIV_REQUIRES
        port_bsp
        ui
        driver
        esp_hw_support
        esp_timer
        esp_wifi
        esp_http_client
        esp_netif
        esp_event
        json
        my_button
        codec_board
        nvs_flash
        waveshare__pcf85063a
    REQUIRES
    INCLUDE_DIRS ".")
```

- [ ] **Step 5: Copy proven audio/storage helper files**

Run:

```bash
cp firmware/apps/factory_program_custom/components/app/memo_types.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_utils.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_utils.cpp firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_storage.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_storage.cpp firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_recorder.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/memo_recorder.cpp firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/audio_loop_policy.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/audio_loop_policy.cpp firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/power_sleep_policy.h firmware/apps/ai_voice_assistant/components/app/
cp firmware/apps/factory_program_custom/components/app/power_sleep_policy.cpp firmware/apps/ai_voice_assistant/components/app/
```

- [ ] **Step 6: Build the blank skeleton**

Run:

```bash
cd firmware/apps/ai_voice_assistant
idf.py build
```

Expected: build reaches compile and fails only if `assistant_client.cpp`, `assistant_ui.cpp`, or `user_app.cpp` are missing. If it fails earlier in BSP dependency resolution, compare `factory_program_custom` CMake and keep the copied dependency shape identical.

- [ ] **Step 7: Commit**

```bash
git add firmware/apps/ai_voice_assistant
git commit -m "feat: scaffold AI voice assistant app"
```

## Task 4: Assistant UI Screens

**Files:**
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_ui.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_ui.cpp`

- [ ] **Step 1: Add UI interface**

Create `assistant_ui.h`:

```cpp
#pragma once

#include "assistant_types.h"

#include <cstdint>
#include <string>

class AssistantUi {
public:
    void Init();
    void ShowHome(bool wifi_configured, bool proxy_configured);
    void ShowRecording(uint32_t elapsed_seconds);
    void ShowBusy(AssistantState state);
    void ShowAnswer(const std::string &answer, const std::string &request_id);
    void ShowError(AssistantError error);
    void ShowPowerOff();

private:
    void Clear();
};
```

- [ ] **Step 2: Implement UI with fixed 200 x 200 bounds**

Create `assistant_ui.cpp`:

```cpp
#include "assistant_ui.h"

#include "assistant_model.h"

#include <lvgl.h>

namespace {

lv_obj_t *root = nullptr;

lv_obj_t *Label(lv_obj_t *parent, const char *text, int x, int y, int w, const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, w);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    return label;
}

}  // namespace

void AssistantUi::Init() {
    root = lv_obj_create(nullptr);
    lv_obj_set_size(root, 200, 200);
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_scr_load(root);
}

void AssistantUi::Clear() {
    if (root == nullptr) {
        Init();
    }
    lv_obj_clean(root);
}

void AssistantUi::ShowHome(bool wifi_configured, bool proxy_configured) {
    Clear();
    Label(root, "AI Assistant", 12, 18, 176, &lv_font_montserrat_20);
    Label(root, "Hold BOOT\nto ask", 12, 60, 176, &lv_font_montserrat_16);
    const char *status = (!wifi_configured) ? "Wi-Fi not set" : (!proxy_configured ? "Proxy not set" : "Ready");
    Label(root, status, 12, 152, 176, &lv_font_montserrat_14);
}

void AssistantUi::ShowRecording(uint32_t elapsed_seconds) {
    Clear();
    Label(root, "Recording", 12, 22, 176, &lv_font_montserrat_20);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", static_cast<unsigned long>(elapsed_seconds / 60), static_cast<unsigned long>(elapsed_seconds % 60));
    Label(root, buffer, 42, 78, 130, &lv_font_montserrat_28);
    Label(root, "Release BOOT", 12, 152, 176, &lv_font_montserrat_14);
}

void AssistantUi::ShowBusy(AssistantState state) {
    Clear();
    Label(root, AssistantStatusLabel(state).c_str(), 12, 54, 176, &lv_font_montserrat_20);
    Label(root, "Please wait", 12, 104, 176, &lv_font_montserrat_16);
}

void AssistantUi::ShowAnswer(const std::string &answer, const std::string &request_id) {
    Clear();
    Label(root, "Answer", 12, 10, 176, &lv_font_montserrat_18);
    auto lines = WrapAssistantText(answer, 20, 5);
    int y = 42;
    for (const auto &line : lines) {
        Label(root, line.c_str(), 12, y, 176, &lv_font_montserrat_14);
        y += 24;
    }
    std::string footer = request_id.empty() ? "PWR back" : ("PWR back #" + request_id.substr(0, 6));
    Label(root, footer.c_str(), 12, 170, 176, &lv_font_montserrat_12);
}

void AssistantUi::ShowError(AssistantError error) {
    Clear();
    Label(root, "Error", 12, 26, 176, &lv_font_montserrat_20);
    Label(root, AssistantErrorLabel(error).c_str(), 12, 82, 176, &lv_font_montserrat_16);
    Label(root, "PWR back", 12, 154, 176, &lv_font_montserrat_12);
}

void AssistantUi::ShowPowerOff() {
    Clear();
    Label(root, "Power off", 12, 72, 176, &lv_font_montserrat_20);
}
```

- [ ] **Step 3: Commit**

```bash
git add firmware/apps/ai_voice_assistant/components/app/assistant_ui.h \
        firmware/apps/ai_voice_assistant/components/app/assistant_ui.cpp
git commit -m "feat: add assistant e-paper UI"
```

## Task 5: Assistant HTTP Client

**Files:**
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_client.h`
- Create: `firmware/apps/ai_voice_assistant/components/app/assistant_client.cpp`

- [ ] **Step 1: Add client interface**

Create `assistant_client.h`:

```cpp
#pragma once

#include "assistant_types.h"

#include <esp_err.h>
#include <string>

struct AssistantResponse {
    bool ok = false;
    std::string text;
    std::string request_id;
    AssistantError error = AssistantError::None;
};

bool AssistantWifiConfigured();
bool AssistantProxyConfigured();
esp_err_t AssistantUploadWav(const char *path, AssistantResponse *response);
bool ParseAssistantResponseJson(const std::string &json, AssistantResponse *response);
```

- [ ] **Step 2: Implement JSON parsing first**

In `assistant_client.cpp`, start with parse logic:

```cpp
#include "assistant_client.h"

#include "assistant_config.h"

#include <cJSON.h>

bool AssistantWifiConfigured() {
    return assistant_config::WifiConfigured();
}

bool AssistantProxyConfigured() {
    return assistant_config::ProxyConfigured();
}

bool ParseAssistantResponseJson(const std::string &json, AssistantResponse *response) {
    if (response == nullptr) {
        return false;
    }
    cJSON *root = cJSON_Parse(json.c_str());
    if (root == nullptr) {
        response->ok = false;
        response->error = AssistantError::AiFailed;
        return false;
    }
    cJSON *ok = cJSON_GetObjectItem(root, "ok");
    cJSON *text = cJSON_GetObjectItem(root, "text");
    cJSON *request_id = cJSON_GetObjectItem(root, "request_id");
    cJSON *error = cJSON_GetObjectItem(root, "error");
    response->ok = cJSON_IsTrue(ok);
    if (cJSON_IsString(text)) {
        response->text = text->valuestring;
    }
    if (cJSON_IsString(request_id)) {
        response->request_id = request_id->valuestring;
    }
    if (!response->ok) {
        response->error = cJSON_IsString(error) ? AssistantError::AiFailed : AssistantError::UploadFailed;
    }
    cJSON_Delete(root);
    return response->ok && !response->text.empty();
}
```

- [ ] **Step 3: Implement Wi-Fi and upload**

Finish `assistant_client.cpp` by adapting the `desktop_helper` Wi-Fi lifecycle and using `esp_http_client`:

```cpp
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr const char *TAG = "assistant_client";
constexpr EventBits_t kWifiConnected = BIT0;
constexpr EventBits_t kWifiFailed = BIT1;

EventGroupHandle_t wifi_events = nullptr;
bool wifi_initialized = false;
int retry_count = 0;

struct HttpBuffer {
    std::string body;
};

void WifiEventHandler(void *, esp_event_base_t event_base, int32_t event_id, void *) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < 2) {
            retry_count++;
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(wifi_events, kWifiFailed);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_count = 0;
        xEventGroupSetBits(wifi_events, kWifiConnected);
    }
}

esp_err_t HttpEventHandler(esp_http_client_event_t *evt) {
    auto *buffer = static_cast<HttpBuffer *>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_DATA && buffer != nullptr && evt->data != nullptr && evt->data_len > 0) {
        buffer->body.append(static_cast<const char *>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

bool EnsureWifiStack() {
    if (wifi_initialized) {
        return true;
    }
    wifi_events = xEventGroupCreate();
    if (wifi_events == nullptr) {
        return false;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t loop_ret = esp_event_loop_create_default();
    if (loop_ret != ESP_OK && loop_ret != ESP_ERR_INVALID_STATE) {
        return false;
    }
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, nullptr, nullptr));
    wifi_initialized = true;
    return true;
}

bool ConnectWifi() {
    if (!AssistantWifiConfigured() || !EnsureWifiStack()) {
        return false;
    }
    xEventGroupClearBits(wifi_events, kWifiConnected | kWifiFailed);
    retry_count = 0;
    wifi_config_t wifi_config = {};
    strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), assistant_config::kWifiSsid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy(reinterpret_cast<char *>(wifi_config.sta.password), assistant_config::kWifiPassword, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_start();
    esp_wifi_connect();
    EventBits_t bits = xEventGroupWaitBits(wifi_events, kWifiConnected | kWifiFailed, pdFALSE, pdFALSE, pdMS_TO_TICKS(assistant_config::kWifiConnectTimeoutMs));
    return (bits & kWifiConnected) != 0;
}

size_t FileSize(FILE *file) {
    long old_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, old_pos, SEEK_SET);
    return size < 0 ? 0 : static_cast<size_t>(size);
}

}  // namespace

esp_err_t AssistantUploadWav(const char *path, AssistantResponse *response) {
    if (response == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!AssistantProxyConfigured()) {
        response->error = AssistantError::ProxyUnavailable;
        return ESP_FAIL;
    }
    if (!ConnectWifi()) {
        response->error = AssistantError::WifiUnavailable;
        return ESP_FAIL;
    }
    FILE *file = fopen(path, "rb");
    if (file == nullptr) {
        response->error = AssistantError::StorageFailed;
        return ESP_FAIL;
    }
    size_t size = FileSize(file);
    std::string wav;
    wav.resize(size);
    size_t read = fread(wav.data(), 1, size, file);
    fclose(file);
    if (read != size || size == 0) {
        response->error = AssistantError::StorageFailed;
        return ESP_FAIL;
    }

    char url[192];
    snprintf(url, sizeof(url), "http://%s:%u%s", assistant_config::kProxyHost, assistant_config::kProxyPort, assistant_config::kAskPath);
    HttpBuffer buffer;
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = assistant_config::kUploadTimeoutMs;
    config.event_handler = HttpEventHandler;
    config.user_data = &buffer;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        response->error = AssistantError::ProxyUnavailable;
        return ESP_FAIL;
    }
    esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_post_field(client, wav.data(), wav.size());
    esp_err_t ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    esp_wifi_disconnect();
    esp_wifi_stop();
    if (ret != ESP_OK || status < 200 || status >= 300) {
        response->error = status == 0 ? AssistantError::ProxyUnavailable : AssistantError::UploadFailed;
        ESP_LOGW(TAG, "upload failed ret=%s status=%d", esp_err_to_name(ret), status);
        return ESP_FAIL;
    }
    return ParseAssistantResponseJson(buffer.body, response) ? ESP_OK : ESP_FAIL;
}
```

- [ ] **Step 4: Build to catch ESP-IDF integration errors**

Run:

```bash
cd firmware/apps/ai_voice_assistant
idf.py build
```

Expected: if `user_app.cpp` is still a stub, the build may fail there, but `assistant_client.cpp` should compile. Fix missing includes or component dependencies before continuing.

- [ ] **Step 5: Commit**

```bash
git add firmware/apps/ai_voice_assistant/components/app/assistant_client.h \
        firmware/apps/ai_voice_assistant/components/app/assistant_client.cpp
git commit -m "feat: add assistant proxy upload client"
```

## Task 6: App Orchestration And Recording Loop

**Files:**
- Create/modify: `firmware/apps/ai_voice_assistant/components/app/user_app.h`
- Create/modify: `firmware/apps/ai_voice_assistant/components/app/user_app.cpp`
- Modify if needed: `firmware/apps/ai_voice_assistant/main/main.cpp`

- [ ] **Step 1: Add the app entry interface**

Create `user_app.h`:

```cpp
#pragma once

#include <stdint.h>

void UserApp_Init();
void UserApp_Start();
void UserApp_Tick();
```

- [ ] **Step 2: Keep main startup identical to proven apps**

Verify `main/main.cpp` calls the app layer after LVGL initialization:

```cpp
extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    UserApp_Init();
    PortLvgl_Init(LvglFlushCb);
    UserUi_Init();
    UserApp_Start();
    while (true) {
        UserApp_Tick();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

- [ ] **Step 3: Implement button events, recording, upload worker, and safe power-off**

Implement `user_app.cpp` by starting from the voice memo `user_app.cpp`, then reduce it to one flow:

```cpp
#include "user_app.h"

#include "assistant_client.h"
#include "assistant_config.h"
#include "assistant_model.h"
#include "assistant_ui.h"
#include "button.h"
#include "memo_recorder.h"
#include "memo_storage.h"
#include "power_sleep_policy.h"
#include "port_codec.h"
#include "port_flash_storage.h"
#include "port_i2c.h"
#include "port_lvgl.h"
#include "port_power.h"
#include "port_sdcard.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

namespace {

constexpr const char *TAG = "ai_voice";
constexpr EventBits_t BIT_BOOT_DOWN = 0x01;
constexpr EventBits_t BIT_BOOT_UP = 0x02;
constexpr EventBits_t BIT_PWR_SHORT = 0x04;
constexpr EventBits_t BIT_PWR_LONG = 0x08;
constexpr EventBits_t BIT_UPLOAD_DONE = 0x10;
constexpr EventBits_t BIT_UPLOAD_ERROR = 0x20;

EventGroupHandle_t app_events = nullptr;
TaskHandle_t upload_task = nullptr;
Button *boot_button = nullptr;
Button *power_button = nullptr;

AssistantUi ui;
AssistantSession session;
MemoStorage storage;
MemoRecorder recorder;
AssistantResponse upload_response;
WavFormat wav_format{16000, 16, 1};
bool flash_ready = false;
bool sd_ready = false;
bool power_button_released = false;

void BootCallback(void *, void *event) {
    auto id = reinterpret_cast<intptr_t>(event);
    if (id == BUTTON_PRESS_DOWN) {
        xEventGroupSetBits(app_events, BIT_BOOT_DOWN);
    } else if (id == BUTTON_PRESS_UP) {
        xEventGroupSetBits(app_events, BIT_BOOT_UP);
    }
}

void PowerCallback(void *, void *event) {
    auto id = reinterpret_cast<intptr_t>(event);
    if (id == BUTTON_SINGLE_CLICK) {
        xEventGroupSetBits(app_events, BIT_PWR_SHORT);
    } else if (id == BUTTON_LONG_PRESS_HOLD) {
        xEventGroupSetBits(app_events, BIT_PWR_LONG);
    } else if (id == BUTTON_PRESS_UP) {
        power_button_released = true;
    }
}

std::string TempWavPath() {
    return storage.BaseDir() + "/assistant_last.wav";
}

bool StorageReady() {
    return sd_ready || flash_ready;
}

void UploadTask(void *) {
    upload_response = AssistantResponse{};
    esp_err_t ret = AssistantUploadWav(TempWavPath().c_str(), &upload_response);
    xEventGroupSetBits(app_events, ret == ESP_OK ? BIT_UPLOAD_DONE : BIT_UPLOAD_ERROR);
    upload_task = nullptr;
    vTaskDelete(nullptr);
}

void StartRecording() {
    if (!StorageReady()) {
        ApplyAssistantError(session, AssistantError::StorageFailed);
        ui.ShowError(session.error);
        return;
    }
    ApplyAssistantEvent(session, AssistantEvent::BootDown);
    ui.ShowRecording(0);
    esp_err_t ret = recorder.Start(TempWavPath().c_str(), wav_format);
    if (ret != ESP_OK) {
        ApplyAssistantError(session, AssistantError::RecordFailed);
        ui.ShowError(session.error);
    }
}

void StopRecordingAndUpload() {
    if (!recorder.IsRecording()) {
        return;
    }
    MemoMetadata metadata;
    esp_err_t ret = recorder.Stop(&metadata);
    if (ret != ESP_OK) {
        ApplyAssistantError(session, AssistantError::RecordFailed);
        ui.ShowError(session.error);
        return;
    }
    ApplyAssistantEvent(session, AssistantEvent::BootUp);
    ui.ShowBusy(AssistantState::Uploading);
    xTaskCreate(UploadTask, "assistant_upload", 8192, nullptr, 4, &upload_task);
}

void PowerOffIfSafe() {
    ApplyAssistantEvent(session, AssistantEvent::PowerLong);
    if (session.state != AssistantState::ShuttingDown) {
        ui.ShowError(AssistantError::Busy);
        return;
    }
    ui.ShowPowerOff();
    vTaskDelay(pdMS_TO_TICKS(800));
    BoardPower_Audio_OFF();
    BoardPower_EPD_OFF();
    esp_deep_sleep_start();
}

}  // namespace

void UserApp_Init() {
    app_events = xEventGroupCreate();
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_Audio_ON();
    PortI2c_Init();
    PortCodec_Init();
    sd_ready = PortSdcard_Init() == ESP_OK;
    flash_ready = PortFlashStorage_Init() == ESP_OK;
    storage.Init(sd_ready ? MemoStorageRoot::SdCard : MemoStorageRoot::Flash);
    boot_button = new Button(BOOT_BUTTON_PIN, 0);
    power_button = new Button(PWR_BUTTON_PIN, 0);
    boot_button->AttachPressDownEventCb(&BootCallback, reinterpret_cast<void *>(BUTTON_PRESS_DOWN));
    boot_button->AttachPressUpEventCb(&BootCallback, reinterpret_cast<void *>(BUTTON_PRESS_UP));
    power_button->AttachSingleClickEventCb(&PowerCallback, reinterpret_cast<void *>(BUTTON_SINGLE_CLICK));
    power_button->AttachLongPressHoldEventCb(&PowerCallback, reinterpret_cast<void *>(BUTTON_LONG_PRESS_HOLD));
    power_button->AttachPressUpEventCb(&PowerCallback, reinterpret_cast<void *>(BUTTON_PRESS_UP));
}

void UserApp_Start() {
    ui.Init();
    ui.ShowHome(AssistantWifiConfigured(), AssistantProxyConfigured());
}

void UserApp_Tick() {
    if (recorder.IsRecording()) {
        recorder.CaptureChunk();
        uint32_t elapsed = recorder.ElapsedSeconds();
        if (elapsed != session.elapsed_seconds) {
            session.elapsed_seconds = elapsed;
            ui.ShowRecording(elapsed);
        }
    }
    EventBits_t bits = xEventGroupWaitBits(app_events, BIT_BOOT_DOWN | BIT_BOOT_UP | BIT_PWR_SHORT | BIT_PWR_LONG | BIT_UPLOAD_DONE | BIT_UPLOAD_ERROR, pdTRUE, pdFALSE, 0);
    if (bits & BIT_BOOT_DOWN) {
        StartRecording();
    }
    if (bits & BIT_BOOT_UP) {
        StopRecordingAndUpload();
    }
    if (bits & BIT_UPLOAD_DONE) {
        ApplyAssistantAnswer(session, upload_response.text, upload_response.request_id);
        ui.ShowAnswer(session.answer, session.request_id);
    }
    if (bits & BIT_UPLOAD_ERROR) {
        ApplyAssistantError(session, upload_response.error);
        ui.ShowError(session.error);
    }
    if (bits & BIT_PWR_SHORT) {
        ApplyAssistantEvent(session, AssistantEvent::PowerShort);
        ui.ShowHome(AssistantWifiConfigured(), AssistantProxyConfigured());
    }
    if (bits & BIT_PWR_LONG) {
        PowerOffIfSafe();
    }
}
```

- [ ] **Step 4: Build and fix compile errors by copying exact proven APIs**

Run:

```bash
cd firmware/apps/ai_voice_assistant
idf.py build
```

Expected: build completes. If button callback names or storage init names differ, inspect the copied headers and adjust `user_app.cpp` to match the actual APIs instead of changing the BSP.

- [ ] **Step 5: Commit**

```bash
git add firmware/apps/ai_voice_assistant/components/app/user_app.h \
        firmware/apps/ai_voice_assistant/components/app/user_app.cpp \
        firmware/apps/ai_voice_assistant/main/main.cpp
git commit -m "feat: wire assistant recording and upload flow"
```

## Task 7: Documentation And Public Config Guard

**Files:**
- Create: `firmware/apps/ai_voice_assistant/README.md`
- Modify: `README.md`
- Check: `firmware/apps/ai_voice_assistant/components/app/assistant_config.h`

- [ ] **Step 1: Document app behavior**

Create `firmware/apps/ai_voice_assistant/README.md`:

```markdown
# AI Voice Assistant

Hold BOOT to record a question. Release BOOT to stop recording, upload the WAV to
the local Mac proxy, and display the returned answer on the 200 x 200 e-paper screen.

PWR short press returns to the home screen. PWR long press powers off when the app
is idle, on an answer screen, or on an error screen. During recording or upload,
shutdown is rejected so the WAV file is not corrupted.

## Configuration

Edit `components/app/assistant_config.h` locally:

- `kWifiSsid`
- `kWifiPassword`
- `kProxyHost`
- `kProxyPort`

The public repository keeps these values empty. Empty Wi-Fi credentials mean
offline mode.

## Proxy

Start the local proxy first:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

Use OpenAI mode only with `OPENAI_API_KEY` set in the shell. The firmware never
contains API keys.

## Build

```bash
cd firmware/apps/ai_voice_assistant
idf.py build
```

Before flashing, verify the target board MAC is the intended ESP32-S3 e-paper board.
```

- [ ] **Step 2: Add root README app index entry**

Add this section to `README.md`:

```markdown
### AI Voice Assistant

Path: `firmware/apps/ai_voice_assistant`

Records while BOOT is held, uploads the WAV to a local Mac proxy, and displays a
short AI answer. The first workflow uses proxy mock mode; OpenAI mode is handled
only by the Mac proxy so no API keys are stored in firmware.
```

- [ ] **Step 3: Scan for secrets before commit**

Run:

```bash
rg -n "szyt1008|1-306|OPENAI_API_KEY=.*[A-Za-z0-9]|kWifiSsid = \"[^\"]+\"|kWifiPassword = \"[^\"]+\"|kProxyHost = \"(192|10|172)\\." firmware/apps/ai_voice_assistant tools/ai_proxy README.md
```

Expected: no matches except documentation text that names `OPENAI_API_KEY` without a value.

- [ ] **Step 4: Commit**

```bash
git add firmware/apps/ai_voice_assistant/README.md README.md
git commit -m "docs: document AI voice assistant workflow"
```

## Task 8: OpenAI Proxy Mode

**Files:**
- Modify: `tools/ai_proxy/server.py`
- Modify: `tools/ai_proxy/test_proxy_contract.py`
- Modify: `tools/ai_proxy/README.md`

- [ ] **Step 1: Add a unit-level OpenAI request builder**

Add these functions to `server.py` so the network calls are easy to test:

```python
def openai_headers():
    key = os.environ.get("OPENAI_API_KEY", "")
    if not key:
        raise RuntimeError("OPENAI_API_KEY is required")
    return {"Authorization": f"Bearer {key}"}


def build_answer_prompt(transcript):
    return (
        "You are a concise assistant for a 200x200 e-paper device. "
        "Answer in the same language as the user. Keep the answer under 80 Chinese characters "
        "or under 120 English characters.\n\nUser said:\n" + transcript.strip()
    )
```

- [ ] **Step 2: Add tests without making live OpenAI calls**

Add to `test_proxy_contract.py`:

```python
class OpenAiBuilderTests(unittest.TestCase):
    def test_openai_headers_use_environment_key(self):
        old_key = os.environ.get("OPENAI_API_KEY")
        os.environ["OPENAI_API_KEY"] = "test-key"
        try:
            self.assertEqual(server.openai_headers()["Authorization"], "Bearer test-key")
        finally:
            if old_key is None:
                os.environ.pop("OPENAI_API_KEY", None)
            else:
                os.environ["OPENAI_API_KEY"] = old_key

    def test_answer_prompt_is_short_screen_prompt(self):
        prompt = server.build_answer_prompt("上海今天冷吗")
        self.assertIn("200x200", prompt)
        self.assertIn("上海今天冷吗", prompt)
        self.assertLess(len(prompt), 260)
```

- [ ] **Step 3: Implement OpenAI mode with HTTP requests**

Implement `ask_openai(audio, request_id)` using Python standard library:

```python
import base64
import mimetypes
import urllib.error
import urllib.request


def multipart_body(fields, files):
    boundary = "----ai-voice-" + uuid.uuid4().hex
    chunks = []
    for name, value in fields.items():
        chunks.append(f"--{boundary}\r\n".encode())
        chunks.append(f'Content-Disposition: form-data; name="{name}"\r\n\r\n'.encode())
        chunks.append(str(value).encode())
        chunks.append(b"\r\n")
    for name, filename, content_type, data in files:
        chunks.append(f"--{boundary}\r\n".encode())
        chunks.append(f'Content-Disposition: form-data; name="{name}"; filename="{filename}"\r\n'.encode())
        chunks.append(f"Content-Type: {content_type}\r\n\r\n".encode())
        chunks.append(data)
        chunks.append(b"\r\n")
    chunks.append(f"--{boundary}--\r\n".encode())
    return boundary, b"".join(chunks)


def post_json(url, body):
    data = json.dumps(body).encode("utf-8")
    headers = {"Content-Type": "application/json", **openai_headers()}
    req = urllib.request.Request(url, data=data, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=45) as resp:
        return json.loads(resp.read().decode("utf-8"))


def transcribe_wav(audio):
    boundary, body = multipart_body(
        {"model": os.environ.get("OPENAI_TRANSCRIBE_MODEL", "gpt-4o-mini-transcribe")},
        [("file", "assistant.wav", "audio/wav", audio)],
    )
    headers = {"Content-Type": f"multipart/form-data; boundary={boundary}", **openai_headers()}
    req = urllib.request.Request("https://api.openai.com/v1/audio/transcriptions", data=body, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=60) as resp:
        result = json.loads(resp.read().decode("utf-8"))
    text = result.get("text", "").strip()
    if not text:
        raise RuntimeError("empty transcription")
    return text


def answer_text(transcript):
    body = {
        "model": os.environ.get("OPENAI_TEXT_MODEL", "gpt-5-mini"),
        "input": build_answer_prompt(transcript),
    }
    result = post_json("https://api.openai.com/v1/responses", body)
    text = result.get("output_text", "").strip()
    if not text:
        raise RuntimeError("empty answer")
    return text


def ask_openai(audio, request_id):
    try:
        transcript = transcribe_wav(audio)
        return answer_text(transcript)
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")[:300]
        raise RuntimeError(f"openai_http_{exc.code}:{detail}") from exc
```

- [ ] **Step 4: Re-check against official docs before live use**

Before running live OpenAI mode, re-open official docs for:

```text
https://platform.openai.com/docs/guides/speech-to-text
https://platform.openai.com/docs/api-reference/audio/createTranscription
https://platform.openai.com/docs/api-reference/responses/create
```

Confirm the current model names and response fields still match this implementation. If the docs changed, update only `transcribe_wav` and `answer_text`; do not change the firmware contract.

- [ ] **Step 5: Run tests**

Run:

```bash
python3 -m unittest tools/ai_proxy/test_proxy_contract.py
```

Expected: `OK`.

- [ ] **Step 6: Commit**

```bash
git add tools/ai_proxy
git commit -m "feat: add OpenAI mode to AI voice proxy"
```

## Task 9: Hardware Verification

**Files:**
- Modify only if needed: firmware files touched by earlier tasks.
- Do not modify: `firmware/apps/factory_program_custom` unless a shared copied bug is found and user approves a separate fix.

- [ ] **Step 1: Start mock proxy on the Mac**

Run:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

Expected: console prints `AI voice proxy listening`.

- [ ] **Step 2: Set local firmware config without committing secrets**

Temporarily edit `firmware/apps/ai_voice_assistant/components/app/assistant_config.h` locally:

```cpp
inline constexpr const char *kWifiSsid = "YOUR_WIFI";
inline constexpr const char *kWifiPassword = "YOUR_PASSWORD";
inline constexpr const char *kProxyHost = "YOUR_MAC_LAN_IP";
```

Before final commit or push, restore these strings to empty values.

- [ ] **Step 3: Verify target board before flashing**

Run the ESP-IDF port check or `esptool.py chip_id` against the connected port. The intended e-paper board has previously shown MAC `70:04:1d:db:f9:6c`. Do not flash the other connected ESP32 board.

- [ ] **Step 4: Build, flash, and monitor**

Run:

```bash
cd firmware/apps/ai_voice_assistant
idf.py -p /dev/cu.usbmodem11101 build flash monitor
```

Expected:

- Display shows home screen.
- Holding BOOT shows recording timer.
- Releasing BOOT shows uploading.
- Proxy logs one `/ask` request with byte count.
- Display shows mock answer.
- PWR short returns home from answer or error.
- PWR long powers off from idle/answer/error.

- [ ] **Step 5: Test failure states**

Run these checks:

```text
1. Empty Wi-Fi config: app shows Wi-Fi not set and does not crash.
2. Valid Wi-Fi but stopped proxy: app shows proxy/upload error.
3. Proxy mock mode running: app shows answer.
4. Long PWR while recording: app rejects shutdown or waits for safe stop; WAV is not corrupted.
```

- [ ] **Step 6: Restore public-safe config and scan**

Run:

```bash
git diff -- firmware/apps/ai_voice_assistant/components/app/assistant_config.h
rg -n "YOUR_WIFI|YOUR_PASSWORD|szyt1008|1-306|192\\.168|10\\.|172\\." firmware/apps/ai_voice_assistant tools/ai_proxy
```

Expected: config diff is empty or only contains empty public defaults; secret scan has no real credential/IP matches.

- [ ] **Step 7: Commit hardware fixes**

If hardware verification required firmware fixes:

```bash
git add firmware/apps/ai_voice_assistant tools/ai_proxy README.md
git commit -m "fix: stabilize AI voice assistant hardware flow"
```

## Final Verification

Run all verification before reporting completion:

```bash
python3 -m unittest tools/ai_proxy/test_proxy_contract.py
cd firmware/apps/ai_voice_assistant
idf.py build
cd ../../..
rg -n "szyt1008|1-306|OPENAI_API_KEY=.*[A-Za-z0-9]|kWifiSsid = \"[^\"]+\"|kWifiPassword = \"[^\"]+\"|kProxyHost = \"(192|10|172)\\." firmware/apps/ai_voice_assistant tools/ai_proxy README.md
git status --short
```

Expected:

- Proxy tests pass.
- ESP-IDF build passes.
- Secret scan has no real private values.
- Git status only contains intentional committed changes or local ignored build artifacts.

## Implementation Notes

- Keep the firmware contract stable: `POST /ask` with raw WAV bytes, response JSON `{ok,text,request_id}` or `{ok:false,error,request_id}`.
- Keep the first hardware milestone on mock proxy mode. OpenAI mode should not be debugged until recording and upload are stable.
- Avoid automatic sleep in V1. The board should only power off from long PWR.
- Do not add streaming audio yet. Recording-then-upload is simpler and matches the proven voice memo storage path.
- Do not commit local Wi-Fi, local proxy IP, API keys, or captured WAV files.
