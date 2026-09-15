# RP2350A Bitcoin miner: optimization roadmap update

Date: 2026-09-15. Evidence cutoff: repository `724c503`, through experiment `E09b-fixed-full-rounds-riscv-80` in [perf_progress.md](perf_progress.md). This is a planning document; no new firmware measurements were made while preparing it.

This update preserves E00–E14 and their original objectives from [planned_optimizations.md](planned_optimizations.md), records what actually happened, and expands the unfinished work. Original substeps remain applicable unless explicitly superseded below. A completed experiment does not imply that every validation or subvariant in its work package is complete. New suffixes identify proposed experiments, not recorded results.

Implementation observations were checked against [src/main.c](src/main.c), [src/software_sha256.c](src/software_sha256.c), [CMakeLists.txt](CMakeLists.txt), and the build/monitor/analysis wrappers. Measurements are taken from the performance history; code inspection and calculated estimates are identified separately.

## 1. Revised direction

Continue from the retained dual-worker implementation, with stock 150 MHz as the comparison point. First resolve the pending ARM result for the latest change, strengthen measurement and worker-lifecycle coverage, then test a simpler DMA trigger and focused software SHA code generation. Keep architecture-specific winners. Defer complex hybrid and mixed-ISA work until a measured cost model supports it.

The largest changes to the original ranking are:

- E03, much of E04, E08, E09-a/b/c, and E10-a have already produced hardware results. Reimplementing them would duplicate completed work.
- E06 first-block DMA helps Hazard3 but regresses M33. E03 software schedule fusion helps M33 but regresses Hazard3. A uniform implementation is not automatically the fastest configuration.
- E07 software compression in SRAM helps under dual-core load; the earlier whole-image and single-worker SRAM experiments did not. Memory placement remains worth investigating only against the current workload.
- E09 software contributes about 8% of the current reported total. A 5% software-worker gain would add only about 0.4% aggregate if hardware throughput were unchanged. Rank work by absolute useful evaluations gained per second, effort, and confidence.
- E01/E02 still have unfinished measurement work. Current aggregate reporting mixes time windows, and many historical comparisons used short runs or final-report medians. Preserve those observations, but improve the measurement contract before chasing tiny aggregate changes.
- E14 nonce allocation and E08 queue/control behavior deserve earlier attention. The current fixed parity split prevents duplicate work in the default dual-worker mode but cannot keep both workers productive over a complete nonce space.
- E11 has short successful 300 MHz results on both architectures. It has no thermal qualification; 350 MHz failed clock configuration, not a hash test.

The original execution override remains relevant: temperature extraction was explicitly disabled after all ADC channels failed, and experimental clocks up to 550 MHz were authorized. Keep `temperature=disabled`; do not revive ADC validity as a prerequisite for stock-clock code work. The ceiling is neither a target nor evidence that any frequency is achievable or stable. Voltage changes and physical board modifications are separate work.

## 2. What the measurements establish

### 2.1 Current comparison points

All rates below are copied from the log, at 150 MHz. “Full software” produces a complete digest. “Software filter” performs exact nonce rejection using the final high word, with full verification for potential shares. Historical dual-worker rates are the reported medians, subject to the timing caveat in E01 below.

| Source state / hardware run | Full software H/s | Software filter evaluations/s | Hardware worker H/s | Software worker evaluations/s | Reported aggregate evaluations/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Last change retained on both ISAs, `90c7dc1`, ARM experiment 78 | 29,205 | 30,718 | 325,746 | 30,554 | 356,300 |
| Same source, RISC-V experiment 79 | 28,484 | 29,909 | 339,024 | 29,706 | 368,729 |
| Pending common change, `1a1ea93`, RISC-V experiment 80 | 28,636 | 30,041 | 338,998 | 29,835 | 368,834 |
| Pending common change, `1a1ea93`, ARM | No recorded result | No recorded result | No recorded result | No recorded result | No recorded result |

Evidence: [ARM fixed filter rounds](logs/E09b-fixed-filter-rounds-arm.log), [RISC-V fixed filter rounds](logs/E09b-fixed-filter-rounds-riscv.log), and [RISC-V fixed full rounds](logs/E09b-fixed-full-rounds-riscv.log), as cited by the performance history.

Component medians need not sum exactly to the median aggregate; the table preserves the recorded values, including one-evaluation/s differences.

Do not call experiment 80 retained on both architectures because both builds succeeded. It has only the RISC-V hardware validation. Its filter helper was unchanged; the log attributes the filter movement primarily to layout. Do not claim the full-digest specialization directly accelerated the common rejection algorithm.

The last paired reported aggregate favors homogeneous Hazard3 by about 3.49%, although M33 has the faster software filter. These are best retained configurations per ISA, including their different DMA/error/fusion choices, rather than a pure CPU comparison with identical feature settings.

### 2.2 Important earlier milestones and limits

| Experiment / condition | ARM observation | Hazard3 observation | Consequence |
| --- | --- | --- | --- |
| E01 matched corrected baseline | 294,091 kernel; 289,441 sustained | 290,667 kernel; 286,690 sustained | Useful historical reference; no longer the current kernel |
| E03 numeric words + register handoff | Handoff kernel 308,608 | Handoff kernel 313,774 | Already retained; Hazard3 preserved all eight digest words in registers |
| E04 constant tail + fast rejection + accounting | E04-c: 324,632 kernel; 320,406 sustained | E04-c: 325,341 kernel; 314,333 sustained | Already retained groundwork |
| E04 removal of redundant post-START poll | Repeated 331,818–331,819 kernel | Repeated 339,326–339,327 kernel | Retained; required inter-block and valid-result waits remain |
| E06 persistent first-block DMA | About -0.20% sustained; rejected | 344,783–344,784 kernel; 335,420–335,421 sustained; retained | Tune this small DMA path first |
| E08 hardware miner on core 1 | 327,469 sustained, +0.70% | About 340,094 sustained, +1.39% | Already implemented |
| E09 expanded software schedule | 26,440 versus 19,144, +38.11% | 24,126 versus 19,005, +26.94% | Compiler-friendly schedule structure mattered far more than generic flag changes |
| E10 independent workers | 348,487 aggregate | 356,366 aggregate | Already implemented; do not add their gains again to today's baseline |
| E07 software compression in SRAM under E10 | 351,958 aggregate, +1.00% | 361,993 aggregate, +1.58% | Retained under demonstrated contention |

The original three-block peripheral floor remains 363 cycles per nonce, or about 413,223 H/s at 150 MHz, excluding surrounding work. The historical E06 Hazard3 kernel at 344,783 H/s corresponds to about 435.06 cycles/nonce. This leaves about 72 cycles above that optimistic floor, not hundreds of freely removable cycles. Current worker rates include additional workload and should not be treated as isolated peripheral timings. The bound is for the hardware worker, not the sum of hardware and independent software work. See the original plan's derivation and [RP2350 SHA timing, section 12.13.2](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

### 2.3 Clock results are separate configurations

The later E11 sweep used `d9c0de8`, before the latest software filter/precomputation changes:

| Clock | ARM reported aggregate evaluations/s | Hazard3 reported aggregate evaluations/s | Evidence status |
| --- | ---: | ---: | --- |
| 150 MHz | 352,188 | 363,300 | Parent comparison |
| 200 MHz | 469,460 | 485,762 | Short functional passes |
| 250 MHz | 586,868 | 607,106 | Short functional passes |
| 300 MHz | 703,987 | 728,213 | Short functional passes |
| 350 MHz | No recorded hardware trial | No accepted rate | `set_sys_clock_khz` rejected the requested frequency |
| Above 350 through 550 MHz | No recorded results | No recorded results | Unqualified; do not extrapolate a measured result |

The earlier E04-c Hazard3 300 MHz image lost USB enumeration; the later E11 experiment 54 passed at an actual 300 MHz. Preserve both results. The cause of the earlier failure was not conclusively established, and the later success does not establish reliability margin. The 350 MHz experiment 56 emitted `FAULT type=system_clock`; it never demonstrated operation at 350 MHz. No recorded 30-minute final qualification or valid temperature measurement exists.

## 3. Status of every original step

| ID | Original work package | Updated status and priority |
| --- | --- | --- |
| E00 | Board identity and temperature | Identity established; temperature disabled by override. Hardware thermal repair deferred |
| E01 | Oracle, benchmark harness, failure gates | Strong cross-engine oracle retained; complete parser, aligned measurements, rare-path and lifecycle coverage unfinished. High priority |
| E02 | Matched architecture baseline and profiling | Historical static profile completed; fresh stage/counter profile on dual-worker code untested. High priority |
| E03 | Endian representation and digest transfer | Hardware work retained; software direct output retained; fusion retained only on ARM. Further transfer tuning conditional on assembly |
| E04 | Feeding, scheduling, accounting | a/c/d and post-START simplification retained; b rejected; e retained only on RISC-V; f still open |
| E05 | Compiler and layout matrix | Several wins/rejections recorded; O2/O3/Os, targeted alignment and remaining source-level forms still open |
| E06 | Persistent DMA and combinations | First block retained on RISC-V; full DMA rejected there; control/trigger variants untested. Promote minimal trigger experiment |
| E07 | Memory placement and contention | Software code SRAM retained; whole image, hot-main-only, and constants relocation rejected in tested settings. Bank/priority work untested |
| E08 | SHA owner plus control core | Core assignment and FIFO implemented; stress, cancellation, nonblocking telemetry, and graceful stop unfinished |
| E09 | Software midstate and specialized rounds | a/b/c extensively tested; latest full-round specialization pending ARM. Further focused specializations remain promising |
| E10 | Hardware + software workers / hybrids | a retained; FIFO poll64 rejected on RISC-V; b/c/d not measured. Prefer current independent workers |
| E11 | Clock, power, thermal qualification | Short sweep through 300 MHz completed; exact 350 rejected; long qualification and clock-feasibility work open |
| E12 | PGO, alternate compiler, assembly | No recorded experiment; conditional, after fresh profiling |
| E13 | Mixed ISA and unusual platform ideas | No recorded experiment; mostly low priority given small present upside |
| E14 | Real-job efficiency and final qualification | Parity allocation implemented; finite jobs, redistribution, cancellation and final matrix unfinished. Move lifecycle work earlier |

“Untested” below means no result for that subvariant in `perf_progress.md`. Build-only failures and byte-identical no-op trials are evidence, even though they have no new hardware rate. Do not relabel them as fresh opportunities merely because they were not flashed.

## 4. Updated work packages

### E00 — Preserve the resolved identity and temperature override

Recorded: package register 1, RP2350A/QFN-60, chip ID `0x30004927`, silicon revision 3. Correct channel 4 and all five ADC inputs returned saturated/error results. Temperature-disabled stock baselines passed on both ISAs.

Keep package assertions and boot identity. Board-model, supply/reference diagnosis, calibration, idle/load thermal sampling, and external instrumentation remain the original deferred hardware work. Do not repeat channel-8 selection or clock-dividing ADC experiments as new optimizations. If thermal work resumes, verify the physical carrier and ADC supply/reference first, then use the original sample-validity/calibration procedure. No hash-rate inference can replace a temperature measurement.

Acceptance for ongoing software work: correct package, passing hash validation, explicit `temperature=disabled`. Thermal qualification remains separately incomplete.

### E01 — Finish the measurement contract before relying on small deltas

Recorded: deterministic 4,096-case host `hashlib` oracle, full hardware/software digest agreement, filter high-word agreement, 10 target-boundary checks, seven suites including sticky-error proof, and benchmark failure propagation. `tools/monitor.py` now requires a seven-pass summary plus hardware benchmark and rejects observed `TEST:FAIL`/`FAULT`. The original description of a one-health-line monitor is obsolete.

Remaining original work, with new detail:

1. **E01-window (new; high priority): aligned aggregate measurements.** Current core 1 reports a recent 100,000-hash interval rate, while core 0 calculates its software rate from startup. `hash_rate_hs` adds those rates. This is a useful steady-state estimate, but not a common-window aggregate. Add worker completion snapshots with timestamps and a bounded synchronization protocol; preferably fixed-work epochs with a shared start and a drained completion boundary. Compute aggregate from validated unique completions divided by one wall-time interval. Report each worker over that same interval. Include queue, printing, stalls, and drain time in sustained mode. Do not remove observed USB dips by calling the median a whole-run average.
2. **E01-repeat (original contract, still unfinished):** use identical nonce sets/counts per A/B pair, at least 30 seconds warm-up, at least five 5–10-second windows, and A–B–B–A ordering across fresh starts. The existing approximately two-second, time-limited startup benchmark remains a smoke measurement. Final-30 reports from one run are not 30 independent trials, especially when software rates are cumulative. Report whole-run throughput, window median/range/MAD, and control latency. Temperature remains disabled, so no thermal-equilibrium claim follows from warm-up.
3. **E01-identity (unfinished):** include source/configuration identity, architecture, clock, fixture digest, run ID and sequence number. Require BOOT, the expected suites/counts, oracle identity, hardware benchmark, both software benchmark modes, and a minimum sustained window when evaluating E09/E10. Reject stale sessions, reset fragments, malformed/partial records and missing stages. The present parser checks prefixes and does not enforce this complete contract. Exercise it with saved synthetic failure/missing/wrong-ID logs and bounded serial setup deadlines.
4. **E01-rare (unfinished):** directly test the actual fast comparator and software candidate fallback. Random oracle cases mostly exercise high-word inequality. Add deterministic high-word equality followed by lower-word rejection, exact equality, and valid-share cases; exercise zero and nonzero highest target words and compact-target boundaries. Feed known qualifying headers/nonces through the software mining decision path, not just its full-digest helper. Verify every emitted candidate independently on the host.
5. **E01-lifecycle (unfinished):** test final partial batches, nonce wrap/exhaustion, invalidated batches, job replacement, cross-core fault propagation and full queues. Keep deliberate illegal SHA writes confined to the existing isolated startup self-test; production feeding must remain valid. A failure must invalidate uncommitted work and park both workers.
6. Retain the original padding-boundary tests if a generic byte-oriented software SHA API is introduced. The current software implementation is specialized for Bitcoin headers; its host oracle is the relevant gate today. A generic API would need its own length/padding coverage.

Retain simple, exact changes below 2% only with evidence above observed variability and a clear cost argument. For complex changes, keep the original 2% screening rule unless the affected component or practical control benefit justifies an explicit exception. A fast isolated helper is insufficient if total useful work regresses. Do not retroactively erase the historical retention decisions; label confidence and recheck the final combination.

### E02 — Refresh the profile for the actual dual-worker implementation

Recorded E02 was a static assembly/cycle-budget study of E01. It added the existing `./tools/analyze arm|riscv summary|disassembly [symbol]` wrapper. It did not measure per-stage DWT/Hazard3 counters or bus events. Its intermediate-buffer bottleneck has largely been removed.

Untested follow-up:

- Measure current hardware-only, software-only and dual-worker modes at 150 MHz with matched work. Distinguish core-0 startup benchmarks from the core-1 worker. Attribute interference by comparing the same linked image with workers enabled/parked where practical.
- Measure SHA feed/start/wait/handoff, DMA rearm/wait, hardware result/error checks, software tail compression, second-hash filter, setup, FIFO blocking and USB formatting. Use supported counters through a repository profiling mode; account for wrap and instrumentation overhead. Keep profiling out of retained throughput measurements.
- Inspect each hot helper's actual address, frame size, spills, loads and emitted round structure. Collect XIP/bus events under each mode and stack high-water marks. Current software placement and size must be remeasured; the original 1.3 KiB SRAM cost described an earlier helper, not today's collection of specializations.
- Publish a removable-cost estimate for each next experiment. One fewer C operation may make no timing difference; the E04 target specialization and E10 poll64 trials demonstrate this.

Acceptance: current cost table and at least one testable bottleneck hypothesis, not an inferred sum of intrusive microtimings.

### E03 — Preserve representation wins; gate further fusion on register pressure

Recorded and retained: E03-a numeric SHA words, E03-b preserving all eight hardware SUM words before reset and feeding directly, software direct header-digest output, and ARM-only second-schedule fusion. Direct software output improved Hazard3's filter by 3.65% but was neutral on M33. Fusion improved M33's filter by 0.53% and regressed Hazard3 by 2.31%.

Untested refinements:

- **E03-handoff-leaf:** only if E02 finds a remaining M33 spill, compare a small kernel boundary that lets all eight digest words stay live through START and WDATA feeding. First try C register lifetime/inlining changes; reserve assembly for E12. Measure the mining call site, not just an isolated helper.
- **E03-batch-api:** move nonce iteration into a software batch entry point so invariant hasher pointers and setup can remain live across multiple evaluations. Preserve ARM's caller-owned schedule and Hazard3's separate digest arrangement initially. Test bounded batches such as 8/32/128 with exact partial counts and control service between batches. This is distinct from the rejected FIFO-poll64 source change; include its effect on hardware throughput.
- A fused header-tail/filter function is an untested higher-cost alternative. Attempt it only if profiling shows meaningful call/copy traffic; inspect peak live state and SRAM use first. The rejected Hazard3 schedule fusion is direct evidence against assuming bigger fusion is better.

Do not combine hardware messages across resets or bypass valid-result waits. Hardware midstate restore remains unavailable; software midstate reuse does not change that constraint.

### E04 — Finish reporting and batch work; avoid rejected scheduling assumptions

| Original substep | Evidence / retained decision | Remaining experiment |
| --- | --- | --- |
| E04-a constant feeders | Retained on both ISAs | Only pursue different literal/register reuse if assembly exposes repeated work |
| E04-b preparation in compression gaps | Explicit next-nonce preparation rejected on both | Other overlap must start with a measured gap; use E10-d for independent software work |
| E04-c fused batches and accounting | 64-bit hardware accounting batching retained; function-local unrolling retained | Explicit factors 1/2/4 on the current worker, and bounded software accounting batches, not yet recorded |
| E04-d target rejection | Generic zero-high-word fast path retained; forcing a zero-MSW-only target shape rejected | General-target dispatch and software exact filtering under E09/E14 |
| E04-e sticky-error batching | RISC-V retained; M33 checked path retained after several regressions | Validate bounded partial batches and candidates with future scheduler changes; do not transplant RISC-V control shape to ARM by default |
| E04-f reporting cadence | No dedicated hardware comparison recorded | Separate computation/accounting, telemetry publication and host formatting cadences |
| Post-START ready poll | Removal tested and repeated on both | Already complete; necessary block/valid synchronization is still required |

**E04-f-cadence (expanded):** compare the existing report interval with 250 ms and 1 s telemetry, documenting bytes/s and host behavior. Publish worker counters without formatting on the SHA owner. Do not make error-check or cancellation latency depend on a slower display cadence; these need their own bounded work epochs. Measure sustained work and USB response, including easy-target share load and a slow reader.

**E04-software-count (new):** core 0 still increments a 64-bit software counter per evaluation. Try a 32-bit local completed count committed at bounded batch boundaries. Retain exact totals on candidate, cancellation, fault and final partial batch. Expected gain is small because SHA rounds dominate; reject if the extra boundary logic costs more than it saves, as happened in E04-e's first ARM attempt.

### E05 — Use targeted code generation, not another broad flag sweep

Recorded: targeted mining/software round unrolling helped. Whole-image LTO failed twice at build/link time; source-only LTO on the earlier `main.c` had no gain. Broad ARM label alignment regressed sustained speed by 4.49%. Generic Hazard3 flags on the feeder were rejected; the three software-source flag trials produced byte-identical UF2s and were correctly rejected without flashing. `restrict` produced no isolated software gain and was reverted.

Still untested or not exhausted:

1. Compare O2/O3/Os for one current hot function or miner-owned source at a time, preserving assertions and the SDK ABI. Keep the current O3 build as control. A changed optimization level is not inherently an improvement.
2. Compare explicit software unroll groups 4/8/16 and full expansion, plus compiler-default versus always-inline versus leaf call boundaries. Do not confuse the retained `unroll-loops` attribute with an exhaustive unroll-factor search.
3. Try small hot-function/loop alignments only where E02 identifies a fetch issue. Test a few nearby placements, inspect code size, and recheck after final linking. Avoid repeating translation-unit-wide `-falign-labels=32:31`.
4. Test equivalent Ch/Maj expressions individually, and recognize whether they actually change supported scalar instructions. Preserve defined unsigned rotates/additions. RISC-V compressed/uncompressed hot-code selection is a separate advanced experiment, not permission to change the ISA or ABI globally.
5. A miner-owned object library containing both `main.c` and `software_sha256.c` is a distinct, untested LTO boundary. Consider it only if E02 shows expensive cross-source calls and both wrapper builds can link cleanly while SDK/startup objects stay outside LTO. Previous main-only neutrality and whole-target failures make this lower priority than local source changes.

Keep an explicit architecture/variant configuration manifest and inspect actual final flags. Compare loadable artifacts before flashing: an identical image is a no-op, not a fresh hardware experiment. No alternate compiler installation is part of this planning task.

### E06 — Optimize the retained first-block DMA control path

Recorded: first-block-only DMA retained for Hazard3 at +1.61% kernel / +0.68% sustained; rejected for M33. Full DMA was explicitly validated on Hazard3 and rejected at 335,527 kernel / about 329,522 sustained versus 344,783 / 335,420. Earlier incomplete captures are not substitute benchmark records. Full DMA on ARM has no recorded hardware result; it is low priority given the negative first-block result and extra handoff traffic.

**E06-trigger (new; high priority, Hazard3 first):** configure the normal 16-word transfer count once, then restore the source and start via `dma_channel_set_read_addr(channel, source, true)`. Current `sha256_write_first_block` performs a non-triggering source write followed by a triggering count write on every nonce. The installed SDK 2.3.1 DMA register definitions specify that a trigger reloads the saved transfer count; `AL3_READ_ADDR_TRIG` can restore the source and trigger together. This offers a concrete one-write reduction with no new buffer or channel. It is a hypothesis until measured. The documented fixed-buffer trigger mechanism is described in [RP2350 DMA control/status aliases](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

Verify repeated transfers of exactly 16 words, normal count mode, channel idle before rearm, valid source lifetime and SHA START ordering. Preserve DMA completion, SHA inter-block/result waits, DMA/SHA error handling and abort behavior. Run the full oracle plus long repeated transfers under both worker and USB load. Record actual instruction/MMIO reduction and aggregate gain. Do not replace normal mode with endless or self-trigger operation.

**E06-ring (original E06-b, now concrete but lower priority):** separately test a 64-byte-aligned invariant first-block buffer and a 64-byte DMA read-address ring, so the source returns to its base. Trigger once per nonce. Compare against E06-trigger: both may already require only one trigger write, so ring alignment/storage complexity needs an additional measured benefit.

The original ping-pong buffers, control blocks/chaining, SUM-to-RAM DMA and alternate CPU/DMA block assignments remain untested subvariants. Advance only when E02 shows overhead that they can reduce or E10-d needs CPU time. SUM copying must complete before reset; DMA completion is not SHA completion. No autonomous sequencer should be built without a valid trigger for every dependency.

### E07 — Measure bank contention in the expanded software implementation

Recorded: whole-image SRAM gave no benefit; moving Hazard3 hot `main` under first-block DMA also gave no benefit. Software compression code in SRAM improved dual-worker totals. Moving the 256-byte constants table to SRAM gave only +0.02% ARM / +0.13% RISC-V and was rejected.

Untested extensions:

- **E07-banks:** after mapping current SRAM functions/stacks, separate the software schedule/stack, SHA DMA header buffer, and high-traffic shared records across suitable SRAM regions. Change one placement at a time. Both 4 KiB scratch banks need an explicit stack/IRQ capacity budget; do not assume their names imply free memory or use RP2040 aliases.
- **E07-hot-set:** measure which of the now-specialized tail/filter/full-digest helpers execute from SRAM. Keep the common filter hot; evaluate placement of rare full-digest/setup code separately if it competes for a useful bank or increases active footprint. Revisit constants only if counters show a new contention mechanism; their previous relocation is already a negative result.
- **E07-priority:** test bus arbitration priority only after observing contention; wait for documented acknowledgement. Distinguish DMA channel scheduling priority from system-bus master priority. With only one active miner DMA channel, a channel-priority flag alone has no demonstrated rival to outrank.
- **E07-cold:** compare warm throughput with frequent job changes and USB activity. Cache/pinning and external PSRAM ideas remain conditional on documented support, actual hardware and measured misses.

Acceptance: more aggregate useful work or a measured latency benefit, with stack headroom and responsive USB. Do not retain additional SRAM placement based solely on isolated helper speed.

### E08 — Complete lifecycle and backpressure handling

Recorded: core 1 owns SHA/DMA; core 0 handles USB/LED and software SHA. FIFO progress/share/fault records work in the measured hard-target runs. A bounded queue/control stress suite, job-generation protocol and graceful stop are not established by those runs.

**E08-queues (expanded original):** compare the blocking FIFO telemetry transport with a bounded single-producer/single-consumer SRAM queue or sequence-protected counter snapshot. Use C11 release/acquire or a documented SDK equivalent. Progress can be coalesced with an explicit count; candidate records require lossless handling or an explicit stop/backpressure policy. Never silently discard a share to maintain throughput. A dedicated fault/stop indication must not depend on successfully printing or filling a blocked queue.

Current FIFO records contain multiple words; consumer delays can stall the producer. Test slow/no USB reader, detached/reattached host, high share frequency, queue saturation, partial record arrival, and stop while a record is pending. Record queue occupancy and producer blocked time. Any improvement must include both software CPU time and hardware stall effects.

**E08-stop (unfinished):** add a generation-tagged immutable job, bounded cancellation checkpoints and a stop/acknowledge protocol. Drain validated final partial counts before job completion, quiesce DMA/SHA ownership on stop, and park both cores on fatal failure. Verify where USB/alarms execute and preserve reset-to-BOOTSEL recovery. Keep the existing hardware lock discipline; the software core must not invoke hardware SHA while the owner is active.

### E09 — Continue exact software specialization, with narrower hypotheses

Recorded and retained: portable header midstate; round unrolling; expanded schedule; SRAM compression code; precomputed rounds 0–2 and W16/W17; specialized second block; exact round-61 rejection; schedule trimmed through W60; partial round 3, W18/W19 and W31/W32; direct digest output; ARM-only fused schedule; native-order rejection word; cached K16+W16/K17+W17; fixed second-hash filter rounds 8–15.

Rejected: persistent reusable padded blocks on Hazard3 (-5.69% isolated software), with no ARM hardware trial. Do not propose the same buffer reuse as untested. The current filter's high word is already native-order and its unused W61–W63 are already removed.

**E09-b-pending (first action):** evaluate the complete-digest fixed rounds 8–15 from `1a1ea93` on ARM at 150 MHz. Compare with experiment 78's 29,205 full / 30,718 filter / 356,300 aggregate. On RISC-V, experiment 80 measured 28,636 full (+0.53%) and 368,834 aggregate, but remains provisional. Repeat if needed, then retain shared, split by ISA, or reject based on actual results. A source-unchanged filter gain should be attributed cautiously to layout.

Additional experiments, all without recorded hardware results:

1. **E09-b-tail-padding:** specialize header-tail rounds 4–15 explicitly: K4+`0x80000000`, K5..K14, K15+640. This differs from the already-tested second-hash fixed rounds and K16/W16, K17/W17 addends. Inspect whether constant propagation already does it. The expected opportunity is instruction scheduling/loads, not skipping nonce-dependent state evolution.
2. **E09-b-schedule-shapes:** generate explicit expressions for padding-heavy schedule ranges, starting with tail W20..W30 and second-hash W16..W31. Fold zero inputs and fixed terms, retaining all nonce/digest dependencies. Current precomputed W18/W19 and W31/W32 are the baseline. Keep one algebraically simple version beside the expanded reference. Reject expressions that merely duplicate compiler folding or expand register lifetimes without benefit.
3. **E09-b-round-groups:** generate 4- or 8-round groups with rotated variable roles, so source-level state moves can disappear without pointer-heavy round interfaces. Compare schedule-first versus small groups of expansion/round work, preserving the successful expanded schedule before revisiting any circular form. This is not the already-tested generic unroll attribute. Measure spills, code size and both ISAs; single-issue execution offers no automatic multi-lane speedup.
4. **E09-c-terminal:** calculate only the live result of the last executed filter round (zero-based t=60): e after 61 rounds is old d plus T1. That final operation does not need T2/new a or a complete state rotation. First inspect assembly: the compiler may already eliminate those dead computations. An explicit terminal helper is worthwhile only if emitted work decreases. Preserve the remaining prerequisite rounds; this is exact dependency trimming, not probabilistic rejection.
5. **E09-c-general-target:** extend high-word filtering to nonzero top-word targets with correct Bitcoin word ordering. Convert the computed numerical SHA word before ordered comparison: greater rejects, less proves qualification but still requires a full digest for a share record, equality requires lower words. The current equality-to-zero test needs no byte swap, but arbitrary ordering does. Dispatch by target shape once per job where useful, keeping general fallback coverage. Measure hard/easy targets separately; gains are workload-dependent.
6. **E09-b-two-lanes:** retain the original two-interleaved-nonce idea as a low-priority trial only after single-lane costs are known. It increases live state and can spill; there is no SIMD assumption. If attempted, allocate unique work and compare aggregate output, not round throughput.

The terminal proposal is derived from the SHA-256 state update equations; it is not a measured speedup. Validate it against the full-word oracle before timing. The existing `IV7 + e_after_61` relation remains the correctness basis. See [FIPS 180-4, section 6.2.2](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf). Additional folding must preserve modulo-2^32 addition; do not distribute sigma through modular addition as if carries did not exist.

Acceptance: all full digests and filter decisions agree with the independent oracle, rare candidates are complete and correct, affected software throughput improves repeatably, and total work does not materially regress. Include per-job setup cost when extra precomputation is added.

### E10 — Keep independent workers; gate hybrid pipelines with stage rates

E10-a is retained. E10 FIFO status polling once per 64 software nonces was tested on Hazard3 and rejected: software gained only 11 H/s while hardware lost 1,130 H/s and aggregate fell about 0.30%. No ARM result exists. Do not rerun it unchanged; use E08 transport improvements or an E03 batch entry point with a specific overhead hypothesis.

The original untested alternatives remain:

- **E10-b, software first hash → hardware second hash:** measure first-digest producer throughput before building a queue. Current complete software evaluations are around 30 k/s while the independent total is around 369 k/s. Removing only the software second hash would need an exceptionally asymmetric stage cost to close that gap. No isolated producer measurement currently establishes such a possibility. Stop if a generous measured upper bound cannot beat the retained independent-worker result.
- **E10-c, hardware first hash → software second hash:** likewise measure the software second-hash consumer, including target handling. The hardware's two-block production rate is not the Bitcoin completion rate. Reject on a limiting-stage estimate before investing in queue machinery.
- **E10-d, independent software work during DMA/waits:** this can add work without replacing the hardware stream, but current first-block DMA still waits synchronously. Profile usable slack first. Maintain a separate software nonce state and advance bounded round groups only while useful slack exists, then perform actual SHA status checks. Do not hold up a ready peripheral to complete a whole software hash. Measure net extra unique evaluations versus any hardware slowdown. Never interleave two messages on the SHA peripheral's single chaining state.

A software gain alone is not acceptance: reconcile issued, completed, cancelled and invalid work across both owners over a common measurement interval. E14 dynamic allocation is a higher-priority extension to E10-a than either replacement pipeline.

### E11 — Resolve clock feasibility, then qualify selected profiles

Retain the original recovery, separate-frequency comparison, USB/flash-clock checks and no-automatic-voltage-increase requirements. Replace the original 180/200 MHz endpoint and old >200 MHz authorization gate with the recorded staged authorization through 550 MHz. Keep stock 150 MHz as default.

Untested follow-up:

1. **E11-feasible:** use the pinned SDK's `check_sys_clock_khz` logic to enumerate supported PLL tuples without changing the active clock. Record actual XOSC/ref-divider and VCO limits. Reject unsynthesizable requests before flashing, and propose the next supported point only after this preflight. Do not repeat 350 MHz unchanged, silently round the frequency or alter PLL limit macros to force success. A synthesizable point is not a qualified operating point.
2. **E11-flash:** identify the carrier's flash part and current QMI timing/divider before further clock work. If necessary, test a conservative documented flash divider separately from CPU frequency changes. The prior missing-USB event does not prove a flash cause. Record sys/peri/USB/flash clocks and the requested regulator setting; do not infer measured voltage.
3. Revalidate the final chosen source at 150, then selected already-working 200/250/300 MHz points. The old sweep does not qualify the latest code. Maintain unchanged voltage and required USB timing. No higher point should follow an unexplained runtime failure.
4. Run at least the original 30-minute controlled qualification for selected profiles, with periodic complete digest checks, common-window accounting, USB/control stress and recorded faults/resets. A temperature-disabled soak establishes only observed functional behavior; thermal margin remains unknown. Restore the selected stock image after experiments.
5. Keep lower-clock hashes/joule, power gating and supply measurements as optional measured-efficiency work. They need actual input-power evidence; temperature and clock ratios cannot supply it.

Thermal repair/instrumentation remains E00 work if requested. Do not reinstate ADC acquisition during this optimization sequence. Preserve a current known-good recovery image and stop for physical recovery if USB/BOOTSEL access is unavailable.

### E12 — PGO, alternate compilers and limited assembly remain untested

No recorded PGO, alternate compiler or handwritten assembly experiment exists. Keep all original options, but select only a measured CPU bottleneck:

- PGO may help hot/cold placement or control branches; the core SHA round loops have predictable control flow. Export bare-metal profiles outside timing, use matching host tools, train on hard/easy targets and job changes, and compare the optimized non-instrumented image. The supported export approach is described in [GCC freestanding profiling](https://gcc.gnu.org/onlinedocs/gcc/Freestanding-Environments.html).
- Prefer a small assembly round group or M33 digest-handoff routine over a full SHA rewrite. Specify ABI, clobbers, stack, MMIO ordering and fallback; validate both wrapper builds and all enabled paths.
- Test another SDK-compatible compiler only when existing tool availability or separately authorized installation makes it practical, and E02 identifies a specific code-generation problem. Preserve the installed toolchains and separate compiler changes from algorithm changes.

Acceptance: reproducible gain that justifies maintenance. No ARM application-profile SHA instructions, MVE/NEON, RISC-V Zknh/RVV or other unsupported silicon features.

### E13 — Preserve speculative ideas, with a smaller mixed-ISA upside

All original ideas remain untested: mixed M33/Hazard3, PIO timing/sequence assistance, SIO interpolator/descriptor preparation, DMA chaining/rings, SRAM/cache isolation, shared-tail schedule work across permitted distinct headers, optional GPIO instrumentation and power gating. E06/E07/E11 now own the practical DMA/memory/power substeps; PIO/SIO require an identified task they can actually perform.

Mixed-ISA ranking changes with current evidence. Using the last paired worker rates, a hypothetical Hazard3 hardware owner plus M33 software worker gives `339,024 + 30,554 = 369,578` evaluations/s, only about 849/s or 0.23% above the reported homogeneous Hazard3 aggregate. This is illustrative arithmetic from separate runs, not a prediction or bound: shared buses, USB routing and mixed startup can change both rates. It weakens the original case based on a larger early software-ISA gap.

Defer mixed ISA unless fresh measurements show a larger transferable benefit or another concrete objective. If resumed, begin with the original reversible mixed-core heartbeat and explicit shared-memory ABI before mining. No boot-security, OTP or partition changes.

The original shared-tail/multiple-midstate idea deserves a bounded software-only feasibility study if authorized job headers support it: compute one nonce-dependent tail schedule for two distinct header midstates, process each state separately, and still compute independent second hashes. Measure schedule fraction and extra state/copy cost first. Count `(header identity, nonce)` uniqueness and respect version-rolling permissions. Hardware cannot accept these cached midstates, and synthetic compatible headers are not evidence of accepted pool work.

### E14 — Optimize finite useful work before final qualification

Recorded: fixed genesis work, tagged as stale, with even hardware nonces and odd software nonces. Both parity ranges stop before repeating in default dual-worker mode. This is better than silent wrap, but it is not dynamic job allocation or a complete lifecycle test. The optional single-core branch still increments a wrapping 32-bit nonce without an exhaustion stop and must not be used for indefinite unique-work claims.

**E14-chunks (new concrete implementation of the original range goal; high priority):** replace permanent parity ownership with disjoint finite chunks from one generation-tagged job. Use a 64-bit allocation cursor/end-exclusive range so `[0, 2^32)` and the final partial chunk are representable. Allocate once per chunk under an SDK synchronization primitive, never lock per nonce. Start with separate experiments for 4,096 and 65,536 nonces; cancellation can be checked more frequently than chunk acquisition. Precompute each worker's job state once, not for every chunk.

Why now: at the last paired Hazard3 worker rates, the hardware parity range exhausts after roughly `2^31 / 339,024 = 6,334 s` (about 106 minutes). Under constant-rate assumptions the software worker would have covered only about 8.8% of its parity range. The hardware exhaustion fault then stops the current run after approximately 54.4% of the full nonce space has been evaluated. These are calculations, not a recorded long run. Dynamic chunks improve completion of finite jobs and avoid stranded work; they do not promise a faster inner hash loop.

Test small ranges near `0xffffffff`, uneven worker speed, one parked worker, cancellation mid-chunk, reassignment rules, final partial error checks and final totals. Never reissue completed work. Natural exhaustion should become an explicit successful job-completion state once all allocated valid work is reconciled; keep it distinct from unexpected timeout/fault.

**E14-setup (expanded original):** benchmark setup, short jobs and frequent replacements. Cache first-block midstate by its actual 64 input bytes; recompute tail precomputations when bytes 64–75 change; target-only changes need correct comparator dispatch but no header-hash recomputation. Keep snapshots immutable. Only introduce a multi-job cache if measured job repetition amortizes lookup/storage cost.

**E14-final (original, still unfinished):** combine retained changes and measure the combination rather than adding individual percentages. Cover both architectures at stock clock, current DMA/software modes, hard/easy targets, short/long jobs, replacement/stop, slow/detached/reconnected USB, long run and recovery. Report stale, cancelled and invalid work separately from useful completed work. Live pool/network integration remains a separate feature; use controlled host jobs for qualification without claiming accepted shares.

## 5. Recommended next sequence

1. Resolve E09-b-pending on ARM at 150 MHz, then record the common retained source/artifacts. Do not stack another code experiment onto an unresolved paired result.
2. Complete E01-window/identity/rare coverage and fresh E02 measurements. Preserve old reported rates as historical data; establish the new common-window baseline before comparing it with future candidates.
3. Test E06-trigger on Hazard3. It has a concrete removable MMIO cost and small implementation scope. Retain ARM's CPU feeder unless a separately tested candidate wins there.
4. Try E09-b-tail-padding, then the most promising E09 schedule/round-group or E03 batch experiment indicated by E02. Test one subvariant at a time on both ISAs; use assembly to eliminate no-op candidates.
5. Implement E08 lifecycle/queue work and E14-chunks, with stress and finite-work accounting. These can move ahead of further kernel tuning when long runs or job changes are the next intended use.
6. Test E04 reporting/accounting and E07 bank placement against the established dual-worker baseline. Re-profile only when a new change alters the bottleneck.
7. Qualify the final stock configuration under E14, then perform selected E11 profile checks. E12 and E13 remain optional branches; E10-b/c require stage-rate evidence first.

For each proposed variant, write the hypothesis, parent artifact, changed feature, architecture, expected removable cost, resource cost and rejection rule before the trial. Treat estimated benefits throughout this document as hypotheses. Low-cost exact simplifications can justify small wins; complex changes need stronger end-to-end evidence.

## 6. Execution and evidence rules for future implementation

This document authorizes no implementation in the current planning task. When implementation is requested, preserve the repository's `AGENTS.md` workflow and the original experiment-record discipline:

- Use `apply_patch`, repository-local files and wrappers. Never modify the installed SDK/toolchains or machine configuration. Preserve unrelated changes.
- Build ARM and RISC-V warning-free before flashing a shared-source candidate. Use `./tools/build arm`, `./tools/build riscv`, and `./tools/cycle arm|riscv`; `cycle` itself rebuilds both. Use `./tools/analyze` for matching binary utilities. Run one flash/serial owner at a time.
- The current 8-second cycle default is too short for many seven-suite plus hardware/full-software/filter captures. Use an explicitly sufficient bounded capture, for example `MONITOR_SECONDS=45 ./tools/cycle arm`, and additional finite monitoring windows for measurement. Required-output timeout or nonzero command status is failure; a scheduled finite capture completing successfully is not a hardware fault.
- Reject any `TEST:FAIL`, `FAULT`, unexpected reset, missing required output, mismatch or invalid counted batch. Diagnose and repeat; never count a successful build/flash as a runtime pass. Preserve the exception that byte-identical loadable artifacts can reference their already-recorded validation rather than inventing new measurements.
- Archive logs under unique names before `*-latest.log` is overwritten. Record source/patch identity, UF2 hash, SDK/toolchain/flags, actual clocks, target/header IDs, nonce ranges, worker mode, run/window IDs, counts and elapsed times. Preserve full digests/fixture identities, checksums, stack/code/SRAM costs and variability. Use `temperature=disabled` throughout this sequence.
- Append every actual future result, including failures and rejections, to `perf_progress.md` and report it promptly. No performance entry is added for this planning-only change. After rejection, restore only the experiment's changes, rebuild both and validate the retained configuration.
- No `sudo`, OTP, erase, security, partition or unrequested physical/voltage changes. Keep the stock recovery image and USB recovery path.

Completion means the final retained combination has paired hardware validation, exact finite-work accounting, reproducible common-window performance, exercised candidate/control paths and explicit qualification limits. It does not require implementing every speculative branch or reaching 550 MHz.
