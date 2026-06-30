# Factory Program Custom Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a custom ESP-IDF app copied from the official V2 factory program without modifying the official reference.

**Architecture:** The custom app starts as a full copy of `firmware/official/v2/esp-idf/11_FactoryProgram`. The copy preserves the original `main`, `components/app`, `components/port_bsp`, and `components/externlib` layering, with only the ESP-IDF project name changed so future feature work has a safe target.

**Tech Stack:** ESP-IDF v5.5, ESP32-S3 target, LVGL v9, Waveshare factory program components, local Git.

---

## File Structure

- Create: `firmware/apps/factory_program_custom/`
  - Full copy of `firmware/official/v2/esp-idf/11_FactoryProgram/`
- Modify: `firmware/apps/factory_program_custom/CMakeLists.txt`
  - Change the copied ESP-IDF project name from `11_Fac` to `factory_program_custom`
- Do not modify: `firmware/official/v2/esp-idf/11_FactoryProgram/**`

## Task 1: Copy The Official Factory Program

**Files:**
- Create: `firmware/apps/factory_program_custom/**`

- [ ] **Step 1: Verify the destination does not exist**

Run:

```bash
test ! -e firmware/apps/factory_program_custom
```

Expected: command exits with status `0`.

- [ ] **Step 2: Copy the official program**

Run:

```bash
cp -R firmware/official/v2/esp-idf/11_FactoryProgram firmware/apps/factory_program_custom
```

Expected: `firmware/apps/factory_program_custom/main/main.cpp` exists.

- [ ] **Step 3: Rename the copied ESP-IDF project**

Edit `firmware/apps/factory_program_custom/CMakeLists.txt` so it contains:

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS components/externlib)
project(factory_program_custom)
```

- [ ] **Step 4: Confirm the official reference is untouched**

Run:

```bash
git diff --name-only -- firmware/official/v2/esp-idf/11_FactoryProgram
```

Expected: no output.

- [ ] **Step 5: Commit the copy**

Run:

```bash
git add firmware/apps/factory_program_custom
git commit -m "Add custom factory program app"
```

Expected: commit succeeds and only files under `firmware/apps/factory_program_custom` are committed.

## Task 2: Build And Verify The Copied App

**Files:**
- Verify: `firmware/apps/factory_program_custom/`

- [ ] **Step 1: Build the custom app**

Run:

```bash
cd firmware/apps/factory_program_custom
. /Users/wq/esp-idf/export.sh
idf.py build
```

Expected: build completes and generates `build/factory_program_custom.bin`.

- [ ] **Step 2: Check generated repository noise**

Run from repository root:

```bash
git status --short
```

Expected: no tracked-file modifications. If `firmware/apps/factory_program_custom/sdkconfig` appears as an untracked file, remove it because `sdkconfig.defaults` is the source-controlled config seed.

- [ ] **Step 3: Reconfirm official reference is untouched**

Run:

```bash
git diff --name-only -- firmware/official/v2/esp-idf/11_FactoryProgram
```

Expected: no output.

- [ ] **Step 4: Commit build hygiene if needed**

If no source or ignore-file changes are needed, do not create a commit. If a repository hygiene file must be changed, commit only that file with:

```bash
git add <changed-file>
git commit -m "Keep custom factory build outputs ignored"
```

## Task 3: Record The Safe Starting Point

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add the custom app to the BSP section**

In `README.md`, under the existing BSP section, add:

```markdown
Custom factory-derived app:

- [firmware/apps/factory_program_custom](firmware/apps/factory_program_custom/)
```

- [ ] **Step 2: Verify documentation diff**

Run:

```bash
git diff -- README.md
```

Expected: diff only adds the custom factory-derived app link.

- [ ] **Step 3: Commit documentation**

Run:

```bash
git add README.md
git commit -m "Document custom factory program app"
```

- [ ] **Step 4: Final checks**

Run:

```bash
git status --short
git log --oneline -5
git diff --name-only -- firmware/official/v2/esp-idf/11_FactoryProgram
```

Expected:

- `git status --short` has no output.
- Latest commits include the custom app and README documentation.
- Official factory program diff has no output.
