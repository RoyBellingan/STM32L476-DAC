#!/usr/bin/env python3
"""Bring-up capture campaign for the LoopSource: static SVG figures for the docs.

    python3 tools/loop_figs.py [--out docs/loop/figures] [--max-amp 0.5]

The DAC drives the amp module and the loop; D12 reads the loop current through
the PGA (`pga 8 ext`). The script changes freq, amp and harm, and puts the
board back at 62.5 Hz, pure sine, amp 0.2 V peak at the end.

Figures (SVG, legend inside, for markdown) and the data behind each one (CSV)
go to --out; summary.json holds the key numbers. Close cutecom first.
"""
import argparse
import json
import math
import os
import sys
import time

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from loop_snap import PORT, Board, field, nice_ticks, parse_dump  # noqa: E402

# Dataviz reference palette, categorical slots 1..3, validated on the light
# surface (slot 3 is below 3:1 contrast: only used with a legend and labels).
C1, C2, C3 = '#2a78d6', '#eb6834', '#1baf7a'
SURFACE, INK, INK2, GRID = '#fcfcfb', '#0b0b0b', '#52514e', '#e6e5e0'


# --------------------------------------------------------------------------
# static SVG
# --------------------------------------------------------------------------

def svg_plot(path, title, xlabel, ylabel, series, xlog=False, ylog=False, xlim=None,
             ylim=None, xfmt='{:g}', yfmt='{:g}', note=None, W=760, H=400):
    """series: dict(name, color, x, y, width=2, opacity=1, marker=False,
    line=True, legend=True)."""
    # the note wraps at the plot width (about 6.4 px a character at 11.5 px)
    lines = []
    if note:
        words, cur = note.split(), ''
        for w in words:
            if len(cur) + len(w) + 1 > (W - 72 - 24) / 6.4:
                lines.append(cur)
                cur = w
            else:
                cur = (cur + ' ' + w).strip()
        lines.append(cur)
    extra = 15 * max(len(lines) - 1, 0)
    H += extra
    ml, mr, mt, mb = 72, 24, 78 + extra, 54
    pw, ph = W - ml - mr, H - mt - mb

    def finite(a):
        a = np.asarray(a, float)
        return a[np.isfinite(a)]
    xs = np.concatenate([finite(s['x']) for s in series])
    ys = np.concatenate([finite(s['y']) for s in series])
    x0, x1 = xlim if xlim else (float(xs.min()), float(xs.max()))
    if ylim:
        y0, y1 = ylim
    else:
        y0, y1 = float(ys.min()), float(ys.max())
        if ylog:
            y0, y1 = y0 / 1.3, y1 * 1.3
        else:
            pad = (y1 - y0) * 0.08 or abs(y1) * 0.1 or 1.0
            y0, y1 = y0 - pad, y1 + pad

    def tx(v):
        if xlog:
            return ml + (math.log10(v) - math.log10(x0)) / (math.log10(x1) - math.log10(x0)) * pw
        return ml + (v - x0) / (x1 - x0) * pw

    def ty(v):
        if ylog:
            return mt + (math.log10(y1) - math.log10(v)) / (math.log10(y1) - math.log10(y0)) * ph
        return mt + (y1 - v) / (y1 - y0) * ph

    def log_ticks(a, b):
        out = []
        for d in range(math.floor(math.log10(a)) - 1, math.ceil(math.log10(b)) + 1):
            for m in (1, 2, 5):
                v = m * 10 ** d
                if a <= v <= b:
                    out.append(v)
        return out

    e = []
    e.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" '
             f'font-family="DejaVu Sans, Arial, sans-serif" role="img" aria-label="{title}">')
    e.append(f'<rect width="{W}" height="{H}" fill="{SURFACE}"/>')
    e.append(f'<text x="{ml}" y="24" font-size="15" font-weight="bold" fill="{INK}">{title}</text>')
    for j, ln in enumerate(lines):
        e.append(f'<text x="{ml}" y="{42 + 15 * j}" font-size="11.5" fill="{INK2}">{ln}</text>')
    # legend, one row
    lx, ly = ml, 58 + extra
    for s in series:
        if not s.get('legend', True):
            continue
        if s.get('marker') == 'ring':
            e.append(f'<circle cx="{lx + 9}" cy="{ly}" r="5" fill="none" stroke="{s["color"]}" stroke-width="2"/>')
        elif s.get('marker') and not s.get('line', True):
            e.append(f'<circle cx="{lx + 9}" cy="{ly}" r="4.5" fill="{s["color"]}"/>')
        else:
            e.append(f'<rect x="{lx}" y="{ly - 1.5}" width="18" height="3" rx="1.5" fill="{s["color"]}" '
                     f'fill-opacity="{max(s.get("opacity", 1), 0.5)}"/>')
        e.append(f'<text x="{lx + 24}" y="{ly + 4}" font-size="12" fill="{INK2}">{s["name"]}</text>')
        lx += 24 + 7.2 * len(s['name']) + 22
    # grid
    yt = log_ticks(y0, y1) if ylog else nice_ticks(y0, y1, 6)
    for t in yt:
        y = ty(t)
        e.append(f'<line x1="{ml}" x2="{ml + pw}" y1="{y:.1f}" y2="{y:.1f}" stroke="{GRID}"/>')
        e.append(f'<text x="{ml - 8}" y="{y + 4:.1f}" text-anchor="end" font-size="11" fill="{INK2}">'
                 f'{yfmt.format(t)}</text>')
    xt = log_ticks(x0, x1) if xlog else nice_ticks(x0, x1, 8)
    for t in xt:
        x = tx(t)
        e.append(f'<line x1="{x:.1f}" x2="{x:.1f}" y1="{mt}" y2="{mt + ph}" stroke="{GRID}"/>')
        e.append(f'<text x="{x:.1f}" y="{mt + ph + 18}" text-anchor="middle" font-size="11" fill="{INK2}">'
                 f'{xfmt.format(t)}</text>')
    e.append(f'<line x1="{ml}" x2="{ml + pw}" y1="{mt + ph}" y2="{mt + ph}" stroke="#c9c8c2"/>')
    e.append(f'<text x="{ml + pw / 2}" y="{H - 12}" text-anchor="middle" font-size="12" fill="{INK2}">{xlabel}</text>')
    e.append(f'<text x="18" y="{mt + ph / 2}" text-anchor="middle" font-size="12" fill="{INK2}" '
             f'transform="rotate(-90 18 {mt + ph / 2})">{ylabel}</text>')
    e.append(f'<clipPath id="pa"><rect x="{ml}" y="{mt}" width="{pw}" height="{ph}"/></clipPath>')
    # marks
    for s in series:
        pts = [(tx(x), ty(y)) for x, y in zip(s['x'], s['y'])
               if np.isfinite(x) and np.isfinite(y) and (not ylog or y > 0) and (not xlog or x > 0)]
        if s.get('line', True) and len(pts) > 1:
            d = ' '.join(f'{x:.1f},{y:.1f}' for x, y in pts)
            e.append(f'<polyline clip-path="url(#pa)" points="{d}" fill="none" stroke="{s["color"]}" '
                     f'stroke-width="{s.get("width", 2)}" stroke-opacity="{s.get("opacity", 1)}" '
                     f'stroke-linejoin="round" stroke-linecap="round"/>')
        if s.get('marker') == 'ring':
            for x, y in pts:
                e.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="7" fill="none" stroke="{s["color"]}" '
                         f'stroke-width="2.2"/>')
        elif s.get('marker'):
            for x, y in pts:
                e.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4.5" fill="{s["color"]}" '
                         f'stroke="{SURFACE}" stroke-width="2"/>')
    e.append('</svg>')
    open(path, 'w').write('\n'.join(e))


def save_csv(path, header, cols):
    np.savetxt(path, np.column_stack(cols), delimiter=',', header=','.join(header), comments='',
               fmt='%.6g')


# --------------------------------------------------------------------------
# board helpers
# --------------------------------------------------------------------------

class Rig:
    def __init__(self, port):
        self.b = Board(port)
        stat = self.b.cmd('stat', ('@T',))[-1]
        self.cal = field(stat, 'vrefcal')

    def c(self, cmd, until=('@',)):
        return self.b.cmd(cmd, until)

    def read(self, n):
        out = self.b.cmd(f'read {n}', ('@G',), timeout=n * 0.03 + 20)
        r = {'R': next(l for l in out if l.startswith('@R')), 'G': out[-1]}
        for l in out:
            if l.startswith('@C'):
                r[l.split()[1]] = l
        r['gain'] = field(r['G'], 'gain')
        return r

    def snap_raw(self):
        self.b.cmd('read 1', ('@G',))
        return parse_dump(self.b.cmd('dump raw', ('@W end',)))

    def dump_avg(self):
        return parse_dump(self.b.cmd('dump avg', ('@W end',)))


def volts(rig, x, cols, ch, gain):
    ci = {c: j for j, c in enumerate(cols)}
    vdda = 3.0 * rig.cal / x[:, ci['vref']].mean()
    v = (x[:, ci[ch]] - x[:, ci[ch]].mean()) * vdda / 4096.0
    return v / gain if ch == 'p' else v


def spectrum(vs, fs):
    n = len(vs[0])
    p = np.mean([np.abs(np.fft.rfft(v) / n * 2) ** 2 / 2 for v in vs], axis=0)
    f = np.fft.rfftfreq(n, 1 / fs)
    return f[1:], 10 * np.log10(np.maximum(p[1:], 1e-14) / 1e-6)


# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', default=PORT)
    ap.add_argument('--out', default='docs/loop/figures')
    ap.add_argument('--max-amp', type=float, default=0.5, help='DAC V peak, upper limit (default 0.5)')
    ap.add_argument('--render-only', action='store_true', help='redraw the figures from the CSV files in --out')
    a = ap.parse_args()
    if a.render_only:
        return render(a.out)
    os.makedirs(a.out, exist_ok=True)
    out = lambda n: os.path.join(a.out, n)
    rig = Rig(a.port)
    summ = {'date': time.strftime('%Y-%m-%d %H:%M')}
    A = min(0.5, a.max_amp)

    rig.c('harm off', ('@P',))
    rig.c('pga 8 ext', ('@S',))
    rig.c('freq 62.5', ('@F',))

    # ---- F1, F2: the loop current at A, waveform and spectrum -------------
    print('F1/F2: waveform and spectrum')
    rig.c(f'amp {A}', ('@S',))
    time.sleep(1.0)
    snaps = [rig.snap_raw() for _ in range(6)]
    r = rig.read(50)
    G = r['gain']
    cols, avg = rig.dump_avg()
    n = len(avg)
    f0 = field(r['R'], 'f0')
    fs = f0 * n
    t = np.arange(n) / fs * 1e3
    raw_p = [volts(rig, x, c, 'p', G) * 1e3 for c, x in snaps]
    raw_i = [volts(rig, x, c, 'i', 1) * 1e3 for c, x in snaps]
    avg_p = volts(rig, avg, cols, 'p', G) * 1e3
    save_csv(out('f1_loop_wave.csv'), ['t_ms', 'avg_mV'] + [f'raw{k}_mV' for k in range(6)], [t, avg_p] + raw_p)
    ff, sp = spectrum([v / 1e3 for v in raw_p], fs)
    _, si = spectrum([v / 1e3 for v in raw_i], fs)
    save_csv(out('f2_loop_spectrum.csv'), ['f_Hz', 'pga_dB', 'direct_dB'], [ff, sp, si])
    summ['f1'] = dict(dac_vpk=A, f0=f0, a1_mV=field(r['p'], 'a1') / G * 1e3,
                      rmsh_mV=field(r['p'], 'rmsh') / G * 1e3, rms_mV=field(r['p'], 'rms') / G * 1e3,
                      thd_pct=field(r['p'], 'thd'), dpf=field(r['p'], 'dpf'), pga_gain=G,
                      phase_deg=field(r['p'], 'ph'))

    # ---- F3: linearity ----------------------------------------------------
    print('F3: linearity')
    levels = [v for v in (0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5) if v <= A + 1e-9]
    rows = []
    for v in levels:
        rig.c(f'amp {v}', ('@S',))
        time.sleep(0.4)
        r = rig.read(50)
        G = r['gain']
        rows.append((v / math.sqrt(2) * 1e3, field(r['p'], 'a1') / G * 1e3,
                     field(r['p'], 'rmsh') / G * 1e3, field(r['p'], 'rms') / G * 1e3))
    rows = np.array(rows)
    save_csv(out('f3_linearity.csv'), ['dac_mVrms', 'a1_mV', 'rmsh_mV', 'rms_mV'], list(rows.T))

    # ---- F4: stability, 50 Hz against 62.5 Hz -------------------------------
    print('F4: stability 50 / 62.5 Hz (about 90 s)')
    rig.c('amp 0.2', ('@S',))
    for fq, per, name in (('50', 10, '50'), ('62.5', 12, '62')):
        rig.c(f'freq {fq}', ('@F',))
        time.sleep(1.0)
        ts, vs = [], []
        t0 = time.time()
        while time.time() - t0 < 45:
            r = rig.read(per)
            ts.append(time.time() - t0)
            vs.append(field(r['p'], 'a1') / r['gain'] * 1e3)
        save_csv(out(f'f4_stability_{name}.csv'), ['t_s', 'a1_mV'], [np.array(ts), np.array(vs)])

    # ---- F5: frequency response of the chain --------------------------------
    print('F5: frequency response')
    fr = []
    for fq in (50, 62.5, 75, 100, 125, 150, 200, 250):
        rf = rig.c(f'freq {fq}', ('@F',))[-1]
        time.sleep(1.0)
        r = rig.read(40)
        f_true = field(rf, 'f0')
        fr.append((f_true, field(r['p'], 'a1') / r['gain'] / (0.2 / math.sqrt(2)),
                   field(r['p'], 'ph') + 360 * f_true * 20e-6))   # remove the DAC's one-sample delay
    fr = np.array(fr)
    save_csv(out('f5_response.csv'), ['f_Hz', 'gain', 'phase_deg'], list(fr.T))

    # ---- F6: a distorted source ---------------------------------------------
    print('F6: distorted source')
    rig.c('freq 62.5', ('@F',))
    rig.c('amp 0.3', ('@S',))
    rig.c('harm 3 0.3 180', ('@P',))
    time.sleep(1.0)
    r = rig.read(50)
    cols, avg = rig.dump_avg()
    n = len(avg)
    ci = {c: j for j, c in enumerate(cols)}

    def aligned(v):
        """One period, rotated so the fundamental is sin(phase), fundamental peak 1."""
        v = v - v.mean()
        X = np.fft.rfft(v) / n * 2
        kk = np.arange(len(X))
        Xr = X * np.exp(-1j * kk * (np.angle(X[1]) + np.pi / 2))
        return np.fft.irfft(Xr * n / 2, n) / abs(X[1])
    loop = aligned(avg[:, ci['p']])
    dac_measured = field(r['a'], 'a1') >= 0.1
    if dac_measured:
        dac = aligned(avg[:, ci['a']])
    else:                                  # A1 no longer on the DAC: the commanded shape
        th = np.arange(n) / n * 2 * np.pi
        dac = np.sin(th) + 0.3 * np.sin(3 * th + np.pi)
    save_csv(out('f6_shape.csv'), ['deg', 'dac', 'loop'], [np.arange(n) / n * 360, dac, loop])
    summ['f6'] = dict(dac_measured=bool(dac_measured), loop_dpf=field(r['p'], 'dpf'),
                      loop_thd_pct=field(r['p'], 'thd'))
    rig.c('harm off', ('@P',))

    # ---- F7: noise floor, drive at 0 ---------------------------------------------
    print('F7: noise floor')
    rig.c('amp 0', ('@S',))
    time.sleep(1.0)
    r = rig.read(10)
    G = r['gain'] if r['gain'] and r['gain'] > 4 else 8.0
    snaps = [rig.snap_raw() for _ in range(6)]
    n = len(snaps[0][1])
    fs = field(r['R'], 'f0') * n
    ff, sp = spectrum([volts(rig, x, c, 'p', G) for c, x in snaps], fs)
    _, si = spectrum([volts(rig, x, c, 'i', 1) for c, x in snaps], fs)
    save_csv(out('f7_noise_floor.csv'), ['f_Hz', 'pga_dB', 'direct_dB'], [ff, sp, si])

    rig.c('amp 0.2', ('@S',))             # back to the running state
    json.dump(summ, open(out('summary.json'), 'w'), indent=1)
    render(a.out)


def load(path):
    with open(path) as fh:
        head = fh.readline().strip().split(',')
    d = np.loadtxt(path, delimiter=',', skiprows=1, ndmin=2)
    return {h: d[:, j] for j, h in enumerate(head)}


def harmonics(v, kmax=7):
    X = np.abs(np.fft.rfft(v - v.mean()))
    return X[1:kmax + 1] / X[1]


def render(d):
    """Draw every figure from the CSV files and summary.json in d."""
    out = lambda n: os.path.join(d, n)
    summ = json.load(open(out('summary.json')))
    dbc = lambda x: 20 * np.log10(np.maximum(x, 1e-9))

    # F1
    w = load(out('f1_loop_wave.csv'))
    f1 = summ['f1']
    raws = [k for k in w if k.startswith('raw')]
    ser = [dict(name=f'raw periods ({len(raws)})', color=C1, x=w['t_ms'], y=w[k], width=1.1, opacity=0.3,
                legend=(j == 0)) for j, k in enumerate(raws)]
    ser.append(dict(name='average of 50 periods', color=C1, x=w['t_ms'], y=w['avg_mV'], width=2.4))
    svg_plot(out('f1_loop_wave.svg'), f'Loop current at D12, one period ({f1["f0"]:.1f} Hz)',
             'time in the period (ms)', 'mV at D12 (PGA path, scaled to the input)', ser,
             note=f'DAC {f1["dac_vpk"]} V peak through the amp. Fundamental {f1["a1_mV"]:.2f} mV rms, '
                  f'THD {f1["thd_pct"]:.2f} %, dpf {f1["dpf"]:.4f}. The raw periods carry the bench noise; '
                  f'the average keeps only what is coherent with the source.')

    # F2
    sp = load(out('f2_loop_spectrum.csv'))
    lo = min(np.percentile(sp['pga_dB'], 2), np.percentile(sp['direct_dB'], 2)) - 5
    hi = max(sp['pga_dB'].max(), sp['direct_dB'].max()) + 5
    svg_plot(out('f2_loop_spectrum.svg'), 'Loop current at D12, spectrum', 'frequency (Hz, log)', 'dB re 1 mV rms',
             [dict(name='PGA path (/G)', color=C1, x=sp['f_Hz'], y=sp['pga_dB'], width=1.6),
              dict(name='direct ADC', color=C2, x=sp['f_Hz'], y=sp['direct_dB'], width=1.6)],
             xlog=True, ylim=(lo, hi),
             note=f'Power averaged over 6 raw periods; bins at k x {sp["f_Hz"][0]:.3f} Hz. The fundamental is the '
                  f'first point; the rest is the bench noise of one period.')

    # F3
    li = load(out('f3_linearity.csv'))
    x, y = li['dac_mVrms'], li['a1_mV']
    mid = (x >= 30) & (x <= 300)
    k = float(np.sum(y[mid] * x[mid]) / np.sum(x[mid] ** 2))
    xs = np.array([x.min() * 0.8, x.max() * 1.25])
    svg_plot(out('f3_linearity.svg'), 'Linearity: loop current at D12 against the DAC drive', 'DAC, mV rms (log)',
             'D12, mV rms (log)',
             [dict(name=f'fit, {k:.4f} x DAC', color=INK2, x=xs, y=k * xs, width=1.2),
              dict(name='raw rms (noise included)', color=C2, x=x, y=li['rms_mV'], marker=True, line=False),
              dict(name='fundamental (a1)', color=C1, x=x, y=y, marker=True, line=False)],
             xlog=True, ylog=True,
             note='62.5 Hz, 50 periods per point. The raw rms bends up at the low end: the bench noise '
                  '(about 7 mV rms here) adds in quadrature. The fundamental stays on the line.')
    dev = (y / (k * x) - 1) * 100
    svg_plot(out('f3b_linearity_dev.svg'), 'Linearity: deviation of the fundamental from the fit',
             'DAC, mV rms (log)', 'deviation (%)',
             [dict(name='a1 / fit - 1', color=C1, x=x, y=dev, marker=True, width=1.4)],
             xlog=True, yfmt='{:.1f}',
             note='Fit on the points from 30 to 300 mV rms of DAC. Within ±0.2 % from 70 mV rms up; the low '
                  'points scatter with the bench noise (50 periods each).')
    summ['f3'] = dict(chain_gain=k, dev_pct=[round(v, 3) for v in dev])

    # F4
    s50, s62 = load(out('f4_stability_50.csv')), load(out('f4_stability_62.csv'))
    pp = {n: float(np.ptp(v['a1_mV']) / np.mean(v['a1_mV']) * 100) for n, v in (('50', s50), ('62.5', s62))}
    sd = {n: float(np.std(v['a1_mV']) / np.mean(v['a1_mV']) * 100) for n, v in (('50', s50), ('62.5', s62))}
    svg_plot(out('f4_stability_50_vs_62.svg'), 'Why 62.5 Hz: the same drive, read again and again',
             'time (s)', 'fundamental at D12, % from its mean',
             [dict(name='50 Hz', color=C2, x=s50['t_s'], y=(s50['a1_mV'] / s50['a1_mV'].mean() - 1) * 100, width=1.8),
              dict(name='62.5 Hz', color=C1, x=s62['t_s'], y=(s62['a1_mV'] / s62['a1_mV'].mean() - 1) * 100, width=1.8)],
             yfmt='{:.1f}',
             note=f'DAC 0.2 V peak, reads of 0.2 s, 45 s each. At 50 Hz the mains hum beats with the source: '
                  f'{pp["50"]:.1f} % p-p (σ {sd["50"]:.2f} %). At 62.5 Hz it does not: {pp["62.5"]:.2f} % p-p '
                  f'(σ {sd["62.5"]:.2f} %), the noise of a 0.2 s read.')
    summ['f4'] = dict(pp_pct=pp, std_pct=sd)

    # F5
    fr = load(out('f5_response.csv'))
    note5 = ('DAC 0.2 V peak, 40 periods per point; amp module, loop and burden together. '
             'The 50 Hz point sits in the mains hum (±4 %, see the stability figure).')
    svg_plot(out('f5_response_gain.svg'), 'Chain response: D12 over the DAC, against frequency',
             'frequency (Hz, log)', 'gain (D12 / DAC)',
             [dict(name='gain', color=C1, x=fr['f_Hz'], y=fr['gain'], marker=True, width=1.6)],
             xlog=True, xlim=(45, 280), yfmt='{:.4f}', note=note5)
    svg_plot(out('f5b_response_phase.svg'), 'Chain response: phase of the loop current against the DAC',
             'frequency (Hz, log)', 'phase (degrees, + = lead)',
             [dict(name='phase', color=C1, x=fr['f_Hz'], y=fr['phase_deg'], marker=True, width=1.6)],
             xlog=True, xlim=(45, 280), yfmt='{:.0f}',
             note='The DAC one-sample delay is removed. The phase falls by about 37° from 50 to 250 Hz, zero near '
                  '120 Hz, while the gain stays within ±2 %. A single high-pass cannot do both; the cause (amp module '
                  'or loop) needs the amp-output channel to split.')

    # F6
    sh = load(out('f6_shape.csv'))
    f6 = summ['f6']
    hd, hl = harmonics(sh['dac']), harmonics(sh['loop'])
    dpf_dac = 1 / math.sqrt(float(np.sum(hd ** 2)))
    dac_name = 'DAC' if f6['dac_measured'] else 'DAC, commanded'
    svg_plot(out('f6_shape.svg'), 'Distorted source: the DAC shape and the loop current it makes',
             'phase in the period (degrees)', 'normalised to the fundamental peak',
             [dict(name=f'{dac_name} (3rd at 30 %, 180°)', color=C2, x=sh['deg'], y=sh['dac'], width=2),
              dict(name='loop current at D12', color=C1, x=sh['deg'], y=sh['loop'], width=2)],
             yfmt='{:.1f}',
             note=f'62.5 Hz, 50 periods, fundamentals aligned. dpf: {dac_name.lower()} {dpf_dac:.4f}, loop current '
                  f'{f6["loop_dpf"]:.4f}. The 3rd keeps its size ({hl[2] * 100:.1f} %) but not its phase: the chain '
                  f'leads at 62.5 Hz and lags at 187.5 Hz, so the peak moves. The reference measures what flows.')
    hk = np.arange(2, 8)
    dser = [dict(name='loop current', color=C1, x=hk, y=dbc(hl[1:]), marker=True, line=False)]
    keep = hd[1:] > 1e-4                   # the commanded shape has only the 3rd
    dser.append(dict(name=dac_name, color=C2, x=hk[keep], y=dbc(hd[1:][keep]), marker='ring', line=False))
    svg_plot(out('f6b_harmonics.svg'), 'Distorted source: harmonics, DAC against loop current', 'harmonic',
             'dBc (re the fundamental)', dser, yfmt='{:.0f}', ylim=(-85, 0),
             note=f'3rd: {dac_name.lower()} {dbc(hd[2]):.1f} dBc, loop current {dbc(hl[2]):.1f} dBc. The other loop '
                  f'harmonics are the amp and the bench (the 4th falls on the mains 5th at 250 Hz).')
    summ['f6'].update(dac_dpf=dpf_dac, loop_h3_pct=float(hl[2] * 100))

    # F7
    nf = load(out('f7_noise_floor.csv'))
    svg_plot(out('f7_noise_floor.svg'), 'Noise floor at D12, drive at 0, amp connected', 'frequency (Hz, log)',
             'dB re 1 mV rms',
             [dict(name='PGA path (/8, 1 nF across R2)', color=C1, x=nf['f_Hz'], y=nf['pga_dB'], width=1.6),
              dict(name='direct ADC', color=C2, x=nf['f_Hz'], y=nf['direct_dB'], width=1.6)], xlog=True,
             note='Mains hum at the left; switching-supply lines with 100 Hz spacing; spikes aliased from above '
                  '25 kHz. With the amp connected the PGA path is only a few dB quieter (without it: 10-20 dB), '
                  'so part of this noise enters after the PGA input.')

    json.dump(summ, open(out('summary.json'), 'w'), indent=1)
    print(json.dumps(summ, indent=1))


if __name__ == '__main__':
    sys.exit(main())
