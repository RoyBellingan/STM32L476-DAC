#!/usr/bin/env python3
"""Build Core/Src/wave_table.c and a JSON manifest from the reference WAV.

Primary source is lesser horseshoe (Rhinolophus hipposideros). Other species
are out of scope until that table is proven.

Without --wav, emits a stub table (synthetic CF is generated on the MCU) and a
manifest that still lists every PLAN §5 / synthetic segment.

  python3 tools/make_wave_table.py
  python3 tools/make_wave_table.py --wav --check
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DAC_RATE = 1_000_000
TONE_HZ = 107_400.0
TONE_PEAK = 1800  # DAC counts at 0 dB, matches firmware SYNTH_TONE_PEAK
REF_PEAK = 3233  # strongest call in the 16-bit source (PLAN §5)

# Lesser horseshoe, PLAN §5. Two-call clip LH_2x.wav is the same species,
# later work — do not mix other bats into this table.
DEFAULT_WAV = Path(
    "/home/roy/Public/Komori/BatRecordings/"
    "FullSpectrum_Rhinolophus_hipposideros.wav"
)

# PLAN §5. Threshold: 15 kHz HP, 1 ms frames, 8× 20th-percentile RMS.
CALLS = [
    # start_ms, dur_ms, peak, f_khz
    (297.0, 35.0, 3233, 107.3),
    (378.0, 36.0, 2717, 107.4),
    (464.0, 16.0, 1230, 107.4),
    (549.0, 27.0, 2801, 107.5),
    (713.0, 1.0, 1552, 98.0),
    (718.0, 24.0, 3587, 107.5),
    (803.0, 20.0, 3455, 107.6),
    (885.0, 25.0, 3150, 107.6),
    (969.0, 11.0, 1363, 107.5),
    (1003.0, 2.0, 1145, 107.5),
    (1016.0, 1.0, 1751, 46.0),
    (1047.0, 18.0, 2656, 107.4),
    (1070.0, 12.0, 1256, 107.2),
    (1094.0, 1.0, 1455, 46.0),
    (1129.0, 2.0, 969, 107.5),
    (1138.0, 2.0, 1013, 107.5),
    (1144.0, 1.0, 2067, 45.0),
]


def read_wav(path: Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as w:
        if w.getsampwidth() != 2:
            raise SystemExit(f"{path}: need 16-bit PCM")
        nch = w.getnchannels()
        rate = w.getframerate()
        raw = w.readframes(w.getnframes())
    samples = list(struct.unpack("<" + "h" * (len(raw) // 2), raw))
    if nch == 2:
        samples = samples[0::2]
    return rate, [float(s) for s in samples]


def write_wav(path: Path, rate: int, x: list[float]) -> None:
    clipped = [max(-32768, min(32767, int(round(v)))) for v in x]
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack("<" + "h" * len(clipped), *clipped))


def highpass(x: list[float], rate: int, fc: float = 15_000.0) -> list[float]:
    """One-pole high-pass, bilinear."""
    dt = 1.0 / rate
    rc = 1.0 / (2.0 * math.pi * fc)
    a = rc / (rc + dt)
    y = [0.0] * len(x)
    prev_x = x[0] if x else 0.0
    prev_y = 0.0
    for i, xi in enumerate(x):
        yi = a * (prev_y + xi - prev_x)
        y[i] = yi
        prev_x, prev_y = xi, yi
    return y


def frame_rms(x: list[float], rate: int, frame_ms: float = 1.0) -> list[float]:
    n = max(1, int(rate * frame_ms / 1000.0))
    out = []
    for i in range(0, len(x), n):
        chunk = x[i : i + n]
        if not chunk:
            break
        acc = sum(v * v for v in chunk) / len(chunk)
        out.append(math.sqrt(acc))
    return out


def extract_calls(x: list[float], rate: int) -> list[tuple[float, float]]:
    """Return (start_ms, dur_ms) using the PLAN §5 rule."""
    hp = highpass(x, rate)
    rms = frame_rms(hp, rate)
    if len(rms) < 5:
        return []
    ordered = sorted(rms)
    floor = ordered[max(0, len(ordered) // 5 - 1)]
    thr = 8.0 * max(floor, 1e-9)
    calls = []
    i = 0
    while i < len(rms):
        if rms[i] >= thr:
            j = i
            while j < len(rms) and rms[j] >= thr:
                j += 1
            calls.append((i * 1.0, (j - i) * 1.0))
            i = j
        else:
            i += 1
    return calls


# Half-width of the interpolation kernel, in source samples. 32 gives a
# transition band of roughly 0.09 x the source rate, so 107 kHz against a
# 256 kHz source (0.42 of the rate) sits inside the flat part.
RESAMPLE_HALF = 32

_KERNEL_CACHE: dict[tuple[int, int], list[list[float]]] = {}


def _sinc(t: float) -> float:
    if t == 0.0:
        return 1.0
    z = math.pi * t
    return math.sin(z) / z


def _blackman(t: float, half: int) -> float:
    """Blackman window over t in [-half, half]."""
    a = math.pi * (t + half) / half
    return 0.42 - 0.5 * math.cos(a) + 0.08 * math.cos(2.0 * a)


def _kernels(up: int, half: int) -> list[list[float]]:
    """One windowed-sinc kernel per output phase, normalised to unity DC.

    Output sample i sits at source position i*down/up, so the fractional part
    is always r/up for integer r. There are only `up` distinct phases; build
    them once instead of evaluating a sinc per output sample.
    """
    key = (up, half)
    cached = _KERNEL_CACHE.get(key)
    if cached is not None:
        return cached
    width = 2 * half
    out: list[list[float]] = []
    for r in range(up):
        frac = r / up
        k = [0.0] * width
        total = 0.0
        for m in range(width):
            t = float(m - half + 1) - frac
            v = _sinc(t) * _blackman(t, half)
            k[m] = v
            total += v
        if total != 0.0:
            k = [v / total for v in k]
        out.append(k)
    _KERNEL_CACHE[key] = out
    return out


def resample(x: list[float], src_hz: float, dst_hz: float) -> list[float]:
    """Band-limited rational resample.

    Linear interpolation is unusable here. At 107 kHz against a 256 kHz source
    it droops the wanted tone by 5.4 dB and leaves the image at 256-107 =
    148.7 kHz only 5.4 dB down, which then lands inside BatV1's detector bands.
    A windowed-sinc kernel puts that image below -60 dB.
    """
    if not x:
        return []
    if src_hz == dst_hz:
        return list(x)
    n_out = max(1, int(round(len(x) * dst_hz / src_hz)))
    up = int(round(dst_hz))
    down = int(round(src_hz))
    g = math.gcd(up, down)
    up //= g
    down //= g
    if up > 4096:
        raise ValueError(f"resample ratio {up}/{down} is not a usable rational")

    half = RESAMPLE_HALF
    ks = _kernels(up, half)
    last = len(x) - 1
    out = [0.0] * n_out
    for i in range(n_out):
        num = i * down
        j = num // up
        k = ks[num % up]
        base = j - half + 1
        acc = 0.0
        for m in range(2 * half):
            n = base + m
            if n < 0:
                n = 0
            elif n > last:
                n = last
            acc += x[n] * k[m]
        out[i] = acc
    return out


def raised_cosine(n: int, edge: int) -> list[float]:
    env = [1.0] * n
    e = max(1, min(edge, n // 2))
    for i in range(e):
        env[i] = 0.5 * (1.0 - math.cos(math.pi * i / e))
        env[n - 1 - i] = env[i]
    return env


def synth_cf(dur_s: float, peak: float, rate: int = DAC_RATE,
             freq: float = TONE_HZ) -> list[float]:
    n = max(1, int(round(dur_s * rate)))
    env = raised_cosine(n, int(rate * 0.001))
    out = []
    for i in range(n):
        out.append(peak * env[i] * math.sin(2.0 * math.pi * freq * i / rate))
    return out


def remove_dc(x: list[float]) -> list[float]:
    if not x:
        return []
    mean = sum(x) / len(x)
    return [v - mean for v in x]


def to_dac(x: list[float], src_peak: float = REF_PEAK) -> list[int]:
    """Map source counts onto ±TONE_PEAK, preserving relative level."""
    scale = TONE_PEAK / max(src_peak, 1.0)
    out = []
    for v in x:
        q = int(round(v * scale))
        out.append(max(-2047, min(2047, q)))
    return out


def emit_c(path: Path, samples: list[int], segs: list[dict], rate: int,
           source: str | None = None) -> None:
    lines = [
        "/* Generated by tools/make_wave_table.py. Do not edit. */",
    ]
    if source:
        lines.append(f"/* Source: {source} */")
        lines.append("/* Species: Rhinolophus hipposideros (lesser horseshoe) */")
    lines.extend([
        "",
        '#include "wave_table.h"',
        "",
    ])
    if samples:
        lines.append(f"const int16_t wave_table[{len(samples)}] = {{")
        for i in range(0, len(samples), 16):
            chunk = ", ".join(f"{v:6d}" for v in samples[i : i + 16])
            lines.append(f"    {chunk},")
        lines.append("};")
        lines.append(f"const uint32_t wave_table_len = {len(samples)};")
    else:
        lines.append("const int16_t wave_table[1] = {0};")
        lines.append("const uint32_t wave_table_len = 0;")
    lines.append("")
    real = [s for s in segs if s["kind"] == "real"]
    if real:
        lines.append(f"const wave_seg_t wave_seg[{len(real)}] = {{")
        for s in real:
            lines.append(f"    {{ {s['offset']}, {s['length']} }},")
        lines.append("};")
        lines.append(f"const uint32_t wave_seg_count = {len(real)};")
    else:
        lines.append("const wave_seg_t wave_seg[1] = {{0, 0}};")
        lines.append("const uint32_t wave_seg_count = 0;")
    lines.append(f"const uint32_t wave_rate_hz = {rate}u;")
    lines.append("")
    path.write_text("\n".join(lines))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--wav",
        type=Path,
        nargs="?",
        const=DEFAULT_WAV,
        help="reference 16-bit WAV (default: lesser horseshoe FullSpectrum)",
    )
    ap.add_argument("--out-c", type=Path, default=ROOT / "Core/Src/wave_table.c")
    ap.add_argument("--out-json", type=Path, default=ROOT / "tools/wave_table.json")
    ap.add_argument("--out-wav", type=Path, help="round-trip WAV of the table")
    ap.add_argument("--check", action="store_true",
                    help="require --wav and confirm table matches the cuts")
    args = ap.parse_args()

    if args.check and args.wav is None:
        print("error: --check needs --wav", file=sys.stderr)
        return 2

    samples: list[int] = []
    segs: list[dict] = []
    source = None
    src_rate = None

    if args.wav is not None:
        src_rate, x = read_wav(args.wav)
        source = str(args.wav)
        found = extract_calls(x, src_rate)
        print(f"extracted {len(found)} calls (PLAN §5 expects 17)", file=sys.stderr)
        offset = 0
        ref_cuts: list[list[float]] = []
        for i, (start_ms, dur_ms, peak, f_khz) in enumerate(CALLS):
            i0 = int(round(start_ms * 1e-3 * src_rate))
            n = int(round(dur_ms * 1e-3 * src_rate))
            cut = remove_dc(x[i0 : i0 + n])
            ref_cuts.append(cut)
            rs = resample(cut, src_rate, DAC_RATE)
            dac = to_dac(rs, REF_PEAK)
            segs.append({
                "id": i,
                "name": f"real_{i}",
                "kind": "real",
                "offset": offset,
                "length": len(dac),
                "start_ms": start_ms,
                "dur_ms": dur_ms,
                "peak": peak,
                "f_khz": f_khz,
            })
            samples.extend(dac)
            offset += len(dac)

        if args.check:
            # Compare the stored 1 MSps table to the processed cuts, not a
            # 256 kHz↔1 MHz bounce — 107 kHz is too close to the source Nyquist.
            max_err = 0
            for s, cut in zip(segs, ref_cuts):
                expect = to_dac(resample(cut, src_rate, DAC_RATE), REF_PEAK)
                got = samples[s["offset"] : s["offset"] + s["length"]]
                if len(got) != len(expect) or not got:
                    print(f"error: call {s['id']} length {len(got)} vs {len(expect)}",
                          file=sys.stderr)
                    return 1
                err = max(abs(a - b) for a, b in zip(got, expect))
                max_err = max(max_err, err)
                if err > 0:
                    print(f"error: call {s['id']} table mismatch err {err}",
                          file=sys.stderr)
                    return 1
            print(f"round-trip ok, {len(segs)} calls, max-err {max_err}",
                  file=sys.stderr)

    for i, (start_ms, dur_ms, peak, f_khz) in enumerate(CALLS):
        segs.append({
            "id": 100 + i,
            "name": f"synth_{i}",
            "kind": "synthetic",
            "offset": None,
            "length": int(round(dur_ms * 1e-3 * DAC_RATE)),
            "start_ms": start_ms,
            "dur_ms": dur_ms,
            "peak": peak,
            "f_khz": TONE_HZ / 1000.0,
            "note": "generated at run time on the MCU, not stored in flash",
        })

    emit_c(args.out_c, samples, segs, DAC_RATE, source)

    manifest = {
        "rate_hz": DAC_RATE,
        "tone_hz": TONE_HZ,
        "tone_peak": TONE_PEAK,
        "species": "Rhinolophus hipposideros",
        "source": source,
        "source_rate_hz": src_rate,
        "threshold": "15 kHz HP, 1 ms frames, 8x 20th-percentile RMS",
        "segments": segs,
    }
    args.out_json.write_text(json.dumps(manifest, indent=2) + "\n")

    if args.out_wav is not None and samples:
        write_wav(args.out_wav, DAC_RATE, [v * 16 for v in samples])

    print(f"wrote {args.out_c} ({len(samples)} samples, "
          f"{sum(1 for s in segs if s['kind']=='real')} real segs)")
    print(f"wrote {args.out_json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
