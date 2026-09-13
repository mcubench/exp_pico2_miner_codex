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

## 2026-09-13 — Unrolled block-feed ARM result

- Change: poll `WDATA_RDY` once per complete 64-byte block instead of once per word, and unroll all 16 MMIO writes. RP2350 keeps `WDATA_RDY` asserted for the first 15 words and starts compression after word 16.
- Result: **169,096 H/s**
- Sample: 339,000 hashes in 2,004,782 us
- Relative to direct padded-block ARM result: **+62.9%**
- Relative to original ARM `pico_sha256` baseline: **4.91x**, or **+390.9%**
- Sustained mining samples: **166,386–166,426 H/s**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-blocks arch=ARM-M33 clock_hz=150000000 hashes=339000 elapsed_us=2004782 hash_rate_hs=169096 checksum=7e
MINING:PROGRESS arch=ARM-M33 nonce=100000 total_hashes=100000 hash_rate_hs=166386
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=166421
MINING:PROGRESS arch=ARM-M33 nonce=300000 total_hashes=300000 hash_rate_hs=166426
```

## 2026-09-13 — Inline digest-transfer ARM result

- Change: write the nonce as one aligned word; inline intermediate/final digest reads from the SHA SUM registers; eliminate temporary digest copies, helper calls, and redundant error clears.
- Initial build attempt: both targets rejected an unused legacy `write_le32` helper under `-Werror`; the helper was removed before flashing.
- Result: **207,736 H/s**
- Sample: 416,000 hashes in 2,002,537 us
- Relative to unrolled block-feed result: **+22.9%**
- Relative to original ARM `pico_sha256` baseline: **6.03x**, or **+503.2%**
- Sustained mining samples: **203,667–203,723 H/s**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-inline arch=ARM-M33 clock_hz=150000000 hashes=416000 elapsed_us=2002537 hash_rate_hs=207736 checksum=ff
MINING:PROGRESS arch=ARM-M33 nonce=100000 total_hashes=100000 hash_rate_hs=203667
MINING:PROGRESS arch=ARM-M33 nonce=400000 total_hashes=400000 hash_rate_hs=203719
```

## 2026-09-13 — O3 ARM result

- Change: compile the miner target at `-O3` while retaining debug symbols and all runtime validation.
- Result: **281,929 H/s**
- Sample: 564,000 hashes in 2,000,502 us
- Relative to inline digest-transfer result: **+35.7%**
- Relative to original ARM `pico_sha256` baseline: **8.19x**, or **+718.6%**
- Sustained mining samples, including target comparison and accounting: **253,818–254,592 H/s**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-inline-o3 arch=ARM-M33 clock_hz=150000000 hashes=564000 elapsed_us=2000502 hash_rate_hs=281929 checksum=5a
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=254577
MINING:PROGRESS arch=ARM-M33 nonce=500000 total_hashes=500000 hash_rate_hs=254592
```

## 2026-09-13 — Lazy final-result ARM result

- Change: leave the final digest in the SHA SUM registers; read only one word for the benchmark checksum and normally one most-significant word for difficulty-1 target rejection. Capture all eight words only for a candidate share or KAT output.
- Result: **294,091 H/s**
- Sample: 589,000 hashes in 2,002,782 us
- Relative to O3 result: **+4.3%** benchmark throughput
- Sustained mining: **289,328–289,501 H/s**, **+13.7%** over the prior ~254.6 kH/s mining loop
- Relative to original ARM `pico_sha256` baseline: **8.54x**, or **+753.9%**
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result arch=ARM-M33 clock_hz=150000000 hashes=589000 elapsed_us=2002782 hash_rate_hs=294091 checksum=cc
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=289480
MINING:PROGRESS arch=ARM-M33 nonce=600000 total_hashes=600000 hash_rate_hs=289501
```

## 2026-09-13 — SRAM execution experiment

- Change: copy the complete firmware to SRAM at boot so the hot loop does not execute through flash XIP.
- Result: **294,093 H/s**
- Sample: 589,000 hashes in 2,002,766 us
- Relative to XIP result: **+0.0007%**, indistinguishable from noise
- Sustained mining: **289,323–289,503 H/s**, also unchanged
- Decision: reject and revert; the XIP cache already serves the tight loop effectively, and copy-to-RAM needlessly consumes SRAM.
- Validation: all four KATs printed and passed; cycle result `CYCLE:PASS`

```text
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-ram arch=ARM-M33 clock_hz=150000000 hashes=589000 elapsed_us=2002766 hash_rate_hs=294093 checksum=cc
MINING:PROGRESS arch=ARM-M33 nonce=600000 total_hashes=600000 hash_rate_hs=289503
```

## 2026-09-13 — 200 MHz ARM result

- Change: raise `clk_sys` from the RP2350 stock 150 MHz to 200 MHz in firmware, retaining the default regulator voltage. This is an experimental overclock and is reversible by flashing a stock-clock build.
- Result: **392,124 H/s**
- Sample: 785,000 hashes in 2,001,920 us
- Relative to the same code at 150 MHz: **+33.3%**, essentially linear scaling
- Relative to original stock-clock ARM `pico_sha256` baseline: **11.39x**, or **+1038.5%**
- Sustained mining: **385,603–385,984 H/s**
- Validation: clock synthesis succeeded, all four KATs printed and passed at 200 MHz, and the cycle result was `CYCLE:PASS`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-200mhz arch=ARM-M33 clock_hz=200000000 hashes=785000 elapsed_us=2001920 hash_rate_hs=392124 checksum=b4
MINING:PROGRESS arch=ARM-M33 nonce=200000 total_hashes=200000 hash_rate_hs=385950
MINING:PROGRESS arch=ARM-M33 nonce=800000 total_hashes=800000 hash_rate_hs=385981
```

## 2026-09-13 — Temperature telemetry, rejected first reading

- Change: add 32-sample internal ADC temperature readings at boot, around the benchmark, and at each mining report.
- Performance: **392,125 H/s**, unchanged from the 200 MHz result; sustained mining **383,728–384,367 H/s** with telemetry overhead.
- Reported temperature: **1,015.828 C**, physically impossible and therefore invalid.
- Decision: reject the thermal measurement. Diagnose RP2350A temperature-channel selection and signed fixed-point conversion before logging a valid temperature.
- Hash validation: all four KATs and the cycle still passed.

```text
TEMP:BOOT source=rp2350-internal-adc approximate=1 temp_mc=1015828
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-200mhz arch=ARM-M33 clock_hz=200000000 hashes=785000 elapsed_us=2001914 hash_rate_hs=392125 checksum=b4 temp_start_mc=1015828 temp_end_mc=1015828
```

## 2026-09-13 — Temperature ADC diagnostics at stock clock

- 150 MHz, ADC clock 48 MHz: raw **4095**, `ADC_ERR=1`, `ERR_STICKY=1`; converted value rejected.
- 150 MHz, ADC clock 24 MHz: raw **4095**, same error flags; converted value rejected.
- Conclusion: the failure is not caused by the 200 MHz system overclock or insufficient ADC comparator time at 48 MHz.
- Hash rates: **294,091 H/s** at ADC 48 MHz and **294,090 H/s** at ADC 24 MHz; temperature sampling remains outside benchmark timing.
- Hash validation: all KATs and both hardware cycles passed.

```text
TEMP:BOOT source=rp2350-internal-adc approximate=1 temp_mc=-1479794 temp_raw=4095 adc_cs=00004703 adc_clock_hz=24000000
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-tempdiag24 arch=ARM-M33 clock_hz=150000000 hashes=589000 elapsed_us=2002790 hash_rate_hs=294090 checksum=cc temp_start_mc=-1479794 temp_end_mc=-1479794 temp_start_raw=4095 temp_end_raw=4095
```
