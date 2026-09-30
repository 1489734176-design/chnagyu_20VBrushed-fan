# Findings

## Requirements
- Create root CLAUDE.md with the exact requested English prefix.
- Include supported build/lint/test commands and non-obvious architectural relationships.
- Do not invent tests or workflows; use README and editor instructions if present.

## Discovery
- No existing CLAUDE.md, README.md, Cursor rules, Copilot instructions, or prior planning files were found in the initial scan.
- Root contains readme.txt, C firmware sources, USER/, Device/ CMSIS and MM32G0001 HAL, and Keil MDK-ARM / IAR EWARM projects.
- This directory is not a Git repository.
- readme.txt is inherited TIM3_TimeBase sample documentation: MM32 MiniBoard MB-091 / Mini-G0001, MDK-ARM 5.23, EWARM 8.22.1, MM32-LINK MINI (CMSIS-DAP); J-LINK requires checking console UART configuration in platform.c.
- Keil target TIM3_TimeBase selects MM32G0001A1TC, Cortex-M0, ARMCC 5.06 update 7 build 960, MindMotion.MM32G0001_DFP.1.0.1, 16 KiB flash and 2 KiB RAM. Hex output enabled under Objects/.
- No Make/CMake, shell build scripts, lint configs, or test-named files found.
- main.c initializes platform/peripherals, seeds battery readings, then runs app_task() whenever flag_1ms is set. USER/user.h exposes peripheral setup and application functions plus a shared six-byte display buffer.
- Keil explicitly compiles main.c, platform.c, mm32g0001_it.c, USER/user.c, vendor HAL, and Keil startup/system sources; USE_STDPERIPH_DRIVER is defined. Include paths contain legacy directory-name dependencies.
- System clock source selects 48 MHz HSI despite 12 MHz IDE metadata. TIM14 is shared between lighting PWM and ~0.25 ms display scan IRQ; every fourth IRQ sets the one-bit app tick. SysTick only decrements the platform delay counter.
- USER/user.c combines peripheral setup, 18-step Charlieplexed display, key scanning, battery/charging, and application logic. Several block comments are stale (PWM period, key behavior, divider/voltage thresholds): document executable constants and active branches, not comments alone.
- Active key scanner reads PA10, short press on release after >50 ms, long press at 1500 calls; prior PB0 implementation is disabled with #if 0.
- ADC_GetChannelVoltage() returns raw 12-bit counts, not volts. Actual battery conversion constants are VREF_MV=5000 and BAT_DIVIDER=2; charge/discharge curves and filtering drive displayed pct.
- IAR is stale: APP group still references missing tim3_timebase.c, omits USER/user.c, lacks USER include path, and reaches five directories up for Device/. Debug and Release configs exist but are not equivalent current firmware build entry points.
- UV4.exe, armcc.exe, IarBuild.exe, GCC, clang, cppcheck not on PATH; uVision absent from the three common C:/D: locations checked. Existing Keil build artifacts are available for inspection, not proof of a fresh build.
- PLATFORM_Init currently only enables SysTick delay; serial console and demo LEDs are disabled. Enabling console would reuse PA10 (actual application key).
- Active app_task handles key/encoder, charge indication, battery filtering and UVP on each tick, then APP_OFF/APP_ON/APP_SLEEP. Short press toggles light; long press controls fan. Encoder changes 1–100% setpoint with accelerated steps. Motor compare is inverse: 1200 off, 0 maximum; intermediate mapping is calibrated, not linear percentage of timer period.
- APP_ON alternates speed display (5 s) and battery display (2 s); APP_BAT and gear-based modes in comments/disabled app_task are legacy. APP_OFF times out to APP_SLEEP after nominal 10 s (initial bat_timer is 9000).
- enter_sleep disables TIM14 and ADC, changes GPIO modes and configures PA10/PA11 EXTI wake before DeepStop/WFI. reinit restores SystemInit, GPIO modes, ADC and TIM14. Charging input prevents entry to DeepStop.
- Keil startup calls SystemInit then __main; heap is zero. Memory limits and ISR/main shared state are important architecture constraints.
- Historical build log records MDK 5.41 with ARMCC 5.06u7, successful output with three unused-symbol warnings (show_gear, gear_duty, light_toggle), and toolchain under D:/Users/mym02/AppData/Local/Keil_v5/. It is not a newly executed build. Do not copy the log's personal/license information into CLAUDE.md.
- Keil startup reserves 0x200 bytes of stack and no heap; linker uses microlib and a generated scatter file.
- The log-derived local UV4.exe and ARMCC executables actually exist. All Keil source references resolve. Include directory ../../TIM3_TimeBase is absent, while ../../NX32G0001 currently resolves to this project root (rename-sensitive).
- Application C/headers inspected are UTF-8-compatible with LF newlines, including Chinese comments.
- Validated UV4 -b against TIM3_TimeBase: incremental build returned 0, 0 errors / 0 warnings (no source recompilation).
- Validated UV4 -r: all sources recompiled and AXF/HEX generated; 0 errors / 3 warnings for existing unused show_gear, gear_duty, light_toggle. UV4 returned 1 for warnings. Code=7232, RO=364, RW=108, ZI=516 bytes. No hardware flashed or tested.
- Deliverable created: root CLAUDE.md, covering verified Git Bash/Keil commands, no automated tests/lint, legacy IAR status, startup/timing, application state and shared hardware, and stale-comment cautions. No firmware sources changed.
