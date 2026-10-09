# Findings & Decisions

## Requirements
- VBus >= 22500 mV: CH_EN low; VBus < 22500 mV: CH_EN high if qualified C+ > 18500 mV and no safety interlock.
- Resume after voltage falls; remove existing unplug-only voltage-stop latch.
- Retain ADC/overvoltage/power-release safeguards and adapter confirmation filtering.
- User has been told 22.5 V is above typical 5S Li-ion full voltage and a 20 V adapter generally cannot reach it. No flashing/charging experiment.

## Research Findings
- `USER/config/config.h` currently contains 18500 mV insertion and 18000 mV removal (different from initial CLAUDE documentation), and a 20000 mV cutoff. Preserve current user edits if any.
- `Charge_IsAllowed()` latches `charge_voltage_stopped` high at cutoff; only confirmed removal clears it. Output currently accepts input above REMOVE threshold, so it must explicitly require > INSERT threshold for the requested condition.
- `Charge_Task()` confirms insertion with >= INSERT; use strict > for the user's wording. Retain 18000 mV removal hysteresis for stable adapter presence/power logic while prohibiting CH_EN at <=18500 mV.
- Output updates are staged: charge task immediately turns off, then end-of-app output may enable; retain this to avoid glitches after battery start checks.
- `Charge_IsVoltageStopped()` is an existing status API, and charge.h/app.c comments still describe an unplug-only latch; update wording with implementation.
- `git diff -- USER/config/config.h` is empty: the existing 18.5/18.0 V thresholds are checked in, not live user edits. Initial application working tree is clean.
- ARMCC/ARMLINK/FROMELF are installed; native gcc/clang are not on PATH. Python is available; isolated default Python has no unicorn/elftools installed.
- Battery module retains ADC-fault/OVP recovery and independent undervoltage; charging below cutoff must still be blocked by ADC/OVP faults, but not solely by discharge UVP.
- CLAUDE.md is stale for both adapter detection (9/8 V versus checked-in 18.5/18.0 V) and the requested cutoff semantics; update related sections only after verification.
- No test-named files were found in the repository. A prior temporary test harness exists at `C:/Users/mym02/AppData/Local/Temp/nx32-charge-tests-g8wpebvo` with actual ADC/battery/charge/app/power tests and mocked HAL. Its reviewed runner uses an already-installed dependency directory `C:/Users/mym02/AppData/Local/Temp/nx32-emulator-deps-vnd5gi_l`. Reuse that local dependency only if present, without installing anything or modifying prior tests.
- Full enabled Keil rebuild passed: Code=6632, RO=236, RW=92, ZI=516; 0 errors, 0 warnings.
- Full-tree diff check reports only generated Keil output whitespace; authored source must be checked separately. Source LF bytes are preserved despite Git autocrlf warnings.

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| Recompute voltage-stop status from current valid sample | No unplug reset required below 22.5 V; preserve public status API. |
| Preserve separate detection hysteresis, enforce >18.5 V for charging | Maintains shutdown debounce without charging under the user threshold. |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| Multiple historical plans prevented implicit resolution | Isolated plan `.planning/2026-10-09-charge-22-5v-threshold`. |

## Resources
- `USER/config/config.h`
- `USER/charge/charge.c`
