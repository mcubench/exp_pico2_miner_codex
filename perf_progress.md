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
