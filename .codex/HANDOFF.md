# Pico 2 miner optimization handoff

## Objective

Continue `planned_optimizations_update.md` one experiment at a time. Build both
ISAs before flashing shared source, capture complete hardware output, archive
logs, append every result/failure to `perf_progress.md`, and commit between
functional attempts. Keep stock 150 MHz code experiments separate from clock
changes and keep temperature disabled.

## Current status

- HEAD: `cf1ab1f` (`perf: retain E01 synchronized measurements`).
- Retained firmware/tooling source identity: `f2687597e485`; E02 profile
  candidate is currently dirty until its pre-hardware commit.
- Device currently runs the retained E01-window RISC-V image at 150 MHz.
- Working tree contains only the intentional E02 profiling implementation and
  candidate ledger/handoff updates.
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
  builds pass; hardware validation is pending.
- One ARM flash completion failure was recorded at `bf7b8f4`; retry passed.

## Work in progress / next actions

1. Commit the E02 profiling candidate before hardware use.
2. Rebuild both profile images from the committed source, flash/capture ARM
   then RISC-V, archive and record each result, then restore a normal image.
3. Use E02 evidence to select the next sequence item, with E06-trigger the
   plan's default next code experiment.

## Files and evidence

- Plan: `planned_optimizations_update.md`; historical detail:
  `planned_optimizations.md`.
- Ledger: `perf_progress.md` through E01-window RISC-V attempt 84b.
- E09 logs: `logs/E09b-fixed-full-rounds-{arm,riscv}.log`.
- E01 logs: `logs/E01-identity-{arm,riscv}.log` and
  `logs/E01-identity-arm-flash-failure.log`.
- E01-window logs: `logs/E01-window-{arm,riscv}.log`.
- E01 implementation: `CMakeLists.txt`, `src/main.c`, `tools/common.sh`,
  `tools/build`, `tools/cycle`, `tools/monitor.py`, `tools/test_monitor.py`.

## Tests and known problems

- `python3 -m unittest tools/test_monitor.py`: 8 passed.
- ARM and RISC-V builds warning-free with SDK 2.3.1 and repository wrappers.
- Current hardware runs: 8 suites, 4,096 oracle cases, no faults.
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
