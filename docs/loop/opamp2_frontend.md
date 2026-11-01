# Loop source, stage 1: gain on the Rs input with OPAMP2

Plan: digitalSpine `todo/fluxStationLoopSource.md` §2.2 (stage 1), §7.1 (results).
Firmware: `Loop/Src/pga.cpp`, command `pga`. Written 2026-11-01.

## Decision (Roy, 2026-11-01)

Stage 1 uses the **internal PGA at gain 8, with the input biased at 0.21 V** (VDDA/16). The
signal at the input is at most about 100 mV rms (rule of thumb: 1/10 of the DAC signal). Gain 8
takes up to 0.39 V p-p (≈ 135 mV rms). No gain switching: a fixed bias allows one gain only.

The **standalone mode** (external R1, R2, mid-rail bias) is kept for later:
[`opamp2_standalone.svg`](opamp2_standalone.svg).

## Facts measured on this board

The DAC drove PA6 (D12) directly. The ADC read the PGA input (`i`, PA6) and output (`p`, PB0)
in the same scan, so the gain is measured, not assumed.

| PGA | Measured gain | Error | Phase out − in | Offset at the output |
|---|---|---|---|---|
| 1 (follower) | 1.00026 | +0.03 % | −0.0001° | −1.7 mV |
| 2 | 2.00029 | +0.02 % | −0.001° | −2.5 mV |
| 4 | 4.01783 | +0.45 % | −0.003° | −4.9 mV |
| 8 | 8.00939 | +0.12 % | −0.004° | −14.4 mV |

DS10198 Table 86 gives ±1 % gain error. Store one factor per gain, or use the live p / i ratio.

**In PGA mode the internal R1 returns to VSSA.** The VINM pin (PA7, `VMSEL` 00, "for
filtering", `stm32l4xx_ll_opamp.h:206`) is the R1/R2 junction, not the end of R1. Measured with
PA7 open: 0.415 V = 3.319 V × 1/8 at gain 8, and 1.625 V = 3.253 V × 1/2 at gain 2. So the PGA
multiplies the input DC too: for a centred output, the input must be at VDDA/(2G).

| PGA | Input bias VDDA/(2G) | Max input, linear (3.12 V / G) |
|---|---|---|
| 16 | 0.10 V | 0.195 V p-p |
| 8 | 0.21 V | 0.39 V p-p |
| 4 | 0.41 V | 0.78 V p-p |
| 2 | 0.83 V | 1.56 V p-p |

The input must not go below 0 V. So the bias must be larger than the signal peak.

## Built: internal PGA × 8

Real Rs input:

```
3V3 ── 150 k ──┬── B (0.207 V) ── 10 k ──┬── D12 (PA6, PGA in, ADC i)
               │                         │
             10 k   10 µF (B to GND)   (+) 47 µF
               │                         │
              GND                     Rs top (0 V DC)
PGA output: A3 (PB0, ADC p), centre ≈ 1.66 V. D11 (PA7) not connected.
```

DAC test (no Rs): A2 → 10 k → D12 → 1.1 k → GND. With the DAC centred at 1.66 V, D12 sits at
0.164 V, and a DAC signal of 1 V rms gives 99 mV rms at D12. Output 1.32 V ± 1.12 V. The DAC
load is 11 kΩ (minimum 5 kΩ, DS10198).

Commands: `pga 8`, `amp <V peak>`, `read <n>`; the `@G` line gives the measured gain, phase,
offset and the noise referred to the input.

### Filter capacitor (fitted 2026-11-01)

1 nF from D11 (PA7, VINM) to A3 (PB0, output), with `pga 8 ext` (VINM on the pin; with plain
`pga 8` the pin is not connected and the capacitor does nothing). It sits across the internal
R2 (≈ 140 k at gain 8), so the gain falls from 8 to 1 above ≈ 1.1 kHz. Measured with the
snapshot tool: above 1 kHz the PGA path is 10–20 dB below the direct input. The bench noise
(switching supplies, 100 Hz-spaced lines around 7.75 kHz after aliasing, spikes of ±100 mV on
D12) is mostly above that corner. The 50 Hz gain changes a little; the firmware measures it
live (`@G gain=`). Use short, twisted leads: D11 is a feedback node.

### Graphs: `tools/loop_snap.py`

`python3 tools/loop_snap.py --pre "pga 8 ext" --pre "freq 62.5" --snaps 4 --avg 50` grabs 4 raw
periods (`read 1` + `dump raw`) and one averaged period (`read 50` + `dump avg`), and writes CSV
files and `loop_snap.html`: one period at D12 (raw and averaged, PGA path and direct) and the
spectrum (bins at k × f0), with hover readouts. Close cutecom first.

## Kept for later: standalone mode

[`opamp2_standalone.svg`](opamp2_standalone.svg). OPAMP2 as a plain op-amp (OPAMODE 00),
external R2 68 k from A3 to D11, R1 10 k from D11 through 47 µF to GND. DC gain 1, so the output
sits on the input bias (1.65 V, the plan's mid-rail). AC gain 1 + R2/R1 = 7.8 (R2 = 27 k for
3.7, if the input reaches 250 mV rms). The gain is fixed by the parts. Command: `pga std 7.8`.

Use it if the mid-rail bias must stay, or if the input needs more headroom than 0.21 V of bias
gives.

## Pins

- OPAMP2: VINP PA6 (D12), VINM PA7 (D11), VOUT PB0 (A3). Keep A3 free of other loads
  (≤ 450 µA in PGA mode). PB0 is the BatTester SYNC output: do not leave both connected.
- Do not use OPAMP1: its output is PA3, the ST-LINK VCP RX.
- PA5, PA6, PA7 are D13, D12, D11 (SPI1). An ADC 15 Click on an Arduino click shield uses
  them. In stage 2 the PGA is not needed, or the Click goes on SPI2 or SPI3.
