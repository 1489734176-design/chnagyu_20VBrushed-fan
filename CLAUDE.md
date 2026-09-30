# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project and toolchain

This is bare-metal C firmware for a battery-powered brushed fan with a rotary encoder, PWM light, Charlieplexed numeric display, charging indication, and low-power sleep. The directory is named `NX32G0001`, but the configured MCU is **MindMotion MM32G0001A1TC**, Cortex-M0, with 16 KiB flash at `0x08000000` and 2 KiB RAM at `0x20000000`.

The current build entry point is `MDK-ARM/TIM3_TimeBase.uvprojx`, target **`TIM3_TimeBase`**. The sample-derived name does not describe the full application. It selects **ARM Compiler 5.06 update 7 (build 960)**, not Arm Compiler 6, and device pack **MindMotion.MM32G0001_DFP.1.0.1**. It defines `USE_STDPERIPH_DRIVER` and links with MicroLIB. The Keil startup reserves a 512-byte stack and zero heap.

`readme.txt` is inherited vendor-example documentation: it lists MDK-ARM 5.23, EWARM 8.22.1, the MB-091 / Mini-G0001 board, and MM32-LINK MINI (CMSIS-DAP). The locally verified Keil installation is MDK 5.41 with the ARMCC version above. Do not assume the example's board wiring or serial-output instructions describe the current fan firmware.

## Build and validation

Run these from the repository root in **Git Bash on Windows**. Set `UV4` to the installed executable; the value below was verified on this machine. Absolute project/log paths avoid ambiguity with the spaces in the working directory.

```bash
UV4="D:/Users/mym02/AppData/Local/Keil_v5/UV4/UV4.exe"
ROOT="$(pwd -W)"

# Incremental build
"$UV4" -b "$ROOT/MDK-ARM/TIM3_TimeBase.uvprojx" -t "TIM3_TimeBase" -o "$ROOT/MDK-ARM/build.log"

# Rebuild all sources
"$UV4" -r "$ROOT/MDK-ARM/TIM3_TimeBase.uvprojx" -t "TIM3_TimeBase" -o "$ROOT/MDK-ARM/rebuild.log"
```

Alternatively, open the `.uvprojx` in Keil and build/rebuild target `TIM3_TimeBase`. Read the output log: uVision returns **0 for success without warnings, 1 for warnings, and 2 or higher for errors**. A verified full rebuild produced 0 errors and 3 existing unused-symbol warnings for `show_gear`, `gear_duty`, and `light_toggle` in `USER/user.c`; exit code 1 alone is not a failed compilation.

Outputs are `MDK-ARM/Objects/TIM3_TimeBase.axf` and `.hex`; the map is `MDK-ARM/Listings/TIM3_TimeBase.map`. `Objects/`, `Listings/`, and `.uvguix.*` contain generated output or IDE user state, not application source. The linker scatter file under `Objects/` is generated from the target memory settings.

There is **no automated test suite, single-test command, lint configuration, or Make/CMake build** in this tree. Compilation does not validate GPIO polarity, ADC calibration, display timing, or sleep/wake behavior; those require the target hardware. The project debug settings use CMSIS-DAP. Building does not require flashing. The readme notes that J-LINK use requires revisiting the console UART configuration, but the current application leaves the console disabled.

### Project-file caveats

- Keil uses explicit source lists, not directory discovery. Add new translation units to the appropriate group in the `.uvprojx`.
- Keil's include list contains obsolete `..\..\TIM3_TimeBase` and directory-name-dependent `..\..\NX32G0001`, relative to `MDK-ARM/`. The latter currently supplies the root headers. Check these when moving or renaming the checkout.
- **`EWARM/` is an unsynchronized legacy project**, not an equivalent current build: it references missing `tim3_timebase.c`, omits `USER/user.c` and its include directory, and uses old five-level-up paths to `Device/`. Its Debug/Release configurations need repair before use.

## Runtime architecture

### Startup and scheduling

`Device/MM32G0001/Source/KEIL_StartAsm/startup_mm32g0001_keil.s` calls `SystemInit()` and the C runtime before `main()`. `Device/MM32G0001/Source/system_mm32g0001.c` selects **48 MHz HSI**, with undivided AHB/APB clocks; the IDE's 12 MHz clock metadata is not the runtime clock configuration.

`main.c` initializes the platform, timers, ADC, display, key, encoder, and light, then seeds the battery estimate. Its foreground loop calls `app_task()` when `flag_1ms` is set. There is no RTOS:

- **TIM14 update interrupt**, in `mm32g0001_it.c`, runs approximately every 0.25 ms, calls `led_scan()`, and sets `flag_1ms` every fourth interrupt.
- **SysTick** runs the separate millisecond countdown for blocking `PLATFORM_DelayMS()`; it does not schedule `app_task()`.
- `flag_1ms` is a single flag, not a queued tick count. Blocking foreground work can coalesce application ticks while interrupt-driven display scanning continues. `disp_buf` is also shared between foreground code and the ISR.

`platform.c` provides the delay and vendor-demo console/LED helpers. `PLATFORM_Init()` currently initializes only the delay. Enabling the demo console or LEDs unchanged would reuse application pins, notably PA10 for the console versus the active key.

### Application and hardware coupling

Most product behavior and board-level drivers are colocated in **`USER/user.c`**, with the public interface in `USER/user.h`. `platform.h` includes `hal_conf.h`, which exposes the peripheral HAL. `Device/MM32G0001/HAL_Lib/`, device register headers, and `Device/CMSIS/` provide the vendor support layer.

The active `app_task()` first scans input and updates charging, filtered battery percentage, and undervoltage protection, then handles `APP_OFF`, `APP_ON`, or `APP_SLEEP`:

- PA10 short press toggles the light; long press controls fan power. The active scanner emits short press on release after more than 50 nominal milliseconds, and long press after 1500, not the timings in older comments.
- The PA0/PB0 quadrature encoder adjusts a 1–100 speed setpoint, with accelerated 10-step changes during sustained rotation. `APP_ON` alternates the speed display for 5 seconds and the battery display for 2 seconds; undervoltage forces the fan off and disables the light.
- `APP_OFF` displays battery status before transitioning toward sleep. `enter_sleep()` disables TIM14 and ADC, changes GPIO modes, and configures **PA10/EXTI10 and PA11/EXTI11** as wake sources before DeepStop/WFI. `reinit()` restores the clock and GPIO setup, then enables ADC and TIM14. Charging prevents DeepStop entry. Changes to peripheral initialization must also account for this restoration path.

Important shared hardware resources:

| Resource | Current use and constraint |
| --- | --- |
| TIM3 CH3 / PA15 | Inverted motor drive, center-aligned PWM, prescaler 0, period 1199 (~20 kHz). Compare **1200 means off**, **0 means maximum drive**. Intermediate setpoints use `840 - (72 * cur_duty_percent) / 10`, with a special case for 100; this is not a direct percentage of ARR. |
| TIM14 CH1 / PA9 | Light PWM shares TIM14 with the display scan and application timebase. Changing TIM14 period/prescaler changes all three. |
| PA4–PA8 | Five-pin Charlieplexed display, scanned in 18 steps. Foreground helpers prepare `disp_buf[6]`; the ISR consumes it. The buffer layout is documented in `USER/user.h`. |
| PB1 / ADC channel 0 | Battery sensing. Despite its name, `ADC_GetChannelVoltage()` returns raw 12-bit ADC counts. Battery tables, filtering, and protection thresholds are in `USER/user.c`; current conversion uses `VREF_MV=5000` and `BAT_DIVIDER=2`. |
| PA11 / ADC channel 4; PA12 | Charge detection and active-low charge-full input. PA11 is repurposed as an EXTI wake input during sleep. |

Several comments and disabled `#if 0` implementations still describe PB0 as a power key, a five-gear UI/`APP_BAT`, a 2399 PWM period, or a 1:11 battery divider. **Use active code, constants, and project source lists as the authority**, not those historical descriptions. Application sources contain Chinese comments and currently use UTF-8-compatible text with LF newlines; preserve them when editing.
