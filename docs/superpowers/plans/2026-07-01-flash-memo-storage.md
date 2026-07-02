# Flash Memo Storage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allow voice memos to record and play back without an SD card by falling back to board flash storage.

**Architecture:** Keep the existing recorder/player file-path design. Add an internal FatFS mount at `/flash`, then choose `/sdcard/memos` when SD mounts and `/flash/memos` when SD is unavailable.

**Tech Stack:** ESP-IDF FatFS, wear levelling, ESP32-S3 8MB partition table, existing WAV recorder/player.

---

### Task 1: Add Internal Flash Filesystem

**Files:**
- Create: `firmware/apps/factory_program_custom/components/port_bsp/port_flash_storage.h`
- Create: `firmware/apps/factory_program_custom/components/port_bsp/port_flash_storage.cpp`
- Modify: `firmware/apps/factory_program_custom/components/port_bsp/CMakeLists.txt`

- [ ] Add a BSP function `FlashStorage_Init()` that mounts the `storage` partition at `/flash` using `esp_vfs_fat_spiflash_mount_rw_wl`.
- [ ] Return `false` and log the ESP-IDF error if mount fails.
- [ ] Keep the wear levelling handle private to the BSP file.

### Task 2: Add 8MB Partition Layout

**Files:**
- Modify: `firmware/apps/factory_program_custom/partitions.csv`
- Modify: `firmware/apps/factory_program_custom/sdkconfig.defaults`

- [ ] Set ESP-IDF flash size to 8MB in defaults.
- [ ] Keep the factory app at `0x10000` size `0x3F0000`.
- [ ] Add `storage,data,fat,0x400000,0x400000`.

### Task 3: Make Memo Storage Selectable

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/memo_storage.h`
- Modify: `firmware/apps/factory_program_custom/components/app/memo_storage.cpp`
- Modify: `firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp`

- [ ] Add `SetBaseDir(std::string base_dir)`.
- [ ] Remove direct `card == nullptr` checks from `MemoStorage`; mounted filesystem availability is determined by path operations.
- [ ] Add a small unit check that changing base dir updates generated memo filenames.

### Task 4: Choose SD or Flash at Boot

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/user_app.cpp`

- [ ] Add storage state for SD, Flash, or none.
- [ ] Initialize SD first.
- [ ] If SD fails, initialize Flash and set memo storage base dir to `/flash/memos`.
- [ ] Allow recording whenever either storage backend is ready.

### Task 5: Show Storage State in UI

**Files:**
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.h`
- Modify: `firmware/apps/factory_program_custom/components/app/voice_ui.cpp`

- [ ] Replace `sd_ready` parameter with a storage status string.
- [ ] Show `SD` or `FLASH` in the top status area.
- [ ] Only show the blocking no-storage screen when neither backend is mounted.

### Task 6: Verify on Device

**Commands:**
- `idf.py build`
- `idf.py -p /dev/cu.usbmodem11101 flash`
- `idf.py -p /dev/cu.usbmodem11101 monitor`

- [ ] Confirm build succeeds.
- [ ] Confirm the target board is `ESP32-S3-PICO-1`, MAC `70:04:1d:db:f9:6c`.
- [ ] Confirm startup logs show SD failure followed by Flash storage ready.
