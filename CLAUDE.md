# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project and toolchain

This is bare-metal C firmware for a battery-powered brushed fan with an integrated water pump, six gear-status LEDs, a power-latch circuit, enabled battery undervoltage/overvoltage protection, adapter-input detection, and **enabled charging-path control with a 22.5 V VBus cutoff and automatic resume below that threshold**. The product uses a **5-series Li-ion pack without a BMS** and a **20 V adapter that the user confirms supplies hardware-regulated charging current** (4 A target). Firmware does not regulate charging current or measure it; the board's I_SENSE is only motor current. The software cutoff is not a full-charge or cell-level protection guarantee. There is no battery-percentage calculation or display. The directory is named `NX32G0001`, but the configured MCU is **MindMotion MM32G0001A1TC**, Cortex-M0, with 16 KiB flash at `0x08000000` and 2 KiB RAM at `0x20000000`. The schematic main IC is silkscreened **NX32F100T** (TSSOP-20); it is treated as MM32G0001-compatible for the HAL/pack, but that part-to-part compatibility is **unverified** — confirm on hardware before trusting register-level behavior.

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

Alternatively, open the `.uvprojx` in Keil and build/rebuild target `TIM3_TimeBase`. Read the output log: uVision returns **0 for success without warnings, 1 for warnings, and 2 or higher for errors**; exit code 1 alone is not a failed compilation. A verified full rebuild with voltage protection, adapter detection and charge control enabled produces **0 errors, 0 warnings** (`Code=6632 RO-data=236 RW-data=92 ZI-data=516`). `BATTERY_PROTECTION_ENABLE=1`, `CHARGE_INPUT_DETECTION_ENABLE=1`, and `CHARGE_MANAGEMENT_ENABLE=1` in `USER/config/config.h`; input detection and actual charge control remain separate compile-time gates. Charging is fail-closed if detection or valid battery sampling is unavailable. Code used only by optional features remains inside the corresponding `#if` guard.

Outputs are `MDK-ARM/Objects/TIM3_TimeBase.axf` and `.hex`; the map is `MDK-ARM/Listings/TIM3_TimeBase.map`. `Objects/`, `Listings/`, and `.uvguix.*` contain generated output or IDE user state, not application source. The linker scatter file under `Objects/` is generated from the target memory settings.

There is **no self-contained checked-in automated test suite, single-test command, lint configuration, or Make/CMake build** in this tree. The 22.5 V non-latching charge policy was tested in a temporary local ARM CPU emulator with the actual ADC/battery/charge/app/power modules and mocked HAL/keys/motors: 156 assertions for the enabled configuration, 59 with detection disabled, 92 with charge control disabled, and 17 with battery protection/sampling disabled (charge fails closed). All four module-test configurations compiled, linked and passed. The full Keil target was rebuilt with all three features enabled, producing zero errors/warnings; these are also the final artifact settings. The regression adapter and results are recorded under `.planning/2026-10-09-charge-22-5v-threshold/`, but rerunning the adapter requires the existing temporary fixture and emulator dependencies. These are software-only tests, not MCU/peripheral validation. Compilation does not validate GPIO polarity, ADC calibration, display timing, or sleep/wake behavior; those require the target hardware. The project debug settings use CMSIS-DAP. Building does not require flashing. The readme notes that J-LINK use requires revisiting the console UART configuration, but the current application leaves the console disabled.

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

`App_Task()` (in `USER/app/app.c`) wraps `App_ProcessTask()`: battery/input sampling and immediate charge cutoff run first, then keys/voltage/ADC protection and motor changes, and finally `Charge_UpdateOutput()` evaluates the latest samples and power decision. This also catches a fault or cutoff discovered by `Battery_CheckBeforeStart()` on the same tick without a spurious charge-enable pulse. The state machine is `app_state_t { APP_STATE_OFF, APP_STATE_ON, APP_STATE_PROTECT }`; OFF describes stopped motors, not necessarily an unpowered MCU:

- **K3 / PA12** is the ON/OFF key. `App_Init()` keeps the motors stopped (preserving the previously commented-out automatic start), and ignores the release of K3 if it was held during boot. A subsequent short press starts fan gear 1 and pump gear 0 only after `Battery_CheckBeforeStart()` succeeds. K3 powers off from ON or PROTECT.
- **K1 / PA9** cycles the fan gear `1 → 2 → 3 → 1` while on.
- **K2 / PA10** cycles the pump gear `0 → 1 → 2 → 3 → 0` while on.
- `Key_Scan()` debounces each key for `KEY_DEBOUNCE_TIME_MS` (20 ms) and emits a short-press event on release.
- **PROTECT** stops motors and gear LEDs but keeps EN for sampling. Discharge undervoltage alone does **not** block the hardware-current-regulated charging path; ADC fault and overvoltage do block it. Once faults clear, it returns to OFF without restarting, discards events on that recovery tick, and requires a new K3 operation.
- K3 shutdown from ON or PROTECT sets a separate user-off request. With confirmed adapter input, it stops motors/LEDs but keeps EN and monitoring; confirmed removal subsequently calls `Power_SafePowerOff()`. Unknown/fault input also keeps monitoring rather than guessing removal. A pending insertion after a previously confirmed absence must not release EN. With input detection compiled out, shutdown uses the original immediate power-off behavior. Running motors are not stopped merely because the adapter is unplugged; battery/ADC protection still takes priority. Initial boot standby remains unchanged and does not automatically start motors or power down.
- Motor use with adapter input is allowed and K3 logical shutdown does not cancel eligible charging. Charging eligibility follows the latest valid voltage rather than a voltage-cutoff latch; voltage sag below 22.5 V can re-enable charging if the other checks pass. K3 may start motors again subject to battery checks; reaching the charging cutoff alone does not stop motors. If the MCU remains powered after an EN release, newly confirmed adapter insertion can reacquire EN and evaluate charging without automatically starting motors.

Battery protection is enabled with `BATTERY_PROTECTION_ENABLE=1` in `config.h`. It has no percentage/SOC calculation or display. `Battery_Init()` (called only in `main.c`, after ADC setup) and `Battery_CheckBeforeStart()` immediately sample VBus and latch unsafe voltages; the start check never clears an existing latch. Periodic detection uses independent saturating counters, incrementing on fault samples and decrementing on normal samples, adapted from the reference drill's `Volt_Handler`:

- Undervoltage: **VBus <= 13.5 V**, 300 fault counts; after latching, recovery requires **VBus >= 15.5 V continuously for 300 task ticks**.
- Overvoltage: **VBus >= 24 V**, 300 fault counts; recovery requires **VBus < 24 V continuously for 300 task ticks** (no separate voltage hysteresis).
- Both faults must clear before another start is allowed. Recovery is not an automatic motor restart. Counts correspond to milliseconds only while the foreground services every 1 ms tick; the single tick flag can coalesce delays.

Charge control is enabled with `CHARGE_MANAGEMENT_ENABLE=1`. `Charge_Task()` uses independent adapter detection (**C+ > 18.5 V insert / C+ <= 18.0 V remove**, 100 consecutive service ticks). The detection hysteresis retains confirmed presence between those thresholds, but charging requires the latest valid **C+ > 18.5 V** sample; C+ <= 18.5 V immediately closes the path without waiting for confirmed removal. These are input-detection thresholds, not charging-voltage targets, and require hardware calibration. Once confirmed input and valid VBus samples are available, `Charge_UpdateOutput()` enables CH_EN unless blocked by ADC fault, overvoltage, actual power release, or the current cutoff condition. **VBus >= 22.5 V immediately drives CH_EN low; VBus < 22.5 V allows CH_EN high again when all other conditions pass.** There is no unplug-only voltage-stop latch or voltage hysteresis, so voltage sag or sampling noise across the threshold can switch the path repeatedly. `Charge_IsEnabled()` is an output state, not proof of battery charging current; `Charge_IsVoltageStopped()` is not a full-charge indication. A 20 V-limited source plus series drops cannot normally reach 22.5 V VBus, so this cutoff cannot be promised to occur with that source. The 4 A constant is a hardware target only. The firmware neither implements CC/CV regulation nor uses motor I_SENSE as charging feedback, and provides no temperature or cell-level protection. The existing 24 V fault threshold is not the charging cutoff.

Important hardware resources (verify polarity on hardware — the NX1031 gate-driver input polarity / PWM stop level is unconfirmed):

| Resource | Current use and constraint |
| --- | --- |
| TIM1 CH1N / PA5 (AF1) — fan | Center-aligned PWM at `MOTOR_PWM_FREQUENCY_HZ` (16 kHz); period = `TIM_GetTIMxClock(TIM1)/(2*16000) - 1` (≈1499 @48 MHz), duty via `TIM_SetCompare1`. Gears: 1 = 32 %, 2 = 18 %, 3 = 0 %. Gated by `TIM_CCxNCmd(TIM1, TIM_Channel_1, …)`. |
| TIM1 CH3 / PA6 (AF4) — pump | Same timer/period, independent CCR via `TIM_SetCompare3`. Gears: 0 = channel off, 1 = 30 %, 2 = 10 %, 3 = 0 %. Gated by `TIM_CCxCmd(TIM1, TIM_Channel_3, …)`. Both channels share `MOE` via `TIM_CtrlPWMOutputs`. |
| PA4 = EN, PA15 = CH_EN | Power latch. `Power_Init()` drives CH_EN low then EN high (keep-alive), push-pull. `Power_SafePowerOff()` drops CH_EN then EN. K3 logic-off keeps monitoring while input is present, unknown or faulty; confirmed removal releases EN. C+ can also hold the rail via R6/Q3 independently of EN. CH_EN starts low and is subsequently controlled by the charging module's cutoff and safety checks. |
| Keys PA12 / PA9 / PA10 | K3 / K1 / K2, active-low with pull-ups, 1 ms polled (no EXTI). Event bits `KEY_EVENT_K3/K1/K2`. |
| LEDs PA11/PA1/PA0/PA7/PA8/PA3 | LED1..LED6, active-low (MCU low = on), push-pull. `Led_Set`, `Led_SetMask`, `Led_AllOff`. |
| ADC1, 12-bit any-channel | VBus = CH0 / PB1, CH_VIN = CH1 / PB0 (both divide by `VBUS_DIVIDER_RATIO`/`CHARGE_INPUT_DIVIDER_RATIO` = 11), I_SENSE = CH5 / PA2 over the 5 mΩ shunt. Helpers in `USER/adc/adc.c` return mV / mA; `APP_ADC_VREF_MV = 5000`. |

The charge path is C+ → Q2 (NX3407 P-MOS) → D1 (SS56) → B+, controlled by Q4 (MMBT5551) from CH_EN. With R28=10 kΩ / R29=20 kΩ, enabled VGS is approximately −C+/3 (about −6.7 V at 20 V), subject to hardware verification. VBus measures post-latch VCC rather than an independent battery-terminal sense. A 20 V adapter minus D1 drop cannot reach the usual 21 V full voltage of 5 × 4.2 V cells, but can still overcharge an imbalanced cell without a BMS. The requested 22.5 V cutoff averages 4.5 V per cell for a 5S pack, above the usual 4.2 V Li-ion limit; it is not a safe full-charge setting for such cells. External current regulation is a user-confirmed hardware responsibility; the adapter's maximum compliance voltage, component thermal limits, ADC/battery-terminal offset, current sharing during motor use and cell-level protection remain unverified. Validate these separately with appropriate equipment; no battery charging experiment or flashing is implied by a software build.

The `mm32g0001_it.c` EXTI0_1 / EXTI4_15 handlers only clear pending bits; no wake source is armed (there is no sleep path in the current build). Application sources use Chinese comments in UTF-8 with LF newlines; preserve them when editing. When historical comments and active code disagree, **the active code, `config.h` constants, and the project source list are the authority**.
