# Pico 2 Bitcoin SHA-256 Performance Log

All rates below count one complete Bitcoin header hash: SHA-256(SHA-256(80-byte header)).
Tests use the RP2350 hardware SHA-256 engine at the stock 150 MHz system clock.

## 2026-09-13 — ARM Cortex-M33 baseline

- Build: `ARM-M33`, Pico SDK 2.3.1, Release
- Implementation: `pico_sha256` blocking API, CPU-fed (`use_dma=false`)
- Workload: full 80-byte first hash plus 32-byte second hash for every nonce
- Result: **34,427 H/s**
- Sample: 69,000 hashes in 2,004,221 us
- Benchmark checksum: `2e`
- Continuous-mining sample: 100,000 hashes at **34,343 H/s**
- Validation completed before measurement: SHA-256 empty string, SHA-256 `abc`, Bitcoin genesis block hash, compact target expansion, and discovery of genesis nonce `2083236893` in 94 attempts.

Serial record:

```text
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 arch=ARM-M33 clock_hz=150000000 hashes=69000 elapsed_us=2004221 hash_rate_hs=34427 checksum=2e
MINING:PROGRESS arch=ARM-M33 nonce=100000 total_hashes=100000 hash_rate_hs=34343
```

## Experiment notes

- A subsequent firmware build changed startup from a fixed delay to waiting for a USB CDC connection. The host found the serial device but received no output, so this is treated as a synchronization regression rather than a performance result. It will not be used as a benchmark.

## 2026-09-13 — ARM Cortex-M33 confirmed run

- Startup synchronization: fixed 3.5-second delay before one-time serial records
- Result: **34,441 H/s**
- Sample: 69,000 hashes in 2,003,417 us
- Change from baseline: **+0.04%**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:PASS kat=nist_empty engine=RP2350-SHA256
TEST:PASS kat=nist_abc engine=RP2350-SHA256
TEST:PASS kat=bitcoin_genesis hash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
TEST:PASS kat=bitcoin_nonce_search nonce=2083236893 attempts=94 hash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 arch=ARM-M33 clock_hz=150000000 hashes=69000 elapsed_us=2003417 hash_rate_hs=34441 checksum=2e
```

## 2026-09-13 — Hazard3 RISC-V baseline

- Build: `RISCV-HAZARD3`, Pico SDK 2.3.1, Debug/optimized, stock 150 MHz
- Implementation: same `pico_sha256` blocking, CPU-fed code as ARM
- Result: **32,084 H/s**
- Sample: 65,000 hashes in 2,025,939 us
- Relative to confirmed ARM run: **-6.84%**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 arch=RISCV-HAZARD3 clock_hz=150000000 hashes=65000 elapsed_us=2025939 hash_rate_hs=32084 checksum=4c
```

## 2026-09-13 — Direct padded-block optimization, first flash attempt

- Change: keep the boot-ROM SHA-256 lock across a nonce batch, prebuild the two padded blocks for the 80-byte header, prebuild the padded block for the intermediate digest, and feed aligned words directly to the peripheral.
- ARM and RISC-V builds: passed with warnings treated as errors.
- Hardware result: **no measurement**. The Pico runtime USB device was no longer accessible when `picotool` attempted the reboot-to-BOOTSEL transition, so flashing did not begin.

## 2026-09-13 — Direct padded-block ARM result

- Build: `ARM-M33`, Pico SDK 2.3.1, Debug/optimized, stock 150 MHz
- Implementation: keep the SHA-256 lock for the batch and directly feed three pre-padded 64-byte blocks per nonce (two for the 80-byte header, one for its intermediate digest)
- Result: **103,800 H/s**
- Sample: 208,000 hashes in 2,003,862 us
- Relative to confirmed ARM `pico_sha256` baseline: **3.01x**, or **+201.4%**
- Sustained mining samples: **102,773 H/s** and **102,787 H/s** per 100,000-nonce interval
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-padded-blocks arch=ARM-M33 clock_hz=150000000 hashes=208000 elapsed_us=2003862 hash_rate_hs=103800 checksum=fa
MINING:PROGRESS arch=ARM-M33 nonce=100000 total_hashes=100000 hash_rate_hs=102773
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=102787
```

## 2026-09-13 — Direct padded-block RISC-V result

- Build: `RISCV-HAZARD3`, Pico SDK 2.3.1, Debug/optimized, stock 150 MHz
- Implementation: identical direct padded-block path used by the ARM test
- Result: **86,300 H/s**
- Sample: 173,000 hashes in 2,004,625 us
- Relative to RISC-V `pico_sha256` baseline: **2.69x**, or **+169.0%**
- Relative to optimized ARM: ARM is **20.3% faster** (`103,800 / 86,300`)
- Sustained mining samples: **85,675 H/s** and **85,695 H/s** per 100,000-nonce interval
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-padded-blocks arch=RISCV-HAZARD3 clock_hz=150000000 hashes=173000 elapsed_us=2004625 hash_rate_hs=86300 checksum=9b
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=100000 total_hashes=100000 hash_rate_hs=85675
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=200000 total_hashes=200000 hash_rate_hs=85695
```

## 2026-09-13 — Final ARM repeatability run

- Purpose: restore the faster architecture as the running firmware and confirm repeatability
- Result: **103,800 H/s**
- Sample: 208,000 hashes in 2,003,849 us
- Difference from the first optimized ARM benchmark: below the displayed 1 H/s resolution
- Sustained mining samples: **102,625 H/s** and **102,786 H/s**
- Validation: both architectures rebuilt successfully; ARM flash verified; all four KATs and the hardware cycle passed

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-padded-blocks arch=ARM-M33 clock_hz=150000000 hashes=208000 elapsed_us=2003849 hash_rate_hs=103800 checksum=fa
MINING:PROGRESS arch=ARM-M33 nonce=100000 total_hashes=100000 hash_rate_hs=102625
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=102786
```
