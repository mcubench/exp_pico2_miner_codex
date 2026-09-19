# Pico 2 development, optimization, and testing

Concise guidance for agent-assisted development on USB-connected Raspberry Pi
Pico 2 and other RP2350 boards. Apply it to embedded workloads generally; keep
application-specific correctness tests and performance metrics in the project.

## Know the target

- RP2350 can run dual Cortex-M33 cores or dual Hazard3 RISC-V cores. Build and
  test both ISAs when source is shared; never assume the same code shape is
  optimal for both.
- Confirm the board, package, flash, LED wiring, and exposed GPIO/ADC resources.
  RP2350A is QFN-60; RP2350B is QFN-80. Check the runtime package register
  against the compile-time board definition.
- Treat chip revision, board design, flash part, cooling, and power integrity as
  part of the test identity. Results from one sample are not device ratings.

## Use a bounded autonomous loop

1. Diagnose SDK, CMake, Ninja, both toolchains, `picotool`, USB serial, and
   permissions before editing.
2. Build both M33 and Hazard3 images with warnings as errors.
3. Flash only after both builds pass, except for an explicitly isolated
   architecture-specific investigation.
4. Capture serial output for a finite time and return nonzero on a fault,
   failed test, reset, timeout, wrong image, or incomplete output.
5. Diagnose one failure, make one attributable change, and repeat.
6. Archive the complete log and firmware identity before the next run.

Prefer stable project wrappers over direct compiler or flashing commands. Make
the wrappers print all effective configuration: architecture, source identity,
clock, voltage, flash divider, board/package, and optional feature flags.

## Make hardware output machine-verifiable

Emit structured records such as `BOOT`, `TEST:PASS`, `TEST:FAIL`, `BENCHMARK`,
`PROGRESS`, and `FAULT`. A strict host monitor should verify:

- exactly one BOOT record and a new run ID;
- expected source, architecture, board, clock, voltage, and mode;
- every required correctness suite before accepting a benchmark;
- ordered progress records and internally consistent counters/rates;
- enough complete measurement windows for a performance claim;
- immediate failure on malformed output, reset, duplicate BOOT, or `FAULT`.

Embed a source identity in every image. Do not trust a filename, build
directory, USB port, or successful flashing exit status as proof that the
expected firmware is running.

## Correctness before speed

- Keep small standard known-answer tests and a larger independent oracle that
  exercises optimized paths, boundary cases, and rare branches.
- Generate expected data independently of the implementation under test.
- Test error latches, timeouts, partial transfers, counter wrap, multicore
  protocols, cancellation, and exact end-of-work accounting.
- Run the full oracle at every overclock point. A successful boot or plausible
  benchmark does not exclude silent computation errors.
- Preserve a complete generic fallback behind common-case shortcuts.

## Measure well

- Define the unit of work precisely. Distinguish kernel, component, and
  end-to-end throughput.
- Use synchronized windows when comparing concurrent workers. Report medians,
  ranges, and median absolute deviation rather than one final sample.
- Exclude startup and warm-up consistently. Keep reporting cadence identical
  across candidates.
- Compare only matching source identities and hardware configurations.
- Repeat a new winner and every stability boundary with byte-identical
  artifacts.
- Treat small changes near measurement noise as neutral unless repeatable.

## Optimize from evidence

- Profile first. Optimize the component with the largest share of end-to-end
  time, not the most interesting function.
- Inspect final linked disassembly, symbol size, stack frame, spills, calls,
  branches, and memory traffic. Source-level intent is not sufficient.
- Reject byte-identical compiler variants without consuming device time.
- Keep architecture-specific implementations when measured behavior differs.
  M33 and Hazard3 have different register allocation, branch, code-size, DMA,
  and fetch tradeoffs.
- Change one mechanism per commit. Restore rejected variants exactly before
  starting the next experiment.

Useful optimization patterns include:

- retain peripheral ownership across batches instead of repeating setup;
- precompute invariant state and constant protocol fields;
- use aligned native-width transfers and avoid format conversions in hot loops;
- keep producer-to-consumer values in registers when possible;
- move rare validation, logging, and error publication out of common paths;
- batch accounting and telemetry without weakening error detection;
- overlap independent work only after profiling proves a safe wait interval;
- partition work statically when it avoids hot shared synchronization;
- use DMA only when it removes CPU/MMIO work after setup and wait costs.

## Memory and multicore guidance

- XIP cache can serve a compact hot loop well. Moving code to SRAM is not
  automatically faster.
- RP2350 SRAM is banked and shared by cores, DMA, stacks, and data. A relocation
  may trade flash stalls for SRAM-bus contention.
- Move one code/data resource at a time and document its physical bank, size,
  stack headroom, and competing users.
- Code footprint and layout can matter even when instruction count does not.
  Test narrow placement changes; avoid broad alignment flags without evidence.
- Prefer message passing or bounded checkpoints over per-iteration shared
  allocation and polling.
- Keep lossless events separate from coalescible telemetry. Shares, faults, and
  completion events must never be overwritten by progress updates.

## Clocking and overclock experiments

- Keep code optimization, system clock, voltage, and flash/QMI divider as
  separate experiment axes.
- Precompute realizable PLL points; requested round numbers may not be exact.
- Configure flash timing/divider from SRAM before raising `clk_sys`. Keep QMI
  SCK within a conservative validated range, not merely a theoretical limit.
- Keep USB and peripheral clocks at their normal frequency so serial failure is
  not confused with core instability.
- For a maximum-speed search, start at the authorized upper voltage, bracket
  the frequency ceiling with coarse then fine exact-PLL steps, and only then
  descend voltage at the winning clock.
- If a point fails, retry it once after verified stock recovery. Change the
  flash divider early when failures correlate with XIP/QMI speed.
- Distinguish boot failure, link/reset failure, correctness failure, incomplete
  capture, and host/flash failure. None is a throughput result.
- Return to a known stock image after risky runs. Archive and hash recovery
  images before starting the campaign.
- Overvoltage and overclocking can damage hardware. Cooling does not eliminate
  electrical risk; do not claim thermal safety without valid measurement.

## USB and recovery lessons

- Pico runtime and BOOTSEL modes have different USB identities. VM passthrough
  should normally match Raspberry Pi vendor ID `2e8a`, not one product ID.
- A serial device can disappear and re-enumerate during flash. Use bounded
  discovery and verify it belongs to Raspberry Pi before opening it.
- Require visible load and verification progress. Some host/device failure
  states can make a flashing tool return success without replacing or rebooting
  the running image.
- After a hang, use physical BOOTSEL if necessary, visibly flash the archived
  stock image, and require a full stock validation before resuming.
- Never modify OTP, boot security, or flash partitioning as part of an
  autonomous optimization loop.

## Experiment record

For every attempt, including failures and rejected variants, record:

- objective and single changed variable;
- commit/source identity and artifact hashes;
- board/package/chip revision and architecture;
- compiler/SDK configuration and relevant symbol/code-size evidence;
- clock, PLL tuple, voltage request/readback, flash divider, and derived clocks;
- correctness gates, capture duration, window count, and statistical result;
- complete log path and checksum;
- decision: retain, reject, retry, recover, or inconclusive;
- exact next action.

Commit between functional candidates. Keep historical plans and negative
results: they prevent future agents from repeating expensive dead ends.

## Final checklist

- [ ] Correct board/package definition and runtime identity
- [ ] Both ISAs build warning-free
- [ ] Expected image visibly flashed and verified
- [ ] Full correctness suite and independent oracle pass
- [ ] Serial capture is complete, ordered, and identity-matched
- [ ] Performance uses comparable synchronized windows
- [ ] Result and failure class are logged with artifact hashes
- [ ] Rejected code is restored exactly
- [ ] Device returns to a validated stock configuration after risky testing

Project-specific evidence and examples are in [summary.md](summary.md) and
[perf_progress.md](perf_progress.md).
