# Task Plan: Charge cutoff at 22.5 V

## Goal
Implement CH_EN high for a confirmed adapter with C+ > 18.5 V and valid VBus < 22.5 V; low at VBus >= 22.5 V, resuming below the threshold while retaining existing fault/power safeguards. Build only; do not flash.

## Next Step
Review final authored-source diff and record validation results.

## Current Phase
Phase 3

## Phases
### Phase 1: Inspect current control
- [x] Capture requested thresholds and no-latch behavior.
- [x] Inspect implementation and dependencies.
- **Status:** complete

### Phase 2: Implement thresholds and resume behavior
- [ ] Update configuration and charge logic with minimal scope.
- [ ] Update affected comments/documentation.
- **Status:** in_progress

### Phase 3: Verify and deliver
- [ ] Exercise threshold boundaries and safety/feature-gate regressions.
- [ ] Rebuild the Keil target and read logs.
- [ ] Review diff and summarize exact behavior and hardware limitations.
- **Status:** pending

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| No voltage-stop latch until unplug | User explicitly requires CH_EN high below 22.5 V when input qualifies. |
| Keep fault interlocks and detection debounce | Voltage setting does not imply bypassing ADC/power protection. |
| Software changes only | 22.5 V exceeds typical 5S Li-ion full voltage; no hardware test is authorized. |

## Errors Encountered
| Error | Resolution |
|-------|------------|
| Initial resolver produced no selection with multiple historic plans | Created this task's named plan; use this exact directory only. |
| Installing temporary Unicorn/pyelftools packages was denied by the permission system | Do not retry installation without explicit approval. Continue local build and source checks; use an already-installed test environment only if available. |
