# Pico 2 Bitcoin miner: repository outcomes and operating guide

This document summarizes the completed work in this repository as of commit
`177cfac`. It separates measured results from hypotheses, describes the current
optimized firmware, and records the practical lessons from autonomous hardware
development and overclocking.

The authoritative experiment ledger is [perf_progress.md](perf_progress.md).
The optimization plans remain useful as design history:
[planned_optimizations.md](planned_optimizations.md),
[planned_optimizations_update.md](planned_optimizations_update.md), and
[planned_optimizations_update2.md](planned_optimizations_update2.md). The two
overclock plans are [overclock_test_plan.md](overclock_test_plan.md) and
[overclock_test_plan_divider_update.md](overclock_test_plan_divider_update.md).

## 1. Executive summary

- The project progressed from a blocking Pico SDK SHA-256 implementation at
  **34,441 H/s on Cortex-M33** and **32,084 H/s on Hazard3** to a validated,
  dual-core hybrid miner at stock 150 MHz measuring **360,308 H/s on M33** and
  **369,844 H/s on Hazard3**. The final stock Hazard3 recovery measured
  **369,929 H/s**. These rates count complete Bitcoin
  `SHA256(SHA256(80-byte header))` evaluations, not compression rounds.
- The stock-clock improvement is approximately **10.46x for M33** and
  **11.53x for Hazard3** when the final aggregate hybrid rate is compared with
  the original same-architecture blocking implementation.
- The final default is a hybrid design: core 1 uses the native RP2350 SHA-256
  engine while core 0 computes an independent optimized software SHA-256
  stream. The workers scan disjoint even and odd nonces.
- The most important hardware-path improvements were retaining the SHA lock,
  direct preformatted word feeding, one wait per block, unrolled MMIO writes,
  register-only digest handoff, direct constant padding, lazy high-word target
  rejection, batched accounting, architecture-specific error handling, and a
  persistent first-block DMA trigger on Hazard3.
- The most important aggregate gain was using both cores for useful work. At
  stock clock the hardware worker contributes about 91.5% and the software
  worker about 8.5% of total throughput.
- The software path uses a cached first-block midstate, fixed-header partial
  round/schedule precomputation, an exact round-61 high-word filter, SRAM code
  placement, and architecture-specific code shapes. Hazard3 gained materially
  from rotated-role round groups; the same idea was not retained on M33.
- Hash rate scales almost perfectly with system clock at a valid operating
  point: about **2,465.5 aggregate H/s/MHz** for the optimized Hazard3 hybrid
  image. Voltage did not make a valid point faster; it only changed whether the
  device booted and computed correctly.
- Flash/QMI speed was a distinct stability constraint. At 516 MHz and 1.60 V,
  QMI divider 4 (129 MHz SCK) failed twice while divider 5 (103.2 MHz) passed.
  Moving from divider 3 to 4 also transformed 420 MHz from failure into a clean
  pass at only 1.30 V. Core voltage and flash divider must therefore be searched
  as separate variables.
- The highest **fully validated sustained** Hazard3 hybrid result in the
  archived logs is **1,361,295 H/s at 552 MHz, requested/read-back 1.60 V, QMI
  divider 5**. The 558 MHz delayed run passed BOOT, all tests, the 4,096-vector
  oracle, and standalone benchmarks, but ended before sustained progress
  windows, so it is only a screen pass. The next exact point, 564 MHz, produced
  no firmware output.
- Hardware-only Hazard3 reached **1,256,632 H/s at 558 MHz**, but was still
  1.23% slower than the hybrid miner at 516 MHz. Disabling the software worker
  is therefore not the throughput-optimal deployment on this device.
- Temperature is explicitly disabled. The on-chip ADC returned saturated
  samples and error flags on every tested channel, so no thermal result is
  valid and no run in this repository is thermally qualified.
- The firmware is a genuine proof-of-work computation and target checker, but
  it mines a fixed, stale genesis-block job. It has no Stratum client, pool
  protocol, job replacement, or Bitcoin P2P integration.

## 2. What was built

### 2.1 Autonomous physical-hardware loop

The repository turns VS Code/Codex and a USB-connected Pico 2 into a bounded,
machine-verifiable edit/build/flash/observe loop:

1. `./tools/doctor` discovers the Pico SDK, CMake, Ninja, both compilers,
   `picotool`, USB serial, and Linux group access.
2. `./tools/build arm` builds `rp2350-arm-s`; `./tools/build riscv` builds
   `rp2350-riscv`. The wrappers find extension-managed dependencies under
   `~/.pico-sdk` and never require direct compiler invocation.
3. `./tools/cycle arm|riscv` builds **both** architectures, flashes the selected
   image, captures finite serial output, validates the expected firmware
   identity/configuration, and returns nonzero unless the complete contract
   passes.
4. `./tools/monitor --seconds N` provides a finite serial observation command.
   It rejects device `TEST:FAIL`/`FAULT`, duplicate or stale BOOT records,
   wrong source/architecture/clock/voltage/divider/mode, malformed accounting,
   missing tests, and incomplete progress windows.
5. Every build embeds a 12-character identity derived from the committed
   `CMakeLists.txt`, `src`, and `tools` trees. The current code identity is
   **`23ebcc93979f`**. Run IDs combine the chip ID and a watchdog-scratch boot
   sequence, preventing stale serial output from being accepted.

VS Code exposes the same operations in [.vscode/tasks.json](.vscode/tasks.json),
and recommends the Raspberry Pi Pico and Codex extensions. A single USB cable
supports reset-to-BOOTSEL, UF2 loading, verification, and CDC serial feedback.
Source-level halt/breakpoint debugging would additionally require SWD hardware.

Linux needs access to both Pico USB identities and `/dev/ttyACM*`. In a VM, a
vendor-only `2e8a` passthrough filter is important because the device changes
product identity between application and BOOTSEL modes. Details are in
[README.md](README.md).

### 2.2 Validation contract

Every accepted hardware result passed, unless a table explicitly says
otherwise:

- NIST SHA-256 empty-string and `abc` vectors;
- the SHA peripheral sticky-error behavior test;
- **4,096 independently generated double-SHA-256 oracle vectors**, checked by
  both hardware and optimized software paths;
- compact-target boundary cases;
- mining decision-path cases, including common rejection, exact candidate,
  generic target comparison, and tail-takeover accounting;
- the Bitcoin genesis block hash;
- discovery of the real genesis nonce `2083236893` from a bounded search;
- standalone hardware, full-software, and filter benchmarks;
- at least five ordered mining progress reports and five synchronized windows
  for a strict sustained-rate pass.

This oracle caught silent core-logic errors at 348 MHz/1.15 V even though the
device booted and simpler SHA tests passed. A booting board is not evidence of
correct overclocked computation.

## 3. Current optimized firmware

The final implementation is primarily in [src/main.c](src/main.c) and
[src/software_sha256.c](src/software_sha256.c), with build policy in
[CMakeLists.txt](CMakeLists.txt).

### 3.1 Clock, voltage, flash, and identity setup

Before normal work begins, `main()`:

- selects an allow-listed regulator setting from 1.10 through 1.60 V;
- explicitly unlocks the SDK voltage limit above 1.30 V;
- configures QMI divider 3, 4, or 5 from an SRAM-resident transition helper;
- validates the requested PLL point, changes `clk_sys`, and keeps USB and
  peripheral clocks at 48 MHz;
- verifies voltage readback, actual system clock, divider readback, and derived
  QMI SCK, with a campaign ceiling of 130 MHz;
- emits a complete BOOT record containing source, run, architecture, mining
  mode, clocks, PLL tuple, voltage, divider, chip, package, and revision;
- releases the core-0 mining start gate only after BOOT telemetry is queued.

That last gate was important for reliable delayed startup at high clock: the
software worker cannot begin contending for resources while clock transition
and BOOT reporting are still in progress.

### 3.2 Hardware SHA-256 worker on core 1

The hardware path performs a complete Bitcoin double hash for every nonce:

- The 80-byte header is converted once to numeric big-endian words. Only the
  nonce word changes per iteration.
- The boot-ROM SHA lock is acquired once per job, not once per hash.
- The first 64-byte block is fed directly. Hazard3 uses a configured DMA
  channel and a single read-address-trigger write per nonce; M33 uses explicit
  unrolled MMIO writes.
- The four remaining header words, `0x80000000`, zeros, and the 640-bit length
  are emitted directly instead of constructing and copying a padded block.
- The first digest is captured into registers and immediately fed to the
  second SHA operation with fixed 32-byte-message padding.
- The common target path reads only the most-significant result word. Full
  byte reversal, eight-word comparison, digest capture, and FIFO publication
  occur only for a rare candidate.
- ARM uses four physically unrolled nonce bodies and a shared cold candidate
  helper. Hazard3 uses one body; factor 2 was neutral there.
- ARM checks the SHA peripheral error for every nonce. Hazard3 safely batches
  the sticky error observation to a reporting boundary, a verified
  architecture-specific choice.

### 3.3 Software SHA-256 worker on core 0

The independent software path is not a reference-only fallback; it contributes
about 31.5 kH/s at 150 MHz:

- It computes and caches the first 64-byte header midstate once.
- It caches fixed header-tail words, state after rounds 0–2, the constant part
  of round 3, schedule words 16/17, round addends, and fixed components of
  schedule words 18/19 and 31/32.
- The compression helpers execute from SRAM. Moving code alone helped; moving
  constants or whole workers indiscriminately did not.
- For ordinary difficulty-1 rejection, it computes the exact digest high word
  through the mathematically sufficient terminal state around round 61. A
  nonzero high word rejects the nonce without finishing/copying the full
  digest. A possible candidate always falls back to the complete digest and
  generic 256-bit comparison.
- Fixed padding rounds and architecture-specific schedule/digest layouts avoid
  repeated work. Hazard3 uses rotated-role four-round groups in the hot filter;
  M33 retains the compiler's O3 form because smaller O2/Os shapes were slower
  or failed static hot-loop gates.

### 3.4 Work division, reporting, and nonce-space completion

In default hybrid mode, core 1 scans even nonces and core 0 scans odd nonces.
This has no hot shared allocator. FIFO messages carry progress, shares, faults,
and bounded synchronization. Four hardware reports form one common measurement
window so both workers are compared over the same elapsed interval.

The hardware worker finishes its parity half much earlier than the software
worker. The retained two-phase takeover protocol therefore stops core 0 at an
exact odd frontier and transfers the unprocessed odd suffix to core 1. Startup
tests prove exact `2^32` accounting and no overlap. Normal-phase performance was
measurement-neutral. The roughly 106-minute stock-clock end-to-end transition
was not endurance-tested, so full-space completion is protocol/arithmetic
qualified rather than long-duration hardware qualified.

`MINER_HARDWARE_ONLY=1` is a supported experimental mode. It gives core 1 a
unit nonce stride and requires all software mining counts to remain zero, while
still retaining the software KAT/oracle and standalone benchmarks.

## 4. Principal optimizations and their final impact

### 4.1 Early hardware-path progression on M33

This table shows isolated historical checkpoints at 150 MHz. Benchmark and
sustained figures are not interchangeable; both are included where recorded.

| Retained step | Hardware benchmark | Sustained mining | Impact from previous relevant step |
| --- | ---: | ---: | --- |
| Pico SDK blocking API | 34,441 H/s | ~34,343 H/s | Baseline |
| Retain lock + direct padded blocks | 103,800 H/s | ~102,787 H/s | 3.01x benchmark baseline |
| Poll once/block + 16 unrolled writes | 169,096 H/s | ~166,426 H/s | +62.9% benchmark |
| Inline nonce/digest transfer | 207,736 H/s | ~203,719 H/s | +22.9% benchmark |
| Target-wide `-O3` | 281,929 H/s | ~254,592 H/s | +35.7% benchmark |
| Lazy final-result/high-word read | 294,091 H/s | ~289,501 H/s | +4.3% benchmark; +13.7% mining |
| Numeric SHA words | 303,003 H/s | 298,082 H/s | +3.03% / +2.98% |
| Register-only first-digest handoff | 308,608 H/s | 306,029 H/s | +1.85% / +2.67% |
| Constant header-tail feeder | 317,088 H/s | 311,098 H/s | +2.75% / +1.66% |
| Fast zero-top-word target rejection | 316,422 H/s | 315,026 H/s | Mining +1.26%; benchmark excludes target path |
| Batched 64-bit accounting | 324,632 H/s | 320,406 H/s | +2.59% / +1.71% |

The whole-image SRAM experiment was unchanged within noise. It was rejected;
the XIP cache already served the then-small hot loop effectively.

### 4.2 Later retained design changes

| Optimization | Final disposition and measured effect |
| --- | --- |
| Independent 4,096-case oracle | Retained. Throughput-neutral infrastructure that made aggressive specialization and overclocking credible; it detected silent overclock errors missed by simpler tests. |
| Sticky SHA error batching | Retained only on Hazard3. ARM batching reduced sustained performance; ARM keeps per-nonce checks. |
| Post-START ready-poll removal | Retained on both after repeated validation; removed redundant synchronization while preserving ordered MMIO behavior. |
| Persistent first-block DMA | Retained only on Hazard3. Full-block DMA regressed the kernel by 2.69%; one fixed first-block DMA plus CPU tail/digest feeding was better. |
| Core-1 hardware worker | Retained. Moving the hardware loop off core 0 improved Hazard3 sustained hardware throughput by about 1.39% and enabled independent useful work on core 0. |
| Independent software worker | Retained. Initially added +4.78% aggregate on Hazard3 and +6.42% on M33 over hardware-only mining; later software optimization increased its contribution further. |
| Software compression in SRAM | Retained. Aggregate +1.58% Hazard3 and +1.00% M33 at the measured checkpoint; constants-in-SRAM was noise and rejected. |
| Expanded software schedule and unrolling | Retained. Expanded scheduling improved the early Hazard3 software path by 26.94%; the final code adds fixed-word/round partials and architecture splits. |
| Exact high-word filter | Retained. It rejects the overwhelmingly common non-share after only the required terminal state, with a full-digest fallback for candidates. |
| Fixed tail and digest rounds | Retained where measured beneficial, often architecture-specific. Late steps improved the software worker by roughly 0.5–3.9% each but aggregate by only about 0.05–0.30% because software is the smaller component. |
| One-second/four-report synchronized windows | Retained. It improved measurement quality and reduced the chance that startup, reporting phase, or unequal time bases distorted comparisons. |
| ARM hardware batch factors | Factor 2 added +0.348% hardware; factor 4 added another +0.582%. Factor 8 lost 0.192% hardware and was rejected. Final ARM factor is 4; Hazard3 factor is 1. |
| ARM shared cold candidate path | Retained. Worker shrank by 744 bytes, frame by 32 bytes, and measured hardware rose 0.241%; it avoided duplicating rare share logic across four nonce bodies. |
| Hazard3 rotated-role filter | Retained. Exact-filter benchmark +4.796%, software mining +4.774%, aggregate +0.412%. M33 did not retain the same transformation. |
| Two-phase odd-tail takeover | Retained. Normal throughput was neutral within 0.02%; it fixes end-of-range load imbalance without adding a hot shared allocator. |
| Hardware-only mode | Retained as an experimental/configuration option, not as the fastest miner. At 516 MHz it was 8.66% below hybrid aggregate. |
| Delayed core-0 start gate | Retained in the final source. It makes clock/BOOT sequencing deterministic and was used for the valid delayed high-clock reruns. |

### 4.3 Important rejected paths

Negative results were as useful as positive ones:

- whole-image SRAM, hot `main` in SRAM, hardware worker in ordinary SRAM, and
  filter scratch-X placement were neutral or slower because XIP locality and
  SRAM/bus contention matter more than a simple “SRAM is faster” model;
- broad LTO, source-only LTO, `restrict`, Hazard3 branch-cost/code-hoisting/
  if-conversion flags, and broad label alignment were neutral, byte-identical,
  failed to link, or regressed;
- next-nonce preparation, reusable padded blocks, full-block DMA, dynamic nonce
  chunks, hot telemetry queues, 32-bit software counters, software batch 8,
  Hazard3 hardware batch 2, and ARM hardware batch 8 all failed their measured
  benefit gates;
- ARM 16- and 32-byte worker alignment were repeatably neutral/slightly worse;
- a smaller M33 O2 filter was 0.503% slower, and Os reintroduced compact loops
  and calls, so O3 remains correct for the hot path;
- specialization that narrowed generic target semantics was restored even when
  it looked fast. Fast paths must preserve exact fallback behavior.

Do not repeat these unchanged. [planned_optimizations_update2.md](planned_optimizations_update2.md)
explains the static and measurement reasons in detail.

## 5. Performance results

### 5.1 Final stock-clock comparison

These are first-seven common-window medians from retained candidate 114 at
150 MHz, 1.10 V, temperature disabled. Standalone rates are separately timed.

| ISA | Aggregate mining | Hardware component | Software component | Standalone hardware | Standalone software full | Standalone exact filter |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ARM Cortex-M33 | 360,308 H/s | 328,871 H/s | 31,438 H/s | 331,821 H/s | 30,364 H/s | 31,578 H/s |
| Hazard3 RISC-V | 369,844 H/s | 338,331 H/s | 31,510 H/s | 343,992 H/s | 28,429 H/s | 31,685 H/s |

The last stock recovery after the complete overclock campaign measured Hazard3
at **369,929 aggregate / 338,338 hardware / 31,591 software H/s**, confirming
that the device returned to baseline.

Interpretation:

- Hazard3 is about **2.65% faster in aggregate** than M33 in the retained
  stock configuration.
- Hazard3 is clearly better at driving the hardware engine, while M33's full
  software digest benchmark is faster. Hazard3's optimized exact-filter path
  is slightly faster than M33 and is the more relevant software mining metric.
- A theoretical mixed “Hazard3 hardware + M33 software” combination was only
  about 0.37% above homogeneous Hazard3 in arithmetic and was not implemented;
  it is not worth boot/ISA complexity without a new use case.

### 5.2 Validated clock/voltage/divider frontier

The following representative Hazard3 hybrid points passed exact identity, all
correctness gates, standalone benchmarks, and strict sustained capture. These
are not recommended voltages; they are the minimum confirmed selector where a
transition was actually bracketed, or the tested selector for that point.

| `clk_sys` | VREG | QMI divider | QMI SCK | Aggregate | Hardware | Software | Key result |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 150 MHz | 1.10 V | 3 | 50.0 MHz | 369,930 H/s | 338,339 | 31,591 | Stock control |
| 300 MHz | 1.10 V | 3 | 100.0 MHz | 739,679 H/s | 676,659 | 63,022 | 99.999% aggregate scaling |
| 348 MHz | 1.20 V | 3 | 116.0 MHz | 857,995 H/s | 784,895 | 73,099 | 1.15 V silently failed oracle twice |
| 396 MHz | 1.60 V | 3 | 132.0 MHz | 976,344 H/s | 893,143 | 83,201 | Historical pre-divider-guard pass; 1.40 V failed |
| 420 MHz | 1.30 V | 4 | 105.0 MHz | 1,035,719 H/s | 947,300 | 88,421 | 1.25 V failed twice |
| 444 MHz | 1.40 V | 4 | 111.0 MHz | 1,094,633 H/s | 1,001,137 | 93,496 | 1.30 V failed twice |
| 468 MHz | 1.60 V | 4 | 117.0 MHz | 1,153,797 H/s | 1,055,253 | 98,545 | Clean pass |
| 516 MHz | 1.60 V | 5 | 103.2 MHz | 1,272,262 H/s | 1,163,606 | 108,659 | Divider 4 failed twice |

The 396 MHz/divider-3 point predates the 130 MHz QMI campaign guard and should
not be copied into a deployment profile. It is retained as evidence that early
failures were not a simple core-frequency ceiling.

### 5.3 Final delayed-start high-clock matrix

All points below used final delayed-start source identity `23ebcc93979f`,
Hazard3 hybrid mode, requested/read-back 1.60 V, and divider 5. Sustained values
are first-seven progress medians; MAD is median absolute deviation.

| Clock | QMI SCK | Aggregate | Hardware | Software | Windows | Aggregate MAD | Status |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 516 MHz | 103.2 MHz | 1,272,516 | 1,163,814 | 108,714 | 34 | 27 | Full pass |
| 520 MHz | 104.0 MHz | 1,282,393 | 1,172,859 | 109,552 | 17 | 36 | Full pass |
| 522 MHz | 104.4 MHz | 1,287,312 | 1,177,363 | 109,971 | 17 | 128 | Full pass |
| 524 MHz | 104.8 MHz | 1,292,257 | 1,181,869 | 110,400 | 17 | 24 | Full pass |
| 528 MHz | 105.6 MHz | 1,302,088 | 1,190,822 | 111,239 | 17 | 24 | Full pass |
| 532 MHz | 106.4 MHz | 1,311,982 | 1,199,896 | 112,070 | 17 | 232 | Full pass |
| 534 MHz | 106.8 MHz | 1,316,923 | 1,204,439 | 112,505 | 17 | 62 | Full pass |
| 540 MHz | 108.0 MHz | 1,331,592 | 1,217,804 | 113,760 | 18 | 137 | Full pass |
| 546 MHz | 109.2 MHz | 1,346,493 | 1,231,487 | 115,029 | 18 | 109 | Full pass |
| **552 MHz** | **110.4 MHz** | **1,361,295** | **1,245,011** | **116,297** | **18** | **27** | **Highest full pass** |
| 558 MHz | 111.6 MHz | — | — | — | 0 | — | BOOT/oracle/bench pass; sustained capture incomplete |
| 564 MHz | 112.8 MHz | — | — | — | 0 | — | No BOOT/runtime output |

The complete log names and SHA-256 hashes are recorded in the “Delayed
dual-core valid reruns” subsection of [perf_progress.md](perf_progress.md).
The 558 MHz standalone rates were 1,285,562 hardware, 106,507 full software,
and 118,194 filter H/s, but these must not be presented as sustained aggregate
mining performance.

### 5.4 Hardware-only and ARM high-clock results

| ISA/mode | Clock, voltage, divider | Sustained result | Interpretation |
| --- | --- | ---: | --- |
| Hazard3 hardware-only | 516 MHz, 1.60 V, d5 | 1,162,045 H/s | 8.66% below hybrid aggregate at same clock |
| Hazard3 hardware-only | 558 MHz, 1.60 V, d5 | 1,256,632 H/s | Maximum full hardware-only pass; still 1.23% below hybrid 516 MHz |
| Hazard3 hardware-only | 564 MHz, 1.60 V, d5 | — | No BOOT/runtime output |
| M33 hardware-only | 516 MHz, 1.60 V, d5 | 1,132,023 H/s | Full post-recovery pass |
| M33 hardware-only | 528–552 MHz, 1.60 V, d5 | screen passes | BOOT/oracle/mining observed; not recorded as comparable full captures |
| M33 hardware-only | 558 MHz, 1.60 V, d5 | — | Verified flash, then no BOOT/runtime output |

Some earlier ARM attempts were invalid because `picotool` returned success
without visible load/verify progress while the board remained in a hung
application state. They are correctly classified `INVALID_HOST_FLASH_NOOP` and
must not be used as stability evidence.

## 6. RP2350 and core-specific knowledge extracted

### 6.1 Hardware and memory behavior

- The SHA-256 peripheral's first 15 WDATA writes can be streamed once ready;
  the 16th starts compression. Polling before every word wastes substantial
  CPU time.
- `START` plus ordered MMIO establishes the required state; the retained path
  safely removes a redundant post-START ready poll.
- `ERR_WDATA_NOT_RDY` is sticky across `START` and can be cleared explicitly.
  This permits Hazard3 report-boundary checking, but M33 measured better with a
  per-nonce observation.
- The native engine is still CPU-control-heavy. At stock clock, the final
  standalone hardware path is about 344 kH/s, and stage profiling attributed
  roughly 483–519 cycles/hash to setup, feeding, digest handoff, second hash,
  target/error handling, and loop overhead.
- XIP is not inherently the bottleneck at stock clock. Whole-image SRAM was
  neutral, and relocating the complete M33 worker to SRAM slowed aggregate
  throughput by 0.261%. Shared SRAM banks, stacks, code, data, and DMA can
  contend across two cores.
- XIP footprint and layout still matter. ARM factor 4 and a smaller cold path
  helped; factor 8 crossed a fetch/locality threshold and regressed. Arbitrary
  alignment was not beneficial.
- QMI flash frequency becomes a first-order boot/stability variable at high
  `clk_sys`. The divider must be selected before increasing the system clock,
  and the transition helper itself must execute from SRAM.
- USB and peripheral clocks can and should remain fixed at 48 MHz while
  `clk_sys` changes. Otherwise loss of serial could be confused with a core or
  flash failure.
- Exact PLL realizability matters. Requested round-number steps such as 560,
  562, or 566 MHz may not exist; use `tools/pll_catalog.py` before building.

### 6.2 Cortex-M33 versus Hazard3

| Property | Cortex-M33 | Hazard3 RISC-V |
| --- | --- | --- |
| Final stock aggregate | 360.3 kH/s | 369.8 kH/s; about 2.65% faster |
| Hardware engine driving | 328.9 kH/s sustained component | 338.3 kH/s; better overall |
| Full software SHA | 30.4 kH/s standalone | 28.4 kH/s standalone |
| Exact software filter | 31.6 kH/s | 31.7 kH/s after rotated-role optimization |
| First-block feed | Explicit unrolled MMIO is best | Persistent DMA with one trigger write is best |
| Hardware loop batching | Factor 4 retained | Factor 1 retained; factor 2 neutral |
| Error checking | Per nonce | Sticky error checked at report boundary |
| Digest handoff | Seven words stayed in registers, one spill in an early profile | All eight stayed in registers |
| Code-shape sensitivity | Strong XIP footprint/layout sensitivity | Larger software frames/code, but rotated roles remove many state moves |
| High-clock evidence | Hardware-only full pass at 516 MHz; 558 MHz failed to emit runtime output | Hybrid full pass through 552 MHz; 558 MHz screen pass; 564 MHz failed |

General guidance follows from this asymmetry: do not force one source shape or
compiler policy on both ISAs. Inspect final linked disassembly, keep
architecture-specific branches when the measured mechanisms differ, and judge
aggregate impact rather than an isolated microbenchmark alone.

### 6.3 Package and temperature caveat

The operator identified the physical token as **RP2350B**, but every accepted
run reported `SYSINFO.PACKAGE_SEL=1`. The
[official RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
defines 1 as QFN-60/RP2350A and 0 as QFN-80/RP2350B. The current `pico2` build,
static assertion, BOOT string, and runtime package check therefore target
**RP2350A** and would reject a conforming B-package readback. This is an
unresolved hardware-description conflict: either the physical identification
or the association between this measurement history and the presently named
token needs correction. Before deployment, inspect the package/board and
reconcile it with the register. If it truly is RP2350B, create the proper board
definition (`PICO_RP2350A=0`) and revalidate both architectures; do not merely
remove the runtime identity check. Also verify the carrier pinout, flash
interface, LED mapping, and ADC wiring.

All ADC inputs tested during this work returned raw 4095 with current and
sticky conversion errors. This was ADC-wide, not only a wrong temperature
channel. Temperature was consequently disabled and must remain described as
unknown. An external sensor or repaired/verified ADC supply/reference is needed
for thermal qualification.

## 7. Overclocking lessons and a faster repeat-test method

### 7.1 What the failures mean

- **BOOT_FAIL** means verified flashing was followed by no runtime USB. It does
  not distinguish core, flash, clock-transition, regulator, or early firmware
  failure.
- **RESET_OR_LINK_FAIL** means USB may enumerate but no complete expected
  firmware stream arrives. It is not a hashrate result.
- **CORRECTNESS_FAIL** is more serious than a missing link because computation
  completed but was wrong. The reproduced 348 MHz/1.15 V oracle failures at
  different vectors prove real, silent core-logic marginality.
- **INVALID_HOST_FLASH_NOOP** means no new test occurred. A zero exit from
  `picotool` is insufficient; visible load/verify progress, a new run ID, and
  the expected source/configuration identity are mandatory.
- A short screen pass proves only its recorded gates. It cannot be promoted to
  a sustained rate or stability boundary without the required windows.

### 7.2 Streamlined search procedure

For future campaigns:

1. Start from an archived, hash-verified 150 MHz/1.10 V recovery UF2 and record
   the board's stock baseline.
2. Precompute exact PLL points. Choose the smallest divider that keeps QMI SCK
   below a conservative limit; the evidence here supports staying near or below
   roughly 110–120 MHz when exploring the upper core range, not merely below
   the hard 130 MHz wrapper guard.
3. Hold two variables fixed. Search frequency at one voltage/divider, or
   voltage at one frequency/divider. Never change code, clock, voltage, and
   flash divider in one attribution step.
4. When the target is maximum speed and the board is cooled, begin at the
   authorized upper voltage, find the frequency ceiling with coarse exact-PLL
   steps, then bracket it with the available intermediate points. Only after
   `f_max` is established should voltage descend at that fixed point.
5. A flash-related-looking failure should immediately trigger a paired divider
   control at the same clock and voltage. This would have avoided the long,
   misleading divider-3 bisection in which even 420 MHz failed at 1.60 V.
6. Use a two-tier gate:
   - screen: exact BOOT identity, all eight tests, full oracle, benchmarks, and
     enough output to prove mining began;
   - measure: at least five synchronized windows, with seven-window median,
     range, and MAD used for comparisons.
7. Repeat every new transition boundary once with byte-identical artifacts.
   Repeated failures at the same point are evidence; a single failure after a
   questionable host operation is not.
8. After a hang, require a visibly verified stock recovery and a strict stock
   pass before the next experiment. If BOOTSEL intervention is needed, record
   it explicitly.
9. Archive the complete serial stream under a unique experiment name and hash
   it immediately. Do not rely on `arm-latest.log` or `riscv-latest.log`, which
   the next run overwrites.
10. Compare rates only within the same committed source identity, mining mode,
    report/window definition, clock, and first-seven-median method.
11. Keep the 4,096-case oracle at every point. Never replace it with only NIST
    vectors, a successful boot, or a plausible hashrate.
12. Return to stock after unsafe-voltage runs. Values above 1.30 V require the
    unsafe-voltage unlock and are a materially different risk class. Cooling
    does not eliminate electrical degradation risk.

Automating steps 2, 6, 7, and 9 in a campaign driver would save the most time.
The driver should reject unsynthesizable PLL points before building, choose the
divider from an explicit SCK policy, require visible `picotool` verification,
rename/hash logs atomically, and stop on any identity or oracle mismatch.

## 8. Deployment recommendations

### 8.1 Recommended profiles

| Purpose | Architecture and configuration | Why |
| --- | --- | --- |
| Safe default/development | Hazard3 hybrid, 150 MHz, 1.10 V, divider 3 | Fastest retained stock ISA; repeatedly recovered and validated; 369.9 kH/s |
| Conservative accelerated experiment | Hazard3 hybrid, 300 MHz, 1.10 V, divider 3 | Fully validated with virtually perfect 2x scaling; still an overclock and not thermally qualified |
| High-clock measured profile | Hazard3 hybrid, 552 MHz, 1.60 V, divider 5 | Highest fully captured result, 1.361295 MH/s; unsafe voltage and no thermal qualification |
| Diagnostic single-worker mode | Hazard3 hardware-only via `MINER_HARDWARE_ONLY=1` | Isolates native-engine behavior; lower total throughput than hybrid |
| M33 compatibility | M33 hybrid at stock | Fully supported and validated; useful for ARM-specific development, but slower than Hazard3 stock |

The high-clock profile is evidence for this one cooled sample, not a portable
rating for RP2350 devices. For unattended or long-duration deployment, use the
stock profile until temperature and supply integrity are externally measured.

### 8.2 Build and run examples

Stock Hazard3 validation:

```sh
./tools/doctor
./tools/cycle riscv
```

Validated 300 MHz experimental profile:

```sh
MINER_SYS_CLOCK_KHZ=300000 \
MINER_VREG_MV=1100 \
MINER_QMI_CLKDIV=3 \
./tools/cycle riscv
```

Highest fully measured repository profile, only with the same explicit
overclock authorization, cooling, and recovery preparation used in the
campaign:

```sh
MINER_SYS_CLOCK_KHZ=552000 \
MINER_VREG_MV=1600 \
MINER_QMI_CLKDIV=5 \
MONITOR_SECONDS=50 \
./tools/cycle riscv
```

The wrapper always builds both architectures before flashing. Do not bypass it
for convenience: cross-build success, source identity, strict monitoring, and
failure propagation are part of the result.

### 8.3 Work needed for a real miner deployment

The current code demonstrates real hashing and target validation but repeats a
fixed stale header. A pool-capable implementation still needs:

- a host or device-side Stratum/job transport;
- generation-tagged, race-free job replacement and cancellation;
- extranonce/merkle-root and nTime handling;
- target updates and share submission;
- reconnect/watchdog behavior and durable operational telemetry;
- a benchmark for job-change latency, since steady-state results amortize setup;
- external temperature and supply monitoring for any sustained overclock;
- security review of all data received from a network.

Keep networking/control work outside the hash hot paths. Apply changes only at
existing report/cancellation boundaries or through generation-tagged snapshots;
the tested per-hash queues and frequent polling regressed performance.

## 9. Additional lessons

- Optimize the dominant component first. A 1% hardware improvement adds about
  0.91–0.92% aggregate; even a 5% software improvement adds only about 0.4%.
- Measure the real workload. Several changes improved code size or a standalone
  helper while reducing synchronized aggregate throughput.
- Compiler output, not source elegance, decides whether a micro-optimization is
  worth flashing. Byte-identical variants should be closed at build time.
- Preserve generic correctness behind common-case shortcuts. Bitcoin target
  comparison and candidate digest capture are rare, but must remain exact.
- Separate correctness infrastructure from performance claims. The oracle,
  source identity, run identity, strict serial grammar, recovery images, and
  log hashes made failures classifiable and prevented stale data from becoming
  “results.”
- Treat host state as part of the experiment. USB passthrough, BOOTSEL identity,
  CDC re-enumeration, permissions, capture duration, and `picotool` behavior can
  invalidate an otherwise correct firmware experiment.
- Git commits between functional candidates made restoration exact. Rejected
  variants could be removed while retaining their source identity, artifacts,
  logs, and measured reason for rejection.
- Final-code results are sensitive to architecture, layout, clock configuration,
  and measurement protocol. Historical rates in [README.md](README.md) describe
  an early milestone and are now stale; use this summary and
  [perf_progress.md](perf_progress.md) for current numbers.

## 10. Evidence map

- Complete chronological results, failures, firmware identities, log paths,
  hashes, and decisions: [perf_progress.md](perf_progress.md)
- Original platform and experiment design: [planned_optimizations.md](planned_optimizations.md)
- Mid-campaign status and carried-forward work: [planned_optimizations_update.md](planned_optimizations_update.md)
- Final source-optimization sequence and closed ideas: [planned_optimizations_update2.md](planned_optimizations_update2.md)
- Initial adaptive voltage/frequency search: [overclock_test_plan.md](overclock_test_plan.md)
- QMI-divider-aware revised search: [overclock_test_plan_divider_update.md](overclock_test_plan_divider_update.md)
- Build/flash/USB setup: [README.md](README.md), [AGENTS.md](AGENTS.md), and
  [.codex/SESSION_CONTINUITY.md](.codex/SESSION_CONTINUITY.md)
- Firmware: [src/main.c](src/main.c), [src/software_sha256.c](src/software_sha256.c),
  and [src/software_sha256.h](src/software_sha256.h)
- Host control and validation: [tools/build](tools/build),
  [tools/cycle](tools/cycle), [tools/monitor.py](tools/monitor.py),
  [tools/analyze](tools/analyze), and [tools/pll_catalog.py](tools/pll_catalog.py)
- Stock recovery images: [artifacts/recovery](artifacts/recovery)
