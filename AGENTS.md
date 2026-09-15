# Pico 2 development instructions

The target is a Raspberry Pi Pico 2 with an RP2350 connected by its USB port.

Supported CPU architectures:

- ARM Cortex-M33: `rp2350-arm-s`
- Hazard3 RISC-V: `rp2350-riscv`

Use the repository wrappers; do not invoke compiler binaries directly.

- Build ARM: `./tools/build arm`
- Build RISC-V: `./tools/build riscv`
- Build both, flash, and capture logs: `./tools/cycle arm` or `./tools/cycle riscv`
- Capture finite serial output: `./tools/monitor --seconds 8`
- Diagnose the host setup: `./tools/doctor`

After changing platform-independent source:

1. Build ARM and RISC-V.
2. Fix all errors and warnings.
3. Flash the requested architecture.
4. Inspect serial output.   
5. Treat `TEST:FAIL`, `FAULT`, a timeout, or any nonzero command status as failure.
6. Diagnose, edit, and repeat until the hardware test passes.

Runtime output uses `TEST:PASS`, `TEST:FAIL`, and `TEST:SUMMARY` for known-answer tests; `BENCHMARK:PASS` for measured Bitcoin double-SHA-256 throughput; `MINING:START`, `MINING:PROGRESS`, and `SHARE:FOUND` for mining work; and `FAULT` for runtime failures.

Safety constraints:

- Never run `picotool otp`, `picotool erase`, or commands that alter boot security, OTP, flash partition tables, or machine configuration.
- Do not use `sudo` from an autonomous coding cycle.
- Do not modify files outside this repository.
- Do not flash unless both architectures currently build, unless explicitly investigating an architecture-specific failure.

## Codex Session Continuity

This repository may be worked on by multiple Codex sessions/accounts.

For any substantial task, read and follow:
`.codex/SESSION_CONTINUITY.md`

Maintain `.codex/HANDOFF.md` as described there so another Codex
session can safely continue interrupted work.

Never discard existing uncommitted changes merely because they were
created by another session.