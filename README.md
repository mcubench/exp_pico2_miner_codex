# Experiment: Pico 2 agentic development loop (bitcoin miner)

This experimental project project builds a hardware-accelerated Bitcoin proof-of-work engine for both RP2350 CPU architectures by giving Codex finite commands for build, flash, and hardware feedback. GPT-6 Astra (for planning) and GPT-5-6 Sol (coding and testing) was used, mostly autonomously - after initial setup a human had to only occasionally (~10x) reinsert token and specify general requirements. If you just want the resulting optimized Pico2 miner, visit [pico2-btcminer](https://github.com/mcubench/pico2-btcminer) instead.

See the [summary.md](summary.md), step-by-step progress report in [perf_progress.md](perf_progress.md), with visualized [optimization progress](/docs/pico2_sha256_hashrate_evolution_v9_unroll_and_filter.html) and [overclocking trends](/docs/overclock_frontier_hazard3_vs_cortex_m33_labelled.html).

<img width="1956" height="1078" alt="image" src="https://github.com/user-attachments/assets/54d94af9-0327-45de-8a5e-4c5d5c9bc557" />

It uses RP2350's native SHA-256 peripheral for both rounds of every Bitcoin header hash. The mining loop retains the SHA peripheral lock and directly feeds three pre-padded 64-byte blocks per nonce, avoiding high-level API setup and padding overhead inside the hot path.
At boot the firmware validates the engine against SHA-256 known-answer vectors, the Bitcoin genesis block hash, and a real compact-target nonce search. It then measures double-SHA-256 hashes per second and continuously scans the genesis header's difficulty-1 nonce space. The ongoing work is deliberately standalone and stale; it demonstrates genuine proof-of-work calculations but does not connect to a pool or the Bitcoin peer-to-peer network.

## Commands

```sh
./tools/doctor
./tools/build arm
./tools/build riscv
./tools/cycle arm
./tools/cycle riscv
./tools/monitor --seconds 30
```

`cycle` always builds both architectures before flashing the selected one. Runtime output contains machine-readable `BOOT`, `TEST`, `BENCHMARK`, `MINING`, `SHARE`, and `FAULT` records that Codex can evaluate reliably.

Measured on the connected Pico 2 at the stock 150 MHz clock, the optimized path reaches 103,800 H/s on ARM Cortex-M33 and 86,300 H/s on Hazard3 RISC-V. These are complete Bitcoin double-SHA-256 header hashes, not individual SHA-256 compression rounds. See [`perf_progress.md`](perf_progress.md) for baselines, optimization history, exact samples, and continuous-mining rates.

The scripts discover extension-managed dependencies below `~/.pico-sdk`. These environment variables can override discovery: `PICO_HOME`, `PICO_SDK_PATH`, `ARM_TOOLCHAIN`, `RISCV_TOOLCHAIN`, `PICOTOOL`, and `PICO_PORT`.

## VS Code

Use **Terminal > Run Task** and choose one of the `Pico 2:` tasks. The official Raspberry Pi Pico extension recognizes this directory because it contains `pico_sdk_import.cmake`.

## USB permissions

On Linux, the logged-in user must be able to access both the Pico's USB device (for `picotool`) and `/dev/ttyACM*` (for logs). Membership in `dialout` takes effect fully after signing out and back in; the monitor wrapper can enter an already-configured group in the meantime. A suitable udev rule is also needed for non-root `picotool` access.

The first flash can be done while holding BOOTSEL during USB connection. After this firmware is running, `picotool load -f` uses the Pico SDK USB reset interface, so subsequent cycles should require no button press.

A single USB cable supports flashing and printf-based diagnosis. Halt/breakpoint/source-level debugging additionally requires an SWD adapter such as Raspberry Pi Debug Probe.

## VirtualBox USB passthrough

The Pico changes USB identity while flashing. A VirtualBox filter that matches only the running firmware will lose the board as soon as `picotool` resets it into BOOTSEL mode. Configure a persistent USB filter with vendor ID `2e8a` and leave product ID blank so both identities are captured:

- Pico SDK application: `2e8a:0009`
- RP2350 BOOTSEL: `2e8a:000f`

If the board is already missing from the guest, reconnect it while holding BOOTSEL, select the RP2350/RP2 boot device from **VirtualBox > Devices > USB**, and then run `./tools/flash arm`. After execution, ensure the running Pico identity is also attached for `/dev/ttyACM*` logging.
