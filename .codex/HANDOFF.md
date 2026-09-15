# Pico 2 miner optimization handoff

## Objective

Continue `planned_optimizations_update.md` one experiment at a time. Build both
ISAs before flashing shared source, capture complete hardware output, archive
logs, append every result/failure to `perf_progress.md`, and commit between
functional attempts. Keep stock 150 MHz code experiments separate from clock
changes and keep temperature disabled.

## Current status

- HEAD: `00f6bab` (`perf: retain E01 identity contract`).
- Firmware/tooling source identity: `ebabf96f233d`.
- Device currently runs the retained RISC-V image at 150 MHz.
- Working tree was clean when this handoff was created.
- E09-b-pending is resolved and retained on both ISAs.
- E01-identity is implemented, paired-hardware validated, and retained.
- E01-rare candidate 83 is being implemented; E01-window follows, then fresh
  E02 profiling.

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
- One ARM flash completion failure was recorded at `bf7b8f4`; retry passed.

## Work in progress / next actions

1. Finish E01-rare candidate 83 and run host/build checks.
2. Direct fast-comparator/fallback tests cover high-word equality plus
   lower-word reject, exact equality, and valid share; independently verify
   emitted candidates where feasible.
3. Commit candidate, build both, then hardware-test both ISAs
   with 45-second cycles. Archive unique logs and record/commit each result.
4. Implement E01-window as a separate experiment; do not confuse existing
   mixed-window `hash_rate_hs` with the future common-window baseline.
5. Refresh E02 measurements only after the measurement contract is in place.

## Files and evidence

- Plan: `planned_optimizations_update.md`; historical detail:
  `planned_optimizations.md`.
- Ledger: `perf_progress.md` through E01 identity attempt 82c.
- E09 logs: `logs/E09b-fixed-full-rounds-{arm,riscv}.log`.
- E01 logs: `logs/E01-identity-{arm,riscv}.log` and
  `logs/E01-identity-arm-flash-failure.log`.
- E01 implementation: `CMakeLists.txt`, `src/main.c`, `tools/common.sh`,
  `tools/build`, `tools/cycle`, `tools/monitor.py`, `tools/test_monitor.py`.

## Tests and known problems

- `python3 -m unittest tools/test_monitor.py`: 6 passed.
- ARM and RISC-V builds warning-free with SDK 2.3.1 and repository wrappers.
- All paired hardware runs: 7 suites, 4,096 oracle cases, no faults.
- Current mining aggregate is still a sum of different timing windows; do not
  present it as E01-window completion.
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
