# BatTester — plan of action

An STM32L476RGTx signal source for the BatV1 bat recorder.

**Written 2026-08-27. Nothing here is built yet.** Every number is either
measured here, or read from a source file that is named. Confidence figures are
given per section. Where a figure is a guess, it says so.

---

## Summary

BatV1 is a triggered recorder. A sound longer than 300 ms arrives on the PC as
**several files**, and the PC must put them back together. This box exists to
ask one question:

> Can the PC reconstruct the original sound, sample-exact, from the clips —
> and how far down in noise does that still hold?

A bench generator with a 4096-point table cannot ask it. 4096 points at 500 kSps
is 8.2 ms — not even one clip, let alone a cut.

The design is **synthesise, do not store**: flash holds only the call waveforms,
while silence, noise, gain and timing are generated into a DMA ring at run time.
Output length becomes unbounded. A 10-minute scenario costs the same flash as a
1-second one.

| | |
|---|---|
| Output | DAC1 dual, PA4/PA5 **antiphase** into BatV1's differential ADC |
| Rate | 1.000 MSps from TIM6, DMA1 circular |
| Ground truth | GPIO sync line into a BatV1 EXTI that latches `session_samples()` |
| Host side | One script builds the flash table, one script verifies reconstruction |
| Blocks nothing | Steps 1–8 need no calibrated clock — see §9 |

---

## 1. What this box is for

BatV1 opens a clip when a band goes over threshold, closes it after **8 frames
(8.19 ms)** of quiet, and **cuts the clip at 293 frames (150016 samples,
300.032 ms)** whether or not the sound has stopped. After **5** consecutive cuts
it enters `Backoff` and refuses to open a clip for about **5 s**.

Two requirements follow, and they set the whole design.

1. **Unbounded length.** The source must emit for minutes, not milliseconds.
   Otherwise it cannot force a cut, and the interesting case never happens.
2. **Known ground truth.** Every emitted sample must carry a number the PC can
   compare against. A generator that free-runs proves nothing.

*Confidence 95%. Constants from `BatV1/todo/claude.md` §1 and
`BatV1/App/Inc/detector.hpp`.*

### A note on the wiring

PA4 and PA5 are **DAC1_OUT1 and DAC1_OUT2**, not ADC pins. The `.ioc` is
correct; only the wording in the brief was. This box outputs, BatV1 inputs.
Nothing to change.

---

## 2. Architecture — synthesise, do not store

> **Built differently, 2026-08-29: the output is single-ended, not antiphase.**
>
> PA5 is `RUN_LED` (LD2) in `BatTester.ioc`, DAC channel 2 is never configured,
> and `synth.cpp` writes `DAC1->DHR12R1` — one channel, one 12-bit word. The
> dual `DHR12RD` drive described below and drawn in the diagram does not exist.
>
> On the BatV1 side **PC1 is tied to GND**, so its differential ADC measures
> PA4 against ground. Three consequences, all accepted:
>
> - Mid-code is **12288**, not the 8192 `adc_capture.hpp:67` assumes. Every
>   stored sample carries +4096 counts (+16384 after the 4× oversample) of DC,
>   and the signal never crosses zero.
> - Only the upper half of the differential range is used.
> - Common mode is 0.825 V nominal where the ADC wants Vref/2.
>
> Detection and reconstruction are unaffected: the offset is constant, it lands
> in FFT bin 0, the bands start at 1 kHz (`detector.cpp:459`), and the adaptive
> floor absorbs it. Expect dumps and WAV exports to look DC-shifted; that is
> this, not a bug. Two resistors making 1.35–1.65 V for PC1 would remove all
> three at once if it ever matters.
>
> The rest of this section — the synthesise-don't-store split, the ring, the
> budget — is what was built and is accurate.

The reference recording at 500 kSps is 1.2 MB. The L476RG has 1 MB of flash.
Storing whole passes does not fit, and would not scale if it did.

But a bat pass is **mostly silence**. The reference file is 1.202 s long and
holds about **250 ms of actual call**. So split the job:

| Part | Where it lives | Cost |
|---|---|---|
| Call waveforms | `const` table in flash, built by a host script | ~250 ms of audio |
| Silence, noise, gain, timing | Generated on the fly into a DMA ring | 0 bytes |

### Signal chain

```
host script ──► wave_table.c ──► flash
                                   │
                       ┌───────────┴───────────┐
                       │  fill half-buffer:    │   HT/TC interrupt,
                       │  call + noise * gain  │   2048 samples, every 2.05 ms
                       └───────────┬───────────┘
                                   ▼
        TIM6 @ 1.000 MHz ──► DAC dual (DHR12RD) ──► DMA1 circular, 4096 x u32
                                   │
                    PA4 ──┐        └──┐ PA5     antiphase: mid+s / mid-s
                          ▼           ▼
                    attenuator + 2-pole RC 180 kHz, VCM 1.35 V
                          │           │
                    PC0 ──┘           └── PC1   BatV1 ADC1_IN1/IN2, differential
```

**Budget.** 80 MHz core. Fill cost is one flash read, one xorshift, one multiply
and one add per sample; at 1 MSps that is roughly 15 % of the core. The DMA
buffer is 4096 × 4 B = 16 KB of 128 KB RAM. The call library is about 500 KB of
1 MB flash.

**Why antiphase.** BatV1's ADC1 is 14-bit **differential**. Driving PA4 and PA5
in antiphase gives a true differential source, rejects common-mode noise, and
doubles the swing for free. The dual holding register `DAC_DHR12RD` writes both
channels in one 32-bit DMA transfer, so it costs nothing in bandwidth either.
This is exactly what the two already-wired pins are good for.

*Confidence 85 % on the CPU budget. The DAC settling figure in §4 is the one
number that could force a change.*

---

## 3. Ground truth — the sync latch — **DROPPED 2026-08-29**

> **This section is kept for the record. The latch is not being built.**
>
> The argument below — "correlation assumes the answer it is trying to prove" —
> is overstated, and that is why this was reversed. Correlating **one** clip
> against the known emitted waveform fits **one scalar**: the offset between
> BatTester sample 0 and BatV1's `sessionSample` axis. Every other clip is then
> a *prediction* checked against that scalar. Sixteen predictions from one
> fitted parameter is not circular reasoning.
>
> Everything the reconstruction tests actually assert lives on BatV1's own
> sample axis and needs no external mark:
>
> - `Komori/docs/stitch-plan.md` §4.3: `wrap == 0 ⇒ sessionSample ==
>   prev.sessionSample + prev.len`, exactly. A hard invariant.
> - Overlap identity (scenario I), clip counts, the backoff hole (E), pass
>   grouping (H and J).
>
> The clock ratio of §9 comes out **better** without the latch: a linear fit of
> correlation offset against `sessionSample` across all 17 clips beats a single
> latched point.
>
> Against that, the latch costs an EXTI, an ISR and a CLI command in BatV1
> firmware that is at RC1 and currently byte-matches its flash. Not worth it.
>
> **Consequence:** BatTester's PB0 SYNC output stays, and stays useful — as a
> **scope trigger**. It is not wired to BatV1.
>
> **Residual risk:** correlation needs SNR, so alignment can fail at the deep
> end of the F and G sweeps. Fit the offset from the loudest step of the same
> run and hold it fixed for the quieter ones. The latch would have needed the
> same treatment once the two clocks drifted apart.

Cross-correlating clips against the reference works, but it assumes the answer
it is trying to prove. A hardware sync line does not.

- BatTester pulls a GPIO high at scenario sample 0.
- BatV1 takes an EXTI on that pin and latches `detect::session_samples()` plus
  an edge count. **Nothing else.**
- A new BatV1 CLI command `sync` prints the latched value and the count.

Every clip already carries `session_sample` — an exact ADC sample count since
capture start, in the BAT2 header at offset 20. With the latch, the PC knows the
exact BatV1 sample index of emitted sample 0, so every reconstruction check
becomes arithmetic instead of estimation.

### It must be inert in the field

**Decided 2026-08-27: build it, on the condition that the field unit works with
nothing attached to this pin.** Four rules meet that condition.

| Rule | Effect |
|---|---|
| Internal **pull-down**, rising edge only | A floating pin cannot fire. No external part needed. |
| The ISR writes **two variables and returns** | It cannot touch the detector, the recorder, the clip queue or the card. |
| Only the `sync` command reads the latch | A spurious edge changes a number nobody types in the field. |
| Lowest NVIC priority | It cannot delay the ADC or GPDMA interrupts. |

So the worst case with the pin unconnected is a wrong number in a bench command.

### Gated on a constexpr as well

```cpp
namespace bench {
/* Bench sync latch. A field build sets this false: the EXTI is never enabled,
   the pin stays a plain input, and `sync` reports "disabled". Costs nothing
   when false — the guarded code folds away. */
inline constexpr bool kSyncLatch = true;
}
```

`if constexpr (bench::kSyncLatch)` around the pin init, the EXTI enable and the
CLI registration. Both branches are compiled by the same compiler in the same
translation unit, so the field build is the same source with one `bool` flipped,
not an `#ifdef` maze. **Keep it `true` by default** so the bench path is the one
that gets exercised, and flip it only for a field release.

**Pin: propose PD2 on BatV1** (Nucleo-U575ZI-Q). Free, EXTI-capable, on the
header, and EXTI2 is unused — PC13 holds EXTI13 for the user button, and
PB7 / PC7 / PG2 are the three LEDs. Confirm against the schematic before wiring.

**Fallback, if this is ever unwanted:** a short unique pilot chirp at scenario
sample 0, outside the four detector bands, aligned by correlation. Weaker — it
depends on the ADC hearing it, which is the thing under test.

*Confidence 90 % on the mechanism. The PD2 suggestion is 70 % — read from the
`.ioc` pin list, not from the board schematic.*

---

## 4. Analog interface

> **As wired 2026-08-29: `PA4 → PC0` direct, `PC1 → GND`.**
>
> No attenuator, no 1.35 V rail, no reconstruction filter. The resistor network
> below is not built. What that changes:
>
> | | Planned | As wired |
> |---|---|---|
> | Attenuation | −27 dB in copper | none; done in firmware by `level` |
> | Common mode | 1.35 V, mic-like | 0.825 V nominal, PC1 at GND |
> | Reconstruction filter | 2-pole ~180 kHz | none — see Q9 |
> | Differential swing | ±148 mV | ±0.814 V at the default `level -5` |
>
> **Set the level in firmware instead.** The default is now **`level -5`**,
> which gives 1010 DAC counts → 0.814 V peak → **2020 counts** at BatV1's ADC.
> That matches the 2000 counts `make_test_signal.py` used when the detector
> thresholds were tuned, which is the number that matters — there is no preamp,
> so imitating the mic's 12.6 mV rms would only mean testing at 46 dB below
> where the detector was calibrated.
>
> **Do not run at `level 0` with `snr` set.** The tone alone reaches 63560 of a
> 65535 `uint16` after the 4× oversample; adding noise clips at BatV1.
>
> The ±100 mV / 2000 counts pairing in the table below is **inconsistent** —
> 2000 counts on a 14-bit differential ADC at Vref 3.3 V is 0.806 V, not
> 100 mV. Trust the count, not the voltage: the count is what the detector was
> tuned against.

BatV1's ADC1 is 14-bit differential, PC0 = VINP, PC1 = VINN, mid-code 8192,
`SamplingTime = 5 cycles`.

**There is no preamp yet** (confirmed 2026-08-27). So direct injection at
PC0/PC1 is not a compromise — it is the only chain that exists. That makes this
box *more* useful, not less:

> BatTester is the reference source against which the preamp and the anti-alias
> filter get designed and measured, once they exist.

The tester should still imitate the *intended* source, so that nothing has to be
re-characterised when the front end arrives. That source is the Infineon
IM73A135 MEMS mic: differential out, **VCM 1.35 V**, **Zout 250 Ω per pin**,
12.6 mV rms at 94 dB SPL (`BatDetector/IM73A135.md`).

| Constraint | Number | Why |
|---|---|---|
| Common mode | 1.35 V | Matches the mic; keeps both inputs in range |
| Source impedance | ≤ ~500 Ω per pin | 5 ADC cycles is about 91 ns of sampling time |
| Differential swing | ±100 mV peak | `make_test_signal.py` uses 2000 counts ≈ 100 mV |
| Reconstruction filter | 2-pole, ~180 kHz | Kills the DAC image; matches the intended 6th-order 150 kHz front end in `BatDetector/ADC_GAIN.md` |

**Starting network, per side.** DAC → 10 kΩ series → node → 470 Ω to a stiff
1.35 V rail → PC0/PC1, with the RC across the node.

- Attenuation ≈ −27 dB
- Node impedance ≈ 449 Ω
- DAC ±1.65 V → ±74 mV per pin → **±148 mV differential** in antiphase

That is enough headroom to attenuate further in firmware rather than in copper.
Putting the divider at the far end also attenuates the DAC's own noise and the
1.35 V rail's noise by the same 27 dB, which is deliberate.

### Open risk — DAC settling

At 1 MSps the update period is 1.0 µs. The L476 buffered DAC settles in about
**1.7 µs typical** for a full-scale step. Our steps are small because the signal
is band-limited to ~120 kHz, so this should hold — **but it is an assumption**,
and it is the one number that could change the design.

**A 250 MHz / 1 GSa/s scope is available** (confirmed 2026-08-27), which settles
this properly rather than by argument: 1000 sample points per DAC update period.
Settling is directly observable, not inferred. See the Step 1 gate for what to
measure.

If it fails: drop to 500 kSps and accept about 1 dB of image error at 107 kHz,
or fit an external buffer.

*Confidence 75 % on the network. The resistor values are a starting point, not a
verified design. Everything else is from datasheets and the BatDetector notes.*

---

## 5. The source material

`Komori/FullSpectrum_Rhinolophus_hipposideros.wav` — 256 kHz, 16-bit, 1.202 s,
recorded by a third party.

Measured here: 15 kHz high-pass, 1 ms frames, threshold 8× the 20th-percentile
frame RMS. **17 calls, CF at 107.2–107.6 kHz, with a 46 kHz component on three
of them.**

| # | start ms | dur ms | gap ms | peak | f kHz |
|---:|---:|---:|---:|---:|---:|
| 0 | 297.0 | 35.0 | — | 3233 | 107.3 |
| 1 | 378.0 | 36.0 | 46.0 | 2717 | 107.4 |
| 2 | 464.0 | 16.0 | 50.0 | 1230 | 107.4 |
| 3 | 549.0 | 27.0 | 69.0 | 2801 | 107.5 |
| 4 | 713.0 | 1.0 | 137.0 | 1552 | 98.0 |
| 5 | 718.0 | 24.0 | 4.0 | 3587 | 107.5 |
| 6 | 803.0 | 20.0 | 61.0 | 3455 | 107.6 |
| 7 | 885.0 | 25.0 | 62.0 | 3150 | 107.6 |
| 8 | 969.0 | 11.0 | 59.0 | 1363 | 107.5 |
| 9 | 1003.0 | 2.0 | 23.0 | 1145 | 107.5 |
| 10 | 1016.0 | 1.0 | 11.0 | 1751 | 46.0 |
| 11 | 1047.0 | 18.0 | 30.0 | 2656 | 107.4 |
| 12 | 1070.0 | 12.0 | 5.0 | 1256 | 107.2 |
| 13 | 1094.0 | 1.0 | 12.0 | 1455 | 46.0 |
| 14 | 1129.0 | 2.0 | 34.0 | 969 | 107.5 |
| 15 | 1138.0 | 2.0 | 7.0 | 1013 | 107.5 |
| 16 | 1144.0 | 1.0 | 4.0 | 2067 | 45.0 |

This agrees with `DETECTOR_NOTES.md` (107.0–107.5 kHz, 29–61 ms calls, 19–50 ms
gaps), so the extraction method is sound. **State the threshold whenever you
quote a call length**, or the number is not reproducible.

### The limit this file has

Its own in-band noise floor is **69.5 rms** against a strongest-call peak of
**3233** — about **33 dB** of usable range.

**You cannot test BatV1 below −33 dB SNR with this file.** Below that you are
measuring the noise that was already in the recording.

So the call library needs **two kinds** of source, and they must not be
conflated:

1. **Real** — calls cut from the WAV, with their own noise. Realistic shape,
   limited depth. Answers "does it behave on a real bat".
2. **Synthetic** — a CF tone at 107.4 kHz with the measured envelope and
   duration, noise-free by construction. Answers "at what SNR does it fail".

**The SNR number is only meaningful on the synthetic set.**

*Confidence 90 %. The 33 dB figure is a measurement made here, not a datasheet
value.*

---

## 6. Scenarios

A single lesser-horseshoe call is 35 ms. It **never** hits the 300 ms cut. So
replaying the reference file alone does not test what needs testing. The
scenarios are built to force cuts on purpose.

| # | Scenario | What it forces | Pass criterion |
|---|---|---|---|
| A | Noise only, 60 s | Floor seeding, false triggers | 0 clips. This calibrates `k`. |
| B | Reference pass, replayed | One clip per call | 17 clips; inter-pulse intervals from `session_sample` match §5 to ±1 sample |
| C | CF 107.4 kHz, continuous, 1.0 s | **3 length cuts** | Joins are sample-exact: `session_sample[k+1] == session_sample[k] + n_samples[k]`. Concatenated audio is one unbroken tone, no phase step at any join. |
| D | Call train, 5 ms gaps, 2.0 s | Cuts, and the 8 ms hangover never firing | Same join test as C. Confirms `todo/claude.md` §2.5 on hardware. |
| E | Continuous, 2.0 s | **`kMaxCutRun` = 5, then backoff** | Exactly 5 clips, then a hole. The hole is **expected**. Its start and length must match `kBackoffFrames`. |
| F | Synthetic CF, level stepped −6 dB | Detection vs SNR | Two separate numbers: the SNR where detection falls below 50 %, and the SNR where reconstruction starts to fail. |
| G | Reference pass + white noise, stepped | Same, on real call shape | Compare against F to see what the file's own 33 dB costs |
| H | One 30 ms call, **80 ms gap**, one 30 ms call | Two clips, **one** pass | Hole on the timeline is **61.6 ms ± 1 frame** — see below |
| I | Same, **15 ms gap** | Two clips that **overlap** | Gap < 18.4 ms, so clip 2's pre-roll reaches into clip 1. The overlapping samples must be **bit-identical**. |
| J | Same, **6 s gap** | **Two passes, not one** | Exceeds Komori's 5 s fill limit. Two WAVs, never one with a 6 s synthetic silence. |

Every scenario begins with **2 s of noise-only preamble**. The detector needs 16
frames of floor seed (`kFloorSeedFrames`) and must learn the quiet level before
the content starts.

### Scenario E, stated plainly

**After 5 cuts, reconstruction is impossible by design.** The test is not "no
data is lost". The test is "the loss is bounded, predictable, and reported".

### Why H predicts 61.6 ms and not 80 ms

This is the arithmetic people get wrong when reading a stitched file.

| Term | Value |
|---|---|
| Emitted gap, call 1 end to call 2 start | 80.00 ms |
| minus close hangover, 8 frames — clip 1 keeps recording | −8.19 ms |
| minus pre-roll, 10 frames — clip 2 starts early | −10.24 ms |
| **Hole on the session timeline** | **61.57 ms** |

Both ends quantise to the 1.024 ms frame grid, so allow ±1 frame each end.
Komori fills the hole with `header.mid`, not zero.

### Sweeping the gap is the real experiment

Emit pairs at 5, 10, 15, 18, 20, 40, 80, 200 ms and 1, 3, 5, 6 s. Plot clips
produced and pass grouping against emitted gap. That maps every regime boundary
— merge, overlap, clean gap, pass split — in one run, on hardware, instead of by
argument. **Nothing but this box can do that experiment.**

### On the pass-split threshold

`Komori/docs/stitch-plan.md` §4.2 suggests a **5 s** fill limit, configurable.
The working intuition is "3–4 s at least". These agree well enough.

Note that this is a **semantic choice** — what counts as one pass — not a
physical constant. The tester cannot measure the right value. What it can do is
prove the boundary behaves exactly as specified at the chosen value, which is
scenario J.

*Confidence 85 %. The gap and duration figures derive from firmware constants
that should be read at build time, not written down twice.*

---

## 7. What this unlocks in Komori

`Komori/docs/stitch-plan.md` (revision 2, 2026-08-26) is a complete plan for
reassembling clips. This box removes two of its problems.

### It resolves the plan's one open question

§11 asks whether the 256 kHz reference was resampled or its rate reinterpreted
before the firmware model saw it, and warns that the acceptance numbers — 17
clips, 4 overlapping pairs, gaps 0 to 14.3 ms — "will rot the first time anyone
re-runs the model, and nobody will know why".

With BatTester the question stops existing. The tester emits at a known, stated
rate, recorded in the manifest. The clip set comes from hardware and a declared
source, not from a model and an ambiguous file.

### It turns the §10 tests from model-based into hardware-based

| stitch-plan test | Today | With BatTester |
|---|---|---|
| 1 Round trip | Synthetic clips, hand-set `sessionSample` | Real clips, ground truth from the sync latch |
| 2 Length invariant | Model | Hardware |
| 3 Session split | Hand-built | Power-cycle BatV1 mid-scenario |
| 4 Out of order | Shuffle input | Unchanged, still a unit test |
| **5 Overlaps identical** | Assumed | **Scenario I measures it.** If the copies differ, that is a firmware bug found for free |
| **6 Split abutment** | Model | **Scenarios C/D/E measure it.** `wrap == 0 ⇒ sessionSample == prev.sessionSample + prev.len`, exactly |
| **7 Reference recording** | Model, at an unclear rate | Scenario B on hardware, rate declared |

Tests 5 and 6 were "assert it and hope". They become measurements.

### The §6 blocker is not ours, but we make it reproducible

stitch-plan §6 says `analyze.cpp` scores a stitched *Rhinolophus* pass as junk.
`Tone && bandwidth < 3 kHz && duration > 0.08` throws away the one genus that is
CF by definition, and `duration >= 0.28` forces `shape = Long` and collapses
`pulseCount` to 0.

Scenarios C, D and H produce exactly such a pass on demand, in seconds. That is
a far tighter loop than waiting for a bat.

**Do not fix §6 in this project.** It is Komori's, and the stitch plan already
specifies both the minimum and the better fix. Only note that our scenarios are
the reproducer.

*Confidence 90 %. Read from `Komori/docs/stitch-plan.md` revision 2.*

---

## 8. Build order

Each step is provable before the next depends on it.

### Step 1 — CubeMX and the DAC path
Add to `BatTester.ioc`: clock to **80 MHz**, **TIM6** as DAC trigger, **DMA1**
circular on DAC1, **USART2 on PA2/PA3** for the CLI over the ST-LINK VCP, one
**GPIO** for sync out.

*Gate,* on the 250 MHz / 1 GSa/s scope. Four measurements, in order:

1. **Settling.** Full-scale step at 1 MSps, single shot, 1 GSa/s. Does it reach
   final value inside 1.0 µs? This is the §4 assumption, and it either passes or
   the rate drops to 500 kSps. Measure it first.
2. **Worst-case slew in the real signal.** 107.4 kHz at full amplitude, zoomed
   on the steepest sample-to-sample step. The settling limit only matters if the
   signal actually asks for that step.
3. **Spectrum.** Scope FFT on the 107.4 kHz tone. Confirm the image near 890 kHz
   is down by the predicted ~43 dB — ZOH sinc plus the 2-pole RC — and that no
   unexpected spur lands in 20–120 kHz.
4. **DMA half boundary.** Long capture, triggered on the sync GPIO. There must
   be no glitch or discontinuity every 2.048 ms where the fill interrupt hands
   over. This is the failure mode that would silently corrupt every scenario.

Measurement 4 is the one worth being stubborn about. A boundary glitch is
periodic, small, and would look exactly like detector noise later.

### Step 2 — host tool `tools/make_wave_table.py`
Reads the WAV, cuts the 17 calls, resamples to the DAC rate, emits
`Core/Src/wave_table.c` plus a `.json` manifest giving every segment's offset,
length and true sample rate. Also emits the synthetic set.

*Gate:* round-trip the table back to a WAV and confirm it matches the source.

### Step 3 — the scenario engine
Half-buffer fill: read a call from flash, add generated noise, apply gain, write
antiphase to the dual DAC register. A scenario is a small table of
`(segment_id, start_sample, gain)` walked by a sample counter.

*Gate:* play scenario B and confirm the timing table of §5.

The 2026-08-28 scope shot of “ref” **failed this gate**, and the cause is now
known: the image was built at **-O0**, where `pack_half()` makes about
0.8 Msample/s against the DMA's 1.0, so the ring replayed stale content. See
`docs/ref_scope_2026-08-28.md`. Release (-O2) fixes it.

A missed fill is now **counted** rather than silent — `synth::underruns()`,
reported by `sync`, by `stop`, and automatically at the end of every run. **A
run reporting anything but `underrun=0` is not evidence of anything.**

Recapture at 200 ms/div. `scn ref` / `run` now self-stops in **~3.24 s** (the
2 s preamble is included), not the 1.25 s this note used to say.

### Step 4 — CLI
`scn <name>`, `level <dB>`, `snr <dB>`, `run`, `stop`, `sync`.

**No waveform upload over serial.** Regenerating the table and reflashing takes
about 2 s with OpenOCD, which is faster than any serial upload and removes a
whole subsystem.

### Step 5 — ~~the BatV1 sync latch~~ **CANCELLED 2026-08-29**
See the box at the top of §3. BatV1 firmware is not modified. Nothing else in
this build order depended on it.

### Step 6 — host tool — **DONE 2026-08-29, in Komori, in C++**

Not `tools/verify_reconstruction.py`. The verifier and Komori's stitcher are the
same program: both must place clips at `sessionSample` on a common timeline, and
`Komori/docs/stitch-plan.md` had specified that work in full without any of it
being built. Writing a second, private copy in Python would have meant
maintaining the rule twice and testing it in neither place.

In `Komori`:

| File | What |
|---|---|
| `lib/stitch.{hpp,cpp}` | stitch-plan §3, §4, §5 — sessions, passes, absolute placement, gap fill, overlap identity, §4.3 abutment |
| `lib/stitch_test.cpp` | stitch-plan §10 tests 1–6, plus exclusions and the §8 round trip. 41 assertions |
| `tools/verify_battester.cpp` | the verifier — target `verify_battester` |
| `lib/wav.{hpp,cpp}` | §8 `Komori\|Stitch*` and `Komori\|Lossy` GUANO fields, written and parsed |

    ./build/lib/verify_battester <clip-dir> [--wav-dir <dir>] [--fill-limit-s 5]

Reads whatever `dump_adc.py --loop` wrote, and reports:

- clip count, and the session / pass grouping
- **split abutment** — the §4.3 invariant, per pass
- **overlap identity** — if two clips covering one instant disagree, that is a
  BatV1 bug found for free
- gaps, and how much of each pass is synthetic fill
- one stitched WAV per pass, carrying the §8 provenance fields

**What it does not do yet:** compare the recovered audio against the waveform
BatTester emitted. That needs BatTester to export the scenario timeline it
played — see §11 Q8. Everything above is BatV1 self-consistency, which is what
scenarios C, D, E, H, I and J are actually about.

The clock ratio becomes a linear fit of correlation offset against
`sessionSample`, once Q8 lands.

### Step 7 — the gap sweep
Scenarios H, I, J and the sweep in §6. Produces the regime map.

### Step 8 — the SNR sweeps
Scenarios F and G. Produces two curves: detection vs SNR, and reconstruction vs
SNR. They are different curves, and the difference is the interesting part.

---

## 9. The clock — a result that falls out, and a trap

`DETECTOR_NOTES.md` §2 lists BatV1's frequency axis as **UNRESOLVED**, uncertain
by 0.4–1 %, because SYSCLK comes from the MSI RC oscillator. Two estimates
disagree by 0.65 %.

Step 6 measures that ratio directly — a known number of emitted samples against
a counted number of BatV1 samples, over seconds, with no spectral estimation
involved. It closes the open item.

**But only if BatTester's own clock is known.** Otherwise it compares two
uncalibrated RC oscillators and proves nothing.

### RESOLVED 2026-08-29 — option 1 works, and the rate is not 1 MHz

**The X2 crystal is fitted on the L476 and MSI is now locked to it.** No
soldering, one `HAL_RCCEx_EnableMSIPLLMode()` call in `SystemClock_Config`,
after starting LSE with a bounded 1 s poll (never the HAL's 5 s timeout). The
`clk` command reports the state.

**The result is not what "locked" suggests, and this matters.** MSI PLL mode
does not trim MSI to 4.000000 MHz. It locks it to the nearest whole multiple of
the crystal:

| | |
|---|---|
| MSI | 122 x 32768 = **3 997 696 Hz** (−576 ppm from nominal, by construction) |
| SYSCLK | MSI x 40 / 2 = **79 953 920 Hz** |
| DAC, TIM6 ARR=79 | **999 424 Hz**, not 1 000 000 |
| Measured | 999 430 Hz — **within 6 ppm** of the predicted value |

Exactly 1.000000 MHz is **unreachable** from an LSE-locked MSI on this part:
it needs `1 998 848 * PLLN` to be a multiple of 1e6, and `1 998 848 = 2^15 x 61`,
so PLLN would have to be a multiple of `5^6 = 15625`. PLLN maxes out at 127.

That is fine, and is in fact what §9 asked for: **an exact known rate replacing
an inexact drifting one.** Sample counts stay exact; only the conversion to
seconds and hertz changes. `synth::kTrueRateHz` and the manifest's
`true_rate_hz` carry it. The 107.4 kHz tone is really **107 360 Hz**.

*Do not "fix" `kRateHz` to 999424.* Scenario boundaries would stop landing on
round sample counts and `kPhaseInc` would no longer put 440 whole tone cycles
in one 4096-sample ring, which the static-ring path in `synth.cpp` depends on.

**How it was measured.** Timing one run against the host clock has a fixed
offset at each end (USB latency, the `ok run` reply) worth about 2000 ppm — it
gave −2624 ppm and would have been believed. Timing **two runs of very
different length and taking the difference** cancels the offset exactly:

    (61 999 104 − 3 100 672) samples / (61.9864 − 3.0544) s = 999 430.2 Hz

Use the differential form for any future clock claim on this box.

### SB16 is OPEN (confirmed 2026-08-27)

So HSE from the 8 MHz ST-LINK MCO is **not** available on this board without
soldering. That is the Nucleo-64 default. Ranked options:

| # | Option | Soldering | Accuracy | Notes |
|---|---|---|---|---|
| 1 | **MSI locked to LSE**, `HAL_RCCEx_EnableMSIPLLMode()` | **none, if the crystal is fitted** | ≈ LSE, ±20 ppm | Software only. The **same fix** `DETECTOR_NOTES.md` §2 proposes for BatV1. |
| 2 | Close SB16, HSE bypass 8 MHz | one blob | ±30–50 ppm | Reliable, 2 minutes with an iron |
| 3 | Measure the DAC rate once with a counter | none | as good as the counter | Needs the instrument, and re-checking with temperature |
| 4 | Leave both on MSI | none | ±1 % or worse | **Not acceptable for this section.** Fine for everything else. |

**Try option 1 first.** It costs one function call, and whether it works is a
one-minute empirical question: enable LSE and see whether `HAL_RCC_OscConfig`
returns `HAL_OK`. If the crystal is not fitted it fails cleanly, and you fall
back to option 2.

### The same fix probably applies to BatV1

`BatV1.ioc` assigns **PC14 = RCC_OSC32_IN and PC15 = RCC_OSC32_OUT, both
`Locked=true`**, but sets no `LSEState`. The crystal pins are reserved and the
oscillator is simply not switched on. That matches `DETECTOR_NOTES.md` §2 ("No
HSE/LSE enabled") and its proposed fix ("present in HAL, needs LSE enabled").

If the crystals are fitted on both boards, **turning LSE on in both projects
fixes both clocks to ±20 ppm with no soldering anywhere**, and BatV1's frequency
axis stops being uncertain at all. §9 then becomes a *verification* that the fix
worked, rather than a measurement of an unknown. That is a better outcome and a
cheaper one.

It also earns the crystal its keep twice over: `DETECTOR_NOTES.md` §8 already
wants LSE for the RTC and absolute clip timestamps.

### Do not let this block the build

**Steps 1 to 8 need no calibrated clock.** Every reconstruction test — the join
continuity of C/D/E, the split-abutment invariant, the overlap identity of I,
the pass grouping of H and J — is about BatV1's internal consistency **on its
own sample axis**. An uncalibrated tester proves all of them.

Only the absolute frequency-axis measurement needs a reference. Treat it as a
separable bonus, not a prerequisite.

*Confidence 90 % on the method. 80 % that option 1 is available — it depends on
a crystal nobody has looked at yet.*

---

## 10. Decisions taken

| Date | Question | Decision |
|---|---|---|
| 2026-08-27 | Which board? | **Nucleo-L476RG.** CLI on USART2 / ST-LINK VCP. |
| 2026-08-27 | Injection point? | **Direct at PC0/PC1**, as wired. There is no preamp yet, so this is the only chain that exists. |
| 2026-08-27 | Sync latch in BatV1? | **Yes**, on the condition that the field unit needs nothing attached. Met by §3: pull-down, rising edge, two-variable ISR, `constexpr` gate. |
| 2026-08-27 | Bench flag style? | `inline constexpr bool`, not `#ifdef`. Default `true`. |
| 2026-08-27 | HSE from ST-LINK MCO? | **Unavailable — SB16 is open.** See §9 for alternatives. Does not block Steps 1–8. |
| 2026-08-27 | Instrument for the Step 1 gate? | **250 MHz / 1 GSa/s scope available.** 1000 points per DAC update period at 1 MSps — settling is directly observable, not inferred. |
| 2026-08-28 | Is the `ref` DAC output the §5 pass? | **No — not on `REF_2026-08-28_182907`.** 4.87 s mid-rail, ~17 bursts of 15–195 ms, first energy +785 ms. Mid 1.649 V and ~109 kHz CF are fine. Notes in `docs/ref_scope_2026-08-28.md`. Recapture before touching FW. |
| 2026-08-29 | Why did that shot fail? | **-O0 build.** `pack_half()` makes ~0.8 MSps against the DMA's 1.0, so half-buffer interrupts were missed and stale ring content replayed. Release is mandatory. Missed fills are now counted, not silent. |
| 2026-08-29 | Antiphase dual-DAC drive, as §2 specifies? | **No — single-ended on PA4 only.** PA5 is `RUN_LED` (LD2) and DAC channel 2 is not configured; `synth.cpp` writes `DHR12R1`, not `DHR12RD`. BatV1's PC1 is tied to GND. §2 and §4 are not what was built — see the boxes there. |
| 2026-08-29 | BatV1 sync latch? | **Cancelled.** Correlation against one clip fits the one scalar needed; the rest are predictions. See the box at the top of §3. |
| 2026-08-29 | Where does the verifier live? | **Komori, in C++, as `lib/stitch.{hpp,cpp}` plus the `verify_battester` target.** It is the same code as Komori's own stitcher, which had been fully specified and never built. One rule, one implementation, one test suite. See Step 6. |
| 2026-08-29 | Lock BatTester's clock to the X2 crystal? | **Yes, done.** `HAL_RCCEx_EnableMSIPLLMode()` after a bounded LSE start. The DAC rate becomes **999 424 Hz exactly**, not 1 MHz — MSI locks to 122x32768, and exactly 1 MHz is arithmetically unreachable. Known beats nominal. See §9. |
| 2026-08-29 | Default output level? | **`level -5`.** Gives 2020 counts at BatV1's ADC, matching the 2000 counts `make_test_signal.py` used to tune the detector. `level 0` reaches 63560 of 65535 and clips as soon as `snr` noise is added. |

---

## 11. Open questions

| # | Question | Why it matters | Leaning |
|---|---|---|---|
| ~~Q6~~ | ~~Is the 32.768 kHz crystal fitted on either board?~~ | **Both: YES, measured 2026-08-29.** BatV1's `rtc` reports `source LSE`; BatTester's `clk` reports `LSE-locked (X2)`. On BatTester §9 option 1 is **done** — MSI is locked and the DAC rate is a known 999 424 Hz. On BatV1 it is available but deliberately not enabled yet: it would move the ADC sample rate that every clip and every stored `sessionSample` is measured against, so it deserves its own change with a before/after measurement. | Closed |
| ~~Q7~~ | ~~Confirm PD2 is free on the Nucleo-U575ZI-Q~~ | **Closed 2026-08-29: no longer needed.** The sync latch is cancelled — see §3. | — |
| Q8 | Export the played scenario timeline from BatTester | The one thing `verify_battester` cannot do today is compare recovered audio against what was emitted. It needs the event table, and §6 says derive it from firmware constants, never write it down twice. A CLI command that dumps the current scenario's events is the authoritative source. | Small. Do it next |
| Q9 | Does the DAC image need an RC on PA4? | PA4 goes **directly** to PC0 with no reconstruction filter. The image at 1000 − 107.4 = 892.6 kHz aliases at BatV1's 500 kSps to 500 − (892.6 − 500) = **107.4 kHz — exactly onto the carrier**, at roughly −19 dB (ZOH sinc) before the 4× oversample averaging. Harmless for detection and for BatV1-internal reconstruction; it is an error floor when comparing against the emitted waveform (Q8). | One R and one C would remove it |

---

## 12. Deliberately out of scope

- **Acoustic playback.** No ultrasonic transducer. Electrical injection only. It
  cannot test the mic, the horn, or anything acoustic. It is not meant to.
- **Waveform upload over serial.** Reflashing is faster. See Step 4.
- **On-board analysis.** The tester emits, and reports what it emitted. All
  comparison happens on the PC — the same split BatV1 already uses.
- **Fixing `analyze.cpp`.** §7 explains why our scenarios are the reproducer for
  Komori's stitch-plan §6 blocker, and why the fix belongs in Komori.
- **More than two output channels.** Both DAC channels are spent on the
  antiphase differential drive. Sync goes out on a GPIO.

---

## Sources

| Fact | Read from |
|---|---|
| Clip cut, pre-roll, hangover, backoff | `BatV1/todo/claude.md` §1, `BatV1/App/Inc/detector.hpp` |
| BAT2 header layout, `session_sample` | `BatV1/App/Inc/sd_fs.hpp`, `BatV1/tools/dump_adc.py` |
| ADC differential config, PC0/PC1 | `BatV1/Core/Src/main.c:372`, `stm32u5xx_hal_msp.c:136` |
| Sample rate, oversampling, mid-code | `BatV1/App/Inc/adc_capture.hpp` |
| Mic VCM, Zout, sensitivity | `BatV1/BatDetector/IM73A135.md` |
| Anti-alias corner | `BatV1/BatDetector/ADC_GAIN.md` |
| Clock uncertainty, floor seeding, band metric | `BatV1/docs/DETECTOR_NOTES.md` |
| Stitch rule, fill limit, `analyze.cpp` blocker | `Komori/docs/stitch-plan.md` rev 2 |
| Call table in §5 | Measured here from `Komori/FullSpectrum_Rhinolophus_hipposideros.wav` |
