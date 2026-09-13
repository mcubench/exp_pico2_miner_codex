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

Safety constraints:

- Never run `picotool otp`, `picotool erase`, or commands that alter boot security, OTP, flash partition tables, or machine configuration.
- Do not use `sudo` from an autonomous coding cycle.
- Do not modify files outside this repository.
- Do not flash unless both architectures currently build, unless explicitly investigating an architecture-specific failure.
