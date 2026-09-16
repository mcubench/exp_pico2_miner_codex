# RP2350A Bitcoin miner: remaining optimization options after candidate 104

Date: 2026-09-16. Evidence cutoff: retained candidate 103 and rejected candidate 104, as recorded in [perf_progress.md](perf_progress.md) and [.codex/HANDOFF.md](.codex/HANDOFF.md).

This is an analysis and prioritization document. No firmware was flashed and no on-device measurements were made while preparing it. Proposed gains are hypotheses or arithmetic bounds, not results.

## 1. Current retained baseline

Candidate 103 is the source baseline:

- source identity `104da455bdbb`;
- ARM hardware-worker batch factor 4;
- Hazard3 hardware-worker batch factor 1;
- ARM: 359,558 aggregate, 328,122 hardware, and 31,435 software evaluations/s;
- Hazard3: 368,377 aggregate, 338,299 hardware, and 30,079 software evaluations/s;
- stock 150 MHz;
- current ARM and Hazard3 images build from the restored candidate-103 source after candidate 104 was rejected.

Candidate 104 moved the complete ARM hardware worker from XIP into ordinary SRAM. It regressed aggregate throughput by 0.261%, hardware throughput by 0.096%, and software throughput by 1.972%. That result is important: reducing XIP execution is not automatically beneficial when it adds contention with the software worker.

The current component shares set the optimization budget:

| Architecture | Hardware share | Software share | Aggregate gain from 1% faster hardware | Aggregate gain from 5% faster software |
| --- | ---: | ---: | ---: | ---: |
| ARM | 91.26% | 8.74% | about 0.91% | about 0.44% |
| Hazard3 | 91.84% | 8.16% | about 0.92% | about 0.41% |

Consequently, hardware-worker work remains the best route to a measurable aggregate increase. Software SHA work is still worthwhile when it is small and architecture-specific, but even a substantial 10% software improvement would add only about 0.87% ARM or 0.82% Hazard3 aggregate throughput if hardware performance were unchanged.

## 2. What has changed since `planned_optimizations_update.md`

The previous update is useful history, but several of its priority items are now complete. The following should not be presented as untested work:

- E01 identity, rare-path tests, and common-window measurement contract were implemented and retained.
- E02 intrusive profiles were captured for ARM and Hazard3.
- E06-trigger was retained on Hazard3; it removed one per-nonce DMA control write.
- E09 fixed header-tail rounds were retained only on ARM.
- E09 terminal live-result filtering was retained on both architectures.
- E09 explicit tail schedule words W20-W30 were rejected on both architectures.
- E14 dynamic nonce chunks, including 4 KiB, native 32-bit, 65,536-entry, and nested variants, regressed and were restored.
- E08 SRAM telemetry queues with polling every hash and every 64 hashes regressed and were restored.
- E04 32-bit software accounting regressed and was restored.
- E04 one-second reporting with four reports per common window was retained.
- E07 ARM filter placement in scratch X regressed and was restored.
- E03 software batch factor 8 was rejected: ARM was neutral and Hazard3 lost aggregate throughput despite a small software gain.
- E04 hardware batching was resolved to ARM factor 4 and Hazard3 factor 1. ARM factor 2 helped, factor 4 helped again, and Hazard3 factor 2 was effectively neutral.
- E07 placement of the ARM hardware worker in ordinary SRAM was rejected as candidate 104.

The remaining list below deliberately excludes unchanged retries of those experiments.

## 3. Static evidence that guides the next work

The retained ARM `mining_worker_core1` is approximately 2,196 bytes at batch factor 4, compared with about 1,028 bytes for the Hazard3 factor-1 worker. Static ARM disassembly contains 27 calls to `multicore_fifo_push_blocking`. The count includes initialization and reporting, but the four physically duplicated nonce bodies also duplicate rare share/candidate publication and comparison paths.

That code shape suggests that the next ARM batching gain may depend less on another loop pragma and more on preventing cold code from being copied with every hot nonce body.

The retained software SHA implementation already has a large SRAM code footprint:

| Architecture | Compression | Exact filter | Full digest | Header tail | Approximate total |
| --- | ---: | ---: | ---: | ---: | ---: |
| ARM | 592 B | 1,712 B | 1,696 B | 1,924 B | 5,924 B |
| Hazard3 | 1,328 B | 2,646 B | 2,898 B | 2,892 B | 9,764 B |

The current software path expands the message schedule and then executes rounds. It uses an inline pointer-style round helper and compiler loop unrolling. ARM benefits from explicit fixed header-tail rounds 4-15; the equivalent Hazard3 shape regressed. Any further source transformation must therefore be evaluated separately by architecture and must first justify itself in final disassembly.

The E02 profile predates the final ARM factor-4 worker, but it still constrains the search:

| Stage | ARM cycles/hash | Hazard3 cycles/hash |
| --- | ---: | ---: |
| Setup/start | 13.08 | 9.28 |
| First feed | 82.28 | 87.01 |
| Tail feed plus first digest wait | 194.14 | 194.88 |
| Digest handoff plus second hash | 173.57 | 164.97 |
| Target/error handling | 55.32 | 26.49 |
| Profiled stage total | 518.38 | 482.63 |
| Wall total | 545.54 | 528.96 |

The approximately 27 ARM and 46 Hazard3 unclassified cycles include loop/control and profiling boundaries; they are not automatically removable. Hazard3's retained one-write DMA trigger already recovered about 2 cycles/hash, so another DMA-control proposal needs comparably concrete static evidence.

## 4. Tier A: best remaining throughput experiments

### A1. Split the ARM factor-4 worker into a minimal hot body and shared cold paths

This is the strongest next source experiment.

Keep the four physical nonce bodies and their per-nonce SHA synchronization, ARM error checks, nonce progression, and generic target semantics. Move only rare or report-boundary work behind shared helpers or shared labels where C control flow can express it safely:

- candidate digest capture and general target comparison after the common high-word rejection;
- share record construction/publication;
- nonce-exhaustion and fault publication;
- report-boundary counter publication, where it is not already shared.

The purpose is not to make rare events faster. It is to reduce the amount of cold code duplicated four times, improve hot-code locality, and make a later factor-8 experiment affordable.

Static preflight gates:

1. The common no-share path must gain no helper call, extra load, or extra conditional branch.
2. The candidate path must still preserve all eight digest words until it completes the exact generic comparison.
3. ARM's per-nonce SHA error observation must not be batched or deferred; earlier sticky-error batching regressed on ARM.
4. The worker's text size should fall materially from 2,196 bytes, or the hot region should become visibly denser in the final linked disassembly.
5. Stack frame and spills on the common path must not grow.
6. Candidate, fault, exhaustion, and partial-report paths must remain representable by existing host tests.

Expected value: uncertain but credible. The direct gain may be small; its larger value is enabling factor 8 without duplicating all cold machinery. This should be tested as its own candidate before adding more unrolling so its effect remains attributable.

### A2. Try ARM hardware batch factor 8 after the hot/cold split

ARM factor 1 to 2 and 2 to 4 both improved hardware throughput, while Hazard3 factor 2 was neutral. Factor 8 is therefore viable only for ARM. The report interval of 340,000 nonces is exactly divisible by 8, so it need not introduce a partial inner group in the steady report loop.

The experiment should use eight explicit physical nonce bodies, not a compiler-emitted inner counter. It should preserve candidate publication, error checks, exact nonce increments, final partial accounting, and exhaustion behavior after every logical nonce.

Static rejection gates before any future hardware run:

- reject if the compiler leaves an inner batch branch/counter;
- reject if new spills or a larger frame appear on the hot path;
- reject if the fast path contains calls introduced by the cold split;
- reject if text growth is dominated by duplicated candidate/report machinery;
- reject if the worker approaches a placement boundary that changes unrelated hot code without the change being documented.

Do not extrapolate the factor-4 gain numerically. Its improvement may partly be a favorable layout effect, and instruction-fetch pressure will eventually outweigh saved loop/report overhead. If A1 cannot reduce the duplicated cold footprint, factor 8 becomes a lower-confidence standalone experiment.

### A3. Test narrow ARM worker alignment/layout variants on the best factor-4 or factor-8 shape

The positive batching sequence and the occasional source-unchanged performance movement show that layout can matter. A focused alignment experiment is viable after the worker structure is settled:

- align only `mining_worker_core1` or its hot entry to a few documented boundaries such as 16, 32, and 64 bytes;
- inspect final addresses, padding, branch reach, and the linked bytes for every variant;
- keep source semantics identical;
- change one placement variable at a time.

This is not a retry of the rejected global `-falign-labels=32:31` experiment, which regressed ARM sustained throughput by 4.49%. Broad label alignment must remain closed. Layout-only wins are fragile and must be revalidated after any later link change.

## 5. Tier B: bounded software-SHA experiments

### B1. Replace pointer-style rounds with rotated-role four-round groups

The current round helper updates eight state variables through pointer arguments. Even when inlined, that source form can encourage moves, repeated role reassignment, or suboptimal live ranges. A four-round group can rotate the logical roles of `a` through `h` at compile time so each round writes one physical variable and the next round consumes the rotated naming directly.

Start with the exact second-hash filter because it executes for every software nonce. Use architecture-specific implementations and compare four-round groups before considering eight-round groups. Keep the current early terminal-word result and exact candidate fallback.

Static gates:

- fewer instructions or state moves in the actual round region;
- no additional stack traffic or larger frame;
- bounded code growth relative to the already large SRAM helper;
- no change to unsigned arithmetic or rotate semantics;
- all current known-answer and host-oracle cases remain applicable.

A 5% filter improvement is worth only about 0.4% aggregate, so reject a complex source expansion unless it creates a clear final-code improvement.

### B2. Interleave schedule generation and rounds in small groups

The expanded schedule was a major retained win, so do not replace it with the old circular schedule. A narrower experiment is to compute a small group of future words, store them for later dependencies, and immediately consume the words whose rounds are ready while their values are still live. This may avoid some write-then-reload traffic without losing the compiler-friendly expanded array.

Begin with the exact filter's second compression and one four-word group. Expand only if disassembly shows fewer loads/stores without spills. ARM and Hazard3 need separate shapes because earlier schedule fusion helped ARM by about 0.53% in the filter and hurt Hazard3 by 2.31%.

Reject when register pressure grows, address arithmetic replaces simple indexed access, or total helper size rises without a clear common-path instruction reduction.

### B3. Apply local code-generation variants to one helper at a time

Whole-image LTO failed, source-only LTO was neutral, broad flags were ineffective, and `restrict` was neutral. A smaller search remains defensible:

- compare local O2/O3/Os attributes on only the exact filter or header-tail helper;
- compare a single `noinline`/`always_inline` boundary where it changes the final caller;
- try a small local loop-alignment choice only after locating the actual loop in the linked image;
- try an equivalent Ch or Maj expression only when it emits different instructions.

Identical final code is a build-only no-op and should not become a device experiment. This work ranks below B1/B2 because generic compiler switches have already had many chances to help.

## 6. Tier C: conditional hardware and memory experiments

### C1. Reconsider the Hazard3 DMA ring only if it removes something beyond E06-trigger

E06-trigger already restores the source address and starts the fixed 16-word transfer with one MMIO write. A 64-byte DMA read ring is viable only if final disassembly shows an additional instruction/control reduction, a better alignment, or a measurable setup dependency that the retained trigger does not solve.

Static requirements:

- fixed 64-byte alignment and exact ring configuration;
- no per-nonce register writes added elsewhere;
- no extra DMA completion or source-lifetime hazard;
- an explicit instruction/MMIO difference from the retained implementation.

If the ring merely expresses the same one-write trigger differently, close it as a no-op without hardware work.

### C2. Try true SRAM-bank partitioning, not another broad SRAM move

The scratch-X filter and ordinary-SRAM hardware worker both regressed. Those failures show contention, but they do not prove that every bank-aware placement is useless. A future E07 experiment must first map:

- software code and data accesses;
- both core stacks;
- DMA source buffers;
- shared progress/candidate records;
- the documented striped SRAM and scratch-bank boundaries.

Only then place one resource into a physically distinct bank with explicit stack and interrupt headroom. Do not place a hot worker beside another core's hot code/data and call that bank separation. Do not reuse scratch X beside the fixed core-1 stack. This is higher effort and lower confidence than A1/B1 because candidate 104 demonstrates that a plausible placement can slow both workers through shared-bus effects.

### C3. Overlap independent software work with a proven hardware wait only after refreshing the profile

The old E10-d concept remains theoretically viable: execute a very small, bounded software-SHA group during a hardware wait that cannot be shortened, then service the peripheral before its next readiness point. It should not be attempted from the old aggregate profile alone.

Required evidence first:

- a refreshed candidate-103/next-candidate stage profile;
- a stable wait long enough for useful work with a conservative deadline;
- proof that software work never delays a ready SHA peripheral;
- no conflict with the independent core-0 software worker or SRAM placement.

This is complex scheduling for a small possible gain and belongs after the simpler code-shape experiments.

## 7. Useful-work and lifecycle improvements that may not raise short-window H/s

### D1. Two-phase nonce allocation with a large tail handoff

The fixed parity split is extremely cheap in the hot path, but the hardware worker consumes its 2^31 even nonces much earlier than the software worker consumes all odd nonces. At roughly 338 kH/s, the hardware parity range lasts about 106 minutes. The software worker has processed only a small fraction of its odd range by then, so the complete job cannot use both workers efficiently through full-space exhaustion.

The rejected dynamic-chunk experiments added allocation overhead to every hot chunk. A lower-overhead alternative is:

1. retain the current even/odd hot loops for the normal phase;
2. when the hardware even range exhausts, stop and acknowledge the software worker at a bounded checkpoint;
3. snapshot its exact odd frontier;
4. transfer the remaining unprocessed odd suffix to the hardware worker as one or a few very large ranges;
5. preserve uniqueness, exact final accounting, candidates, cancellation, and fault propagation.

This will not improve ordinary short-window H/s. It improves complete nonce-space latency and avoids the current end-of-range imbalance without reintroducing frequent shared allocation checks.

### D2. Cache immutable job setup for rapid job replacement

If real workloads replace jobs frequently, cache or precompute only values proven invariant for the relevant header prefix and target dispatch. The current steady-state measurements largely amortize setup, so the benefit would be lower job-start latency rather than a higher long-run hash rate.

Generation-tagged ownership remains essential: no cached midstate, target classification, or header words may cross a job change incorrectly. This is worth doing only with a representative job-change benchmark.

### D3. Improve control snapshots/coalescing for robustness, not throughput

The two attempted telemetry queues regressed because their polling and shared-memory traffic entered the hot path. Do not retry those queue shapes. A generation-tagged control snapshot or coalesced progress record may still be useful for stop/replace/fault robustness if it is read only at existing report or cancellation boundaries. Candidate/share delivery must remain lossless.

## 8. Separate configurations and research options

### Clock qualification

Earlier source states completed short runs at 200, 250, and 300 MHz, with 300 MHz producing approximately 703,987 ARM and 728,213 Hazard3 aggregate evaluations/s. These are the largest known raw-throughput increases, but they are separate clock configurations, not stock-code optimizations. They were not thermally qualified, and the current source has not been revalidated at those clocks. No voltage or temperature claim follows from the short runs.

If clock work resumes, requalify the retained current source independently at each frequency, preserve `temperature=disabled`, and keep voltage changes outside this plan.

### Mixed-ISA execution

Using the current best component rates as pure arithmetic, Hazard3 hardware plus ARM software would be 338,299 + 31,435 = 369,734 evaluations/s, only 1,357 evaluations/s or about 0.37% above the homogeneous Hazard3 aggregate. This is an upper-bound motivation calculation, not proof that a mixed execution arrangement is practical or free. The margin is too small to justify architecture/boot/toolchain complexity before the higher-confidence work above.

### PGO or a very small assembly kernel

Profile-guided layout could automate some hot/cold separation, but toolchain integration and profile representativeness make it a late experiment. Hand assembly should likewise be limited to a proven small round group or feeder fragment after C-source variants fail. Rewriting a complete SHA path or worker would greatly enlarge the correctness and maintenance burden for an aggregate ceiling below the hardware worker's share.

## 9. Options that are no longer viable unchanged

Do not spend device time repeating these without a materially new mechanism:

- complete ARM hardware-worker placement in ordinary SRAM;
- ARM exact-filter placement in scratch X beside the core-1 stack;
- whole-image SRAM placement, constants-only SRAM placement, or Hazard3 hot-main SRAM placement;
- software batch factor 8 in its tested form;
- hardware batch factor 2 on Hazard3;
- dynamic nonce chunks in the tested 4 KiB, native-counter, 65,536, or nested forms;
- SRAM telemetry queues polled every hash or every 64 hashes;
- next-nonce precomputation;
- reusable padded blocks;
- exact-target or forced zero-MSW target specialization that narrows generic semantics;
- ARM sticky-error batching;
- broad ARM label alignment;
- whole-image or source-only LTO in the tested forms;
- generic `restrict` and broad Hazard3 flag sweeps;
- circular software schedules or unchanged Hazard3 schedule fusion;
- FIFO poll-every-64 hashing;
- a DMA ring that emits the same one-write control path as retained E06-trigger.

Negative results are part of the design: several removed one or two apparent operations but lost throughput through layout, register pressure, or inter-core memory contention.

## 10. Recommended order for future work

No on-device work is required to use this ordering. Each step begins with source and final-disassembly analysis; an unchanged or statically worse image should stop before flashing.

1. Capture the retained worker's exact hot/cold boundaries, frame, spills, calls, addresses, and size as a static control.
2. Implement A1 alone: shared ARM cold paths with factor 4. Stop if the common path grows.
3. On the best A1 shape, implement A2 factor 8 as a separate candidate. Keep Hazard3 factor 1.
4. Once the ARM worker shape is final, try A3 single-function alignment variants.
5. Try B1 four-round rotated-role groups in the exact filter, independently per architecture.
6. If B1 leaves obvious schedule reloads, try one B2 four-word interleave group.
7. Use B3 local code-generation variants only on a helper whose disassembly exposes a concrete issue.
8. Perform C1 as a static comparison; abandon it if retained E06-trigger is already equivalent.
9. Design D1 two-phase tail takeover for complete-space behavior, measuring it separately from steady-state H/s when device testing is eventually authorized.
10. Refresh E02 only before advancing C2 or C3, because those ideas depend on contention/wait evidence from the current image.

## 11. Decision matrix

| Option | Primary target | Potential aggregate value | Confidence | Effort | First rejection gate |
| --- | --- | --- | --- | --- | --- |
| A1 ARM hot/cold split | ARM hardware | Small directly; enables A2 | Medium-high | Medium | Common fast path gains any work |
| A2 ARM factor 8 | ARM hardware | Potentially measurable, diminishing returns unknown | Medium | Medium | Inner counter, spills, or excessive text |
| A3 worker alignment | ARM hardware/layout | Small and fragile | Medium-low | Low | No useful linked-layout change |
| B1 rotated-role rounds | Software filter | About 0.4% aggregate per 5% software gain | Medium | Medium | No instruction/move reduction |
| B2 schedule/round interleave | Software filter | About 0.4% aggregate per 5% software gain | Medium-low | Medium-high | More spills or address work |
| B3 local codegen | One software helper | Usually small | Low-medium | Low | Identical or larger/worse final code |
| C1 DMA ring delta | Hazard3 hardware | Very small unless another operation disappears | Low | Low-medium | Equivalent to retained one-write trigger |
| C2 true bank partition | Both workers | Unknown; contention-sensitive | Low-medium | High | No demonstrably separate hot bank/resource |
| C3 work during wait | Hardware/core scheduling | Unknown | Low | High | No refreshed safe slack interval |
| D1 tail takeover | Full-space completion | Large near exhaustion, none in short windows | High conceptually | Medium-high | Cannot prove disjoint ranges/accounting |

The practical next choice is A1, followed by A2 only if A1 produces a clean common path. B1 is the best independent fallback. C2/C3 should wait for new profiling rather than being driven by the older cycle table.
