# Pico 2 miner optimization handoff

## Objective

Continue `planned_optimizations_update.md` one experiment at a time. Build both
ISAs before flashing shared source, capture complete hardware output, archive
logs, append every result/failure to `perf_progress.md`, and commit between
functional attempts. Keep stock 150 MHz code experiments separate from clock
changes and keep temperature disabled.

## Current status

- HEAD before E06-trigger source changes: `4409b84` (`perf: record E02 normal
  RISC-V control`); parent source identity `02b7825c7499`.
- The Pico is accessible when hardware commands run outside the filesystem
  sandbox. It currently runs the retained E06-trigger RISC-V image at stock
  150 MHz.
- E06-trigger is retained on Hazard3. Attempt 86b passed all gates with a
  seven-window median of 368,130 H/s aggregate and 338,274 H/s hardware,
  +0.42%/+0.45% over immediate control 85e. ARM keeps its CPU feeder.
- E09-b-pending is resolved and retained on both ISAs.
- E01-identity is implemented, paired-hardware validated, and retained.
- E01-rare candidate 83 and E01-window candidate 84 are retained on both ISAs.
  Fresh E02 profiling is next.

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
7. Candidate 93 implements that core-1 nested-loop shape. Build both, inspect
   assembly, then hardware-test independently.

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
