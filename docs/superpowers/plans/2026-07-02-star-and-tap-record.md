# Star and Tap Record Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add starred memos and a tap-to-record mode without breaking the existing hold-to-record workflow.

**Architecture:** Store star state in a sidecar metadata file under the memo directory so WAV files are never modified. Extend the settings model with `REC MODE HOLD/TAP`, and route BOOT press events through a small mode-aware branch in the app state machine.

**Tech Stack:** ESP-IDF C++, LVGL, host-side C++ assertions for pure logic, existing FAT storage.

---

### Task 1: Star Metadata and Settings Policy

**Files:**
- Create: `firmware/apps/factory_program_custom/components/app/memo_favorites.h`
- Create: `firmware/apps/factory_program_custom/components/app/memo_favorites.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/memo_types.h`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_settings.h`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_settings.cpp`
- Test: `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`

- [x] **Step 1: Write failing tests for star toggling and record mode**
- [x] **Step 2: Run host tests and verify RED**
- [x] **Step 3: Implement pure logic and metadata parsing**
- [x] **Step 4: Run host tests and verify GREEN**

### Task 2: App UI Integration

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/user_app.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.h`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/touch_gesture.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/CMakeLists.txt`

- [x] **Step 1: Add STAR button and list marker**
- [x] **Step 2: Make CLEAR ALL skip starred memos**
- [x] **Step 3: Add REC MODE setting and tap-record branch**
- [x] **Step 4: Build, flash, and monitor startup**
