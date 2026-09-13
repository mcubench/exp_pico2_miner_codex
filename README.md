# Pico 2 agentic development loop

This project builds the same USB-serial LED blinker for both RP2350 CPU architectures and gives VS Code/Codex finite commands for build, flash, and hardware feedback.

## Commands

```sh
./tools/doctor
./tools/build arm
./tools/build riscv
./tools/cycle arm
./tools/cycle riscv
./tools/monitor --seconds 30
```

`cycle` always builds both architectures before flashing the selected one. Runtime output contains `BOOT`, `TEST:PASS`, and `HEARTBEAT` records that Codex can evaluate reliably.

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
