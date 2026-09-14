# RP2350A Bitcoin miner: optimization roadmap

Date: 2026-09-13. Status: **plan only; none of the experiments below was implemented or measured while writing this document.**

Execution override, 2026-09-14: the user requested that temperature extraction
be skipped after every ADC channel failed, and authorized experimental clocks up
to 550 MHz. Runtime measurements must say `temperature=disabled`. Establish code
changes at 150 MHz first and test clock changes separately in increasing steps;
550 MHz is a ceiling, not a required or presumed-stable target.

The connected chip is now treated as **RP2350A / QFN-60**. This supersedes the original RP2350B assumption: direct firmware readout reported `SYSINFO.PACKAGE_SEL=1`, which the RP2350 datasheet defines as QFN-60. The exact carrier-board model, supply/reference arrangement, and silicon stepping still matter. Package suffix A is distinct from the reported silicon revision `3`.

Objective: maximize **correct, unique Bitcoin double-SHA-256 nonce evaluations per second**, with working USB control, reproducible validation, and valid temperature telemetry. Retain both `rp2350-arm-s` and `rp2350-riscv` builds. Report hash-kernel throughput separately from sustained mining throughput and from any microbenchmark.

## 1. Starting point and evidence

Inspected repository revision: `64c341c`. The working tree was clean before this document was added. The following are historical results from [perf_progress.md](perf_progress.md), not new measurements:

| Historical variant | Architecture / system clock | Benchmark H/s | Sustained mining H/s | Interpretation |
| --- | --- | ---: | ---: | --- |
| Original SDK CPU-fed SHA API | M33 / 150 MHz | 34,427; confirmed 34,441 | 34,343 in initial run | Reference, not the current implementation |
| Direct padded blocks, per-word polling | Hazard3 / 150 MHz | 86,300 | 85,675–85,695 | Older kernel; not a fair comparison with today's ARM kernel |
| Unrolled, inlined, `-O3`, lazy final-result reads | M33 / 150 MHz | 294,091 | 289,328–289,501 | Best established stock-clock kernel before telemetry |
| Entire image copied to SRAM | M33 / 150 MHz | 294,093 | 289,323–289,503 | No measurable benefit; already rejected and reverted |
| Same optimized kernel, overclocked | M33 / 200 MHz | 392,124 | 385,603–385,984 | Short functional test, not thermal/reliability qualification |
| Temperature telemetry added | M33 / 200 MHz | 392,125 | 383,728–384,367 | Temperature readings invalid |
| Current temperature diagnostic variant | M33 / 150 MHz; ADC 24 MHz | 294,090 | Re-measure after board correction | Raw ADC 4095 plus error flags; temperature invalid |

Current code facts, from [src/main.c](src/main.c), [CMakeLists.txt](CMakeLists.txt), and [tools/build](tools/build):

- One CPU owns the one SHA peripheral. Each nonce uses two blocks for the 80-byte header, then one block for the 32-byte intermediate digest: **three hardware compressions per nonce**.
- Existing optimizations already include pre-padding, aligned nonce updates, 16 unrolled writes per block, readiness polling once per block, inlining, `-O3`, and reading only the necessary final-result words. Do not present these as new ideas.
- Eight intermediate digest words are byte-swapped and copied through RAM. The benchmark performs a volatile byte checksum and 64-bit accounting. Mining does target checks, 64-bit accounting, ADC sampling, and USB printing on the same CPU.
- Current system-clock default is 150 MHz. The benchmark lasts approximately two seconds; mining reports every 100,000 attempts. Startup has a fixed 3.5-second USB enumeration delay.
- Both the CMake default and build wrapper select `PICO_BOARD=pico2`. SDK 2.3.1's `boards/pico2.h` defines `PICO_RP2350A=1`, matching the hardware package register. Temperature is therefore on ADC channel 4.
- The wrapper specifies `Debug`, while the target adds `-O3`. Inspect actual compile/link commands, not just the build-type label. The historical log's introductory stock-clock claim and initial `Release` label are not reliable descriptions of every later experiment.
- Current tests cover SHA empty/`abc`, genesis hashing, and genesis nonce search. The first two use the SDK path, not the optimized Bitcoin kernel. More coverage is needed before delicate optimizations.
- `tools/monitor.py --require-pass` currently accepts a single health line. It does not require a complete test summary, the expected benchmark, or valid temperatures. A printed `CYCLE:PASS` alone is insufficient evidence.

## 2. Non-negotiable hardware and algorithm constraints

### 2.1 Package, memory, CPUs, and instruction sets

RP2350A has 30 package GPIOs and five ADC mux inputs including the temperature sensor. It has two active processor sockets, not four simultaneously active CPUs. Homogeneous M33 or Hazard3 operation is the normal SDK workflow; mixed M33/Hazard3 operation is a separate advanced boot experiment. SRAM is 520 KiB: two 256 KiB striped groups plus two 4 KiB scratch banks. RP2040 unstriped aliases must not be copied into an RP2350 linker script. See the [RP2350 datasheet, sections 2.2.3 and 3.9](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

Use the installed SDK's CPU configuration. Its RISC-V toolchain selection tries `-mcpu=hazard3-rp2350`, then supported `-march` alternatives. Useful RP2350 Hazard3 extensions include Zba, Zbb, Zbs, Zbkb, Zcb, and Zcmp. Zbkb is **not** a SHA-256 round instruction extension. Do not enable Zknh, RVV, or optional upstream Hazard3 features absent from this silicon. M33 has useful scalar rotate/byte-reverse instructions, but not NEON/MVE or Arm application-profile SHA instructions. Verify generated instructions and register pressure rather than guessing from architecture names. Consult the [Hazard3 implementation documentation](https://github.com/Wren6991/Hazard3) and installed SDK toolchain files listed in section 9.

### 2.2 SHA state and scheduling

The official [SHA register definitions](https://raw.githubusercontent.com/raspberrypi/pico-sdk/master/src/rp2350/hardware_regs/include/hardware/regs/sha256.h) establish these constraints:

- `SUM0`–`SUM7` are read-only. There is **no supported hardware midstate restore**. Reading a midstate is not enough to resume it after another message.
- `START` resets the chaining state and counters. Save the entire first digest before starting the second hash.
- `START` immediately makes `SUM_VLD` high. A valid flag before submitting the intended data does not mean the intended hash has completed.
- SHA DREQ describes block feeding; DMA completion does not imply compression completion. Wait for the appropriate SHA status before reading results.
- There is no input FIFO permitting arbitrary writes during compression. Preserve block boundaries and 32-bit transfer sizing.
- Read result registers only when valid; never exploit undocumented invalid-result contents or deliberately write while not ready.

### 2.3 Throughput budget

The documented minimum per block is 16 APB word writes at four cycles each, followed by 57 compression cycles: **121 system cycles/block**. Source: [RP2350 datasheet, section 12.13.2](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

The following are calculated bounds, **not benchmarks**:

```text
three-block hardware-only lower bound = 3 × 121 = 363 cycles/nonce
ideal ceiling at 150 MHz              = 150,000,000 / 363 = 413,223 H/s
ideal ceiling at 200 MHz              = 200,000,000 / 363 = 550,964 H/s
historical 150 MHz implementation     = 150,000,000 / 294,091 ≈ 510 cycles/nonce
```

Reset writes, intermediate-result reads, polling, target checks, and software overhead reduce achievable throughput. Thus even the ideal stock-clock ceiling is only about 40.5% above the historical 294,091 H/s. Treat a claimed hardware-only result above this ceiling as a counting/timing/configuration defect until explained. Independent software hashing on a CPU or a different hybrid algorithm has a different bound; label it explicitly.

## 3. Execution rules for the implementing agent

This section describes **future work**, not commands to run while preparing the plan.

1. Read `AGENTS.md`, this document, and the latest performance log. Inspect `git status`; preserve all unrelated user changes.
2. Implement one experiment/subvariant at a time using `apply_patch`. Keep a selectable, validated reference path. Do not combine a clock change with a kernel change.
3. All generated sources, local board definitions, scripts, and artifacts belong inside this repository. Never edit the installed SDK or toolchain. Ask the user before installing anything not already available.
4. Use repository wrappers. Do not invoke compiler binaries directly. For future profiling/disassembly, add a repository wrapper that discovers the selected toolchain and invokes its matching analysis utilities.
5. After a shared-source change, build both architectures, resolve all warnings, then flash and capture each architecture being evaluated. Never flash a shared-source variant with one build failing.

   ```bash
   ./tools/build arm
   ./tools/build riscv
   MONITOR_SECONDS=20 ./tools/cycle arm
   MONITOR_SECONDS=20 ./tools/cycle riscv
   ```

   `cycle` already rebuilds both before flashing its selected architecture. The explicit builds are useful preflight checks. Twenty seconds suits startup/smoke tests, not the full measurement protocol. Use bounded captures of at most 60 seconds per interactive tool wait, with updates between longer-run captures. Do not run two USB monitors or two flashing operations concurrently.

6. Treat `TEST:FAIL`, `FAULT`, unexpected reset, missing expected output, timeout, invalid benchmark temperature, or nonzero command status as failure. Never mine after a latched validation failure. Keep hash correctness and thermal validity as separate reported fields.
7. **For every newly obtained intermediate measurement**, promptly print its result in chat and append it to `perf_progress.md`, including unsuccessful, slower, and thermally invalid trials. Include an experiment ID and raw-log path. Do not wait until finding a winner. Bounded batches of related samples may be summarized together, but preserve each sample in the log.
8. Archive `logs/arm-latest.log` / `logs/riscv-latest.log` under unique names before another cycle overwrites them. Record firmware digest and exact configuration so logs cannot be attributed to a different binary.
9. Revert only the agent's rejected experiment edits, not unrelated work. Rebuild both and revalidate the retained baseline after reverting. Do not use destructive Git cleanup.
10. No OTP, erase, boot-security, flash-partition, machine-configuration changes, or `sudo`. Preserve USB recovery. Board replacement, cooling modifications, extra instruments, and broader voltage experiments require user involvement where indicated below.

### Experiment record template

Append a filled-in version to `perf_progress.md`; angle-bracket values below are placeholders, not results:

```text
Date/time/timezone; experiment=<E04-b>; run=<unique ID>
Hypothesis and exact change; parent baseline ID
Board=<confirmed model or unknown>; package=RP2350A; silicon_revision=<observed/unknown>
Architecture; SDK/toolchain versions; effective compile/link flags
Source revision plus patch identity; firmware SHA-256; archived build/serial logs
Actual sys/peri/ADC/USB clocks; flash clock/divider; requested regulator setting
Workload/header/target IDs; nonce ranges; warm-up; logging/sampling cadence
Validation: required KAT counts; oracle matches; target tests; SHA/ADC errors
Per window: complete unique hashes; elapsed_us; kernel_H/s; mining_H/s
Temperature: valid; start/end/min/max; raw mean/min/max; sample count/time
ADC reference assumption/calibration; ADC error flags; ambient if measured
Compared with baseline: median delta; variability; code/SRAM/stack cost
Decision: retain / reject / inconclusive / blocked, with reason and next step
```

Use `temperature_valid=0 temperature_c=NA` for invalid readings, retaining raw/error diagnostics. Do not publish an impossible converted value as a real temperature. CPU regulator voltage is a requested setting unless independently measured; ambient and power are unknown unless actually measured. A stable ADC value is not proof of accuracy.

## 4. Ranked experiment sequence

Potential below is a hypothesis about opportunity, not a promised gain. Complete E00–E02 before evaluating speed improvements. The implementing agent should finish one ID and report before starting the next major phase.

| ID | Work | Prerequisites | Opportunity / effort | Stop or advance rule |
| --- | --- | --- | --- | --- |
| E00 | Confirm RP2350A board and temperature configuration | Board information | Required correctness / small–medium | No thermal qualification until valid |
| E01 | Strong oracle, benchmark harness, failure gates | E00 | Required confidence / medium | Reject unverifiable results |
| E02 | Matched ARM/RISC-V baseline and cycle profile | E01 | Finds actual bottleneck / small–medium | Rank next variants by measured costs |
| E03 | Endian representation and register-only digest transfer | E02 | Promising low-level savings / small–medium | Keep only correct, repeatable gains |
| E04 | Specialized feeding, scheduling, and accounting | E03 baseline | Small–medium gains / medium | Preserve equal workload and safety |
| E05 | Compiler/code-layout matrix | E02; repeat on winning kernel | Small–medium, architecture-dependent / medium | Inspect assembly; avoid flag soup |
| E06 | Persistent DMA and CPU/DMA combinations | E02 | Uncertain speed, possible CPU release / medium | Compare complete double hashes |
| E07 | Selective memory placement and bus contention | E02; stronger reason after E06/E08 | Low alone, useful under contention / medium | Do not repeat failed full-SRAM test blindly |
| E08 | Dedicated SHA owner plus telemetry/control core | E01; best single-core path | Sustained-rate/jitter improvement / medium | One owner, no lost jobs/shares |
| E09 | Software SHA midstate and specialized rounds | E01, E05 | Enables independent CPU work / high | Must beat generic software reference |
| E10 | Hardware + software workers and hybrid pipeline | E06/E08/E09 | Potential extra aggregate throughput / high | Unique work, end-to-end benefit |
| E11 | Qualified clock/power/thermal sweep | E00/E01; stable chosen kernel | Strong scaling but overclock risk / medium | Explicit limits, abort/recovery, soak |
| E12 | PGO, alternate compiler, limited assembly | Profiling shows remaining CPU cost | Incremental / medium–high | Justify maintenance and installs |
| E13 | Mixed-ISA and unusual platform experiments | E08–E10 evidence | Speculative / very high | Time-box; defer without clear case |
| E14 | Real-job efficiency and final qualification | Stable selected configuration | Useful-work improvement / medium | No stale/duplicate work disguised as speed |

## 5. Detailed work packages

### E00 — Correct board identity and establish trustworthy temperature telemetry

Files to inspect/change later: `CMakeLists.txt`, `tools/build`, relevant `.vscode` configuration, `src/main.c`; optionally a new repository-local board header under `boards/`. Do not change `AGENTS.md` without a specific reason/authorization.

1. Confirm the carrier-board model if possible. Preserve the already working official Pico 2 flash and LED settings, and verify the ADC reference/supply arrangement if temperature remains invalid. USB VID `2e8a` alone does not identify the carrier board.
2. Select the installed `pico2` definition, consistent with the direct QFN-60 package readout. Do not override its package macro from the command line.
3. Keep the selection consistent in wrapper/default/editor configuration.
4. Add compile-time assertions for this target: `PICO_RP2350A == 1`, `NUM_ADC_CHANNELS == 5`, `ADC_TEMPERATURE_CHANNEL_NUM == 4`. Print compile-time and hardware package identity plus channel in `BOOT`. Use the SDK macro in sampling code rather than scattering literal channel numbers; account for LED polarity.
5. Initialize ADC and enable the temperature sensor; select its channel and allow settling. At 150 MHz system clock, establish a normal ADC configuration first. Verify all clock API return values; report actual ADC clock and selected channel. Only retain the 24 MHz diagnostic setting if measurements justify it.
6. Sample 32 conversions, checking conversion error flags rather than merely averaging returned numbers. Clear sticky flags using documented semantics before the acquisition. Reject rail/saturated/error samples. Keep raw sum/count through conversion to avoid prematurely rounding away averaged resolution.
7. Use sufficiently wide arithmetic. With nominal 3.3 V reference, calculate voltage from the averaged 12-bit ADC value, then `T_C = 27 - (V - 0.706) / 0.001721`. Label it uncalibrated/approximate. Store fixed-point millidegrees if useful, but display sensible precision. See the [official ADC API and channel mapping](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_adc).
8. Record startup, idle, and loaded temperatures with raw values and error flags. Require a plausible response over time, not a particular room-temperature number. If possible, compare with an independent thermometer and the actual ADC reference; do not invent calibration from a single assumed ambient value.
9. If channel 4 still fails, check ADC_AVDD/reference wiring, ADC register selection, sensor enable/settling, and documented silicon errata. Stop clock experiments until explained; do not try random channels or voltage writes.
10. Preserve the historical RP2350B/channel-8 attempts as rejected evidence and append the authoritative RP2350A correction. Qualify stale global stock-clock/build-label wording rather than silently rewriting historical measurements.

Acceptance: both builds are warning-free, boot identifies A/channel 4 and hardware `PACKAGE_SEL=1`, all existing KATs pass, temperatures are explicitly valid with no ADC errors under idle and load, and failure injection into the telemetry validator is rejected. A speed increase is not required.

Observed E00 disposition on 2026-09-14: package identity is resolved, but all
five ADC channels return saturated/error results. Per the later user override,
temperature extraction is disabled and E00 is closed as a documented hardware
limitation. Continue performance experiments, explicitly marking temperature
as disabled. This is not a valid-temperature result.

### E01 — Establish an independent oracle and reproducible measurement contract

Proposed additions, **not existing commands yet**: a host oracle/fixture generator under `tools/`, deterministic firmware test mode, benchmark configuration, and stricter parsing in `tools/monitor.py` or a separate results validator.

1. Use host Python `hashlib.sha256(hashlib.sha256(header).digest()).digest()` as an independent byte-level oracle. Preserve exact 80-byte serialization and little-endian nonce insertion at byte offset 76. Keep host computation outside device timing.
2. Keep the four current tests, explicitly asserting genesis nonce `2083236893` is found after 94 attempts starting at `2083236800`. Expected displayed genesis hash is `000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f`.
3. For every optimized kernel, compare complete 32-byte digests for at least 4,096 deterministic, host-generated header/nonce cases. Include zero/ones/random headers and nonces `0`, `1`, `0x7fffffff`, `0x80000000`, `0xfffffffe`, `0xffffffff`, plus values with distinct bytes. Persist the seed, fixture digest, and expected results. Batch fixtures if flash/serial size warrants it.
4. For a new generic software SHA implementation, add padding-boundary lengths 0, 1, 55, 56, 63, 64, 65, 80, 119, and 120 bytes and byte-oriented [NIST SHA test vectors](https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing). NIST vector agreement is testing, not certification. SDK-only tests do not validate a separate fast path.
5. Test target comparison independently: hash below/equal/above target, equality through seven leading words, target zero, very easy target, and difficult target. Use synthetic digests to exercise rare paths; waiting for a naturally occurring rare share is not coverage. Verify compact target negative/zero/overflow and valid boundary cases against [Bitcoin Core `SetCompact`](https://github.com/bitcoin/bitcoin/blob/master/src/arith_uint256.cpp). Do not assume all exponents above 32 overflow regardless of mantissa.
6. Test nonce-range exhaustion, job replacement, and later multicore cancellation. Count only completed unique `(job_id, nonce)` work. Controlled easy targets must exercise full share capture and host verification.
7. Give each binary a configuration/variant ID and each session a run ID. Require the expected `BOOT`, complete `TEST:SUMMARY`, oracle success, and benchmark records for that ID. Detect stale logs, missing records, resets, duplicate windows, and incomplete lines. A normal finite capture endpoint is not failure if all required output arrived; a missing-output deadline is failure.
8. Make benchmark failure propagate to `main`; currently `run_benchmark()` can print `FAULT`, return, and then mining still starts. Latch faults and stop the worker. Test parser failures using saved synthetic logs containing one passing line followed by failure, missing summaries, wrong IDs, and invalid temperatures.
9. Harden serial deadline handling, including setup operations such as `tty.setraw`, not just the read loop. Add an outer bounded process timeout in the future harness. Preserve the proven startup/recovery behavior; a previous DTR-wait experiment lost output.

Measurement contract:

- Establish a fixed-work nonce range long enough for roughly 5–10 seconds at the baseline rate; use the same count/range in each compared variant. Record exact integer counts and elapsed microseconds. Keep a time-based mode only as a separately labeled continuity test.
- Warm the workload for at least 30 seconds, then collect at least five measured windows per configuration. Continue warm-up if temperature is still changing materially. Use paired baseline/candidate ordering, such as A–B–B–A, to expose drift.
- Record median and range, plus a documented variability measure such as median absolute deviation. Initially require a gain of at least 2% and larger than three times observed relative variability to retain extra complexity; smaller gains need stronger repetition and a clear cost argument. This is an engineering screening rule, not a statistical confidence interval.
- Report two modes: **kernel benchmark**, with equal result-consumption/checksum policy; and **sustained mining**, including target checks, required telemetry, and control handling. Optional full-digest microbenchmarks are separate. Changing to a cheaper checksum is not automatically a kernel improvement.
- Never put serial printing, ADC conversion, or host oracle work inside the kernel's timed window. Record boundary temperatures. During long runs, sample periodically outside the worker or between bounded windows; document cadence and overhead. Sustained throughput must include telemetry costs.
- Preserve a deterministic checksum and periodically verify complete digests. If a software rejection kernel evaluates only the information needed for correct rejection, label it as such and validate decisions independently; do not label incomplete hashes as full-digest production.
- At fixed 150 MHz, compare both architectures using identical source features, test sets, nonce work, logging cadence, and thermal conditions. Archive `.elf`, `.uf2`, map/disassembly where available, actual compile commands, and results under one experiment directory.

Acceptance: the harness rejects deliberately malformed results and confirms both architectures against the oracle. No optimization result is accepted until this works.

### E02 — Obtain matched baselines and identify the expensive stages

1. Measure the corrected, otherwise unchanged kernel on both architectures at 150 MHz. The old 86,300 H/s RISC-V result predates several ARM optimizations; do not infer the current architecture ranking from it.
2. Add an optional profiling mode. On M33 use the available DWT cycle counter after checking its enable/access behavior. On Hazard3 use supported cycle/retired-instruction counters. Account for counter wrap and profiling overhead. Use the monotonic hardware timer for end-to-end throughput.
3. Profile repeated batches and selected isolated stages: start/reset; first block feed/wait; tail block feed/wait; eight-word digest transfer; second-hash feed/wait; result/target/error check; accounting/reporting. Counter reads can perturb short stages, so compare instrumented and uninstrumented totals and do not sum intrusive timings as exact truth.
4. Inspect disassembly for MMIO instruction count, spills, extra loads, byte swaps, loop branches, helper calls, and address recalculation. Record code size. Identify whether CPU issue, APB traffic, compression waiting, flash misses, or telemetry dominates.
5. Use bus performance counters and XIP statistics in short diagnostic windows where useful; clear/sample them so saturating counters are not mistaken for complete counts. Record which event was selected. They measure selected bus events, not automatically CPU stall cycles.
6. Produce a cycle-budget table beside the 363-cycle theoretical floor. Select the next experiment based on a measured removable cost; keep instrumentation disabled for final rate comparisons.

Acceptance: two fresh baselines, valid thermal logs, and a bottleneck hypothesis. No need to change performance in this step.

### E03 — Avoid repeated endian conversion and intermediate RAM traffic

Hypothesis: the eight intermediate swaps/stores/loads are avoidable; the best implementation may differ between M33 and Hazard3 because of available registers.

E03-a, preformatted numeric SHA words:

1. Keep the baseline byte-buffer representation behind a variant switch. In a new path, convert invariant header words once at job setup to numeric big-endian SHA words.
2. Set hardware `BSWAP=false` for the whole optimized job. The per-nonce header word becomes `bswap32(nonce)`. Padding words become numeric `0x80000000`, with final lengths 640 for the header and 256 for the second hash.
3. Copy raw `SUM[i]` numerical words directly into the first eight words of the second-hash block without swapping them. Do not byte-swap both in DMA and hardware.
4. Preserve external digest serialization and Bitcoin target semantics. SDK KAT calls may configure BSWAP themselves; every optimized-job begin must restore its required configuration.
5. Run all oracle and target tests on both architectures, then the paired benchmark protocol.

E03-b, register-only digest handoff:

1. Starting from the winner of E03-a, load all eight intermediate `SUM` values into explicit local `uint32_t` values before `START` for the second hash.
2. Feed those values directly, followed by constant padding, without a RAM round trip. Keep the MMIO pointer and hot state accessible without excessive spills.
3. Inspect assembly. A C local is not a guarantee of a physical register. If spills/callee saves erase the gain, retain the simpler RAM path on that architecture.
4. Never stream a `SUM` word into the second hash before preserving all words: writing `START` destroys the preceding digest state. Direct DMA from SUM to WDATA has the same dependency problem.

Expected benefit: reduced CPU work, not fewer SHA blocks. Stop if it does not outperform the paired baseline or complicates correctness without measurable gain.

### E04 — Specialize feeds and place useful work in compression gaps

Run these as separate variants, not one combined patch:

- **E04-a: constant-tail feeders.** Replace generic loads of the padding-heavy blocks with specialized writes of four header-tail words, padding, zeros, and length; similarly specialize the second-hash padding. Test compiler-generated zero reuse versus literal loads. Do not use `memcpy` or incrementing-destination block stores for WDATA: all sixteen writes target the same MMIO address.
- **E04-b: hide preparation in waits.** After submitting a block, prepare the next nonce word, local accounting, or invariant values while compression proceeds. Finish with a real readiness/validity check. Do not replace synchronization with a guessed fixed delay; the CPU, IRQ, and bus schedule changes between configurations.
- **E04-c: fuse the hot batch loop.** Keep hasher/base pointers and counters live across nonces; benchmark unroll factors 1, 2, and 4. Update 64-bit totals once per completed bounded batch using a 32-bit local count. Correctly account for partial batches, stop requests, shares, and nonce wrap. Keep timed work equivalent.
- **E04-d: cheaper common target rejection.** For targets whose most-significant 32-bit word is zero, raw `SUM7 != 0` is a byte-order-independent rejection. Only after zero/equality enter the complete comparison. For a nonzero target word, byte-swapping both operands does not preserve numerical ordering; retain a correct comparison. Exercise target equality and easy targets.
- **E04-e: sticky-error check batching.** First prove documented sticky-error behavior and correct clearing. A bounded batch may check an error once at its end instead of each hash if any error invalidates the entire batch and no candidate is published before validation. Keep a diagnostic per-hash mode and immediate full verification of candidates. Never clear away evidence to improve timing.
- **E04-f: reduce reporting work.** Compare fixed time-based reporting at a documented cadence with the current 100,000-hash threshold. Use elapsed wall time consistently: do not exclude telemetry from a value labeled sustained throughput. Keep temperature coverage and bounded control latency. Log reporting-cadence changes as workload changes.

Optional experiment: verify whether known state transitions allow avoiding an immediately redundant ready read after an explicit reset, while retaining all necessary block-completion checks. Derive this from the documented state machine, not merely from a few successful trials.

Acceptance: equal-work oracle success and repeatable gains; logging/control latency remains bounded. Restore the previous subvariant after a regression.

### E05 — Explore compiler and layout choices methodically

Files: `CMakeLists.txt`, `tools/build`, optional repository CMake helpers. Add explicit experiment settings that the wrapper passes through; do not silently rely on arbitrary environment flags or leave several conflicting `-O` flags in one command. Proposed settings include `MINER_VARIANT`, `MINER_OPT_LEVEL`, and `MINER_ENABLE_LTO`; these do not exist yet. Print resolved settings during configuration and in the firmware identity, and isolate or explicitly reconfigure cached build directories so flags cannot leak between variants.

1. Record the current effective flags. Debug symbols themselves do not imply unoptimized code. Distinguish `-g`, optimization level, assertions, and `NDEBUG`; keep validation and runtime failures active.
2. Compare `-O2`, `-O3`, and `-Os` on the same kernel. Then test link-time optimization (`-flto`, or supported CMake interprocedural optimization) at compile **and** link stages, with compatible SDK objects and startup/linker behavior. Keep all warnings as errors. Guidance: [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html).
3. Test selected inlining decisions: existing always-inline hot helpers, a dedicated non-inlined kernel, and compiler-default inlining. Test loop unrolling separately. Capture code growth, spills, and instruction-cache behavior rather than assuming larger code is faster.
4. Test hot-function/loop alignment at 4, 16, 32, and 64 bytes only where disassembly shows it can change fetch/branch layout. Keep the best reproducible result; do not tune to one accidental link layout without rechecking the final image.
5. Apply `restrict`, const qualification, and alignment guarantees only when true for every caller. Keep volatile on MMIO. Use unsigned 32-bit arithmetic for SHA wraparound and defined rotate helpers; no signed-overflow, aliasing, or shift-width tricks.
6. For ARM, retain Cortex-M33/Thumb and the SDK ABI. Look for `rev`, rotate forms, efficient pointer addressing, and spills. Do not change float ABI globally to optimize an integer kernel.
7. For RISC-V, verify the SDK selected `hazard3-rp2350` or the supported equivalent, not generic RV32I. Check byte reversal/rotate generation and compare compressed versus selectively uncompressed hot code as an advanced layout subvariant. Preserve ABI and ISA compatibility throughout the image. See [GCC RISC-V options](https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html).
8. The [Hazard3 project's CoreMark example](https://github.com/Wren6991/Hazard3) motivates testing `-mbranch-cost=1`, `-fno-code-hoisting`, `-fno-if-conversion2`, and unrolling individually. Those are hypotheses for this workload, not a recommended bundle. Verify each option is supported by the installed compiler through the normal wrapper build.
9. Re-test the winning flags after introducing a software SHA kernel: its instruction mix/register pressure differs substantially from a MMIO feeder. Avoid `-Ofast`/fast-math, global assertion removal, and unrelated flags with no measured explanation.

Acceptance: a small, documented flag set per architecture with reproducible gain or justified simplicity. Unsupported flags, warnings, or incorrect outputs fail the trial; do not install a new compiler just to continue the matrix without asking.

### E06 — Revisit DMA with persistent configuration, not per-call SDK overhead

The current comment that short messages cannot repay DMA setup is a hypothesis. A channel configured once may differ from the initial SDK path. The [DMA API](https://raw.githubusercontent.com/raspberrypi/pico-sdk/master/src/rp2_common/hardware_dma/include/hardware/dma.h) exposes byte swapping, chaining, ring addressing, and RP2350 self-triggering transfer counts; none automatically implements a Bitcoin hash state machine.

E06-a, minimal persistent DMA:

1. Claim a DMA channel once per worker lifetime. Configure `DMA_SIZE_32`, incrementing source, fixed WDATA destination, and `DREQ_SHA256`; set SHA DMA size through `sha256_set_dma_size(4)`. Keep aligned source buffers alive until completion.
2. For the first hash, explicitly `START`, then trigger 32 words from the padded header. Wait for DMA completion **and** final SHA validity before accessing the intermediate result.
3. Preserve/reformat the eight result words, explicitly `START` the second hash, and transfer its 16 words. Wait for result validity before comparing or starting another nonce.
4. Re-arm only fields that must change, using documented trigger aliases. Poll initially; do not introduce one interrupt per block. Validate channel completion/error/abort handling and preserve SHA error checks.
5. Compare CPU-only, DMA-only, and CPU/DMA mixed feeders for the three blocks. Measure complete nonce cost, not a long-message SHA MB/s benchmark. Retain a slower feed mode only if E10 demonstrates greater aggregate throughput from the freed CPU time.

E06-b, ping-pong and control overhead reduction, only if E06-a is competitive:

1. Use two owned header/result buffers so preparation never races a DMA read. Publish ownership transitions with appropriate ordering.
2. Test fixed DMA control blocks/chaining to reduce reconfiguration writes. Self-triggering/ring transfers do not perform nonce arithmetic, result-valid polling, or conditional target comparison by themselves.
3. If testing DMA SUM-to-RAM copying, start only after validity and finish all eight reads before reset. Compare its setup/bus cost with eight CPU loads. Do not DMA SUM-to-WDATA as if it could preserve state across reset.
4. Time-box fully autonomous sequencing. Before implementation, draw every reset/feed/wait/save dependency and identify what produces each trigger. If a required SHA-complete condition has no safe trigger, retain CPU supervision instead of inventing one.

Acceptance: both architectures pass full-digest/random/target tests, DMA and SHA have no errors, and no race appears under USB activity. Reject DMA speed claims that omit resets, digest transfer, or final comparison.

### E07 — Place only contention-sensitive code and data in useful memory

1. Do not repeat whole-image SRAM copying as a fresh optimization: the log already shows no gain. Revisit memory placement when profiling or a second core/DMA introduces a new contention mechanism.
2. Inspect the link map. Try only the hot kernel in SRAM, then separately its constants/state. Use SDK section helpers such as `__not_in_flash_func` or a repository linker fragment, not edits to SDK linker scripts. Inlined code executes wherever its caller is placed: verify the actual hot instructions' addresses rather than relying on the helper's annotation.
3. Place core stacks and high-traffic buffers to avoid contention; consider the two 4 KiB scratch banks and the two distinct striped SRAM groups. Budget stack space first, including IRQs and SDK multicore defaults. An attribute naming a scratch bank does not prove it has room.
4. Compare code/data placements under the actual CPU+DMA/USB load, with XIP/bus counters. Keep caches warm for throughput runs and separately test job-change/cold behavior. Cache-line pinning is an advanced variant only if supported SDK mechanisms and profiling justify it.
5. Test bus priority only after observing arbitration pressure; wait for documented priority-update acknowledgement. Report effects on both worker throughput and USB/ADC responsiveness. A higher priority cannot eliminate the APB protocol's inherent cost.
6. External PSRAM, if this board actually has it, is for bulky logs/job queues, not automatically a faster place for hot state. Do not enable or drive unverified external-memory pins.

Acceptance: measurable improvement under a specified contention scenario, no stack overlap or control regression. Record SRAM/flash budget and retain the simpler XIP layout if results are noise.

### E08 — Separate SHA ownership from telemetry and USB control

Start with homogeneous cores; keep existing single-core mode selectable.

1. Add `pico_multicore` linkage and launch a worker using supported SDK startup. Initially let core 0 own USB, ADC, and control, and core 1 own the SHA engine. Verify where USB background work/IRQs and alarm pools actually execute; merely moving `printf` does not relocate every interrupt.
2. Exactly one core owns SHA, its configuration, and any associated DMA channel. Acquire the SHA lock once for its work interval. Never call the generic hardware SHA API concurrently from the telemetry core. Run startup KATs before launching the exclusive worker or route them through it.
3. Exchange jobs, counters, temperatures, and candidate records through a bounded single-producer/single-consumer queue using C11 atomics with release/acquire semantics or an SDK primitive with equivalent documented guarantees. `volatile` is not inter-core synchronization. Avoid unsynchronized 64-bit reads on a 32-bit core.
4. Share immutable job snapshots tagged by generation. Publish candidate nonce plus full digest before the next SHA reset can destroy it. Handle queue-full explicitly: verified candidates must not disappear silently; telemetry may be coalesced only with a dropped-record counter.
5. Use nonce chunks, not per-hash locks. Check cancellation/stop flags at bounded intervals. Publish totals once per batch. ADC sampling on the other core must not hold a worker lock.
6. Measure both kernel rate and sustained aggregate rate, jitter, USB response time, queue high-water mark, and temperature. Test host detach/reattach, stop/job replacement, and an easy target that stresses share output.
7. Stop/join or safely park the worker before any flash/reboot operation; do not execute from XIP while flash is being modified. Preserve the established USB BOOTSEL recovery path.

Expected benefit: better sustained throughput and responsiveness, not two independent hardware miners. If telemetry load is tiny, the speedup may also be tiny; compare energy/thermal cost if measurable.

### E09 — Build a software-midstate path that can exploit CPU resources

This is a separate implementation of exact SHA-256, not a way to write the hardware SUM registers. Keep it isolated, portable, and independently tested before hand optimization. Algorithm source: [FIPS 180-4](https://csrc.nist.gov/pubs/fips/180-4/upd1/final).

E09-a, software midstate:

1. Implement/reference-test a portable 32-bit SHA compression function. Use it as the trusted on-device software baseline, with the host oracle as the independent authority.
2. Compute the chaining state after header bytes 0–63 once per job. Save all eight words and the fact that 64 bytes have been consumed. For each nonce, restore this **software** state and compress the padded header tail, then hash the resulting 32-byte digest from the standard SHA IV.
3. This removes one repeated software compression: two compressions per nonce, plus amortized setup. The tail length remains **640 bits**, not 128 bits. This operation is continuation of the first hash, not hashing a 16-byte message in isolation.
4. Invalidate the cached midstate whenever any first-64-byte header data changes. Regenerate tail precomputations when its fixed words change. Never reuse a midstate based only on nonce/job-counter assumptions.

E09-b, constant folding and partial precomputation:

1. The tail block's initial words are `W0..W2 = fixed header bytes 64..75`, `W3 = bswap32(nonce)`, `W4 = 0x80000000`, `W5..W14 = 0`, and `W15 = 640`, in numeric SHA word order.
2. Precompute working variables after rounds 0–2 because those rounds precede the nonce word. Retain the original chaining state for the final feed-forward addition; the precomputed working variables do not replace it.
3. Precompute schedule words W16 and W17, which are nonce-independent. W18 depends on the nonce; do not cache the whole schedule. Generate a dependency map for all 64 words before attempting more folding.
4. Specialize the second hash's padding: eight digest words, `0x80000000`, six zeros, length 256. Fold constants without changing modular arithmetic.
5. Compare a 16-word circular schedule with an expanded/unrolled schedule. Test round unrolling in small increments, boolean Ch/Maj equivalent expressions, rotate idioms, and two interleaved nonce states. Inspect register spills; two software lanes are not SIMD and may be slower.

E09-c, exact rejection before the final three software rounds, advanced:

1. Derive and independently test the SHA shift-register relation. If `e_61` means the working variable e after exactly 61 rounds of the **second hash**, final numerical digest word 7 is `(IV7 + e_61) mod 2^32`, since `h_64 = e_61`.
2. For a target whose highest word is zero, a nonzero word 7 already proves rejection, without computing the last three rounds. For general targets, convert word order correctly and compare; equality needs remaining work. Initially implement only the zero-high-word case.
3. Keep full-hash mode for KATs. On a possible share, finish/recompute the full digest and compare the complete target before publishing. Compare every rejection decision with the full software/hardware/host reference over deterministic tests, including synthetic equality cases.
4. Label performance as exact nonce evaluation with early rejection, not complete-digest output. This saves only three rounds of the second software compression, not most of SHA. It does **not** apply to invalid/incomplete hardware SUM reads.

Acceptance: full oracle equivalence and measured software H/s on each architecture. Software need not beat the SHA peripheral to be useful as an independent second-core worker.

### E10 — Combine hardware and software without double-counting work

Measure alternatives separately; use queue/bus observations to decide whether to continue.

E10-a, independent workers (simplest useful combination):

1. Let one core run the best hardware path; the other runs E09 software hashing while also servicing bounded telemetry/control work as appropriate.
2. Allocate disjoint nonce chunks from one job generation. Test allocation around `0xffffffff`; do not let two cores revisit the same nonce after wrap. Chunk allocation overhead is outside the per-nonce inner loop but included in sustained totals.
3. Report hardware-worker, software-worker, and aggregate **unique** rates. Compare the hardware worker's slowdown under shared-memory/USB load with the software contribution. Retain only a positive net gain with correct control behavior.

E10-b, software first hash plus hardware second hash:

1. A software worker uses a cached header midstate to compute the first 32-byte digest for each nonce, placing tagged records into a queue.
2. The SHA owner hashes each record's single padded 32-byte message from the standard IV and checks the target. It still needs reset/feed/wait/comparison for every record.
3. The resulting throughput is limited by the slower producer/consumer stage plus communication cost. The one-block hardware feed/compression bound at 150 MHz is a calculated 1,239,669 operations/s, but this is **not** a predicted Bitcoin rate: the software producer may be far slower than the existing all-hardware miner.
4. Measure isolated producer/consumer limits first. Stop before building a complex pipeline if those limits cannot plausibly beat the retained baseline.

E10-c, hardware first hash plus software second hash:

1. The engine produces first digests with two blocks each; a software worker completes the final SHA and comparison. Preserve FIFO records and count only fully completed nonce evaluations.
2. Determine whether software throughput and transfer overhead make this competitive. Avoid presenting first-hash production rate as Bitcoin mining rate.

E10-d, DMA-assisted overlap:

Let CPU software rounds for a different, independent nonce run while DMA feeds/compresses hardware work. Begin with coarse bounded chunks, not cycle-perfect interleaving. Measure whether synchronization and bus contention outweigh useful overlap. Only one owner may reset/configure SHA; never interleave two unfinished hardware messages on its single chaining state.

Acceptance: all issued/completed/cancelled jobs reconcile, every candidate verifies, no duplicate work is counted, and aggregate sustained performance improves under matched temperature/reporting conditions.

### E11 — Clock, thermal, power, and reliability qualification

The historical 200 MHz result scales almost linearly, but it is an overclock with invalid historical thermal readings. It does not establish safe temperature, voltage margin, or long-term reliability.

1. First complete E00/E01. Identify chip revision, board regulator/supply capability, flash maximum clock, and relevant operating limits from the exact component documentation. Record the chosen temperature abort threshold with calibration/uncertainty margin and rationale. Do not invent a universal safe temperature from the internal sensor formula.
2. Preserve a stock 150 MHz recovery image with current correctness/temperature fixes. Establish how to restore it via the existing USB/BOOTSEL workflow. Implement bounded stop/fault behavior before unattended clock tests.
3. Test clock-only changes at unchanged regulator setting, one supported PLL setting at a time. Candidate study points are 150, 180, and 200 MHz, subject to clock synthesis and board limits. Treat any above-rated point as an explicitly labeled experiment, not a production default.
4. Keep USB at its required 48 MHz and ADC within its documented clock limits. Record actual clock tree values. Check QSPI/QMI timing: a flash divider of two changes flash clock when `clk_sys` changes. Use verified flash limits, not the nominal CPU clock alone.
5. At each point, run oracle/KATs before and after warm-up and measured windows. Record start/end/min/max die temperature, errors/resets, and throughput. Compare cycles per nonce as well as H/s to detect non-linear stalls or a configuration mistake.
6. Qualify retained settings with at least a 30-minute controlled soak, periodic full-digest comparisons, temperature logging, and USB/control tests. Use bounded captures and communicate/log each new intermediate result. A short KAT pass is not a reliability guarantee.
7. Abort on SHA/ADC fault, digest mismatch, unexpected reset, missed deadline, or chosen thermal threshold. Recover to stock settings; do not respond by automatically increasing voltage. If USB disappears, stop autonomous retries and request BOOTSEL recovery as needed.
8. Higher clocks (for example 225/250 MHz), regulator-voltage sweeps, or changed cooling are deferred until the earlier data provides a rationale and the user agrees to the added risk/work. Do not disable voltage safety limits, alter OTP, or apply undocumented voltage settings.
9. If power efficiency matters, also measure lower-clock points and idle/sleep behavior. Use measured input power for hashes/joule; temperature is not a power meter. Ask before acquiring a USB power meter, probe, heatsink, or other hardware. Distinguish core power from board/USB total power.

Acceptance: a documented stock/default profile and, only if justified, separately labeled experimental profiles. Prefer a reliable measured operating point to the highest briefly bootable clock.

### E12 — PGO, alternative compiler, and limited assembly

Only after E02 shows material CPU overhead remaining:

1. Investigate GCC profile-guided optimization with the installed toolchain first. Bare-metal code does not have a normal writable filesystem at exit; use supported profile export, potentially `-fprofile-info-section`, and matching host `gcov` tools. Export data over USB outside measurement. See [GCC freestanding profiling](https://gcc.gnu.org/onlinedocs/gcc/Freestanding-Environments.html).
2. Train on representative hard-target mining plus job switches and controlled easy-target candidate paths. Rebuild with the profile and compare against non-PGO with the same compiler/flags. Never report the instrumented training build as the optimized result.
3. Inspect whether an alternate SDK-supported Clang/GCC version could change identified bad code generation. Ask before installing it. Pin toolchain identity and build both architectures through wrappers; keep existing tools untouched. Do not conflate a toolchain update with a kernel change.
4. If a small hot sequence still emits redundant instructions, write an architecture-specific leaf routine with a portable fallback. Candidates: digest handoff, repeated WDATA stores, specialized software SHA rounds. Specify ABI, clobbers, register preservation, memory ordering, stack use, and IRQ assumptions explicitly.
5. Test complete oracle coverage on both architectures and re-run stress/thermal checks. A few saved cycles do not justify an unreviewable assembly rewrite or dependence on undocumented peripheral behavior.

Acceptance: bounded, explainable gain with reproducible builds and maintainable fallbacks; otherwise retain C.

### E13 — Creative options with explicit feasibility limits

Time-box each idea to an initial written dependency/cost model and, only if promising, one isolated prototype. These are not prerequisites for the normal optimization sequence.

| Property / idea | Plausible experiment | Limit / reason to defer |
| --- | --- | --- |
| Mixed M33 + Hazard3 sockets | Pair the better MMIO owner ISA with the better software-SHA ISA after measuring each separately | Requires two images, correct core-1 boot/architecture selection, shared layout, and recovery; ordinary homogeneous `pico_multicore` startup is not enough |
| PIO state machines | Experimental DMA pacing/sequence assistance, or low-overhead external timing markers | PIO cannot directly execute general memory-mapped SHA access or replace SHA's arithmetic; identify a real safe trigger before building a sequencer |
| SIO interpolators / DMA address generation | Evaluate cheap descriptor/nonce preparation only if CPU arithmetic is a measured cost | MMIO setup/reads can cost more than a CPU add/rotate; interpolation is not a SHA round engine |
| DMA ring/chaining/self-trigger | Amortize descriptor setup for known block transfers and queues | Does not solve SHA state reset, completion detection, or result dependencies by itself |
| SRAM banks / pinned XIP lines | Isolate hot software kernel, DMA buffers, and stacks under measured contention | No new memory bandwidth benefit if the current tight loop already hits in cache |
| Multiple header midstates with a shared tail schedule | Software-only, ASICBoost-style schedule sharing across distinct permitted header versions; precompute compatible midstates and process the same tail words through several states | Needs legitimately distinct job headers and measured schedule savings; hardware hides its schedule and cannot accept restored midstates; do not change unauthorized version bits |
| Available RP2350A GPIO | Optional timing markers, measured reference/temperature instrumentation, or a future external accelerator | Requires actual board pin mapping and possibly equipment; GPIO alone does not raise H/s |
| Power/clock gating unused blocks | Improve energy efficiency after throughput is stable | Preserve USB, timer, ADC, SHA, DMA, and recovery dependencies; do not claim throughput from a power-only change |

Mixed-ISA feasibility specifically: hardware supports per-socket selection, but an implementation must first prove a minimal mixed-core heartbeat with documented RAM/boot behavior, matching shared-data layout, and a reversible normal boot path. No security/OTP/partition changes are permitted. Do not start this complex branch unless the homogeneous measurements predict a worthwhile net gain. See the [RP2350 architecture-switching documentation](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf), section 3.9.2.

Explicit non-options: DMA CRC/sniffer, HSTX, floating-point/DCP, and packed 16-bit DSP arithmetic are not SHA-256 accelerators. Independent 32-bit modular additions/rotates cannot be substituted by packed narrower arithmetic with cross-lane carries. Lookup-table precomputation cannot turn arbitrary nonce increments into a simple digest increment. Do not propose four active cores, hardware midstate writes, unsupported SHA instructions, or reads of invalid partial hardware digests.

### E14 — Optimize useful work and qualify the final combination

The current firmware performs genuine hashes of a fixed genesis header but labels the work stale. It is not a live pool miner. Do not equate a faster repeated stale search with productive accepted shares.

1. Add explicit job IDs and finite nonce ranges before claiming unique-work throughput over long runs. On exhaustion, stop/request new work or switch to another authorized job; silently wrapping the same 32-bit range repeats work.
2. Precompute padding, targets, midstates, and invariant schedule terms once per job. Measure setup/cancellation latency separately from steady-state rate. Use immutable job snapshots and account for in-flight stale work during replacements.
3. For future host-provided jobs, validate header length, target, compact representation where applicable, and job generation. Version/time/extranonce rolling must obey job permissions; do not mutate consensus fields arbitrarily to manufacture unique work.
4. Keep USB status sparse and structured, with a bounded command path. Candidate records need nonce, job ID, and full hash for independent host verification. Pool/network integration is a separate feature request, not required for this optimization plan.
5. Combine only individually retained changes, then rerun the full paired baseline comparison. Optimizations can interact negatively through register pressure, caches, bus traffic, or logging. Measure the combination; do not add individual speedup percentages.
6. Final matrix: both architectures at stock clock; chosen multicore/DMA/software mode; easy and hard targets; frequent and infrequent job changes; USB connected/disconnected/reconnected; idle/load temperatures; long soak; stock recovery.
7. Leave the agreed stable image on the device after future implementation, document its exact build settings and wrapper commands, and keep rejected experiments and raw data in the append-only performance history. Do not leave a high-clock experimental image running by accident.

## 6. Recommended first implementation session

When the user authorizes implementation, perform **only E00 → E01 → E02 → E03 initially**, at 150 MHz. Pause after the new matched architecture baselines to summarize findings. If board identity blocks E00, request the specific missing board information and continue only independent host-side test planning/work that does not assume pin/flash configuration.

Suggested handoff instruction for a less advanced coding model:

> Implement the next unfinished experiment in planned_optimizations.md, starting with E00. Do not implement later experiments in the same patch. Use repository wrappers, build both architectures before flashing, validate against the required tests, and record every measured result with temperature validity in chat and perf_progress.md. Keep stock 150 MHz until the clock experiment is separately authorized. If a prerequisite fails, diagnose it; do not bypass the gate or invent a measurement. End with the retained variant, exact evidence, and next experiment ID.

After E03, prioritize E04/E05 for cheap wins. Promote E06 only if feeding/control overhead remains significant; use E08 when sustained-rate loss or responsiveness matters. E09/E10 are the main route to exploiting otherwise idle CPU compute beyond the shared accelerator's limit. E11 changes operating conditions and must remain a separate qualified comparison. E12/E13 are optional, evidence-driven branches.

## 7. Completion checklist for future implementation

- [ ] Correct carrier-board configuration for RP2350A, including sensor channel 4.
- [ ] Valid, explicitly approximate temperatures and recorded reference assumptions.
- [ ] Independent full-digest and target oracle coverage for every enabled fast path.
- [ ] No warnings; both architectures build via wrappers.
- [ ] Strict boot/test/benchmark/temperature/fault parsing and bounded serial deadlines.
- [ ] Fresh equivalent ARM and RISC-V baselines, not comparisons across historical kernels.
- [ ] Every measurement in chat and `perf_progress.md`, with unique artifact/log identity.
- [ ] Kernel and sustained mining rates separated; no duplicate/incomplete work counted.
- [ ] Safe SHA/DMA ownership, queue behavior, nonce wrap, cancellation, and recovery.
- [ ] Retained speedups exceed noise; rejected/no-benefit experiments remain documented.
- [ ] Any overclock separately labeled, thermally qualified, and explicitly selected.
- [ ] Final combined variant revalidated; stock recovery image available.

## 8. What this planning change does not do

This document does not correct the existing board setting, repair ADC sampling, alter compiler options, build firmware, flash the chip, run a benchmark, append invented performance entries, install tools, or change machine configuration. Those actions await implementation authorization.

## 9. Source navigation for the implementing model

Start with these repository files:

- `src/main.c`: all current hashing, tests, target checks, telemetry, and mining loops.
- `CMakeLists.txt`: target options, libraries, default board/platform, USB configuration.
- `tools/build`, `tools/cycle`, `tools/monitor.py`: authoritative workflow and current validation limitations.
- `.vscode/settings.json`, `.vscode/tasks.json`: editor integration; verify consistency after board selection.
- `perf_progress.md`: measured historical evidence and rejected experiments.

Installed SDK inspected for this plan: `/home/claude/.pico-sdk/sdk/2.3.1`. Read it; do not edit it. Relevant paths relative to that directory:

- `src/boards/include/boards/pico2.h` and the actual carrier's candidate board header.
- `src/rp2350/hardware_regs/include/hardware/platform_defs.h` and `src/rp2350/pico_platform/include/pico/platform.h`.
- `src/rp2_common/hardware_adc/include/hardware/adc.h`.
- `src/rp2350/hardware_regs/include/hardware/regs/sha256.h` and `src/rp2350/hardware_structs/include/hardware/structs/sha256.h`.
- `src/rp2_common/hardware_sha256/include/hardware/sha256.h`, `src/rp2_common/pico_sha256/sha256.c`.
- `src/rp2_common/hardware_dma/include/hardware/dma.h` and relevant DMA register definitions.
- `src/rp2_common/pico_multicore/`, `src/rp2_common/pico_stdio_usb/`, and the selected linker script.
- `cmake/preload/toolchains/pico_riscv_gcc.cmake` and `pico_arm_cortex_m33_gcc.cmake`.

Use the primary-source links near each experiment for rationale. Web `master` and current GCC documentation can differ from installed releases: resolve API/flag details against the pinned local version before implementing. Distinguish verified hardware facts, historical measurements, calculated limits, and speculative proposals in all future reports.
