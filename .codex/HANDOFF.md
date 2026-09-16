# Pico 2 miner optimization handoff

## Latest checkpoint — authoritative

- The first 408 MHz / 1.60 V Hazard3 bisection image built both architectures
  warning-free and flashed/verified, but runtime USB never reappeared during
  the 50-second strict capture. No Pico USB device or serial node remains.
  Classification is **BOOT_FAIL**. Empty log
  `logs/OC-riscv-v1600-f408000-boot-fail-1.log` has SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  This is not yet a confirmed boundary: physically recover in BOOTSEL,
  validate stock, then retry the identical 408 MHz / 1.60 V point once.

- Identical 480 MHz / 1.60 V Hazard3 retry reproduced BOOT_FAIL after both
  architecture builds and verified flash. No USB or `/dev/ttyACM*` appeared
  during the 50-second capture; empty log
  `logs/OC-riscv-v1600-f480000-boot-fail-2.log` (SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`).
  Two failures establish 480 MHz as a failing bound at 1.60 V. Physically
  recover and validate stock before selecting the next lower exact midpoint.
- Physical BOOTSEL recovery after that failure passed stock again. Recovery
  log `logs/OC-riscv-v1100-f150000-recovery-15.log` has SHA-256
  `37f4c0420aebee09eba1dde7923590c7ac3e120cacb7effe048f2259b2b0b126` and
  first-seven medians 369,847 aggregate / 338,330 hardware / 31,517 software
  H/s. Select the next lower exact PLL midpoint in the 396–480 MHz interval
  (approximately 444 MHz) at 1.60 V.
- The first 444 MHz / 1.60 V bisection image built and verified but produced
  no runtime USB or serial output, classified **BOOT_FAIL**. Empty log
  `logs/OC-riscv-v1600-f444000-boot-fail-1.log` has SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  Physically recover and validate stock, then retry 444 MHz identically.
- Physical BOOTSEL recovery passed stock again. Recovery 16 log
  `logs/OC-riscv-v1100-f150000-recovery-16.log` has SHA-256
  `7394981a86d095ef669fc2b3c00f3af6d8f5ff585e48da5f560538c2858dd9e9` and
  first-seven medians 369,846 aggregate / 338,329 hardware / 31,517 software
  H/s. Retry 444 MHz / 1.60 V identically now.
- The identical 444 MHz / 1.60 V retry again produced no USB or serial output
  after verified flash, reproducing **BOOT_FAIL**. Empty log
  `logs/OC-riscv-v1600-f444000-boot-fail-2.log` has SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  Two failures establish 444 MHz as a failing bound at 1.60 V. Recover and
  validate stock before the next lower exact midpoint.
- Physical recovery passed stock again. Recovery 17 log
  `logs/OC-riscv-v1100-f150000-recovery-17.log` has SHA-256
  `3d97c76eef0b4491abdd5ce29e10501d244a394f1f6cf54ff777f03a3d65136f` and
  first-seven medians 369,845 aggregate / 338,328 hardware / 31,517 software
  H/s. Next lower midpoint is approximately 420 MHz / 1.60 V.
- The first 420 MHz / 1.60 V image built and verified but produced no runtime
  USB or serial output, classified **BOOT_FAIL**. Empty log
  `logs/OC-riscv-v1600-f420000-boot-fail-1.log` has SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  Physically recover and validate stock, then retry 420 MHz identically.
- Stock recovery 18 passed after the 420 MHz failure. Log
  `logs/OC-riscv-v1100-f150000-recovery-18.log` has SHA-256
  `ff2b250b6d0a2b262ab88f2ed03055f3759d79d135bdb57b52c047c2d22d795d` and
  first-seven medians 369,844 aggregate / 338,327 hardware / 31,517 software
  H/s. Retry 420 MHz / 1.60 V identically.
- The identical 420 MHz / 1.60 V retry again produced no USB or serial output
  after verified flash, reproducing **BOOT_FAIL**. Empty log
  `logs/OC-riscv-v1600-f420000-boot-fail-2.log` has SHA-256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
  Two failures establish 420 MHz as a failing bound at 1.60 V. Recover and
  validate stock before the next lower exact midpoint.
- Stock recovery 19 passed after the reproduced 420 MHz failure. Log
  `logs/OC-riscv-v1100-f150000-recovery-19.log` has SHA-256
  `9fb27115d09df596da2ac58250645ef1194571f0c81c4e10ba33f9f895c456ce` and
  first-seven medians 369,847 aggregate / 338,330 hardware / 31,517 software
  H/s. Next midpoint is approximately 408 MHz / 1.60 V.

- Overclock execution has started. Phase-0 source/wrapper/monitor infrastructure
  is implemented and stock-builds on both ISAs; 13 host monitor tests pass.
  It adds allow-listed requested VREG, strict readback/clock identity, PLL/USB/
  peri/QMI telemetry, and an exact PLL catalog. No device flash has occurred
  yet. Commit the infrastructure, clean-build both recovery images, archive
  them, then hardware-validate stock Hazard3 before any overclock point.
- Phase 0 is committed as `640dbca`, identity `ac3e4e469db5`. Clean stock
  recovery images are archived under `artifacts/recovery/`. Hazard3 stock run
  `OC-riscv-v1100-f150000-measure-1` passed at 369,845 aggregate / 338,328
  hardware / 31,517 software H/s, matching retained baseline. Next commit this
  evidence and run the fresh 300 MHz/1.10 V Hazard3 control.
- Hazard3 300 MHz/1.10 V run `OC-riscv-v1100-f300000-measure-1` passed at
  739,679 aggregate / 676,659 hardware / 63,022 software H/s, 99.999% of the
  fresh stock-control linear prediction. Commit its complete log/evidence, then
  continue the default-voltage frontier at the exact 348 MHz PLL point.
- Hazard3 348 MHz/1.10 V attempt 1 built/flashed/verified but runtime USB never
  reappeared. Host doctor and `lsusb -d 2e8a:` find no device, so bounded
  automatic recovery is exhausted. Failure is logged as BOOT_FAIL, not yet a
  voltage boundary. After physical BOOTSEL reconnect, flash the archived stock
  Hazard3 recovery image and validate it, then retry identical 348/1.10 once.
- Commit `a9e73dd` records the reproduced 348 MHz/1.10 V BOOT_FAIL boundary.
  A second physical recovery passed stock again, then 348 MHz/1.20 V passed
  every correctness gate and 23 windows. First-seven medians are 857,996
  aggregate / 784,896 hardware / 73,098 software H/s, 99.995% aggregate
  scaling efficiency. Archive/commit this run and recovery evidence, then test
  the skipped 1.15 V selector at the same 348 MHz clock. If 1.15 V fails,
  repeat it identically before accepting 1.20 V as the minimum selector.
- Commit `0672192` records the stock recovery and clean 348 MHz/1.20 V pass.
  The first 348 MHz/1.15 V attempt booted with correct identity but failed the
  optimized oracle at vector 1412/nonce 3526006413. USB remains reachable.
  Archive/commit the CORRECTNESS_FAIL, then retry the identical 1.15 V point
  once. A matching failure establishes 1.20 V as the minimum selector here.
- Commit `2891d69` records 1.15 V attempt 1. Attempt 2 reproduced a strict
  optimized-oracle failure at vector 123/nonce 1129751015 with byte-identical
  firmware. Thus 348 MHz is bracketed by 1.15 V fail and 1.20 V pass. Commit
  attempt 2, repeat the 1.20 V Tier-B pass once as required for a voltage
  transition, then continue upward from the 1.20 V selector.
- Commit `cf9e10a` records the reproduced 1.15 V failure. The required 1.20 V
  transition repeat passed all gates and 23 windows at 857,995 aggregate /
  784,895 hardware / 73,099 software H/s, matching attempt 1. Archive/commit
  this repeat. The 348 MHz frontier is closed at minimum requested 1.20 V;
  select the next exact roughly +48 MHz PLL point and start at 1.20 V.
- Commit `93ed95b` closes the 348 MHz frontier. Exact 396 MHz/1.20 V attempt 1
  then built/flashed/verified, but runtime USB never appeared and the bounded
  check finds no Pico device. Record/commit this BOOT_FAIL. Physical BOOTSEL
  recovery is required; validate stock, then retry identical 396/1.20 once.
  Do not raise voltage based on only this first failure.
- Physical recovery after 396/1.20 attempt 1 succeeded. Stock Hazard3 run
  `30004927-00000001` passed all gates and nine windows at 369,843 aggregate /
  338,328 hardware / 31,517 software H/s. Its log is archived as
  `logs/OC-riscv-v1100-f150000-recovery-4.log`. Retry identical 396/1.20 now.
- The identical 396 MHz/1.20 V retry again built/flashed/verified but produced
  no runtime USB device or BOOT record. This reproduces BOOT_FAIL and makes
  1.20 V a failing lower bound at 396 MHz. Commit recovery 4 and attempt 2.
  Physical BOOTSEL recovery is again required; after stock validation test
  396 MHz/1.30 V (two selector indices higher), then 1.25 V if 1.30 V passes.
- Commit `ded6724` records that reproduced boundary. After physical recovery,
  a 25-second stock capture passed firmware checks but failed the host contract
  because only three of five required windows fit; it is retained as an
  incomplete RESET_OR_LINK_FAIL. The immediate 50-second recovery repeat
  passed nine windows at 369,846 aggregate / 338,329 hardware / 31,517
  software H/s. Test 396 MHz/1.30 V next.
- The stock recovery evidence includes an intentionally short 25-second
  incomplete capture and a clean 50-second pass. Then 396 MHz/1.30 V attempt 1
  built/flashed/verified but again produced no runtime USB device or BOOT.
  Record/commit the BOOT_FAIL. Physical recovery and an identical 396/1.30
  retry are required; do not change voltage or clock before that retry.
- Physical recovery after 396 MHz/1.30 V attempt 1 is complete. Stock Hazard3
  run `30004927-00000001` passed all correctness gates and nine windows at
  first-seven medians 369,846 aggregate / 338,329 hardware / 31,517 software
  H/s. It is archived as `logs/OC-riscv-v1100-f150000-recovery-7.log`.
  Retry the identical 396 MHz/1.30 V point now; do not change voltage or clock.
- The identical 396 MHz/1.30 V retry again built and flashed successfully but
  produced no runtime USB device or BOOT record. The bounded check found no
  Pico device. This reproduces BOOT_FAIL and establishes 1.30 V as a failing
  lower bound. Commit this evidence, then request physical BOOTSEL recovery,
  validate stock, and test 396 MHz/1.40 V (the next two-selector jump).
- Physical recovery after the confirmed 1.30 V boundary is complete. Stock
  Hazard3 run `30004927-00000001` passed all gates and nine windows with
  first-seven medians 369,846 aggregate / 338,328 hardware / 31,517 software
  H/s. Recovery log is `logs/OC-riscv-v1100-f150000-recovery-8.log`. Commit
  it, then test 396 MHz/1.40 V with the explicit unsafe-voltage telemetry.
- 396 MHz/1.40 V attempt 1 built/flashed/verified and enumerated `/dev/ttyACM0`,
  but emitted no BOOT or test bytes during the full 50-second capture. The port
  remains present. Classification is RESET_OR_LINK_FAIL, not a pass or voltage
  boundary. Commit the evidence, then repeat the identical 1.40 V point once.
- The byte-identical 396 MHz/1.40 V retry reproduced USB enumeration with zero
  BOOT/test bytes for 50 seconds; the Pico and tty remain present. Commit this
  RESET_OR_LINK_FAIL evidence. Per failure rule 10.3, return to the last passing
  frontier point (348 MHz/1.20 V) before attempting another voltage at 396 MHz.
- The attempted automatic return to 348 MHz/1.20 V was not a valid device run:
  picotool reported success without its normal load/verify transcript and with
  an empty tracked serial, then the same tty emitted zero bytes. Host doctor
  passes. Preserve this as RESET_OR_LINK_FAIL; it does not overturn the prior
  348 MHz passes. Physical BOOTSEL recovery is now required.
- Physical BOOTSEL recovery restored normal stock load/verify and a clean full
  pass. Hazard3 run `30004927-00000001` passed all gates and nine windows at
  first-seven medians 369,847 aggregate / 338,330 hardware / 31,517 software
  H/s. Archive/commit recovery 9, then revalidate 348 MHz/1.20 V.
- Clean post-recovery 348 MHz/1.20 V revalidation passed all gates and 23
  windows at first-seven medians 857,997 aggregate / 784,899 hardware / 73,097
  software H/s. This confirms the device/host path and isolates the silent
  failures to 396 MHz. Commit the log, then the no-bracket two-selector rule
  selects 396 MHz/1.60 V; label unsafe-overvoltage and recover stock afterward.
- 396 MHz/1.60 V attempt 1 passed all gates and 23 windows at first-seven
  medians 976,340 aggregate / 893,137 hardware / 83,201 software H/s, with
  correct unsafe flag/readback. The immediate automatic stock return was
  indeterminate (empty tracked serial, no load/verify, silent tty). Commit both
  artifacts. Physical BOOTSEL recovery and stock validation are required, then
  repeat 396/1.60 once because it defines the voltage transition.
- Physical post-1.60 V recovery is complete. Stock Hazard3 passed all gates and
  nine windows at first-seven medians 369,845 aggregate / 338,328 hardware /
  31,517 software H/s. Archive/commit recovery 10, then run the required
  396 MHz/1.60 V Tier-B repeat; return to stock again immediately afterward.
- The 396 MHz/1.60 V repeat passed all gates and 26 windows at first-seven
  medians 976,344 aggregate / 893,143 hardware / 83,201 software H/s. The
  immediate stock return then programmed normally and passed at 369,844 /
  338,327 / 31,517 H/s. Archive/commit both. The 396 MHz transition is closed
  at 1.40 V fail / 1.60 V pass; next run the early 570 MHz/1.60 V anchor.
- 570 MHz/1.60 V anchor attempt 1 built/flashed/verified, but runtime USB never
  appeared and the bounded check found no Pico or tty. Record this BOOT_FAIL;
  it is only the first failure. Physical BOOTSEL recovery and stock validation
  are required, then retry the byte-identical 570/1.60 image once.
- Physical recovery after the first 570 MHz anchor failure is complete. Stock
  Hazard3 passed all gates and nine windows at first-seven medians 369,847
  aggregate / 338,330 hardware / 31,517 software H/s. Recovery 12 is archived;
  retry the identical 570 MHz/1.60 V anchor now.
- The identical 570 MHz/1.60 V retry again produced no runtime USB device after
  verified flash. This reproduces BOOT_FAIL and establishes 570 MHz as the
  failing upper frequency bound at the 1.60 V cap. Commit the evidence, recover
  and validate stock, then bisect frequency between 396 MHz pass and 570 MHz
  fail at the nearest exact midpoint (target 480 MHz/1.60 V).
- Physical recovery after the reproduced 570 MHz failure passed stock again at
  first-seven medians 369,847 aggregate / 338,330 hardware / 31,517 software
  H/s. Recovery 13 is archived; test the exact midpoint candidate around 480 MHz
  at 1.60 V next.
- Physical recovery after the first 480 MHz bisection failure passed stock at
  first-seven medians 369,845 aggregate / 338,328 hardware / 31,517 software
  H/s. Recovery 14 is archived; retry the identical 480 MHz/1.60 V point now.
- 480 MHz/1.60 V bisection attempt 1 built/flashed/verified but produced no
  runtime USB device. Record BOOT_FAIL, recover and validate stock, then retry
  the identical 480 MHz/1.60 V point before narrowing toward 396 MHz.
- Planning checkpoint: `overclock_test_plan.md` defines the next campaign. It
  is planning-only; no voltage, clock, firmware, or device state was changed.
  The plan adaptively traces the minimum-voltage stability frontier for both
  architectures instead of sweeping a full grid, with hard campaign caps of
  570 MHz and requested 1.60 V. On resume, implement its Phase 0 measurement,
  PLL-catalog, identity, and recovery infrastructure as one isolated commit
  before any overclock flash.
- Pause checkpoint after completing candidate 114 (D1 two-phase odd-tail
  takeover). Candidate source commit `b54dd2d`, identity `0a0357882807`, is
  **retained**. Evidence is being committed with this handoff.
- Both architectures build warning-free and all ten host monitor tests pass.
  Paired 150 MHz, temperature-disabled hardware runs passed all 8 device tests,
  the 4,096-case oracle, expanded 13-case decision paths, nine windows, and
  strict capture without faults.
- First-seven medians: ARM **360,308 aggregate / 328,871 hardware / 31,438
  software H/s**; Hazard3 **369,844 / 338,331 / 31,510 H/s**. Changes versus
  retained parents are only -0.009%/-0.012%/+0.025% on ARM and
  -0.014%/-0.014%/-0.016% on Hazard3. The isolated filter exactly matches each
  parent. Logs are `logs/D1-tail-takeover-{arm,riscv}.log`; hashes and full
  comparisons are at the end of `perf_progress.md`.
- The roughly 106-minute full nonce-space transition was not run. D1 is
  arithmetic/protocol-qualified by bounded firmware and host cases plus paired
  ordinary-phase hardware evidence, not by an endurance completion run.
- The board now runs retained candidate 114 Hazard3 at stock 150 MHz with
  temperature disabled.
- On resume, acquire `.codex/ACTIVE_SESSION`, re-read the end of
  `perf_progress.md`, and continue with the next uncompleted item in
  `planned_optimizations_update2.md`. Do not follow older `next actions` text
  lower in this historical handoff if it conflicts with this checkpoint.
- Firmware still prints package `RP2350A` with `sysinfo_package_sel=1` on the
  user's RP2350B. This pre-existing label issue was not mixed into D1.

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
- Candidate 108 is rejected: 360,301 aggregate / 328,864 hardware H/s,
  -0.011%/-0.015% versus candidate 105. With candidate 107 also neutral/slightly
  negative, A3 is closed without a 64-byte run. Board runs rejected 108 ARM.
- Restoration commit `70c31a0` returns exactly to retained candidate 105:
  identity `8b850732e27f`, all 8 host tests pass, and clean ARM/RISC-V UF2
  hashes are `0c12f2...a351f` / `e8b324...0d942`. Source and build output are
  ready for B1; the board still carries rejected candidate 108 ARM.
- Candidate 109 B1 preflight targets only Hazard3's exact second-hash filter.
  Rotated-role four-round groups remove all state-rotation `mv` instructions
  from the repeated eight-round body (211 instructions total), keep its frame
  at 288 bytes, and add 768 total text bytes. Both dirty builds and all 8 host
  tests pass; commit and clean-build before any Hazard3 flash.
- Candidate 109 commit `71e9c28`, identity `525dd4932d37`, is now retained.
  Hazard3 attempt 109a passed all gates at 369,894 aggregate / 338,379 hardware
  / 31,515 software H/s: +0.412% aggregate and +4.774% software over candidate
  105. The exact filter improved 4.796% to 31,685 H/s. Complete log:
  `logs/B1-rotated-filter-riscv.log`. The board runs retained candidate 109
  Hazard3; ARM remains candidate 105 code apart from firmware identity.
- Candidate 110 B1 ARM rotated-role preflight is rejected without flashing.
  Although both builds and all host tests passed, the ARM exact-filter helper
  grew from 1,712 to 2,256 bytes (+31.8%), total text grew 552 bytes, and its
  56-byte frame did not improve. Hazard3 stayed unchanged. ARM has been
  restored to the compact candidate-109 architecture split. Both clean builds,
  all eight host tests, helper sizes, text/BSS, and UF2 hashes exactly match
  retained candidate 109. Commit the rejection/restoration evidence, then
  proceed to B2.
- Candidate 111 begins B2 as one ARM-only four-word experiment: compute/store
  W16-W19 after round 15, consume them immediately in rounds 16-19, then expand
  W20-W60 and resume at round 20. Hazard3 must remain unchanged. Its hypothesis,
  resource risk, and no-flash static rejection gates are in the ledger.
- Candidate 111 failed those gates and was not flashed: ARM's helper grew from
  1,712 to 2,180 bytes, total text grew 480 bytes, and its frame grew from 56
  to 72 bytes. Hazard3's helper/frame stayed unchanged. The experiment source
  has been removed. Both rebuilt artifacts, helper sizes, source identity, and
  all eight host tests exactly match candidate 109. Commit the rejection
  evidence and close this B2 shape rather than expanding it.
- Candidate 112 starts B3 with a local ARM-only `O2` attribute on the exact
  filter, retaining loop unrolling and leaving Hazard3 on its existing code.
  The ledger defines static no-op/regression gates before any possible flash.
- Candidate 112 static preflight passes: ARM's exact filter shrinks 1,712 to
  1,620 bytes and 543 to 517 instructions, total text falls 80 bytes, and the
  56-byte frame is unchanged. Hazard3 remains unchanged. Commit the candidate,
  clean-build both, then run strict ARM hardware validation at stock 150 MHz.
- Candidate 112 hardware passes correctness but is rejected: first-seven ARM
  medians are 360,164 aggregate / 328,886 hardware / 31,278 software H/s,
  -0.049%/-0.008%/-0.484% versus retained candidate 105/109. Isolated filter
  falls 0.503% to 31,419 H/s. Log `logs/B3-local-o2-arm.log`. Source is restored
  to the retained O3 helper. Restoration commit `9439905` rebuilds both exact
  candidate-109 UF2 hashes with all eight host tests passing. Commit this final
  restoration evidence; the board remains on rejected candidate 112 ARM.
- Candidate 113 completes B3's ARM local-level comparison with `Os` plus the
  existing unroll request on only the exact filter. It is build-only unless it
  preserves the hot structure/frame while improving linked code; a smaller
  branchier helper is explicitly insufficient after candidate 112.
- Candidate 113 is rejected without flashing: the helper shrank to 680 bytes
  but introduced a `memset` call and compact common-path loops/branches. Frame
  stayed 56 bytes and Hazard3 remained unchanged. O3 is restored and remains
  the winner of the ARM O2/O3/Os comparison. Both restored artifacts, sizes,
  identity, and all eight host tests exactly match candidate 109. Commit this
  evidence, then perform C1's required static equivalence check.
- C1 is closed by static equivalence. Retained Hazard3 emits one
  `al3_read_addr_trig` store at `0x10001544`; a 64-byte ring still needs one
  trigger/count-reload store plus the same address calculation and completion
  wait. No source or hardware candidate was created. Next is D1 two-phase tail
  takeover; leave C2/C3 gated on refreshed profiling evidence.
- Candidate 114 begins D1. Keep the normal even/odd loops unchanged; at even
  exhaustion core 1 requests core 0's next-unprocessed odd frontier once, core
  0 stops hashing, and core 1 consumes the remaining odd suffix. Require exact
  `2^32` final accounting, parity/frontier checks, bounded startup tests, and no
  new normal hot-path coordination check. Detailed gates are in the ledger.
- Candidate 114 static preflight passes after moving tail hasher initialization
  fully into the noinline takeover helper: retained normal frames are restored,
  worker growth is only 8/28 bytes at the terminal branch, both builds pass,
  and ten host tests cover takeover/completion parsing. Commit and clean-build,
  then perform paired short stock-clock hardware validation.

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

Candidate 108 rejection evidence is committed in `eec17b6`; A3 is closed.
Restoration commit `70c31a0` has been clean-built and verified byte-exact to
retained candidate 105 on both architectures, with all host tests passing.
Candidate 109 B1 is hardware-validated and retained on Hazard3. Record/commit
attempt 109a and its archived log. Next follow `planned_optimizations_update2`:
test a separately bounded ARM rotated-role four-round exact-filter shape only
if static disassembly reduces moves/instructions without frame/spill growth;
otherwise reject it build-only and proceed to B2's single four-word
schedule/round interleave. Keep Hazard3's retained B1 path unchanged.
Candidate 110's ARM rotated-round preflight failed its static gate and was not
flashed. Source and both rebuilt artifacts are exact retained candidate-109
matches. Commit the build-only rejection/restoration evidence, then begin one bounded B2 four-word
schedule/round interleave experiment only if disassembly identifies a concrete
reload reduction. Keep Hazard3's retained B1 path unchanged.
Candidate 111 tested that smallest B2 shape and failed statically through ARM
frame/code growth; exact restoration is complete. Commit its evidence, then
advance to B3 local code-generation variants one helper and architecture at a
time. Identical or statically worse linked code is a build-only rejection.

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
