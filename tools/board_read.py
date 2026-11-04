#!/usr/bin/env python3
"""Read the DUT's raw ADC buffer through the demo shell and show the start of the scan.

    board_read.py --n 5 --out reads.csv          # opto as in a normal read
    board_read.py --n 5 --hold --out reads.csv   # stage1 and optoHP on before each read

Sends only: pin, sample hp, print adc. No drive: the DAQ is not touched here.
"""
import argparse
import csv
import re
import sys
import time

import numpy as np
import serial

PORT = '/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF545771485067013556-if01'
ROW = re.compile(r'^\s*(\d+)\s+(-?\d+) \(\s*-?\d+\)\s+(-?\d+) \(\s*-?\d+\)\s+(-?\d+) \(\s*-?\d+\)\s+(-?\d+) \(\s*-?\d+\)\s+(-?\d+) \(\s*-?\d+\)')
HEAD = 12   # samples at the start of the scan that are shown apart from the rest


class Shell:
    def __init__(self):
        self.s = serial.Serial(PORT, 115200, timeout=0.1)
        time.sleep(0.2)
        self.s.reset_input_buffer()

    def cmd(self, c, until, timeout=8.0):
        self.s.write((c + '\r\n').encode())
        out = ''
        t0 = time.time()
        while time.time() - t0 < timeout:
            out += self.s.read(8192).decode(errors='replace')
            if 'err:' in out:
                raise RuntimeError(f'{c}: {out[-200:]}')
            if until in out:
                time.sleep(0.1)
                return out + self.s.read(8192).decode(errors='replace')
        raise RuntimeError(f'{c}: no answer: {out[-200:]}')


def read_once(sh):
    """One `sample hp`, then the raw buffer. n x (raw1, raw2, diff, e1, e2) and the shell's values."""
    text = sh.cmd('sample hp', 'P28 DPF')
    m = re.search(r'current raw \(A\)\s*\|\s*([-\d.]+)', text)
    r = re.search(r'rmsRaw \(counts\)\s*\|\s*([-\d.]+)', text)
    time.sleep(0.05)                     # the rest of the DPF line
    text += sh.s.read(256).decode(errors='replace')
    p = re.search(r'P28 DPF\s*\|\s*([-\d.]+)', text)
    dump = sh.cmd('print adc', 'END ADC Raw')
    rows = [list(map(int, x.groups())) for x in map(ROW.match, dump.splitlines()) if x]
    return np.array(rows), float(m.group(1)), float(r.group(1)), float(p.group(1)) if p else float('nan')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--n', type=int, default=5)
    ap.add_argument('--hold', action='store_true')
    ap.add_argument('--gap', type=float, default=0.3, help='s between reads')
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    sh = Shell()
    with open(args.out, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['read', 'n', 'raw1', 'raw2', 'diff', 'e1', 'e2'])
        print(f'read  current_A  rmsRaw  head_peak  rest_peak  rest_rms  samples  (head = first {HEAD})')
        try:
            for i in range(args.n):
                if args.hold:
                    sh.cmd('pin stage1 on', 'ok: pin')
                    sh.cmd('pin optohp on', 'ok: pin')
                    time.sleep(0.2)
                rows, amps, rms, _ = read_once(sh)
                d = rows[:, 3] - rows[:, 3].mean()
                print(f'{i:4d} {amps:10.4f} {rms:7.2f} {np.abs(d[:HEAD]).max():10.1f} '
                      f'{np.abs(d[HEAD:]).max():10.1f} {d[HEAD:].std():9.2f} {len(rows):8d}', flush=True)
                for r in rows:
                    w.writerow([i] + list(r))
                time.sleep(args.gap)
        finally:
            if args.hold:
                sh.cmd('pin optohp off', 'ok: pin')
                sh.cmd('pin stage1 off', 'ok: pin')
    print(f'wrote {args.out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
