# Adaptive clock/voltage benchmark plan

## 1. Objective and scope

Systematically benchmark the currently retained miner on both RP2350 CPU
architectures while varying system clock and core-regulator setting. Find:

1. the highest correctly validated aggregate Bitcoin double-SHA-256 rate;
2. the highest validated clock at or below the test ceiling;
3. the minimum requested regulator setting needed at each important clock;
4. where throughput stops scaling linearly, even if the clock can go higher;
5. a fast profile and a more conservative profile for each architecture.

This is an adaptive boundary search, not a Cartesian sweep. It should spend
most device time near stability transitions and likely throughput maxima.

Planning only: creating this document does not change firmware, voltage, clock,
or the connected board.

## 2. Fixed baseline and limits

- Start from retained candidate 114, source commit `b54dd2d`, evidence commit
  `f005c31`, source identity `0a0357882807`.
- Use the current best architecture-specific implementation already selected by
  the repository: ARM factor-4 hardware batching and Hazard3's DMA trigger plus
  rotated-role exact filter. Do not tune algorithms during this sweep.
- Stock reference at 150 MHz:
  - ARM: 360,308 H/s aggregate, 328,871 hardware, 31,438 software.
  - Hazard3: 369,844 H/s aggregate, 338,331 hardware, 31,510 software.
- Temperature acquisition stays disabled. The user is cooling and observing
  the board. Do not reintroduce the known-bad internal-temperature path.
- Hard test limits for this campaign: **590 MHz** requested system clock and
  **1.60 V** requested regulator selector. Never select 1.65 V or higher.
- Regulator values above 1.30 V require the Pico SDK's explicit unsafe-voltage
  unlock. Label every such image and result `unsafe-overvoltage`; never make it
  the repository default.
- A requested voltage is not a physical voltage measurement. Report it as the
  requested/read-back regulator selector, not as measured VCORE.
- Never alter OTP, boot security, flash partitions, or persistent machine
  configuration. Every clock/voltage choice remains a build-time, reversible
  firmware profile.
- The user's external results, Hazard3 1.60 V at 570 MHz yielding 1.261 MH/s
  and another implementation at 1.50 V/488 MHz yielding about 1.052 MH/s, are
  search anchors and sanity checks, not validation of this firmware.

The 590 MHz cap deliberately respects the highest point already demonstrated
by the user. Any later search above it needs a new plan and explicit approval.

## 3. Why a full matrix is wasteful

The supported voltage selectors through this campaign are:

`1.10, 1.15, 1.20, 1.25, 1.30, 1.35, 1.40, 1.50, 1.60 V`.

A naive 9-voltage by dozens-of-clock grid would repeat obviously dominated
points. Stability normally has a monotone envelope: as clock increases, the
minimum stable voltage usually stays equal or rises. Trace that envelope. At a
given clock, once the minimum stable selector is bracketed by a failing lower
value and a passing higher value, do not run all higher voltages. At a given
voltage, do not retest clocks already disproved unless a repeat is needed to
classify a noisy failure.

The two architectures get separate frontiers. Do not assume an ARM result is
valid for Hazard3, or vice versa, even though they share the SHA peripheral.

## 4. Phase 0 — make experiments identifiable and recoverable

Implement this infrastructure as one standalone commit before any overclock
measurement. It is measurement infrastructure, not a hashing optimization.

### 4.1 Build parameters

1. Add `MINER_VREG_MV`, defaulting to `1100`.
2. Accept only the explicit selector list above. Reject every other value in
   the wrapper and CMake configuration.
3. Raise `tools/build`'s clock ceiling from 550,000 to exactly 570,000 kHz.
4. Link `hardware_vreg` and map each allowed millivolt value to the matching SDK
   `VREG_VOLTAGE_*` enum. Do not write POWMAN registers directly.
5. For values above 1.30 V only, call `vreg_disable_voltage_limit()` before
   `vreg_set_voltage()`. Do not globally disable the limit for lower profiles.
6. Apply voltage before increasing the PLL clock, then wait at least the SDK
   settling delay. If clock configuration fails, lower the clock/restore the
   default selector before entering the visible fault loop where practical.
7. Keep USB at 48 MHz. Do not modify algorithm, compiler options, XIP/QMI
   divider, report interval, or temperature behavior in this commit.

### 4.2 PLL catalog

Do not request arbitrary round numbers. Add a read-only helper that enumerates
exact frequencies accepted by the SDK's RP2350 `check_sys_clock_khz` rules for
the installed 12 MHz crystal, legal VCO range, and post-dividers. For every
candidate record requested clock, exact realized clock, VCO, post-divider 1,
and post-divider 2.

This prevents repeating the historical 350 MHz mistake: that run failed in
clock synthesis before KATs and did not demonstrate voltage instability.
When an attractive number such as 488 or 570 MHz is exactly synthesizable,
include it. Otherwise choose the nearest lower and upper realizable points and
record the substitution.

### 4.3 Boot identity and logs

Extend `BOOT` with:

- requested voltage in mV;
- regulator selector read back after settling;
- `unsafe_voltage_limit_disabled=0|1`;
- requested and actual `clk_sys`;
- PLL VCO and both post-dividers;
- `clk_usb`, `clk_peri`, and any available QMI/XIP divider state.

Extend the monitor contract so a mismatch in architecture, source identity,
requested voltage, actual clock, or profile is a hard failure. Add host parser
tests before hardware use.

Use experiment names of the form:

`OC-<arch>-v<mV>-f<kHz>-<stage>-<attempt>`

Archive the complete serial log plus UF2 SHA-256 for every point, including
clock-synthesis failures, timeouts, resets, and rejected variants. Append each
result immediately to `perf_progress.md` and report it in chat. Commit the
infrastructure before the first flash, and commit logged evidence at each
completed frontier step or tightly related pair.

### 4.4 Recovery preparation

1. Clean-build both architectures at 150 MHz/1.10 V.
2. Archive both recovery UF2 files and their hashes outside the overwritten
   per-architecture build output but inside this repository.
3. Flash and validate the stock Hazard3 recovery image once.
4. Confirm `./tools/flash` can return from an experimental image to stock.
5. At every test point, build both ISAs before flashing, as required by
   `AGENTS.md`.
6. If USB disappears, allow one bounded automatic rediscovery/reflash attempt.
   Then stop and report that a physical reconnect/BOOTSEL recovery is needed;
   do not loop flashes indefinitely.

## 5. Outcome classification

Each attempted `(architecture, voltage, clock)` point must end in exactly one
class:

- `SYNTHESIS_REJECT`: PLL catalog/API cannot produce the requested clock. This
  says nothing about voltage stability.
- `BOOT_FAIL`: no valid matching `BOOT` record before timeout.
- `CORRECTNESS_FAIL`: any `TEST:FAIL`, `FAULT`, digest/oracle mismatch, invalid
  candidate, protocol error, or nonzero wrapper status.
- `RESET_OR_LINK_FAIL`: unexpected run-ID change, USB loss, truncated stream,
  watchdog/reset evidence, or missing required reports.
- `PERFORMANCE_ANOMALY`: correct run, but normalized H/s per MHz drops more
  than 3% from the local trend or window dispersion exceeds 1%. Repeat once to
  distinguish host/report noise from a real memory/clock bottleneck.
- `SCREEN_PASS`: correctness plus a short stable measurement.
- `MEASURE_PASS`: full measurement protocol passes.
- `SOAK_PASS`: qualification protocol passes.

A hang or missing serial output is a failure, not zero H/s and not permission
to raise voltage automatically. First retry the identical point once. Only a
reproducible computational/boot failure may move the boundary search upward in
voltage.

## 6. Measurement tiers

### Tier A — screen

Use only while locating a boundary:

1. Cold or explicit reset into the exact image.
2. Validate BOOT identity and actual clocks/selectors.
3. Run all firmware KATs, including the 4,096-case cross-engine oracle and the
   13 mining-decision cases.
4. Capture at least three complete synchronized mining windows after standalone
   benchmarks.
5. Reject on any failure class above.

### Tier B — measure

For every new minimum-voltage frontier point and every local throughput leader:

1. Run the normal complete capture with nine synchronized windows.
2. Use the first seven windows for the primary median, matching current ledger
   practice; retain all nine in the log.
3. Record aggregate, hardware, software, standalone hardware, full software,
   and exact-filter H/s.
4. Record min/max/range and median absolute deviation for aggregate windows.
5. Calculate H/s per MHz for all three sustained rates and scaling efficiency:
   `measured_rate / (150_MHz_rate * actual_MHz / 150)`.
6. Repeat Tier B once for points within 2% of the current best hashrate, points
   defining a voltage transition, and the highest passing clock.

### Tier C — qualify

After the frontier is known, qualify only the final Pareto candidates:

- highest measured aggregate H/s per architecture;
- highest passing clock per architecture, if different;
- lowest-voltage point within 2% of peak H/s;
- one conservative point with at least one voltage step and one clock step of
  observed margin.

Run each for 30 minutes with periodic KAT/oracle rechecks at start and end,
continuous sequence/reset checking, bounded serial capture, and all mining
windows retained. Repeat the absolute winner after returning to stock for a
control run, eliminating simple warm-up/order bias.

## 7. Adaptive frontier-search algorithm

Run Hazard3 first because it is the present stock-clock winner and has the
user's 570 MHz anchor. Then run ARM using the information only to choose useful
starting brackets; ARM still needs its own evidence.

### 7.1 Establish fresh controls

For each architecture:

1. Measure candidate 114 at 150 MHz/1.10 V and require agreement within 1% of
   the retained baseline.
2. Revalidate 300 MHz/1.10 V. Historical firmware passed there nearly linearly,
   but the current candidate needs fresh evidence.
3. If 300 MHz fails, stop the upward search and diagnose infrastructure; do not
   compensate with voltage because that contradicts the existing evidence.

### 7.2 Trace the minimum-voltage envelope

Maintain `last_stable_clock`, `minimum_stable_voltage`, and a set of known
failing lower-voltage bounds for each architecture.

Choose the next exact PLL point adaptively:

- below 420 MHz: target roughly +48 MHz;
- 420–500 MHz: target roughly +24 MHz;
- above 500 MHz: target roughly +12 MHz;
- always include the nearest exact points around 488 MHz and exactly 570 MHz
  when the PLL catalog permits them;
- after any failure, bisect between the last passing and first failing exact
  PLL points until the interval is at most 6 MHz or has no untested point.

At a new clock:

1. Start at the minimum selector that passed the preceding clock.
2. If it passes Tier A, run Tier B. Do not test higher voltages there.
3. If it fails, repeat the identical point once.
4. On two matching failures, choose a higher selector using the known bracket.
   If no bracket exists, initially jump two selector indices; once a pass is
   found, binary-search the skipped selector indices.
5. The selected minimum is valid only when it passes Tier B and either the next
   lower selector fails twice at the same clock or a lower failure is already
   established there.
6. Continue upward from that selector. Never restart each clock at 1.10 V.

This staircase needs approximately one pass per chosen frequency plus only the
voltage-boundary failures, rather than every voltage at every frequency.

### 7.3 Use the user's anchors efficiently

- Hazard3: after fresh 300 MHz control and one or two intermediate frontier
  points, test 570 MHz/1.60 V early as a cap/anchor. If it passes, 590 MHz is
  the campaign's maximum clock by definition; If it fails,
  repeat once, then bisect frequency downward at 1.60 V.
- ARM: include an exact point at or adjacent to 488 MHz/1.50 V early. If it
  passes, use it as the upper bracket for minimum-voltage search and probe
  upward adaptively toward 590 MHz. If the user's 488 MHz result belonged to a
  different architecture, it remains merely a useful performance anchor.

Do not begin directly with the anchors before validating infrastructure at
stock and 300 MHz.

### 7.4 Locate maximum hashrate, not merely maximum clock

For every Tier-B pass, compare aggregate H/s and normalized H/s/MHz with its
two neighboring frontier points. Clock remains worth increasing while:

- aggregate median improves by at least 0.5%; and
- normalized efficiency does not suffer a repeatable drop greater than 3%.

If a higher stable clock gains less than 0.5%, repeat both neighbors in
alternating order. If the plateau is confirmed, stop spending time on dense
interior points; retain the lower-voltage/lower-clock point unless the higher
one still wins absolute H/s. The reported answers must distinguish:

- highest briefly booted clock;
- highest Tier-B validated clock;
- highest Tier-C qualified clock;
- highest measured hashrate and its clock/voltage;
- lowest requested voltage for each retained point.

## 8. Expected-rate sanity checks

If candidate 114 scaled perfectly from 150 MHz, it would predict roughly:

- Hazard3 at 570 MHz: `369,844 * 570 / 150 = 1.405 MH/s` aggregate;
- ARM at 488 MHz: `360,308 * 488 / 150 = 1.172 MH/s` aggregate.

The user's other-firmware measurements are lower, so some loss at high clock is
plausible. These calculations are diagnostics, not acceptance targets. A rate
materially above linear scaling is suspicious and requires hash-accounting and
timer verification. A falling H/s/MHz trend may reveal XIP/QMI, DMA, peripheral,
or reporting contention rather than inadequate core voltage.

## 9. Keep third variables out of the primary matrix

The primary sweep changes only clock and requested VREG selector. Keep source,
compiler, algorithm, USB clock, report cadence, XIP/QMI settings, and cooling
arrangement fixed.

If correct high-clock points show a sharp repeatable efficiency collapse,
finish and commit the primary frontier first. A separate follow-up experiment
may compare one documented QMI/flash divider or SRAM placement at only the
affected point and its lower-clock control. Treat that as a third-variable code
experiment with its own plan and commits; never silently mix it into the
clock/voltage frontier.

Likewise, do not combine voltage testing with compiler flags, LTO, worker batch
changes, DMA changes, or algorithmic work.

## 10. Failure handling and stopping rules

1. On `TEST:FAIL`, `FAULT`, wrong digest, wrong BOOT identity, unexpected reset,
   monitor timeout, or nonzero command status, reject the run immediately.
2. Retry an ambiguous failure once at the identical image and settings.
3. For a reproduced high-clock failure, return to the last passing point before
   attempting a new voltage. Never alter voltage and clock simultaneously when
   resolving a boundary.
4. After any lost USB connection, attempt bounded automatic rediscovery once.
   If absent, pause for user-assisted physical recovery.
5. After any 1.50/1.60 V run, return to the 150 MHz/1.10 V recovery image and
   perform a short correctness check before changing architecture or ending the
   session.
6. Stop an architecture when 590 MHz passes, or when 1.60 V has a reproduced
   failure and frequency bisection identifies the highest passing PLL point.
7. Never infer that more voltage would fix a synthesis rejection, USB-host
   problem, software fault, or performance plateau.

## 11. Result ledger

Maintain a compact table in `perf_progress.md` and update it after every result:

| Field | Required value |
| --- | --- |
| Experiment | ID and attempt number |
| Firmware | commit, source ID, UF2 SHA-256 |
| Target | architecture and RP2350B/package selector evidence |
| Voltage | requested mV, selector readback, unsafe-limit flag |
| Clock | requested/actual kHz, VCO, post-dividers, USB/peri clocks |
| Validation | KAT count, oracle cases, decision cases, monitor status |
| Performance | all standalone rates; per-window and median aggregate/hardware/software H/s |
| Scaling | H/s/MHz and efficiency versus 150 MHz and previous point |
| Stability | duration, retries, resets, USB loss, faults |
| Outcome | one classification from section 5 |
| Artifacts | archived log and hashes |

Also maintain two derived summaries per architecture:

1. minimum stable requested voltage versus clock (the frontier);
2. aggregate hashrate versus clock along that frontier.

Rejected and failed points stay in the ledger. Do not rewrite them as if they
were never attempted.

## 12. Recommended execution order

1. Commit Phase-0 voltage/PLL/identity infrastructure.
2. Build both architectures and run host parser tests.
3. Create and validate stock recovery artifacts.
4. Hazard3 fresh controls: 150 MHz, then 300 MHz at 1.10 V.
5. Hazard3 adaptive frontier, including early 570 MHz/1.60 V anchor after the
   controls and intermediate checks.
6. Hazard3 Tier-C qualification of only its Pareto candidates.
7. Return to stock and validate recovery.
8. ARM fresh controls: 150 MHz, then 300 MHz at 1.10 V.
9. ARM adaptive frontier, including the 488 MHz/1.50 V anchor.
10. ARM Tier-C qualification.
11. Return to stock and validate recovery.
12. Compare both architecture winners in alternating A/B/A order to reduce
    order and warm-up bias.
13. Commit final logs, ledger, frontier tables, and a concise handoff.

## 13. Acceptance criteria

The campaign is complete only when:

- both architectures have fresh stock controls and an adaptively traced
  voltage/clock frontier;
- every accepted point passed complete correctness checks and has an archived
  serial log and reproducible firmware identity;
- voltage-transition claims have a failing lower selector and passing selected
  value at the same clock;
- peak-hashrate and maximum-clock claims are clearly separated;
- final winners pass Tier C and a repeat after returning through stock;
- the board is left on a validated profile, preferably the stock recovery
  image unless the user requests otherwise;
- `perf_progress.md`, `.codex/HANDOFF.md`, and git history are sufficient for a
  less advanced model to resume without repeating points.
