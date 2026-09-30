# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project and toolchain

This is bare-metal C firmware for a battery-powered brushed fan with an integrated water pump, six status LEDs, a power-latch circuit, and (currently disabled) battery-gauge and charge management. The directory is named `NX32G0001`, but the configured MCU is **MindMotion MM32G0001A1TC**, Cortex-M0, with 16 KiB flash at `0x08000000` and 2 KiB RAM at `0x20000000`. The schematic main IC is silkscreened **NX32F100T** (TSSOP-20); it is treated as MM32G0001-compatible for the HAL/pack, but that part-to-part compatibility is **unverified** — confirm on hardware before trusting register-level behavior.

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

Alternatively, open the `.uvprojx` in Keil and build/rebuild target `TIM3_TimeBase`. Read the output log: uVision returns **0 for success without warnings, 1 for warnings, and 2 or higher for errors**; exit code 1 alone is not a failed compilation. A verified full rebuild of the modular sources produces **0 errors, 0 warnings** (`Code=4360 RO-data=224 RW-data=68 ZI-data=516`). Battery and charge management are macro-gated off (see `USER/config/config.h`); code that is only referenced when they are enabled is wrapped in the same `#if` guard so it does not trip unused-symbol warnings while disabled.

Outputs are `MDK-ARM/Objects/TIM3_TimeBase.axf` and `.hex`; the map is `MDK-ARM/Listings/TIM3_TimeBase.map`. `Objects/`, `Listings/`, and `.uvguix.*` contain generated output or IDE user state, not application source. The linker scatter file under `Objects/` is generated from the target memory settings.

There is **no automated test suite, single-test command, lint configuration, or Make/CMake build** in this tree. Compilation does not validate GPIO polarity, ADC calibration, display timing, or sleep/wake behavior; those require the target hardware. The project debug settings use CMSIS-DAP. Building does not require flashing. The readme notes that J-LINK use requires revisiting the console UART configuration, but the current application leaves the console disabled.

### Project-file caveats

- Keil uses explicit source lists, not directory discovery. The APP group lists `main.c`, `mm32g0001_it.c`, `platform.c`, the `USER/user.c` stub, and each module's `.c` (`power`, `motor`, `key`, `led`, `adc`, `battery`, `charge`, `timer`, `app`). Adding a module means adding both its `.c` to the APP group **and** its folder to the C `IncludePath`.
- The C `IncludePath` (relative to `MDK-ARM/`) carries obsolete `..\..\TIM3_TimeBase` and directory-name-dependent `..\..\NX32G0001` (the latter supplies root headers), plus `..\USER` and one entry per module folder (`..\USER\config`, `..\USER\power`, …, `..\USER\app`). Check these when moving or renaming the checkout.
- **`EWARM/` is an unsynchronized legacy project**, not an equivalent current build: it references missing `tim3_timebase.c`, omits `USER/user.c` and its include directory, and uses old five-level-up paths to `Device/`. Its Debug/Release configurations need repair before use.

## Runtime architecture

### Startup and scheduling

`Device/MM32G0001/Source/KEIL_StartAsm/startup_mm32g0001_keil.s` calls `SystemInit()` and the C runtime before `main()`. `Device/MM32G0001/Source/system_mm32g0001.c` selects **48 MHz HSI**, with undivided AHB/APB clocks; the IDE's 12 MHz clock metadata is not the runtime clock configuration.

`main.c` calls `PLATFORM_Init()`, then `Power_Init()` first so the EN latch holds the rail up before the rest of init runs, then `Led_Init()`, `Motor_Init()`, `AppAdc_Init()`, `Key_Init()`, `AppTimer_Init()`, `Battery_Init()`, `Charge_Init()`, and `App_Init()`. Its foreground loop runs `App_Task()` once per set `flag_1ms`. There is no RTOS:

- **TIM14 update interrupt**, in `mm32g0001_it.c`, fires every 1 ms and sets `flag_1ms`. It no longer scans a display or divides by four; `AppTimer_Init()` in `USER/timer/timer.c` configures it (period = `TIM_GetTIMxClock(TIM14)/1000 - 1`).
- **SysTick** runs the separate millisecond countdown for blocking `PLATFORM_DelayMS()`; it does not schedule `App_Task()`.
- `flag_1ms` is a single flag, not a queued tick count. Blocking foreground work coalesces application ticks.

`platform.c` provides the delay and vendor-demo console/LED helpers. `PLATFORM_Init()` currently initializes only the delay. The demo console/LED helpers reuse application pins and should stay disabled.

### Application and hardware coupling

Product behavior is split by function under `USER/<module>/`, each as `<module>.c` + `<module>.h`: `config` (central macros, `.h` only), `power`, `motor`, `key`, `led`, `adc`, `battery`, `charge`, `timer`, `app`. `USER/user.c` and `USER/user.h` are now compatibility stubs (`user.h` just includes `app.h`). `platform.h` includes `hal_conf.h`, which exposes the peripheral HAL; `Device/MM32G0001/HAL_Lib/`, device register headers, and `Device/CMSIS/` provide the vendor support layer.

`App_Task()` (in `USER/app/app.c`) runs each 1 ms tick: it calls `Battery_Task()` and `Charge_Task()` (both no-ops while gated off), then `Key_Scan()`, then acts on key events. The state machine is `app_state_t { APP_STATE_OFF, APP_STATE_ON }`:

- **K3 / PA12** is the ON/OFF key: a short press toggles power and returns early that tick. `App_Init()` powers on with fan gear 1 and pump gear 0.
- **K1 / PA9** cycles the fan gear `1 → 2 → 3 → 1` while on.
- **K2 / PA10** cycles the pump gear `0 → 1 → 2 → 3 → 0` while on.
- `Key_Scan()` debounces each key for `KEY_DEBOUNCE_TIME_MS` (20 ms) and emits a short-press event on release.

Battery gauge (`Battery_Task`) and charge control (`Charge_Task`) are written but wrapped in `#if BATTERY_MANAGEMENT_ENABLE` / `#if CHARGE_MANAGEMENT_ENABLE`, both `0` in `config.h`. While disabled they take no ADC samples and never change motor or power state; `Charge_SetEnable()` forces the CH_EN pin low. Enable them only after the fan/pump path is validated.

Important hardware resources (verify polarity on hardware — the NX1031 gate-driver input polarity / PWM stop level is unconfirmed):

| Resource | Current use and constraint |
| --- | --- |
| TIM1 CH1N / PA5 (AF1) — fan | Center-aligned PWM at `MOTOR_PWM_FREQUENCY_HZ` (16 kHz); period = `TIM_GetTIMxClock(TIM1)/(2*16000) - 1` (≈1499 @48 MHz), duty via `TIM_SetCompare1`. Gears: 1 = 32 %, 2 = 18 %, 3 = 0 %. Gated by `TIM_CCxNCmd(TIM1, TIM_Channel_1, …)`. |
| TIM1 CH3 / PA6 (AF4) — pump | Same timer/period, independent CCR via `TIM_SetCompare3`. Gears: 0 = channel off, 1 = 30 %, 2 = 10 %, 3 = 0 %. Gated by `TIM_CCxCmd(TIM1, TIM_Channel_3, …)`. Both channels share `MOE` via `TIM_CtrlPWMOutputs`. |
| PA4 = EN, PA15 = CH_EN | Power latch. `Power_Init()` drives CH_EN low then EN high (keep-alive), push-pull. `Power_SafePowerOff()` drops CH_EN then EN. CH_EN defaults low and is forced low while charge management is disabled. |
| Keys PA12 / PA9 / PA10 | K3 / K1 / K2, active-low with pull-ups, 1 ms polled (no EXTI). Event bits `KEY_EVENT_K3/K1/K2`. |
| LEDs PA11/PA1/PA0/PA7/PA8/PA3 | LED1..LED6, active-low (MCU low = on), push-pull. `Led_Set`, `Led_SetMask`, `Led_AllOff`. |
| ADC1, 12-bit any-channel | VBus = CH0 / PB1, CH_VIN = CH1 / PB0 (both divide by `VBUS_DIVIDER_RATIO`/`CHARGE_INPUT_DIVIDER_RATIO` = 11), I_SENSE = CH5 / PA2 over the 5 mΩ shunt. Helpers in `USER/adc/adc.c` return mV / mA; `APP_ADC_VREF_MV = 5000`. |

The `mm32g0001_it.c` EXTI0_1 / EXTI4_15 handlers only clear pending bits; no wake source is armed (there is no sleep path in the current build). Application sources use Chinese comments in UTF-8 with LF newlines; preserve them when editing. When historical comments and active code disagree, **the active code, `config.h` constants, and the project source list are the authority**.
