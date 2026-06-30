# Factory Program Custom Design

## Goal

Create a custom application derived from the official V2 factory program while preserving the official example unchanged.

The first step is intentionally conservative: copy the official factory program into a new application directory, rename it as a custom app, and verify it builds before changing behavior.

## Source Program

Official reference:

- `firmware/official/v2/esp-idf/11_FactoryProgram/`

This directory must remain unchanged. It is the baseline for comparing future behavior and recovering original vendor logic.

## New Application

Target directory:

- `firmware/apps/factory_program_custom/`

The new app starts as a full copy of the official factory program. After the copy builds successfully, future changes should be made only in the custom app unless a shared BSP fix is explicitly required.

## Existing Architecture To Preserve

The factory program has three useful layers:

- `main/main.cpp`: startup order and LVGL display flush bridge
- `components/app/user_app.cpp`: application orchestration, tasks, event handling, page switching
- `components/port_bsp/`: hardware adaptation for power, display, LVGL, I2C, touch, ADC, SHTC3, SD card, and audio

External generated or vendor-style code should stay isolated:

- `components/externlib/ui/`: generated LVGL factory-test UI
- `components/externlib/ui_res/`: images and fonts
- `components/externlib/my_button/`: button wrapper
- `components/externlib/codec_board/`: codec support

## First Implementation Slice

1. Copy `11_FactoryProgram` to `firmware/apps/factory_program_custom`.
2. Rename the ESP-IDF project from `11_Fac` to `factory_program_custom`.
3. Keep the initial behavior identical to the official program.
4. Build the copied app with ESP-IDF for `esp32s3`.
5. Confirm the official factory program directory has no changes.

## Future Change Rules

- Add or remove product behavior in `components/app/` first.
- Extend `components/port_bsp/` only for hardware-facing features or driver cleanup.
- Avoid putting business logic into `main/main.cpp`.
- Avoid editing generated UI/resource files unless the requested change is specifically visual.
- Keep official examples under `firmware/official/` read-only.

## Acceptance Criteria

- `firmware/apps/factory_program_custom/` exists and builds.
- `firmware/official/v2/esp-idf/11_FactoryProgram/` is unchanged.
- The custom app still follows the original startup flow:
  `UserApp_Init -> PortLvgl_Start_Init -> Lvgl_PortInit -> UserUi_Init -> UserApp_Start_Init`.
- Git history clearly separates the copy/setup step from later feature changes.
