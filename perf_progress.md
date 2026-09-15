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

## 2026-09-13 — E00 RP2350B channel-8 attempt: invalid temperature

- Experiment: `E00-rp2350b-channel8-arm-01`
- Change: select a repository-local RP2350B board definition, assert nine ADC
  channels and temperature channel 8 at compile time, use the normal 48 MHz
  ADC clock, and reject ADC errors/saturated samples before benchmarking.
- Build: ARM and RISC-V passed without warnings; ARM was flashed at 150 MHz.
- Temperature result: **invalid**. All 32 boot samples were raw **4095**;
  raw minimum/maximum were also 4095. ADC status `0x00008703` showed channel 8
  selected and both current/sticky conversion errors.
- Performance: **not measured**. The new validity gate emitted `FAULT` and
  stopped before the KAT/benchmark, as designed.
- Decision: E00 remains incomplete. Channel 8 selection alone did not repair
  the physical conversion failure; diagnose package/board-specific ADC setup
  before any speed experiment.
- Firmware UF2 SHA-256:
  `360c65f853597331583b4d8753897b345ec002536d27c1d345c5c565072e1a64`
- Archived serial log: `logs/E00-rp2350b-arm-invalid-temperature.log`

```text
BOOT app=pico2_bitcoin_miner board=miner_rp2350b package=RP2350B arch=ARM-M33 engine=RP2350-SHA256 adc_temp_channel=8
TEMP:BOOT source=rp2350-internal-adc approximate=1 temp_valid=0 temp_mc=-1479794 temp_raw=4095 temp_raw_min=4095 temp_raw_max=4095 adc_cs=00008703 adc_clock_hz=48000000 adc_channel=8
FAULT type=temperature boot=1 temp_valid=0
```

## 2026-09-14 — E00 all-channel ADC isolation: ADC subsystem unavailable

- Experiment: `E00-rp2350a-all-adc-arm-04`
- Change: sample every RP2350A ADC mux input (GPIO channels 0–3 and internal
  temperature channel 4), eight conversions per channel, after disabling the
  corresponding GPIO digital functions. Record status before and during each
  acquisition.
- Build: ARM and RISC-V passed without warnings; ARM was flashed at 150 MHz.
- Result: **all five channels failed identically**. Every conversion returned
  raw 4095 and set both current/sticky ADC conversion errors. Channel 3, which
  is VSYS/3 on an official Pico 2, failed exactly like the temperature channel.
- Temperature: **unavailable** (`temperature_valid=0`); no temperature value is
  accepted from these samples.
- Performance: **not measured** because this diagnostic still used the strict
  boot thermal gate.
- Conclusion: the failure is ADC-wide, not a temperature mux/channel bug.
  Likely physical causes include absent/incorrect ADC_AVDD/reference wiring on
  the carrier or faulty ADC hardware. Continue stock-150-MHz software/hash
  experiments with explicit invalid-temperature telemetry, but block thermal
  qualification and all further overclocking until hardware ADC operation is
  restored or an external sensor is provided.
- Firmware UF2 SHA-256:
  `ff0f4e8544ddeb82bdc824eaa60db479659985830167d50711d4238af7dbe138`
- Archived serial log: `logs/E00-rp2350a-all-adc-channels-invalid.log`

```text
ADC:DIAG channel=0 kind=gpio samples=8 raw_mean=4095 raw_min=4095 raw_max=4095 cs_before=00000703 cs_or=00000703
ADC:DIAG channel=1 kind=gpio samples=8 raw_mean=4095 raw_min=4095 raw_max=4095 cs_before=00001703 cs_or=00001703
ADC:DIAG channel=2 kind=gpio samples=8 raw_mean=4095 raw_min=4095 raw_max=4095 cs_before=00002703 cs_or=00002703
ADC:DIAG channel=3 kind=gpio samples=8 raw_mean=4095 raw_min=4095 raw_max=4095 cs_before=00003703 cs_or=00003703
ADC:DIAG channel=4 kind=temperature samples=8 raw_mean=4095 raw_min=4095 raw_max=4095 cs_before=00004703 cs_or=00004703
```

## 2026-09-14 — E00 completed with temperature disabled: ARM baseline

- Experiment: `E00-rp2350a-stock-arm-05`
- Change: remove ADC initialization/sampling and its runtime gate at the user's
  request; retain hardware package identity checks. All output explicitly says
  `temperature=disabled`. System clock remained stock 150 MHz.
- Validation: all four KATs passed, including the 94-attempt genesis nonce
  search; cycle result `CYCLE:PASS`.
- Kernel benchmark: **294,090 H/s**, 589,000 complete Bitcoin double hashes in
  2,002,785 us, checksum `cc`.
- Sustained mining: **289,439–289,452 H/s** after startup convergence.
- Relative to the prior optimized 150 MHz ARM baseline (294,091 H/s): unchanged
  within measurement resolution.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept as the stock ARM reference for later code experiments. Keep
  clock changes separate from kernel comparisons.
- Firmware UF2 SHA-256:
  `1645213f8825b61dd7257954d1e92ebe2ed44c681047c6b69717ca4894a9ccfe`
- Archived serial log: `logs/E00-rp2350a-stock-arm-baseline.log`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-rp2350a-stock arch=ARM-M33 clock_hz=150000000 hashes=589000 elapsed_us=2002785 hash_rate_hs=294090 checksum=cc temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=1700000 total_hashes=1700000 hash_rate_hs=289452 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=4000000 total_hashes=4000000 hash_rate_hs=289445 temperature=disabled
```

## 2026-09-14 — E00 matched RISC-V stock baseline

- Experiment: `E00-rp2350a-stock-riscv-06`
- Build/workload: same temperature-disabled source and 150 MHz clock as the
  preceding ARM reference; Hazard3 RISC-V target.
- Validation: all four KATs passed, including the 94-attempt genesis nonce
  search; cycle result `CYCLE:PASS`.
- Kernel benchmark: **290,664 H/s**, 582,000 complete Bitcoin double hashes in
  2,002,310 us, checksum `75`.
- Sustained mining after startup convergence: **286,697–286,705 H/s**.
- Architecture comparison: the matched ARM result is 1.18% faster in the kernel
  benchmark and approximately 0.96% faster in sustained mining. The much older
  86,300 H/s RISC-V entry predates the current optimized kernel and is not a
  valid current-architecture comparison.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept as the stock RISC-V reference. Continue code experiments on
  both builds; use ARM as the first flash target when only one is needed.
- Firmware UF2 SHA-256:
  `cf7f53084ff7dd5aa3f81c9304982e8e8a2e86ef2157825bf96cdd9a018f35ca`
- Archived serial log: `logs/E00-rp2350a-stock-riscv-baseline.log`

```text
TEST:SUMMARY pass=4 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-rp2350a-stock arch=RISCV-HAZARD3 clock_hz=150000000 hashes=582000 elapsed_us=2002310 hash_rate_hs=290664 checksum=75 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=1500000 total_hashes=1500000 hash_rate_hs=286705 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=4000000 total_hashes=4000000 hash_rate_hs=286701 temperature=disabled
```

## 2026-09-13 — E00 hardware package-identity check: RP2350A reported

- Experiment: `E00-sysinfo-package-identity-arm-02`
- Change: read the RP2350's `SYSINFO.CHIP_ID` and `SYSINFO.PACKAGE_SEL`
  registers directly at boot, before accepting the compile-time board target.
- Build: ARM and RISC-V passed without warnings; ARM was flashed at 150 MHz.
- Hardware identity result: `PACKAGE_SEL=1`, which the RP2350 datasheet defines
  as QFN-60 / RP2350A. QFN-80 / RP2350B would report zero. Chip ID was
  `0x30004927`; the SDK decoded silicon revision as `3`.
- Performance and temperature: **not measured** in this attempt. The strict
  package check emitted `FAULT` before temperature sampling or benchmarking.
- Decision: the connected silicon's package identity conflicts with the stated
  RP2350B target and explains why channel 8 produces conversion errors. Stop E00
  until the physical board/chip identity is confirmed; do not bypass the check
  or optimize against knowingly mismatched ADC/package configuration.
- Firmware UF2 SHA-256:
  `5acc7af668e0977e6f1b1844d7887a9834467381503085d9b6b287b3c85165c1`
- Archived serial log: `logs/E00-sysinfo-package-identity-arm.log`

```text
BOOT app=pico2_bitcoin_miner board=miner_rp2350b package=RP2350B arch=ARM-M33 engine=RP2350-SHA256 adc_temp_channel=8 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
FAULT type=package_mismatch expected_sysinfo_package_sel=0 actual_sysinfo_package_sel=1
```

## 2026-09-14 — E00 RP2350A channel-4 confirmation: ADC still invalid

- Experiment: `E00-rp2350a-channel4-arm-03`
- Change: accept the hardware identity as authoritative, restore the official
  `pico2` / RP2350A definition and temperature channel 4, retain direct package
  diagnostics, and use the SDK-initialized 48 MHz ADC clock without reconfiguring
  it in application code. The optimization plan was corrected to RP2350A.
- Build: ARM and RISC-V passed without warnings; ARM was flashed at 150 MHz.
- Identity: `PACKAGE_SEL=1` matched RP2350A; compile-time and runtime temperature
  channel were both 4.
- Temperature result: **invalid**. All 32 boot samples were raw **4095**;
  raw minimum/maximum were 4095. ADC status `0x00004703` showed current and
  sticky conversion errors at an actual 48 MHz ADC clock.
- Performance: **not measured**. The thermal validity gate stopped before KATs
  and benchmarking.
- Decision: package selection is now resolved, but E00 remains incomplete.
  Isolate ADC/reference/supply behavior before optimization measurements.
- Firmware UF2 SHA-256:
  `0e147b3cc3ce7c2f0bc821af8f9bfda3fdd0c730562dd0371f8ba254783c0dec`
- Archived serial log: `logs/E00-rp2350a-channel4-arm-invalid-temperature.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 adc_temp_channel=4 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEMP:BOOT source=rp2350-internal-adc approximate=1 temp_valid=0 temp_mc=-1479794 temp_raw=4095 temp_raw_min=4095 temp_raw_max=4095 adc_cs=00004703 adc_clock_hz=48000000 adc_channel=4
FAULT type=temperature boot=1 temp_valid=0
```

## 2026-09-14 — E01 independent optimized-kernel oracle: ARM

- Experiment: `E01-oracle-arm-01`
- Change: generate 4,096 deterministic Bitcoin-header fixtures with Python's
  `hashlib`, compare every complete 256-bit optimized-kernel digest on-device,
  add 10 compact-target boundary tests, and require both the exact successful
  test summary and benchmark marker in the serial monitor.
- Build: ARM and RISC-V passed without warnings; checkpoint `9f87e98` was
  committed before flashing ARM at the stock 150 MHz clock.
- Validation: all six test groups passed. The independent optimized-path oracle
  passed all **4,096/4,096** headers; all **10/10** target-boundary cases passed;
  cycle result `CYCLE:PASS`.
- Kernel benchmark: **294,091 H/s**, 589,000 complete Bitcoin double hashes in
  2,002,779 us, checksum `cc`.
- Sustained mining: converged to **289,441 H/s** at 600,000 total hashes.
- Relative to E00 ARM: +1 H/s in the benchmark and effectively unchanged
  sustained throughput; the validation data and boot-time tests are outside the
  timed kernel.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E01 on ARM. Run the identical checkpoint on Hazard3 before
  changing the hashing path.
- Firmware UF2 SHA-256:
  `ac7ec9f5c8b7262fbc83a6834fb14dac4cd49dfc0df7671e30cf83ee455d8bea`
- Archived serial log: `logs/E01-oracle-arm.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:PASS kat=target_boundaries cases=10
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-rp2350a-stock arch=ARM-M33 clock_hz=150000000 hashes=589000 elapsed_us=2002779 hash_rate_hs=294091 checksum=cc temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=600000 total_hashes=600000 hash_rate_hs=289441 temperature=disabled
```

## 2026-09-14 — E01 independent optimized-kernel oracle: RISC-V

- Experiment: `E01-oracle-riscv-02`
- Build/workload: identical checkpoint `9f87e98`, fixtures, and 150 MHz clock as
  the accepted E01 ARM run; Hazard3 RISC-V target.
- Validation: all six test groups passed. The independent optimized-path oracle
  passed all **4,096/4,096** headers and all **10/10** target-boundary cases;
  cycle result `CYCLE:PASS`.
- Kernel benchmark: **290,667 H/s**, 582,000 complete Bitcoin double hashes in
  2,002,288 us, checksum `75`.
- Sustained mining: converged to **286,690 H/s** at 600,000 total hashes.
- Relative to E00 RISC-V: +3 H/s in the benchmark and -11 H/s sustained, both
  negligible run-to-run variation.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E01 on both architectures. Use this harness as the mandatory
  correctness gate for subsequent kernel changes.
- Firmware UF2 SHA-256:
  `2febb69665d8e32cd52cabf699e0315ac9f3f0e8de490a157a0a9e4ba65696f2`
- Archived serial log: `logs/E01-oracle-riscv.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:PASS kat=target_boundaries cases=10
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-unrolled-o3-lazy-result-rp2350a-stock arch=RISCV-HAZARD3 clock_hz=150000000 hashes=582000 elapsed_us=2002288 hash_rate_hs=290667 checksum=75 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=286690 temperature=disabled
```

## 2026-09-14 — E02 matched cycle-budget and assembly profile

- Experiment: `E02-static-profile-01`
- Inputs: the fresh E01 150 MHz measurements, exact compile commands, link
  maps, binary sizes, and source-correlated disassembly for both architectures.
  No new firmware was flashed and no new throughput sample was obtained.
- Reproducibility: added `./tools/analyze arm|riscv summary|disassembly
  [symbol]`, which locates the repository-selected toolchain instead of invoking
  a compiler utility ad hoc.
- Effective main-source flags: ARM uses Cortex-M33/Thumb/Armv8-M Main plus `-O3`;
  RISC-V uses `-mcpu=hazard3-rp2350` plus `-O3`. CMake's earlier `-Og` remains
  in each command but is superseded by the later target `-O3`.
- E01 benchmark cycle budget at the measured 150 MHz clock:
  - ARM: 150,000,000 / 294,091 = **510.05 cycles/hash**; **147.05 cycles**
    above the 363-cycle three-compression hardware-only floor.
  - RISC-V: 150,000,000 / 290,667 = **516.05 cycles/hash**; **153.05 cycles**
    above that floor.
- E01 sustained cycle budget: ARM **518.24 cycles/hash** at 289,441 H/s;
  RISC-V **523.21 cycles/hash** at 286,690 H/s.
- Assembly finding: both hot paths copy the intermediate state through SRAM on
  every nonce: 8 MMIO loads, 8 byte-reverse instructions (`rev` / `rev8`), 8
  SRAM stores, then 8 SRAM reloads before WDATA writes. This is directly
  removable by E03. Both compilers already emit native one-instruction byte
  reversal, so the expected gain is primarily transfer/load-store overhead.
- Stack frames: ARM mining function **300 bytes**; RISC-V **352 bytes**. Current
  ELF allocation: ARM text/data/bss **175,980/0/2,640 bytes**; RISC-V
  **183,860/0/2,388 bytes**. The 4,096-vector oracle dominates the text/rodata
  increase but is outside the timed hot loop.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: E03-a preformatted numeric SHA words is the next justified variant;
  follow it with E03-b register-only digest handoff, inspecting both assemblies.

## 2026-09-14 — E03-a numeric SHA words: ARM

- Experiment: `E03a-numeric-words-arm-01`
- Change: preformat the 80-byte job as numeric big-endian SHA words once, run
  the SHA peripheral with BSWAP disabled, byte-reverse only the changing nonce,
  and transfer raw numeric `SUM0..7` words to the second-hash SRAM block. Clock
  remained stock 150 MHz. Source checkpoint: `5cbc8de`.
- Build/validation: ARM and RISC-V passed without warnings. On ARM all six test
  groups passed, including **4,096/4,096** optimized-path oracle cases and
  **10/10** target-boundary cases; cycle result `CYCLE:PASS`.
- Kernel benchmark: **303,003 H/s**, 607,000 complete Bitcoin double hashes in
  2,003,282 us, checksum `b6`; estimated **495.04 cycles/hash**.
- Sustained mining: converged to **298,082 H/s** at 600,000 total hashes.
- Relative to E01 ARM (294,091 benchmark; 289,441 sustained): **+3.03%**
  benchmark and **+2.98%** sustained. The kernel saves about **15.00
  cycles/hash**.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain E03-a on ARM and test the identical source on Hazard3 before
  proceeding to register-only handoff.
- Firmware UF2 SHA-256:
  `fe09fbe00405c4de1d810622fb335d48f30f53b37fea2c45ff7eaa4075d8b763`
- Archived serial log: `logs/E03a-numeric-words-arm.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-numeric-words-e03a arch=ARM-M33 clock_hz=150000000 hashes=607000 elapsed_us=2003282 hash_rate_hs=303003 checksum=b6 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=600000 total_hashes=600000 hash_rate_hs=298082 temperature=disabled
```

## 2026-09-14 — E03-a numeric SHA words: RISC-V

- Experiment: `E03a-numeric-words-riscv-02`
- Build/workload: identical E03-a checkpoint `5cbc8de`, fixtures, and stock
  150 MHz clock as the ARM run; Hazard3 RISC-V target.
- Validation: all six test groups passed, including **4,096/4,096** oracle
  cases and **10/10** target-boundary cases; cycle result `CYCLE:PASS`.
- Kernel benchmark: **298,774 H/s**, 598,000 complete Bitcoin double hashes in
  2,001,511 us, checksum `97`; estimated **502.05 cycles/hash**.
- Sustained mining: reached **294,576 H/s** at 700,000 total hashes (the first
  100,000 window was a startup outlier at 290,600 H/s).
- Relative to E01 RISC-V (290,667 benchmark; 286,690 sustained): **+2.79%**
  benchmark and **+2.75%** sustained. The kernel saves about **14.00
  cycles/hash**.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain E03-a for both architectures. Proceed to E03-b direct
  register handoff as a separate functional checkpoint.
- Firmware UF2 SHA-256:
  `18555b28eefce040b3a897443939484cf5228e955c3d49a016f7c6ea4e3508f3`
- Archived serial log: `logs/E03a-numeric-words-riscv.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-numeric-words-e03a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=598000 elapsed_us=2001511 hash_rate_hs=298774 checksum=97 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=294576 temperature=disabled
```

## 2026-09-14 — E03-b register-only digest handoff: ARM

- Experiment: `E03b-register-handoff-arm-01`
- Change: preserve all eight first-hash `SUM` words in explicit locals before
  reset, then feed those values and constant padding directly to WDATA; remove
  the 64-byte second-hash SRAM block. Clock remained stock 150 MHz. Source
  checkpoint: `b03dc0b`.
- Assembly: ARM retained seven digest words in registers and spilled/reloaded
  only the eighth (2 SRAM operations rather than E03-a's 16). The mining stack
  frame shrank from 300 to 268 bytes.
- Build/validation: ARM and RISC-V passed without warnings. On ARM all six test
  groups passed, including **4,096/4,096** optimized-path oracle cases and
  **10/10** target-boundary cases; cycle result `CYCLE:PASS`.
- Kernel benchmark: **308,608 H/s**, 618,000 complete Bitcoin double hashes in
  2,002,542 us, checksum `3e`; estimated **486.05 cycles/hash**.
- Sustained mining: converged to **306,029 H/s** at 700,000 total hashes.
- Relative to E03-a ARM: **+1.85%** benchmark and **+2.67%** sustained, saving
  about **8.99 cycles/hash**. Relative to E01: **+4.94%** benchmark and
  **+5.73%** sustained, saving about **23.99 cycles/hash**.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain E03-b on ARM and test identical source on Hazard3.
- Firmware UF2 SHA-256:
  `df0dc567bc2abd04f7dbb2bcdfcd224c20d197516f3ac7fe53789bba2a59d24b`
- Archived serial log: `logs/E03b-register-handoff-arm.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-register-handoff-e03b arch=ARM-M33 clock_hz=150000000 hashes=618000 elapsed_us=2002542 hash_rate_hs=308608 checksum=3e temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=306029 temperature=disabled
```

## 2026-09-14 — E03-b register-only digest handoff: RISC-V

- Experiment: `E03b-register-handoff-riscv-02`
- Build/workload: identical E03-b checkpoint `b03dc0b`, fixtures, and stock
  150 MHz clock as the ARM run; Hazard3 RISC-V target.
- Assembly: Hazard3 retained all eight intermediate digest words in registers;
  there were no SRAM spills in the handoff.
- Validation: all six test groups passed, including **4,096/4,096** oracle
  cases and **10/10** target-boundary cases; cycle result `CYCLE:PASS`.
- Kernel benchmark: **313,774 H/s**, 628,000 complete Bitcoin double hashes in
  2,001,442 us, checksum `32`; estimated **478.05 cycles/hash**.
- Sustained mining: converged to **304,119 H/s** at 700,000 total hashes.
- Relative to E03-a RISC-V: **+5.02%** benchmark and **+3.24%** sustained,
  saving about **24.00 cycles/hash**. Relative to E01: **+7.95%** benchmark and
  **+6.08%** sustained, saving about **38.00 cycles/hash**.
- Cross-architecture: E03-b RISC-V benchmark is **1.67% faster** than ARM, but
  ARM's short sustained sample is **0.63% faster**. Keep reporting both paths.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E03-b as the new common kernel baseline. Proceed to E04
  subvariants one at a time at 150 MHz before any clock sweep.
- Firmware UF2 SHA-256:
  `4ff352a9eb782a19d272598363d9fb07baafa8585e179e820e2e8211feb5346c`
- Archived serial log: `logs/E03b-register-handoff-riscv.log`

```text
TEST:PASS kat=optimized_oracle cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=direct-register-handoff-e03b arch=RISCV-HAZARD3 clock_hz=150000000 hashes=628000 elapsed_us=2001442 hash_rate_hs=313774 checksum=32 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=304119 temperature=disabled
```

## 2026-09-14 — E04-a constant header-tail feeder: ARM

- Experiment: `E04a-constant-tail-arm-01`
- Change: retain only the 20 unpadded header words, load the four second-block
  data words, and emit padding, ten zeros, and 640-bit length directly to WDATA.
  The per-job state shrank by 48 bytes. Clock remained stock 150 MHz. Source
  checkpoint: `ef7e379`.
- Build/validation: both architectures passed without warnings. ARM passed all
  six test groups, **4,096/4,096** oracle cases and **10/10** target tests;
  cycle result `CYCLE:PASS`.
- Kernel benchmark: **317,088 H/s**, 635,000 hashes in 2,002,598 us, checksum
  `f6`; estimated **473.05 cycles/hash**.
- Sustained mining: converged to **311,098 H/s** at 700,000 hashes.
- Relative to E03-b ARM: **+2.75%** benchmark, **+1.66%** sustained, and about
  **13.00 cycles/hash** saved. Relative to E01: **+7.82%** benchmark and
  **+7.48%** sustained.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain on ARM; test identical source on Hazard3.
- Firmware UF2 SHA-256:
  `7edc6800b005b87f3c96c5e693ece1f31f2e278b87e325557767efb2fd5039e8`
- Archived serial log: `logs/E04a-constant-tail-arm.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=constant-header-tail-e04a arch=ARM-M33 clock_hz=150000000 hashes=635000 elapsed_us=2002598 hash_rate_hs=317088 checksum=f6 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=311098 temperature=disabled
```

## 2026-09-14 — E04-a constant header-tail feeder: RISC-V

- Experiment: `E04a-constant-tail-riscv-02`
- Build/workload: identical checkpoint `ef7e379` and stock 150 MHz clock;
  Hazard3 RISC-V target.
- Validation: all six test groups passed, including **4,096/4,096** oracle
  cases and **10/10** target tests; cycle result `CYCLE:PASS`.
- Kernel benchmark: **321,161 H/s**, 643,000 hashes in 2,002,111 us, checksum
  `fe`; estimated **467.06 cycles/hash**.
- Sustained mining: converged to **307,877 H/s** at 700,000 hashes.
- Relative to E03-b RISC-V: **+2.35%** benchmark, **+1.24%** sustained, and
  about **10.99 cycles/hash** saved. Relative to E01: **+10.49%** benchmark
  and **+7.39%** sustained.
- Cross-architecture: RISC-V is **1.28% faster** than ARM in the E04-a kernel
  benchmark; ARM is **1.05% faster** in the short sustained samples.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E04-a as the new common baseline. Continue E04 variants at
  150 MHz, separately from the later authorized overclock sweep.
- Firmware UF2 SHA-256:
  `924b592fbb7b635f5bc6f3b46036521180d522328a583f1c57b12eaa65408336`
- Archived serial log: `logs/E04a-constant-tail-riscv.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=constant-header-tail-e04a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=643000 elapsed_us=2002111 hash_rate_hs=321161 checksum=fe temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=307877 temperature=disabled
```

## 2026-09-14 — E04-d fast zero-top-word target rejection: ARM

- Experiment: `E04d-fast-target-reject-arm-01`
- Change: when target word 7 is zero, reject any nonzero raw `SUM7` before byte
  reversal and the ordered uint256 loop; preserve the complete comparator for
  equality, candidates, and targets with a nonzero top word. Stock 150 MHz;
  source checkpoint `a1b8709`.
- Validation: both builds passed; ARM passed all six test groups, **4,096/4,096**
  oracle cases and the known genesis nonce search; cycle `CYCLE:PASS`.
- Kernel benchmark (does not run target comparison): **316,422 H/s**, 633,000
  hashes in 2,000,495 us, checksum `84`; **-0.21%** versus E04-a, treated as
  minor run/layout variation rather than the objective of this subvariant.
- Sustained mining: converged to **315,026 H/s** at 700,000 hashes, **+1.26%**
  versus E04-a ARM and **+8.84%** versus E01 ARM.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain on ARM; measure identical source on Hazard3.
- Firmware UF2 SHA-256:
  `8f90e464fbfc72872bf4ec5b9caa60e863db609a345a4a8ed1361751c0e2bfb8`
- Archived serial log: `logs/E04d-fast-target-reject-arm.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=fast-zero-msw-reject-e04d arch=ARM-M33 clock_hz=150000000 hashes=633000 elapsed_us=2000495 hash_rate_hs=316422 checksum=84 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=315026 temperature=disabled
```

## 2026-09-14 — E04-d fast zero-top-word target rejection: RISC-V

- Experiment: `E04d-fast-target-reject-riscv-02`
- Build/workload: identical checkpoint `a1b8709`, difficulty-1 target, and stock
  150 MHz clock; Hazard3 RISC-V target.
- Validation: all six test groups passed, including **4,096/4,096** oracle
  cases and the known genesis nonce search; cycle `CYCLE:PASS`.
- Kernel benchmark (target comparison excluded): **321,164 H/s**, 643,000
  hashes in 2,002,091 us, checksum `fe`; +3 H/s versus E04-a, unchanged.
- Sustained mining: converged to **311,063 H/s** at 700,000 hashes, **+1.03%**
  versus E04-a RISC-V and **+8.50%** versus E01 RISC-V.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E04-d on both architectures. The current best stock-clock
  source is checkpoint `a1b8709`.
- Firmware UF2 SHA-256:
  `0065f18fe41518252718e9dbab3ba987809cc3e6bdf1b2b994b03665419520ea`
- Archived serial log: `logs/E04d-fast-target-reject-riscv.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=fast-zero-msw-reject-e04d arch=RISCV-HAZARD3 clock_hz=150000000 hashes=643000 elapsed_us=2002091 hash_rate_hs=321164 checksum=fe temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=311063 temperature=disabled
```

## 2026-09-14 — E04-c batched 64-bit accounting: ARM

- Experiment: `E04c-batched-accounting-arm-01`
- Change: update 64-bit benchmark totals once per 1,000 hashes and mining totals
  once per 100,000-hash reporting window; candidates report the exact total as
  completed windows plus the current 32-bit window. Work and elapsed-time
  definitions are unchanged. Stock 150 MHz; checkpoint `c24b260`.
- Validation: both builds passed; ARM passed all six test groups, **4,096/4,096**
  oracle cases, target tests, and exact nonce search; cycle `CYCLE:PASS`.
- Kernel benchmark: **324,632 H/s**, 650,000 hashes in 2,002,268 us, checksum
  `f9`; estimated **462.06 cycles/hash**.
- Sustained mining: converged to **320,406 H/s** at 700,000 hashes.
- Relative to E04-d ARM: **+2.59%** benchmark and **+1.71%** sustained. Relative
  to E01 ARM: **+10.38%** benchmark and **+10.70%** sustained.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: retain on ARM; measure identical source on Hazard3.
- Firmware UF2 SHA-256:
  `3f98fcf172d7a47c71ada88f22e4713b5e17834c71a3f400a74d06cf78397615`
- Archived serial log: `logs/E04c-batched-accounting-arm.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=650000 elapsed_us=2002268 hash_rate_hs=324632 checksum=f9 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=320406 temperature=disabled
```

## 2026-09-14 — E04-c batched 64-bit accounting: RISC-V

- Experiment: `E04c-batched-accounting-riscv-02`
- Build/workload: identical checkpoint `c24b260` and stock 150 MHz clock;
  Hazard3 RISC-V target.
- Validation: all six test groups passed, including **4,096/4,096** oracle
  cases, target tests, and exact nonce search; cycle `CYCLE:PASS`.
- Kernel benchmark: **325,341 H/s**, 651,000 hashes in 2,000,977 us, checksum
  `28`; estimated **461.06 cycles/hash**.
- Sustained mining: converged to **314,333 H/s** at 600,000 hashes (the first
  100,000 window was a startup outlier).
- Relative to E04-d RISC-V: **+1.30%** benchmark and **+1.05%** sustained.
  Relative to E01 RISC-V: **+11.93%** benchmark and **+9.64%** sustained.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Decision: accept E04-c on both architectures. Checkpoint `c24b260` is the new
  stock-clock source baseline for compiler and clock experiments.
- Firmware UF2 SHA-256:
  `53fcd7ddca35f2633cc26785004fc13c4e020dee42f7559881f996ea03af1510`
- Archived serial log: `logs/E04c-batched-accounting-riscv.log`

```text
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=RISCV-HAZARD3 clock_hz=150000000 hashes=651000 elapsed_us=2000977 hash_rate_hs=325341 checksum=28 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=314333 temperature=disabled
```

## 2026-09-14 — E11 clock-sweep preflight

- Added an explicit `MINER_SYS_CLOCK_KHZ` CMake/wrapper parameter constrained
  to 12–550 MHz. The wrapper always supplies 150 MHz when the environment is
  unset, preventing a cached overclock from becoming an accidental default.
- Boot output now records the requested and actual clock and labels settings
  above 150 MHz `experimental-overclock`.
- Both 150 MHz recovery images build without warnings from the current E04-c
  source. Local recovery copies:
  - `artifacts/recovery-arm-150mhz-e04c.uf2`, SHA-256
    `ce6a56dac56a840cbeffb2f9784ee200ef645fcfd1aa9ec5cc8af611a29fe12c`
  - `artifacts/recovery-riscv-150mhz-e04c.uf2`, SHA-256
    `5f073bb3eeef535aba2e6a5e32a3f7a295801ac6e22897a2e28a71a937851396`
- Performance and temperature: no new measurement; temperature remains
  disabled. Initial sweep uses unchanged regulator settings and staged clocks.

## 2026-09-14 — E11 short overclock sweep: ARM at 200 MHz

- Experiment: `E11-arm-200mhz-01`
- Configuration: E04-c source, ARM M33, requested/actual 200,000/200,000,000
  kHz/Hz, unchanged regulator setting, `experimental-overclock` profile.
- Validation: both 200 MHz profiles built without warnings. ARM passed all six
  test groups, **4,096/4,096** oracle cases and **10/10** target tests; cycle
  `CYCLE:PASS`.
- Kernel benchmark: **431,915 H/s**, 864,000 hashes in 2,000,395 us, checksum
  `47`; estimated **463.05 cycles/hash**.
- Sustained mining: converged to **427,219 H/s** at 900,000 hashes.
- Relative to E04-c ARM at 150 MHz: **+33.05%** benchmark and **+33.34%**
  sustained for a +33.33% clock change. Cycle cost is effectively unchanged.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Qualification: short functional/performance experiment only; no thermal or
  reliability soak. Retain as a candidate experiment, not the default profile.
- Firmware UF2 SHA-256:
  `a6962d61c134af5c46604a5a92a0139b89bad92c40d6deccc8490881be394a1f`
- Archived serial log: `logs/E11-200mhz-arm.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=200000 actual_clock_hz=200000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=200000000 hashes=864000 elapsed_us=2000395 hash_rate_hs=431915 checksum=47 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=900000 total_hashes=900000 hash_rate_hs=427219 temperature=disabled
```

## 2026-09-14 — E11 short overclock sweep: RISC-V at 200 MHz

- Experiment: `E11-riscv-200mhz-02`
- Configuration: identical E04-c source, Hazard3 RISC-V, requested/actual
  200,000/200,000,000 kHz/Hz, unchanged regulator setting,
  `experimental-overclock` profile.
- Validation: all six test groups passed, **4,096/4,096** oracle cases and
  **10/10** target tests; cycle `CYCLE:PASS`.
- Kernel benchmark: **433,790 H/s**, 868,000 hashes in 2,000,969 us, checksum
  `6b`; estimated **461.05 cycles/hash**.
- Sustained mining: converged to **419,097 H/s** at 900,000 hashes.
- Relative to E04-c RISC-V at 150 MHz: **+33.33%** benchmark and **+33.33%**
  sustained for a +33.33% clock change; cycle cost is unchanged.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Qualification: short functional/performance experiment only; no thermal or
  reliability soak. Retain as a candidate experiment, not the default profile.
- Firmware UF2 SHA-256:
  `e5c4225b74ed5c2de8c18b2edb9f4084824b5eda472b6065eb4bc3885749a682`
- Archived serial log: `logs/E11-200mhz-riscv.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=200000 actual_clock_hz=200000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=RISCV-HAZARD3 clock_hz=200000000 hashes=868000 elapsed_us=2000969 hash_rate_hs=433790 checksum=6b temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=900000 total_hashes=900000 hash_rate_hs=419097 temperature=disabled
```

## 2026-09-14 — E11 short overclock sweep: ARM at 250 MHz

- Experiment: `E11-arm-250mhz-03`
- Configuration: E04-c ARM M33, requested/actual 250,000/250,000,000 kHz/Hz,
  unchanged regulator setting, `experimental-overclock` profile.
- Validation: both 250 MHz profiles built without warnings. ARM passed all six
  test groups, **4,096/4,096** oracle cases and **10/10** target tests; cycle
  `CYCLE:PASS`.
- Kernel benchmark: **539,897 H/s**, 1,080,000 hashes in 2,000,383 us,
  checksum `2c`; estimated **463.05 cycles/hash**.
- Sustained mining: converged to **534,023 H/s** at 1,100,000 hashes.
- Relative to E04-c ARM at 150 MHz: **+66.31%** benchmark and **+66.67%**
  sustained for a +66.67% clock change; cycle cost remains unchanged.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Qualification: short functional/performance experiment only; no thermal or
  reliability soak. Not a safe/default setting.
- Firmware UF2 SHA-256:
  `bb41e254289fe2ddafa96fee2bd4aaea50dbfb3f9ed4c68105019db190b6a95d`
- Archived serial log: `logs/E11-250mhz-arm.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=250000 actual_clock_hz=250000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=250000000 hashes=1080000 elapsed_us=2000383 hash_rate_hs=539897 checksum=2c temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=1100000 total_hashes=1100000 hash_rate_hs=534023 temperature=disabled
```

## 2026-09-14 — E11 short overclock sweep: RISC-V at 250 MHz

- Experiment: `E11-riscv-250mhz-04`
- Configuration: E04-c Hazard3 RISC-V, requested/actual
  250,000/250,000,000 kHz/Hz, unchanged regulator setting,
  `experimental-overclock` profile.
- Validation: all six test groups passed, **4,096/4,096** oracle cases and
  **10/10** target tests; cycle `CYCLE:PASS`.
- Kernel benchmark: **542,239 H/s**, 1,085,000 hashes in 2,000,961 us,
  checksum `02`; estimated **461.05 cycles/hash**.
- Sustained mining: converged around **523,851 H/s** through 1,100,000 hashes.
- Relative to E04-c RISC-V at 150 MHz: **+66.67%** benchmark and **+66.66%**
  sustained for a +66.67% clock change; cycle cost remains unchanged.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Qualification: short functional/performance experiment only; no thermal or
  reliability soak. Not a safe/default setting.
- Firmware UF2 SHA-256:
  `b0021cb100d778d567b8411cbecf69179002ec3b4f93576f2625d1a59db32780`
- Archived serial log: `logs/E11-250mhz-riscv.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=250000 actual_clock_hz=250000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=RISCV-HAZARD3 clock_hz=250000000 hashes=1085000 elapsed_us=2000961 hash_rate_hs=542239 checksum=02 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=1100000 total_hashes=1100000 hash_rate_hs=523851 temperature=disabled
```

## 2026-09-14 — E11 short overclock sweep: ARM at 300 MHz

- Experiment: `E11-arm-300mhz-05`
- Configuration: E04-c ARM M33, requested/actual 300,000/300,000,000 kHz/Hz,
  unchanged regulator setting, `experimental-overclock` profile.
- Validation: both 300 MHz profiles built without warnings. ARM passed all six
  test groups, **4,096/4,096** oracle cases and **10/10** target tests; cycle
  `CYCLE:PASS`.
- Kernel benchmark: **647,882 H/s**, 1,296,000 hashes in 2,000,363 us,
  checksum `11`; estimated **463.05 cycles/hash**.
- Sustained mining: converged to **640,820 H/s** through 1,300,000 hashes.
- Relative to E04-c ARM at 150 MHz: **+99.57%** benchmark and **+100.00%**
  sustained for a +100% clock change; cycle cost remains unchanged.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Qualification: highest clock-only short ARM result so far; no thermal or
  reliability soak. Not a safe/default setting.
- Firmware UF2 SHA-256:
  `ec7a1712828773c6f503d72ed580188e43674813c6813e1122acc8e203f25df1`
- Archived serial log: `logs/E11-300mhz-arm.log`

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=300000 actual_clock_hz=300000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=300000000 hashes=1296000 elapsed_us=2000363 hash_rate_hs=647882 checksum=11 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=1300000 total_hashes=1300000 hash_rate_hs=640820 temperature=disabled
```

## 2026-09-14 — E11 failed clock-only attempt: RISC-V at 300 MHz

- Experiment: `E11-riscv-300mhz-06-fail`
- Configuration: E04-c Hazard3 RISC-V, requested 300,000 kHz, unchanged
  regulator setting, `experimental-overclock` profile.
- Build/flash: ARM and RISC-V 300 MHz profiles built without warnings. The
  RISC-V UF2 flashed and verified successfully, then the device rebooted.
- Failure: USB CDC did not enumerate within the bounded monitor deadline.
  Therefore no BOOT line, actual clock, KAT/oracle result, benchmark, or
  sustained rate exists. `./tools/doctor` subsequently confirmed no
  `/dev/ttyACM*` device.
- Temperature: disabled; no measurement.
- Decision: reject 300 MHz RISC-V at unchanged regulator voltage. Do not retry
  or test a higher Hazard3 clock under this voltage. Recover via BOOTSEL and a
  saved 150 MHz UF2.
- Firmware UF2 SHA-256:
  `878a42f82048d040518591edb0eee715c7b5f61b95195833ec0f960f15983ebd`
- `logs/riscv-latest.log` is zero bytes because serial never appeared; the
  complete failure evidence is the cycle/doctor command output summarized here.

```text
FLASH:PASS arch=riscv
ERROR: Raspberry Pi USB serial device did not appear
FAIL USB serial           no /dev/ttyACM* device
DOCTOR:FAIL count=1
```

## 2026-09-14 — Recovery after rejected RISC-V 300 MHz attempt

- Experiment: `E11-recovery-arm-150mhz-07`
- Recovery: manually entered BOOTSEL, then flashed and verified the current
  E04-c ARM/M33 image rebuilt at the default 150 MHz clock. Both ARM and
  RISC-V 150 MHz builds passed without warnings before the flash.
- Runtime: USB CDC enumerated normally and the miner produced uninterrupted
  progress through 4,100,000 hashes during the captured window.
- Sustained mining: **320,415 H/s** at 4,000,000 hashes.
- Comparison: previous E04-c ARM result was 320,406 H/s; the +0.003% difference
  is measurement noise and confirms successful recovery to the retained
  baseline.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `ce6a56dac56a840cbeffb2f9784ee200ef645fcfd1aa9ec5cc8af611a29fe12c`
- Archived serial log: `logs/E11-recovery-arm-150mhz.log`
- Decision: recovery passed. Retain 150 MHz as the default and resume
  code/compiler optimization from this known-good image.

```text
MINING:PROGRESS arch=ARM-M33 nonce=4000000 total_hashes=4000000 hash_rate_hs=320415 temperature=disabled
```

## 2026-09-14 — E05 rejected whole-image LTO build

- Experiment: `E05-lto-whole-image-01-fail`
- Hypothesis/change: enable CMake interprocedural optimization on the complete
  `pico2_agent` target at 150 MHz, which applies GCC LTO to the application and
  Pico SDK objects assembled into that target.
- ARM result: **build failed** at link time. GCC LTO coalesced code in a way
  incompatible with the SDK's wrapped `printf`/`puts` symbols and produced
  unsupported/dangerous relocations involving time-critical sections.
- RISC-V result: **build failed** during compilation. The installed compiler
  rejected CMake's `-fno-fat-lto-objects` because its linker-plugin support is
  unavailable in this toolchain configuration.
- Validation/benchmark: not run; no firmware existed and nothing was flashed.
  The recovered 150 MHz ARM baseline remained on the board.
- Temperature: intentionally disabled; no measurement.
- Decision: reject whole-image CMake IPO for both installed toolchains. If LTO
  is explored further, constrain it to miner-owned source and preserve normal
  SDK object generation/linkage; do not enable target-wide IPO.

```text
ARM: undefined reference to `__wrap_printf`; dangerous relocation: unsupported relocation
RISC-V: cc1: error: '-fno-fat-lto-objects' are supported only with linker plugin
```

## 2026-09-14 — E05 source-only LTO: ARM at 150 MHz

- Experiment: `E05-source-lto-arm-02`
- Change: compile only repository-owned `src/main.c` with `-flto` and pass
  `-flto` at the final link; all Pico SDK translation units remain normally
  compiled. Requested/actual system clock: 150,000/150,000,000 kHz/Hz.
- Validation: both architecture builds passed without warnings. ARM passed all
  six test groups, **4,096/4,096** independent oracle cases and **10/10** target
  boundary tests; cycle `CYCLE:PASS`.
- Kernel benchmark: **324,635 H/s**, 650,000 hashes in 2,002,247 us, checksum
  `f9`; estimated **462.06 cycles/hash**.
- Sustained mining: **320,415 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline (324,632 benchmark; 320,406 sustained):
  **+0.001% benchmark, +0.003% sustained**. This is measurement noise rather
  than an optimization win.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `2f671fc2dd7fabe15e8a31f3b4ba310ef2e427b3b663a788af233067a9e7c0db`
- Archived serial log: `logs/E05-source-lto-arm.log`
- Decision: do not select source-only LTO for ARM on performance grounds; wait
  for the matched RISC-V result before removing or retaining the option.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled lto=on clock_profile=stock requested_clock_khz=150000 actual_clock_hz=150000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=650000 elapsed_us=2002247 hash_rate_hs=324635 checksum=f9 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=320415 temperature=disabled
```

## 2026-09-14 — SDK-pattern sticky-error test guard fails on RISC-V

- Experiment: `sticky-error-stimulus-fix3-riscv-guard-fail`
- Candidate commit: `798baaa`; ARM validation recorded above passed.
- Build/flash: both builds passed without warnings; RISC-V flash and verify
  passed.
- Runtime failure: `observed_not_ready=0 latched=0 survived_start=0
  cleared=1`; therefore **no valid benchmark**.
- Diagnosis: Hazard3 is slow enough relative to the SHA peripheral that the
  first CSR poll after the sixteenth store occurred after compression had
  completed. Because fix attempt 3 incorrectly guarded the SDK-style burst on
  observing the brief busy window, it skipped the error stimulus entirely.
  The SDK test's unpaced burst is unconditional: even if initially ready, its
  first block starts compression and subsequent stores provoke the error.
- Temperature: intentionally **disabled**, not measured and not inferred.
- RISC-V UF2 SHA-256:
  `1f8ca40d57148de205a068d7a3eeacaf8011d5a3932eec46193794b3e497beac`
- Archived serial log:
  `logs/sticky-error-sdk-pattern-riscv-guard-fail.log`
- Decision: retain the passing ARM evidence but remove the readiness-observation
  precondition and use the SDK's unconditional burst on both architectures.

```text
TEST:FAIL kat=sha_error_sticky cases=4 observed_not_ready=0 latched=0 survived_start=0 cleared=1
```

## 2026-09-14 — E05 branch-cost confirmation with deterministic RISC-V test

- Experiment: `E05-riscv-branch-cost1-13-confirm-pass`
- Candidate source commits: `afd38f5` (`-mbranch-cost=1`) and `c195377`
  (unconditional SDK-pattern sticky-error stimulus).
- Validation: both builds passed without warnings. RISC-V passed the
  three-part sticky-error proof, all seven test groups, **4,096/4,096** oracle
  cases and **10/10** target cases; cycle passed.
- Kernel benchmark: **328,909 H/s**, 658,000 hashes in 2,000,555 us, checksum
  `bb`; repeat-identical to E05 run 1 and **-0.44%** versus retained E04-e.
- Sustained mining: **323,115 H/s** at 800,000 hashes; within 10 H/s of E05
  run 1 and **+0.64%** versus retained E04-e.
- Interpretation: the mixed branch-cost result is reproducible, but the
  changed out-of-band self-test also changed image layout. Measure a no-flag
  RISC-V artifact with the same deterministic test before deciding.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `8dbb750625b1e364f9197213f5ef0e94925a420aaa601b29b59a79d7093316d6`
- Archived serial log: `logs/E05-riscv-branch-cost1-confirm-pass.log`
- Decision: functional fix passes RISC-V; E05 compiler flag remains pending a
  layout-matched no-flag A/B.

```text
TEST:PASS kat=sha_error_sticky cases=3 burst_words=2500 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=658000 elapsed_us=2000555 hash_rate_hs=328909 checksum=bb temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=800000 total_hashes=800000 hash_rate_hs=323115 temperature=disabled
```

## 2026-09-14 — Unconditional sticky-error proof paired ARM pass

- Experiment: `sticky-error-stimulus-fix4-arm-pass`
- Candidate commit: `c195377`.
- Validation: the unconditional 2,500-word SDK-pattern stimulus passed on ARM,
  completing paired runtime validation of the shared self-test. All seven test
  groups, **4,096/4,096** oracle cases, and **10/10** target cases passed;
  cycle passed.
- ARM kernel benchmark: **323,934 H/s**, 648,000 hashes in 2,000,405 us,
  checksum `2a`; exact retained kernel rate.
- ARM sustained mining: **320,415 H/s** at 800,000 hashes; exact retained rate.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `6e8718d3c606287143a8e4bb4a5eadab16b4cbe5a4bbdbe8ddf1676618a21f35`
- Archived serial log: `logs/sticky-error-unconditional-arm-pass.log`
- Decision: retain the deterministic startup proof. It now passes both ISAs
  and does not measurably perturb the timed ARM path.

```text
TEST:PASS kat=sha_error_sticky cases=3 burst_words=2500 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=648000 elapsed_us=2000405 hash_rate_hs=323934 checksum=2a temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=800000 total_hashes=800000 hash_rate_hs=320415 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 branch-cost layout-matched control

- Experiment: `E05-riscv-branch-cost1-13-control-reject`
- Control commit: `ead476b`; deterministic sticky test retained, only
  `-mbranch-cost=1` removed.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- No-flag kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,867 us,
  checksum `6f`; exactly reproduces the retained E04-e rate.
- No-flag sustained mining: **321,033 H/s** at 700,000 hashes, within 18 H/s
  (**-0.006%**) of the retained 321,051 H/s result.
- Matched E05 effect: `-mbranch-cost=1` reproducibly changes kernel throughput
  to 328,909 H/s (**-0.44%**) and sustained throughput to about 323,120 H/s
  (**+0.65%**). This is below the plan's 2% retention threshold and sacrifices
  the primary bounded kernel benchmark for a small mining-loop gain.
- Temperature: intentionally **disabled**, not measured and not inferred.
- No-flag RISC-V UF2 SHA-256:
  `c9ef278d9e16b25243499a6a349820a30ef74c1d6c0be274b8623bee53d1927a`
- Archived serial log: `logs/E05-riscv-branch-cost1-no-flag-control.log`
- Decision: reject `-mbranch-cost=1`; retain the no-flag build and the new
  deterministic sticky-error proof.

```text
TEST:PASS kat=sha_error_sticky cases=3 burst_words=2500 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000867 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=321033 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 no-code-hoisting trial

- Experiment: `E05-riscv-no-code-hoisting-14-reject`
- Candidate commit: `eb7e3f7`.
- Change: compile only RISC-V `src/main.c` with `-fno-code-hoisting` at the
  existing `-O3`; ARM remains unchanged.
- Artifact result: RISC-V UF2 is byte-for-byte identical to the no-flag
  control, proving this option changes no emitted firmware in the current
  translation unit.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,867 us, checksum
  `6f`; exact no-flag result.
- Sustained mining: **321,043 H/s** at 700,000 hashes, a noise-level 10 H/s
  above the immediately preceding identical-artifact control.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `c9ef278d9e16b25243499a6a349820a30ef74c1d6c0be274b8623bee53d1927a`
- Archived serial log: `logs/E05-riscv-no-code-hoisting.log`
- Decision: reject as no effect and remove the redundant flag.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000867 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=321043 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 no-if-conversion2 trial

- Experiment: `E05-riscv-no-if-conversion2-15-reject`
- Candidate commit: `885306e`.
- Change: compile only RISC-V `src/main.c` with `-fno-if-conversion2` at the
  existing `-O3`; ARM remains unchanged.
- Artifact result: RISC-V UF2 is byte-for-byte identical to the no-flag
  control, so the option changes no emitted firmware for this source.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,866 us, checksum
  `6f`; exact baseline rate.
- Sustained mining: **321,040 H/s** at 800,000 hashes, within normal variation
  of the identical artifact.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `c9ef278d9e16b25243499a6a349820a30ef74c1d6c0be274b8623bee53d1927a`
- Archived serial log: `logs/E05-riscv-no-if-conversion2.log`
- Decision: reject as no effect and remove the redundant flag.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000866 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=800000 total_hashes=800000 hash_rate_hs=321040 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 global loop-unrolling trial

- Experiment: `E05-riscv-unroll-loops-16-reject`
- Candidate commit: `b543b24`.
- Change: compile only RISC-V `src/main.c` with `-funroll-loops` at `-O3`;
  ARM remains byte-identical to baseline.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **326,048 H/s**, 653,000 hashes in 2,002,771 us, checksum
  `e8`; **-1.30%** versus the 330,357 H/s no-flag baseline.
- Sustained mining: **324,527 H/s** at 700,000 hashes; **+1.08%** versus the
  retained 321,051 H/s result.
- Code/artifact impact: RISC-V UF2 grew from 367,616 to 371,200 bytes
  (**+3,584 bytes, +0.98%**). Global
  unrolling creates a reproducible trade toward the mining-loop layout but
  slows the primary bounded kernel and increases flash footprint.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `e414d682f1cbbc9cb2fde01b6a640dfc42918241037e3eb88fd030b9fa8e0aaf`
- Archived serial log: `logs/E05-riscv-unroll-loops.log`
- Decision: reject. Neither direction exceeds the 2% retention threshold,
  kernel throughput regresses, and the global flag adds code-size complexity.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=653000 elapsed_us=2002771 hash_rate_hs=326048 checksum=e8 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=324527 temperature=disabled
```

## 2026-09-14 — E05 targeted Hazard3 mining-loop unrolling, run 1

- Experiment: `E05-riscv-targeted-mining-unroll-17-run1`
- Candidate commit: `1225980`.
- Change: apply GCC `optimize("unroll-loops")` only to RISC-V
  `mine_forever()`. The separate benchmark function and all ARM code retain
  their normal options.
- Isolation: ARM UF2 remains byte-identical. RISC-V UF2 grows only from
  367,616 to 368,128 bytes (**+512 bytes, +0.14%**), versus +3,584 bytes for
  the rejected global flag.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **330,356 H/s**, 661,000 hashes in 2,000,871 us, checksum
  `6f`; effectively unchanged from 330,357 H/s.
- Sustained mining: **326,646 H/s** at 700,000 hashes; **+1.74%** versus the
  retained 321,051 H/s baseline.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `e056f2ec011d16359cae2259d40f55477cfe9e4c310f908a316ea1e21613d832`
- Archived serial log: `logs/E05-riscv-targeted-mining-unroll-run1.log`
- Decision: promising but below the nominal 2% gate; repeat the identical
  artifact. The complexity cost is one ISA-specific function attribute.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000871 hash_rate_hs=330356 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=326646 temperature=disabled
```

## 2026-09-14 — E05 targeted Hazard3 mining-loop unrolling confirmed

- Experiment: `E05-riscv-targeted-mining-unroll-17-confirm-retain`
- Candidate commit: `1225980`; identical artifact to run 1.
- Validation: both builds passed without warnings. RISC-V again passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,863 us, checksum
  `6f`; exact retained baseline rate.
- Sustained mining: **326,649 H/s** at 700,000 hashes; **+1.74%** versus the
  321,051 H/s baseline and within 3 H/s of run 1.
- Artifact cost: RISC-V UF2 is 368,128 bytes, **+512 bytes (+0.14%)**; ARM UF2
  remains byte-identical to its retained image.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `e056f2ec011d16359cae2259d40f55477cfe9e4c310f908a316ea1e21613d832`
- Archived serial log:
  `logs/E05-riscv-targeted-mining-unroll-confirm.log`
- Decision: retain. Although the gain is just below the nominal 2% screening
  threshold, two identical-artifact runs agree within 3 H/s, bounded kernel
  performance is preserved, and maintenance cost is one ISA-specific function
  attribute. The board is left running this stock-clock RISC-V image.

```text
TEST:PASS kat=sha_error_sticky cases=3 burst_words=2500 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000863 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=326649 temperature=disabled
```

## 2026-09-14 — E04-d specialized zero-MSW target, RISC-V

- Experiment: `E04d-zero-msw-specialized-18-riscv`
- Candidate commit: `51e26c4`, layered on retained targeted RISC-V mining-loop
  unrolling.
- Change: require the fixed mining target's most-significant 32-bit word to be
  zero once at setup, then remove its per-nonce load/test. The hot common path
  is one raw `SUM7` load and one rejection branch; the rare zero case compares
  words 6 through 0. The genesis nonce KAT uses this specialized path.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,864 us, checksum
  `6f`; unchanged, as expected because the kernel benchmark excludes target
  comparison.
- Sustained mining: **326,638 H/s** at 900,000 hashes; **-0.003%** versus the
  retained 326,649 H/s targeted-unroll result, effectively zero effect.
- Interpretation: disassembly confirms two common-path instructions were
  removed, but their cost is hidden by peripheral/loop scheduling on Hazard3.
- Temperature: intentionally **disabled**, not measured and not inferred.
- RISC-V UF2 SHA-256:
  `8fd49744c0db49a15a074300864bb2c294aed52e841685075079aab8fc730f40`
- Archived serial log: `logs/E04d-zero-msw-specialized-riscv.log`
- Decision: no RISC-V gain; test ARM before deciding whether an ISA-specific
  specialization is justified.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000864 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=900000 total_hashes=900000 hash_rate_hs=326639 temperature=disabled
```

## 2026-09-14 — E04-d specialized zero-MSW target, ARM

- Experiment: `E04d-zero-msw-specialized-18-arm-reject`
- Candidate commit: `51e26c4`; paired RISC-V result recorded above.
- Validation: both builds passed without warnings. ARM passed the sticky-error
  proof, all seven test groups, **4,096/4,096** oracle cases and **10/10**
  target cases; cycle passed.
- Kernel benchmark: **324,634 H/s**, 650,000 hashes in 2,002,257 us, checksum
  `f9`; **+0.22%** versus the retained 323,934 H/s ARM result. The benchmark
  contains no target comparison, so this is incidental code-layout movement.
- Sustained mining: **321,097 H/s** at 700,000 hashes; **+0.21%** versus the
  retained 320,415 H/s result.
- Paired interpretation: RISC-V changed by effectively 0%; ARM's sub-quarter
  percent movement is below the retention threshold and inseparable from
  fragile image layout. The target-shape restriction is not justified.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `cf656c9d7d78206d1350c96d9ddebcca1b8207ecf149b5eb4efcaec5a88b6228`
- Archived serial log: `logs/E04d-zero-msw-specialized-arm.log`
- Decision: reject on both ISAs and restore the retained generic comparator.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=650000 elapsed_us=2002257 hash_rate_hs=324634 checksum=f9 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=321097 temperature=disabled
```

## 2026-09-14 — E04 post-START ready-poll removal, ARM run 1

- Experiment: `E04-start-ready-poll-removal-19-arm-run1`
- Candidate commit: `60aa7ec`.
- Change: after each explicit SHA `START`, feed the first block/second-hash
  block directly instead of first reading and branching on `WDATA_RDY`.
  Ordered MMIO preserves START-before-WDATA. The required inter-block ready
  wait, both digest-valid waits, sticky error detection, and full digest
  validation remain.
- Validation: both builds passed without warnings. ARM passed the sticky-error
  proof, all seven test groups, **4,096/4,096** oracle cases and **10/10**
  target cases; cycle passed with no SHA fault.
- Kernel benchmark: **331,818 H/s**, 664,000 hashes in 2,001,100 us, checksum
  `37`; **+2.43%** versus the retained 323,934 H/s ARM result.
- Sustained mining: **321,789 H/s** at 500,000 hashes; **+0.43%** versus the
  retained 320,415 H/s result.
- Artifact size: ARM UF2 remains 344,576 bytes.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `69aa6d486464bdc411aa5c661021b8483b67ddd557d4cbc657e95020c5f593e2`
- Archived serial log: `logs/E04-start-ready-poll-removal-arm-run1.log`
- Decision: promising; exceeds the 2% kernel gate. Validate RISC-V and repeat
  before retaining a state-machine assumption.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=664000 elapsed_us=2001100 hash_rate_hs=331818 checksum=37 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=500000 total_hashes=500000 hash_rate_hs=321789 temperature=disabled
```

## 2026-09-14 — E04 post-START ready-poll removal, RISC-V run 1

- Experiment: `E04-start-ready-poll-removal-19-riscv-run1`
- Candidate commit: `60aa7ec`; same source change as the ARM result above.
- Validation: both builds passed without warnings. RISC-V passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed with no SHA fault.
- Kernel benchmark: **339,326 H/s**, 679,000 hashes in 2,001,026 us, checksum
  `32`; **+2.72%** versus the retained 330,357 H/s result.
- Sustained mining: **333,152 H/s** at 600,000 hashes; **+1.99%** versus the
  retained targeted-unroll result of 326,649 H/s and **+3.77%** versus the
  earlier 321,051 H/s E04-e result.
- Artifact size: RISC-V UF2 remains 368,128 bytes.
- Temperature: intentionally **disabled**, not measured and not inferred.
- RISC-V UF2 SHA-256:
  `046d49334a9648833b3b58668124e09e52c8acbc82bfd46f724fc93ecbfa7f71`
- Archived serial log: `logs/E04-start-ready-poll-removal-riscv-run1.log`
- Decision: promising on both architectures; repeat identical ARM and RISC-V
  artifacts before retaining the state-machine optimization.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=679000 elapsed_us=2001026 hash_rate_hs=339326 checksum=32 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=333152 temperature=disabled
```

## 2026-09-14 — E04 post-START ready-poll removal, RISC-V confirmation

- Experiment: `E04-start-ready-poll-removal-19-riscv-confirm`
- Artifact: identical commit `60aa7ec` and UF2 to RISC-V run 1.
- Validation: both builds passed without warnings. All seven RISC-V test
  groups, the sticky-error proof, **4,096/4,096** oracle cases and **10/10**
  target cases passed again; cycle passed with no SHA fault.
- Kernel benchmark: **339,327 H/s**, 679,000 hashes in 2,001,017 us, checksum
  `32`; within 1 H/s of run 1.
- Sustained mining: **333,171 H/s** at 500,000 hashes; within 19 H/s of run 1.
- Temperature: intentionally **disabled**, not measured and not inferred.
- RISC-V UF2 SHA-256:
  `046d49334a9648833b3b58668124e09e52c8acbc82bfd46f724fc93ecbfa7f71`
- Archived serial log:
  `logs/E04-start-ready-poll-removal-riscv-confirm.log`
- Decision: RISC-V repeatability confirmed; finish paired confirmation on ARM.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=679000 elapsed_us=2001017 hash_rate_hs=339327 checksum=32 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=500000 total_hashes=500000 hash_rate_hs=333171 temperature=disabled
```

## 2026-09-14 — E04 post-START ready-poll removal, ARM confirmation

- Experiment: `E04-start-ready-poll-removal-19-arm-confirm-retain`
- Artifact: identical commit `60aa7ec` and UF2 to ARM run 1.
- Validation: both builds passed without warnings. All seven ARM test groups,
  the sticky-error proof, **4,096/4,096** oracle cases and **10/10** target
  cases passed again; cycle passed with no SHA fault.
- Kernel benchmark: **331,819 H/s**, 664,000 hashes in 2,001,092 us, checksum
  `37`; within 1 H/s of run 1 and **+2.43%** over the retained predecessor.
- Sustained mining: **321,791 H/s** at 500,000 hashes; within 2 H/s of run 1
  and **+0.43%** over the retained predecessor.
- Paired retained result: RISC-V repeated at 339,326–339,327 H/s kernel and
  333,152–333,171 H/s sustained; ARM repeated at 331,818–331,819 H/s kernel
  and 321,789–321,791 H/s sustained.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `69aa6d486464bdc411aa5c661021b8483b67ddd557d4cbc657e95020c5f593e2`
- RISC-V UF2 SHA-256:
  `046d49334a9648833b3b58668124e09e52c8acbc82bfd46f724fc93ecbfa7f71`
- Archived serial log: `logs/E04-start-ready-poll-removal-arm-confirm.log`
- Decision: retain on both architectures. Explicit START plus ordered MMIO
  establishes the first-write state; inter-block/valid waits and sticky error
  detection remain. Both architectures reproduce the gain without errors.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=664000 elapsed_us=2001092 hash_rate_hs=331819 checksum=37 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=500000 total_hashes=500000 hash_rate_hs=321791 temperature=disabled
```

## 2026-09-14 — E05 targeted ARM mining-loop unrolling, run 1

- Experiment: `E05-arm-targeted-mining-unroll-20-run1`
- Candidate commit: `e62a89e`, layered on retained post-START poll removal.
- Change: apply the same function-scoped GCC `optimize("unroll-loops")`
  attribute already retained for RISC-V to ARM `mine_forever()`; the separate
  benchmark function is unchanged. RISC-V remains byte-identical.
- Validation: both builds passed without warnings. ARM passed the sticky-error
  proof, all seven test groups, **4,096/4,096** oracle cases and **10/10**
  target cases; cycle passed.
- Kernel benchmark: **331,819 H/s**, 664,000 hashes in 2,001,089 us, checksum
  `37`; unchanged from the retained post-START result.
- Sustained mining: **325,209 H/s** at 500,000 hashes; **+1.06%** versus the
  retained 321,791 H/s post-START result and **+1.50%** versus the older
  320,415 H/s ARM result.
- Artifact size: ARM UF2 remains 344,576 bytes.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `9898f3013920729e84b4533d87e5edca3605ba02aaa5ec53a833555014d3f9b4`
- Archived serial log: `logs/E05-arm-targeted-mining-unroll-run1.log`
- Decision: promising but below the nominal 2% screen; repeat the identical
  artifact before deciding. Complexity is one shared function attribute.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=664000 elapsed_us=2001089 hash_rate_hs=331819 checksum=37 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=500000 total_hashes=500000 hash_rate_hs=325209 temperature=disabled
```

## 2026-09-14 — E05 targeted ARM mining-loop unrolling confirmed

- Experiment: `E05-arm-targeted-mining-unroll-20-confirm-retain`
- Artifact: identical commit `e62a89e` and UF2 to run 1.
- Validation: both builds passed without warnings. ARM again passed the
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- Kernel benchmark: **331,820 H/s**, 664,000 hashes in 2,001,086 us, checksum
  `37`; unchanged from the retained post-START result.
- Sustained mining: **325,207 H/s** at 600,000 hashes; within 2 H/s of run 1,
  **+1.06%** over the post-START predecessor, and **+1.50%** over the older
  320,415 H/s ARM result.
- Cross-architecture isolation: RISC-V stays byte-identical to its retained
  targeted-unroll/post-START image and keeps its confirmed 339,326–339,327 H/s
  kernel and 333,152–333,171 H/s sustained range.
- Artifact size: ARM UF2 remains 344,576 bytes.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `9898f3013920729e84b4533d87e5edca3605ba02aaa5ec53a833555014d3f9b4`
- RISC-V UF2 SHA-256:
  `046d49334a9648833b3b58668124e09e52c8acbc82bfd46f724fc93ecbfa7f71`
- Archived serial log: `logs/E05-arm-targeted-mining-unroll-confirm.log`
- Decision: retain on ARM as well. The repeated sustained gain is stable, the
  benchmark does not regress, the image does not grow, and both ISAs now use
  the same narrowly scoped mining-loop attribute.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=664000 elapsed_us=2001086 hash_rate_hs=331820 checksum=37 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=600000 total_hashes=600000 hash_rate_hs=325207 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 branch-cost hint, run 1

- Experiment: `E05-riscv-branch-cost1-13-run1`
- Candidate commit: `afd38f5`.
- Change: compile only `src/main.c` for `rp2350-riscv` with GCC
  `-mbranch-cost=1`; ARM has no new flag and its UF2 remains byte-identical to
  the retained build.
- Validation: both builds passed without warnings. RISC-V passed all seven test
  groups, **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- Kernel benchmark: **328,909 H/s**, 658,000 hashes in 2,000,554 us, checksum
  `bb`; **-0.44%** versus the retained 330,357 H/s E04-e RISC-V result.
- Sustained mining: **323,125 H/s** at 600,000 hashes; **+0.65%** versus the
  retained 321,051 H/s result.
- Interpretation: mixed first result. The generic branch-cost hint reduced the
  bounded kernel score but improved the full mining loop. Repeat the identical
  artifact before deciding whether this is a stable layout/scheduling tradeoff.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `afe73eaa768e34a68d5afe3e6de7c9494dc213dd74353c530188713d1d200c8b`
- Archived serial log: `logs/E05-riscv-branch-cost1-run1.log`
- Decision: pending one identical-artifact confirmation run.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=658000 elapsed_us=2000554 hash_rate_hs=328909 checksum=bb temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=323125 temperature=disabled
```

## 2026-09-14 — E05 Hazard3 branch-cost hint, run 2 failed validation

- Experiment: `E05-riscv-branch-cost1-13-run2-test-fail`
- Artifact: identical candidate commit `afd38f5`, UF2 SHA-256
  `afe73eaa768e34a68d5afe3e6de7c9494dc213dd74353c530188713d1d200c8b`.
- Build/flash: both builds passed without warnings; RISC-V flash and verify
  passed.
- Runtime failure: the deliberate sticky-error test reported
  `latched=0 survived_start=0 cleared=1`. The run stopped before the oracle,
  target, Bitcoin, benchmark, and mining stages, so it provides **no valid
  performance measurement**.
- Diagnosis: the test wrote its deliberately invalid seventeenth word
  immediately after the sixteenth block word. The peripheral's `WDATA_RDY`
  transition is not guaranteed to have become observable by that next store,
  so the error stimulus is timing-dependent. The production path did not
  report a hardware fault; this is a test-stimulus defect exposed by the
  confirmation run.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Archived serial log: `logs/E05-riscv-branch-cost1-run2-test-fail.log`
- Decision: failed gate. Make the self-test wait until `WDATA_RDY` is observed
  low before issuing the deliberate invalid write, validate both ISAs, and
  only then resume E05 measurement.

```text
TEST:FAIL kat=sha_error_sticky cases=3 latched=0 survived_start=0 cleared=1
```

## 2026-09-14 — Sticky-error self-test fix attempt 1 failed on ARM

- Experiment: `sticky-error-stimulus-fix1-arm-fail`
- Candidate commit: `80c49bd` (with E05 RISC-V flag still isolated to RISC-V).
- Change: after completing a zero block, poll up to 1,024 times for
  `WDATA_RDY=0`, then issue the deliberate invalid WDATA store.
- Build/flash: both builds passed without warnings; ARM flash and verify
  passed.
- Runtime failure: ARM observed the busy state but the immediately sampled
  sticky flag was still clear:
  `observed_not_ready=1 latched=0 survived_start=0 cleared=1`.
- Result: **no valid benchmark**; the runtime gate stopped execution.
- Refined diagnosis: `sha256_put_word()` is a raw volatile WDATA store, not a
  waiting wrapper. The remaining timing dependence is therefore between the
  invalid peripheral write and immediate CSR sampling. Poll the sticky flag
  for a bounded interval after the invalid store before declaring the
  stimulus unsuccessful.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `e4deca0af4c12c1eb5567adab3330773565b5c0a2c19b7474199b5fd78fc4d32`
- Archived serial log: `logs/sticky-error-deterministic-arm-fail.log`
- Decision: reject fix attempt 1; retain its diagnostic evidence and refine
  the out-of-timed-path test only.

```text
TEST:FAIL kat=sha_error_sticky cases=4 observed_not_ready=1 latched=0 survived_start=0 cleared=1
```

## 2026-09-14 — SDK-pattern sticky-error test passes on ARM

- Experiment: `sticky-error-stimulus-fix3-arm-pass`
- Candidate commit: `798baaa`.
- Change: reproduce the installed Pico SDK 2.3.1 non-DMA SHA error test
  pattern with an unpaced 2,500-word WDATA burst after observing busy, then
  wait for ready and verify latch, START persistence, and explicit clearing.
- Validation: both builds passed without warnings. ARM passed the four-part
  sticky-error proof, all seven test groups, **4,096/4,096** oracle cases and
  **10/10** target cases; cycle passed.
- ARM kernel benchmark: **323,935 H/s**, 648,000 hashes in 2,000,404 us,
  checksum `2a`; effectively identical to the retained 323,934 H/s result.
- ARM sustained mining: **320,415 H/s** at 700,000 hashes, exactly matching
  the retained result.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `ea3c6a0d14a460796cf2d2adc702f55f846c89ced720e52ce3af4a6ea9fbe854`
- Archived serial log: `logs/sticky-error-sdk-pattern-arm-pass.log`
- Decision: ARM validation passes without measurable hot-path regression;
  validate the same test on RISC-V before resuming E05.

```text
TEST:PASS kat=sha_error_sticky cases=4 observed_not_ready=1 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=648000 elapsed_us=2000404 hash_rate_hs=323935 checksum=2a temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=320415 temperature=disabled
```

## 2026-09-14 — Sticky-error self-test fix attempt 2 failed on ARM

- Experiment: `sticky-error-stimulus-fix2-arm-fail`
- Candidate commit: `a6092cd`.
- Change: after observing `WDATA_RDY=0` and issuing the deliberate invalid
  store, poll `ERR_WDATA_NOT_RDY` up to 1,024 times; wait for ready before the
  subsequent START persistence check.
- Build/flash: both builds passed without warnings; ARM flash and verify
  passed.
- Runtime result: identical failure,
  `observed_not_ready=1 latched=0 survived_start=0 cleared=1`; therefore **no
  valid benchmark**.
- Diagnosis: a single CPU store is not a reliable error stimulus even when
  software observes the ready bit low. The installed Pico SDK 2.3.1 test uses
  a long unpaced burst (2,500 word stores from a 10,000-byte buffer) and only
  checks the flag after waiting for ready. Reproduce that documented SDK test
  pattern with a bounded local burst rather than relying on a single store.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `e955fe54cf8d2472ded57edbe1d29dbebceb1a277a65d5bd7947564234a9a231`
- Archived serial log: `logs/sticky-error-latch-poll-arm-fail.log`
- Decision: reject fix attempt 2 and switch to the SDK's tested error stimulus.

```text
TEST:FAIL kat=sha_error_sticky cases=4 observed_not_ready=1 latched=0 survived_start=0 cleared=1
```

## 2026-09-14 — E05 source-only LTO: RISC-V at 150 MHz

- Experiment: `E05-source-lto-riscv-03`
- Configuration: same source-only `-flto` candidate as the matched ARM run;
  Pico SDK objects remain conventionally compiled. Requested/actual system
  clock: 150,000/150,000,000 kHz/Hz.
- Validation: all six test groups passed, **4,096/4,096** independent oracle
  cases and **10/10** target boundary tests; cycle `CYCLE:PASS`.
- Kernel benchmark: **325,339 H/s**, 651,000 hashes in 2,000,988 us, checksum
  `28`; estimated **461.06 cycles/hash**.
- Sustained mining: **314,321 H/s** at 600,000 hashes (700,000 sample was
  314,317 H/s).
- Relative to E04-c RISC-V baseline (325,341 benchmark; 314,333 sustained):
  **-0.001% benchmark, -0.004% sustained**. This is measurement noise.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `fcc4aaf0b82024cea1b0b33848baa00791a0c4ebf61e20e9722ea32bb19dc056`
- Archived serial log: `logs/E05-source-lto-riscv.log`
- Decision: reject source-only LTO. It produces no measurable speed gain on
  either ISA and therefore does not justify its extra build/link complexity.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled lto=on clock_profile=stock requested_clock_khz=150000 actual_clock_hz=150000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=6 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=RISCV-HAZARD3 clock_hz=150000000 hashes=651000 elapsed_us=2000988 hash_rate_hs=325339 checksum=28 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=314321 temperature=disabled
```

## 2026-09-14 — E04-e sticky-error batching (1,000): ARM at 150 MHz

- Experiment: `E04e-sticky-batch1000-arm-04`
- Change: split the hot hash kernel into checked and unchecked forms. The
  benchmark reads the sticky SHA write-error flag once per 1,000 hashes;
  mining adds a separate per-nonce counter and validates every 1,000 hashes
  and immediately before publishing a candidate.
- Hardware premise test: **passed**. An intentionally induced illegal WDATA
  write set `ERR_WDATA_NOT_RDY`; the flag survived `START`; the SDK's explicit
  clear operation removed it (`latched=1 survived_start=1 cleared=1`).
- Validation: both architectures built without warnings. ARM passed seven test
  groups, **4,096/4,096** oracle cases and **10/10** target tests; cycle
  `CYCLE:PASS`.
- Kernel benchmark: **328,904 H/s**, 658,000 hashes in 2,000,586 us, checksum
  `bb`; estimated **456.06 cycles/hash**.
- Sustained mining: **314,360 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline (324,632 benchmark; 320,406 sustained):
  **+1.32% benchmark, -1.89% sustained**.
- Interpretation: batching removes about six cycles/hash in the benchmark, but
  the additional mining counter/increment/compare branch costs more than the
  eliminated CSR read. The premise is valid; this first mining implementation
  is not.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `c33a95a97c1dad576d1be858cfa0b04a96984668ebf7d74c9f591067ec10b2b9`
- Archived serial log: `logs/E04e-sticky-batch1000-arm.log`
- Decision: reject the separate 1,000-hash mining counter. Retain the benchmark
  batching concept for the next subvariant; check mining errors at its existing
  100,000-hash report boundary and still check immediately before any share.

```text
TEST:PASS kat=sha_error_sticky cases=3 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=658000 elapsed_us=2000586 hash_rate_hs=328904 checksum=bb temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=314360 temperature=disabled
```

## 2026-09-14 — E04-e report-boundary error batching: ARM at 150 MHz

- Experiment: `E04e-report-boundary-arm-05`
- Change from the first E04-e subvariant: remove its dedicated 1,000-hash
  mining counter/branch. Benchmark still checks each 1,000-hash batch; mining
  checks the sticky flag at the existing 100,000-hash reporting boundary and
  immediately before any candidate publication.
- Validation: both builds passed without warnings. The sticky-error hardware
  premise and all seven test groups passed, including **4,096/4,096** oracle
  cases and **10/10** target cases; cycle `CYCLE:PASS`.
- Kernel benchmark: **328,902 H/s**, 658,000 hashes in 2,000,595 us, checksum
  `bb`; estimated **456.06 cycles/hash**.
- Sustained mining: **319,723 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline (324,632 benchmark; 320,406 sustained):
  **+1.32% benchmark, -0.21% sustained**.
- Interpretation: report-boundary reuse recovers nearly all of the first
  subvariant's mining regression, but the measured end-to-end miner remains
  slightly slower. The benchmark-only gain is repeatable.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `9ce389ea2647a36acf19697c11ed2acfe0b2b7fd652b37e1e8329ea169003d62`
- Archived serial log: `logs/E04e-report-boundary-arm.log`
- Decision: defer retain/reject until the matched RISC-V measurement.

```text
TEST:PASS kat=sha_error_sticky cases=3 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=ARM-M33 clock_hz=150000000 hashes=658000 elapsed_us=2000595 hash_rate_hs=328902 checksum=bb temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=319723 temperature=disabled
```

## 2026-09-14 — E04-e report-boundary error batching: RISC-V at 150 MHz

- Experiment: `E04e-report-boundary-riscv-06`
- Configuration: identical E04-e report-boundary candidate measured on
  Hazard3 RISC-V at requested/actual 150,000/150,000,000 kHz/Hz.
- Validation: all seven test groups passed, including the sticky-error hardware
  premise, **4,096/4,096** oracle cases and **10/10** target cases; cycle
  `CYCLE:PASS`.
- Kernel benchmark: **330,356 H/s**, 661,000 hashes in 2,000,872 us, checksum
  `6f`; estimated **454.06 cycles/hash**.
- Sustained mining: **321,050 H/s** at 700,000 hashes.
- Relative to E04-c RISC-V baseline (325,341 benchmark; 314,333 sustained):
  **+1.54% benchmark, +2.14% sustained**.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `71731c6e02b1559165f59f6328d9be3b42509a0f463230a0ec5aaf3ce3e58c41`
- Archived serial log: `logs/E04e-report-boundary-riscv.log`
- Decision: retain sticky-error batching for Hazard3. The matched ARM result
  improved its benchmark but slightly reduced sustained mining, so the next
  subvariant will keep batched benchmark checks on both ISAs and select the
  batched mining path only for RISC-V.

```text
TEST:PASS kat=sha_error_sticky cases=3 latched=1 survived_start=1 cleared=1
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000872 hash_rate_hs=330356 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=321050 temperature=disabled
```

## 2026-09-14 — E04-e architecture split, checked ARM mining

- Experiment: `E04e-arch-split-arm-07-reject`
- Change: retain the batched benchmark for both ISAs; compile checked-per-hash
  mining on ARM and report-boundary batching on RISC-V. The ARM branch still
  shared the candidate variable/control structure introduced for RISC-V.
- Validation: both builds passed without warnings. ARM passed all seven test
  groups, **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- Kernel benchmark: **328,901 H/s**, 658,000 hashes in 2,000,600 us, checksum
  `bb`; estimated **456.06 cycles/hash**.
- Sustained mining: **315,032 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline: **+1.31% benchmark, -1.68% sustained**.
- Interpretation: checked-per-hash semantics alone did not restore ARM mining
  speed. The shared loop source shape changes M33 register allocation/layout;
  the next subvariant must restore the original ARM loop shape exactly.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `d3cd7503d974936ccd8ee5e81b5af760bd0a583668535aebdcb2ab2cd790fbf7`
- RISC-V UF2 remained byte-identical to the accepted preceding result:
  `71731c6e02b1559165f59f6328d9be3b42509a0f463230a0ec5aaf3ce3e58c41`.
- Archived serial log: `logs/E04e-arch-split-arm-checked.log`
- Decision: reject this ARM source shape; do not reflash the byte-identical
  RISC-V image.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=ARM-M33 clock_hz=150000000 hashes=658000 elapsed_us=2000600 hash_rate_hs=328901 checksum=bb temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=315032 temperature=disabled
```

## 2026-09-14 — E05 ARM 32-byte label alignment

- Experiment: `E05-arm-align-labels32-08-reject`
- Change: compile only `src/main.c` on ARM with
  `-falign-labels=32:31`. This placed the benchmark and mining back-edge
  targets on 64-byte addresses but also aligned many non-hot labels.
- Validation: both architecture builds passed without warnings. RISC-V stayed
  byte-identical to its accepted E04-e image. ARM passed all seven test groups,
  **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- Code size: ARM text increased from 176,108 to **178,108 bytes** (+2,000).
- Kernel benchmark: **317,074 H/s**, 635,000 hashes in 2,002,684 us, checksum
  `f6`; estimated **473.08 cycles/hash**.
- Sustained mining: **306,026 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline: **-2.33% benchmark, -4.49% sustained**.
- Interpretation: broadly aligning labels increases XIP footprint/fetch cost
  more than favorable placement helps. Address alignment by itself is not a
  performance guarantee on this flash-resident image.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `7b4cda7247fcd8151beda13811b6a3cc772c6e7c406581090a5ea5b50ccfff44`
- Archived serial log: `logs/E05-arm-align-labels32.log`
- Decision: reject and remove `-falign-labels=32:31`. Do not apply broad
  alignment flags to the full miner translation unit.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=ARM-M33 clock_hz=150000000 hashes=635000 elapsed_us=2002684 hash_rate_hs=317074 checksum=f6 temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=306026 temperature=disabled
```

## 2026-09-14 — E04-b next-nonce preparation overlap: ARM at 150 MHz

- Experiment: `E04b-nonce-overlap-arm-09`
- Change: prepare nonce zero once, then byte-swap/store `nonce+1` after the
  final second-hash WDATA write and before waiting for `SUM_VLD`, attempting to
  hide preparation inside the final 57-cycle compression. E04-e error strategy
  remains architecture-specific.
- Validation: both builds passed without warnings. ARM passed all seven test
  groups, **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- Kernel benchmark: **328,903 H/s**, 658,000 hashes in 2,000,590 us, checksum
  `bb`; estimated **456.06 cycles/hash**.
- Sustained mining: **313,715 H/s** at 700,000 hashes.
- Relative to the refined E04-e ARM result: benchmark effectively unchanged
  (+0.0003%); sustained **-1.88%**. Relative to E04-c sustained: **-2.09%**.
- Interpretation: nonce preparation was already hidden or overlapped in the
  benchmark schedule; the altered M33 mining layout is harmful.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `77c24836ffca7a5b2393082bddef1f58791f10e69bcedb5c0f6c0f472fcb0491`
- Archived serial log: `logs/E04b-nonce-overlap-arm.log`
- Decision: reject for ARM. Measure the matched Hazard3 result before deciding
  whether to retain it conditionally for RISC-V.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=nonce-prep-overlap-e04b-e04e arch=ARM-M33 clock_hz=150000000 hashes=658000 elapsed_us=2000590 hash_rate_hs=328903 checksum=bb temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=313715 temperature=disabled
```

## 2026-09-14 — E04-b next-nonce preparation overlap: RISC-V at 150 MHz

- Experiment: `E04b-nonce-overlap-riscv-10-reject`
- Configuration: matched nonce-preparation overlap candidate on Hazard3, with
  E04-e report-boundary sticky-error checks retained.
- Validation: all seven test groups passed, including **4,096/4,096** oracle
  cases and **10/10** target cases; cycle `CYCLE:PASS`.
- Kernel benchmark: **329,632 H/s**, 660,000 hashes in 2,002,235 us, checksum
  `16`; estimated **455.05 cycles/hash**.
- Sustained mining: **321,048 H/s** at 700,000 hashes.
- Relative to accepted E04-e RISC-V (330,356 benchmark; 321,050 sustained):
  **-0.22% benchmark, -0.001% sustained**.
- Interpretation: explicit next-nonce preparation does not expose a useful
  compression gap on Hazard3; sustained code was already effectively
  overlapped and benchmark layout became slightly worse.
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `dd5944577b52144bfa9a3b29ea53e068db157fca84fd24154140a50f2e3129f6`
- Archived serial log: `logs/E04b-nonce-overlap-riscv.log`
- Decision: reject E04-b for both architectures and revert to the accepted
  E04-e implementation.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=nonce-prep-overlap-e04b-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=660000 elapsed_us=2002235 hash_rate_hs=329632 checksum=16 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=700000 total_hashes=700000 hash_rate_hs=321048 temperature=disabled
```

## 2026-09-14 — Retained E04-e RISC-V confirmation

- Experiment: `E04e-retained-riscv-11-confirm`
- Recovery/configuration: after reverting rejected E04-b, rebuilt both ISAs
  and reproduced the accepted RISC-V E04-e UF2 byte-for-byte, then flashed it.
- Validation: all seven test groups passed, including sticky-error semantics,
  **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- Kernel benchmark: **330,357 H/s**, 661,000 hashes in 2,000,867 us, checksum
  `6f` (first accepted run: 330,356 H/s).
- Sustained mining: **321,051 H/s** at 600,000 hashes (first accepted run:
  321,050 H/s).
- Temperature: intentionally **disabled**, not measured and not inferred.
- Firmware UF2 SHA-256:
  `71731c6e02b1559165f59f6328d9be3b42509a0f463230a0ec5aaf3ce3e58c41`
- Archived serial log: `logs/E04e-retained-riscv-confirm.log`
- Decision: retain. The repeated result confirms the E04-e Hazard3 gain and
  leaves the board running the known-good 150 MHz image.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=661000 elapsed_us=2000867 hash_rate_hs=330357 checksum=6f temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=321051 temperature=disabled
```

## 2026-09-14 — E04-e isolated to RISC-V; retained ARM result

- Experiment: `E04e-isa-isolation-arm-12-retain`
- Change: select the original checked-per-hash E04-c benchmark/mining path for
  ARM; retain batched sticky-error checks in both benchmark and mining only for
  RISC-V. The RISC-V UF2 remained byte-identical to its accepted E04-e image.
- Validation: both builds passed without warnings. ARM passed all seven test
  groups, **4,096/4,096** oracle cases and **10/10** target cases; cycle passed.
- ARM kernel benchmark: **323,934 H/s**, 648,000 hashes in 2,000,410 us,
  checksum `2a`; estimated **463.06 cycles/hash**.
- ARM sustained mining: **320,415 H/s** at 700,000 hashes.
- Relative to E04-c ARM baseline: **-0.22% benchmark, +0.003% sustained**;
  effectively the retained ARM performance envelope.
- RISC-V build identity: exact accepted E04-e UF2, whose confirmation measured
  330,357 H/s benchmark and 321,051 H/s sustained.
- Temperature: intentionally **disabled**, not measured and not inferred.
- ARM UF2 SHA-256:
  `39d740eee0f47f23ed2bcc2fbfd3b015e76d858fc1ca990a3489bfed7607783f`
- RISC-V UF2 SHA-256:
  `71731c6e02b1559165f59f6328d9be3b42509a0f463230a0ec5aaf3ce3e58c41`
- Archived ARM serial log: `logs/E04e-isolated-arm-retained.log`
- Decision: retain the ISA-specific policy. It preserves M33 sustained speed
  and the reproducible Hazard3 E04-e gain without broad layout flags.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=648000 elapsed_us=2000410 hash_rate_hs=323934 checksum=2a temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=700000 total_hashes=700000 hash_rate_hs=320415 temperature=disabled
```

## 2026-09-14 — Retained stock-clock RISC-V device state

- Experiment: `retained-riscv-150mhz-final-21`
- Source/head: `49a54ae`
- Purpose: leave the connected board running the fastest retained image after the paired ARM measurement.
- Validation: ARM and RISC-V builds passed without warnings; all 7 device tests passed, including the 4096-case oracle and 10 target checks.
- Kernel benchmark: **339,327 H/s** (`679000` hashes in `2001019 us`, checksum `32`).
- Sustained mining: **333,166 H/s** at nonce `600000`.
- Temperature: disabled as requested.
- UF2 SHA-256: `046d49334a9648833b3b58668124e09e52c8acbc82bfd46f724fc93ecbfa7f71`.
- Serial log: `logs/retained-riscv-150mhz-final-20260914.log`.
- Decision: retained. The board is running this RISC-V image at 150 MHz.

Runtime evidence:

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=sticky-error-batched-e04e arch=RISCV-HAZARD3 clock_hz=150000000 hashes=679000 elapsed_us=2001019 hash_rate_hs=339327 checksum=32 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=600000 total_hashes=600000 hash_rate_hs=333166 temperature=disabled
```

## 2026-09-14 — E06a persistent first-block DMA, ARM

- Experiment: `E06a-persistent-first-block-dma-arm-22`.
- Candidate commit: `a930ac8`, based on retained commit `c66b434`.
- Change: claim and configure one 32-bit DMA channel once per hashing job and
  reuse it to feed the invariant first 64-byte header block to SHA WDATA. The
  tail block, intermediate-digest handoff, and second hash remain CPU-fed.
- Configuration: RP2350A, ARM Cortex-M33, stock 150 MHz, DMA paced by
  `DREQ_SHA256`, temperature disabled. ARM and RISC-V builds passed without
  warnings before flashing.
- Validation: all 7 tests passed, including 4,096 independent oracle vectors,
  10 target-boundary cases, genesis digest/search, and the sticky-error proof.
- Kernel benchmark: **331,087 H/s** (`663000` hashes in `2002494 us`, checksum
  `cb`), **-0.22%** versus the retained ARM result of 331,819 H/s.
- Sustained mining: **324,568 H/s** at 1,000,000 hashes, about **-0.20%** versus
  the retained ARM result of 325,207 H/s.
- Firmware: ARM UF2 SHA-256
  `a133cd562cdb6219673b337ee2d4f8c1ceccc35e2440983b4432805fe3044987`,
  344,576 bytes.
- Archived serial log: `logs/E06a-persistent-first-block-dma-arm.log`.
- Decision: reject for ARM on measured performance, but measure the identical
  Hazard3 image before reverting because DMA setup/polling costs are
  architecture-dependent.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=ARM-M33 clock_hz=150000000 hashes=663000 elapsed_us=2002494 hash_rate_hs=331087 checksum=cb temperature=disabled
MINING:PROGRESS arch=ARM-M33 nonce=1000000 total_hashes=1000000 hash_rate_hs=324568 temperature=disabled
```

## 2026-09-14 — E06a persistent first-block DMA, RISC-V run 1

- Experiment: `E06a-persistent-first-block-dma-riscv-23-run1`.
- Candidate commit: `a930ac8`; measurement follows ARM record commit
  `cb3c882` with identical firmware source.
- Configuration: RP2350A, Hazard3 RISC-V, stock 150 MHz, one persistent 32-bit
  DMA channel paced by `DREQ_SHA256` for the invariant first block;
  temperature disabled.
- Validation: all 7 tests passed, including 4,096 independent oracle vectors,
  10 target-boundary cases, genesis digest/search, and sticky-error proof.
- Kernel benchmark: **344,784 H/s** (`690000` hashes in `2001251 us`, checksum
  `92`), **+1.61%** versus the retained RISC-V result of 339,327 H/s.
- Sustained mining: **335,421 H/s** at 2,000,000 hashes, **+0.68%** versus the
  retained RISC-V result of 333,166 H/s.
- Firmware: RISC-V UF2 SHA-256
  `493193f79243ae533efed0114c335ecc72d4a7825b2fb4d3c754e8496de74250`,
  367,616 bytes.
- Archived serial log: `logs/E06a-persistent-first-block-dma-riscv-run1.log`.
- Decision: promising architecture-specific result. It is the fastest measured
  stock-clock kernel so far but remains below the nominal 2% retention gate;
  repeat the byte-identical artifact before selecting an ISA split.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=690000 elapsed_us=2001251 hash_rate_hs=344784 checksum=92 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=2000000 total_hashes=2000000 hash_rate_hs=335421 temperature=disabled
```

## 2026-09-14 — E06a persistent first-block DMA, RISC-V confirmed

- Experiment: `E06a-persistent-first-block-dma-riscv-23-confirm`.
- Artifact: byte-identical RISC-V UF2
  `493193f79243ae533efed0114c335ecc72d4a7825b2fb4d3c754e8496de74250`.
- Validation: both architectures rebuilt without warnings before flashing;
  all 7 RISC-V device tests passed again.
- Kernel benchmark: **344,783 H/s** (`690000` hashes in `2001260 us`, checksum
  `92`), within 1 H/s of run 1 and **+1.61%** over the retained 339,327 H/s.
- Sustained mining: **335,420 H/s** at 1,000,000 hashes, within 1 H/s of run 1
  and **+0.68%** over the retained 333,166 H/s.
- Archived serial log:
  `logs/E06a-persistent-first-block-dma-riscv-confirm.log`.
- Decision: retain for Hazard3 despite being below the nominal 2% gate. The
  byte-identical repeat is stable, it establishes the fastest stock-clock
  result, correctness coverage is complete, and the implementation uses one
  persistent channel with bounded ownership. Restore the CPU feeder on ARM,
  where the same change regressed both metrics.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=690000 elapsed_us=2001260 hash_rate_hs=344783 checksum=92 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=1000000 total_hashes=1000000 hash_rate_hs=335420 temperature=disabled
```

## 2026-09-14 — E06a full-DMA RISC-V, late mining capture

- Experiment: `E06a-full-dma-riscv-24-late-capture`.
- Candidate commit: `d78f547`, based on retained ISA split `d8979ba`.
- Change: retain persistent DMA ownership, but send the complete 32-word padded
  header in one DREQ-paced transfer and the complete 16-word padded second hash
  in another. Eight intermediate SUM words are saved into persistent SRAM
  before the required second START/transfer.
- Configuration: RP2350A, Hazard3 RISC-V, stock 150 MHz, temperature disabled.
  Both architecture builds passed without warnings; ARM remained byte-identical
  to its retained UF2.
- Recovery note: the candidate UF2 flashed and verified, but VM USB runtime
  capture was initially absent. After reattaching the runtime device, firmware
  was already in `MINING:PROGRESS`. By program control flow this is reachable
  only after all gated tests and benchmark complete, but their original serial
  lines were not captured; an explicit reboot capture remains required.
- Sustained mining: **328,800 H/s** at 203,800,000 hashes, **-1.97%** versus the
  retained first-block-DMA result of 335,420 H/s.
- Firmware: RISC-V UF2 SHA-256
  `3605faeae9a89d063b3104fd3dc6a78b5daeb3996a29c09c4de99ed5bb96929a`,
  367,104 bytes.
- Archived serial log: `logs/E06a-full-dma-riscv-late-capture.log`.
- Decision: sustained regression; pending one explicit startup/benchmark
  capture before rejection and restoration of the first-block-only DMA path.

```text
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=203800000 total_hashes=203800000 hash_rate_hs=328800 temperature=disabled
```

## 2026-09-14 — E06a full-DMA RISC-V, delayed-capture confirmation

- Experiment: `E06a-full-dma-riscv-24-delayed-confirm`.
- Capture instrumentation commit: `4cc78b0`; RISC-V startup delay increased
  to 120 seconds solely to accommodate VM USB reattachment. The full-DMA
  hashing implementation is unchanged from `d78f547`; ARM remains
  byte-identical to its retained image.
- The USB runtime was attached after the delay and one-shot validation output
  had already passed. Firmware was again in mining, which is reachable only
  after the gated tests and benchmark complete, but explicit validation lines
  were not captured.
- Sustained mining: **328,800 H/s** at 10,000,000 hashes, confirming the prior
  328,800 H/s result and the **-1.97%** regression versus first-block DMA.
- Delayed RISC-V UF2 SHA-256:
  `507e91cc34b8d1e274ac8ac1493e8344756a8f0a570a81c0a8d5620547985ff1`.
- Archived serial log: `logs/E06a-full-dma-riscv-delayed-explicit.log`.
- Decision: performance rejection is repeatable. Replace the unreliable
  one-shot delay with temporary repeated validation output to capture the
  exact kernel rate and explicit pass lines before restoring E06a first-block
  DMA.

```text
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=10000000 total_hashes=10000000 hash_rate_hs=328800 temperature=disabled
```

## 2026-09-14 — E06a full-DMA RISC-V explicit validation and rejection

- Experiment: `E06a-full-dma-riscv-24-explicit-reject`.
- Hashing candidate commit: `d78f547`; temporary repeated-validation capture
  commit: `9659435`. The repeated wrapper does not change `run_benchmark()` or
  the full-DMA hash routine.
- Configuration: RP2350A, Hazard3 RISC-V, stock 150 MHz, temperature disabled.
- Validation: six complete captured repetitions each passed all 7 tests,
  including 4,096 oracle vectors, 10 target cases, genesis digest/search, and
  sticky-error proof. No `FAULT` or SHA error appeared.
- Kernel samples: **335,513, 335,527, 335,526, 335,528, 335,527, and 335,528
  H/s**; median **335,527 H/s**. Representative window: `672000` hashes in
  `2002819 us`, checksum `c6`.
- Sustained mining: **329,522 H/s** at 5,000,000 hashes.
- Versus retained first-block-only DMA: kernel **-2.69%** from 344,783 H/s;
  sustained **-1.76%** from 335,420 H/s.
- Capture UF2 SHA-256:
  `b5ede94df1a20058938869fcd379103038bbd3e0d1572a5399060800a5f37f56`.
- Archived serial log:
  `logs/E06a-full-dma-riscv-repeated-validation.log`.
- Interpretation: DMA efficiently replaces Hazard3's sixteen first-block MMIO
  stores, but extending it across all three blocks loses more to eight SUM-to-
  SRAM stores plus a second DMA rearm/wait than it saves on direct WDATA feeds.
- Decision: reject full DMA. Restore the byte-confirmed E06a first-block-only
  DMA path and remove all temporary capture repetition/delay instrumentation.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-full-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=672000 elapsed_us=2002819 hash_rate_hs=335527 checksum=c6 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=5000000 total_hashes=5000000 hash_rate_hs=329518 temperature=disabled
```

## 2026-09-14 — E06a first-block DMA restored after full-DMA rejection

- Experiment: `E06a-first-block-dma-riscv-25-restored`.
- Restoration commit: `b1bb7dd`; source exactly matches the retained E06a
  architecture split from `d8979ba`.
- Both architectures rebuilt without warnings. ARM UF2 is byte-identical at
  `9898f3013920729e84b4533d87e5edca3605ba02aaa5ec53a833555014d3f9b4`;
  RISC-V UF2 is byte-identical at
  `493193f79243ae533efed0114c335ecc72d4a7825b2fb4d3c754e8496de74250`.
- Validation basis: the identical RISC-V artifact previously passed all 7
  tests in two explicit captures and measured 344,783–344,784 H/s kernel.
- Restored sustained mining: **335,420 H/s** at 3,900,000 hashes, exactly the
  retained performance envelope.
- Archived serial log: `logs/E06a-restored-riscv-final.log`.
- Decision: restoration confirmed; board runs the stock-clock RISC-V
  first-block-DMA winner. Advance to selective placement under DMA contention.

```text
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=3900000 total_hashes=3900000 hash_rate_hs=335420 temperature=disabled
```

## 2026-09-14 — E07 selective RISC-V hot-main SRAM placement

- Experiment: `E07-riscv-main-sram-26-reject`.
- Candidate commit: `28e9125`, based on retained E06a commit `48fa3c3`.
- Change: place only RISC-V `main` in the SDK `.time_critical.main` SRAM
  section. The compiler's single-use benchmark/mining paths remain inlined, so
  their actual hot instructions move without adding call boundaries. ARM is
  unchanged and byte-identical.
- Placement evidence: RISC-V XIP `.text` decreases from `0x846c` to `0x80ec`;
  SRAM `.data` increases from `0x2f68` to `0x3368`, an exact **1,024-byte**
  SRAM cost. Candidate UF2 SHA-256:
  `781fc69bbe144dedc1a919a8675d3597203060984a58dd85cccba0ca404db033`.
- Validation: all 7 tests passed in the initial cycle, including 4,096 oracle
  vectors and the target/error tests. The benchmark line fell into the VM USB
  monitor handoff gap; reaching mining proves the benchmark returned success,
  but no kernel rate is claimed for this candidate.
- Sustained mining: stable **335,414–335,420 H/s** across two restarts and
  multi-million-hash captures; representative **335,420 H/s** at 3,600,000
  hashes. This is indistinguishable from retained E06a at 335,420 H/s.
- Archived logs: `logs/E07-riscv-main-sram-startup.log`,
  `logs/E07-riscv-main-sram-followup.log`, and
  `logs/E07-riscv-main-sram-explicit.log`.
- Decision: reject. DMA/XIP contention does not measurably reduce sustained
  throughput in this configuration, and spending 1 KiB SRAM has no benefit.
  Restore the byte-identical first-block-DMA image.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=RISCV-HAZARD3 nonce=3600000 total_hashes=3600000 hash_rate_hs=335420 temperature=disabled
```

## 2026-09-14 — E08 RISC-V core1 mining worker, first run

- Experiment: `E08-riscv-core1-worker-27-run1`.
- Candidate commit: `64ef8e5`, based on the retained E06a first-block DMA
  implementation. Core 1 owns the SHA engine and hot mining loop; core 0 owns
  USB serial output and the LED. Progress/share/fault records cross the
  inter-core FIFO, so `printf` no longer interrupts the mining core.
- Both ARM and RISC-V builds passed without warnings before flashing.
- Validation: all 7 test suites passed, including 4,096 oracle vectors,
  target/error tests, and the Bitcoin genesis digest/search. No `FAULT`
  occurred and the hardware cycle completed successfully.
- Kernel benchmark: **343,994 H/s** (`688000` hashes in `2000037 us`). This
  startup benchmark still executes on core 0 and is not the target of E08.
- Sustained core1 mining: **340,094 H/s** representative, with reports stable
  at approximately 340,084–340,103 H/s through 9.9 million hashes.
- Versus retained E06a sustained rate of 335,420 H/s: **+1.39%**. Moving USB
  formatting/reporting off the mining core recovers about 4,674 H/s.
- Candidate RISC-V UF2 SHA-256:
  `b7c261b8fb9bcd06b1fbfaa26c95b596b65647757266280d0e6463ff03539c22`.
- Archived serial log: `logs/E08-riscv-core1-worker-run1.log`.
- Decision: promising; retain provisionally and confirm from a fresh reboot.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=688000 elapsed_us=2000037 hash_rate_hs=343994 checksum=c8 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 worker_core=1 nonce=9900000 total_hashes=9900000 hash_rate_hs=340094 temperature=disabled
```

## 2026-09-14 — E08 RISC-V core1 mining worker, confirmation

- Experiment: `E08-riscv-core1-worker-27-confirm`.
- The same installed candidate was autonomously cold-restarted through BOOTSEL
  and explicit RISC-V application reboot; no rebuild or source change occurred.
- Sustained mining settled at **340,093 H/s** and remained in the narrow
  340,079–340,101 H/s range through 7.7 million hashes.
- This reproduces run 1 within **1 H/s** and confirms the **+1.39%** gain over
  retained E06a (335,420 H/s).
- Decision: accept E08 as the new RISC-V sustained-mining baseline. The board is
  currently running the accepted image.

```text
MINING:PROGRESS arch=RISCV-HAZARD3 worker_core=1 nonce=7700000 total_hashes=7700000 hash_rate_hs=340097 temperature=disabled
```

## 2026-09-14 — E08 ARM core1 mining worker

- Experiment: `E08-arm-core1-worker-28`.
- Candidate/retained implementation: commit `64ef8e5`; core 1 owns the SHA
  mining loop and core 0 owns USB output and LED reporting.
- Both architectures built cleanly before flash. On ARM, all 7 test suites
  passed, no `FAULT` occurred, and the complete hardware cycle passed.
- Kernel benchmark: **331,084 H/s** (`663000` hashes in `2002515 us`).
- Sustained core1 mining: **327,469 H/s** representative, stable within roughly
  327,451–327,477 H/s through 9.5 million hashes.
- Versus the retained pre-E08 ARM sustained baseline of 325,207 H/s:
  **+0.70%** (+2,262 H/s). The benefit is smaller than RISC-V's +1.39%, but
  repeatability and separation of USB work justify the shared implementation.
- ARM UF2 SHA-256:
  `3d3383b569a60b527bdc1b62fa81ae409371acc4bb3c69a1eac78a256fad323d`.
- Archived serial log: `logs/E08-arm-core1-worker.log`.
- Decision: accept E08 for ARM and RISC-V. RISC-V remains the faster stock-clock
  architecture at 340,093–340,094 H/s sustained.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=663000 elapsed_us=2002515 hash_rate_hs=331084 checksum=cb temperature=disabled
MINING:PROGRESS arch=ARM-M33 worker_core=1 nonce=9500000 total_hashes=9500000 hash_rate_hs=327473 temperature=disabled
```

## 2026-09-14 — E09-a portable software midstate, RISC-V baseline

- Experiment: `E09a-software-midstate-riscv-29`.
- Candidate commit: `dc11e82`. Added an independent portable 32-bit SHA-256
  compression path. It caches the first 64-byte Bitcoin-header midstate and
  performs exactly two software compressions per nonce.
- Correctness: all 4,096 deterministic host-oracle cases matched both the
  retained hardware kernel and the new software-midstate kernel. All 7 test
  suites passed, including genesis digest/search and target/error tests.
- Software benchmark: **18,656 H/s**, `38000` complete double hashes in
  `2036924 us`, checksum `1c`.
- Hardware benchmark in the same image: **344,781 H/s**. Sustained core1
  hardware mining remained **340,101 H/s**, showing no regression while the
  software code is idle.
- Candidate RISC-V UF2 SHA-256:
  `d93e1a0e4e03b614b3784ec585497c5c99df79c10ab61e33defef01f2685bcca`.
- Code-size cost versus E08 RISC-V: 2,520 bytes of ELF text.
- Archived serial log: `logs/E09a-software-midstate-riscv.log`.
- Interpretation: unoptimized portable software is only 5.5% of the hardware
  worker's rate, but could raise dual-core aggregate throughput to about
  358.8 kH/s before contention. Proceed to ARM measurement and E09-b tuning.

```text
TEST:PASS kat=optimized_oracle engines=hardware,software-midstate cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=38000 elapsed_us=2036924 hash_rate_hs=18656 checksum=1c temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 worker_core=1 nonce=11400000 total_hashes=11400000 hash_rate_hs=340106 temperature=disabled
```

## 2026-09-14 — E09-a portable software midstate, ARM baseline

- Experiment: `E09a-software-midstate-arm-30`.
- Same candidate commit `dc11e82` and exact two-compression workload as the
  RISC-V run. All 4,096 host-oracle vectors matched hardware and software; all
  7 suites and the full cycle passed without faults.
- Software benchmark: **18,946 H/s**, `38000` hashes in `2005701 us`, checksum
  `1c`. This is only **1.55%** faster than RISC-V's 18,656 H/s.
- Hardware benchmark: **330,354 H/s**. Sustained core1 hardware mining remained
  **327,471 H/s** through 11.1 million hashes.
- Candidate ARM UF2 SHA-256:
  `3b65af6c048498d5022df1d0437ee260fdaa9ed7ebf7106986b43662c3ecc39a`.
- Code-size cost versus E08 ARM: 1,624 bytes of ELF text.
- Archived serial log: `logs/E09a-software-midstate-arm.log`.
- Decision: retain the correct E09-a reference implementation, but optimize
  it before dual-worker integration. The portable round/schedule loop leaves
  substantial CPU opportunity on both ISAs.

```text
TEST:PASS kat=optimized_oracle engines=hardware,software-midstate cases=4096 fixture_sha256=4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=38000 elapsed_us=2005701 hash_rate_hs=18946 checksum=1c temperature=disabled
MINING:PROGRESS arch=ARM-M33 worker_core=1 nonce=11000000 total_hashes=11000000 hash_rate_hs=327475 temperature=disabled
```

## 2026-09-14 — E09-b1 software round-loop unrolling, RISC-V

- Experiment: `E09b1-unroll-riscv-31`.
- Candidate commit: `9b1c228`. Only `software_sha256_compress()` receives the
  compiler's `unroll-loops` function option; algorithm and workload are
  unchanged from E09-a.
- Both architectures built cleanly. All 4,096 hardware/software oracle cases
  and all 7 test suites passed on RISC-V; no fault occurred.
- Software benchmark: **19,005 H/s**, `39000` hashes in `2052070 us`, checksum
  `2c`. Versus E09-a RISC-V 18,656 H/s: **+1.87%** (+349 H/s).
- Hardware benchmark: **344,780 H/s**; sustained hardware mining remained
  **340,120 H/s**.
- Candidate RISC-V UF2 SHA-256:
  `94809f10022dd767c328d0fe2571bc2d7abed69baaf2c1674125ca3f5f3ff18d`.
- Text-size increase over E09-a: 248 bytes on RISC-V, 160 bytes on ARM.
- Archived serial log: `logs/E09b1-unroll-riscv.log`.
- Decision: provisional retain; small positive gain at low complexity. Measure
  ARM before making the attribute architecture-specific or shared.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=39000 elapsed_us=2052070 hash_rate_hs=19005 checksum=2c temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 worker_core=1 nonce=3000000 total_hashes=3000000 hash_rate_hs=340118 temperature=disabled
```

## 2026-09-14 — E09-b1 software round-loop unrolling, ARM

- Experiment: `E09b1-unroll-arm-32`.
- Same candidate commit `9b1c228`; both builds were clean and all 4,096
  cross-engine oracle cases plus all 7 suites passed on ARM.
- Software benchmark: **19,144 H/s**, `39000` hashes in `2037175 us`, checksum
  `2c`. Versus E09-a ARM 18,946 H/s: **+1.05%** (+198 H/s).
- Hardware benchmark: **330,360 H/s**; sustained core1 mining was about
  **327,461 H/s**, effectively unchanged.
- Candidate ARM UF2 SHA-256:
  `3b39764d2d9042af11d8c05d38de602edb9791d3027baec8697084c23bd768cd`.
- Archived serial log: `logs/E09b1-unroll-arm.log`.
- Decision: retain the function-local unroll option for both architectures;
  its gain is repeatable and its text-size cost is small.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=39000 elapsed_us=2037175 hash_rate_hs=19144 checksum=2c temperature=disabled
MINING:PROGRESS arch=ARM-M33 worker_core=1 nonce=3000000 total_hashes=3000000 hash_rate_hs=327468 temperature=disabled
```

## 2026-09-14 — E09-b2 expanded software schedule, RISC-V

- Experiment: `E09b2-expanded-schedule-riscv-33`.
- Candidate commit: `8bf57fa`. Replace the 16-word circular schedule and
  per-round wrapped indexing with a 64-word expanded schedule. SHA rounds,
  nonce work, checksumming, and the E09-b1 unroll setting remain unchanged.
- Both architectures built cleanly. All 4,096 hardware/software oracle cases
  and all 7 suites passed on RISC-V; no fault occurred.
- Software benchmark: **24,126 H/s**, `49000` hashes in `2031001 us`, checksum
  `ce`. Versus retained E09-b1 RISC-V 19,005 H/s: **+26.94%** (+5,121 H/s).
- Hardware benchmark: **344,782 H/s**; sustained hardware remained about
  **340,120 H/s**.
- Candidate RISC-V UF2 SHA-256:
  `e46d8f3029f62b6853c4f678362b0c724c822d32a9565811dc62612ce20f6de9`.
- Resource delta versus E09-b1: +556 bytes ELF text and +192 bytes temporary
  stack per compression invocation; calls are sequential, not nested.
- Archived serial log: `logs/E09b2-expanded-schedule-riscv.log`.
- Decision: retain for RISC-V. The large gain justifies the small memory cost;
  measure ARM next.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=49000 elapsed_us=2031001 hash_rate_hs=24126 checksum=ce temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 worker_core=1 nonce=3300000 total_hashes=3300000 hash_rate_hs=340123 temperature=disabled
```

## 2026-09-14 — E09-b2 expanded software schedule, ARM

- Experiment: `E09b2-expanded-schedule-arm-34`.
- Same candidate commit `8bf57fa`; both builds were clean and all 4,096
  cross-engine oracle cases plus all 7 suites passed on ARM.
- Software benchmark: **26,440 H/s**, `53000` hashes in `2004536 us`, checksum
  `45`. Versus E09-b1 ARM 19,144 H/s: **+38.11%** (+7,296 H/s).
- Hardware benchmark: **330,356 H/s**; sustained core1 hardware remained
  approximately **327,468 H/s**.
- Candidate ARM UF2 SHA-256:
  `229f2f9c94b7abafcb24f6857693dffd1f14e6bd99cc390e9b59d10fb8e5ef96`.
- Resource delta versus E09-b1: -56 bytes ELF text and +192 bytes temporary
  stack. The expanded form is both smaller and much faster for ARM.
- Archived serial log: `logs/E09b2-expanded-schedule-arm.log`.
- Decision: retain expanded scheduling for both architectures. ARM is the
  faster homogeneous software SHA choice, while RISC-V remains the faster
  hardware-peripheral owner; this also strengthens the future mixed-ISA case.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=53000 elapsed_us=2004536 hash_rate_hs=26440 checksum=45 temperature=disabled
MINING:PROGRESS arch=ARM-M33 worker_core=1 nonce=3000000 total_hashes=3000000 hash_rate_hs=327474 temperature=disabled
```

## 2026-09-14 — E10-a independent hardware + software workers, RISC-V

- Experiment: `E10a-dual-worker-riscv-35`.
- Candidate commit: `61a3913`. Core 1 evaluates even nonces with the retained
  hardware/DMA path; core 0 evaluates odd nonces with E09-b2 software SHA and
  services FIFO/USB between hashes. Each parity range stops before repetition,
  so reported aggregate work is disjoint and unique.
- Both architectures built cleanly. All 4,096 cross-engine oracle cases and all
  7 suites passed on RISC-V; no fault occurred and the cycle passed.
- Representative sustained rates: hardware **336,698 H/s**, software
  **19,668 H/s**, aggregate **356,366 H/s**. Aggregate reports stayed roughly
  355,891–357,102 H/s; settled values cluster near 356.3 kH/s.
- Versus E08 RISC-V hardware-only sustained 340,093 H/s: aggregate **+4.78%**
  (+16,273 H/s). Compared with isolated components, bus/XIP/USB contention
  costs about 3.4 kH/s hardware and 4.5 kH/s software, but net work is positive.
- Startup isolated benchmarks in this linked image: hardware **342,420 H/s**,
  software **23,972 H/s**.
- Candidate RISC-V UF2 SHA-256:
  `0ded2c8cf49d89b18c424e75fe253a459e14553cb6f5d79b909228c1b0cdaff2`.
- Archived serial log: `logs/E10a-dual-worker-riscv.log`.
- Decision: retain provisionally. Confirm ARM, then optimize contention and
  software common-path overhead against this measured aggregate baseline.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=RISCV-HAZARD3 hardware_core=1 hardware_nonce=9000000 hardware_hashes=4500000 hardware_rate_hs=336758 software_core=0 software_nonce=524241 software_hashes=262120 software_rate_hs=19608 total_hashes=4762120 hash_rate_hs=356366 temperature=disabled
```

## 2026-09-14 — E10-a independent hardware + software workers, ARM

- Experiment: `E10a-dual-worker-arm-36`.
- Same candidate commit `61a3913`; hardware uses even nonces on core 1 and
  software uses odd nonces on core 0. Both builds were clean, all 4,096
  cross-engine oracle cases and all 7 suites passed, and no fault occurred.
- Representative sustained rates: hardware **323,961 H/s**, software
  **24,526 H/s**, aggregate **348,487 H/s**. Most settled aggregate reports are
  approximately 348.4–348.5 kH/s.
- Versus E08 ARM hardware-only sustained 327,469 H/s: aggregate **+6.42%**
  (+21,018 H/s). The hardware worker loses about 1.1%, while the active
  software/control core contributes about 24.5 kH/s net.
- Startup isolated benchmarks: hardware **331,083 H/s**, software
  **26,444 H/s**.
- Candidate ARM UF2 SHA-256:
  `b846356adc884b59e201eb58268e5b294d36253c298a3d19b706aafc2e3800fb`.
- Archived serial log: `logs/E10a-dual-worker-arm.log`.
- Decision: retain E10-a for both architectures. RISC-V remains the fastest
  homogeneous aggregate at about 356.4 kH/s; revisit SRAM placement because
  the dual-core XIP workload now introduces the contention absent in E07.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=ARM-M33 hardware_core=1 hardware_nonce=9000000 hardware_hashes=4500000 hardware_rate_hs=323961 software_core=0 software_nonce=681461 software_hashes=340730 software_rate_hs=24526 total_hashes=4840730 hash_rate_hs=348487 temperature=disabled
```

## 2026-09-14 — E07-b/E10-a software compression in SRAM, RISC-V

- Experiment: `E07b-software-compress-sram-riscv-37`.
- Candidate commit: `b7ae98c`. Move only the 1,322-byte RISC-V software SHA
  compression routine from XIP flash to SRAM; round constants and all hardware
  mining/control code remain in their prior locations. This specifically tests
  the dual-core contention introduced by E10-a, unlike rejected single-core E07.
- Both architectures built cleanly. All 4,096 cross-engine oracle cases and all
  7 suites passed; no fault occurred and the cycle passed.
- Median of the final 30 sustained reports: hardware **338,600 H/s**, software
  **23,394 H/s**, aggregate **361,993 H/s**.
- Versus retained E10-a RISC-V aggregate 356,366 H/s: **+1.58%** (+5,627 H/s).
  Versus E08 hardware-only 340,093 H/s: aggregate **+6.44%**.
- Isolated startup rates in this image: hardware **342,420 H/s**, software
  **24,357 H/s**.
- Candidate RISC-V UF2 SHA-256:
  `b22558c1f3e4f3bc06ce6e1e313d7640dfefe254ac49728a939db4a71f1f790d`.
- SRAM cost: 1,322 bytes of executable `.data`; stack usage is unchanged.
- Archived serial log: `logs/E07b-E10a-software-compress-sram-riscv.log`.
- Decision: retain provisionally for RISC-V. The second core makes XIP
  placement material; measure ARM before selecting a shared/ISA-specific form.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=RISCV-HAZARD3 hardware_core=1 hardware_nonce=9000000 hardware_hashes=4500000 hardware_rate_hs=338681 software_core=0 software_nonce=621787 software_hashes=310893 software_rate_hs=23394 total_hashes=4810893 hash_rate_hs=362075 temperature=disabled
```

## 2026-09-14 — E07-b/E10-a software compression in SRAM, ARM

- Experiment: `E07b-software-compress-sram-arm-38`.
- Same candidate commit `b7ae98c`; ARM moves its software compression routine
  to SRAM while constants and hardware/control code remain in XIP.
- Both builds were clean. All 4,096 cross-engine oracle cases and all 7 suites
  passed; no fault occurred and the hardware cycle passed.
- Median of the final 30 sustained reports: hardware **325,546 H/s**, software
  **26,411 H/s**, aggregate **351,958 H/s**.
- Versus E10-a ARM aggregate 348,487 H/s: **+1.00%** (+3,471 H/s). Versus E08
  hardware-only 327,469 H/s: aggregate **+7.48%**.
- Isolated startup rates: hardware **331,085 H/s**, software **26,613 H/s**.
- Candidate ARM UF2 SHA-256:
  `1a2aad5933c82925b316d835cd9a8b07e1150115b1e68892497c56807ec73fa8`.
- Archived serial log: `logs/E07b-E10a-software-compress-sram-arm.log`.
- Decision: retain the 1.3 KiB-class software compression SRAM placement for
  both architectures under E10-a dual-core load.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=ARM-M33 hardware_core=1 hardware_nonce=8400000 hardware_hashes=4200000 hardware_rate_hs=325625 software_core=0 software_nonce=681489 software_hashes=340744 software_rate_hs=26412 total_hashes=4540744 hash_rate_hs=352037 temperature=disabled
```

## 2026-09-14 — E07-c SHA round constants in SRAM, RISC-V

- Experiment: `E07c-round-constants-sram-riscv-39`.
- Candidate commit: `2313352`. In addition to the accepted 1,322-byte
  compression routine, move its 256-byte SHA round-constant table from XIP to
  SRAM. No algorithm or reporting change.
- Both architectures built cleanly. All 4,096 cross-engine oracle cases and all
  7 suites passed; no fault occurred and the cycle passed.
- Median of the final 30 reports: hardware **338,976 H/s**, software
  **23,500 H/s**, aggregate **362,477 H/s**.
- Versus code-only SRAM RISC-V aggregate 361,993 H/s: **+0.13%** (+484 H/s),
  below the normal retention threshold and small relative to run variation.
- Candidate RISC-V UF2 SHA-256:
  `92e2cc22b92ed7fad19d30f937ac4a2bf73bb1a6fad8b404e1cbbbcae4d30062`.
- Archived serial log: `logs/E07c-round-constants-sram-riscv.log`.
- Decision: inconclusive/provisional until ARM is measured; likely reject the
  extra 256-byte SRAM cost unless the second architecture shows a clearer gain.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=RISCV-HAZARD3 hardware_core=1 hardware_nonce=9400000 hardware_hashes=4700000 hardware_rate_hs=339027 software_core=0 software_nonce=655003 software_hashes=327501 software_rate_hs=23615 total_hashes=5027501 hash_rate_hs=362642 temperature=disabled
```

## 2026-09-15 — E07-c SHA round constants in SRAM, ARM and rejection

- Experiment: `E07c-round-constants-sram-arm-40`.
- Same candidate commit `2313352`; 256-byte constant-table move layered on the
  retained software-compression-in-SRAM implementation.
- Both architectures built cleanly. All 4,096 cross-engine oracle cases and all
  7 suites passed on ARM; no fault occurred and the cycle passed.
- Median of the final 30 ARM reports: hardware **325,709 H/s**, software
  **26,312 H/s**, aggregate **352,022 H/s**.
- Versus code-only SRAM ARM aggregate 351,958 H/s: **+0.02%** (+64 H/s), noise.
  RISC-V had similarly marginal +0.13%.
- Candidate ARM UF2 SHA-256:
  `058f136264e7c8df1fdbf4fb8be659fd5731f507ac9eeec8b67f27d23f38f1c4`.
- Archived serial log: `logs/E07c-round-constants-sram-arm.log`.
- Decision: reject E07-c on both architectures. Keep the round constants in
  XIP and retain only the clearly beneficial compression-code SRAM placement.

```text
TEST:SUMMARY pass=7 fail=0
MINING:PROGRESS arch=ARM-M33 hardware_core=1 hardware_nonce=8400000 hardware_hashes=4200000 hardware_rate_hs=325707 software_core=0 software_nonce=678793 software_hashes=339396 software_rate_hs=26319 total_hashes=4539396 hash_rate_hs=352026 temperature=disabled
```

## 2026-09-15 — E05 RISC-V `-mbranch-cost=1` software flag

- Experiment: `E05-branch-cost-riscv-41` at candidate commit `bd99396`.
- Scoped `-mbranch-cost=1` only to `src/software_sha256.c`; ARM was unchanged.
- Both architectures built cleanly. The RISC-V UF2 remained byte-identical to
  the retained code-only SRAM image at
  `b22558c1f3e4f3bc06ce6e1e313d7640dfefe254ac49728a939db4a71f1f790d`;
  ARM likewise remained byte-identical.
- Decision: reject without flashing. The installed Hazard3 compiler's existing
  choices for this branch-light expanded-schedule kernel are unchanged, so the
  expected hardware result is exactly the retained binary's measured result.

## 2026-09-15 — E05 RISC-V `-fno-code-hoisting` software flag

- Experiment: `E05-no-code-hoisting-riscv-42`, candidate commit `e887274`.
- The flag was scoped only to `src/software_sha256.c`; both architectures built
  cleanly, but both UF2 files remained byte-identical to the retained binaries.
- Decision: reject without flashing because generated code did not change.

## 2026-09-15 — E05 RISC-V `-fno-if-conversion2` software flag

- Experiment: `E05-no-if-conversion2-riscv-43`, candidate commit `9d071d6`.
- The flag was scoped only to `src/software_sha256.c`; both architectures built
  cleanly, but both UF2 files remained byte-identical to the retained binaries.
- Decision: reject without flashing because generated code did not change.
  This completes the three individually tested Hazard3 flag suggestions from
  E05; none altered this expanded-schedule software kernel.

## 2026-09-15 — E09-b3 reusable padded blocks, RISC-V

- Experiment: `E09b3-reusable-blocks-riscv-44`, candidate commit `720b573`.
- Prebuilt and retained both padded blocks per job, mutating only nonce/digest
  words, instead of clearing local blocks per nonce. Both architectures built
  cleanly; all 4,096 cross-engine oracle vectors and all 7 suites passed.
- Isolated software: **22,970 H/s**, down 5.69% from the code-only-SRAM
  reference's 24,357 H/s.
- Median of the final 30 dual-worker reports: hardware **338,233 H/s**,
  software **21,770 H/s**, aggregate **360,002 H/s**.
- Versus retained RISC-V aggregate 361,993 H/s: **-0.55%** (-1,991 H/s).
- Candidate RISC-V UF2 SHA-256:
  `33d4df58f9d7fb12dfb5dd5e9b406bacd3f78395ab5cdd5ccc0164ea357c7aa6`.
- Archived serial log: `logs/E09b3-reusable-blocks-riscv.log`.
- Decision: reject; the enlarged persistent state/load pattern outweighs the
  saved per-call clears. Restore local blocks and do not spend an ARM flash.

## 2026-09-15 — E05 whole-target link-time optimization build rejection

- Experiment: `E05-whole-target-lto-45`.
- Enabled CMake interprocedural optimization on the complete retained
  dual-worker target so cross-translation-unit calls and constants could be
  optimized together.
- ARM compilation completed, but the LTO link failed on Pico SDK wrapped
  `printf`/`puts` references with unsupported ARM/Thumb relocations.
- RISC-V compilation failed because its Hazard3 toolchain reported that
  `-fno-fat-lto-objects` requires an unavailable linker plugin.
- No image was flashed and no throughput measurement was taken. This is a
  build-system/toolchain incompatibility, not a runtime performance result.
- Decision: reject portable whole-target LTO and restore the retained build.
  Any future LTO attempt must be narrowly scoped and must first demonstrate
  clean builds for both supported architectures.

## 2026-09-15 — E09-b fixed header-tail precomputation, RISC-V

- Experiment: `E09b-round3-w16w17-riscv-46`, candidate commit `df70e18`.
- Precomputed the SHA-256 working state after header-tail rounds 0--2 and the
  nonce-independent schedule words W16/W17 once per job. The per-nonce first
  compression starts at nonce-dependent round 3 and schedule word W18.
- Both architectures built warning-free. On RISC-V, all 4,096 cross-engine
  oracle cases and all 7 suites passed; no fault occurred.
- Isolated software: **25,530 H/s**, up **4.82%** from the retained code-SRAM
  reference's 24,357 H/s.
- Median of the final 30 dual-worker reports: hardware **338,550 H/s**,
  software **24,588 H/s**, aggregate **363,138 H/s**.
- Versus retained RISC-V aggregate 361,993 H/s: **+0.32%** (+1,145 H/s).
- Candidate RISC-V UF2 SHA-256:
  `9cb1c388c5e11fbce1c6d2ff364933391c516e40ea69f73c7f49544419ed0691`.
- Archived serial log: `logs/E09b-round3-w16w17-riscv.log`.
- Decision: provisionally successful because the isolated software gain is
  substantial and exact; measure ARM before the cross-architecture decision.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=52000 elapsed_us=2036788 hash_rate_hs=25530 checksum=88 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 hardware_core=1 hardware_nonce=8600000 hardware_hashes=4300000 hardware_rate_hs=338691 software_core=0 software_nonce=623201 software_hashes=311600 software_rate_hs=24534 total_hashes=4611600 hash_rate_hs=363225 temperature=disabled
```

## 2026-09-15 — E09-b fixed header-tail precomputation, ARM and retention

- Experiment: `E09b-round3-w16w17-arm-47`, candidate commit `df70e18`.
- Both architectures remained warning-free. On ARM, all 4,096 cross-engine
  oracle cases and all 7 suites passed; no fault occurred.
- Isolated software: **26,727 H/s**, up **1.09%** from the prior expanded-
  schedule reference's 26,440 H/s.
- Median of the final 30 dual-worker reports: hardware **325,541 H/s**,
  software **26,410 H/s**, aggregate **351,950 H/s**.
- Versus retained ARM aggregate 351,958 H/s: **-0.002%** (-8 H/s), noise.
  The sustained ARM result is unchanged, while the isolated test sees a small
  repeatable-looking reduction in work; RISC-V gains 4.82% isolated and 0.32%
  aggregate.
- Candidate ARM UF2 SHA-256:
  `df89e629b7f29b3eb5e6dd186bf4eb47da7a4d4276c25a155e2148681f595866`.
- Archived serial log: `logs/E09b-round3-w16w17-arm.log`.
- Decision: retain E09-b on both architectures. Architecture-gating it would
  add a second code path without a demonstrated ARM aggregate benefit; the
  shared implementation is exact, improves the ARM isolated benchmark, and
  does not measurably regress sustained ARM throughput.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=54000 elapsed_us=2020403 hash_rate_hs=26727 checksum=61 temperature=disabled
MINING:PROGRESS arch=ARM-M33 hardware_core=1 hardware_nonce=8600000 hardware_hashes=4300000 hardware_rate_hs=325546 software_core=0 software_nonce=698415 software_hashes=349207 software_rate_hs=26435 total_hashes=4649207 hash_rate_hs=351981 temperature=disabled
```

## 2026-09-15 — E09-b specialized fixed second block, RISC-V

- Experiment: `E09b-fixed-second-block-riscv-48`, candidate commit `d9c0de8`.
- Added a dedicated second-compression path for the fixed 32-byte SHA digest
  layout: eight digest words, `0x80000000`, six zeros, and length 256. This
  removes the temporary padded block, its clear/copy, and the second-state copy.
- Both architectures built warning-free. On RISC-V, all 4,096 cross-engine
  oracle cases and all 7 suites passed; no fault occurred.
- Isolated software: **25,927 H/s**, up **1.55%** from E09-b's 25,530 H/s.
- Median of the final 30 dual-worker reports: hardware **338,473 H/s**,
  software **24,826 H/s**, aggregate **363,300 H/s**.
- Versus E09-b RISC-V aggregate 363,138 H/s: **+0.045%** (+162 H/s).
- Candidate RISC-V UF2 SHA-256:
  `6352d8d034fd137c780eb73dd742ebb9d89ff7133d1bb69f3818b2c7bc9b8518`.
- Archived serial log: `logs/E09b-fixed-second-block-riscv.log`.
- Decision: provisional; software-specific metrics improve, but measure ARM
  before accepting the extra specialized SRAM-resident routine.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=52000 elapsed_us=2005609 hash_rate_hs=25927 checksum=88 temperature=disabled
MINING:PROGRESS arch=RISCV-HAZARD3 hardware_core=1 hardware_nonce=9200000 hardware_hashes=4600000 hardware_rate_hs=338790 software_core=0 software_nonce=673853 software_hashes=336926 software_rate_hs=24798 total_hashes=4936926 hash_rate_hs=363588 temperature=disabled
```

## 2026-09-15 — E09-b specialized fixed second block, ARM and retention

- Experiment: `E09b-fixed-second-block-arm-49`, candidate commit `d9c0de8`.
- Both architectures remained warning-free. On ARM, all 4,096 cross-engine
  oracle cases and all 7 suites passed; no fault occurred.
- Isolated software: **26,905 H/s**, up **0.67%** from E09-b's 26,727 H/s.
- Median of the final 30 dual-worker reports: hardware **325,515 H/s**,
  software **26,672 H/s**, aggregate **352,188 H/s**.
- Versus E09-b ARM aggregate 351,950 H/s: **+0.068%** (+238 H/s).
- Candidate ARM UF2 SHA-256:
  `d64ad9ecf2bf02e1364d12b9ba31c8c54f6ee72f85b7496912d4b3d1ef52e6a3`.
- Archived serial log: `logs/E09b-fixed-second-block-arm.log`.
- Decision: retain on both architectures. The aggregate gain is deliberately
  reported as small, but isolated and sustained software-worker rates improve
  consistently on both ISAs with exact full-digest output.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=54000 elapsed_us=2007083 hash_rate_hs=26905 checksum=61 temperature=disabled
MINING:PROGRESS arch=ARM-M33 hardware_core=1 hardware_nonce=8400000 hardware_hashes=4200000 hardware_rate_hs=325531 software_core=0 software_nonce=689005 software_hashes=344502 software_rate_hs=26699 total_hashes=4544502 hash_rate_hs=352230 temperature=disabled
```

## 2026-09-15 — E11 200 MHz clock profile, RISC-V

- Experiment: `E11-200mhz-riscv-50` using retained source commit `d9c0de8`.
- Built both architectures warning-free with `MINER_SYS_CLOCK_KHZ=200000`,
  then flashed RISC-V. Startup reported an actual 200,000,000 Hz system clock
  and the separate `experimental-overclock` profile.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **459,711 H/s**. Isolated software: **34,570 H/s**.
- Median of the final 30 dual-worker reports: hardware **451,738 H/s**,
  software **34,024 H/s**, aggregate **485,762 H/s**.
- Versus the same retained kernel at 150 MHz (363,300 H/s aggregate):
  **+33.71%** (+122,462 H/s), close to ideal 4/3 frequency scaling.
- Archived serial log: `logs/E11-200mhz-riscv.log`.
- Qualification: this is a short functional/performance experiment only.
  Temperature is disabled by user direction, so it is not a thermal or
  long-duration reliability qualification and is not the default profile.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=200000 actual_clock_hz=200000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=200000000 hashes=920000 elapsed_us=2001259 hash_rate_hs=459711 checksum=c0 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=200000000 hashes=70000 elapsed_us=2024883 hash_rate_hs=34570 checksum=93 temperature=disabled
```

## 2026-09-15 — E11 200 MHz clock profile, ARM

- Experiment: `E11-200mhz-arm-51` using retained source commit `d9c0de8`.
- The already warning-free 200 MHz builds were used. Startup reported an
  actual 200,000,000 Hz system clock and `experimental-overclock` profile.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **441,450 H/s**. Isolated software: **35,872 H/s**.
- Median of the final 30 dual-worker reports: hardware **434,011 H/s**,
  software **35,449 H/s**, aggregate **469,460 H/s**.
- Versus the same retained kernel at 150 MHz (352,188 H/s aggregate):
  **+33.30%** (+117,272 H/s), essentially ideal 4/3 scaling.
- Archived serial log: `logs/E11-200mhz-arm.log`.
- Qualification: short functional/performance experiment only; temperature is
  disabled, so this does not establish thermal margin or long-term stability.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=200000 actual_clock_hz=200000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=200000000 hashes=883000 elapsed_us=2000227 hash_rate_hs=441450 checksum=da temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=200000000 hashes=72000 elapsed_us=2007147 hash_rate_hs=35872 checksum=be temperature=disabled
```

## 2026-09-15 — E11 250 MHz clock profile, RISC-V

- Experiment: `E11-250mhz-riscv-52` using retained source commit `d9c0de8`.
- Built both architectures warning-free with `MINER_SYS_CLOCK_KHZ=250000`,
  without changing regulator voltage, then flashed RISC-V. Startup reported an
  actual 250,000,000 Hz system clock.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **574,641 H/s**. Isolated software: **43,214 H/s**.
- Median of the final 30 dual-worker reports: hardware **564,689 H/s**,
  software **42,418 H/s**, aggregate **607,106 H/s**.
- Versus 200 MHz aggregate 485,762 H/s: **+24.98%** (+121,344 H/s).
  Versus 150 MHz aggregate 363,300 H/s: **+67.11%** (+243,806 H/s), near
  ideal 5/3 scaling.
- Archived serial log: `logs/E11-250mhz-riscv.log`.
- Qualification: short experimental run only, with temperature disabled; it
  is not a long-duration reliability result or a production default.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=250000 actual_clock_hz=250000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=250000000 hashes=1150000 elapsed_us=2001250 hash_rate_hs=574641 checksum=3d temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=250000000 hashes=87000 elapsed_us=2013243 hash_rate_hs=43214 checksum=cc temperature=disabled
```

## 2026-09-15 — E11 250 MHz clock profile, ARM

- Experiment: `E11-250mhz-arm-53` using retained source commit `d9c0de8`.
- Startup reported an actual 250,000,000 Hz system clock; no regulator-voltage
  change was made.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **551,815 H/s**. Isolated software: **44,842 H/s**.
- Median of the final 30 dual-worker reports: hardware **542,516 H/s**,
  software **44,353 H/s**, aggregate **586,868 H/s**.
- Versus 200 MHz aggregate 469,460 H/s: **+25.01%** (+117,408 H/s).
  Versus 150 MHz aggregate 352,188 H/s: **+66.64%** (+234,680 H/s).
- Archived serial log: `logs/E11-250mhz-arm.log`.
- Qualification: short experimental run only, with temperature disabled; it
  is not a long-duration reliability result or a production default.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=250000 actual_clock_hz=250000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=250000000 hashes=1104000 elapsed_us=2000671 hash_rate_hs=551815 checksum=d3 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=250000000 hashes=90000 elapsed_us=2007058 hash_rate_hs=44842 checksum=8f temperature=disabled
```

## 2026-09-15 — E11 300 MHz clock profile, RISC-V

- Experiment: `E11-300mhz-riscv-54` using retained source commit `d9c0de8`.
- Built both architectures warning-free with `MINER_SYS_CLOCK_KHZ=300000`,
  without changing regulator voltage, then flashed RISC-V. Startup reported an
  actual 300,000,000 Hz system clock.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **689,572 H/s**. Isolated software: **51,857 H/s**.
- Median of the final 30 dual-worker reports: hardware **677,580 H/s**,
  software **50,633 H/s**, aggregate **728,213 H/s**.
- Versus 250 MHz aggregate 607,106 H/s: **+19.95%** (+121,107 H/s).
  Versus 150 MHz aggregate 363,300 H/s: **+100.44%** (+364,913 H/s).
- A few brief periodic throughput dips coincide with host USB/reporting
  activity in the raw log; the final-30 median excludes their distortion.
- Archived serial log: `logs/E11-300mhz-riscv.log`.
- Qualification: short experimental run only, temperature disabled, no
  long-duration reliability claim.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=RISCV-HAZARD3 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=300000 actual_clock_hz=300000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=300000000 hashes=1380000 elapsed_us=2001240 hash_rate_hs=689572 checksum=d6 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=300000000 hashes=104000 elapsed_us=2005521 hash_rate_hs=51857 checksum=43 temperature=disabled
```

## 2026-09-15 — E11 300 MHz clock profile, ARM

- Experiment: `E11-300mhz-arm-55` using retained source commit `d9c0de8`.
- Startup reported an actual 300,000,000 Hz system clock; no regulator-voltage
  change was made.
- All 4,096 cross-engine oracle cases and all 7 suites passed; no fault
  occurred during the short capture.
- Isolated hardware: **662,179 H/s**. Isolated software: **53,810 H/s**.
- Median of the final 30 dual-worker reports: hardware **651,014 H/s**,
  software **52,973 H/s**, aggregate **703,987 H/s**.
- Versus 250 MHz aggregate 586,868 H/s: **+19.96%** (+117,119 H/s).
  Versus 150 MHz aggregate 352,188 H/s: **+99.89%** (+351,799 H/s), almost
  exact 2x scaling.
- Archived serial log: `logs/E11-300mhz-arm.log`.
- Qualification: short experimental run only, temperature disabled, no
  long-duration reliability claim.

```text
BOOT app=pico2_bitcoin_miner board=pico2 package=RP2350A arch=ARM-M33 engine=RP2350-SHA256 temperature=disabled clock_profile=experimental-overclock requested_clock_khz=300000 actual_clock_hz=300000000 sysinfo_package_sel=1 chip_id=30004927 silicon_revision=3
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=300000000 hashes=1325000 elapsed_us=2000969 hash_rate_hs=662179 checksum=d7 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=300000000 hashes=108000 elapsed_us=2007049 hash_rate_hs=53810 checksum=5e temperature=disabled
```

## 2026-09-15 — E11 350 MHz clock profile rejection, RISC-V

- Experiment: `E11-350mhz-riscv-56` using retained source commit `d9c0de8`.
- Both architectures built warning-free with `MINER_SYS_CLOCK_KHZ=350000` and
  the RISC-V image flashed successfully.
- At startup, `set_sys_clock_khz(350000, false)` returned failure and firmware
  emitted `FAULT type=system_clock requested_khz=350000`. Per the test gate,
  the cycle exited nonzero and no KAT or benchmark result was accepted.
- Archived serial log: `logs/E11-350mhz-riscv-rejected.log`.
- Decision: reject 350 MHz with the current exact-frequency/no-voltage-change
  configuration. Do not attempt higher points through this path; 300 MHz is
  the highest validated short-run point in this sweep. This is a clock API /
  synthesis rejection, not evidence of arithmetic instability at 350 MHz.

```text
FAULT type=system_clock requested_khz=350000
```

## 2026-09-15 — E09-c exact round-61 rejection, RISC-V

- Experiment: `E09c-round61-filter-riscv-57`, candidate commit `41ab4bd`, at
  the stock 150 MHz clock.
- For zero-high-word Bitcoin targets, the software worker now stops after
  round 60 of the second compression and uses `(IV7 + e_61)` as the exact
  final numerical digest word 7. Only a zero match triggers full-digest
  recomputation and the complete uint256 comparison.
- The derived word was checked against the full oracle digest for all 4,096
  deterministic cases. All 7 suites passed and no fault occurred.
- Full-digest software reference: **25,927 H/s**. Exact round-61 filter:
  **26,333 H/s**, a **1.57%** filter-path gain.
- Median of the final 30 dual-worker reports: hardware **338,846 H/s**,
  software **25,608 H/s**, aggregate **364,453 H/s**.
- Versus the preceding fixed-second-block RISC-V aggregate 363,300 H/s:
  **+0.32%** (+1,153 H/s).
- Candidate RISC-V UF2 SHA-256:
  `3099af51ce1d72b75027f3174f1d2e6272c36c88b48a3eec0d1152d3fe791b6b`.
- Archived serial log: `logs/E09c-round61-filter-riscv.log`.
- Decision: provisionally successful; verify M33 before retention. The metric
  is exact nonce rejection for the configured target, not full digest output.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=52000 elapsed_us=2005604 hash_rate_hs=25927 checksum=88 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=53000 elapsed_us=2012667 hash_rate_hs=26333 checksum=90732335 temperature=disabled
```

## 2026-09-15 — E09-c exact round-61 rejection, ARM and retention

- Experiment: `E09c-round61-filter-arm-58`, candidate commit `41ab4bd`, at
  the stock 150 MHz clock.
- Both architectures built warning-free. The derived high word matched the
  full oracle digest for all 4,096 deterministic ARM cases; all 7 suites
  passed and no fault occurred.
- Full-digest software reference: **26,900 H/s**. Exact round-61 filter:
  **27,376 H/s**, a **1.77%** filter-path gain.
- Median of the final 30 dual-worker reports: hardware **325,712 H/s**,
  software **27,208 H/s**, aggregate **352,919 H/s**.
- Versus the preceding fixed-second-block ARM aggregate 352,188 H/s:
  **+0.21%** (+731 H/s).
- Candidate ARM UF2 SHA-256:
  `e79f716c22a5623065bed0e6f371236821ee00a3187ae987c7cb1aa95f0d9725`.
- Archived serial log: `logs/E09c-round61-filter-arm.log`.
- Decision: retain E09-c on both architectures. It performs exact rejection
  for the configured zero-high-word target; any rare high-word equality is
  recomputed through the existing full-digest path before share publication.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=54000 elapsed_us=2007413 hash_rate_hs=26900 checksum=61 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=55000 elapsed_us=2009035 hash_rate_hs=27376 checksum=73309c95 temperature=disabled
```

## 2026-09-15 — E05 truthful `restrict` contracts, RISC-V

- Experiment: `E05-restrict-riscv-59`, candidate commit `4b570b6`, at the
  stock 150 MHz clock.
- Added non-aliasing contracts to the public software SHA-256 buffers and the
  internal specialized tail-compression buffers. Both architectures built
  warning-free before the RISC-V image was flashed.
- All 4,096 oracle cases and all 7 suites passed; no fault occurred.
- Full-digest software: **25,928 H/s**, versus 25,927 H/s before the change
  (effectively unchanged). Exact round-61 filter: **26,333 H/s**, exactly the
  preceding measured rate.
- Median of the final 30 dual-worker reports: hardware **338,936 H/s**,
  software **26,094 H/s**, aggregate **365,030 H/s**.
- Versus E09-c RISC-V aggregate 364,453 H/s: **+0.16%** (+577 H/s), but the
  isolated metrics show no hot-path improvement, so this small movement is
  currently classified as run variance rather than a demonstrated gain.
- Archived serial log: `logs/E05-restrict-riscv.log`.
- Decision: provisionally neutral; test ARM before retaining or reverting.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=52000 elapsed_us=2005522 hash_rate_hs=25928 checksum=88 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=53000 elapsed_us=2012659 hash_rate_hs=26333 checksum=90732335 temperature=disabled
```

## 2026-09-15 — E05 truthful `restrict` contracts, ARM and rejection

- Experiment: `E05-restrict-arm-60`, candidate commit `4b570b6`, at the stock
  150 MHz clock. A 45-second cycle captured the filter and a stable mining
  window after the default 8-second capture proved too short.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **26,900 H/s**, exactly the pre-change rate. Exact
  round-61 filter: **27,376 H/s**, also exactly the pre-change rate.
- Median of the final 30 dual-worker reports: hardware **325,729 H/s**,
  software **27,246 H/s**, aggregate **352,975 H/s**.
- Versus E09-c ARM aggregate 352,919 H/s: **+0.016%** (+56 H/s), well inside
  run variance and unsupported by either isolated metric.
- Archived serial log: `logs/E05-restrict-arm.log`.
- Decision: reject E05 `restrict` as a performance optimization. It is
  correct on both targets but produces no measurable filter/full-digest gain;
  revert it to keep the API contracts no stronger than necessary.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=54000 elapsed_us=2007424 hash_rate_hs=26900 checksum=61 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=55000 elapsed_us=2009027 hash_rate_hs=27376 checksum=73309c95 temperature=disabled
```

## 2026-09-15 — E09-c trim unused filter schedule, ARM

- Experiment: `E09c-trim61-arm-61`, candidate commit `3835437`, at the stock
  150 MHz clock.
- Reduced the round-61 rejection helper's local schedule from 64 to 61 words
  and stopped expansion at W60, matching its rounds 0 through 60. The full
  digest implementation is unchanged. Both architectures built warning-free.
- All 4,096 oracle cases and all 7 suites passed on M33; no fault occurred.
- Full-digest software: **26,900 H/s**, unchanged. Exact round-61 filter:
  **27,447 H/s**, versus 27,376 H/s in both preceding ARM trials: **+0.26%**
  (+71 H/s).
- Median of the final 30 dual-worker reports: hardware **325,698 H/s**,
  software **27,316 H/s**, aggregate **353,014 H/s**.
- Versus the immediately preceding matched E05 ARM run, sustained software is
  **+0.26%** (+70 H/s) and aggregate is **+0.011%** (+39 H/s). Versus the
  retained E09-c aggregate 352,919 H/s, aggregate is **+0.027%** (+95 H/s).
- Candidate ARM UF2 SHA-256:
  `41dd0d3e72107a5b506aafdd9272d4fe9b45063cda2a85e412aa407b531fc574`.
- Archived serial log: `logs/E09c-trim61-arm.log`.
- Decision: provisionally retain because the isolated and sustained software
  deltas agree, but the gain is very small; test RISC-V before final decision.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=54000 elapsed_us=2007402 hash_rate_hs=26900 checksum=61 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=55000 elapsed_us=2003894 hash_rate_hs=27447 checksum=73309c95 temperature=disabled
```

## 2026-09-15 — E09-c trim unused filter schedule, RISC-V and retention

- Experiment: `E09c-trim61-riscv-62`, candidate commit `3835437`, at the
  stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **25,884 H/s**. This path is source-identical to the
  preceding candidate and its -0.17% movement is benchmark quantization/noise.
- Exact round-61 filter: **26,548 H/s**, versus 26,333 H/s in both preceding
  RISC-V trials: **+0.82%** (+215 H/s).
- Median of the final 30 dual-worker reports: hardware **338,921 H/s**,
  software **26,307 H/s**, aggregate **365,228 H/s**.
- Versus the immediately preceding matched E05 RISC-V run, sustained software
  is **+0.82%** (+213 H/s) and aggregate is **+0.054%** (+198 H/s). Versus the
  retained E09-c aggregate 364,453 H/s, aggregate is **+0.21%** (+775 H/s).
- Candidate RISC-V UF2 SHA-256:
  `054f1a4455ff11a844e00790bb8ba55b53cb71949caa6f65ca4910a4878a9b7d`.
- Archived serial log: `logs/E09c-trim61-riscv.log`.
- Decision: retain. Both M33 and Hazard3 show matching improvements in the
  isolated exact-filter metric and the sustained software worker, while all
  full-digest oracle tests remain valid. The aggregate improvement is small
  because the independent hardware worker dominates total throughput.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=52000 elapsed_us=2008988 hash_rate_hs=25884 checksum=88 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=54000 elapsed_us=2034074 hash_rate_hs=26548 checksum=3f1babeb temperature=disabled
```

## 2026-09-15 — E09-b round-3 partial precomputation, RISC-V

- Experiment: `E09b-round3-partial-riscv-63`, candidate commit `1e90bf1`, at
  the stock 150 MHz clock.
- Header-tail rounds 0 through 2 were already job-precomputed. This candidate
  also precomputes round 3's nonce-independent `temp1` base and `temp2`; each
  nonce completes that round with additions/state moves, then enters the
  ordinary expanded-schedule loop at round 4.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **26,018 H/s**. Exact round-61 filter: **26,694 H/s**,
  versus 26,548 H/s for the trimmed-schedule parent: **+0.55%** (+146 H/s).
- Median of the final 30 dual-worker reports: hardware **338,868 H/s**,
  software **26,445 H/s**, aggregate **365,313 H/s**.
- Versus the parent run, sustained software is **+0.52%** (+138 H/s) and
  aggregate is **+0.023%** (+85 H/s); hardware moved -53 H/s.
- The isolated hardware benchmark reported 339,320 H/s, but no hardware path
  source changed and sustained hardware remained near its parent rate. Treat
  that isolated movement as unrelated run variability.
- Candidate RISC-V UF2 SHA-256:
  `292bb515dddb7d6eefd04d8ff41ab7aa99852b71afc2d5d3adbdc3f906f42a72`.
- Archived serial log: `logs/E09b-round3-partial-riscv.log`.
- Decision: provisionally retain; isolated and sustained software gains agree.
  Verify M33 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=53000 elapsed_us=2037023 hash_rate_hs=26018 checksum=45 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=54000 elapsed_us=2022913 hash_rate_hs=26694 checksum=3f1babeb temperature=disabled
```

## 2026-09-15 — E09-b round-3 partial precomputation, ARM and retention

- Experiment: `E09b-round3-partial-arm-64`, candidate commit `1e90bf1`, at
  the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **27,282 H/s**, versus the parent 26,900 H/s:
  **+1.42%** (+382 H/s). Exact round-61 filter: **27,844 H/s**, versus the
  parent 27,447 H/s: **+1.45%** (+397 H/s).
- Median of the final 30 dual-worker reports: hardware **325,733 H/s**,
  software **27,697 H/s**, aggregate **353,430 H/s**.
- Versus the parent run, sustained software is **+1.39%** (+381 H/s) and
  aggregate is **+0.12%** (+416 H/s). The full, filter, and sustained
  software measurements agree on the direction and approximate magnitude.
- Candidate ARM UF2 SHA-256:
  `13f5e46162a9a64b1c73a53c875445daa7b2a790ac1623d86f635e1d657ff916`.
- Archived serial log: `logs/E09b-round3-partial-arm.log`.
- Decision: retain round-3 partial precomputation on both architectures. It
  removes invariant work without weakening the oracle/full-share path.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=55000 elapsed_us=2015963 hash_rate_hs=27282 checksum=e2 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=56000 elapsed_us=2011195 hash_rate_hs=27844 checksum=fffebb38 temperature=disabled
```

## 2026-09-15 — E09-b W18/W19 partial precomputation, ARM

- Experiment: `E09b-w18w19-partial-arm-65`, candidate commit `fa333bc`, at
  the stock 150 MHz clock.
- W18 and W19 still depend on the nonce, but their invariant schedule terms
  are now computed once per header. Per nonce, W18 adds `sigma0(nonce)` to its
  cached base and W19 adds the nonce word to its cached base.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **27,347 H/s**, versus parent 27,282 H/s: **+0.24%**
  (+65 H/s). Exact filter: **27,911 H/s**, versus parent 27,844 H/s:
  **+0.24%** (+67 H/s).
- Median of the final 30 dual-worker reports: hardware **325,650 H/s**,
  software **27,774 H/s**, aggregate **353,424 H/s**.
- Versus the parent run, sustained software is **+0.28%** (+77 H/s).
  Aggregate is effectively flat (-6 H/s) because hardware moved -83 H/s.
- Candidate ARM UF2 SHA-256:
  `b4bac1775d30e626df21bbb55dc37447b21ae4f399fa88dffe46241213ad4aa6`.
- Archived serial log: `logs/E09b-w18w19-partial-arm.log`.
- Decision: provisionally retain; all three software metrics agree on a small
  gain. Verify Hazard3 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=55000 elapsed_us=2011198 hash_rate_hs=27347 checksum=e2 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=56000 elapsed_us=2006349 hash_rate_hs=27911 checksum=fffebb38 temperature=disabled
```

## 2026-09-15 — E09-b W18/W19 partial precomputation, RISC-V and retention

- Experiment: `E09b-w18w19-partial-riscv-66`, candidate commit `fa333bc`, at
  the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **26,278 H/s**, versus parent 26,018 H/s: **+1.00%**
  (+260 H/s). Exact filter: **26,968 H/s**, versus parent 26,694 H/s:
  **+1.03%** (+274 H/s).
- Median of the final 30 dual-worker reports: hardware **338,828 H/s**,
  software **26,720 H/s**, aggregate **365,548 H/s**.
- Versus the parent run, sustained software is **+1.04%** (+275 H/s) and
  aggregate is **+0.064%** (+235 H/s); hardware moved -40 H/s.
- Candidate RISC-V UF2 SHA-256:
  `12109449758f7bb25513084099bcf11fda357a283f57cfe0615253ee4010e1df`.
- Archived serial log: `logs/E09b-w18w19-partial-riscv.log`.
- Decision: retain W18/W19 partial precomputation on both architectures. Its
  software benefit is present in full-digest, filter, and sustained metrics,
  with complete oracle agreement.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=53000 elapsed_us=2016886 hash_rate_hs=26278 checksum=45 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=54000 elapsed_us=2002387 hash_rate_hs=26968 checksum=3f1babeb temperature=disabled
```

## 2026-09-15 — E09-b W31/W32 partial precomputation, RISC-V

- Experiment: `E09b-w31w32-partial-riscv-67`, candidate commit `c8f6857`, at
  the stock 150 MHz clock.
- Cached the last two header-schedule sigma terms whose inputs are entirely
  job-invariant: `640 + sigma0(W16)` for W31 and `W16 + sigma0(W17)` for W32.
  Schedule words from W33 onward depend on nonce-derived inputs.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **26,623 H/s**, versus parent 26,278 H/s: **+1.31%**
  (+345 H/s). Exact filter: **27,331 H/s**, versus parent 26,968 H/s:
  **+1.35%** (+363 H/s).
- Median of the final 30 dual-worker reports: hardware **338,885 H/s**,
  software **27,080 H/s**, aggregate **365,965 H/s**.
- Versus the parent run, sustained software is **+1.35%** (+360 H/s) and
  aggregate is **+0.11%** (+417 H/s); hardware moved +57 H/s.
- Candidate RISC-V UF2 SHA-256:
  `d62875c32cfb21e875e1a1cc4a778b9f860463adaa113ceffdd972ca56ad8ac1`.
- Archived serial log: `logs/E09b-w31w32-partial-riscv.log`.
- Decision: provisionally retain; all software measurements show a consistent
  gain. Verify M33 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=54000 elapsed_us=2028296 hash_rate_hs=26623 checksum=61 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=55000 elapsed_us=2012337 hash_rate_hs=27331 checksum=73309c95 temperature=disabled
```

## 2026-09-15 — E09-b W31/W32 partial precomputation, ARM and retention

- Experiment: `E09b-w31w32-partial-arm-68`, candidate commit `c8f6857`, at
  the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **27,855 H/s**, versus parent 27,347 H/s: **+1.86%**
  (+508 H/s). Exact filter: **28,441 H/s**, versus parent 27,911 H/s:
  **+1.90%** (+530 H/s).
- Median of the final 30 dual-worker reports: hardware **325,746 H/s**,
  software **28,300 H/s**, aggregate **354,046 H/s**.
- Versus the parent run, sustained software is **+1.89%** (+526 H/s) and
  aggregate is **+0.18%** (+622 H/s); hardware moved +96 H/s.
- Candidate ARM UF2 SHA-256:
  `20156f6fd116e3b01fd2be4e16daa04664f669c369539594725e2467bb878a81`.
- Archived serial log: `logs/E09b-w31w32-partial-arm.log`.
- Decision: retain W31/W32 partial precomputation on both architectures. The
  complete correctness gate passed and all three software metrics improved.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=56000 elapsed_us=2010444 hash_rate_hs=27855 checksum=05 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=57000 elapsed_us=2004174 hash_rate_hs=28441 checksum=59f5c0dc temperature=disabled
```

## 2026-09-15 — E03 direct header digest output, ARM

- Experiment: `E03-direct-header-digest-arm-69`, candidate commit `b4af8dd`,
  at the stock 150 MHz clock.
- Removed the caller's 32-byte midstate-to-digest copy. The specialized header
  tail compressor now writes `midstate + working_state` directly into its
  output digest instead of feed-forward updating a preinitialized buffer.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **27,860 H/s**, versus parent 27,855 H/s (+5 H/s).
  Exact filter: **28,446 H/s**, versus parent 28,441 H/s (+5 H/s). Both are
  **+0.018%**, below measurement significance.
- Median of the final 30 dual-worker reports: hardware **325,704 H/s**,
  software **28,298 H/s**, aggregate **354,002 H/s**. Sustained software moved
  -2 H/s and aggregate -44 H/s versus the parent.
- Candidate ARM UF2 SHA-256:
  `e8c452ae807fc56bd67a19e36fa57c44d073b131d7e60ca11e85f82b971ceb90`.
- Archived serial log: `logs/E03-direct-header-digest-arm.log`.
- Decision: neutral on M33, likely because optimized code already eliminated
  most copy overhead. Test Hazard3 before rejecting the shared change.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=56000 elapsed_us=2010065 hash_rate_hs=27860 checksum=05 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=57000 elapsed_us=2003793 hash_rate_hs=28446 checksum=59f5c0dc temperature=disabled
```

## 2026-09-15 — E03 direct header digest output, RISC-V and retention

- Experiment: `E03-direct-header-digest-riscv-70`, candidate commit `b4af8dd`,
  at the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **27,568 H/s**, versus parent 26,623 H/s: **+3.55%**
  (+945 H/s). Exact filter: **28,328 H/s**, versus parent 27,331 H/s:
  **+3.65%** (+997 H/s).
- Median of the final 30 dual-worker reports: hardware **338,982 H/s**,
  software **28,145 H/s**, aggregate **367,128 H/s**.
- Versus the parent run, sustained software is **+3.93%** (+1,065 H/s) and
  aggregate is **+0.32%** (+1,163 H/s); hardware moved +97 H/s.
- Candidate RISC-V UF2 SHA-256:
  `bcae2f578ebfe4144a4bad3d6d92f1b31ccfa974c33f29d42a534447c2368cd9`.
- Archived serial log: `logs/E03-direct-header-digest-riscv.log`.
- Decision: retain on both architectures. It is neutral on M33 but materially
  faster on Hazard3, removes unnecessary source-level traffic, and preserves
  complete oracle equivalence.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=56000 elapsed_us=2031367 hash_rate_hs=27568 checksum=05 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=57000 elapsed_us=2012172 hash_rate_hs=28328 checksum=59f5c0dc temperature=disabled
```

## 2026-09-15 — E03 fused second-hash schedule, RISC-V rejection

- Experiment: `E03-fused-second-schedule-riscv-71`, candidate commit
  `eda2c61`, at the stock 150 MHz clock.
- Allocated the second-hash expanded schedule in the caller, wrote the first
  digest directly into W0..W7, and expanded it in place. This removes the
  digest-to-schedule copy and 32 bytes of nominal peak stack.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **26,968 H/s**, versus parent 27,568 H/s: **-2.18%**
  (-600 H/s). Exact filter: **27,674 H/s**, versus parent 28,328 H/s:
  **-2.31%** (-654 H/s).
- Median of the final 30 dual-worker reports: hardware **338,963 H/s**,
  software **27,494 H/s**, aggregate **366,457 H/s**.
- Versus the parent run, sustained software is **-2.31%** (-651 H/s) and
  aggregate is **-0.18%** (-671 H/s); hardware moved -19 H/s.
- Candidate RISC-V UF2 SHA-256:
  `5f3686e1143c0031a8f78c250cfc550c9d148df699496c2e78a26c939bb99681`.
- Archived serial log: `logs/E03-fused-second-schedule-riscv-rejected.log`.
- Decision: reject on Hazard3. The longer-lived expanded-schedule buffer
  causes worse generated-code behavior than the eliminated copy saves. Test
  M33 once to decide between a complete revert and an architecture split.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=54000 elapsed_us=2002372 hash_rate_hs=26968 checksum=61 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=56000 elapsed_us=2023541 hash_rate_hs=27674 checksum=fffebb38 temperature=disabled
```

## 2026-09-15 — E03 fused second-hash schedule, ARM architecture split

- Experiment: `E03-fused-second-schedule-arm-72`, candidate commit `eda2c61`,
  at the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **28,142 H/s**, versus parent 27,860 H/s: **+1.01%**
  (+282 H/s). Exact filter: **28,598 H/s**, versus parent 28,446 H/s:
  **+0.53%** (+152 H/s).
- Median of the final 30 dual-worker reports: hardware **325,706 H/s**,
  software **28,453 H/s**, aggregate **354,160 H/s**.
- Versus the parent run, sustained software is **+0.55%** (+155 H/s) and
  aggregate is **+0.045%** (+158 H/s); hardware moved +2 H/s.
- Candidate ARM UF2 SHA-256:
  `3212cb78915d9d6daef2c6cec1d178fa9751cc7c6c976b723d0cd0d391f1d228`.
- Archived serial log: `logs/E03-fused-second-schedule-arm.log`.
- Decision: retain the fused layout for M33 only. Restore the separate
  first-digest buffer on Hazard3, where the same layout was 2.31% slower.
  Verify architecture-split builds against the two already validated binary
  digests before continuing.
- Architecture-split verification: after correcting the internal buffer-size
  declaration that initially failed the M33 warning gate, both builds passed.
  The ARM UF2 remained exactly
  `3212cb78915d9d6daef2c6cec1d178fa9751cc7c6c976b723d0cd0d391f1d228`;
  the RISC-V UF2 exactly matched its pre-fusion winner,
  `bcae2f578ebfe4144a4bad3d6d92f1b31ccfa974c33f29d42a534447c2368cd9`.
  No new runtime measurement was needed because both loadable images are
  byte-identical to the artifacts validated above.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=57000 elapsed_us=2025433 hash_rate_hs=28142 checksum=47 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=58000 elapsed_us=2028113 hash_rate_hs=28598 checksum=db92efaa temperature=disabled
```

## 2026-09-15 — E09-c native-order high-word rejection, ARM

- Experiment: `E09c-native-high-word-arm-73`, candidate commit `c5634ac`, at
  the stock 150 MHz clock.
- The rejection helper now returns the final SHA word in its native big-endian
  numerical representation. Zero comparison is endian-invariant, so this
  removes a byte-reverse instruction from every filtered nonce. The oracle
  explicitly converts its expected representation; full digest output is
  unchanged.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **28,142 H/s**, unchanged. Exact filter:
  **28,609 H/s**, versus parent 28,598 H/s: **+0.038%** (+11 H/s).
- Median of the final 30 dual-worker reports: hardware **325,732 H/s**,
  software **28,464 H/s**, aggregate **354,196 H/s**.
- Versus the parent run, sustained software is **+0.039%** (+11 H/s) and
  aggregate is +36 H/s; hardware moved +26 H/s.
- Candidate ARM UF2 SHA-256:
  `78ca61fc81e22e8638d6337cc49cd07feef143209ef9db7c0945fa2d0ad5e278`.
- Archived serial log: `logs/E09c-native-high-word-arm.log`.
- Decision: provisionally retain as a semantically accurate low-complexity
  change; verify Hazard3 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=57000 elapsed_us=2025440 hash_rate_hs=28142 checksum=47 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=58000 elapsed_us=2027339 hash_rate_hs=28609 checksum=aaef92db temperature=disabled
```

## 2026-09-15 — E09-c native-order high-word rejection, RISC-V and retention

- Experiment: `E09c-native-high-word-riscv-74`, candidate commit `c5634ac`,
  at the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **27,613 H/s**. That path has no algorithmic change;
  its +0.16% movement is treated as layout/measurement variation.
- Exact filter: **28,500 H/s**, versus parent 28,328 H/s: **+0.61%**
  (+172 H/s).
- Median of the final 30 dual-worker reports: hardware **338,968 H/s**,
  software **28,314 H/s**, aggregate **367,282 H/s**.
- Versus the parent run, sustained software is **+0.60%** (+169 H/s) and
  aggregate is **+0.042%** (+154 H/s); hardware moved -14 H/s.
- Candidate RISC-V UF2 SHA-256:
  `b84299c1669a2688619055f4775e032369b22d93e1a4cf2c72d3892feecfe47e`.
- Archived serial log: `logs/E09c-native-high-word-riscv.log`.
- Decision: retain on both architectures. The representation is explicit,
  oracle-checked, and removes unnecessary work from every rejection.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=56000 elapsed_us=2028000 hash_rate_hs=27613 checksum=05 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=57000 elapsed_us=2000016 hash_rate_hs=28500 checksum=dcc0f559 temperature=disabled
```

## 2026-09-15 — E10 software FIFO polling every 64 nonces, RISC-V rejection

- Experiment: `E10-fifo-poll64-riscv-75`, candidate commit `a38a3d5`, at the
  stock 150 MHz clock.
- Replaced the core-0 FIFO status read after every software nonce with one read
  per 64 nonces. Expected worst-case control-message latency was about 2.3 ms.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Isolated full software: **27,613 H/s**; isolated filter: **28,500 H/s**.
  Both are unchanged from the parent, as expected because polling is outside
  the isolated benchmark.
- Median of the final 30 dual-worker reports: hardware **337,838 H/s**,
  software **28,325 H/s**, aggregate **366,164 H/s**.
- Versus the parent run, sustained software gains only **+0.039%** (+11 H/s),
  while hardware loses **-0.33%** (-1,130 H/s) and aggregate loses **-0.30%**
  (-1,118 H/s). The source/layout or control-timing interaction is harmful.
- Candidate RISC-V UF2 SHA-256:
  `661bafca0f6bf0c8eca8e84610abb7a94153a1fba55795cd137b3af2ff5540ba`.
- Archived serial log: `logs/E10-fifo-poll64-riscv-rejected.log`.
- Decision: reject and revert. The tiny software saving cannot justify a clear
  end-to-end loss; do not spend an ARM hardware run on this interval.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=56000 elapsed_us=2028005 hash_rate_hs=27613 checksum=05 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=57000 elapsed_us=2000006 hash_rate_hs=28500 checksum=dcc0f559 temperature=disabled
```

## 2026-09-15 — E09-b W16/W17 round addends, RISC-V

- Experiment: `E09b-round16-17-addends-riscv-76`, candidate commit `c3e2be2`,
  at the stock 150 MHz clock.
- Cached `K16 + W16` and `K17 + W17` once per header. The header-tail round
  loop is split around explicit rounds 16 and 17, which consume those cached
  addends while the unmodified W16/W17 remain in the expanded schedule.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **28,414 H/s**, versus parent 27,613 H/s: **+2.90%**
  (+801 H/s). Exact filter: **29,353 H/s**, versus parent 28,500 H/s:
  **+2.99%** (+853 H/s).
- Median of the final 30 dual-worker reports: hardware **338,974 H/s**,
  software **29,151 H/s**, aggregate **368,126 H/s**.
- Versus the parent run, sustained software is **+2.96%** (+837 H/s) and
  aggregate is **+0.23%** (+844 H/s); hardware moved +6 H/s.
- The gain is larger than the two eliminated additions predict, indicating
  that splitting the fully unrolled round region also improved Hazard3 code
  generation/layout.
- Candidate RISC-V UF2 SHA-256:
  `8466045ac4c31af0db85681439ca9848d194db421c48737adf0ce8ffe5da9d7d`.
- Archived serial log: `logs/E09b-round16-17-addends-riscv.log`.
- Decision: provisionally retain; verify M33 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=57000 elapsed_us=2006081 hash_rate_hs=28414 checksum=47 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=59000 elapsed_us=2009998 hash_rate_hs=29353 checksum=b0d24730 temperature=disabled
```

## 2026-09-15 — E09-b W16/W17 round addends, ARM and retention

- Experiment: `E09b-round16-17-addends-arm-77`, candidate commit `c3e2be2`,
  at the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **29,205 H/s**, versus parent 28,142 H/s: **+3.78%**
  (+1,063 H/s). Exact filter: **29,708 H/s**, versus parent 28,609 H/s:
  **+3.84%** (+1,099 H/s).
- Median of the final 30 dual-worker reports: hardware **325,658 H/s**,
  software **29,572 H/s**, aggregate **355,230 H/s**.
- Versus the parent run, sustained software is **+3.89%** (+1,108 H/s) and
  aggregate is **+0.29%** (+1,034 H/s); hardware moved -74 H/s.
- Candidate ARM UF2 SHA-256:
  `b679bf3ba58820dcd643206cc203f7143a14ebda60bbb5e364f1f9c24806410a`.
- Archived serial log: `logs/E09b-round16-17-addends-arm.log`.
- Decision: retain on both architectures. The explicit rounds plus cached
  addends improve compiler output materially while preserving all oracle and
  target behavior.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=59000 elapsed_us=2020196 hash_rate_hs=29205 checksum=50 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=60000 elapsed_us=2019653 hash_rate_hs=29708 checksum=a7578a11 temperature=disabled
```

## 2026-09-15 — E09-b fixed padding rounds in exact filter, ARM

- Experiment: `E09b-fixed-filter-rounds-arm-78`, candidate commit `90c7dc1`,
  at the stock 150 MHz clock.
- Specialized second-hash rounds 8 through 15 in the exact-filter helper.
  Round 8 consumes `K8 + 0x80000000`, rounds 9..14 consume only K, and round
  15 consumes `K15 + 256`; dynamic schedule loops remain before and after.
  Full-digest code is deliberately unchanged in this experiment.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on M33; no fault occurred.
- Full-digest software: **29,205 H/s**, unchanged from the parent. Exact
  filter: **30,718 H/s**, versus parent 29,708 H/s: **+3.40%** (+1,010 H/s).
- Median of the final 30 dual-worker reports: hardware **325,746 H/s**,
  software **30,554 H/s**, aggregate **356,300 H/s**.
- Versus the parent run, sustained software is **+3.32%** (+982 H/s) and
  aggregate is **+0.30%** (+1,070 H/s); hardware moved +88 H/s.
- Candidate ARM UF2 SHA-256:
  `288a620a953364cc0f44f3ae04c61d4fa51259fcd16e8ff511e9074af997c978`.
- Archived serial log: `logs/E09b-fixed-filter-rounds-arm.log`.
- Decision: provisionally retain; it is a clean filter-only gain. Verify
  Hazard3 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=59000 elapsed_us=2020192 hash_rate_hs=29205 checksum=50 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=62000 elapsed_us=2018357 hash_rate_hs=30718 checksum=78fd8ac1 temperature=disabled
```

## 2026-09-15 — E09-b fixed padding rounds in exact filter, RISC-V and retention

- Experiment: `E09b-fixed-filter-rounds-riscv-79`, candidate commit `90c7dc1`,
  at the stock 150 MHz clock.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **28,484 H/s**. This path is source-unchanged; its
  +0.25% movement is treated as layout/measurement variation.
- Exact filter: **29,909 H/s**, versus parent 29,353 H/s: **+1.89%**
  (+556 H/s).
- Median of the final 30 dual-worker reports: hardware **339,024 H/s**,
  software **29,706 H/s**, aggregate **368,729 H/s**.
- Versus the parent run, sustained software is **+1.90%** (+555 H/s) and
  aggregate is **+0.16%** (+603 H/s); hardware moved +50 H/s.
- Candidate RISC-V UF2 SHA-256:
  `e67af4e1731d54b24dd8d8b600c3dd420f731f4cae913b8222dcd937335f75c4`.
- Archived serial log: `logs/E09b-fixed-filter-rounds-riscv.log`.
- Decision: retain on both architectures. This filter-only specialization is
  oracle-equivalent and improves both isolated and sustained filtered work.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=57000 elapsed_us=2001137 hash_rate_hs=28484 checksum=47 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=60000 elapsed_us=2006062 hash_rate_hs=29909 checksum=a7578a11 temperature=disabled
```

## 2026-09-15 — E09-b fixed padding rounds in full digest, RISC-V

- Experiment: `E09b-fixed-full-rounds-riscv-80`, candidate commit `1a1ea93`,
  at the stock 150 MHz clock.
- Applied the same fixed second-hash round 8..15 specialization to the complete
  digest helper. The exact-filter helper is source-unchanged in this attempt.
- Both architectures built warning-free. All 4,096 oracle cases and all 7
  suites passed on Hazard3; no fault occurred.
- Full-digest software: **28,636 H/s**, versus parent 28,484 H/s: **+0.53%**
  (+152 H/s). Exact filter: **30,041 H/s**, versus parent 29,909 H/s:
  +0.44% (+132 H/s), attributed primarily to favorable code layout because
  its helper did not change.
- Median of the final 30 dual-worker reports: hardware **338,998 H/s**,
  software **29,835 H/s**, aggregate **368,834 H/s**.
- Versus the parent run, sustained software is +129 H/s and aggregate +105
  H/s; hardware moved -26 H/s.
- Candidate RISC-V UF2 SHA-256:
  `c5406a6a3a65151ae332f1b168e06b4cf520cf4c5b47650ee0c7c3654bc72e10`.
- Archived serial log: `logs/E09b-fixed-full-rounds-riscv.log`.
- Decision: provisionally retain for the measured full-digest gain; verify
  M33 before final retention.

```text
TEST:SUMMARY pass=7 fail=0
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=58000 elapsed_us=2025420 hash_rate_hs=28636 checksum=64 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=61000 elapsed_us=2030550 hash_rate_hs=30041 checksum=b2bf5392 temperature=disabled
```

## 2026-09-15 — E09-b fixed padding rounds in full digest, ARM and retention

- Experiment: `E09b-fixed-full-rounds-arm-81`, source candidate commit
  `1a1ea93`, at the stock 150 MHz clock. Repository HEAD also contains later
  evidence/documentation commits but no later firmware-source change.
- Before flashing, ARM (`rp2350-arm-s`, toolchain `15_2_Rel1`) and RISC-V
  (`rp2350-riscv`, toolchain `RISCV_PICO_2_3_1_0`) both built warning-free
  against Pico SDK 2.3.1.
- All 4,096 oracle cases and all 7 suites passed on M33; no fault occurred.
- Full-digest software: **30,012 H/s**, versus parent experiment 78's
  29,205 H/s: **+2.76%** (+807 H/s). Exact filter: **30,718 H/s**, identical
  to the parent as expected because its helper is source-unchanged.
- Median of the final 30 dual-worker reports: hardware **325,746 H/s**,
  software **30,554 H/s**, aggregate **356,300 H/s**. These equal the parent
  experiment's recorded medians; the mining worker uses the filter helper, so
  the full-digest improvement is not expected to raise hard-target steady
  mining throughput.
- Hardware startup benchmark: **330,357 H/s**. Candidate ARM UF2 SHA-256:
  `370a97545aa3f78b9627d2dba98bf437e22128c15c2abc760f2eddc2bdc0b81a`
  (363,520 bytes). Paired candidate RISC-V UF2 SHA-256 remains
  `c5406a6a3a65151ae332f1b168e06b4cf520cf4c5b47650ee0c7c3654bc72e10`
  (395,776 bytes).
- Fixture SHA-256:
  `4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98`.
  Temperature remained explicitly disabled.
- Archived serial log: `logs/E09b-fixed-full-rounds-arm.log`.
- Decision: retain on both architectures. The complete-digest helper gains
  2.76% on ARM and 0.53% on RISC-V with exact oracle equivalence, while the
  source-unchanged mining/filter path does not regress on ARM.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=661000 elapsed_us=2000868 hash_rate_hs=330357 checksum=6f temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=61000 elapsed_us=2032553 hash_rate_hs=30012 checksum=55 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=62000 elapsed_us=2018360 hash_rate_hs=30718 checksum=78fd8ac1 temperature=disabled
```

## 2026-09-15 — E01 identity contract candidate definition

- Experiment: `E01-identity-82`, parent artifact/source `b6a3b97`, stock
  150 MHz, shared ARM/RISC-V instrumentation change.
- Hypothesis: embedding the committed source identity and warm-reset run ID,
  then requiring ordered seven-suite, hardware/full/filter benchmark, mining
  start and sequenced-progress records will reject stale, partial, reset and
  wrong-image captures without changing either hashing loop.
- Changed feature: BOOT source/run fields, run-tagged sequenced progress,
  expected architecture/source arguments in `tools/cycle`, and a strict host
  validation state machine with synthetic negative tests. Default cycle
  capture becomes 45 seconds so the complete contract can finish.
- Expected removable cost: none; this removes measurement ambiguity rather
  than hash work. Resource cost is two watchdog scratch words, two retained
  identity words, report-format bytes, and host parser complexity. No
  temperature acquisition or clock change is included.
- Rejection rule: reject or revise if synthetic failures are accepted, either
  ISA misses/misorders required output, any oracle/fault gate fails, or the
  report-only firmware change materially reduces sustained throughput versus
  experiments 80/81.

### E01 identity ARM attempt 82a — flash completion failure

- Candidate `c89e8ee` built warning-free for both architectures at 150 MHz.
  The ARM upload reached 100% verification, then `picotool load -f -u -v -x`
  aborted with `picoboot::connection_error` while completing/rebooting. The
  cycle exited nonzero before serial capture, so this attempt is **failed**
  and provides no validation or performance result.
- Immediate autonomous diagnosis: `./tools/doctor` passed and found the Pico
  runtime USB serial device at `/dev/ttyACM0`, consistent with a transient
  disconnect after a verified upload. The full cycle will be retried rather
  than treating the verified flash alone as success.
- Archived failure record: `logs/E01-identity-arm-flash-failure.log`.

Before retry, review found that candidate `c89e8ee` derived `source_id` from
repository HEAD, so an evidence-only commit would change the next firmware
identity. The candidate is revised to hash the committed `CMakeLists.txt`,
`src`, and `tools` trees and append `-dirty` only for relevant tracked changes.
This preserves identity across log/ledger-only commits; no result from 82a is
promoted by this correction.

### E01 identity ARM attempt 82b — pass

- Revised candidate commit `c2cf4d0`, stable firmware source identity
  `ebabf96f233d`, stock 150 MHz, temperature disabled. Both architectures
  built warning-free before the ARM flash.
- The strict host contract accepted exactly one BOOT with expected ARM/source
  identity, all 7 suites (including 4,096 oracle cases), all three benchmark
  stages, matching mining run ID, and **114 contiguous progress records**.
  No `TEST:FAIL`, `FAULT`, unexpected reset, missing stage, or sequence gap
  occurred.
- Full software **30,012 H/s**; exact filter **30,718 H/s**. Median of the
  final 30 reports: hardware **325,769 H/s**, software **30,540 H/s**,
  aggregate **356,309 H/s**. Versus experiment 81, hardware is +23 H/s,
  software -14 H/s and aggregate +9 H/s: no material report-only regression.
- Hardware startup benchmark: **330,357 H/s**. ARM UF2 SHA-256:
  `f65303383ccb26b45de9342b790b0bada8ca5c0f42d3d0a99fff769f980ef3b0`.
  Paired unflashed RISC-V UF2 SHA-256:
  `ae141f1c9ccfba07e9608d7c9f743203cc94798898f9d8dbf142839e73f3a4fe`.
- Run ID `30004927-00000002`; fixture SHA-256
  `4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98`.
  Archived log: `logs/E01-identity-arm.log`.
- Decision: ARM passes; retain provisionally pending paired RISC-V hardware
  validation.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=661000 elapsed_us=2000865 hash_rate_hs=330357 checksum=6f temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=61000 elapsed_us=2032554 hash_rate_hs=30012 checksum=55 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=62000 elapsed_us=2018362 hash_rate_hs=30718 checksum=78fd8ac1 temperature=disabled
```

### E01 identity RISC-V attempt 82c — pass and retention

- Candidate commit `c2cf4d0`, firmware source identity `ebabf96f233d`, stock
  150 MHz, temperature disabled. The cycle rebuilt both architectures
  warning-free before flashing the paired RISC-V artifact.
- The strict contract accepted one expected RISC-V BOOT, all 7 suites and
  4,096 oracle cases, all benchmark stages, matching run IDs and **118
  contiguous progress records**. No failure, reset, partial record, or
  sequence gap occurred.
- Full software **28,631 H/s**; exact filter **30,041 H/s**. Median final-30:
  hardware **339,033 H/s**, software **29,804 H/s**, aggregate **368,837
  H/s**. Versus experiment 80, hardware is +35 H/s, software -31 H/s and
  aggregate +3 H/s; isolated full moves -5 H/s and filter is identical.
  These are immaterial layout/reporting movements.
- Hardware startup benchmark: **343,997 H/s**. RISC-V UF2 SHA-256:
  `ae141f1c9ccfba07e9608d7c9f743203cc94798898f9d8dbf142839e73f3a4fe`.
  Paired ARM UF2 SHA-256:
  `f65303383ccb26b45de9342b790b0bada8ca5c0f42d3d0a99fff769f980ef3b0`.
- Run ID `30004927-00000003`; fixture SHA-256
  `4cf1f1db9d05f9208b74edec0f616497a286269e73a1e89747fed60ac5648f98`.
  Archived log: `logs/E01-identity-riscv.log`.
- Decision: retain the identity contract on both architectures. It now rejects
  stale/wrong/partial sessions in synthetic tests and accepted complete paired
  hardware captures without a material throughput regression. This closes
  E01-identity's core capture requirements; common-window and rare-path work
  remain separate experiments.

```text
TEST:SUMMARY pass=7 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=688000 elapsed_us=2000020 hash_rate_hs=343997 checksum=c8 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=58000 elapsed_us=2025803 hash_rate_hs=28631 checksum=64 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=61000 elapsed_us=2030546 hash_rate_hs=30041 checksum=b2bf5392 temperature=disabled
```

## 2026-09-15 — E01 rare mining-decision candidate definition

- Experiment: `E01-rare-83`, parent firmware source identity `ebabf96f233d`
  (`c2cf4d0` implementation), stock 150 MHz, shared ARM/RISC-V test-path
  change.
- Hypothesis: factoring the production software fast-reject/full-digest
  fallback into one always-inlined decision helper permits direct validation
  of the real mining path without changing generated hot-loop work. A known
  adjacent loser exercises no-full-digest rejection; the genesis winner
  exercises fallback, full comparison and candidate publication data.
- Coverage adds hardware high-word equality with lower-word reject, exact
  equality and above-target acceptance; software fast rejection, valid-share,
  lower-word rejection, exact equality, and nonzero-high-target general path.
  The strict host parser independently checks the emitted genesis nonce/hash.
- Expected removable cost: none; this is a correctness gate. Resource cost is
  startup-only test code/data and one extra KAT record. Temperature remains
  disabled and the clock remains 150 MHz.
- Rejection rule: reject/revise on any path mismatch, missing full-digest
  fallback, incorrect host candidate, build warning, strict-cycle failure, or
  material sustained regression on either ISA.

### E01 rare ARM attempt 83a — pass

- Candidate commit `1a3f619`, firmware source identity `6826dc1fdfe2`, stock
  150 MHz, temperature disabled. Both architectures built warning-free before
  flashing ARM.
- All 8 suites passed, including 4,096 oracle cases and 8 direct mining
  decision cases. Nonce 2083236892 was rejected by high word `3d34dc8c`
  without computing the full digest. Nonce 2083236893 exercised full fallback,
  exact ordered comparison, and produced the host-verified genesis hash.
  Hardware equal-high/lower-reject, exact-equality and above-target cases also
  passed. No fault or capture-contract violation occurred.
- Full software **30,017 H/s**; filter **30,718 H/s**. Final-30 medians:
  hardware **325,809 H/s**, software **30,516 H/s**, aggregate **356,326
  H/s**. Versus E01-identity ARM, these move +5, 0, +40, -24, and +17 H/s
  respectively, with no material regression.
- Hardware startup benchmark **330,359 H/s**. ARM UF2 SHA-256:
  `b98f191af6f0de490a9d54fff3ff67c38571f742db981d08bf277e8e6aeb01c1`.
  Paired RISC-V UF2 SHA-256:
  `086c113cc64e5e4313b73ab5e49ec3e4f9489cb24102f346dedbe282e02ccf2b`.
- Run ID `30004927-00000004`; archived log `logs/E01-rare-arm.log`.
  Decision: ARM passes; retain provisionally pending RISC-V validation.

```text
TEST:PASS kat=mining_decision_paths cases=8 rejected_nonce=2083236892 rejected_high_word=3d34dc8c candidate_nonce=2083236893 candidate_hash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
TEST:SUMMARY pass=8 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=661000 elapsed_us=2000854 hash_rate_hs=330359 checksum=6f temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=61000 elapsed_us=2032155 hash_rate_hs=30017 checksum=55 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=62000 elapsed_us=2018360 hash_rate_hs=30718 checksum=78fd8ac1 temperature=disabled
```

### E01 rare RISC-V attempt 83b — pass and retention

- Candidate commit `1a3f619`, firmware source identity `6826dc1fdfe2`, stock
  150 MHz, temperature disabled. Both architectures rebuilt warning-free.
- All 8 suites passed, including the same 8 direct production mining-decision
  cases and host-verified genesis candidate as ARM. The deterministic loser
  used high word `3d34dc8c` and did not invoke full digest fallback; all target
  equality/lower/general paths passed. No fault or capture-contract failure.
- Full software **28,636 H/s**; filter **30,041 H/s**. Final-30 medians:
  hardware **339,114 H/s**, software **29,811 H/s**, aggregate **368,925
  H/s**. Versus E01-identity RISC-V these move +5, 0, +81, +7, and +88 H/s,
  so sustained mining does not regress.
- Hardware startup benchmark **339,321 H/s**, versus 343,997 in the identity
  run. The hardware algorithm is source-unchanged and 339.3 kH/s matches the
  earlier E09 range; record this as a code-layout sensitivity, not a claimed
  algorithm gain or hidden sustained regression.
- RISC-V UF2 SHA-256:
  `086c113cc64e5e4313b73ab5e49ec3e4f9489cb24102f346dedbe282e02ccf2b`;
  paired ARM UF2 SHA-256:
  `b98f191af6f0de490a9d54fff3ff67c38571f742db981d08bf277e8e6aeb01c1`.
  Run ID `30004927-00000005`; archived log `logs/E01-rare-riscv.log`.
- Decision: retain on both architectures. E01-rare now directly covers the
  actual software mining decision/fallback and hardware comparator branches;
  the common-window and lifecycle portions of E01 remain open.

```text
TEST:PASS kat=mining_decision_paths cases=8 rejected_nonce=2083236892 rejected_high_word=3d34dc8c candidate_nonce=2083236893 candidate_hash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
TEST:SUMMARY pass=8 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=679000 elapsed_us=2001056 hash_rate_hs=339321 checksum=32 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=58000 elapsed_us=2025412 hash_rate_hs=28636 checksum=64 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=61000 elapsed_us=2030541 hash_rate_hs=30041 checksum=b2bf5392 temperature=disabled
```

## 2026-09-15 — E01 common-window candidate definition

- Experiment: `E01-window-84`, parent source identity `6826dc1fdfe2`
  (`1a3f619` firmware), stock 150 MHz, shared ARM/RISC-V measurement change.
- Hypothesis: a start handshake and cross-core barrier every 16 hardware
  progress intervals can snapshot both unique worker counts over one common
  approximately 4.7-second wall interval. The resulting total includes FIFO,
  USB printing, stalls and barrier idle time and removes the current mixed
  recent-hardware/cumulative-software timing ambiguity.
- Changed feature: READY/ACK protocol, exact count/time deltas, and
  `MEASUREMENT:WINDOW` records. Ordinary 100,000-hash progress remains for
  historical comparison. The strict parser verifies identity, sequence,
  counts and recomputed rates and requires at least five windows.
- Expected removable cost: measurement ambiguity only. Resource cost is two
  FIFO control words per common boundary, several counters on core 0, and
  report code; no inner hash calculation changes. Temperature stays disabled
  and clock stays at 150 MHz.
- Rejection rule: reject/revise for deadlock, protocol fault, non-unique or
  inconsistent counts, fewer than five complete windows, any validation
  failure, or a material useful-work regression. Compare future candidates
  against the new common-window baseline rather than summing mixed rates.

### E01 common-window ARM attempt 84a — pass

- Candidate commit `1839955`, firmware source identity `f2687597e485`, stock
  150 MHz, temperature disabled. Both ARM and RISC-V wrapper builds passed
  without warnings before the ARM flash.
- Strict capture passed all 8 suites, including 4,096 oracle cases and the
  production mining-decision rare paths. Run ID `30004927-00000006`; no
  `TEST:FAIL`, `FAULT`, timeout, identity mismatch, or protocol failure.
- Seven synchronized windows completed at sequences 16 through 112. Each
  covered 1,600,000 hardware hashes and 150,278–150,301 software hashes over
  4,921,449–4,921,641 us. Aggregate rates were **355,630–355,643 H/s**, with
  median **355,640 H/s**; hardware median **325,104 H/s** and software median
  **30,535 H/s**. Counts summed exactly and independently recomputed rates
  matched every record.
- Standalone hardware **330,356 H/s**, full software **30,017 H/s**, and
  software filter **30,718 H/s**. The prior E01-rare mixed-window final-30
  figures were 325,809 / 30,516 / 356,326 H/s; they are retained only as a
  historical sanity check because their time bases differ.
- ARM UF2 SHA-256:
  `2a1857e60441d9b9ef6239e58628de19ddc33623a0df2cc097f981e756d8d6d9`.
  Archived complete log: `logs/E01-window-arm.log`.
- Decision: ARM passes and establishes its common-window baseline. Retain
  provisionally pending the required RISC-V run of the identical source.

```text
TEST:SUMMARY pass=8 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=batched-accounting-e04c arch=ARM-M33 clock_hz=150000000 hashes=661000 elapsed_us=2000870 hash_rate_hs=330356 checksum=6f temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=ARM-M33 clock_hz=150000000 hashes=61000 elapsed_us=2032148 hash_rate_hs=30017 checksum=55 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=ARM-M33 clock_hz=150000000 hashes=62000 elapsed_us=2018356 hash_rate_hs=30718 checksum=78fd8ac1 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=1 sequence=16 elapsed_us=4921611 hardware_hashes=1600000 software_hashes=150301 total_hashes=1750301 hardware_rate_hs=325097 software_rate_hs=30539 hash_rate_hs=355636 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=2 sequence=32 elapsed_us=4921502 hardware_hashes=1600000 software_hashes=150279 total_hashes=1750279 hardware_rate_hs=325104 software_rate_hs=30535 hash_rate_hs=355639 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=3 sequence=48 elapsed_us=4921641 hardware_hashes=1600000 software_hashes=150285 total_hashes=1750285 hardware_rate_hs=325095 software_rate_hs=30536 hash_rate_hs=355630 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=4 sequence=64 elapsed_us=4921449 hardware_hashes=1600000 software_hashes=150278 total_hashes=1750278 hardware_rate_hs=325108 software_rate_hs=30535 hash_rate_hs=355643 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=5 sequence=80 elapsed_us=4921481 hardware_hashes=1600000 software_hashes=150282 total_hashes=1750282 hardware_rate_hs=325105 software_rate_hs=30536 hash_rate_hs=355641 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=6 sequence=96 elapsed_us=4921497 hardware_hashes=1600000 software_hashes=150280 total_hashes=1750280 hardware_rate_hs=325104 software_rate_hs=30535 hash_rate_hs=355640 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000006 window=7 sequence=112 elapsed_us=4921556 hardware_hashes=1600000 software_hashes=150280 total_hashes=1750280 hardware_rate_hs=325100 software_rate_hs=30535 hash_rate_hs=355635 temperature=disabled
```

### E01 common-window RISC-V attempt 84b — pass and retention

- Candidate commit `1839955`, firmware source identity `f2687597e485`, stock
  150 MHz, temperature disabled. Both architectures were rebuilt successfully
  from the same source immediately before flashing RISC-V.
- Strict capture passed all 8 suites and 4,096 oracle cases. Run ID
  `30004927-00000007`; no validation, identity, sequencing, timeout, protocol,
  or fault condition occurred.
- Seven synchronized windows completed at sequences 16 through 112. Each
  covered 1,600,000 hardware hashes and 141,885–141,912 software hashes over
  4,752,000–4,752,171 us. Aggregate rates were **366,550–366,560 H/s**, with
  median **366,556 H/s**; hardware median **336,697 H/s** and software median
  **29,860 H/s**. Counts summed exactly and independently recomputed rates
  matched every record.
- Standalone hardware **343,994 H/s**, full software **28,636 H/s**, and
  software filter **30,041 H/s**. Against E01-rare's mixed-window final-30
  figures (339,114 / 29,811 / 368,925 H/s), the new medians differ by -0.71%,
  +0.16%, and -0.64%. That is not an A/B comparison because the old aggregate
  mixed recent hardware and cumulative software time bases; it is recorded as
  a layout/measurement-overhead watch item rather than a demonstrated mining
  regression. The unchanged isolated hardware path returns to 343,994 H/s,
  essentially identical to E01-identity's 343,997 H/s.
- RISC-V UF2 SHA-256:
  `238f314f3012188082c35a2324c0b9f74bee42adeeffb010c8fc1b086e702ac4`;
  paired ARM UF2 SHA-256:
  `2a1857e60441d9b9ef6239e58628de19ddc33623a0df2cc097f981e756d8d6d9`.
  Archived complete log: `logs/E01-window-riscv.log`.
- Decision: retain the synchronized measurement contract on both ISAs. It
  produces seven stable, internally reconciled common-time measurements per
  45-second capture and passes all correctness gates. These ARM and RISC-V
  medians are the baseline for future candidate comparisons; do not compare a
  future common-window result directly with the historical mixed aggregate.

```text
TEST:SUMMARY pass=8 fail=0
BENCHMARK:PASS algorithm=bitcoin-double-sha256 engine=RP2350-SHA256 path=persistent-first-block-dma-e06a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=688000 elapsed_us=2000036 hash_rate_hs=343994 checksum=c8 temperature=disabled
SOFTWARE_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=portable-midstate-e09a arch=RISCV-HAZARD3 clock_hz=150000000 hashes=58000 elapsed_us=2025404 hash_rate_hs=28636 checksum=64 temperature=disabled
SOFTWARE_FILTER_BENCHMARK:PASS algorithm=bitcoin-double-sha256 path=exact-round61-high-word arch=RISCV-HAZARD3 clock_hz=150000000 hashes=61000 elapsed_us=2030534 hash_rate_hs=30041 checksum=b2bf5392 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=1 sequence=16 elapsed_us=4752101 hardware_hashes=1600000 software_hashes=141912 total_hashes=1741912 hardware_rate_hs=336693 software_rate_hs=29863 hash_rate_hs=366556 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=2 sequence=32 elapsed_us=4752074 hardware_hashes=1600000 software_hashes=141896 total_hashes=1741896 hardware_rate_hs=336695 software_rate_hs=29860 hash_rate_hs=366555 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=3 sequence=48 elapsed_us=4752031 hardware_hashes=1600000 software_hashes=141885 total_hashes=1741885 hardware_rate_hs=336698 software_rate_hs=29858 hash_rate_hs=366556 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=4 sequence=64 elapsed_us=4752007 hardware_hashes=1600000 software_hashes=141896 total_hashes=1741896 hardware_rate_hs=336700 software_rate_hs=29860 hash_rate_hs=366560 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=5 sequence=80 elapsed_us=4752000 hardware_hashes=1600000 software_hashes=141893 total_hashes=1741893 hardware_rate_hs=336700 software_rate_hs=29860 hash_rate_hs=366560 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=6 sequence=96 elapsed_us=4752046 hardware_hashes=1600000 software_hashes=141896 total_hashes=1741896 hardware_rate_hs=336697 software_rate_hs=29860 hash_rate_hs=366557 temperature=disabled
MEASUREMENT:WINDOW run_id=30004927-00000007 window=7 sequence=112 elapsed_us=4752171 hardware_hashes=1600000 software_hashes=141910 total_hashes=1741910 hardware_rate_hs=336688 software_rate_hs=29862 hash_rate_hs=366550 temperature=disabled
```

## 2026-09-15 — E02 fresh profiling candidate definition

- Experiment: `E02-profile-85`, parent source identity `f2687597e485`, stock
  150 MHz, temperature disabled, compile-time `MINER_PROFILE` diagnostic mode.
- Static platform check: RP2350 M33 declares `DWT_CTRL.NOCYCCNT=1` and
  `NOPRFCNT=1`, so the plan's preferred DWT cycle/performance counters are not
  implemented. The diagnostic uses interrupt-free 24-bit SysTick core-clock
  deltas instead and reports the live DWT capability bits. Hazard3 uses its
  supported `mcycle` and `minstret`; its extended HPM counters are documented
  as hardwired zero. Both ISAs also sample the global XIP hit/access counters.
- The profile build times 4,096 nonces and separates hardware setup/start,
  first-block feed, tail feed plus first-digest wait, digest handoff plus second
  hash, and final target/error check. It separately times software header-tail
  compression and second-hash full/filter stages. Every record is explicitly
  marked `intrusive=1`; these stage totals are diagnostic and will not replace
  uninstrumented common-window rates.
- Normal (`MINER_PROFILE=0`) and profiling (`MINER_PROFILE=1`) builds pass for
  both architectures with warnings as errors; 8 host monitor tests pass.
  Profile image sizes are ARM text 190,792 / BSS 4,708 bytes and RISC-V text
  203,168 / BSS 4,440 bytes. The approximately 2.2–2.4 KiB diagnostic text is
  compiled out of normal images.
- Hardware acceptance: complete KAT and strict capture must still pass, counter
  capability/overhead must be reported, profile totals must be internally
  plausible against end-to-end elapsed time, and the normal post-profile
  benchmark/common windows must remain functional. Archive each full log.

### E02 ARM SysTick attempt 85a — functional pass, profiler revision required

- Candidate commit `9181c33`, source identity `5559d0ea81c1`, profile=1,
  stock 150 MHz, temperature disabled. Both profile architectures built
  warning-free before flashing. All 8 test suites, standalone benchmarks, and
  seven common windows passed; no fault or capture failure occurred.
- The live ARM register was `DWT_CTRL=0x40000000`, meaning `NOCYCCNT=0` and
  `NOPRFCNT=0`. This contradicts the generated register header's reset-value
  annotations and establishes that the connected silicon exposes DWT counters.
  The SysTick measurement is retained as evidence, but the ARM profiler must
  be revised to use the plan-preferred live DWT counter before final E02 use.
- SysTick read interval overhead averaged 11.527 cycles. Intrusive hardware
  totals over 4,096 nonces: setup/start 53,532 (**13.07/hash**), first feed
  336,408 (**82.13/hash**), tail feed plus first-digest wait 797,484
  (**194.70/hash**), digest handoff plus second hash 709,779 (**173.29/hash**),
  and target/error/check 259,536 (**63.36/hash**). The staged sum is 526.55
  cycles/hash; elapsed wall time 15,121 us corresponds to 553.75 cycles/hash,
  leaving about 27.20 cycles/hash in loop/timer/instrumentation overhead.
- Intrusive software filter: header tail 9,810,138 (**2,395.05/hash**) and
  second filter 10,216,184 (**2,494.19/hash**), with 133,922 us end-to-end
  (**4,904.37 cycles/hash**). Full digest: header tail 9,806,478
  (**2,394.16/hash**) and second full 10,536,721 (**2,572.44/hash**), with
  136,101 us end-to-end (**4,984.17 cycles/hash**). This points to a nearly
  even first/second software split; the exact filter saves about 78.25
  measured second-stage cycles/hash versus full output.
- XIP hit/access observations: hardware 902,323/902,443; filter
  685,447/685,473; full 693,593/693,612. These are global request counters,
  not stall-cycle counters; the near-total hit ratio does not prove zero fetch
  contention.
- Uninstrumented post-profile paths: hardware 330,359 H/s, full software
  30,017 H/s, filter 30,718 H/s. Common-window aggregate range
  355,513–355,525 H/s, median **355,519 H/s**, only -0.034% from the retained
  ARM 355,640 H/s baseline; the dormant profile facility did not materially
  disrupt normal execution.
- ARM profile UF2 SHA-256:
  `138b3860133cb2c7437aca3241e77e5b44311fe4bb2907ea840bcf6c079c5726`.
  Paired RISC-V profile UF2 at the same candidate:
  `0a6536526714278c5624759cd8c0dffb2a4699bca7558550a18ba32b4a6ffdcc`.
  Run ID `30004927-00000008`; archived log
  `logs/E02-profile-arm-systick.log`.
- Decision: preserve the measurement as a successful exploratory attempt, but
  do not call the ARM counter choice complete. Revise only the counter backend
  to DWT and repeat ARM before advancing to the paired RISC-V profile.

### E02 ARM DWT revision candidate 85b

- Parent `9181c33` plus the recorded attempt-85a evidence commit `89bf7b7`.
  The only firmware change replaces ARM's SysTick fallback with the live DWT
  `CYCCNT`: set `DEMCR.TRCENA`, clear `DWT_CYCCNT`, set
  `DWT_CTRL.CYCCNTENA`, and issue DSB/ISB before sampling. Hazard3 profiling
  and all measured algorithms are source-unchanged.
- Both profile-enabled wrapper builds pass with warnings as errors; all 8 host
  monitor tests pass. Temperature remains disabled and clock remains stock
  150 MHz.
- Acceptance: live DWT counter increments, capability bits remain present,
  stage totals agree reasonably with the independent microsecond wall time,
  and the complete strict ARM hardware capture passes. This is a counter
  backend correction, not a hash optimization claim.

#### E02 ARM DWT attempt 85b — flash completion failure

- Candidate commit `30a6fd2`, source identity `02b7825c7499`, profile=1,
  stock 150 MHz, temperature disabled. Both profile architectures completed
  warning-free builds before flashing.
- Picotool loaded and verified the ARM image to 100%, then aborted with
  `picoboot::connection_error` while completing the reboot. The cycle returned
  status 1 before serial monitoring began; therefore there is no BOOT,
  validation, profile, or benchmark measurement from this attempt.
- Decision: hardware attempt failed and cannot support the DWT revision.
  Preserve the failure, make no source/clock change, and retry the identical
  committed candidate. Archived terminal record:
  `logs/E02-profile-arm-dwt-flash-failure.log`.

#### E02 ARM DWT retry 85c — pass

- Retried the identical candidate commit `30a6fd2`, source identity
  `02b7825c7499`, profile=1, stock 150 MHz, temperature disabled. Both profile
  architectures built warning-free before flashing. Run ID
  `30004927-0000000a` passed all 8 test suites, the profile workloads,
  standalone benchmarks, seven aligned common windows, and strict capture;
  no fault occurred.
- The live counter is the plan-preferred M33 DWT `CYCCNT`:
  `DWT_CTRL=0x40000001`, `NOCYCCNT=0`, `NOPRFCNT=0`, with an observed read
  interval overhead of **9.347 cycles**.
- Intrusive hardware totals over 4,096 nonces: setup/start 53,582
  (**13.08 cycles/hash**), first-block feed 337,002 (**82.28**), tail feed plus
  first-digest wait 795,192 (**194.14**), digest handoff plus second hash
  710,931 (**173.57**), and target/error check 226,575 (**55.32**). The stage
  sum is **518.38 cycles/hash**; 14,897 us wall time is **545.54 cycles/hash**,
  leaving 27.17 cycles/hash for loop, timing, and unclassified overhead.
- Intrusive software filter: header tail 9,812,984 (**2,395.75 cycles/hash**)
  and second filter 10,207,458 (**2,492.06**), stage sum **4,887.80** and wall
  time **4,900.23 cycles/hash**. Full digest: header tail 9,810,248
  (**2,395.08**) and second full 10,530,809 (**2,571.98**), stage sum
  **4,967.05** and wall time **4,981.70 cycles/hash**. The filter saves about
  **79.92 cycles/hash** versus full output in the measured second stage (3.11%
  of that stage, 1.61% of the full two-stage cost).
- XIP hit/access observations: hardware 881,960/882,063; filter
  677,760/677,800; full 697,695/697,716. These global counters show very high
  hit ratios but do not measure stall cycles.
- Uninstrumented post-profile benchmarks were hardware **330,360 H/s**, full
  software **30,017 H/s**, and filter **30,718 H/s**. The seven aligned common
  windows had aggregate range **355,464–355,476 H/s**, median
  **355,468 H/s**; hardware median **324,942 H/s** and software median
  **30,527 H/s**. The aggregate is 172 H/s (-0.048%) below the retained ARM
  parent median 355,640 H/s, so diagnostic mode did not materially perturb the
  normal mining paths.
- ARM profile UF2 SHA-256:
  `cbae3ae9973d1ecf272a25bee657d4fc3371c27322bd59f38dc7726dea181fbd`.
  Paired RISC-V profile UF2 SHA-256:
  `5c658609dcdf811dc617de65f9645a033c74ec4f453781e5637fe28c5e102df4`.
  Complete log: `logs/E02-profile-arm-dwt.log`.
- Decision: accept the corrected ARM profile evidence. The main hardware
  opportunity is the 194-cycle tail/wait segment, followed by the 174-cycle
  second hash; proceed to paired Hazard3 profiling before selecting a code
  experiment.

### E02 Hazard3 profile attempt 85d — pass

- Candidate commit `30a6fd2` plus evidence-only commits, source identity
  `02b7825c7499`, profile=1, stock 150 MHz, temperature disabled. Both profile
  architectures built warning-free before flash. Run ID
  `30004927-0000000b` passed all 8 test suites, profile workloads, standalone
  benchmarks, seven common windows, and strict capture with no fault.
- Hazard3 used `mcycle` and `minstret`; measured read-interval overhead was
  **17.707 cycles** for the cycle counter and **15.972 instructions** for the
  retired-instruction counter. Extended HPM counters are hardwired zero.
- Intrusive hardware totals over 4,096 nonces: setup/start 38,016
  (**9.28 cycles/hash**), first-block DMA feed 356,404 (**87.01**), tail feed
  plus first-digest wait 798,232 (**194.88**), digest handoff plus second hash
  675,730 (**164.97**), and target/error check 108,485 (**26.49**). The stage
  sum is **482.63 cycles/hash**; 14,444 us wall time is **528.96 cycles/hash**,
  leaving 46.32 cycles/hash in loop, measurement, and unclassified overhead.
  The whole measured hardware path retired 942,448 instructions, or
  **230.09 instructions/hash**.
- Intrusive software filter: header tail 9,835,560 (**2,401.26 cycles/hash**)
  and second filter 11,349,523 (**2,770.88**), stage sum **5,172.14** and wall
  time **5,192.47 cycles/hash**. Full digest: header tail 9,829,868
  (**2,399.87**) and second full 12,135,189 (**2,962.69**), stage sum
  **5,362.56** and wall time **5,383.70 cycles/hash**. Full/filter workloads
  retired 21,757,023/21,069,063 instructions, or about 5,311.77/5,143.81 per
  hash. The exact filter saves **191.81 second-stage cycles/hash** (6.47% of
  that stage, 3.58% of the full two-stage cost).
- XIP hit/access observations: hardware 1,146,035/1,146,136; filter
  1,129,388/1,129,439; full 1,148,614/1,148,655. They show a near-total global
  hit ratio, not direct stall-cycle attribution.
- Uninstrumented post-profile benchmarks were hardware **343,205 H/s**, full
  software **27,578 H/s**, and filter **29,035 H/s**. Seven aligned common
  windows had aggregate range **365,320–365,332 H/s**, median
  **365,328 H/s**; hardware median **336,590 H/s** and software median
  **28,736 H/s**. Aggregate is 1,228 H/s (-0.335%) below the retained normal
  parent median 366,556 H/s. This reinforces that the linked profile image is
  diagnostic and must not replace normal-mode throughput evidence.
- RISC-V profile UF2 SHA-256:
  `5c658609dcdf811dc617de65f9645a033c74ec4f453781e5637fe28c5e102df4`;
  paired ARM UF2:
  `cbae3ae9973d1ecf272a25bee657d4fc3371c27322bd59f38dc7726dea181fbd`.
  Complete log: `logs/E02-profile-riscv.log`.
- E02 conclusion: paired counter evidence confirms the hardware tail/wait and
  second-hash segments dominate both ISAs. Hazard3 first-block DMA feeding is
  still a concrete **87.01-cycle/hash** segment, so E06-trigger's removal of
  one per-nonce MMIO operation is the next bounded experiment. The expected
  gain is small; only normal-image hardware/common-window evidence can retain
  it. E03 handoff fusion is not promoted because M33's measured handoff/second
  segment does not isolate a remaining digest spill, and the software paths
  remain dominated by SHA rounds rather than a cross-source call boundary.

### E02 normal-image restoration/control 85e — pass

- Source identity `02b7825c7499`, `MINER_PROFILE=0`, stock 150 MHz,
  temperature disabled. Both normal architectures rebuilt warning-free before
  flashing RISC-V. Run ID `30004927-0000000c` passed all 8 suites, standalone
  benchmarks, seven aligned windows, and strict capture without a fault.
- Standalone rates: hardware **343,193 H/s**, full software **28,631 H/s**,
  filter **30,041 H/s**. Seven-window aggregate range
  **366,596–366,606 H/s**, median **366,601 H/s**; hardware median
  **336,750 H/s**, software median **29,850 H/s**.
- The fresh normal aggregate is 45 H/s (+0.012%) versus the retained E01
  normal median 366,556 H/s. This confirms the parent remains stable and makes
  **366,601 H/s** the immediate same-session comparator for E06-trigger.
- Normal UF2 SHA-256: ARM
  `0d7faf52ddf9baca3fede920b752895e8e9c2aa160644aa3303740d234ecb900`,
  RISC-V
  `00db86d95bd8cacff5654730691f6dfc7f881368d8d69ae1d545c3f330689742`.
  Complete log: `logs/E02-normal-restored-riscv.log`.

## 2026-09-15 — E06-trigger candidate 86 definition

- Parent checkpoint `4409b84`, immediate normal RISC-V control
  **366,601 H/s** aggregate / **336,750 H/s** hardware / **29,850 H/s**
  software medians, stock 150 MHz, temperature disabled.
- Hazard3 only: retain the job-time normal 16-word DMA transfer count, then
  replace the per-nonce non-triggering read-address write plus triggering count
  write with one `dma_channel_set_read_addr(..., true)`. SDK 2.3.1 maps this to
  `AL3_READ_ADDR_TRIG`, whose RP2350 register definition states that a nonzero
  write reloads the channel counter and starts the channel. ARM retains its CPU
  feeder unchanged.
- Correctness requirements: every repeated transfer remains exactly 16 words;
  the channel is idle before rearm; source lifetime, SHA START ordering,
  completion wait, inter-block/result waits, error checks, and cleanup remain
  unchanged. Both architectures must build before flashing; all 8 hardware
  suites, 4,096-case oracle, standalone benchmarks, seven common windows, and
  strict capture must pass.
- Performance hypothesis: eliminate one SDK-inlined MMIO store from the
  profiled 87.01-cycle/hash first-feed segment. Compare the normal image against
  same-session control 85e; treat the variant as untested until hardware data.
- Pre-flash gates: both normal wrapper builds pass warning-free and all 8 host
  monitor tests pass. Hazard3 `mining_worker_core1` disassembly shows one
  `sw s7,60(t2)` to `AL3_READ_ADDR_TRIG` immediately after SHA START, followed
  by the unchanged DMA busy wait; the former separate source/count stores are
  absent. Dirty-build text is 200,744 bytes on RISC-V and 188,600 on ARM; clean
  committed artifacts will be rebuilt and hashed before flash.

### E06-trigger RISC-V attempt 86a — host libUSB failure

- Candidate commit `f6d3f90`, source identity `39129d52483f`, normal profile,
  stock 150 MHz, temperature disabled. Fresh ARM and RISC-V wrapper builds
  passed warning-free before the cycle attempted to flash Hazard3.
- `./tools/doctor` could not see `/dev/ttyACM*`. The authorized
  `MONITOR_SECONDS=50 ./tools/cycle riscv` rebuilt both architectures, then
  failed before loading the image because picotool reported
  `ERROR: Failed to initialise libUSB`. The command returned status 1 and
  serial monitoring never began. A subsequent `lsusb` also failed with
  `unable to initialize libusb: -99`.
- This attempt produced no BOOT line, device validation, benchmark, or mining
  measurement. It does not accept or reject the DMA trigger optimization.
  Retry the identical committed candidate after the VM/host USB subsystem and
  Pico runtime or BOOTSEL identity are available; do not add another code
  experiment while this comparison is pending.
- ARM UF2 SHA-256:
  `26af99da219ee01a87b143caa827bb45508a657313b5d5f753c6b6f9480dbbd9`.
  RISC-V UF2 SHA-256:
  `0bdb6c31236103bda04fafbf49b87d84895ae6d84a754ee2f737b815c891e9a3`.
- Archived terminal record:
  `logs/E06-trigger-riscv-libusb-failure.log`.

### E06-trigger RISC-V attempt 86b — pass and retention

- Retried the identical candidate commit `f6d3f90`, source identity
  `39129d52483f`, after exposing the host USB devices to this Codex execution
  environment. Both normal architectures rebuilt warning-free before flash.
  Run ID `30004927-00000002` passed all 8 suites, the 4,096-case cross-engine
  oracle, standalone hardware/full-software/filter benchmarks, seven aligned
  common windows, and strict capture without a fault.
- Standalone rates were hardware **344,783 H/s**, full software **28,636
  H/s**, and exact filter **30,041 H/s**. The hardware benchmark is unchanged
  from the parent, consistent with the benchmark's different control path.
- The first seven common windows had aggregate range **368,124–368,136 H/s**,
  median **368,130 H/s**; hardware range **338,268–338,280 H/s**, median
  **338,274 H/s**; software median **29,856 H/s**.
- Versus immediate normal control 85e, aggregate improves from 366,601 by
  **1,529 H/s (+0.42%)**, and hardware improves from 336,750 by **1,524 H/s
  (+0.45%)**; software moves +6 H/s. The hardware medians correspond to about
  445.43 cycles/hash before and 443.43 after, a reduction of approximately
  **2.01 cycles/hash**. This matches the intended removal of one SDK-inlined
  DMA register write from every sustained hardware nonce.
- Retain E06-trigger on Hazard3. It is a small, exact source simplification
  with a stable common-window gain and no new buffer, channel, or state-machine
  assumption. ARM remains on its retained CPU feeder and is byte-identical to
  its parent implementation.
- ARM UF2 SHA-256:
  `26af99da219ee01a87b143caa827bb45508a657313b5d5f753c6b6f9480dbbd9`.
  RISC-V UF2 SHA-256:
  `0bdb6c31236103bda04fafbf49b87d84895ae6d84a754ee2f737b815c891e9a3`.
  Complete serial log SHA-256:
  `c3e0d4aeb242e39102c94e2a5a1e1c60490a641d289f355a7815c96e4f7baf05`.
  Archived log: `logs/E06-trigger-riscv.log`.

## 2026-09-15 — E09-b header-tail fixed-padding rounds candidate 87

- Parent checkpoint `7a85905`, with E06-trigger retained only on Hazard3.
  Parent common-window medians are ARM **355,640 H/s** and immediate
  RISC-V **368,130 H/s**. Stock 150 MHz and temperature disabled remain fixed.
- Specialize software header-tail rounds 4 through 15. Round 4 consumes
  `K4 + 0x80000000`, rounds 5 through 14 consume only K, and round 15 consumes
  `K15 + 640`; the schedule array remains complete for later dependent words.
  This is distinct from the retained second-hash fixed-padding rounds 8--15
  and retained cached W16/W17 addends.
- Hypothesis: explicit fixed addends let the compiler avoid generic schedule
  loads/additions or choose a better round layout. SHA state evolution and all
  nonce-dependent schedule terms remain unchanged. Inspect both emitted
  artifacts before hardware; reject without flashing if an architecture is
  byte-identical, and retain only a repeatable affected-software/common-window
  gain with all oracle and mining-decision gates passing.

### E09-b header-tail fixed-padding rounds ARM attempt 87a — pass and retention

- Candidate commit `f1a9226`, source identity `babee796be42`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000003`. Both
  architectures built warning-free before flash; all 8 suites, the 4,096-case
  oracle, standalone benchmarks, eight common windows, and strict capture
  passed without a fault.
- Standalone hardware was **331,087 H/s**. Full software improved from the
  recorded parent 30,017 to **30,364 H/s** (**+347 H/s, +1.16%**), and the
  exact filter improved from 30,718 to **31,087 H/s** (**+369 H/s, +1.20%**).
- Across the first seven aligned common windows, aggregate range was
  **355,929–355,984 H/s**, median **355,955 H/s**; hardware range was
  **325,006–325,057 H/s**, median **325,030 H/s**; software range was
  **30,923–30,928 H/s**, median **30,926 H/s**. Versus parent medians, the
  software path gains **391 H/s (+1.28%)**, hardware moves -74 H/s, and total
  gains **315 H/s (+0.09%)**.
- Retain the fixed-addend form on ARM. Its isolated and sustained software
  measurements agree, and the common workload improves despite normal small
  hardware-side variation.
- Candidate ARM UF2 SHA-256:
  `e65fc53bcb992931e17fa88d27e564da7551b21ada58058550890c4a56c06d4a`.
  Paired RISC-V UF2 SHA-256:
  `6c128809c5ddc1d9cb958f3191093cfff83b15553185c1a934f326a8747d4e58`.
  Complete log SHA-256:
  `0a5baaf6b4c8d522bf4fa361714551420be4bb0824d67c2fced0fc6c911eeddb`.
  Archived log: `logs/E09b-header-tail-fixed-arm.log`.

### E09-b header-tail fixed-padding rounds RISC-V attempt 87b — pass, reject variant

- The identical candidate commit and source identity ran at stock 150 MHz,
  temperature disabled, run ID `30004927-00000004`. Both architectures built
  before flash; all validation gates, standalone benchmarks, eight common
  windows, and strict capture passed without a fault.
- Standalone hardware was **344,785 H/s**. Full software fell from the
  immediate E06-trigger parent 28,636 to **28,565 H/s** (**-71 H/s, -0.25%**),
  and the exact filter fell from 30,041 to **29,963 H/s** (**-78 H/s, -0.26%**).
- The first seven aligned windows had aggregate range **368,050–368,065 H/s**,
  median **368,057 H/s**; hardware range **338,268–338,282 H/s**, median
  **338,274 H/s**; software range **29,781–29,786 H/s**, median **29,783 H/s**.
  Hardware is unchanged from the parent median, while software and aggregate
  each lose **73 H/s (-0.24% software, -0.02% aggregate)**.
- Reject this round shape on Hazard3 and restore its parent generic rounds
  4–15 loop. Keep the fixed form only under the ARM build. This is an ISA split
  supported by matching isolated and sustained hardware results, not a
  compiler-output assumption.
- Complete log SHA-256:
  `453e242c44e806012a82ff903fc579603ab0e6574578fe85202c984e95674b6b`.
  Archived log: `logs/E09b-header-tail-fixed-riscv.log`.

### E09-b final architecture split attempts 87c/87d — pass and resolved

- Final commit `688148d`, source identity `f0b612529b69`, normal profile,
  stock 150 MHz, temperature disabled. The fixed rounds remain on ARM only;
  Hazard3 compiles its exact parent generic loop. Fresh clean ARM and RISC-V
  wrapper builds passed warning-free before either flash. Host monitor tests
  remained 8/8 passing.
- ARM run `30004927-00000005` passed all validation and capture gates. Its
  standalone rates exactly repeated candidate 87a: hardware **331,087 H/s**,
  full software **30,364 H/s**, filter **31,087 H/s**. First-seven common
  medians were aggregate **355,924 H/s**, hardware **324,996 H/s**, software
  **30,923 H/s**. This second full run confirms the ARM software gain; total
  remains above the 355,640 H/s parent despite normal hardware-rate variation.
- RISC-V run `30004927-00000006` also passed every gate. Standalone rates were
  hardware **344,785 H/s**, full software **28,636 H/s**, filter **30,041
  H/s**. First-seven common medians were aggregate **368,130 H/s**, hardware
  **338,273 H/s**, software **29,856 H/s**—effectively exact restoration of
  the E06-trigger parent (368,130 / 338,274 / 29,856 H/s).
- The experiment is resolved: retain explicit header-tail padding rounds 4–15
  on ARM, retain the generic loop on Hazard3, and use this architecture split
  as the next 150 MHz code baseline.
- Final ARM UF2 SHA-256:
  `4aa4b971435e89ca7c8d9fdf77e09d94fdafab7487ee86f32388df9ec4fc5ff2`.
  Final RISC-V UF2 SHA-256:
  `be8a6031f3d87fb2d39a52e625249190bc4284dde4099844f4d51f197582a13e`.
  ARM log SHA-256:
  `a363f574a9b46235b4713ec98140d8d61d4249b82bdbbcb55c9b260b8cc00d13`.
  RISC-V log SHA-256:
  `2c851bd3881bb389240344fb7b508d5728032a54f80947fb8cbee2d4a2b9f4d2`.
  Archived logs: `logs/E09b-header-tail-final-arm.log` and
  `logs/E09b-header-tail-final-riscv.log`.

## 2026-09-15 — E09-c terminal live-result candidate 88 definition

- Parent checkpoint `af7c38c`, source identity `f0b612529b69`, normal profile,
  stock 150 MHz, temperature disabled. Parent common medians are ARM **355,924
  H/s** and RISC-V **368,130 H/s**; standalone filter rates are ARM **31,087
  H/s** and RISC-V **30,041 H/s**.
- Peel zero-based round 60 from the second-hash rejection loop and compute only
  its live result, `new_e = old_d + T1`. Omit T2 (`sum0 + majority`), new-a,
  and the otherwise dead final state rotation. Rounds 0–59 and schedule W0–W60
  remain unchanged; a qualifying high word still falls back to the complete
  double digest before reporting a candidate.
- Baseline disassembly confirms this is not already optimized away: both
  compilers execute sum0, majority, T2 and new-a inside the final loop
  iteration before returning e. Expected removable cost is those rotates,
  boolean operations and additions once per filtered nonce, offset by one
  explicit terminal expression and any changed loop control. No new persistent
  RAM is required.
- Build both architectures and inspect the emitted terminal paths before
  flash. Accept only if all full-digest/oracle/decision tests pass and the
  standalone filter plus sustained software/common-window measurements show
  a repeatable gain without materially reducing the unaffected hardware path.
- Pre-flash gates pass: both wrapper builds are warning-free and all 8 host
  monitor tests pass. Disassembly shows the peeled terminal path no longer
  computes sum0, majority, T2, or new-a after the loop on either ISA. The
  compiler schedules some T1 inputs before the final loop exit. Candidate text
  is 188,680 bytes on ARM (+112) and 200,872 on RISC-V (+128); BSS is
  unchanged. The modest code growth is acceptable only if hardware throughput
  confirms that the removed dynamic work wins.

### E09-c terminal live-result ARM attempt 88a — pass and retention

- Candidate commit `a628426`, source identity `71b48132e899`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000007`. Both clean
  architecture builds passed before flash. All 8 suites, the 4,096-case
  oracle, target/rare-decision checks, standalone benchmarks, eight common
  windows, and strict capture passed without a fault.
- Standalone hardware/full-software remained **331,084 / 30,364 H/s**. The
  affected exact filter improved from 31,087 to **31,578 H/s** (**+491 H/s,
  +1.58%**).
- First-seven common medians were aggregate **356,531 H/s** (range
  356,528–356,532), hardware **325,123 H/s** (325,120–325,125), and software
  **31,408 H/s** (31,407–31,410). Versus the final E09-b parent, software gains
  **485 H/s (+1.57%)** and aggregate gains **607 H/s (+0.17%)**; the 127 H/s
  hardware movement is small favorable run variation on unaffected code.
- Retain on ARM. Isolated and sustained filter gains match closely and all
  exact fallback/decision coverage passes.
- ARM UF2 SHA-256:
  `503e315bed9cf5ef334500645044a121a6a0e123a07fbd1f9d3ec4d9a036b5c5`.
  Complete log SHA-256:
  `51ab6eb056cc19b1879ba032b8226c4980635fec49ddb8f8ff892bd290e278ac`.
  Archived log: `logs/E09c-terminal-arm.log`.

### E09-c terminal live-result RISC-V attempt 88b — pass and retention

- The identical committed candidate ran as `30004927-00000008`, with stock
  150 MHz and temperature disabled. Both architectures built before flash and
  every runtime/capture gate passed without a fault.
- Standalone hardware/full-software were unchanged at **344,785 / 28,636
  H/s**. The affected exact filter improved from 30,041 to **30,235 H/s**
  (**+194 H/s, +0.65%**).
- First-seven common medians were aggregate **368,488 H/s** (range
  368,465–368,492), hardware **338,434 H/s** (338,415–338,441), and software
  **30,051 H/s** (30,049–30,055). Against the immediate E09-b parent, software
  gains **195 H/s (+0.65%)** and aggregate gains **358 H/s (+0.10%)**; the
  unaffected hardware path accounts for +161 H/s of the total difference.
- Retain the shared terminal live-result computation. Both ISAs show a stable
  affected-path win, while the 112/128-byte text growth uses no additional BSS
  and remains small relative to the exact dynamic work removed each nonce.
- RISC-V UF2 SHA-256:
  `6fd2b7f053cac2f4ed8b2e8b94626892fbc31ff01e0e69cbf8bdb92165a646b6`.
  Complete log SHA-256:
  `4918f56598f9788ef86757b629a6fb6ca5f244879447ff344d68641a211eb1f9`.
  Archived log: `logs/E09c-terminal-riscv.log`.

## 2026-09-15 — E09-b tail W20–W30 schedule-shape candidate 89 definition

- Parent checkpoint `cf839d2`, source identity `71b48132e899`, normal profile,
  stock 150 MHz, temperature disabled. Parent common medians are ARM **356,531
  H/s** and RISC-V **368,488 H/s**; standalone full/filter rates are ARM
  **30,364 / 31,578 H/s** and RISC-V **28,636 / 30,235 H/s**.
- Replace only the generic header-tail schedule recurrence for W20 through W30
  with explicit algebraic expressions. Because W5–W14 are zero, W20–W29 need
  no small-sigma0 term; W30 retains the compile-time constant sigma0(W15=640).
  Existing nonce-dependent W18/W19, job-precomputed W16/W17 and W31/W32, all
  W33–W63 expansion, and every SHA round remain unchanged.
- Expected removable cost is nine dynamically evaluated sigma0 triples plus
  their zero-input additions per software nonce if GCC did not fully fold the
  loop. Resource risk is code growth and longer live ranges. The exact
  expressions are checked by the existing 4,096-case independent oracle; no
  probabilistic shortcut or persistent RAM is introduced.
- Build both ISAs and inspect size/disassembly first. Reject without hardware
  if the emitted kernel is unchanged, or after hardware if either correctness
  fails or affected software/common-window throughput regresses. Treat ARM
  and Hazard3 independently if compiler scheduling produces an ISA split.
- Pre-flash gates pass: both wrapper builds are warning-free and all 8 host
  monitor tests pass. Total text/BSS are unchanged from the parent at
  188,680/4,708 bytes on ARM and 200,872/4,440 bytes on RISC-V, but the
  header-tail disassemblies are not no-ops: they contain the direct W20–W30
  dependency chain and no zero-input sigma0 work for W20–W29. Hardware remains
  necessary because the compiler traded the old loop for a differently
  scheduled sequence without reducing whole-image size.

### E09-b tail W20–W30 ARM attempt 89a — pass, reject

- Candidate commit `1a01c37`, source identity `6484d798b682`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000009`. Both clean
  architecture builds passed before flash. All 8 suites, the 4,096-case
  oracle, standalone benchmarks, eight common windows, and strict capture
  passed without a fault.
- Standalone hardware was **331,086 H/s**. Full software moved from 30,364 to
  **30,382 H/s** (+0.06%), and filter from 31,578 to **31,598 H/s** (+0.06%).
- First-seven common medians were aggregate **356,450 H/s**, hardware
  **325,026 H/s**, and software **31,424 H/s**. Versus the E09-c parent,
  software moves only **+16 H/s (+0.05%)**, hardware -97 H/s, and aggregate
  -81 H/s. The intended affected-path change is well below meaningful noise.
- Reject on ARM: the source complexity and changed scheduling are not
  justified by a sub-0.1% affected-path movement.
- Candidate ARM UF2 SHA-256:
  `3c7679b9987af8a5defb9185c60227792df397b3c512a7079f2a13e3d8fe8f98`.
  Complete log SHA-256:
  `5fe39fcadab038d0d3a41d9f1105eadeee5122cb94a0eb4d2e34b8f4582a9e52`.
  Archived log: `logs/E09b-tail-schedule-arm.log`.

### E09-b tail W20–W30 RISC-V attempt 89b — pass, reject and restore

- The identical committed candidate ran as `30004927-0000000a`, at stock
  150 MHz with temperature disabled. Both architectures built before flash;
  every validation and capture gate passed without a fault.
- Standalone hardware/full/filter were **344,785 / 28,642 / 30,241 H/s**.
  The software figures are just **+6 H/s (+0.02%)** from the parent.
- First-seven common medians were aggregate **368,321 H/s**, hardware
  **338,265 H/s**, and software **30,056 H/s**. Software is only +5 H/s
  (+0.02%), while hardware/aggregate move -169/-167 H/s through unaffected
  run variation.
- Reject the explicit W20–W30 schedule shape on both ISAs and restore the exact
  E09-c parent loop. GCC's existing loop shape already captures essentially
  all available benefit; identical whole-image size and negligible measured
  movement do not justify maintaining eleven manual expressions.
- Candidate RISC-V UF2 SHA-256:
  `0046eb2e95b2ae3d5625197318c8ac839f0052beee3e6ac5c49edbfa2d0ce5f3`.
  Complete log SHA-256:
  `795652d4f0c9782237e89fb71470fe7a2e8f27e10a704f45cb9fd75cd00435c9`.
  Archived log: `logs/E09b-tail-schedule-riscv.log`.
- Restoration commit `d2b7085` returns `src/software_sha256.c` exactly to
  accepted parent `cf839d2`. Fresh warning-free builds reproduce the accepted
  E09-c artifacts byte-for-byte: ARM
  `503e315bed9cf5ef334500645044a121a6a0e123a07fbd1f9d3ec4d9a036b5c5`
  and RISC-V
  `6fd2b7f053cac2f4ed8b2e8b94626892fbc31ff01e0e69cbf8bdb92165a646b6`.
  Their existing attempts 88a/88b therefore remain the hardware validation
  for the restored baseline; no duplicate flash is needed.

## 2026-09-15 — E14 finite dynamic chunks candidate 90 definition

- Parent checkpoint `effc1ad`, accepted E09-c firmware identity
  `71b48132e899`, normal profile, stock 150 MHz, temperature disabled. Parent
  first-seven common medians are ARM **356,531 H/s** and RISC-V **368,488
  H/s**.
- Replace permanent even/odd nonce ownership with a shared generation-tagged
  64-bit cursor over the exact end-exclusive range `[0, 2^32)`. Both workers
  acquire disjoint 4,096-nonce chunks under a Pico SDK cross-core critical
  section; the lock is taken only at chunk boundaries and each worker retains
  its precomputed job state across chunks.
- Natural exhaustion is no longer a fault. Core 1 reports its validated final
  partial count, core 0 waits for both workers, and completion succeeds only
  when reconciled hardware plus software hashes equal exactly `2^32`.
  Cancellation, job replacement, and FIFO telemetry changes are intentionally
  excluded from this candidate so their correctness and performance can be
  evaluated separately.
- Add a ninth startup KAT for the pure allocator boundary logic. Its seven
  checks cover the range ending at `2^32`, a final partial chunk, contiguous
  allocation, one worker draining the range while the other is parked,
  generation mismatch without cursor movement, zero chunk rejection, and
  exhaustion without reissue. Update the strict host validation contract and
  its synthetic tests to require the named ninth suite.
- Pre-flash gates pass: all 8 host contract unit tests pass, `git diff
  --check` is clean, and both repository wrapper builds are warning-free.
  Candidate image size is 189,576 text / 4,740 BSS bytes on ARM and 201,636
  text / 4,472 BSS bytes on RISC-V, respectively +896/+32 and +764/+32 bytes
  over the E09-c parent.
- Hardware acceptance requires all nine suites and strict capture to pass on
  both ISAs, no fault or protocol stall, and common-window aggregate throughput
  that does not materially regress. Compare 4,096-nonce chunk synchronization
  overhead independently on each architecture before attempting the separate
  65,536-nonce variant.

### E14 finite 4,096-nonce chunks ARM attempt 90a — pass, reject variant

- Candidate commit `00a1e22`, source identity `406eaa5b2565`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000002`. Both clean
  architecture builds passed before flash. All nine suites, including the new
  seven-case allocator KAT, the 4,096-case hash oracle, five synchronized
  windows, and strict capture passed without a fault or protocol stall.
- Standalone hardware/full/filter rates remained **331,085 / 30,364 / 31,578
  H/s**, confirming the hash kernels themselves are unchanged.
- The five common windows had aggregate range **263,840–263,892 H/s**, median
  **263,860 H/s**; hardware range **238,657–238,702 H/s**, median **238,678
  H/s**; and software range **25,180–25,189 H/s**, median **25,182 H/s**.
  Versus the accepted E09-c parent this is aggregate **-92,671 H/s (-25.99%)**,
  hardware **-86,445 H/s (-26.59%)**, and software **-6,226 H/s (-19.82%)**.
- Reject this 4,096-chunk implementation shape on ARM. Correct finite/disjoint
  allocation is demonstrated, but placing a 64-bit cursor increment and chunk
  boundary test in each worker's hot loop causes an unacceptable sustained
  regression; the infrequent lock alone is not yet established as the cause.
- ARM UF2 SHA-256:
  `d94d49684523929cdc1d25b72dd6ceb8313c1d35b0c2103af6739b78f3c99a5d`.
  Complete log SHA-256:
  `917c5f524dc1561f29b98f8a34105d29e6e5b92ca28aefb5c7dc863f67588129`.
  Archived log: `logs/E14-chunks-4096-arm.log`.

### E14 finite 4,096-nonce chunks RISC-V attempt 90b — pass, reject variant

- The identical committed candidate ran as `30004927-00000003`, at stock
  150 MHz with temperature disabled. Both builds, all nine suites, eight
  synchronized windows, and strict capture passed without a fault or stall.
- Standalone hardware/full/filter rates were **344,785 / 28,631 / 30,235
  H/s**, effectively matching the parent kernels.
- The first seven common windows had aggregate range **361,673–361,693 H/s**,
  median **361,682 H/s**; hardware range **331,665–331,686 H/s**, median
  **331,675 H/s**; and software range **30,006–30,009 H/s**, median **30,006
  H/s**. Against E09-c this is aggregate **-6,806 H/s (-1.85%)**, hardware
  **-6,759 H/s (-2.00%)**, and software **-45 H/s (-0.15%)**.
- Reject the 4,096-chunk implementation on Hazard3 as well. The much smaller
  ISA-specific loss supports testing a cheaper 32-bit per-chunk local loop
  representation before deciding whether finite dynamic allocation itself is
  too costly. Keep the plan's 65,536 size as a separate experiment after the
  hot-loop representation is corrected.
- RISC-V UF2 SHA-256:
  `e3de2d61fd6078fe7194d18f502d82102a3890087ce227cd718b081c936250a4`.
  Complete log SHA-256:
  `521c622e1c79aea3d33b5d652ab48dc207203f345b1ac7a299ecc483111a3f69`.
  Archived log: `logs/E14-chunks-4096-riscv.log`.

## 2026-09-15 — E14 32-bit local chunk-loop candidate 91 definition

- Parent candidate `00a1e22` proved the generation-tagged 64-bit allocator and
  completion accounting correct on both ISAs, but its per-hash 64-bit local
  cursor shape was rejected at **-25.99% ARM** and **-1.85% Hazard3** aggregate.
- Preserve the shared 64-bit cursor, exact `[0, 2^32)` range, SDK critical
  section, generation, 4,096-nonce chunk size, ninth KAT, and all FIFO/reporting
  behavior. Change only each worker's acquired-range representation to a
  32-bit nonce and 32-bit remaining count, so the hot path performs a native
  increment/decrement and zero test rather than 64-bit increment/comparison.
- The conversion is safe even for the final chunk: its start is representable
  as `uint32_t`, its length is at most 4,096, and wrapping the local nonce after
  hashing `0xffffffff` is ignored when remaining reaches zero. The shared
  64-bit cursor remains authoritative for exhaustion and final accounting.
- Build and hardware-test both ISAs as a separate implementation-shape
  experiment. Accept only if all nine suites remain exact and sustained rates
  recover close to E09-c; do not attribute any result to the 65,536 chunk size,
  which remains a later one-variable experiment.

### E14 32-bit local chunk loop ARM attempt 91a — pass, reject variant

- Candidate commit `b8f53c9`, source identity `514ad72bd3ac`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000004`. Both clean
  builds passed before flash; all nine suites, seven synchronized windows and
  strict capture passed without a fault or stall.
- Standalone hardware/full/filter rates were **331,089 / 30,370 / 31,578
  H/s**, matching the accepted kernels.
- First-seven common medians were aggregate **345,302 H/s** (range
  345,298–345,305), hardware **313,939 H/s** (313,932–313,941), and software
  **31,364 H/s** (31,363–31,366). Versus E09-c this is aggregate **-11,229
  H/s (-3.15%)**, hardware **-11,184 H/s (-3.44%)**, and software **-44 H/s
  (-0.14%)**.
- Native local state recovers 81,442 aggregate H/s relative to candidate 90,
  confirming that per-hash 64-bit local cursor operations caused most of its
  catastrophic ARM loss. Reject this 4,096 variant because the remaining
  3.15% aggregate cost is still material; test the planned 65,536 chunk size
  without any other change to isolate acquisition frequency.
- ARM UF2 SHA-256:
  `4b9e12813540444b494de229e9afd578dab09d4353c622669495ffb8a67a72a0`.
  Complete log SHA-256:
  `cdd8f89ee81dbaf1c5e5f8c52eea69602900d5ddd98f8d1c7d64a51e3609308d`.
  Archived log: `logs/E14-chunks-native-arm.log`.

### E14 32-bit local chunk loop RISC-V attempt 91b — pass, reject variant

- The identical committed candidate ran as `30004927-00000005`. Both builds,
  all nine suites, eight synchronized windows and strict capture passed at
  stock 150 MHz with temperature disabled.
- Standalone hardware/full/filter rates were **344,785 / 28,636 / 30,235
  H/s**. First-seven common medians were aggregate **360,017 H/s** (range
  360,003–360,024), hardware **330,042 H/s** (330,027–330,050), and software
  **29,974 H/s** (29,971–29,976).
- Versus E09-c this is aggregate **-8,471 H/s (-2.30%)**, hardware **-8,392
  H/s (-2.48%)**, and software **-77 H/s (-0.26%)**. It is also 1,665 H/s
  slower than candidate 90's 64-bit Hazard3 shape, demonstrating another ISA
  split: native local counters help M33 greatly but not Hazard3.
- Reject the 4,096 native-local variant on both ISAs, while carrying its safer
  final-range representation into the one-variable 65,536-chunk experiment.
- RISC-V UF2 SHA-256:
  `efcaf84f40acb68916e17934bc28e02351c0bf2ad8d9d0bb6c4474b80dfef76b`.
  Complete log SHA-256:
  `6c3c6ff08d7d9dacc6bbe282093ddc07584bd535cae3d99321c64b24be02c30a`.
  Archived log: `logs/E14-chunks-native-riscv.log`.

## 2026-09-15 — E14 finite 65,536-nonce chunks candidate 92 definition

- Parent implementation candidate `b8f53c9` uses the correct native local
  nonce/remaining representation but loses 3.15% aggregate on ARM and 2.30%
  on Hazard3 with 4,096-nonce chunks.
- Change only `MINING_CHUNK_SIZE` from 4,096 to 65,536, the second size
  prescribed by E14-chunks. The shared 64-bit end-exclusive allocator,
  generation, SDK critical section, native local loop, precomputed job state,
  ninth allocator KAT, FIFO protocol, report cadence, clock and temperature
  configuration remain identical.
- This reduces chunk acquisitions and synchronization calls by 16x, while
  increasing worst-case redistribution/cancellation granularity from 4,096 to
  65,536 hashes. Hardware-test both ISAs independently; accept only if the
  sustained aggregate cost becomes small enough to justify finite-work
  balancing, with correctness and final partial-range coverage intact.

### E14 finite 65,536-nonce chunks ARM attempt 92a — pass, reject variant

- Candidate commit `14f4ec9`, source identity `76942cb57ac5`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000006`. Both clean
  builds, all nine suites, seven synchronized windows and strict capture
  passed without fault or stall.
- Standalone hardware/full/filter were **331,086 / 30,370 / 31,578 H/s**.
  First-seven common medians were aggregate **345,308 H/s**, hardware
  **313,943 H/s**, and software **31,364 H/s**. Versus E09-c these are
  **-3.15%**, **-3.44%**, and **-0.14%**, respectively.
- The aggregate median is only 6 H/s above the otherwise identical 4,096
  native-local candidate. A 16x reduction in allocator/critical-section calls
  therefore has no measurable benefit on M33 and rules out acquisition
  frequency as the source of the remaining regression. Reject this variant.
- ARM UF2 SHA-256:
  `21a7700f707e89338b9c102d6c89e8e284d0383d2c40f3000391b93d1117b8b1`.
  Complete log SHA-256:
  `1ffee210e857a698ae7d039aaa56032bacf1f3a853969244b21b2ce9171c5115`.
  Archived log: `logs/E14-chunks-65536-arm.log`.

### E14 finite 65,536-nonce chunks RISC-V attempt 92b — pass, reject variant

- The identical committed candidate ran as `30004927-00000007`. Both builds,
  all nine suites, eight synchronized windows and strict capture passed at
  stock 150 MHz with temperature disabled.
- Standalone hardware/full/filter were **343,993 / 28,631 / 30,235 H/s**.
  First-seven common medians were aggregate **357,198 H/s**, hardware
  **327,210 H/s**, and software **29,988 H/s**. Versus E09-c these are
  aggregate **-11,290 H/s (-3.06%)**, hardware **-11,224 H/s (-3.32%)**, and
  software **-63 H/s (-0.21%)**.
- This is 2,819 H/s slower than the otherwise identical 4,096 native-local
  variant, so larger chunks do not recover Hazard3 throughput either. Reject
  candidate 92 on both ISAs. The next bounded implementation-shape test should
  move chunk acquisition outside a true nested inner nonce loop, allowing the
  compiler to optimize that loop independently while preserving finite range
  ownership and the accepted 4,096 responsiveness target.
- RISC-V UF2 SHA-256:
  `2e5ccdeb7def91f726be007f4126d400c344f0f38f3af60d6d731ccc36409c8e`.
  Complete log SHA-256:
  `0500961cb1dd11adf15e890b30668316155ca2f3232cbe4a6eb4b4b887abfafe`.
  Archived log: `logs/E14-chunks-65536-riscv.log`.

## 2026-09-15 — E14 nested hardware chunk loop candidate 93 definition

- Candidates 91/92 show that changing acquisition frequency by 16x does not
  recover throughput. Return to the 4,096 responsiveness target and preserve
  candidate 91's validated allocator, native local range state, software loop,
  protocol, accounting, KATs and configuration.
- Restructure only the core-1 hardware worker into an outer chunk-acquisition
  loop and an inner `do`/`while` nonce loop. This makes allocation/completion
  control structurally cold and gives GCC a self-contained native-width hot
  loop, while report and share checks remain per hash exactly as before.
- Compare emitted assembly and hardware rates to candidate 91. This is an
  implementation-shape experiment, not a new allocation policy; reject if it
  does not recover the hardware-side loss on each ISA.

### E14 nested hardware chunk loop ARM attempt 93a — pass, reject variant

- Candidate commit `5946ae8`, source identity `388e3f5d3d28`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000008`. Both clean
  wrapper builds, all nine suites, eight synchronized windows and strict
  capture passed without a fault, timeout or reset.
- Standalone hardware/full/filter rates were **331,083 / 30,370 / 31,578
  H/s**. First-seven common medians were aggregate **355,003 H/s** (range
  354,999–355,005), hardware **323,654 H/s** (323,653–323,659), and software
  **31,347 H/s** (31,345–31,350).
- Versus E09-c this is aggregate **-1,528 H/s (-0.43%)**, hardware **-1,469
  H/s (-0.45%)**, and software **-61 H/s (-0.19%)**. The nested shape recovers
  9,701 aggregate H/s and 9,715 hardware H/s over candidate 91, consistent
  with its 28-byte smaller emitted M33 worker, but it does not eliminate the
  finite-allocation regression. Reject the ARM variant.
- ARM UF2 SHA-256:
  `2e2a53a1b2865d35e68c8777fb545fee799e38dda96047851ff37a412e00b25c`.
  Complete log SHA-256:
  `43d6eea50351e4188b2e428777946736229b6c430591dd3cba96309d6b0ed2d1`.
  Archived log: `logs/E14-chunks-nested-arm.log`.

### E14 nested hardware chunk loop RISC-V attempt 93b — pass, reject variant

- The identical committed candidate ran as `30004927-00000009`. Both builds,
  all nine suites, eight synchronized windows and strict capture passed at
  stock 150 MHz with temperature disabled.
- Standalone hardware/full/filter rates were **344,783 / 28,631 / 30,235
  H/s**. First-seven common medians were aggregate **363,813 H/s** (range
  363,798–363,817), hardware **333,804 H/s** (333,787–333,807), and software
  **30,010 H/s** (30,010–30,013).
- Versus E09-c this is aggregate **-4,675 H/s (-1.27%)**, hardware **-4,630
  H/s (-1.37%)**, and software **-41 H/s (-0.14%)**. Nesting recovers 3,796
  aggregate H/s and 3,762 hardware H/s over candidate 91 and reduces the
  emitted Hazard3 worker by 166 bytes, but a material loss remains. Reject
  candidate 93 on both ISAs and restore E09-c before another experiment.
- RISC-V UF2 SHA-256:
  `86fa63fe6e713cfa5e7209a49dbf7d167cedc96c1e926cc97d86c10bbdd7892d`.
  Complete log SHA-256:
  `9cf4719d0be01fbf0b4995cc1d58370ab45acb71d9e7df9ad36c79432e396d4c`.
  Archived log: `logs/E14-chunks-nested-riscv.log`.

## 2026-09-15 — E14 rejected-source restoration definition

- Restore only `src/main.c`, `tools/monitor.py`, and `tools/test_monitor.py`
  from accepted pre-E14 evidence commit `effc1ad`; preserve every E14 ledger
  entry and archived log. This removes the finite allocator, ninth allocator
  KAT and corresponding monitor contract after all three implementation shapes
  were rejected, returning runtime behavior to accepted E09-c.
- Build both ISAs warning-free and compare UF2 hashes with the accepted E09-c
  artifacts. Byte identity is sufficient to reference attempts 88a/88b; any
  mismatch requires fresh paired hardware validation before further work.

### E14 rejected-source restoration — pass, accepted baseline restored

- Restoration commit `c3c6836` builds warning-free with source identity
  `71b48132e899`; all eight host monitor tests pass.
- ARM UF2 SHA-256 is
  `503e315bed9cf5ef334500645044a121a6a0e123a07fbd1f9d3ec4d9a036b5c5`
  and RISC-V UF2 SHA-256 is
  `6fd2b7f053cac2f4ed8b2e8b94626892fbc31ff01e0e69cbf8bdb92165a646b6`.
  Both are byte-identical to the accepted E09-c artifacts already validated
  in attempts 88a/88b, so no redundant hardware run is required.

## 2026-09-15 — E08 bounded telemetry queue candidate 94 definition

- Parent is the restored E09-c artifact at source identity `71b48132e899`.
  Core 1 currently publishes multiword progress/share/fault records through
  the eight-word hardware FIFO, so a 12-word share necessarily blocks midway
  and consumer delays can stall the hardware owner with a partial record.
- Replace only core-1 outbound telemetry with an eight-entry, fixed-record
  SPSC SRAM queue using C11 release/acquire publication. Keep the FIFO for the
  one-word startup/window acknowledgements. Preserve lossless shares with
  explicit producer backpressure; publish faults through a separate atomic
  latch so a full telemetry queue cannot suppress a fatal stop. Add a bounded
  wrap/full/order KAT and report queue depth plus cumulative blocked time.
- Expected removable cost is producer FIFO serialization and partial-record
  blocking, not SHA work. Resource cost is approximately 384 bytes plus queue
  metadata. Reject on either correctness/protocol failure, lost/reordered
  record, unsafe fault behavior, or material sustained regression. A neutral
  hard-target result may still justify later slow-consumer stress only if the
  lifecycle semantics are demonstrably stronger.

### E08 bounded telemetry queue ARM attempt 94a — pass, reject variant

- Candidate commit `4e65e44`, source identity `8dba444a311c`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-0000000a`. Both clean
  builds, all nine suites, six synchronized windows and strict capture passed.
  Queue maximum depth remained one and cumulative producer blocked time stayed
  zero throughout, proving the normal run never saturated the queue.
- Standalone hardware/full/filter rates were **331,820 / 30,364 / 31,578
  H/s**. The midpoint median of all six available windows was aggregate
  **285,615 H/s** (range 285,424–285,923), hardware **260,775 H/s**
  (260,592–261,068), and software **24,840 H/s** (24,832–24,855).
- Versus E09-c this is aggregate **-70,916 H/s (-19.89%)**, hardware **-64,348
  H/s (-19.79%)**, and software **-6,568 H/s (-20.91%)**. Since standalone
  kernels are intact and no producer wait occurred, reject this per-hash queue
  and fault polling shape on M33. The result indicates severe shared-memory,
  placement or acquire-poll interference rather than backpressure.
- Candidate size is 189,656 text / 5,132 BSS bytes, respectively +976/+424
  versus E09-c. ARM UF2 SHA-256:
  `95aff764b20cdc81dbab1a13767f33666b3b50a973516481f369a5bc569a6d90`.
  Complete log SHA-256:
  `2cd7f76f7a497c5a9e2a657361466f8654697903149785025e6016f51af6dcc7`.
  Archived log: `logs/E08-queue-arm.log`.

### E08 bounded telemetry queue RISC-V attempt 94b — pass, reject shared variant

- The identical candidate ran as `30004927-0000000b`; all nine suites, eight
  windows and strict capture passed. Queue depth remained one and producer
  blocked time remained zero.
- Standalone hardware/full/filter rates were **345,572 / 28,631 / 30,235
  H/s**. First-seven common medians were aggregate **368,259 H/s** (range
  368,255–368,262), hardware **338,254 H/s** (338,247–338,256), and software
  **30,006 H/s** (30,005–30,009). Versus E09-c these are **-0.06%**, **-0.05%**
  and **-0.15%**: practically neutral, but not a throughput win.
- Candidate size is 202,424 text / 4,864 BSS bytes, +1,552/+424 versus E09-c.
  Reject candidate 94 as shared code because of the ARM failure. Before fully
  restoring, test one bounded variant that polls queue/fault state once per 64
  software hashes; reject it if M33 does not recover or Hazard3 materially
  regresses. This differs from rejected E10 polling because outbound records
  remain atomic complete SPSC entries and the fault latch remains independent.
- RISC-V UF2 SHA-256:
  `913c3cbcf132f959b52c49482883bf32ddc1d5e41efb719aa0798ca5bf45045a`.
  Complete log SHA-256:
  `12f346a346d3a5a294cf17758394f20d54f0b1266dd5b036c6412e541d687b9d`.
  Archived log: `logs/E08-queue-riscv.log`.

## 2026-09-15 — E08 batched telemetry polling candidate 95 definition

- Preserve candidate 94's complete-record SPSC queue, lossless backpressure,
  independent fatal latch, record formats, KAT, clock and temperature state.
  Change only core 0's steady-state polling cadence from every software hash
  to once per 64 hashes. Startup still polls the fault latch while waiting for
  READY, and a steady-state fatal fault remains bounded by at most 64 software
  evaluations (about 2 ms at current rates).
- The hypothesis is that repeated shared acquire loads or their placement/bus
  effects caused candidate 94's 19.89% M33 aggregate loss. Queue capacity eight
  is ample for the roughly 0.3-second normal progress cadence; the periodic
  16-report ACK may be delayed by only one polling batch. Accept only if ARM
  returns close to E09-c and Hazard3 does not repeat the previously rejected
  E10 polling loss. Queue depth, blocked time and all nine suites must remain
  valid.

### E08 batched telemetry polling ARM attempt 95a — pass, reject variant

- Candidate commit `d9490a9`, source identity `dc055a3ef461`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-0000000c`. Both clean
  builds, all nine suites, six windows and strict capture passed. Queue depth
  remained one and producer blocked time remained zero.
- Standalone hardware/full/filter were **331,819 / 30,364 / 31,578 H/s**.
  The midpoint median of six windows was aggregate **272,198 H/s** (range
  272,147–272,235), hardware **247,953 H/s** (247,902–247,991), and software
  **24,246 H/s** (24,240–24,250).
- Versus E09-c these are aggregate **-84,333 H/s (-23.65%)**, hardware
  **-77,170 H/s (-23.73%)**, and software **-7,162 H/s (-22.80%)**. Polling
  64x less often worsens candidate 94 by another 13,417 aggregate H/s. This
  falsifies acquire frequency as the dominant ARM cause and points instead to
  candidate-wide code/data placement or shared-memory interaction. Reject.
- ARM UF2 SHA-256:
  `2c0469ad31c5a10926b4cde0906b9bf64f1246f9675ab46a29f725014f21b0e0`.
  Complete log SHA-256:
  `cf5b416ae12479120f61a07cfe47f5301ddc97ddf574012c6b320e434437a80e`.
  Archived log: `logs/E08-queue-poll64-arm.log`.

### E08 batched telemetry polling RISC-V attempt 95b — pass, reject variant

- The identical candidate ran as `30004927-0000000d`; all nine suites, eight
  windows and strict capture passed, again at queue depth one with no blocking.
- Standalone hardware/full/filter were **345,578 / 28,631 / 30,235 H/s**.
  First-seven medians were aggregate **368,281 H/s** (range 368,280–368,315),
  hardware **338,195 H/s** (338,194–338,227), and software **30,086 H/s**
  (30,086–30,089). Versus E09-c these are **-0.06%**, **-0.07%**, and
  **+0.12%**, respectively: neutral and not enough to justify an ISA split.
- Reject candidate 95 and the complete queue branch. Restore the accepted
  E09-c source/test contract byte-identically. Future E08 work needs a smaller
  representation or measured placement/bank experiment; do not repeat either
  polling cadence unchanged.
- RISC-V UF2 SHA-256:
  `f3ca5338728a95dad47e08ea2b5dc5472b178c8baaa40d3dd58e865c57eb537c`.
  Complete log SHA-256:
  `3a4bb9d767bacd8dcbc388510de20f82f7e56f71de3a27aad31a5a9374ec70e4`.
  Archived log: `logs/E08-queue-poll64-riscv.log`.

## 2026-09-15 — E08 rejected-queue restoration definition

- Restore `src/main.c`, `tools/monitor.py`, and `tools/test_monitor.py` from
  accepted restoration commit `487737e`, removing candidates 94/95 while
  preserving their ledger entries and logs. Build both architectures and
  require exact accepted E09-c UF2 hashes before relying on prior hardware
  validation; otherwise run fresh paired hardware tests.

### E08 rejected-queue restoration — pass, accepted baseline restored

- Restoration commit `9962a36` passes all eight host monitor tests and both
  warning-free builds at source identity `71b48132e899`. ARM/RISC-V UF2 hashes
  are exactly the accepted E09-c values
  `503e315bed9cf5ef334500645044a121a6a0e123a07fbd1f9d3ec4d9a036b5c5`
  and `6fd2b7f053cac2f4ed8b2e8b94626892fbc31ff01e0e69cbf8bdb92165a646b6`.
- The accepted RISC-V artifact was flashed to replace rejected candidate 95.
  Run `30004927-0000000e` passed all eight suites, eight windows and strict
  capture at stock 150 MHz, temperature disabled. First-seven medians were
  aggregate **368,474 H/s**, hardware **338,422 H/s**, and software **30,050
  H/s**, matching the accepted parent within 0.01%.
- Complete restoration log SHA-256:
  `b6fb5e3f5bf9330eea6e2dcc17d9f28cd8bde57cd04fd1e14ff6958cf81cbb9a`.
  Archived log: `logs/E08-queue-restored-riscv.log`. The board is left on this
  accepted stock RISC-V image.

## 2026-09-15 — E04 exact 32-bit software counter candidate 96 definition

- Parent is restored E09-c at source identity `71b48132e899`. The software
  parity worker increments a 64-bit total in every hot iteration, although it
  explicitly stops when its odd nonce wraps after exactly `2^31` evaluations.
- Change only the live `software_hashes` counter to `uint32_t` and its two
  direct print formats. Window snapshots/deltas, rate arithmetic and combined
  totals remain 64-bit through existing promotion. No batch, polling,
  telemetry, nonce allocation, clock or temperature behavior changes, and the
  complete reachable count is exactly representable.
- Expected removable cost is one 64-bit increment/carry sequence per software
  evaluation, especially on Hazard3. There is no BSS cost. Reject if emitted
  hot work does not decrease, either ISA regresses materially, accounting
  changes, or any validation gate fails.

### E04 exact 32-bit software counter ARM attempt 96a — pass, reject variant

- Candidate commit `30210e8`, source identity `2a603214b584`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-0000000f`. Both clean
  builds, all eight suites, eight common windows and strict capture passed.
- Standalone hardware/full/filter rates were **331,087 / 30,364 / 31,578
  H/s**. First-seven common-window medians were aggregate **356,521 H/s**
  (range 356,512–356,525), hardware **325,125 H/s** (325,119–325,130), and
  software **31,395 H/s** (31,393–31,397). Versus E09-c these are effectively
  neutral: aggregate **-10 H/s (-0.00%)**, hardware **+2 H/s (+0.00%)**, and
  software **-13 H/s (-0.04%)**.
- Text decreased 16 bytes to 188,664 and BSS remained 4,708 bytes, but the M33
  mining loop still emits a two-instruction `adds`/`adc` carry chain for the
  live count. Thus the hypothesized hot work was not removed; the size change
  is cold formatting/layout and does not justify retaining the source change.
- ARM UF2 SHA-256:
  `936881ef2009965ea93174c39e2812fb33616425d0f476701ca3f5e93f303ff8`.
  Complete log SHA-256:
  `d2276bcdd3a62a1367174eefa8cefb85c68f610f038591d8ee11b76aeb0de298`.
  Archived log: `logs/E04-software-count32-arm.log`.

### E04 exact 32-bit software counter RISC-V attempt 96b — pass, reject variant

- The identical candidate ran as `30004927-00000010`; all eight suites, eight
  common windows and strict capture passed. Standalone hardware/full/filter
  rates were **344,785 / 28,636 / 30,235 H/s**.
- First-seven medians were aggregate **368,316 H/s** (range 368,270–368,339),
  hardware **338,277 H/s** (338,236–338,299), and software **30,038 H/s**
  (30,035–30,040). Versus E09-c these are aggregate **-172 H/s (-0.05%)**,
  hardware **-157 H/s (-0.05%)**, and software **-13 H/s (-0.04%)**: neutral,
  with no measurable gain.
- Text decreased 28 bytes to 200,844 and BSS remained 4,440 bytes. Hazard3
  nevertheless still emits `addi`/`sltu`/`add` for a low-word increment and
  carry, so the exact hot-loop rejection rule is met on both ISAs. Reject
  candidate 96 and restore the accepted E09-c counter/format contract.
- RISC-V UF2 SHA-256:
  `a17fe5ef2c1d47854a3055bd906a85a7bf094d315ed4755ab04dbc606c85f34b`.
  Complete log SHA-256:
  `3de4abc163f7ac783714920a0937f492cd09939a06bb197fcef0782f15e968bd`.
  Archived log: `logs/E04-software-count32-riscv.log`.

### E04 rejected-counter restoration — pass, accepted baseline restored

- Restoration commit `ba39b4c` returns the live software counter and both
  direct print formats to the accepted 64-bit contract. All eight host monitor
  tests pass and both architectures build warning-free at source identity
  `71b48132e899`.
- ARM/RISC-V UF2 hashes are exactly the accepted E09-c values
  `503e315bed9cf5ef334500645044a121a6a0e123a07fbd1f9d3ec4d9a036b5c5`
  and `6fd2b7f053cac2f4ed8b2e8b94626892fbc31ff01e0e69cbf8bdb92165a646b6`.
  Prior paired hardware validation therefore remains applicable. The board is
  still running rejected candidate 96 RISC-V and must be replaced by the next
  validated candidate or an accepted recovery image before handoff.

## 2026-09-15 — E04-f approximately one-second telemetry candidate 97 definition

- Parent is the byte-identical restored E09-c/E06-trigger source at commit
  `ba39b4c`, identity `71b48132e899`. Accepted first-seven common medians are
  ARM **356,531 H/s** (hardware 325,123, software 31,408) and RISC-V
  **368,488 H/s** (hardware 338,434, software 30,051), all at stock 150 MHz
  with temperature disabled.
- Change only `MINING_REPORT_INTERVAL` from 100,000 to 340,000 hardware hashes.
  At the retained worker rates this changes progress telemetry from about
  3.25–3.38 reports/s to about 0.96–1.00 reports/s. Keep the 16-report common
  window unchanged, so each measurement window becomes approximately
  16–17 seconds. Error checks remain per hash/report boundary as before;
  candidate/share handling, SHA/DMA waits, FIFO record shape, and window ACK
  semantics are unchanged.
- Hypothesis: less frequent FIFO serialization and USB formatting can return
  useful core/bus time while keeping roughly one-second progress visibility.
  The expected serial progress-line rate falls by about 70%; exact bytes/s
  will be calculated from archived logs. This is a code/cadence experiment at
  the stock clock, not a clock experiment.
- Both ISAs must build and pass host tests before either flash. Because windows
  are longer, capture at least five complete synchronized windows and prefer
  seven. Reject any correctness/capture/fault issue or material aggregate
  regression; retain a small gain only if it is reproducible and has a clear
  reduced-telemetry cost explanation.
- Pre-flash gates: both normal wrapper builds pass warning-free and all 8 host
  monitor tests pass. Dirty-build size is ARM 188,680 text / 4,708 BSS and
  RISC-V 200,876 / 4,440 bytes. Disassembly preserves the per-hash increment,
  target/error checks and one compare/branch to the report path; only the
  materialized comparison constant changes to 340,000 (Hazard3 `0x53020`, ARM
  literal load). Clean committed images will be rebuilt and hashed before the
  first flash.

### E04-f one-second cadence ARM attempt 97a — pass, paired result pending

- Candidate commit `c41523a`, source identity `010c23e20b3f`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000011`. Both clean
  architecture builds passed before flash. All 8 suites, the 4,096-case
  oracle, standalone benchmarks, seven long common windows, and strict capture
  passed without a fault.
- Standalone hardware/full/filter rates were **331,067 / 30,364 / 31,578
  H/s**, matching the accepted algorithms. Seven-window medians were aggregate
  **356,602 H/s** (range 356,601–356,602), hardware **325,169 H/s**
  (325,168–325,169), and software **31,433 H/s** (31,432–31,434).
- Versus accepted E09-c, aggregate is **+71 H/s (+0.020%)**, hardware
  **+46 H/s (+0.014%)**, and software **+25 H/s (+0.080%)**. This is a clean,
  exceptionally stable run but remains below the normal small-change retention
  threshold and is not yet attributed to the cadence change.
- Each window covers 5,440,000 hardware hashes and about 525,858 software
  hashes in 16.7297–16.7298 seconds. The serialized `MINING:START`, 16 progress
  lines and window line averaged about **305 B/s**, versus about **1,027 B/s**
  in the accepted 100,000-hash ARM log: roughly **70.3% less steady telemetry
  payload**, consistent with the intended cadence change.
- Clean size is 188,672 text / 4,708 BSS bytes on ARM and 200,876 / 4,440 on
  RISC-V. ARM/RISC-V UF2 SHA-256:
  `c56cd075e620911ce0020e726c632d5286293fbdb39d4c0205019d0561eca520` /
  `89176f5011a121a2de936fd0eef32125497eff4bd4c5ab4718146147205b43f8`.
  Complete ARM log SHA-256:
  `ffcff0a0497cc14db7c51739190a905c928015787f928ac384e4557cae84982b`.
  Archived log: `logs/E04f-cadence1s-arm.log`.
- Decision: functional pass; defer retain/reject until the identical long
  Hazard3 run. Do not infer a speedup from the +0.020% ARM movement alone.

### E04-f one-second cadence RISC-V attempt 97b — pass, reject exact variant

- The identical candidate ran at source identity `010c23e20b3f`, run ID
  `30004927-00000012`, normal profile, stock 150 MHz, temperature disabled.
  Both ISAs built before flash. All 8 suites, the 4,096-case oracle, standalone
  benchmarks, seven long windows, and strict capture passed without a fault.
- Standalone hardware/full/filter rates were **343,993 / 28,631 / 30,235
  H/s**. Seven-window medians were aggregate **368,426 H/s** (range
  368,422–368,442), hardware **338,358 H/s** (338,355–338,374), and software
  **30,067 H/s** (30,066–30,068).
- Versus accepted E09-c, aggregate is **-62 H/s (-0.017%)**, hardware
  **-76 H/s (-0.022%)**, and software **+16 H/s (+0.053%)**. Together with
  ARM's +0.020%, candidate 97 is throughput-neutral rather than a mining-speed
  improvement.
- Each window covers 5,440,000 hardware hashes and about 483,397 software
  hashes in 16.0769–16.0778 seconds. Serialized steady telemetry had a median
  of about **323.5 B/s**, versus about **1,087.5 B/s** in the accepted
  100,000-hash RISC-V log, a **70.3% reduction**.
- RISC-V UF2 SHA-256:
  `89176f5011a121a2de936fd0eef32125497eff4bd4c5ab4718146147205b43f8`.
  Complete log SHA-256:
  `64f7c2832c8e3a56d6d4803275f2927f8240157207f3db2935da5ceab595dfbf`.
  Archived log: `logs/E04f-cadence1s-riscv.log`.
- Decision: reject candidate 97 **as an exact configuration**. It reduces
  serial payload with neutral throughput, but retaining 16 reports per window
  stretches the measurement/ACK period to about 16 seconds and makes the
  repository's default 45-second strict cycle unable to collect its required
  five windows. Test one bounded operational follow-up: retain 340,000 hashes
  per progress report but group four reports per window, restoring about
  four-second windows/default-cycle compatibility. This changes only window
  grouping/host expectation and will reveal the cost of restoring the prior
  ACK cadence. Restore the accepted source if that follow-up does not preserve
  correctness and neutral throughput.

## 2026-09-15 — E04-f four-report window candidate 98 definition

- Parent is candidate 97's tested 340,000-hash progress cadence. Change only
  the common measurement/ACK grouping from 16 progress reports to four,
  producing approximately 4.0-second Hazard3 and 4.2-second ARM windows. This
  restores five-window coverage within the default 45-second cycle and keeps
  ACK cadence close to the accepted 4.7–4.9-second windows.
- Add `report_hashes` and `window_reports` to BOOT so the host validates the
  firmware-advertised sequence relationship instead of hardcoding 16. Update
  synthetic tests to the new four-report contract. This protocol metadata is
  part of the window-grouping change; mining algorithms, progress record
  shape, SHA/DMA operations, error/candidate checks, and clocks are unchanged.
- Hypothesis: retain candidate 97's approximately 70% progress-traffic
  reduction without its strict-cycle usability regression. The extra window
  line and ACK every four reports will modestly raise bytes/s from candidate
  97, but should remain far below the accepted 100,000-hash cadence. Compare
  throughput directly with candidate 97 to expose ACK/window cost and with
  E09-c for final retention.
- Pre-hardware verification at dirty identity `010c23e20b3f-dirty`: stock
  150 MHz ARM and Hazard3 repository builds both pass without warnings, and
  all eight `tools/test_monitor.py` tests pass. `tools/analyze` reports ARM
  text/BSS 188,720/4,708 bytes and Hazard3 text/BSS 200,924/4,440 bytes. This
  is 48 text bytes per ISA above candidate 97, attributable to advertising the
  reporting configuration in BOOT; data and BSS are unchanged.

### E04-f four-report window ARM attempt 98a — pass, paired result pending

- Candidate commit `e2912db`, source identity `1eb3d9edb2b0`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000013`. The cycle
  rebuilt both architectures before flashing ARM. All 8 suites, the 4,096-case
  oracle, standalone benchmarks, eight common windows, and the default
  45-second strict capture passed without a fault.
- Standalone hardware/full/filter rates were **331,085 / 30,364 / 31,578
  H/s**. The prescribed first-seven window medians were aggregate **356,545
  H/s** (range 356,423–356,587), hardware **325,092 H/s** (324,983–325,133),
  and software **31,450 H/s** (31,439–31,455).
- Versus accepted E09-c, aggregate is **+14 H/s (+0.004%)**, hardware **-31
  H/s (-0.010%)**, and software **+42 H/s (+0.134%)**. Versus candidate 97,
  aggregate is **-57 H/s (-0.016%)**, hardware **-77 H/s (-0.024%)**, and
  software **+17 H/s (+0.054%)**. Both comparisons are throughput-neutral.
- Each window covers 1,360,000 hardware hashes and about 131,570 software
  hashes in 4.1829–4.1848 seconds, restoring default-cycle coverage. Median
  serialized mining telemetry over the first seven windows is about **343.5
  B/s**: **66.6% below** the accepted approximately 1,027 B/s, and 12.6% above
  candidate 97's approximately 305 B/s because window/ACK records are four
  times as frequent.
- Clean ARM/RISC-V UF2 SHA-256:
  `5fb2d345ba0f9eb2c99ddb51ceceabf4b9c25e60ab9169812c9685d961bd54ef` /
  `f109ef25a8a89cd41ec6e4068f5407af6cae9b87708d91aa605b16b5bd404c6a`.
  Complete ARM log SHA-256:
  `72945f5ef90e6e26055c13d716edefd2e0b9330207b3cace352951afd3795e4a`.
  Archived log: `logs/E04f-cadence1s-window4-arm.log`.
- Decision: functional and operational pass; defer retention until the
  identical Hazard3 run. ARM supports the hypothesis that the shorter window
  restores strict-cycle usability without a measurable throughput cost.

### E04-f four-report window RISC-V attempt 98b — pass, retain candidate 98

- The identical committed candidate ran at source identity `1eb3d9edb2b0`,
  run ID `30004927-00000014`, normal profile, stock 150 MHz, temperature
  disabled. The cycle rebuilt both architectures before flashing Hazard3.
  All 8 suites, the 4,096-case oracle, standalone benchmarks, eight common
  windows, and the default 45-second strict capture passed without a fault.
- Standalone hardware/full/filter rates were **344,783 / 28,636 / 30,235
  H/s**. The first-seven window medians were aggregate **368,378 H/s** (range
  368,375–368,381), hardware **338,300 H/s** (338,296–338,303), and software
  **30,078 H/s** (30,077–30,082).
- Versus accepted E09-c, aggregate is **-110 H/s (-0.030%)**, hardware **-134
  H/s (-0.040%)**, and software **+27 H/s (+0.090%)**. Versus candidate 97,
  aggregate is **-48 H/s (-0.013%)**, hardware **-58 H/s (-0.017%)**, and
  software **+11 H/s (+0.037%)**. Both paired results establish that the
  cadence/grouping change is throughput-neutral.
- Each window covers 1,360,000 hardware hashes and about 120,918 software
  hashes in 4.0200–4.0202 seconds. Median serialized mining telemetry over the
  first seven windows is about **363.4 B/s**: **66.6% below** the accepted
  approximately 1,087.5 B/s and 12.3% above candidate 97's approximately
  323.5 B/s due to the more frequent window/ACK records.
- Clean image hashes remain ARM
  `5fb2d345ba0f9eb2c99ddb51ceceabf4b9c25e60ab9169812c9685d961bd54ef`
  and RISC-V
  `f109ef25a8a89cd41ec6e4068f5407af6cae9b87708d91aa605b16b5bd404c6a`.
  Complete RISC-V log SHA-256:
  `f9edd141f18a52a6b393397c67c34b1a617416e601da6b883d8ef39e6d6dd6d2`.
  Archived log: `logs/E04f-cadence1s-window4-riscv.log`.
- **Decision: retain candidate 98.** It delivers the measured approximately
  66.6% serial-payload reduction on both architectures, keeps roughly
  one-second progress visibility, restores approximately four-second
  synchronized windows and default-cycle compatibility, and has no measurable
  throughput penalty. The BOOT-advertised reporting contract prevents the
  host validator from silently assuming a stale grouping.

## 2026-09-15 — E07 ARM exact-filter scratch-X candidate 99 definition

- Parent is retained candidate 98 at source identity `1eb3d9edb2b0`, stock
  150 MHz and temperature disabled. Change only the ARM placement of
  `software_sha256_digest_high_word_after_round61`: move its 1,712-byte emitted
  body from the striped main-SRAM `.time_critical` region to scratch X. Keep
  Hazard3's 2,646-byte helper in main SRAM because it cannot fit alongside the
  fixed core-1 stack.
- Current maps place all software compression helpers at the start of main
  SRAM: ARM filter `0x20000360`, Hazard3 filter `0x20000530`. Both scratch
  data sections are empty; the linker reserves a 2,048-byte core-1 stack in
  scratch X and a 2,048-byte core-0 stack in scratch Y. Candidate ARM usage is
  therefore 1,712 bytes of scratch-X code plus the unchanged 2,048-byte stack,
  leaving 336 bytes unused in the 4 KiB bank. Core-1 stack/IRQ capacity remains
  2,048 bytes; scratch Y and core-0 stack capacity are unchanged.
- Hypothesis: separating core 0's hottest instruction stream from striped
  main SRAM can reduce instruction/data or cross-master contention. Cost: ARM
  instruction fetches now share scratch X with the hardware worker's stack;
  this may instead slow either worker. SHA algorithms, schedules, data
  placement, clocks, telemetry and error checks are unchanged.
- Rejection rule: reject on any correctness fault, link overflow, material
  hardware-worker regression, or aggregate result that is not repeatably
  better than candidate 98. Hazard3 is a build/artifact control and should not
  be reflashed unless its loadable image unexpectedly changes.
- Pre-hardware gates at dirty identity `1eb3d9edb2b0-dirty`: both repository
  builds pass warning-free and all eight host tests pass. The ARM map places
  the unchanged 0x6b0-byte helper exactly at `0x20080000..0x200806b0`; the
  fixed stack remains `0x20080800..0x20081000`, confirming the 0x150-byte gap.
  Hazard3 retains its helper at `0x20000530` in `.time_critical`. Total
  text/BSS remains ARM 188,720/4,708 and Hazard3 200,924/4,440 bytes, so this
  candidate adds no code or static RAM and changes only ARM's load/run address.

### E07 ARM exact-filter scratch-X attempt 99a — pass correctness, reject

- Candidate commit `6788e70`, source identity `0f4021180475`, normal profile,
  stock 150 MHz, temperature disabled, run ID `30004927-00000015`. The cycle
  rebuilt both architectures before flashing ARM. All 8 suites, the 4,096-case
  oracle, standalone benchmarks, eight windows, and strict capture passed
  without a fault.
- Standalone hardware/full/filter rates were **331,080 / 30,364 / 31,538
  H/s**. The first-seven window medians were aggregate **356,146 H/s** (range
  356,144–356,148), hardware **325,051 H/s** (325,045–325,053), and software
  **31,095 H/s** (31,095–31,099).
- Versus retained candidate 98, aggregate is **-399 H/s (-0.112%)**, hardware
  **-41 H/s (-0.013%)**, and software **-355 H/s (-1.129%)**. The isolated
  filter also loses 40 H/s (-0.127%). The stable, component-local loss supports
  the contention hypothesis: core 0 fetching the filter from scratch X
  conflicts with core 1's stack/hardware-worker accesses instead of helping
  striped-main-SRAM traffic.
- Candidate ARM/RISC-V UF2 SHA-256:
  `45fcfc448a7c2173d1e97d14b2a9a3a7803e77f3e09a49b830caa597b432d2c7` /
  `3ab735d34881f142fe9f824a6c161763c117c19c8fd1b97a804c8f17755440fd`.
  Complete ARM log SHA-256:
  `daf3c632ab6df7c14f0bfe23793369360790ce1cbdc04f980f5ca2b96a7c4adb`.
  Archived log: `logs/E07-filter-scratchx-arm.log`.
- **Decision: reject candidate 99.** Restore the default `.time_critical`
  placement, rebuild both architectures, and require candidate 98's exact UF2
  hashes before advancing. Do not test the larger Hazard3 helper in scratch X;
  it does not fit the established stack budget.

### E07 ARM exact-filter placement restoration after candidate 99

- Restoration commit `8a89427` removes only candidate 99's conditional
  placement macro and returns the helper to the retained `.time_critical`
  section. Both stock repository builds pass warning-free and all eight host
  tests pass at restored source identity `1eb3d9edb2b0`.
- ARM UF2 SHA-256 is exactly the retained candidate-98 hash
  `5fb2d345ba0f9eb2c99ddb51ceceabf4b9c25e60ab9169812c9685d961bd54ef`;
  RISC-V is exactly
  `f109ef25a8a89cd41ec6e4068f5407af6cae9b87708d91aa605b16b5bd404c6a`.
  Existing attempts 98a/98b therefore remain the hardware-validation basis;
  no redundant restoration flash is claimed as a new measurement.

## 2026-09-15 — E03 software batch-8 API candidate 100 definition

- Parent is retained candidate 98, source identity `1eb3d9edb2b0`, stock
  150 MHz and temperature disabled. Final disassembly has a real call to
  `software_bitcoin_hash_nonce_high_word_be` for every core-0 nonce on both
  ISAs. Add a software-SHA translation-unit batch entry point that evaluates
  eight stride-two nonces and returns an exact bitmask of zero-high-word
  candidates. The mining loop invokes it once per eight hashes.
- A set bit is not accepted as a share: core 0 recomputes that nonce's complete
  double-SHA-256 digest and performs the existing full ordered target compare.
  Add batch-mask coverage around the known genesis winner to the existing
  mining-decision KAT. Hash counts advance exactly by eight, and the existing
  odd-range wrap check remains exact because the final batch is
  `0xfffffff1..0xffffffff` and advances back to one.
- Hypothesis: amortize the outer call/loop setup, keep invariant hasher/nonce
  state live within `software_sha256.c`, and reduce steady FIFO-status polling
  from every hash to every eight. Worst-case hardware-message service latency
  is bounded to eight software filters, about 0.27 ms at retained rates, far
  below the roughly one-second progress cadence. No hardware-owner code, SHA
  algorithm, clock, telemetry cadence, or error/candidate semantics changes.
- Rejection rule: reject any oracle/KAT/share/count failure, meaningful
  hardware-worker loss, or less than a repeatable software/aggregate gain on
  either ISA. Inspect emitted code and flash one ISA at a time only after both
  builds pass; this candidate is shared source and requires paired evidence.
- Pre-hardware gates at dirty identity `1eb3d9edb2b0-dirty`: both stock builds
  pass warning-free and all eight host tests pass. Disassembly shows the batch
  loop is compact rather than eight-way expanded, but the compiler inlines the
  former high-word wrapper into it: one 252-byte ARM / 48-byte Hazard3 schedule
  frame is allocated per batch, then each nonce directly calls only the
  header-tail and filter SRAM kernels. This removes seven outer calls and
  repeated wrapper/frame setup per eight hashes. Text grows by 120 bytes on
  ARM (188,840 total) and 172 on Hazard3 (201,096); BSS stays 4,708/4,440.
