# Pico 2 miner optimization handoff

## Objective

Continue from retained candidate 103 using the ranked remaining-options analysis
in `planned_optimizations_update2.md`. For future code experiments, build both
ISAs before flashing shared source, capture complete hardware output, archive
logs, append every result/failure to `perf_progress.md`, and commit between
functional attempts. Keep stock 150 MHz code experiments separate from clock
changes and keep temperature disabled.

## Current status

- Retained candidate 103 source commit is `b2b8dfb`, source identity
  `104da455bdbb`: ARM uses hardware-worker batch factor 4 and Hazard3 factor 1.
- The Pico is accessible when hardware commands run outside the filesystem
  sandbox. It currently runs rejected candidate 104 ARM at stock 150 MHz;
  working source/build artifacts are restored to accepted candidate 103.
- E06-trigger is retained on Hazard3. Attempt 86b passed all gates with a
  seven-window median of 368,130 H/s aggregate and 338,274 H/s hardware,
  +0.42%/+0.45% over immediate control 85e. ARM keeps its CPU feeder.
- E09-b-pending is resolved and retained on both ISAs.
- E01-identity is implemented, paired-hardware validated, and retained.
- E01-rare candidate 83 and E01-window candidate 84 are retained on both ISAs.
  E02 and the updated sequence through candidate 103 are complete.
- Candidate 100 software batch-8 passed correctness but was rejected: ARM was
  neutral, while Hazard3 aggregate fell 0.417% because its 0.216% software
  gain accompanied a 0.474% hardware loss. Restoration commit `a38b99e`
  returns to source identity `1eb3d9edb2b0`; both rebuilt UF2 hashes exactly
  match retained candidate 98 and all eight host tests pass.
- Candidate 102 passed final paired validation. ARM's first-seven median is
  357,657 H/s aggregate / 326,224 hardware, +0.312%/+0.348% over candidate 98;
  Hazard3 returned to factor-1 at 368,377 / 338,299 H/s, effectively identical
  to candidate 98. Logs are `logs/E04c-hardware-batch-split-{arm,riscv}.log`.
- Candidate 103 ARM factor 4 passed at 359,558 H/s aggregate / 328,122 hardware,
  +0.532%/+0.582% over factor 2 and +0.845%/+0.932% over factor 1. Hazard3
  remains compile-time factor 1 at its retained size. The factor 1/2/4 study
  is complete; log `logs/E04c-hardware-batch4-arm.log`.
- `planned_optimizations_update2.md` now analyzes the viable work remaining
  after candidate 104. It is a planning-only update; no device test or source
  experiment was performed while preparing it. Its first recommendation is an
  ARM factor-4 hot/cold worker split, followed conditionally by ARM factor 8.
- Candidate 105 commit `421121c`, source identity `8b850732e27f`, is retained.
  ARM reached 360,342 aggregate / 328,912 hardware H/s, +0.218%/+0.241% over
  candidate 103, while removing 616 total text bytes. Hazard3 remains factor 1.
  The board now runs accepted candidate 105 ARM.
- Candidate 106 A2 is rejected after passing correctness. ARM factor 8 measured
  359,709 aggregate / 328,281 hardware H/s, -0.176%/-0.192% versus retained
  candidate 105 factor 4. The board currently runs rejected candidate 106 ARM.
- Restoration commit `012f03d` returns source/builds exactly to candidate 105:
  source identity `8b850732e27f`, all 8 host tests pass, and ARM/RISC-V UF2
  hashes are exactly `0c12f2...a351f` / `e8b324...0d942`.
- Candidate 107 A3 requests 16-byte alignment for the ARM worker only. Dirty
  preflight passes: entry moves `0x100003c4` to `0x100003d0`, body/frame remain
  1,452/156 bytes, ARM text adds 16 bytes, and Hazard3 remains exact.
- Candidate 107 is rejected after passing correctness: 360,299 aggregate /
  328,863 hardware H/s, -0.012%/-0.015% versus candidate 105. The board runs
  rejected candidate 107 ARM.
- Candidate 108 requests 32-byte ARM worker alignment. Dirty preflight passes:
  entry `0x100003e0`, unchanged 1,452-byte body/156-byte frame, +32 ARM text
  bytes, and exact retained Hazard3 size/path.

## Completed work in this session

- E09-b full-digest fixed rounds ARM: 30,012 H/s versus 29,205 parent
  (+2.76%); filter 30,718; final-30 aggregate 356,300. Retained shared change.
- E01 identity contract adds stable source identity, warm-reset run ID,
  progress sequence, strict ordered output validation, five-progress minimum,
  and six synthetic parser tests.
- E01 ARM: full 30,012, filter 30,718, final-30 325,769 hardware / 30,540
  software / 356,309 aggregate. Strict contract passed 114 reports.
- E01 RISC-V: full 28,631, filter 30,041, final-30 339,033 hardware / 29,804
  software / 368,837 aggregate. Strict contract passed 118 reports.
- E01-rare ARM/RISC-V passed 8 suites. ARM aggregate 356,326; RISC-V
  aggregate 368,925. Known loser fast-rejection and genesis fallback/candidate
  are directly and host-validated.
- E01-window ARM passed seven synchronized 4.921-second windows. Aggregate
  range 355,630–355,643 H/s, median 355,640; hardware median 325,104 and
  software median 30,535. Complete log `logs/E01-window-arm.log`, UF2 SHA-256
  `2a1857e60441d9b9ef6239e58628de19ddc33623a0df2cc097f981e756d8d6d9`.
- E01-window RISC-V passed seven synchronized 4.752-second windows. Aggregate
  range 366,550–366,560 H/s, median 366,556; hardware median 336,697 and
  software median 29,860. Complete log `logs/E01-window-riscv.log`, UF2
  SHA-256 `238f314f3012188082c35a2324c0b9f74bee42adeeffb010c8fc1b086e702ac4`.
- E02 profile mode compiles on both ISAs and adds intrusive stage, CPU-counter,
  and XIP-counter records only when `MINER_PROFILE=1`. Normal and profile dual
  builds pass. ARM attempt 85a passed functionally, but live DWT capability
  bits disproved the header reset annotation; its counter backend needs a
  DWT-only revision and repeat before the RISC-V profile.
- One ARM flash completion failure was recorded at `0a03543`; the identical
  retry passed. Corrected ARM DWT stage profiling is now accepted: hardware
  stage sum 518.38 cycles/hash, common-window median 355,468 H/s (-0.048%
  versus retained parent), complete log `logs/E02-profile-arm-dwt.log`.
- Hazard3 profile passed: hardware stage sum 482.63 cycles/hash and 230.09
  instructions/hash; its first-block DMA feed is 87.01 cycles/hash. Common
  median 365,328 H/s was -0.335% versus retained normal parent, confirming the
  profile image is intrusive-only. Log `logs/E02-profile-riscv.log`.
- Normal RISC-V restoration passed all gates: common-window median 366,601
  H/s (hardware 336,750, software 29,850), a +0.012% match to retained parent.
  Use this as the immediate E06-trigger control.

## Work in progress / next actions

Current continuation point: candidate 104 is rejected and restoration commit
`093a1c2` returns source to retained candidate 103. Commit `81edb25` records the
restoration evidence. Both builds, 8 host tests, and exact candidate-103 UF2
hashes pass. The board still carries rejected candidate 104 ARM, but no device
work was requested for the planning update. Do not repeat main-SRAM hardware-
worker placement, software batch factor 8, scratch-X filter placement, E08 queue
polling at 1 or 64 hashes, or rejected E14 chunk variants unchanged.

Candidate 108 A3-32 is implemented and passes dirty preflight. Commit it,
clean-rebuild both architectures, then run a strict ARM cycle against unaligned
candidate 105's 360,342 aggregate / 328,912 hardware H/s. Reject regression or
neutrality. The board currently runs rejected candidate 107 ARM.

Candidate 104 commit `6dc1941`, identity `638fe640df92`, passed correctness but
is rejected. ARM worker-in-main-SRAM measured 358,620 aggregate / 327,806
hardware / 30,815 software H/s: -0.261%/-0.096%/-1.972% versus candidate 103.
Inter-core SRAM contention outweighs XIP relief. Archive
`logs/E07-hardware-worker-sram-arm.log` contains the run, the evidence is
committed, and the worker placement is restored to XIP in source/build output.

The candidate-104 restoration is complete: source identity `104da455bdbb`,
both builds and 8 host tests pass, and ARM `fae222...e67b` / RISC-V
`fc1f13...6693` exactly match retained candidate 103 artifacts. Board still
carries rejected candidate 104 ARM.

Candidate 103 commit `b2b8dfb`, identity `104da455bdbb`, passed ARM hardware:
359,558 aggregate / 328,122 hardware / 31,435 software H/s, +0.532%/+0.582%
over candidate 102 factor 2. All correctness and nine windows passed. Retain
ARM factor 4; Hazard3 remains factor 1 at its retained size. Artifact/log
hashes are recorded in the ledger. The board was subsequently flashed with the
rejected candidate 104 ARM image and has not been reflashed after restoration.

Candidate 101 is the selected E04-c factor-2 hardware-worker experiment. It
nests exactly two complete nonce iterations inside the existing report loop;
340,000 is statically divisible by two. Per-nonce candidates, ARM error checks,
nonce progression and exhaustion remain exact. The hypothesis and rejection
rule are recorded at the end of `perf_progress.md`. Build/test both, inspect
whether the compiler truly duplicates the hot body and its code/register cost,
then commit before the first hardware run.
Preflight A built and passed host tests but the compiler did not unroll the
inner loop on either ISA; this is logged as a build-only rejection. An explicit
`#pragma GCC unroll 2` is now being tested as preflight B. Do not flash unless
disassembly confirms two physical hash bodies without an inner batch branch.
Preflight B succeeded and candidate commit is `5216b86`, source identity
`8ac61bed6329`. ARM attempt 101a passed all gates: first-seven median 357,655
H/s aggregate, 326,223 hardware, 31,433 software, a +0.311% aggregate and
+0.348% hardware gain over candidate 98. Archive
`logs/E04c-hardware-batch2-arm.log`. Commit this evidence, then run the
identical Hazard3 image and decide the shared candidate.
Hazard3 attempt 101b passed all gates but was neutral: first-seven median
368,408 aggregate, 338,344 hardware, 30,064 software, only +0.008% aggregate
versus candidate 98 despite +576 text bytes. Archive
`logs/E04c-hardware-batch2-riscv.log`. Retain factor 2 only for ARM, restore
Hazard3 to factor 1 in a final architecture split, rebuild both, and hardware
validate the split before considering factor 4.
Candidate 102 commit `e7266e6`, source identity `8c8ea1cbf514`, passed final
paired hardware validation. ARM run `...1a` reproduced factor 2 at 357,657
aggregate / 326,224 hardware H/s; Hazard3 run `...1b` restored factor 1 at
368,377 / 338,299 H/s. All 8 suites, 4,096 oracle cases, nine windows, and
strict captures passed on both. Final UF2 hashes are ARM `6fd874...47e94` and
RISC-V `a3d7e...3bd9`; archive hashes are recorded in the ledger.

0. Candidate 97 is now defined as E04-f telemetry cadence: change only the
   hardware report interval from 100,000 to 340,000 hashes (approximately one
   second on both retained ISAs), leaving the 16-report common-window structure
   intact. Both builds and 8 host tests pass; disassembly confirms only the
   report comparison constant/cold cadence changes. Commit before hardware,
   then capture at least five and preferably seven longer
   windows on ARM and RISC-V. Archive logs, calculate serial bytes/s, decide,
   and restore if rejected.
   ARM attempt 97a passed all gates: seven-window median 356,602 H/s (+0.020%
   versus E09-c), with steady payload reduced about 70.3% to 305 B/s. Archive
   `logs/E04f-cadence1s-arm.log`. Hazard3 attempt 97b also passed: median
   368,426 H/s (-0.017%) and payload 323.5 B/s (-70.3%). Exact candidate 97 is
   rejected because 16-second windows break the default 45-second strict
   cycle's five-window contract. Archive `logs/E04f-cadence1s-riscv.log`.
   Commit evidence, then test one follow-up with four one-second reports per
   window and matching host expectation; restore accepted E09-c if it fails.
   Candidate 98 now makes that grouping change and adds BOOT report/window
   configuration so the host validates the advertised cadence dynamically.
   Both stock builds pass without warnings, all eight host tests pass, and
   text/BSS is ARM 188,720/4,708 and Hazard3 200,924/4,440 bytes. Candidate
   commit is `e2912db`, source identity `1eb3d9edb2b0`. ARM attempt 98a passed
   all gates and eight windows: first-seven median 356,545 H/s aggregate,
   325,092 hardware, 31,450 software; median payload 343.5 B/s. Archive
   `logs/E04f-cadence1s-window4-arm.log`. Hazard3 attempt 98b also passed all
   gates: median 368,378 aggregate, 338,300 hardware, 30,078 software; payload
   363.4 B/s. Archive `logs/E04f-cadence1s-window4-riscv.log`. Candidate 98 is
   retained: both ISAs are throughput-neutral versus E09-c, mining payload is
   about 66.6% lower, and four-second windows restore default-cycle coverage.
   Commit the paired evidence, then select the next untested bounded E07-bank
   placement experiment from the updated plan.
   Candidate 99 is now defined: place only ARM's 1,712-byte exact-filter helper
   in scratch X, alongside but not overlapping the fixed 2,048-byte core-1
   stack (336 bytes remain); keep Hazard3 unchanged because its 2,646-byte
   helper cannot fit. Both builds and eight host tests pass. The ARM map
   confirms code `0x20080000..0x200806b0`, stack
   `0x20080800..0x20081000`, and the 336-byte gap; total sizes are unchanged.
   Commit before an ARM hardware run, archive it, then retain only for a
   repeatable aggregate improvement without hardware-worker regression.
   ARM attempt 99a passed correctness but is rejected: 356,146 aggregate,
   325,051 hardware, 31,095 software, a 1.129% software-worker loss versus
   candidate 98. Archive `logs/E07-filter-scratchx-arm.log`. Commit evidence,
   restore the default `.time_critical` placement, rebuild both and require
   exact candidate-98 UF2 hashes before the next experiment.
   The source placement has now been restored with a focused inverse patch;
   both builds and eight host tests pass. ARM/RISC-V UF2 hashes exactly match
   retained candidate 98 (`5fb2...54ef` / `f109...04c6a`). Commit this
   restoration evidence. The board still carries rejected candidate 99 ARM;
   build artifacts contain the retained source.
   Candidate 100 is now defined and implemented but unbuilt: add an 8-nonce
   software-filter batch API, poll FIFO once per batch, and retain exact full
   digest/target comparison for every mask hit. The mining-decision KAT covers
   a batch containing the genesis winner. Both builds and eight host tests
   pass. Assembly reuses one schedule frame per batch and removes seven outer
   calls/frame setups; text cost is ARM +120 and Hazard3 +172 bytes, BSS
   unchanged. Candidate commit `22ebd73`, source identity `aebc1223a0d2`.
   ARM attempt 100a passed all gates but is neutral: median 356,553 aggregate,
   325,099 hardware, 31,452 software (+0.002% aggregate versus candidate 98).
   Archive `logs/E03-batch8-arm.log`. Commit evidence, then run Hazard3 and
   retain an ISA split only for a clear Hazard3 gain. Hazard3 attempt 100b
   passed correctness but regressed to 366,841 aggregate: software +0.216%,
   hardware -0.474%, aggregate -0.417%. Archive `logs/E03-batch8-riscv.log`.
   Reject candidate 100 on both ISAs, commit evidence, restore the single-nonce
   API/loop, rebuild both, and require exact candidate-98 artifact hashes.
   The focused source restoration is now applied but uncommitted/unbuilt.

1. E09-b-tail-padding is resolved at commit `688148d`: retain explicit fixed
   rounds on ARM and the generic loop on Hazard3. Both final architecture
   images passed at source identity `f0b612529b69`.
2. E09-c-terminal is retained on both ISAs at candidate commit `a628426`,
   source identity `71b48132e899`: ARM filter +1.58%, Hazard3 filter +0.65%,
   with every hardware gate passing.
3. E09-b candidate 89 explicit tail W20–W30 passed correctness but is rejected
   on both ISAs: ARM affected software +0.05%, Hazard3 +0.02%. Restoration
   commit `d2b7085` rebuilds byte-identical accepted E09-c artifacts on both
   ISAs, so attempts 88a/88b remain valid.
4. E14-chunks candidate 90 is implemented with generation-tagged 64-bit finite
   allocation and 4,096-nonce chunks. Both hardware runs passed all correctness
   gates but the variant is rejected: ARM aggregate -25.99%, Hazard3 -1.85%.
5. Candidate 91 isolates the 64-bit hot-loop cost with 32-bit local
   nonce/remaining state. Both ISAs pass correctness but remain too slow:
   ARM aggregate -3.15%, Hazard3 -2.30%. Next change only chunk size from
   4,096 to the planned 65,536 to isolate acquisition frequency.
6. Candidate 92 makes that one-variable 65,536 size change. Build and test
   both ISAs. It is rejected: ARM is unchanged at -3.15%; Hazard3 worsens to
   -3.06%. Allocation frequency is not the cause. Next use a nested inner
   nonce loop with acquisition outside it, returning to 4,096 chunks.
7. Candidate 93 nested the core-1 hot loop and passed both ISAs, recovering
   most of candidate 91's loss, but is rejected: ARM remains -0.43% aggregate
   and Hazard3 -1.27% versus E09-c. Restoration commit `c3c6836` returns the
   three E14-touched source/test files to accepted E09-c. Both clean UF2 files
   are byte-identical to accepted attempts 88a/88b. Proceed with E08 lifecycle
   and transport work as a separately bounded experiment.
8. Candidate 94 is defined as an E08 outbound-telemetry SPSC queue experiment:
   fixed complete records, release/acquire publication, lossless share
   backpressure, separate fatal latch, queue-depth/blocked-time telemetry and
   a bounded wrap/full/order KAT. Keep ACK control on the hardware FIFO.
9. Candidate 94 passed correctness but is rejected as shared code. ARM fell
   19.89% aggregate despite queue depth one and zero producer blocking;
   Hazard3 was neutral at -0.06%. Test only one follow-up: poll the queue/fault
   latch every 64 software hashes, then restore if M33 does not recover or
   Hazard3 materially regresses.
10. Candidate 95 implements only that 64-hash steady-state polling cadence;
    it passed both ISAs but is rejected. ARM worsened to -23.65% aggregate;
    Hazard3 remained neutral at -0.06%. Queue depth was one and blocked time
    zero on both. Restore the entire candidate-94/95 source/test contract to
    accepted E09-c; do not repeat either polling cadence unchanged.
11. The three E08-touched source/monitor files are restored from accepted
    commit `487737e` in restoration commit `9962a36`. Both UF2 files are exact
    accepted E09-c matches. A full RISC-V restoration run passed at 368,474
    H/s median; the board is left on this accepted stock image.
12. Next candidate 96 is E04 exact 32-bit software accounting: the current odd
    parity worker stops after 2^31 hashes, so its hot counter needs no 64-bit
    increment. Preserve 64-bit window/rate/aggregate math and all cadence.
13. Candidate 96 passed every hardware gate on both ISAs but is rejected. ARM
    was neutral at 356,521 H/s and Hazard3 at 368,316 H/s. Disassembly shows
    that both compilers still maintain the carry chain in the mining loop, so
    the intended hot work was not removed despite 16/28-byte text reductions.
    Restore the accepted 64-bit counter and print formats, rebuild both ISAs,
    and require the exact E09-c UF2 hashes before the next experiment.
14. Restoration commit `ba39b4c` builds at accepted identity `71b48132e899`;
    both UF2 hashes exactly match E09-c, so no redundant hardware run is
    needed. The board still carries rejected candidate 96 RISC-V. Continue
    with the next bounded E04/E07 experiment, or flash an accepted image before
    stopping.

## Files and evidence

- Plan: `planned_optimizations_update.md`; historical detail:
  `planned_optimizations.md`.
- Ledger: `perf_progress.md` through E01-window RISC-V attempt 84b.
- E09 logs: `logs/E09b-fixed-full-rounds-{arm,riscv}.log`.
- E01 logs: `logs/E01-identity-{arm,riscv}.log` and
  `logs/E01-identity-arm-flash-failure.log`.
- E01-window logs: `logs/E01-window-{arm,riscv}.log`.
- E02 exploratory ARM log: `logs/E02-profile-arm-systick.log`.
- E02 corrected ARM log: `logs/E02-profile-arm-dwt.log`; ARM profile UF2
  SHA-256 `cbae3ae9973d1ecf272a25bee657d4fc3371c27322bd59f38dc7726dea181fbd`.
- E02 Hazard3 log: `logs/E02-profile-riscv.log`; RISC-V profile UF2 SHA-256
  `5c658609dcdf811dc617de65f9645a033c74ec4f453781e5637fe28c5e102df4`.
- Restored normal control: `logs/E02-normal-restored-riscv.log`; normal RISC-V
  UF2 SHA-256 `00db86d95bd8cacff5654730691f6dfc7f881368d8d69ae1d545c3f330689742`.
- E06-trigger failed host attempt:
  `logs/E06-trigger-riscv-libusb-failure.log`; candidate RISC-V UF2 SHA-256
  `0bdb6c31236103bda04fafbf49b87d84895ae6d84a754ee2f737b815c891e9a3`.
- E06-trigger passing hardware log: `logs/E06-trigger-riscv.log`, SHA-256
  `c3e0d4aeb242e39102c94e2a5a1e1c60490a641d289f355a7815c96e4f7baf05`.
- E09-b header-tail logs: `logs/E09b-header-tail-fixed-{arm,riscv}.log`.
  ARM improved and is retained; Hazard3 regressed and is restored to its
  parent loop in the final architecture split.
- Final split validation logs: `logs/E09b-header-tail-final-{arm,riscv}.log`.
- E09-c retained terminal-round logs: `logs/E09c-terminal-{arm,riscv}.log`.
- Rejected E09-b schedule logs: `logs/E09b-tail-schedule-{arm,riscv}.log`.
- E01 implementation: `CMakeLists.txt`, `src/main.c`, `tools/common.sh`,
  `tools/build`, `tools/cycle`, `tools/monitor.py`, `tools/test_monitor.py`.

## Tests and known problems

- `python3 -m unittest tools/test_monitor.py`: 8 passed.
- ARM and RISC-V builds warning-free with SDK 2.3.1 and repository wrappers.
- Current hardware runs: 8 suites, 4,096 oracle cases, no faults.
- Sandboxed libusb cannot initialize because `/dev/bus/usb` and libudev
  hotplug access are hidden. Run `lsusb`, `./tools/doctor`, and hardware cycle
  commands with the environment's hardware-access escalation. No `sudo` is
  needed; outside the sandbox the Pico node and `dialout` serial access pass.
- The retained common-window baseline is ARM 355,640 H/s and RISC-V 366,556
  H/s (medians of seven aligned windows each).
- Warm-reset run sequence is not persistent across power loss; source/run
  matching plus ordered BOOT handling still rejects stale capture fragments.
- The BOOT package label/assertion remains RP2350A/QFN-60 based on observed
  `SYSINFO.PACKAGE_SEL=1`; do not change identity handling without new evidence.

## Important decisions

- Preserve all plan documents and inherited edits.
- Source identity hashes committed `CMakeLists.txt`, `src`, and `tools` trees,
  so evidence-only commits do not alter firmware identity.
- A build/verified flash is not runtime success; nonzero cycle status is logged
  as failure and retried.
- No OTP/erase/security/partition/sudo operations. One serial/flash owner only.
