# QMI-divider overclock plan update

## 1. Decision and scope

This document supersedes the frequency-bisection sequence in
`overclock_test_plan.md`. Preserve that file as the historical plan and retain
all of its correctness, logging, recovery, temperature, voltage, and commit
rules unless this update explicitly changes the sequence.

Stop bisecting the 396--420 MHz interval with QMI divider 3. The next firmware
experiment is a controlled QMI-divider change, followed by a new Hazard3
frequency walk that keeps external flash SCK at or below approximately
130 MHz:

- use divider 4 through 516 MHz;
- use divider 5 above 516 MHz;
- begin with exactly 420 MHz, divider 4, and requested/read-back 1.20 V;
- retain the 590 MHz clock ceiling and 1.60 V selector ceiling;
- keep temperature acquisition disabled;
- never treat requested VREG as a physical voltage measurement;
- keep the complete 4,096-case optimized oracle mandatory at every point.

Planning only: this document does not authorize a flash by itself and does not
change firmware, voltage, clock, QMI timing, or linker placement.

## 2. Failure-pattern analysis

Every successful overclock in the current campaign reports QMI divider 3.
Therefore its approximate external flash SCK is `clk_sys / 3`:

| System clock | Divider | Flash SCK | Result |
|---:|---:|---:|---|
| 150 MHz | 3 | 50 MHz | repeatable stock pass at 1.10 V |
| 300 MHz | 3 | 100 MHz | pass at 1.10 V |
| 348 MHz | 3 | 116 MHz | pass at 1.20 V; 1.15 V boots but fails oracle |
| 396 MHz | 3 | 132 MHz | pass only at 1.60 V; lower selectors fail or go silent |
| 408 MHz | 3 | 136 MHz | BOOT_FAIL attempt 1 at 1.60 V |
| 420 MHz | 3 | 140 MHz | reproduced BOOT_FAIL at 1.60 V |
| 444 MHz | 3 | 148 MHz | reproduced BOOT_FAIL at 1.60 V |
| 480 MHz | 3 | 160 MHz | reproduced BOOT_FAIL at 1.60 V |
| 570 MHz | 3 | 190 MHz | reproduced BOOT_FAIL at 1.60 V |

This is strong evidence for a flash/QMI ceiling near 130--135 MHz. It explains
why additional core voltage does not restore 408 MHz and above, and why an
alternative firmware with less XIP dependence or a larger flash divider can
run beyond 500 MHz. It is still a hypothesis until 420 MHz/divider-4 hardware
evidence exists.

Core voltage remains an independent variable. At 348 MHz/divider 3, 1.15 V
booted normally but the 4,096-case oracle failed at vectors 1412 and 123 in two
attempts. Those failures demonstrate silent core-logic marginality that a
successful boot cannot detect. No divider result may bypass the oracle.

The current 396 MHz result is almost perfectly linear at about 2,465.5 H/s/MHz.
Its simple 570 MHz projection is approximately 1,405,000 H/s (about 1.41 MH/s),
compared with the user's alternative implementation at 1.261 MH/s. Treat this
as a search target, not an acceptance criterion.

## 3. Divider-control infrastructure

Implement this as one standalone source commit before the 420 MHz run. Do not
combine it with SRAM placement.

### 3.1 Build parameter and wrapper

1. Add `MINER_QMI_CLKDIV`, default `3`, to `CMakeLists.txt` and `tools/build`.
2. Allow only integer values `3`, `4`, and `5` for this campaign.
3. Pass it as a compile definition and print it in wrapper configuration.
4. Extend `tools/cycle` and `tools/monitor.py` with an exact expected-divider
   argument. A requested/read-back mismatch is a hard failure.
5. Calculate `qmi_sck_hz = actual_clk_sys_hz / readback_divider` in firmware
   and print it in `BOOT`. The monitor must recompute and verify this value.
6. For campaign profiles above stock, reject a configuration in the wrapper if
   its calculated QMI SCK exceeds 130 MHz. This prevents accidental regression
   to divider 3 at high system clocks.

### 3.2 Safe ordering in firmware

The RP2350 register documentation explicitly permits changing
`QMI_M0_TIMING.CLKDIV` on the fly. It also requires a dummy memory-window access
and barriers when increasing the divider in anticipation of a system-clock
increase.

Implement a small SRAM-resident startup helper and use this order:

1. validate the requested PLL and VREG selector;
2. apply VREG and its settling delay as already implemented;
3. while still at the safe initial system clock, replace only the CLKDIV field
   in `qmi_hw->m[0].timing`, preserving every other timing bit;
4. perform a volatile dummy XIP read, then `__dsb()`; use the SDK's portable
   synchronization definitions so ARM and Hazard3 both receive a real fence;
5. read the divider field back and refuse to raise `clk_sys` on mismatch;
6. only then call `set_sys_clock_pll()`;
7. after clock change, re-read the divider, actual system clock, and derived
   QMI SCK before initializing the test/benchmark path.

Place the helper itself in SRAM with the SDK section macros. Do not change
RXDELAY, command format, pad drive, XIP cache policy, USB clock, peripheral
clock, or other QMI timing fields in this experiment.

If configuration fails, keep or return the clock to its safe value before
printing `FAULT type=qmi_clock` where practical. Include requested divider,
read-back divider, requested system clock, actual system clock, and calculated
QMI SCK in the fault record.

### 3.3 Identity and host gates

Extend `BOOT` and strict host validation with:

- `requested_qmi_clkdiv`;
- `qmi_clkdiv` read back from `QMI_M0_TIMING`;
- `qmi_sck_hz`;
- a profile label that distinguishes the divider experiment.

Add monitor tests for correct divider 3/4/5 records, divider mismatch, invalid
zero/out-of-range values, inconsistent QMI SCK, and a QMI SCK above the campaign
limit. Run all host tests and build both architectures warning-free before any
flash.

### 3.4 Infrastructure controls

Before 420 MHz:

1. clean-build both architectures at 150 MHz/1.10 V/divider 3;
2. flash Hazard3 and require a normal stock pass, including divider read-back 3
   and 50 MHz QMI SCK;
3. optionally build 420 MHz/divider 3 without flashing to prove the wrapper's
   130 MHz guard rejects its 140 MHz QMI SCK;
4. archive the new recovery UF2 files and their hashes.

## 4. Decisive divider experiment

Run `420 MHz / 1.20 V / QMI divider 4` first. Its QMI SCK is 105 MHz, lower
than the already proven 116 MHz at 348 MHz/divider 3.

The point passes only if:

- ARM and Hazard3 both build warning-free before flash;
- BOOT reports exact 420 MHz, requested/read-back 1.20 V, divider 4, and
  105 MHz QMI SCK;
- all eight firmware test suites pass;
- the optimized oracle passes all 4,096 cases;
- all 13 mining-decision cases pass;
- standalone benchmarks and at least nine synchronized windows complete;
- no `FAULT`, reset, sequence discontinuity, USB loss, or strict-monitor error
  occurs.

Because this point overturns the prior divider-3 failure boundary, repeat a
passing Tier-B measurement once before proceeding. Record first-seven medians,
dispersion, standalone rates, H/s/MHz, source identity, UF2 hashes, and complete
logs for both attempts.

Failure branches:

1. On an ambiguous boot/link failure, recover stock and retry the byte-identical
   point once.
2. If 420/1.20/divider-4 fails twice, run 396/1.20/divider-4 as a diagnostic.
3. If 396 also fails, diagnose divider setup/order; do not raise voltage.
4. If 396 passes but 420 fails, treat core voltage as the likely boundary and
   test 420 at 1.25 V, then 1.30 V, one selector at a time. Keep oracle gating.
5. Do not jump directly to 1.60 V and do not resume divider-3 bisection.

## 5. SRAM-residency experiment

Do this only after divider 4 has a validated 420 MHz baseline. It is a separate
source experiment and commit so divider effects are not confused with placement
effects.

### 5.1 Current placement facts

The current Hazard3 map shows:

- `mining_worker_core1` executes from XIP at `0x100018ae`, size `0x420`
  (1,056 bytes);
- the software SHA compression/header-tail/exact-filter functions already
  execute from main SRAM;
- `software_sha256_digest_high_word_after_round61` is in SRAM, but the 256-byte
  SHA round-constant table and 32-byte initial-state table remain in XIP;
- scratch X is 4 KiB and reserves 2 KiB for the core-1 stack, leaving 2 KiB;
  the current 1,056-byte worker therefore fits with 992 bytes of nominal space.

Thus “move the filter to SRAM” is partly already true, but its constant loads
still touch flash. The placement experiment must move those tables too before
claiming that steady-state filter execution is independent of XIP.

### 5.2 Candidate placement

1. Under an explicit Hazard3-only build flag, place the complete
   `mining_worker_core1` function in scratch X below the reserved core-1 stack.
2. Place `sha256_round_constants` and `sha256_initial_state` in initialized main
   SRAM with `__not_in_flash(...)`.
3. Leave ARM unchanged.
4. Do not move the oracle table, strings, USB/reporting code, startup code, or
   rare share/fault handlers merely to claim a whole-image SRAM build.
5. Do not combine this with compiler, DMA, batching, algorithm, or report-cadence
   changes.

Static preflight must prove:

- worker start/end are wholly within scratch X and below `__StackOneBottom`;
- at least 512 bytes remain between worker end and the core-1 stack boundary;
- both SHA tables are at `0x200...`, not `0x100...`;
- the normal per-nonce worker path has no call or data load into XIP;
- any XIP references are limited to startup or rare report/share/fault paths;
- both architectures fit and build warning-free.

Compare XIP and SRAM candidates at the same 420 MHz/1.20 V/divider-4 point in
alternating order. Retain SRAM placement only if both attempts pass every gate,
aggregate and component medians regress by no more than 0.5%, dispersion stays
below 1%, and the static XIP-independence claim holds. A clear regression means
scratch/stack or inter-core SRAM contention outweighs the benefit; restore XIP
worker placement and continue with divider control alone.

If retained, repeat the accepted 420 MHz baseline under the final placement
before advancing. If rejected, restore and require exact restored build hashes
before advancing.

## 6. Re-walk the Hazard3 frequency axis

All listed clocks are exact PLL points in the repository catalog.

| Order | System clock | Divider | QMI SCK | Purpose |
|---:|---:|---:|---:|---|
| 1 | 420 MHz | 4 | 105 MHz | decisive divider test at 1.20 V |
| 2 | 444 MHz | 4 | 111 MHz | resume upward walk |
| 3 | 480 MHz | 4 | 120 MHz | old reproduced failure, now below flash limit |
| 4 | 516 MHz | 4 | 129 MHz | highest divider-4 point under limit |
| 5 | 516 MHz | 5 | 103.2 MHz | paired divider-transition control |
| 6 | 528 MHz | 5 | 105.6 MHz | first point beyond divider-4 region |
| 7 | 552 MHz | 5 | 110.4 MHz | approach user anchor |
| 8 | 570 MHz | 5 | 114 MHz | explicit user anchor |
| 9 | 588 MHz | 5 | 117.6 MHz | highest exact point under 590 MHz cap |

At each new frequency:

1. start with the minimum selector that passed the previous frequency;
2. require the complete 4,096-case oracle before accepting any benchmark;
3. on failure, retry the identical image once after recovery;
4. after two matching computational failures, increase voltage by one selector
   step and repeat; do not alter clock and voltage in the same comparison;
5. record a minimum voltage only after it passes Tier B and the next lower
   selector has failed twice at that exact clock;
6. repeat points within 2% of peak, every voltage transition, 570 MHz, and the
   highest passing clock;
7. after every 1.50 or 1.60 V run, return to stock and validate recovery before
   continuing.

The 516 MHz divider-4/divider-5 pair isolates throughput sensitivity to flash
SCK. If SRAM placement is retained and steady-state rates differ materially,
inspect remaining XIP references and cache behavior rather than assuming core
instability.

## 7. Voltage and safety interpretation

- Up to 1.30 V uses the normal SDK voltage limit.
- 1.35 V and above uses the unsafe-voltage unlock and must be labelled
  `unsafe-overvoltage` in BOOT, logs, ledger, and chat.
- Short captures above 1.30 V and sustained qualification runs are different
  risk profiles. Do not run a 30-minute qualification at 1.50/1.60 V merely
  because a short Tier-B capture passed.
- Keep clock changes separate from voltage changes and source/placement changes.
- Cooling observed by the user does not replace correctness checks.

## 8. Evidence, commits, and stopping rules

For each attempt, including rejected variants and empty logs:

1. archive the complete serial log under a name containing architecture,
   voltage, clock, divider, stage, and attempt;
2. record configuration, source identity, actual telemetry, UF2 SHA-256, log
   SHA-256, outcome class, and rates in `perf_progress.md` immediately;
3. report every intermediate hardware result in chat;
4. update `.codex/HANDOFF.md` after each meaningful boundary or recovery;
5. commit between functional attempts as required by the repository workflow.

Stop and diagnose rather than continuing upward if:

- divider read-back or derived QMI SCK is wrong;
- the oracle fails, even when boot and benchmarks begin normally;
- a point produces a reset, sequence break, `FAULT`, or unexplained dispersion;
- rates exceed the linear projection suspiciously enough to suggest accounting
  or timer error;
- recovery no longer returns to the stock baseline within 1%.

The campaign ends with separate answers for highest correct hashrate, highest
passing clock, minimum selector at important clocks, and a conservative profile.
Only final Pareto candidates proceed to long qualification, and unsafe-voltage
profiles require an explicit decision before sustained qualification.
