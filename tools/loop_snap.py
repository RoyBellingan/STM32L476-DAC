#!/usr/bin/env python3
"""Grab ADC snapshots from the LoopSource board and draw them.

    python3 tools/loop_snap.py [--snaps 4] [--avg 50] [--pre "freq 62.5"] [--out DIR]

Close cutecom first: the serial port is exclusive.

For each snapshot the board runs `read 1` and `dump raw`: one fresh raw period,
all channels, every DAC sample (no decimation). Then `read <avg>` and
`dump avg`: the period averaged over <avg> periods (synchronous averaging, so
anything not coherent with the source frequency drops by sqrt(avg)).

Writes into DIR (default: loop_snap_<time>/):
  snap_<k>.csv    raw periods, codes, one column per channel
  avg.csv         averaged period, codes
  report.txt      the @R / @C / @G lines of the averaged read
  loop_snap.html  two charts with hover readouts; open it in a browser

Scaling: VDDA from the VREFINT column and VREFINT_CAL (from `stat`). The PGA
output `p` is shown at the PGA input (D12): (p - mean) / G, with G the gain the
board measured in the same read (`@G gain=`). With the PGA off, `p` is not drawn.
"""
import argparse
import html
import json
import math
import os
import re
import sys
import time

import numpy as np
import serial

PORT = '/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_0669FF313937424757082637-if02'

# Categorical slots 1 and 2 of the dataviz reference palette (validated, light
# surface #fcfcfb). Blue = the PGA path, orange = the direct ADC input, on both
# charts.
C_PGA = '#2a78d6'
C_DIR = '#eb6834'
SURFACE = '#fcfcfb'
INK = '#0b0b0b'
INK2 = '#52514e'
GRID = '#e6e5e0'


class Board:
    def __init__(self, port):
        self.s = serial.Serial(port, 460800, timeout=0.3)
        time.sleep(0.5)
        self.s.write(b'\r\n')            # the first bytes after an open can be lost
        time.sleep(0.2)
        self.s.reset_input_buffer()

    def cmd(self, c, until, timeout=60.0):
        self.s.write((c + '\r\n').encode())
        out = []
        t0 = time.time()
        while time.time() - t0 < timeout:
            line = self.s.readline().decode(errors='replace').strip()
            if not line:
                continue
            out.append(line)
            if line.startswith('@?'):
                raise RuntimeError(f'{c}: {line}')
            if any(line.startswith(u) for u in until):
                return out
        raise RuntimeError(f'{c}: timeout, got {out[-3:]}')


def field(line, key, cast=float):
    m = re.search(r'(?:^| )' + key + r'=([^ ]+)', line)
    return cast(m.group(1)) if m else None


def parse_dump(lines):
    head = next(l for l in lines if l.startswith('@W begin'))
    cols = field(head, 'cols', str).split(',')[1:]       # first column is the index
    rows = [l.split()[2:] for l in lines
            if l.startswith('@W ') and l.split()[1].isdigit()]
    return cols, np.array(rows, float)


def nice_ticks(lo, hi, n=6):
    span = hi - lo
    if span <= 0:
        return [lo]
    step = 10 ** math.floor(math.log10(span / n))
    for m in (1, 2, 2.5, 5, 10):
        if span / (step * m) <= n:
            step *= m
            break
    first = math.ceil(lo / step) * step
    return [first + i * step for i in range(int((hi - first) / step) + 1)]


def svg_chart(cid, title, xlab, ylab, series, xlog=False, ylim=None, fmt_x='{:.2f}',
              fmt_y='{:.2f}', hover_unit=('', '')):
    """series: list of dict(name, color, x, y, width, opacity, hover)."""
    W, H = 900, 340
    ml, mr, mt, mb = 70, 20, 16, 46
    xs = np.concatenate([s['x'] for s in series])
    ys = np.concatenate([s['y'] for s in series])
    x0, x1 = float(xs.min()), float(xs.max())
    y0, y1 = ylim if ylim else (float(ys.min()), float(ys.max()))
    pad = (y1 - y0) * 0.06 or 1.0
    y0, y1 = y0 - pad, y1 + pad
    if xlog:
        fx = lambda x: ml + (math.log10(x) - math.log10(x0)) / (math.log10(x1) - math.log10(x0)) * (W - ml - mr)
    else:
        fx = lambda x: ml + (x - x0) / (x1 - x0) * (W - ml - mr)
    fy = lambda y: mt + (y1 - y) / (y1 - y0) * (H - mt - mb)

    out = [f'<svg id="{cid}" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" '
           f'aria-label="{html.escape(title)}">']
    # grid and ticks: recessive
    for t in nice_ticks(y0, y1):
        y = fy(t)
        out.append(f'<line x1="{ml}" x2="{W - mr}" y1="{y:.1f}" y2="{y:.1f}" stroke="{GRID}"/>')
        out.append(f'<text x="{ml - 8}" y="{y + 4:.1f}" text-anchor="end" class="tick">{fmt_y.format(t)}</text>')
    if xlog:
        xt = [v for d in range(0, 6) for v in (10 ** d, 2 * 10 ** d, 5 * 10 ** d) if x0 <= v <= x1]
    else:
        xt = nice_ticks(x0, x1, 8)
    for t in xt:
        x = fx(t)
        out.append(f'<line x1="{x:.1f}" x2="{x:.1f}" y1="{mt}" y2="{H - mb}" stroke="{GRID}"/>')
        out.append(f'<text x="{x:.1f}" y="{H - mb + 18}" text-anchor="middle" class="tick">{fmt_x.format(t)}</text>')
    out.append(f'<text x="{(ml + W - mr) / 2}" y="{H - 8}" text-anchor="middle" class="axl">{html.escape(xlab)}</text>')
    out.append(f'<text x="16" y="{(mt + H - mb) / 2}" text-anchor="middle" class="axl" '
               f'transform="rotate(-90 16 {(mt + H - mb) / 2})">{html.escape(ylab)}</text>')
    # lines
    for s in series:
        pts = ' '.join(f'{fx(x):.1f},{fy(min(max(y, y0), y1)):.1f}' for x, y in zip(s['x'], s['y']))
        out.append(f'<polyline points="{pts}" fill="none" stroke="{s["color"]}" '
                   f'stroke-width="{s.get("width", 2)}" stroke-opacity="{s.get("opacity", 1)}" '
                   f'stroke-linejoin="round" stroke-linecap="round"/>')
    out.append(f'<line class="xh" x1="0" x2="0" y1="{mt}" y2="{H - mb}" stroke="{INK2}" '
               f'stroke-dasharray="3 3" visibility="hidden"/>')
    out.append(f'<rect x="{ml}" y="{mt}" width="{W - ml - mr}" height="{H - mt - mb}" '
               f'fill="transparent" class="hit"/>')
    out.append('</svg>')

    hov = [s for s in series if s.get('hover')]
    data = {
        'x0': x0, 'x1': x1, 'ml': ml, 'mr': mr, 'W': W, 'log': xlog,
        'x': [float(v) for v in hov[0]['x']],
        'series': [{'name': s['name'], 'color': s['color'], 'y': [float(v) for v in s['y']]} for s in hov],
        'ux': hover_unit[0], 'uy': hover_unit[1],
    }
    legend = ''.join(
        f'<span class="lg"><i style="background:{s["color"]};opacity:{s.get("opacity", 1)}"></i>'
        f'{html.escape(s["name"])}</span>' for s in series if s.get('legend', True))
    return (f'<section><h2>{html.escape(title)}</h2><div class="legend">{legend}</div>'
            f'<div class="wrap">{"".join(out)}<div class="tip" id="{cid}-tip"></div></div>'
            f'<script>hover("{cid}", {json.dumps(data)});</script></section>')


PAGE = """<!doctype html><html lang="en"><head><meta charset="utf-8">
<title>{title}</title><style>
body{{background:{surface};color:{ink};font:14px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif;margin:24px}}
h1{{font-size:18px;margin:0 0 4px}} h2{{font-size:15px;margin:22px 0 6px}}
.sub{{color:{ink2};margin-bottom:10px}} .tick{{font-size:11px;fill:{ink2}}} .axl{{font-size:12px;fill:{ink2}}}
.legend{{display:flex;gap:18px;color:{ink2};font-size:13px;margin:0 0 4px 70px}}
.lg i{{display:inline-block;width:18px;height:3px;border-radius:2px;margin-right:6px;vertical-align:middle}}
.wrap{{position:relative;width:900px}}
.tip{{position:absolute;pointer-events:none;background:#fff;border:1px solid #d6d5cf;border-radius:6px;
padding:6px 8px;font-size:12px;color:{ink};display:none;white-space:nowrap;box-shadow:0 2px 6px #0001}}
.tip b{{display:inline-block;width:10px;height:3px;border-radius:2px;margin-right:6px;vertical-align:middle}}
table{{border-collapse:collapse;font-size:12.5px;margin-top:6px}} td,th{{padding:3px 10px;text-align:right;border-bottom:1px solid {grid}}}
th{{color:{ink2};font-weight:600}} td:first-child,th:first-child{{text-align:left}}
</style><script>
function hover(id, d) {{
  const svg = document.getElementById(id), hit = svg.querySelector('.hit'),
        xh = svg.querySelector('.xh'), tip = document.getElementById(id + '-tip');
  const pw = d.W - d.ml - d.mr;
  const toX = v => d.log ? Math.pow(10, Math.log10(d.x0) + (v - d.ml) / pw * (Math.log10(d.x1) - Math.log10(d.x0)))
                         : d.x0 + (v - d.ml) / pw * (d.x1 - d.x0);
  hit.addEventListener('mousemove', e => {{
    const r = svg.getBoundingClientRect(), px = (e.clientX - r.left) * d.W / r.width, xv = toX(px);
    let k = 0, best = Infinity;
    d.x.forEach((v, i) => {{ const e2 = Math.abs(v - xv); if (e2 < best) {{ best = e2; k = i; }} }});
    const xp = d.log ? d.ml + (Math.log10(d.x[k]) - Math.log10(d.x0)) / (Math.log10(d.x1) - Math.log10(d.x0)) * pw
                     : d.ml + (d.x[k] - d.x0) / (d.x1 - d.x0) * pw;
    xh.setAttribute('x1', xp); xh.setAttribute('x2', xp); xh.setAttribute('visibility', 'visible');
    tip.innerHTML = '<div>' + d.x[k].toFixed(d.log ? 1 : 3) + ' ' + d.ux + '</div>' + d.series.map(s =>
      '<div><b style="background:' + s.color + '"></b>' + s.name + ': ' + s.y[k].toFixed(2) + ' ' + d.uy + '</div>').join('');
    tip.style.display = 'block';
    tip.style.left = Math.min(xp * r.width / d.W + 12, r.width - 230) + 'px';
    tip.style.top = '20px';
  }});
  hit.addEventListener('mouseleave', () => {{ tip.style.display = 'none'; xh.setAttribute('visibility', 'hidden'); }});
}}
</script></head><body>
<h1>{title}</h1><div class="sub">{sub}</div>
{body}
</body></html>"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', default=PORT)
    ap.add_argument('--snaps', type=int, default=4, help='raw periods to grab (default 4)')
    ap.add_argument('--avg', type=int, default=50, help='periods for the averaged read (default 50)')
    ap.add_argument('--pre', action='append', default=[], help='command to send first, e.g. "freq 62.5" (repeat)')
    ap.add_argument('--out', default=None, help='output directory')
    a = ap.parse_args()

    out = a.out or time.strftime('loop_snap_%Y%m%d_%H%M%S')
    os.makedirs(out, exist_ok=True)
    b = Board(a.port)
    for c in a.pre:
        print('>', c, '\n ', '\n  '.join(b.cmd(c, ('@',))))
    stat = b.cmd('stat', ('@T',))[-1]
    cal = field(stat, 'vrefcal')

    snaps = []
    for k in range(a.snaps):
        b.cmd('read 1', ('@E read',))
        cols, x = parse_dump(b.cmd('dump raw', ('@W end',)))
        snaps.append(x)
        np.savetxt(os.path.join(out, f'snap_{k}.csv'), x, fmt='%d', delimiter=',',
                   header=','.join(cols), comments='')
        print(f'snapshot {k + 1}/{a.snaps}: {len(x)} samples')
    rep = b.cmd(f'read {a.avg}', ('@E read',), timeout=a.avg * 0.03 + 30)
    cols, avg = parse_dump(b.cmd('dump avg', ('@W end',)))
    np.savetxt(os.path.join(out, 'avg.csv'), avg, fmt='%.3f', delimiter=',',
               header=','.join(cols), comments='')
    open(os.path.join(out, 'report.txt'), 'w').write('\n'.join(rep) + '\n')

    r = next(l for l in rep if l.startswith('@R'))
    f0, pga = field(r, 'f0'), field(r, 'pga', str)
    g_line = next((l for l in rep if l.startswith('@G')), None)
    G = field(g_line, 'gain') if g_line else None
    ci = {c: j for j, c in enumerate(cols)}
    vdda = 3.0 * cal / avg[:, ci['vref']].mean()
    lsb = vdda / 4096.0
    n = len(avg)
    fs = f0 * n
    t_ms = np.arange(n) / fs * 1e3

    def volts(codes, ch):
        v = (codes - codes.mean()) * lsb
        return v / G if ch == 'p' else v

    paths = [('i', 'direct ADC (D12)', C_DIR)]
    if G:
        paths.insert(0, ('p', f'PGA path (A3 / {G:.3f})', C_PGA))

    # chart 1: one period
    s1 = []
    for ch, name, color in paths:
        for k, x in enumerate(snaps):
            s1.append(dict(name=f'{name}, raw', color=color, x=t_ms, y=volts(x[:, ci[ch]], ch) * 1e3,
                           width=1.2, opacity=0.35, legend=(k == 0)))
    for ch, name, color in paths:
        s1.append(dict(name=f'{name}, avg of {a.avg}', color=color, x=t_ms,
                       y=volts(avg[:, ci[ch]], ch) * 1e3, width=2, hover=True))

    # chart 2: spectrum, power averaged over the raw snapshots, bins at k * f0
    s2 = []
    for ch, name, color in paths:
        p = np.mean([np.abs(np.fft.rfft(volts(x[:, ci[ch]], ch)) / n * 2) ** 2 / 2 for x in snaps], axis=0)
        f = np.fft.rfftfreq(n, 1 / fs)
        db = 10 * np.log10(np.maximum(p[1:], 1e-14) / 1e-6)   # dB re 1 mV rms
        s2.append(dict(name=name, color=color, x=f[1:], y=db, width=1.6, hover=True))
    lo = min(float(np.percentile(s['y'], 2)) for s in s2) - 5

    rows = []
    for l in rep:
        if l.startswith('@C'):
            ch = l.split()[1]
            if ch not in ('p', 'i'):
                continue
            sc = (1e3 / G) if (ch == 'p' and G) else 1e3
            rows.append('<tr><td>{}</td>{}</tr>'.format(
                'PGA path (scaled to D12)' if ch == 'p' else 'direct ADC (D12)',
                ''.join(f'<td>{field(l, k) * sc:.3f}</td>' for k in ('rms', 'rmsh', 'a1'))
                + ''.join(f'<td>{field(l, k):.3f}</td>' for k in ('thd',))
                + f'<td>{field(l, "nse") * sc / 1e6:.3f}</td>'))
    table = ('<h2>Averaged read</h2><table><tr><th>path</th><th>rms mV</th><th>rmsh mV</th>'
             '<th>a1 mV</th><th>thd %</th><th>noise mV</th></tr>' + ''.join(rows) + '</table>'
             '<div class="sub">rms: all of the signal. rmsh: the part coherent with the source '
             '(harmonics 1..20). Data: snap_*.csv, avg.csv, report.txt.</div>')

    body = (svg_chart('wave', f'One period at D12 ({a.snaps} raw periods and the average of {a.avg})',
                      'time in the period (ms)', 'mV at D12, DC removed', s1, fmt_x='{:.0f}',
                      fmt_y='{:.0f}', hover_unit=('ms', 'mV'))
            + svg_chart('spec', f'Spectrum at D12 (power averaged over {a.snaps} raw periods, bins at k x f0)',
                        'frequency (Hz, log)', 'dB re 1 mV rms', s2, xlog=True,
                        ylim=(lo, max(float(s['y'].max()) for s in s2)), fmt_x='{:g}',
                        fmt_y='{:.0f}', hover_unit=('Hz', 'dB'))
            + table)
    sub = (f'f0 {f0:.5f} Hz, {n} samples per period, fs {fs:.0f} Hz, PGA {pga}'
           + (f' (measured gain {G:.4f})' if G else '') + f', VDDA {vdda:.4f} V, '
           + time.strftime('%Y-%m-%d %H:%M:%S'))
    page = PAGE.format(title='LoopSource ADC snapshots', sub=html.escape(sub), body=body,
                       surface=SURFACE, ink=INK, ink2=INK2, grid=GRID)
    path = os.path.join(out, 'loop_snap.html')
    open(path, 'w').write(page)
    print('wrote', os.path.abspath(path))


if __name__ == '__main__':
    sys.exit(main())
