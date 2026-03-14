# AM32 Flight Safety Remediation — Live Status

This document tracks the safety remediation effort against the AM32 ESC firmware.
All work is being done publicly so the community can follow along and adopt changes.

For the full analysis and methodology see [AM32_Code_Review_Report.md](AM32_Code_Review_Report.md)
and [AM32_Remediation_Plan.md](AM32_Remediation_Plan.md).

---

## Overall Progress

| Tier | Tasks | Done | In Progress | Remaining |
|------|-------|------|-------------|-----------|
| T0 — CI/CD Infrastructure | 1 | ✅ 1 | — | — |
| T1 — Critical Safety | 6 | ✅ 2 | — | 4 |
| T2 — Robustness Hardening | 8 | — | — | 8 |
| T3 — Code Quality | 6 | — | — | 6 |
| **Total** | **21** | **3** | **—** | **18** |

---

## Tier 0 — CI/CD Infrastructure

### ✅ T0.0 — Safety CI Pipeline
**Status:** Complete  
**Commit:** `safety: T0.0+T1.1+T1.6 — volatile qualifiers, cmd 36 guard, CI pipeline`

Added `.github/workflows/safety_ci.yml` — a 4-job pipeline that runs on every push and PR:

1. **Volatile Audit** — `tests/check_volatile.py` scans all 10 MCU families and verifies every ISR-shared variable carries `volatile` in both its definition and every `extern` declaration (67 declarations checked)
2. **cppcheck** — Static analysis on `Src/` and `Src/DroneCAN/` with `--error-exitcode=1`
3. **Unit Tests** — Host-compiled Unity tests (no hardware required): 23 tests covering DShot CRC, throttle decode, command 36 guard, bidirectional telemetry CRC, and CRC-16/CCITT-FALSE spec
4. **Firmware Build Verify** — Full build of all 10 MCU targets, gated behind the three jobs above

---

## Tier 1 — Critical Safety Fixes

### ✅ T1.1 — `volatile` Qualifiers on All ISR-Shared Variables
**Status:** Complete  
**Risk closed:** Compiler at `-O3` was free to cache ISR-written variables in registers, causing the main loop to never observe updates — leading to missed desync events, stale duty cycle, and incorrect arming state.

**Variables fixed (10):**

| Variable | Shared between |
|----------|----------------|
| `armed` | DShot/signal ISR ↔ main state machine |
| `running` | Commutation ISR ↔ main loop |
| `step` | Comparator/commutation ISR ↔ main loop |
| `commutation_interval` | Commutation ISR ↔ PID / timing |
| `commutation_intervals[6]` | Commutation ISR ↔ `e_com_time` averaging |
| `duty_cycle` | PID loop ↔ PWM timer ISR |
| `zero_crosses` | ZC ISR ↔ state machine |
| `zcfound` | ZC ISR ↔ main loop |
| `bemfcounter` | Comparator ISR ↔ main loop |
| `desync_check` | Commutation ISR ↔ desync recovery |

**DMA buffers fixed (3 types, all 10 MCU families):**

| Buffer | Files |
|--------|-------|
| `dma_buffer[64]` | `Mcu/*/Src/IO.c` (10 files) |
| `ADCDataDMA[N]` | `Mcu/*/Src/ADC.c` (9 families) |
| `ADC1DataDMA[2]` / `ADC2DataDMA[2]` | `Mcu/g431/Src/ADC.c` |

**Extern declarations updated (25 locations across 15 files):**
`Inc/common.h`, `Inc/dshot.h`, all 10 `Mcu/*/Inc/comparator.h`, 6 MCU `*_it.c` ISR files, `Src/DroneCAN/DroneCAN.c`

**Bonus fix:** `extern int zero_crosses` in `Src/dshot.c` corrected to `extern volatile uint32_t zero_crosses` (type mismatch with the `uint32_t` definition in `main.c`).

---

### ✅ T1.6 — DShot Command 36 (Programming Mode) Flight Guard
**Status:** Complete  
**Risk closed:** DShot command 36 triggered EEPROM writes regardless of arming state. A flight controller glitch or malicious frame could reprogram the ESC mid-flight, causing immediate motor behavioural change or shutdown.

**Fix in `Src/dshot.c`:** Entry to programming mode is now gated:
```c
case 36:
    // Only allow programming mode entry when motor is disarmed and stopped
    if (!armed && !running) {
        programming_mode = 1;
    }
    break;
```

---

### ⬜ T1.2 — Critical Sections Around `commutation_intervals[]` Array Read
**Status:** Not started  
**Risk:** Non-atomic 6-element array read of `commutation_intervals[]` during `e_com_time` averaging — a mid-read ISR update can corrupt the average, causing incorrect timing and desync.

---

### ⬜ T1.3 — Replace `NVIC_SystemReset()` with Graceful Failsafe
**Status:** Not started  
**Risk:** Signal loss triggers a hard MCU reset after ~500 ms. Mid-flight this is catastrophic — the motor goes open-circuit during reset (50–100 ms) before the ESC restarts and re-arms. Should fade to a configurable safe throttle instead.

---

### ⬜ T1.4 — EEPROM CRC-16 Integrity Check
**Status:** Not started  
**Risk:** Settings are loaded from EEPROM with no integrity verification. Corrupted flash (power loss during write, flash wear) silently loads garbage motor parameters.

**Note:** CRC-16/CCITT-FALSE test vectors and roundtrip tests are already written in `tests/test_crc.c` (9 tests passing) as a TDD specification for the implementation.

---

### ⬜ T1.5 — Gradual Desync Recovery
**Status:** Not started  
**Risk:** Desync detection sets `running = 0` immediately — full motor stop. At flight speed this is a hard stop event. Recovery should be a controlled deceleration to a retry-startup, not an instant cutoff.

---

## Tier 2 — Robustness Hardening

| ID | Task | Status |
|----|------|--------|
| T2.1 | Watchdog timer enable + feed discipline | ⬜ Not started |
| T2.2 | Input signal bounds validation (clamp before use) | ⬜ Not started |
| T2.3 | Startup self-test (ADC range, BEMF sense check) | ⬜ Not started |
| T2.4 | Stack overflow detection (canary pattern) | ⬜ Not started |
| T2.5 | `commutation_interval` overflow / stall guard | ⬜ Not started |
| T2.6 | DroneCAN node health monitoring | ⬜ Not started |
| T2.7 | Over-temperature hysteresis (prevent rapid enable/disable) | ⬜ Not started |
| T2.8 | Consistent ADC averaging across all MCU families | ⬜ Not started |

---

## Tier 3 — Code Quality

| ID | Task | Status |
|----|------|--------|
| T3.1 | Extract motor control into separate translation unit | ⬜ Not started |
| T3.2 | Replace magic numbers with named constants | ⬜ Not started |
| T3.3 | Eliminate duplicated HAL code across MCU families | ⬜ Not started |
| T3.4 | Add doxygen headers to public API functions | ⬜ Not started |
| T3.5 | MISRA-C subset compliance pass | ⬜ Not started |
| T3.6 | Remove dead/commented-out code | ⬜ Not started |

---

## Test Coverage

| Suite | Tests | Pass | Fail |
|-------|-------|------|------|
| `tests/test_dshot_decode.c` | 14 | 14 | 0 |
| `tests/test_crc.c` | 9 | 9 | 0 |
| `tests/check_volatile.py` | 67 declarations | 67 | 0 |
| **Total** | **90 checks** | **90** | **0** |

Run locally:
```bash
cd tests && make run          # unit tests
python3 tests/check_volatile.py  # volatile audit
```

---

*Last updated: 2026-03-13*
