# Delete Memo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a delete button on the memo detail screen with a confirmation step before removing the WAV file.

**Architecture:** Reuse the existing `VoiceUiAction` and `VoiceAppState` state machine. Add a pure touch gesture test for the new button hit areas, then add one UI screen and one app handler path that stops playback, deletes the selected file, rescans storage, and returns to the list.

**Tech Stack:** ESP-IDF C++, LVGL, host-side C++ assertions for pure logic.

---

### Task 1: Add Delete Action and Confirmation Flow

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/memo_types.h`
- Modify: `firmware/apps/factory_program_custom/components/app/touch_gesture.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.h`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/user_app.cpp`
- Test: `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`

- [x] **Step 1: Write failing gesture tests**

Add tests that expect detail taps to classify as `PlayStop`, `Delete`, and `Back`, and confirmation taps to classify as `ConfirmDelete` and `Cancel`.

- [x] **Step 2: Run host test and verify RED**

Run the existing host compile command. Expected: compile failure because `DeleteConfirm`, `Delete`, `ConfirmDelete`, or `Cancel` do not exist yet.

- [x] **Step 3: Implement minimal state/action/UI support**

Add the new enum values, classify the new hit areas, draw `PLAY/STOP`, `DEL`, `BACK` on detail, and draw `DELETE` / `CANCEL` on the confirmation screen.

- [x] **Step 4: Implement app deletion flow**

On `Delete`, stop playback and show confirmation. On `ConfirmDelete`, remove the selected memo file, reload memos, clamp the page, and return to list. On failure, show an error briefly and return to detail/list depending on whether the memo still exists.

- [x] **Step 5: Verify**

Run host tests, `idf.py build`, then flash to `/dev/cu.usbmodem11101` only if build passes.
