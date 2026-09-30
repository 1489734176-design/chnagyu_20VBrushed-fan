# Repository Guidelines

## Project Structure & Module Organization

This repository contains bare-metal C firmware for a MindMotion MM32G0001A1TC brushed fan controller. Application code is in the repository root (`main.c`, `platform.c`, and interrupt headers) and `USER/` (`user.c` and `user.h`). `Device/` contains CMSIS, MM32G0001 headers, startup code, and the vendor HAL. `MDK-ARM/` is the active Keil project; `EWARM/` is a legacy, unsynchronized IAR project. Build outputs are generated under `MDK-ARM/Objects/` and `MDK-ARM/Listings/`.

## Build, Test, and Development Commands

Build with Keil uVision 5. The active target is `TIM3_TimeBase` in `MDK-ARM/TIM3_TimeBase.uvprojx`:

```bash
UV4="D:/Users/mym02/AppData/Local/Keil_v5/UV4/UV4.exe"
ROOT="$(pwd -W)"
"$UV4" -b "$ROOT/MDK-ARM/TIM3_TimeBase.uvprojx" -t "TIM3_TimeBase" -o "$ROOT/MDK-ARM/build.log"
"$UV4" -r "$ROOT/MDK-ARM/TIM3_TimeBase.uvprojx" -t "TIM3_TimeBase" -o "$ROOT/MDK-ARM/rebuild.log"
```

The first command performs an incremental build; the second rebuilds all sources. There is no automated test suite, lint configuration, or Make/CMake build. Validate GPIO behavior, ADC calibration, display timing, sleep/wake, and power control on target hardware.

## Coding Style & Naming Conventions

Use C consistent with the surrounding firmware: four-space indentation, braces on the same line, descriptive `UPPER_SNAKE_CASE` macros, and `snake_case` functions and variables. Preserve UTF-8 comments and LF line endings. Keep interrupt-shared state and hardware mappings explicit, and update the Keil source list when adding `.c` files.

## Testing Guidelines

After changes, rebuild the Keil target and inspect the log for errors and warnings. Flash and exercise the hardware when changing timing, GPIO polarity, ADC thresholds, PWM, display scanning, charging, or low-power code. Do not treat the inherited `readme.txt` as authoritative for current pin assignments.

## Commit & Pull Request Guidelines

No Git history is available in this checkout, so no existing commit convention can be inferred. Use concise imperative messages, for example `Fix charge wake handling`. Pull requests should summarize behavior changes, list affected hardware or pins, include build results, and attach hardware-test notes or photos when the change affects visible or electrical behavior.

## Configuration & Safety Notes

The active toolchain uses ARM Compiler 5.06 and the MindMotion device pack. Do not enable the vendor demo console or LEDs without checking pin conflicts. Treat active code and project source lists as authoritative over historical comments and disabled `#if 0` implementations.
