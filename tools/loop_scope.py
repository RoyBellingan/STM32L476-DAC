#!/usr/bin/env python3
"""Compare the scope with the DAQ on the same cycles of the loop source.

    python3 tools/loop_scope.py --run 1b --amps 0.02,0.05,0.1,0.15,0.2,0.3,0.5,0.75,1.0
    python3 tools/loop_scope.py --run 0 --amps 0
    python3 tools/loop_scope.py --run 6a --hold-hp --amps 0.05,0.2,1.0    # DUT on TX_R6_demo.elf
    python3 tools/loop_scope.py --run 6b --hold-hp --sample --amps 0.05,0.2,1.0
    python3 tools/loop_scope.py --run 6c --during --amps 0.05,0.2,1.0
    python3 tools/loop_scope.py --selftest

Plan: digitalSpine todo/fluxStationDaqScopeTest.md. Close cutecom and the station first.

For each amplitude: `amp`, fit the scope scales, `:SING`, `read`. D8 rises at the first captured
scan and falls at the last one, so the D8-high window of the record holds the cycles of the `read`.
scope_download (Rigol-DSO900-Code) pulls the waveforms. Each channel is then cut to that window.

The step: a signal x = A sin(u) - d sign(sin(u)) has a fixed step at each zero crossing. `step` is d,
fitted on the odd harmonics 3 to 19 with the phase of the fundamental of that channel. `share` is
the part of the harmonic power that the step explains. A channel with no step has d near 0.

--hold-hp: optoHP stays on (shell `pin`), so the clamp has its HP burden during the reads. --sample: `sample hp`
after each capture: the board's current on the same load. --during: one 2 s record with a `sample hp` inside it and
the opto as in a normal read; the envelope of the current before and during the read goes to envelope.csv.

Writes into --out: run_<run>.csv, one row per capture. Raw data: Rigol-DSO900-Code/aq_loop_<date>_<run>/
(ignored by git), with one averaged period per capture in period.csv.
"""
import argparse
import csv
import json
import os
import re
import socket
import subprocess
import sys
import tempfile
import time

import numpy as np
import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from loop_snap import PORT, Board, field  # noqa: E402

MAX_AMP = 1.0                # at 1.1 V a spike on D12 reaches 0 V (2026-11-02)
SCOPE_TOOL = '/home/roy/Public/Rigol-DSO900-Code/scope_download'
RAW_ROOT = '/home/roy/Public/Rigol-DSO900-Code'
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MIN_SCALE = 1e-3             # 200 and 500 uV/div are only a magnification of 1 mV/div
D8_LEVEL = 1.65
KMAX = 20                    # as kHarm in the firmware
DUT_PORT = '/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF545771485067013556-if01'


# --------------------------------------------------------------------------
# analysis
# --------------------------------------------------------------------------

def harmonics(x, dt, f0):
    """DC and c[k] = a_k e^(j phi_k) for x = DC + sum a_k sin(k w t + phi_k), t = 0 at x[0]."""
    t = np.arange(len(x)) * dt
    dc = float(x.mean())
    y = x - dc
    c = np.zeros(KMAX + 1, complex)
    for k in range(1, KMAX + 1):
        c[k] = 2.0 / len(y) * np.sum(y * np.exp(-1j * (2 * np.pi * k * f0 * t - np.pi / 2)))
    return dc, c


def step_fit(c):
    """(d, share) for a step in phase with the fundamental. d > 0 lowers the fundamental."""
    phi = np.angle(c[1])
    ks = np.arange(3, KMAX + 1, 2)
    s = (4 / np.pi) / ks * np.exp(1j * ks * phi)
    h = c[ks]
    d = -np.real(np.sum(np.conj(s) * h)) / np.sum(np.abs(s) ** 2)
    total = np.sum(np.abs(c[2:]) ** 2)
    left = np.sum(np.abs(h + d * s) ** 2) + np.sum(np.abs(c[2::2]) ** 2)
    return float(d), float(1 - left / total) if total > 0 else 0.0


def channel_stats(x, dt, f0):
    dc, c = harmonics(x, dt, f0)
    a1 = abs(c[1])
    d, share = step_fit(c)
    return {
        'dc': dc,
        'rms': float(np.std(x)),
        'a1': a1 / np.sqrt(2),
        'ph': float(np.degrees(np.angle(c[1]))),
        'thd': float(np.sqrt(np.sum(np.abs(c[2:]) ** 2)) / a1 * 100) if a1 > 0 else 0.0,
        'even': float(np.sqrt(np.sum(np.abs(c[2::2]) ** 2)) / a1 * 100) if a1 > 0 else 0.0,
        'step': d,
        'share': share,
        'c1': complex(c[1]),
    }


def load_channel(folder, ch):
    with open(os.path.join(folder, f'_CHAN{ch}.json')) as f:
        meta = json.load(f)
    raw = np.fromfile(os.path.join(folder, f'_CHAN{ch}.bin'), dtype='<i2')
    return raw * meta['scale'] + meta['offset'], meta['dt']


def d8_window(v, dt):
    """(first, last) sample of the D8-high window. Pulses under 1 ms are not D8."""
    hi = (v > D8_LEVEL).astype(np.int8)
    d = np.diff(hi)
    rises = np.flatnonzero(d == 1) + 1
    falls = np.flatnonzero(d == -1) + 1
    for r in rises:
        after = falls[falls > r]
        if len(after) and (after[0] - r) * dt > 1e-3:
            return int(r), int(after[0])
    raise RuntimeError('D8: no full pulse in the record (no trigger, or the capture is longer than the record)')


def analyse_capture(folder, chans, trig, f0, periods, write_period=True):
    """Cut every channel to whole periods from the rise of D8."""
    vt, dt = load_channel(folder, trig)
    first, last = d8_window(vt, dt)
    n = int(round(periods / f0 / dt))
    if first + n > len(vt):
        raise RuntimeError('D8: the record ends before the last captured period')
    out = {'d8_ms': (last - first) * dt * 1e3}
    bins = 1000
    idx = (np.floor((np.arange(n) * dt * f0 % 1.0) * bins)).astype(int)
    counts = np.maximum(np.bincount(idx, minlength=bins), 1)
    period = {}
    for ch, label in chans.items():
        v, _ = load_channel(folder, ch)
        x = v[first:first + n]
        out[label] = channel_stats(x, dt, f0)
        period[label] = np.bincount(idx, weights=x, minlength=bins) / counts
    if write_period:
        with open(os.path.join(folder, 'period.csv'), 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['phase_deg'] + [f'{k}_V' for k in period])
            for i in range(bins):
                w.writerow([f'{i * 360.0 / bins:.2f}'] + [f'{period[k][i]:.7f}' for k in period])
    return out


def envelope(x, dt, f0, steps=8):
    """(t, rms of the fundamental over one period), a value each 1 / steps of a period."""
    n = int(round(1 / f0 / dt))
    t = np.arange(len(x)) * dt
    z = (x - x.mean()) * np.exp(-2j * np.pi * f0 * t)
    cs = np.concatenate(([0], np.cumsum(z)))
    hop = max(n // steps, 1)
    starts = np.arange(0, len(x) - n, hop)
    env = np.abs(cs[starts + n] - cs[starts]) * 2 / n / np.sqrt(2)
    return t[starts] + n * dt / 2, env


def read_step(t, env):
    """(before, during, width in s) for one step up of the envelope."""
    before = float(np.median(env))
    top = float(np.percentile(env, 99.9))
    if top < before * 1.03:
        return before, before, 0.0
    on = np.flatnonzero(env > (before + top) / 2)
    width = float(t[on[-1]] - t[on[0]])
    return before, float(np.median(env[on])), width


# --------------------------------------------------------------------------
# scope
# --------------------------------------------------------------------------

class Scope:
    def __init__(self, ip, port=5555):
        self.s = socket.create_connection((ip, port), timeout=5)

    def w(self, c):
        self.s.sendall((c + '\n').encode())
        time.sleep(0.03)

    def q(self, c):
        self.s.sendall((c + '\n').encode())
        b = b''
        while not b.endswith(b'\n'):
            b += self.s.recv(4096)
        return b.decode(errors='replace').strip()

    def close(self):
        self.s.close()

    def errors(self, where, fail=True):
        got = []
        for _ in range(20):
            e = self.q(':SYST:ERR?')
            if e.startswith('0'):
                break
            got.append(e)
        if got and fail:
            raise RuntimeError(f'scope, {where}: {got}')

    def meas(self, item, ch):
        v = float(self.q(f':MEAS:ITEM? {item},CHAN{ch}'))
        return None if abs(v) > 1e30 else v

    def wait_status(self, want, timeout):
        t0 = time.time()
        while time.time() - t0 < timeout:
            if self.q(':TRIG:STAT?') == want:
                return True
            time.sleep(0.05)
        return False


def grid_up(v):
    """The smallest 1-2-5 scale at or above v."""
    for e in range(-3, 2):
        for m in (1, 2, 5):
            s = m * 10.0 ** e
            if s >= v * 0.999:
                return s
    return 10.0


def setup_scope(sc, chans, trig):
    sc.errors('start', fail=False)
    for ch in chans:
        sc.w(f':CHAN{ch}:DISP 1')
        sc.w(f':CHAN{ch}:COUP DC')
    sc.w(f':CHAN{trig}:DISP 1')
    sc.w(f':CHAN{trig}:SCAL 1')
    sc.w(f':CHAN{trig}:OFFS -1.65')
    sc.w(':TRIG:MODE EDGE')
    sc.w(f':TRIG:EDGE:SOUR CHAN{trig}')
    sc.w(':TRIG:EDGE:SLOP POS')
    sc.w(f':TRIG:EDGE:LEV {D8_LEVEL}')
    sc.w(':TIM:MAIN:SCAL 0.02')
    sc.w(':TIM:MAIN:OFFS 0.05')      # the record is -150 .. +250 ms from the rise of D8
    sc.errors('setup')


def fit_scales(sc, chans):
    """Free run, then each V/div so that VPP fills 30 to 75 % of the 8 divisions. CH offset = -DC."""
    sc.w(':TRIG:SWE AUTO')
    sc.w(':RUN')
    probe = {ch: float(sc.q(f':CHAN{ch}:PROB?')) for ch in chans}
    open_ch = set(chans)
    for _ in range(6):
        time.sleep(0.7)
        for ch in sorted(open_ch):
            cur = float(sc.q(f':CHAN{ch}:SCAL?'))
            vpp = sc.meas('VPP', ch)
            avg = sc.meas('VAVG', ch)
            if vpp is None or vpp > 7.5 * cur:
                want = grid_up(min(cur * 2.5, 10.0 * probe[ch]) / probe[ch]) * probe[ch]
            else:
                want = grid_up(max(vpp / 6.0, MIN_SCALE * probe[ch]) / probe[ch]) * probe[ch]
            if avg is not None:
                sc.w(f':CHAN{ch}:OFFS {-avg:.6f}')
            if abs(want - cur) > 1e-9:
                sc.w(f':CHAN{ch}:SCAL {want:g}')
            else:
                open_ch.discard(ch)
        if not open_ch:
            break
    sc.errors('scales')


def scale_up(sc, ch):
    cur = float(sc.q(f':CHAN{ch}:SCAL?'))
    probe = float(sc.q(f':CHAN{ch}:PROB?'))
    sc.w(f':CHAN{ch}:SCAL {grid_up(cur * 1.5 / probe) * probe:g}')


def download(ip, chans, trig, rawdir, tag):
    ids = ','.join(str(c) for c in sorted(set(chans) | {trig}))
    p = subprocess.run([SCOPE_TOOL, '--ip', ip, '--channels', ids, '--no-plots', '--no-analysis',
                        '--no-aligned', '--out-dir-prefix', tag + '_'],
                       cwd=rawdir, capture_output=True, text=True, timeout=180)
    if p.returncode == 2:
        raise RuntimeError(f'scope_download: {p.stderr.strip()}')
    m = re.search(r'Output directory: "?([^"\n]+)"?', p.stdout)
    if not m:
        raise RuntimeError(f'scope_download: no output folder in: {p.stdout[-300:]}')
    return m.group(1), p.returncode == 1


# --------------------------------------------------------------------------
# DAQ
# --------------------------------------------------------------------------

def port_holders(port):
    real = os.path.realpath(port)
    out = []
    for pid in filter(str.isdigit, os.listdir('/proc')):
        try:
            for fd in os.listdir(f'/proc/{pid}/fd'):
                if os.readlink(f'/proc/{pid}/fd/{fd}') == real:
                    out.append(pid)
                    break
        except OSError:
            continue
    return out


def set_amp(daq, amp):
    if not 0.0 <= amp <= MAX_AMP:
        raise ValueError(f'amp {amp} is outside 0 .. {MAX_AMP} V')
    daq.cmd('off' if amp == 0 else f'amp {amp:.5f}', ['@S'], timeout=5)


def drive_off(daq):
    for _ in range(3):
        try:
            if field(daq.cmd('off', ['@S'], timeout=3)[-1], 'amp') == 0.0:
                print('drive off')
                return
        except Exception as e:  # noqa: BLE001: the next try, then the warning
            print(f'off: {e}')
    print('WARNING: the DAQ did not confirm `off`. THE DRIVE CAN BE ON.')


def parse_read(lines):
    out = {'head': next(l for l in lines if l.startswith('@R'))}
    for l in lines:
        if l.startswith('@C '):
            out[l.split()[1]] = l
        elif l.startswith('@G'):
            out['gain'] = l
    return out


class Dut:
    """The shell of TX_R6_demo.elf on the DUT's ST-Link VCP."""

    def __init__(self, port):
        self.s = serial.Serial(port, 115200, timeout=0.1)
        time.sleep(0.2)
        self.s.reset_input_buffer()

    def cmd(self, c, until, timeout=3.0):
        self.s.reset_input_buffer()
        self.s.write((c + '\r\n').encode())
        t0 = time.time()
        out = ''
        while time.time() - t0 < timeout:
            out += self.s.read(4096).decode(errors='replace')
            if 'err:' in out:
                raise RuntimeError(f'DUT, {c}: {out[-200:]}')
            if until in out:
                # the rest of the lines of this reply
                time.sleep(0.15)
                return out + self.s.read(4096).decode(errors='replace')
        raise RuntimeError(f'DUT, {c}: no answer: {out[-200:]}')

    def hold_hp(self, on):
        word = 'on' if on else 'off'
        if on:
            self.cmd('pin stage1 on', 'ok: pin')
        self.cmd(f'pin optohp {word}', 'ok: pin')
        if not on:
            self.cmd('pin stage1 off', 'ok: pin')

    def sample_send(self):
        self.s.reset_input_buffer()
        self.s.write(b'sample hp\r\n')

    def sample_read(self, timeout=5.0):
        t0 = time.time()
        out = ''
        while time.time() - t0 < timeout:
            out += self.s.read(4096).decode(errors='replace')
            if 'P28 DPF' in out:
                break
        m = re.search(r'current raw \(A\)\s*\|\s*([-\d.]+)', out)
        if not m:
            raise RuntimeError(f'DUT, sample hp: no current in: {out[-200:]}')
        return float(m.group(1))


# --------------------------------------------------------------------------
# one capture
# --------------------------------------------------------------------------

def capture(args, daq, dut, amp, chans, trig, rawdir):
    t0 = time.time()
    set_amp(daq, amp)
    time.sleep(args.settle)
    sc = Scope(args.ip)
    try:
        sc.w(':TIM:MAIN:SCAL 0.02')
        sc.w(':TIM:MAIN:OFFS 0.05')
        fit_scales(sc, chans)
    finally:
        sc.close()
    for attempt in range(2):
        sc = Scope(args.ip)
        try:
            sc.w(':SING')
            if not sc.wait_status('WAIT', 5):
                raise RuntimeError('scope: not armed in 5 s')
            time.sleep(0.5)          # the part of the record before the trigger
            rd = parse_read(daq.cmd(f'read {args.periods}', ['@E read'], timeout=20))
            if not sc.wait_status('STOP', 5):
                raise RuntimeError('scope: no trigger in 5 s. Is D8 on the trigger channel?')
            scales = {ch: float(sc.q(f':CHAN{ch}:SCAL?')) for ch in chans}
        finally:
            sc.close()
        folder, clipped = download(args.ip, chans, trig, rawdir, f'a{amp:.4f}')
        if not clipped:
            break
        if attempt == 1:
            print('  clipped again: the row is marked')
            break
        sc = Scope(args.ip)
        try:
            for ch in chans:
                scale_up(sc, ch)
        finally:
            sc.close()
    f0 = field(rd['head'], 'f0')
    res = analyse_capture(folder, chans, trig, f0, field(rd['head'], 'n', int))
    row = {'run': args.run, 'note': args.note, 'time': time.strftime('%H:%M:%S'), 'folder': os.path.basename(folder),
           'f0': f0, 'amp_cmd': amp, 'amp_read': field(rd['head'], 'amp'), 'vdda': field(rd['head'], 'vdda'),
           'daq_st': field(rd['head'], 'st', str), 'clipped': int(clipped), 'd8_ms': round(res['d8_ms'], 3),
           'capture_s': round(time.time() - t0, 1)}
    for name in ('i', 'p'):
        line = rd.get(name)
        if line:
            for key, unit in (('a1', 1e3), ('rmsh', 1e3), ('dc', 1e3), ('ph', 1), ('thd', 1)):
                row[f'daq_{name}_{key}'] = round(field(line, key) * unit, 4)
    if 'gain' in rd:
        row['daq_pga_gain'] = field(rd['gain'], 'gain')
    for ch, label in chans.items():
        s = res[label]
        row.update({f'{label}_a1_mV': round(s['a1'] * 1e3, 4), f'{label}_rms_mV': round(s['rms'] * 1e3, 4),
                    f'{label}_dc_mV': round(s['dc'] * 1e3, 3), f'{label}_ph_deg': round(s['ph'], 3),
                    f'{label}_thd_pct': round(s['thd'], 3), f'{label}_even_pct': round(s['even'], 3),
                    f'{label}_step_mV': round(s['step'] * 1e3, 4), f'{label}_step_share': round(s['share'], 3),
                    f'{label}_vdiv': scales[ch]})
    if 'amp' in res and 'rs' in res and abs(res['rs']['c1']) > 0:
        z = (res['amp']['c1'] - res['rs']['c1']) / (res['rs']['c1'] / args.rs)
        row['loop_R_ohm'] = round(z.real, 3)
        row['loop_X_ohm'] = round(z.imag, 3)
    if dut is not None:
        dut.sample_send()
        board = dut.sample_read()
        row['board_A'] = board
        if args.hold_hp:
            dut.hold_hp(True)        # the read ends with optoHP off
        if 'rs' in res and res['rs']['a1'] > 0:
            row['N_x_gain'] = round(board / res['rs']['a1'], 2)
    return row


def capture_during(args, daq, dut, amp, chans, trig, rawdir):
    """One 2 s record with a board read in it. The opto is as in a normal read."""
    t0 = time.time()
    set_amp(daq, amp)
    time.sleep(args.settle)
    sc = Scope(args.ip)
    try:
        sc.w(':TIM:MAIN:SCAL 0.02')
        sc.w(':TIM:MAIN:OFFS 0.05')
        fit_scales(sc, chans)
        # room for the step: it can be larger than the swing the scales were fitted on
        for ch in chans:
            scale_up(sc, ch)
        sc.w(':TIM:MAIN:SCAL 0.1')
        sc.w(':TIM:MAIN:OFFS 0.8')   # the record is -0.2 .. +1.8 s from the rise of D8
        sc.w(':SING')
        if not sc.wait_status('WAIT', 5):
            raise RuntimeError('scope: not armed in 5 s')
        time.sleep(0.7)
        rd = parse_read(daq.cmd('read 2', ['@E read'], timeout=20))
        dut.sample_send()
        board = dut.sample_read()
        if not sc.wait_status('STOP', 6):
            raise RuntimeError('scope: no trigger in 6 s')
        sc.w(':TIM:MAIN:SCAL 0.02')
    finally:
        sc.close()
    folder, clipped = download(args.ip, chans, trig, rawdir, f'a{amp:.4f}')
    f0 = field(rd['head'], 'f0')
    row = {'run': args.run, 'note': args.note, 'time': time.strftime('%H:%M:%S'), 'folder': os.path.basename(folder),
           'f0': f0, 'amp_cmd': amp, 'clipped': int(clipped), 'board_A': board, 'capture_s': round(time.time() - t0, 1)}
    series = {}
    for ch, label in chans.items():
        v, dt = load_channel(folder, ch)
        t, env = envelope(v, dt, f0)
        before, during, width = read_step(t, env)
        row.update({f'{label}_before_mV': round(before * 1e3, 3), f'{label}_during_mV': round(during * 1e3, 3),
                    f'{label}_width_ms': round(width * 1e3, 1)})
        series[label] = env
    with open(os.path.join(folder, 'envelope.csv'), 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['t_s'] + [f'{k}_Vrms' for k in series])
        for i, ti in enumerate(t):
            w.writerow([f'{ti:.5f}'] + [f'{series[k][i]:.6f}' for k in series])
    if row.get('rs_during_mV'):
        row['N_x_gain'] = round(board / (row['rs_during_mV'] * 1e-3), 2)
    return row


def show_during(row, labels):
    parts = [f"{l} {row[l + '_before_mV']:8.3f} -> {row[l + '_during_mV']:8.3f} mV, {row[l + '_width_ms']:6.1f} ms" for l in labels]
    print(f"amp {row['amp_cmd']:.3f} | " + ' | '.join(parts) + f" | board {row['board_A']:.4f} A, N x gain {row.get('N_x_gain', float('nan')):.1f}"
          f" {row['capture_s']:.0f} s" + (' CLIPPED' if row['clipped'] else ''), flush=True)


def show(row, labels):
    daq = f"DAQ i {row.get('daq_i_a1', float('nan')):8.3f} mV thd {row.get('daq_i_thd', float('nan')):5.1f}"
    parts = [f"{l} {row[l + '_a1_mV']:8.3f} mV thd {row[l + '_thd_pct']:5.1f} step {row[l + '_step_mV']:7.3f}"
             for l in labels]
    z = f" Z {row['loop_R_ohm']:.2f}{row['loop_X_ohm']:+.2f}j" if 'loop_R_ohm' in row else ''
    if 'board_A' in row:
        z += f" board {row['board_A']:.4f} A, N x gain {row.get('N_x_gain', float('nan')):.1f}"
    print(f"amp {row['amp_cmd']:.3f} | {daq} | " + ' | '.join(parts) + f" |{z} d8 {row['d8_ms']:.1f} ms "
          f"{row['capture_s']:.0f} s" + (' CLIPPED' if row['clipped'] else ''), flush=True)


def write_csv(path, rows):
    keys = []
    for r in rows:
        keys += [k for k in r if k not in keys]
    with open(path, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        w.writerows(rows)


# --------------------------------------------------------------------------
# self test: synthetic records, no hardware
# --------------------------------------------------------------------------

def selftest():
    f0, dt, periods, rs = 62.50306, 400e-9, 10, 1.0
    t = np.arange(1_000_000) * dt
    start = 150e-3                       # the rise of D8
    tc = t - start
    u = 2 * np.pi * f0 * tc
    rng = np.random.default_rng(1)
    pk, d = 0.100, 0.0136
    noise = 0.002
    sig = {
        1: 0.2 + pk * np.sin(u + 0.3) - d * np.sign(np.sin(u + 0.3)) + rng.normal(0, noise, t.size),  # D12: step
        2: 1.0 * np.sin(u + 0.5) + rng.normal(0, noise, t.size),                                      # amp: clean
        3: np.where((tc >= 0) & (tc < periods / f0), 3.3, 0.0),                                       # D8
        4: pk * np.sin(u + 0.3) + rng.normal(0, noise, t.size),                                       # Rs: clean
    }
    with tempfile.TemporaryDirectory() as tmp:
        for ch, v in sig.items():
            scale = 10.0 / 65536
            raw = np.round(v / scale).astype('<i2')
            raw.tofile(os.path.join(tmp, f'_CHAN{ch}.bin'))
            with open(os.path.join(tmp, f'_CHAN{ch}.json'), 'w') as f:
                json.dump({'t0': 0.0, 'dt': dt, 'scale': scale, 'offset': 0.0}, f)
        chans = {1: 'd12', 2: 'amp', 4: 'rs'}
        res = analyse_capture(tmp, chans, 3, f0, periods)
        assert os.path.getsize(os.path.join(tmp, 'period.csv')) > 1000
    want_a1 = (pk - 4 * d / np.pi) / np.sqrt(2)
    checks = [
        ('D8 width, ms', res['d8_ms'], periods / f0 * 1e3, 0.01),
        ('D12 DC, V', res['d12']['dc'], 0.2, 1e-3),
        ('D12 fundamental, V rms', res['d12']['a1'], want_a1, 1e-4),
        ('D12 step, V', res['d12']['step'], d, 2e-4),
        ('D12 step share', res['d12']['share'], 1.0, 0.02),
        ('Rs step, V', res['rs']['step'], 0.0, 2e-4),
        ('Rs fundamental, V rms', res['rs']['a1'], pk / np.sqrt(2), 1e-4),
        ('Rs phase, deg', res['rs']['ph'], np.degrees(0.3), 0.1),
        ('Amp fundamental, V rms', res['amp']['a1'], 1.0 / np.sqrt(2), 1e-3),
        ('Amp thd, %', res['amp']['thd'], 0.0, 0.1),
    ]
    z = (res['amp']['c1'] - res['rs']['c1']) / (res['rs']['c1'] / rs)
    want_z = (1.0 * np.exp(0.5j) - pk * np.exp(0.3j)) / (pk * np.exp(0.3j) / rs)
    checks += [('Z R, ohm', z.real, want_z.real, 0.05), ('Z X, ohm', z.imag, want_z.imag, 0.05)]
    tt = np.arange(250_000) * 2e-6
    u = 2 * np.pi * f0 * tt
    amp_env = np.where((tt > 0.2) & (tt < 0.285), 1.3, 1.0)
    et, ee = envelope(amp_env * 0.1 * np.sin(u), 2e-6, f0)
    before, during, width = read_step(et, ee)
    checks += [('Envelope before, V rms', before, 0.1 / np.sqrt(2), 1e-3),
               ('Envelope during, V rms', during, 0.13 / np.sqrt(2), 3e-3),
               ('Envelope width, s', width, 0.085, 0.012)]
    bad = 0
    for name, got, want, tol in checks:
        ok = abs(got - want) <= tol
        bad += not ok
        print(f"{'ok ' if ok else 'BAD'} {name}: {got:.5f} (want {want:.5f} +- {tol})")
    print(f'selftest: {len(checks) - bad} of {len(checks)} ok')
    return 1 if bad else 0


# --------------------------------------------------------------------------

def parse_chans(items):
    chans, trig = {}, None
    for it in items:
        ch, label = it.split('=')
        if label == 'trig':
            trig = int(ch)
        else:
            chans[int(ch)] = label
    if trig is None:
        raise SystemExit('--ch: one channel must be trig')
    return chans, trig


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--selftest', action='store_true')
    ap.add_argument('--run', default='x', help='name of the run, for the files')
    ap.add_argument('--note', default='', help='the state of the bench, kept in each row')
    ap.add_argument('--amps', default='0', help=f'V peak, comma separated, at most {MAX_AMP}')
    ap.add_argument('--freq', type=float, default=62.5)
    ap.add_argument('--periods', type=int, default=10)
    ap.add_argument('--rs', type=float, default=1.0)
    ap.add_argument('--settle', type=float, default=1.0, help='s after `amp`: covers the 47 uF at D12')
    ap.add_argument('--hold-hp', action='store_true', help='optoHP on through the DUT shell, for the whole run')
    ap.add_argument('--sample', action='store_true', help='`sample hp` on the DUT after each capture')
    ap.add_argument('--during', action='store_true', help='one 2 s record with a board read inside it')
    ap.add_argument('--ip', default='192.168.0.116')
    ap.add_argument('--ch', nargs='+', default=['1=d12', '2=amp', '3=trig', '4=rs'])
    ap.add_argument('--out', default=os.path.join(HERE, 'docs/loop/scope_' + time.strftime('%Y-%m-%d')))
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    amps = [float(a) for a in args.amps.split(',')]
    if any(not 0.0 <= a <= MAX_AMP for a in amps):
        raise SystemExit(f'amps must be 0 .. {MAX_AMP} V')
    chans, trig = parse_chans(args.ch)
    holders = port_holders(PORT)
    if holders:
        raise SystemExit(f'the DAQ port is held by pid {holders}: close cutecom or the station')

    rawdir = os.path.join(RAW_ROOT, f"aq_loop_{time.strftime('%Y-%m-%d')}_{args.run}")
    os.makedirs(rawdir, exist_ok=True)
    os.makedirs(args.out, exist_ok=True)
    csv_path = os.path.join(args.out, f'run_{args.run}.csv')

    sc = Scope(args.ip)
    try:
        print(sc.q('*IDN?'))
        for ch in sorted(set(chans) | {trig}):
            print(f"CH{ch} probe x{float(sc.q(f':CHAN{ch}:PROB?')):g}, {chans.get(ch, 'trig')}")
        setup_scope(sc, chans, trig)
    finally:
        sc.close()

    dut = None
    if args.hold_hp or args.sample or args.during:
        if port_holders(DUT_PORT):
            raise SystemExit('the DUT port is held by another program')
        dut = Dut(DUT_PORT)
    daq = Board(PORT)
    rows = []
    try:
        daq.cmd(f'freq {args.freq:g}', ['@F'], timeout=5)
        daq.cmd('pga 8 ext', ['@S'], timeout=5)
        if args.hold_hp:
            dut.hold_hp(True)
        for amp in amps:
            if args.during:
                row = capture_during(args, daq, dut, amp, chans, trig, rawdir)
                show_during(row, list(chans.values()))
            else:
                row = capture(args, daq, dut if args.sample else None, amp, chans, trig, rawdir)
                show(row, list(chans.values()))
            rows.append(row)
            write_csv(csv_path, rows)
    finally:
        drive_off(daq)
        if args.hold_hp:
            dut.hold_hp(False)
        try:
            sc = Scope(args.ip)
            sc.w(':TRIG:SWE AUTO')
            sc.w(':RUN')
            sc.close()
        except OSError as e:
            print(f'scope: {e}')
    print(f'{len(rows)} rows: {csv_path}\nraw: {rawdir}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
